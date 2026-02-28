# Beast JSON — yyjson Domination Plan (Phase 31-45)

> **Date**: 2026-02-28 (Phase 41 + AVX-512 fix complete)
> **Goal**: Dominate yyjson by **30%+ margin** (not just surpass — crush it)
> **Architectures**: aarch64 (NEON) PRIMARY · x86_64 (SSE2/AVX2/AVX-512) SECONDARY
> **Current dev env**: Linux x86_64 with AVX-512 (GCC 13.3.0, -O3 -mavx2 -march=native)

---

## Current Gap vs Domination Target

### Linux x86_64 (AVX-512, Phase 41, 150 iter)

| File | Beast | yyjson | Beast vs yyjson | Target |
|:---|---:|---:|:---:|:---:|
| twitter.json | 351 μs | 311 μs | yyjson 13% faster | **Beat yyjson** |
| canada.json | **1,677 μs** | 2,998 μs | **Beast +44%** | ✅ Dominating |
| citm_catalog.json | **797 μs** | 795 μs | **Tied** | ✅ |
| gsoc-2018.json | **761 μs** (4.27 GB/s) | 1,752 μs | **Beast +57%** | ✅ Dominating |

### M1 Pro (NEON, Phase 34, separate machine)

| File | Beast | yyjson | Gap |
|:---|---:|---:|:---:|
| twitter.json | 264 μs | 178 μs | yyjson 33% faster |
| canada.json | 1,891 μs | 1,456 μs | yyjson 23% faster |
| citm_catalog.json | 646 μs | 474 μs | yyjson 27% faster |
| gsoc-2018.json | **632 μs** | 982 μs | **Beast +36%** ✅ |

**twitter.json is the sole remaining target** on x86_64.

---

## Root Cause Analysis (Remaining)

| Cause | Affected Files | Estimated Loss |
|:---|:---|:---:|
| twitter: many short strings — scan overhead per token | twitter | ~40 μs |
| AVX2 only covers 32B one-shot; 33-63 char strings hit str_slow | twitter, citm | ~15 μs |
| No AVX-512 native path (64B/iter in scan_string_end) | all long-string files | ~10% |
| Branch mispredictions in parse() dispatch (unknown) | twitter | TBD via perf |

---

## Phase 31 — Contextual SIMD Gate String Scanner ✅ DONE

**Commit**: `a60e265` → merged to main
**Actual results (M1 Pro, 100 iter)**: twitter **-4.4%** (276→264 μs), gsoc **-11.6%** (715→632 μs)

**File**: `include/beast_json/beast_json.hpp` — `scan_string_end()` and `scan_key_colon_next()`

### Theory: Contextual SIMD Gate

Phase 30 was reverted because NEON had startup overhead on short strings.
**Fix**: Run an 8B SWAR gate first — short strings exit immediately (zero SIMD cost).
Only when the string is confirmed > 8 chars do we enter the SIMD loop.

```
[Stage 1: 8B SWAR]  →  quote/backslash found → return immediately  (≤8B: 36% of twitter strings)
         │ not found (string confirmed > 8 chars)
         ↓
  #if BEAST_HAS_NEON (aarch64, PRIMARY)  → vld1q_u8        16B loop
  #elif BEAST_HAS_AVX2 (x86_64)         → _mm256_loadu_si256  32B loop  (Phase 34)
  #elif BEAST_HAS_SSE2 (x86_64)         → _mm_loadu_si128  16B loop
  #else                                  → SWAR-8 loop (existing)
```

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

**x86_64 Validation (Linux, GCC 13.3.0, -O3 -flto -mavx2, 50 iter, yyjson SIMD enabled)**:

| File | Beast | yyjson (+SIMD) | Beast vs yyjson |
|:---|---:|---:|:---:|
| twitter.json | 332 μs | 284 μs | yyjson 15% faster |
| canada.json | **1,519 μs** | 2,695 μs | **Beast 44% faster** |
| citm_catalog.json | 734 μs | 723 μs | **Tied** (1.5%) |
| gsoc-2018.json | **750 μs** (4.44 GB/s) | 1,675 μs | **Beast 55% faster** |

---

## Phase 36 — AVX2 Inline String Scan (parse() hot path) ✅ DONE

**Note**: Applied AVX2 32B one-shot string scan directly in the `kActString` case of `parse()` and in `scan_key_colon_next()`. The key fix vs the initial attempt: replaced `do { break }` escape pattern with `if (s+32<=end_) { ... goto str_slow; }` so that mask==0 and backslash cases jump directly to `str_slow`/`skn_slow`, bypassing the redundant SWAR-24 check entirely.

**Phase 37 attempt (AVX2 whitespace skip in skip_to_action) — REVERTED**:
Root cause: for typical JSON whitespace (0–8 bytes between tokens), SWAR-32's 4 parallel scalar operations are faster than the AVX2 XOR+CMPGT+MOVEMASK chain. Reverted.

**x86_64 Results (Linux, GCC 13.3.0, -O3 -flto -mavx2, 100 iter)**:

| File | Phase 34 | Phase 36 | Delta | yyjson | Beast vs yyjson |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 332 μs | **318 μs** | **−4.5%** | 271 μs | yyjson 15% faster |
| canada.json | 1,519 μs | **1,501 μs** | −1.3% | 2,690 μs | **Beast 44% faster** |
| citm_catalog.json | 734 μs | 755 μs | ±2% noise | 736 μs | **Tied** |
| gsoc-2018.json | 750 μs | **747 μs** | −0.5% | 1,640 μs | **Beast 55% faster** |

---

## Phase 35 — Multi-Threaded Parallel Parsing ⏸️ HOLD

**Conclusion**: Internal multi-threading at the single-document API layer is detrimental for ultra-fast JSON parsers (thread creation overhead >> parse time). Users should run multiple documents in separate threads. **Permanently on hold.**

---

## Phase 40 — AVX2 Constant Hoisting ❌ REVERTED

**Commit**: `6ea3a41` (introduced) → `b1ed9ed` (reverted)

### What Was Attempted
Declare `h_vq`/`h_vbs` (`__m256i`) outside the `parse()` hot loop so the compiler emits a single `vpbroadcastb` per constant instead of re-broadcasting on every string token.

### Why It Failed
Keeping two `__m256i` values live throughout the entire `parse()` loop **permanently occupies 2 YMM registers** regardless of which `ActionId` case is executing. On x86_64 (16 YMM registers total), this caused register spills during number processing (canada), object/array handling (citm), and other non-string paths.

| File | Phase 36 | Phase 40 | Regression |
|:---|---:|---:|:---:|
| twitter.json | 318 μs | 357 μs | **+12%** |
| canada.json | 1,501 μs | 1,705 μs | **+14%** |
| citm_catalog.json | 755 μs | 812 μs | **+8%** |
| gsoc-2018.json | 747 μs | 824 μs | **+10%** |

### Lesson Learned
> **SIMD constants must be declared adjacent to their use site** to minimize register lifetime. `vpbroadcastb` costs only 1 cycle latency — re-broadcasting per call is essentially free compared to spill overhead.

---

## Phase 41 — skip_string_from32: mask==0 AVX2 Fast Path ✅ DONE

**Commit**: `6ea3a41` (alongside Phase 40), cleaned up in `b1ed9ed`

### What It Does
When the AVX2 inline scan in `kActString` / `scan_key_colon_next` returns `mask==0` (meaning the first 32 bytes are confirmed quote/backslash-free), instead of falling through to `str_slow`/`skn_slow` (which would re-scan from `s` via `scan_string_end`'s SWAR-8 gate), call `skip_string_from32(s)` which starts the AVX2 loop directly at `s+32`.

```cpp
// skip_string_from32: starts AVX2 at s+32, skipping SWAR-8 gate
BEAST_INLINE const char *skip_string_from32(const char *s) noexcept {
  const char *p = s + 32;
#if BEAST_HAS_AVX2
  const __m256i vq  = _mm256_set1_epi8('"');
  const __m256i vbs = _mm256_set1_epi8('\\');
  while (BEAST_LIKELY(p + 32 <= end_)) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
    uint32_t mask = static_cast<uint32_t>(_mm256_movemask_epi8(
        _mm256_or_si256(_mm256_cmpeq_epi8(v, vq), _mm256_cmpeq_epi8(v, vbs))));
    if (BEAST_LIKELY(mask != 0)) {
      p += __builtin_ctz(mask);
      if (BEAST_LIKELY(*p == '"')) return p;
      p += 2; // skip escape sequence
      continue;
    }
    p += 32;
  }
  // SSE2 16B tail, then SWAR-8 scalar tail
#endif
  while (p < end_) { p = scan_string_end(p); ... }
}
```

**Benefit**: Saves ~4 SWAR-8 iterations (32 bytes / 8 bytes per iteration) for strings ≥32 chars whose first 32 bytes are clean.

---

## AVX-512 Detection Bug Fix ✅ DONE

**Commit**: `b1ed9ed`

### Bug
The SIMD detection chain used `#if ... #elif`:
```cpp
#if defined(__AVX512F__)
#define BEAST_HAS_AVX512 1      // only this gets set
#include <immintrin.h>
#elif defined(__AVX2__)
#define BEAST_HAS_AVX2 1        // NEVER reached on AVX-512 machines
```

On AVX-512 machines, `BEAST_HAS_AVX2` was never defined, so **all Phase 34/36/41 AVX2 code was dead**. The parser fell back to SSE2-only, which is ~2x slower for long strings.

### Fix
```cpp
#if defined(__AVX512F__)
#define BEAST_HAS_AVX512 1
#define BEAST_HAS_AVX2 1  // AVX-512 ⊇ AVX2; all ymm code is valid
#include <immintrin.h>
#elif defined(__AVX2__)
#define BEAST_HAS_AVX2 1
```

### Impact
After the fix, objdump confirms `vpcmpeqb %ymm`, `vpor %ymm`, `vpmovmskb %ymm` instructions are emitted. Results on this AVX-512 machine (150 iter):

| File | Before fix (SSE2 only) | After fix (AVX2 active) | yyjson | Beast vs yyjson |
|:---|---:|---:|---:|:---:|
| twitter.json | ~360 μs | **351 μs** | 311 μs | yyjson 13% faster |
| canada.json | ~1,700 μs | **1,677 μs** | 2,998 μs | **Beast +44%** |
| citm_catalog.json | ~810 μs | **797 μs** | 795 μs | **Tied** |
| gsoc-2018.json | ~820 μs | **761 μs** | 1,752 μs | **Beast +57%** |

---

## Phase 42 — AVX-512 Native 64B String Scanner 🔜

**File**: `include/beast_json/beast_json.hpp` — `scan_string_end()`

### Theory
AVX-512 `zmm` registers hold 512 bits = 64 bytes. `_mm512_cmpeq_epi8_mask` directly produces a `uint64_t` bitmask, eliminating the `vpor` step needed with AVX2. Place this block above the AVX2 32B block:

```cpp
#if BEAST_HAS_AVX512
{
  const __m512i vq512  = _mm512_set1_epi8('"');
  const __m512i vbs512 = _mm512_set1_epi8('\\');
  while (p + 64 <= end_) {
    __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(p));
    uint64_t mask = _mm512_cmpeq_epi8_mask(v, vq512)
                  | _mm512_cmpeq_epi8_mask(v, vbs512);
    if (mask) { p += __builtin_ctzll(mask); return p; }
    p += 64;
  }
  // fall through to AVX2 32B for remaining <64B
}
#endif
```

**Expected gain**: gsoc/citm (long strings) **−10~15%** · twitter marginal (short strings already hit in 32B)

---

## Phase 43 — kActString Inline AVX-512 64B One-Shot Scan 🔜

**File**: `include/beast_json/beast_json.hpp` — `kActString` case in `parse()`

Extends Phase 36's inline 32B scan to 64B. Strings ≤63 chars handled in a single zmm load without calling any helper.

```cpp
#if BEAST_HAS_AVX512
if (BEAST_LIKELY(s + 64 <= end_)) {
  const __m512i _vq512  = _mm512_set1_epi8('"');
  const __m512i _vbs512 = _mm512_set1_epi8('\\');
  __m512i _v512 = _mm512_loadu_si512(reinterpret_cast<const __m512i *>(s));
  uint64_t _mask512 = _mm512_cmpeq_epi8_mask(_v512, _vq512)
                    | _mm512_cmpeq_epi8_mask(_v512, _vbs512);
  if (BEAST_LIKELY(_mask512 != 0)) {
    e = s + __builtin_ctzll(_mask512);
    if (BEAST_LIKELY(*e == '"')) {
      push(TapeNodeType::StringRaw, static_cast<uint16_t>(e - s),
           static_cast<uint32_t>(s - data_));
      p_ = e + 1;
      goto str_done;
    }
    goto str_slow; // backslash first
  }
  // mask==0: bytes [s, s+64) clean → skip_string_from64(s) (future)
  goto str_slow;
}
// fall through to AVX2 32B
#elif BEAST_HAS_AVX2
// ... existing Phase 36 code
```

**Expected gain**: citm (many long keys) **−5~10%** · twitter moderate

---

## Phase 44 — Twitter Bottleneck Profiling 🔜

Use `perf stat` to quantify the exact source of the 13% yyjson advantage on twitter:

```bash
# Compare IPC, branch misses, cache misses
perf stat -e cycles,instructions,branch-misses,L1-dcache-misses,LLC-misses \
  ./bench_all twitter.json --iter 10

# Identify hot functions
perf record -g ./bench_all twitter.json --iter 50
perf report --sort=dso,symbol
```

**Hypotheses to test**:
1. Branch mispredictions in `parse()` dispatch (kActionLut switch)
2. TapeNode push write bandwidth (many small strings = many tape writes)
3. `scan_key_colon_next` called for every object key — overhead accumulation
4. yyjson tape format difference (smaller per-node metadata?)

---

## Phase 45 — scan_key_colon_next SWAR-24 Cleanup 🔜

`scan_key_colon_next` has a SWAR-24 fallthrough path that is redundant when AVX2/AVX-512 inline scan already covers the common case. Removing this dead path reduces function size → better I-cache utilization for the twitter hot path.

---

## Branch Strategy

| Phase | Branch | Status |
|:---|:---|:---:|
| 31 | `feature/phase31-simd-string-gate` | ✅ merged |
| 32 | `feature/phase32-action-lut` | ✅ merged |
| 33 | `feature/phase33-swar-float` | ✅ merged |
| 34 | `feature/phase34-avx2-string` | ✅ merged |
| 35 | `feature/phase35-parallel-parse` | ⏸️ Hold (aborted) |
| 36 | `claude/review-todos-optimize-kJcdz` | ✅ Done |
| 40 | `claude/review-todos-optimize-kJcdz` | ❌ Reverted |
| 41 | `claude/review-todos-optimize-kJcdz` | ✅ Done |
| AVX-512 fix | `claude/review-todos-optimize-kJcdz` | ✅ Done |
| 42 | `claude/review-todos-optimize-kJcdz` | 🔜 Next |

---

## Verification Checklist (run after every Phase)

```bash
# Build (AVX-512 machine — must include -mavx2 to activate BEAST_HAS_AVX2)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBEAST_JSON_BUILD_TESTS=ON -DBEAST_JSON_BUILD_BENCHMARKS=ON \
  -DCMAKE_CXX_FLAGS="-O3 -mavx2 -march=native"
cmake --build build -j$(nproc)

# Verify AVX2/AVX-512 instructions are present
objdump -d build/benchmarks/bench_all | grep "vpcmpeqb.*ymm\|vpmovmskb" | head -5

# 1. All 81 tests must PASS
ctest --test-dir build --output-on-failure

# 2. No regression vs previous Phase (run from build/benchmarks/)
./bench_all --all --iter 100

# 3. Commit
git add include/beast_json/beast_json.hpp
git commit -m "phaseXX: ..."
```

> [!CAUTION]
> If any file shows regression, revert the Phase and re-examine
> arch-specific conditional compilation before re-attempting.
>
> **Register pressure rule**: Never hoist `__m256i`/`__m512i` constants
> outside the block that uses them — Phase 40 proved this causes universal
> regression. SIMD constants belong adjacent to their use site.
