# Beast JSON — yyjson Domination Plan (Phase 31-35)

> **Date**: 2026-02-28 (Phase 33 complete)  
> **Goal**: Dominate yyjson by **30%+ margin** (not just surpass — crush it)  
> **Architectures**: aarch64 (NEON) PRIMARY · x86_64 (SSE2/AVX2) SECONDARY  
> **Note**: Current dev machine is Apple M1 Pro (aarch64). Linux x86_64 agents should
> validate Phase 34 (AVX2) and confirm no regressions on the Linux benchmark baseline.

---

## Current Gap vs Domination Target

| File | yyjson (M1) | Beast Phase 33 | **Target** | yyjson (Linux) | Beast (Linux) |
|:---|---:|---:|---:|---:|---:|
| twitter.json | 178 μs | 264 μs | **< 120 μs** | 267 μs | ~340 μs |
| canada.json | 1,456 μs | 1,891 μs | **< 950 μs** | 2,552 μs | ~2,052 μs ✅ |
| citm_catalog.json | 474 μs | 646 μs | **< 320 μs** | 668 μs | ~741 μs |
| gsoc-2018.json | 982 μs | **632 μs ✅** | **< 500 μs ✅** | 1,685 μs | ~1,079 μs ✅ |

---

## Root Cause Analysis

| Cause | Affected Files | Estimated Loss |
|:---|:---|:---:|
| `scan_string_end()`: SWAR only, SSE2/NEON unused | twitter, citm | ~40 μs |
| `switch(c)` 17-case dispatch: BTB misses | all | ~15 μs |
| Float fractional-part scalar loop | canada | ~400 μs |
| Single-threaded only | all | theoretical ceiling |

---

## Phase 31 — Contextual SIMD Gate String Scanner ✅ DONE

**Commit**: `a60e265` → merged to main  
**Actual results (M1 Pro, 100 iter)**: twitter **-4.4%** (276→264 μs), gsoc **-11.6%** (715→632 μs)

**File**: `include/beast_json/beast_json.hpp` — `scan_string_end()` (L5293) and `scan_key_colon_next()` (L5357)

### Theory: Contextual SIMD Gate

Phase 30 was reverted because NEON had startup overhead on short strings.
**Fix**: Run an 8B SWAR gate first — short strings exit immediately (zero SIMD cost).
Only when the string is confirmed > 8 chars do we enter the SIMD loop.

```
[Stage 1: 8B SWAR]  →  quote/backslash found → return immediately  (≤8B: 36% of twitter strings)
         │ not found (string confirmed > 8 chars)
         ↓
  #if BEAST_HAS_NEON (aarch64, PRIMARY)  → vld1q_u8        16B loop
  #elif BEAST_HAS_SSE2 (x86_64)         → _mm_loadu_si128  16B loop
  #else                                  → SWAR-8 loop (existing)
```

### aarch64 — NEON 16B (PRIMARY)

```cpp
// #if BEAST_HAS_NEON block, placed after Stage 1 SWAR gate
const uint8x16_t vq  = vdupq_n_u8('"');
const uint8x16_t vbs = vdupq_n_u8('\\');
while (p + 16 <= end_) {
  uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
  uint8x16_t m = vorrq_u8(vceqq_u8(v, vq), vceqq_u8(v, vbs));
  if (vmaxvq_u32(vreinterpretq_u32_u8(m)) != 0) {
    while (*p != '"' && *p != '\\') ++p;
    return p;
  }
  p += 16;
}
```

### x86_64 — SSE2 16B

```cpp
// #elif BEAST_HAS_SSE2 block
const __m128i vq  = _mm_set1_epi8('"');
const __m128i vbs = _mm_set1_epi8('\\');
while (p + 16 <= end_) {
  __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
  int mask  = _mm_movemask_epi8(
                _mm_or_si128(_mm_cmpeq_epi8(v, vq), _mm_cmpeq_epi8(v, vbs)));
  if (mask) return p + __builtin_ctz(mask);
  p += 16;
}
```

**Expected gain**: twitter **−20%** (276→220 μs), citm **−15%** — both architectures

---

## Phase 32 — 256-Entry constexpr Action LUT

**File**: `include/beast_json/beast_json.hpp` — `parse()` hot loop (L5492)

Current `switch(c)` has 17 cases → strains Branch Target Buffer on both M1 and x86_64.

```cpp
// Add in namespace lazy, before the Parser class
enum ActionId : uint8_t {
  kActNone=0, kActString, kActNumber, kActObjOpen, kActArrOpen,
  kActClose, kActColon, kActComma, kActTrue, kActFalse, kActNull
};
static constexpr uint8_t kActionLut[256] = []() consteval {
  uint8_t t[256] = {};
  t[static_cast<uint8_t>('"')] = kActString;
  for (uint8_t c : {'-','0','1','2','3','4','5','6','7','8','9'})
    t[c] = kActNumber;
  t[static_cast<uint8_t>('{')] = kActObjOpen;
  t[static_cast<uint8_t>('[')] = kActArrOpen;
  t[static_cast<uint8_t>('}')] = kActClose;
  t[static_cast<uint8_t>(']')] = kActClose;
  t[static_cast<uint8_t>(':')] = kActColon;
  t[static_cast<uint8_t>(',')] = kActComma;
  t[static_cast<uint8_t>('t')] = kActTrue;
  t[static_cast<uint8_t>('f')] = kActFalse;
  t[static_cast<uint8_t>('n')] = kActNull;
  return t;
}();

// In parse(): switch(kActionLut[static_cast<uint8_t>(c)])  // 11 cases instead of 17
```

256 bytes = 4 L1 cache lines. Architecture-agnostic pure C++.  
**Expected gain**: all files **−8%**

---

## Phase 33 — SWAR Float Scanner

**File**: `include/beast_json/beast_json.hpp` — number case in `parse()` (L5760)

canada.json has 2.32M floats; the fractional-part digit loop is a pure scalar bottleneck.

```cpp
// Replace: while (*p_ >= '0' && *p_ <= '9') ++p_;
// With: same SWAR-8 digit scanner already used for the integer part

auto swar_skip_digits = [&]() BEAST_INLINE {
  while (p_ + 8 <= end_) {
    uint64_t v; std::memcpy(&v, p_, 8);
    uint64_t s  = v - 0x3030303030303030ULL;
    uint64_t nd = (s | ((s & 0x7F7F7F7F7F7F7F7FULL) + 0x7676767676767676ULL))
                  & 0x8080808080808080ULL;
    if (nd) { p_ += BEAST_CTZ(nd) >> 3; return; }
    p_ += 8;
  }
  while (p_ < end_ && static_cast<unsigned>(*p_ - '0') < 10u) ++p_;
};
// Apply to fractional part (after '.') AND exponent part (after 'e'/'E' sign)
```

Architecture-agnostic (pure SWAR).  
**Expected gain**: canada **−20%** (2,021→1,600 μs)

---

## Phase 32 — 256-Entry constexpr Action LUT ✅ DONE

**Commit**: `d2581d4` → merged to main  
**Note**: Replaced `switch(c)` 17 char-literal cases with `switch(kActionLut[c])` 11 ActionId cases.
Used `std::array<uint8_t,256>` with `consteval` lambda (Apple Clang 17 compatible).
**Actual results**: flat (BTB improvement masked by thermal variability on M1).

---

## Phase 33 — SWAR-8 Float Digit Scanner ✅ DONE

**Commit**: `39ca6d9` → merged to main  
**Note**: Lambda approach caused regression (no inlining guarantee). Replaced with
`BEAST_SWAR_SKIP_DIGITS()` inline macro for zero call overhead.  
**Actual results (M1 Pro, 50 iter)**: canada **-6.4%** (2,021→1,891 μs)

---

## Phase 34 — AVX2 32B String Scanner (x86_64 only) ✅ DONE

**Commit**: `c5b6b73` → merged to main
**Note**: Added `#elif BEAST_HAS_AVX2` block in `scan_string_end()` using `_mm256_loadu_si256` and `_mm256_movemask_epi8` for 32 bytes/iter.
aarch64 NEON is 128-bit (16B). 32B would require SVE (not available on M1).
x86_64 with Haswell+ supports AVX2 (256-bit = 32B).

```cpp
// Placed ABOVE the SSE2 block: #if BEAST_HAS_AVX2
const __m256i vq  = _mm256_set1_epi8('"');
const __m256i vbs = _mm256_set1_epi8('\\');
while (p + 32 <= end_) {
  __m256i v     = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
  uint32_t mask = static_cast<uint32_t>(_mm256_movemask_epi8(
                    _mm256_or_si256(_mm256_cmpeq_epi8(v, vq),
                                    _mm256_cmpeq_epi8(v, vbs))));
  if (mask) return p + __builtin_ctz(mask);
  p += 32;
}
// then SSE2 16B tail
```

> **aarch64 agents**: this block is `#if BEAST_HAS_AVX2` — no impact on M1 builds.  
> **x86_64 agents**: validate with `cmake -DCMAKE_CXX_FLAGS="-mavx2"` on Linux.

**Expected gain (x86_64 only)**: citm/gsoc **additional −15%**

---

## Phase 35 — Multi-Threaded Parallel Parsing ⏸️ HOLD

**Commit**: Feature branch test only; rollback and aborted.

**Attempted Implementation**:
- Pre-scan: `scan_toplevel_value_offsets()` to find top-level value boundaries.
- Partition: Distributed string slices to N threads (using `std::thread`).
- Parse: Each thread parsed independently into its own lock-free `DocumentView`.
- Merge: Main thread used zero-copy `std::memcpy` concatenation of sub-tapes.

**Failure Analysis (Why it was rolled back)**:
The total parsing time for target documents (e.g. `twitter.json`, `canada.json`) in single-thread highly-optimized C++ is measured in **hundreds of microseconds** (e.g., 260~1800 μs).
The overhead of creating, scheduling, and joining `std::thread`s on the OS level is inherently in the millisecond regime, drastically overpowering the theoretical gains of splitting the work. 

**Conclusion**: Internal multi-threading at the single-document API layer is detrimental for ultra-fast JSON parsers. It is architecturally sounder for the library user to process separate documents in parallel worker threads (which Beast JSON perfectly supports due to zero global state).

## Branch Strategy

| Phase | Branch | Status |
|:---|:---|:---:|
| 31 | `feature/phase31-simd-string-gate` | ✅ merged |
| 32 | `feature/phase32-action-lut` | ✅ merged |
| 33 | `feature/phase33-swar-float` | ✅ merged |
| 34 | `feature/phase34-avx2-string` | ✅ merged |
| 35 | `feature/phase35-parallel-parse` | ⏸️ Hold (aborted) |

---

## Verification Checklist (run after every Phase)

```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBEAST_JSON_BUILD_TESTS=ON -DBEAST_JSON_BUILD_BENCHMARKS=ON
cmake --build build -j$(sysctl -n hw.ncpu)        # or nproc on Linux

# 1. All 81 tests must PASS
ctest --test-dir build --output-on-failure

# 2. No regression vs previous Phase
cd build/benchmarks && ./bench_all --all --iter 50

# 3. Commit
git add include/beast_json/beast_json.hpp
git commit -m "phase3X: ..."
```

> [!CAUTION]
> If canada/gsoc/citm show regression, revert the Phase and re-examine
> arch-specific conditional compilation before re-attempting.
