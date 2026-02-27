# Beast JSON — twitter.json Performance Overtake Development Plan

> **Date**: 2026-02-27
> **Current State**: beast::lazy 314μs vs yyjson 250μs (twitter.json benchmark)
> **Goal**: Match or surpass yyjson on twitter.json

---

## 1. Root Cause Diagnosis

Precise bottleneck analysis based on measured data:

| Characteristic | twitter.json | gsoc-2018.json | canada.json |
|:---|:---:|:---:|:---:|
| Whitespace ratio | **29.6%** (pretty-print) | 18.7% | 0% |
| Median string length | 15 bytes | 19 bytes | 7 bytes |
| Strings ≤16 bytes | **50.9%** | 29.9% | 75% |
| Avg keys per object | **10.6** | 5.0 | 2.0 |
| Avg nesting depth | 4.4 | 2.5 | 6.7 |
| Token type diversity | strings+numbers+bool+null+`{[` | strings only | numbers only |

**Key conclusion**: beast-json losing to yyjson is not due to a weak parser algorithm.
twitter.json's pretty-print format and high token diversity cause **extremely frequent `skip_to_action()` calls — a structural problem**.

- On gsoc-2018, canada, citm_catalog, beast::lazy dominates yyjson (3/4 wins)
- twitter.json's 167KB of whitespace (29.6% of total) causes explosive repeated `skip_to_action()` calls
- beast-json already has Stage1 bitmap infrastructure, but **it is not wired into the parse_reuse path** — the sole structural weakness

---

## 2. Overall Roadmap Summary

| Priority | Phase | Work | Expected Gain | Difficulty | Status |
|:---:|:---:|:---|:---:|:---:|:---:|
| 1 | A1 | Stage1 bitmap → parse_reuse integration | ~15-20% | High | ❌ Reverted — fill_bitmap() O(n) pre-scan costs more than it saves on 616KB file (regression observed) |
| 2 | A2 | 32-byte SWAR quad-pump whitespace skip | ~5-8% | Low | ✅ Done (x86-64 `#else` branch) |
| 3 | B1 | Value→Separator→Key 3-way fused scanner | ~5-7% | Medium | Pending |
| 4 | B2 | ≤8-byte inline string embedded in tape | ~10% (lookup/serialize) | Medium | Pending |
| 5 | C1 | next_sib-based multi-threaded split parsing | ~40%+ (multi-core) | Very High | Pending |
| Aux | C2 | TLAB direct parsing (eliminate tape pre-allocation) | ~2-3% | Medium | Pending |

---

## 3. Phase A — Immediate Impact (Bitmap-Based Redesign)

### A1. Integrate Stage1 Bitmap into parse_reuse Hot Path

**Goal**: Reduce `skip_to_action()` calls to zero when processing twitter.json's 167KB of whitespace.

**Current structural problem**:

```
Current beast-json has two completely separate paths:

beast::lazy (parse_reuse → Phase 19 Parser)
  → byte-by-byte scanning
  → repeated skip_to_action() calls
  → loop overhead for every 29.6% whitespace byte

Stage1 fill_bitmap (separate path)
  → indexes all structural positions in 64-byte blocks at once
  → tracks string interiors with SWAR prefix-XOR
  → collects structural character positions into a bitmap
  → currently NOT used by parse_reuse
```

**Target structure**:

```
Current: token → skip_ws → switch → token → skip_ws → switch (repeated)

Target:  [pre-scan: fill_bitmap → generate positions array]
          → [walk positions array: switch only, no skip_ws]
```

**Implementation steps**:

1. From `fill_bitmap()` output, generate compressed position array `uint32_t positions[]`
2. Modify Phase 19 Parser to iterate over positions array instead of raw bytes
3. Remove `skip_to_action()` call path (positions array already skips whitespace)
4. Allocate positions array from stack or TLAB (no heap allocation)

**Expected effect**: Eliminate `skip_to_action()` calls for twitter.json → **~15-20% speedup**

---

### A2. 32-byte SWAR Quad-Pump Whitespace Skip (No SIMD)

**Goal**: 4x throughput improvement in whitespace processing on fallback paths, before and after A1.

**Current code (8 bytes at a time)**:

```cpp
// Current (8 bytes at a time)
while (p_ + 8 <= end_) {
    uint64_t am = swar_action_mask(load64(p_));
    if (am) { p_ += CTZ(am) >> 3; return *p_; }
    p_ += 8;
}
```

**Target code (32 bytes, SWAR 4-pump)**:

```cpp
// Target (32 bytes at a time, SWAR 4-pump)
while (p_ + 32 <= end_) {
    uint64_t a0 = swar_action_mask(load64(p_));
    uint64_t a1 = swar_action_mask(load64(p_ + 8));
    uint64_t a2 = swar_action_mask(load64(p_ + 16));
    uint64_t a3 = swar_action_mask(load64(p_ + 24));
    if (a0 | a1 | a2 | a3) {
        // Find exact position of first hit
        if (a0) { p_ += CTZ(a0) >> 3; return *p_; }
        if (a1) { p_ += 8 + (CTZ(a1) >> 3); return *p_; }
        if (a2) { p_ += 16 + (CTZ(a2) >> 3); return *p_; }
        p_ += 24 + (CTZ(a3) >> 3); return *p_;
    }
    p_ += 32;
}
// Existing 8-byte fallback...
```

**Implementation notes**:
- OR-merge with `a0|a1|a2|a3` → check 32 bytes with a single branch
- Modern CPU out-of-order execution pipelines four `load64` calls in parallel
- Works on both x86-64 and ARM64 without SIMD

**Expected effect**: 4x throughput for pretty-print JSON whitespace → **~5-8% speedup** on twitter.json

---

## 4. Phase B — Medium Impact (Fused Scanner Extensions)

### B1. Value→Separator→Key 3-Way Fused Scanner

**Goal**: Eliminate repeated function call overhead for twitter.json objects with 10.6 key-value pairs each.

**Current path**:

```
[parse string]
  → [double-pump: consume ,]
  → [skip_to_action]
  → [check for "]
  → [scan_key_colon_next]
  (→ repeat for next pair)
```

**Target path**:

```
[parse string]
  → [fused_val_sep_key: detect , immediately → scan key → consume :]
  → complete in a single function
```

**Implementation key**:
- Extend `scan_key_colon_next()` to also handle the preceding value's closing delimiter (`,` or `}`)
- twitter.json: 10.6 pairs/object × reduced function calls = significant reduction in iteration overhead
- Consolidate and eliminate duplicate `skip_to_action()` call paths

**Expected effect**: Eliminate intra-object traversal function calls on twitter.json → **~5-7% speedup**

---

### B2. ≤8-byte Inline String Embedded Directly in Tape

**Goal**: Eliminate cache misses for the 38.7% of twitter.json strings that are ≤8 bytes.

**Current TapeNode structure**:

```cpp
struct TapeNode {
    TapeNodeType type;   // 1 byte
    uint8_t flags;       // 1 byte
    uint16_t length;     // 2 bytes
    uint32_t offset;     // 4 bytes  ← pointer into original JSON (cache miss source)
    uint32_t next_sib;   // 4 bytes
    uint32_t aux;        // 4 bytes  ← currently unused
};  // 16 bytes total
```

**Target structure (add inline-string mode)**:

```cpp
// Flag bit definition
constexpr uint8_t INLINE_STR = 0x01;

// For strings ≤8 bytes: copy content directly into next_sib(4) + aux(4) = 8 bytes
// At parse time:
if (node.length <= 8) {
    node.flags |= INLINE_STR;
    memcpy(&node.next_sib, src_ptr, node.length);
    // offset is 0 (unused)
}

// At lookup time:
inline std::string_view get_string_sv() const {
    if (flags & INLINE_STR) {
        return std::string_view(
            reinterpret_cast<const char*>(&next_sib), length);
    }
    return std::string_view(base_ptr + offset, length);
}
```

**Implementation notes**:
- `memcpy` of 8 bytes at parse time → compiler optimizes to a single `STR` instruction
- `find()` and `get_string()` calls resolve entirely from tape without accessing original JSON memory (no cache miss)
- Serialization (dump) path benefits from the same cache miss reduction

**Expected effect**: Fewer cache misses on simple key-value lookups and serialization → **~10% improvement**

---

## 5. Phase C — Unique Innovations (Industry-Novel Techniques)

### C1. Speculative Parallel Parsing via next_sib Links

**Goal**: Leverage beast-json's unique next_sib link structure for multi-threaded split parsing.

**Principle**:

```
{                    ← next_sib links allow instant access to parsed positions
  "key1": <subtree1> ← Thread 1 handles
  "key2": <subtree2> ← Thread 2 handles
  "key3": <subtree3> ← Thread 3 handles
}
```

**Prerequisites**:
- Phase A1 Stage1 bitmap pre-scan completed → top-level object key positions known in advance
- TLAB supports independent per-thread allocation → minimal merge overhead

**Implementation steps**:
1. Extract depth=1 key position list from Stage1 bitmap (O(n) pre-scan)
2. Partition work among threads based on key count
3. Each thread parses its subtree in an independent TLAB
4. Main thread merges results via next_sib links

**Expected effect**: Multi-core utilization → **~40%+ speedup** (4-core baseline)

---

### C2. TLAB Direct Parsing (Fully Eliminate Tape Pre-allocation)

**Goal**: Remove malloc/realloc decision logic and bounds checking in `parse_reuse`.

**Current**:
```
parse_reuse → check tape.base size → realloc if insufficient
             → set tape_head_ → check arena->head bounds
```

**Target**:
```
Direct TLAB write → tape_head_++ → operates entirely within arena
                  → no bounds check (TLAB pre-sized larger than JSON)
```

**Implementation notes**:
- Pre-reserve TLAB based on `json.size() * 2` (twitter.json: ~1.2MB)
- `tape_head_` operates directly as a pointer within TLAB
- Remove realloc path code → fewer branches

**Expected effect**: Eliminate parse initialization overhead → **~2-3% speedup**

---

## 6. Implementation Order and Milestones

```
Week 1: Phase A2 (32-byte SWAR quad-pump)
  → Low difficulty, immediate 5-8% improvement on twitter.json
  → Compatible with existing NEON fallback

Week 2-3: Phase A1 (Stage1 bitmap → parse_reuse integration)
  → Core structural change, requires thorough testing
  → Upon completion: 15-20% additional improvement on twitter.json
  → Must maintain all 81 existing tests passing

Week 4: Phase B1 (3-way fused scanner)
  → Maximize synergy after A1 completion
  → Extend scan_key_colon_next()

Week 5-6: Phase B2 (inline string tape embedding)
  → TapeNode layout change → review backward compatibility
  → Update find() / get_string() / dump() throughout

Week 7+: Phase C1 (multi-threaded split parsing)
  → Requires A1 completion as prerequisite
  → Design → prototype → thread safety verification
```

---

## 7. Testing and Verification Criteria

### Regression Prevention
- All changes must pass `ctest --output-on-failure` (81 tests)
- No performance regression on canada.json, citm_catalog.json, gsoc-2018.json

### Performance Targets

| Milestone | twitter.json Target | Baseline |
|:---|:---:|:---|
| A2 complete | ≤300μs | Current 314μs |
| A1 complete | ≤265μs | Approaching yyjson 250μs |
| A1+B1 complete | ≤250μs | Equal to yyjson |
| A1+B1+B2 complete | ≤230μs | Surpasses yyjson |
| C1 complete (4-core) | ≤150μs | Dominates yyjson |

### Benchmark Environment
- Current: Linux x86-64, GCC 13.3.0 `-O3`, 300 iterations
- After A1: ARM64 NEON path measured separately
- After C1: Per-core scaling measurements added

---

## 8. Key Message

> **Implementing A1 alone is likely sufficient to match or surpass yyjson on twitter.json.**

beast-json already has three unique strengths: bitmap infrastructure, TLAB, and next_sib links.
The sole structural weakness is that these strengths are not yet wired into the parse_reuse hot path.

Once connected:
- **Short-term (A complete)**: Surpass yyjson on twitter.json
- **Mid-term (B complete)**: Dominate yyjson across all benchmarks
- **Long-term (C complete)**: Achieve world's fastest JSON parser on multi-core
