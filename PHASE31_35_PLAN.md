# Beast JSON — yyjson 압도 플랜 (Phase 31-35)

> **작성일**: 2026-02-28  
> **목표**: yyjson 대비 **30% 이상 우세** (추월이 아닌 압도)  
> **환경**: x86\_64 (SSE2/AVX2) + aarch64 (NEON) 크로스플랫폼

---

## 현재 성능 vs 압도 목표

| 파일 | yyjson (M1) | Beast 현재 | **압도 목표** | yyjson (Linux) | Beast 현재 |
|:---|---:|---:|---:|---:|---:|
| twitter.json | 176 μs | 276 μs | **< 120 μs** | 267 μs | 340 μs |
| canada.json | 1,426 μs | 2,021 μs | **< 950 μs** | 2,552 μs | 2,052 μs ✅ |
| citm.json | 465 μs | 643 μs | **< 320 μs** | 668 μs | 741 μs |
| gsoc-2018.json | 978 μs | 715 μs ✅ | **< 500 μs** | 1,685 μs | 1,079 μs ✅ |

---

## Gap 핵심 원인

| 원인 | 영향 파일 | 추정 손실 |
|:---|:---|:---:|
| `scan_string_end()`: SWAR만 사용, SSE2/NEON 미활용 | twitter, citm | ~40 μs |
| `switch(c)` 17-case dispatch: BTB miss | 전체 | ~15 μs |
| float 소수부 스칼라 루프 | canada | ~400 μs |
| 단일 스레드 | 전체 | 이론 상한 |

---

## Phase 31 — Contextual SIMD Gate String Scanner

**파일**: `include/beast_json/beast_json.hpp` — `scan_string_end()` (L5293), `scan_key_colon_next()` (L5357)

### 이론: Contextual SIMD Gate

Phase 30 revert 원인 = "short string에서 NEON 초기화 오버헤드". 해결:
**8B SWAR gate**를 먼저 실행해 short string은 즉시 종료(비용 0), 8자 초과 확정 후 SIMD 진입.

```
[Stage 1: 8B SWAR]  →  quote/backslash 발견 → 즉시 종료 (≤8B: twitter strings 36%)
     │ 없으면 (>8자 확정)
     ↓
  #if BEAST_HAS_AVX2 → _mm256_loadu_si256 32B loop  (Phase 34에서 활성화)
  #elif BEAST_HAS_SSE2 (x86_64만) → _mm_loadu_si128 16B loop
  #elif BEAST_HAS_NEON (aarch64만) → vld1q_u8        16B loop
  #else                            → SWAR-8 loop (기존)
```

### x86_64 구현 (SSE2)

```cpp
// #if BEAST_HAS_SSE2 블록, Stage 1 8B SWAR 뒤에 위치
const __m128i vq  = _mm_set1_epi8('"');
const __m128i vbs = _mm_set1_epi8('\\');
while (p + 16 <= end_) {
  __m128i v = _mm_loadu_si128((const __m128i*)p);
  int mask  = _mm_movemask_epi8(
                _mm_or_si128(_mm_cmpeq_epi8(v, vq), _mm_cmpeq_epi8(v, vbs)));
  if (mask) return p + __builtin_ctz(mask);
  p += 16;
}
```

### aarch64 구현 (NEON)

```cpp
// #elif BEAST_HAS_NEON 블록
const uint8x16_t vq  = vdupq_n_u8('"');
const uint8x16_t vbs = vdupq_n_u8('\\');
while (p + 16 <= end_) {
  uint8x16_t v = vld1q_u8((const uint8_t*)p);
  uint8x16_t m = vorrq_u8(vceqq_u8(v, vq), vceqq_u8(v, vbs));
  if (vmaxvq_u32(vreinterpretq_u32_u8(m))) {
    while (*p != '"' && *p != '\\') ++p;
    return p;
  }
  p += 16;
}
```

**목표**: twitter **-20%** (276→220μs), citm **-15%**

---

## Phase 32 — 256-Entry constexpr Action LUT

**파일**: `include/beast_json/beast_json.hpp` — `parse()` (L5492)

현재 `switch(c)` 17 cases → CPU Branch Target Buffer (BTB) miss 유발.

```cpp
// 헤더 상단 (namespace lazy 내부)
enum ActionId : uint8_t {
  kActNone=0, kActString, kActNumber,
  kActObjOpen, kActArrOpen, kActClose,
  kActColon, kActComma, kActTrue, kActFalse, kActNull
};

static constexpr uint8_t kActionLut[256] = []() consteval {
  uint8_t t[256] = {};
  t[(uint8_t)'"'] = kActString;
  for (uint8_t c : {'-','0','1','2','3','4','5','6','7','8','9'})
    t[c] = kActNumber;
  t[(uint8_t)'{'] = kActObjOpen;  t[(uint8_t)'['] = kActArrOpen;
  t[(uint8_t)'}'] = kActClose;    t[(uint8_t)']'] = kActClose;
  t[(uint8_t)':'] = kActColon;    t[(uint8_t)','] = kActComma;
  t[(uint8_t)'t'] = kActTrue;     t[(uint8_t)'f'] = kActFalse;
  t[(uint8_t)'n'] = kActNull;
  return t;
}();

// parse() 변경:
// switch(static_cast<unsigned char>(c))  →  switch(kActionLut[(uint8_t)c])
// 17 cases → 11 cases (BTB 정확도 향상)
```

**아키텍처 무관 (pure C++)**, 256B = L1 cache 4 lines 상주.  
**목표**: 전체 **-8%**

---

## Phase 33 — SWAR Float Scanner

**파일**: `include/beast_json/beast_json.hpp` — `parse()` number case (L5760)

canada.json: float 2.32M개 × 소수부 스칼라 루프 = 최대 병목.

```cpp
// 기존 (스칼라): while (*p >= '0' && *p <= '9') ++p;
// 신규 (SWAR-8): 정수부와 동일한 digit scanner 재사용

// 소수부(`.` 뒤) + 지수부(`e` 뒤) 모두 적용
auto swar_skip_digits = [&]() {
  while (p_ + 8 <= end_) {
    uint64_t v; std::memcpy(&v, p_, 8);
    uint64_t s  = v - 0x3030303030303030ULL;
    uint64_t nd = (s | ((s & 0x7F7F7F7F7F7F7F7FULL) + 0x7676767676767676ULL))
                  & 0x8080808080808080ULL;
    if (nd) { p_ += BEAST_CTZ(nd) >> 3; return; }
    p_ += 8;
  }
  while (p_ < end_ && (unsigned)(*p_ - '0') < 10u) ++p_;
};
```

**아키텍처 무관 (pure SWAR)**  
**목표**: canada **-20%** (2,021→1,600μs)

---

## Phase 34 — AVX2 32B String Scanner (x86_64 전용)

**파일**: `include/beast_json/beast_json.hpp` — Phase 31 SSE2 블록을 AVX2로 업그레이드

aarch64 NEON은 128bit(16B) 최대. 32B는 SVE 필요(M1 미지원).  
x86_64 Haswell 이상은 AVX2 = 256bit(32B) 지원.

```cpp
#if BEAST_HAS_AVX2
const __m256i vq  = _mm256_set1_epi8('"');
const __m256i vbs = _mm256_set1_epi8('\\');
while (p + 32 <= end_) {
  __m256i v     = _mm256_loadu_si256((const __m256i*)p);
  uint32_t mask = (uint32_t)_mm256_movemask_epi8(
                    _mm256_or_si256(_mm256_cmpeq_epi8(v, vq),
                                    _mm256_cmpeq_epi8(v, vbs)));
  if (mask) return p + __builtin_ctz(mask);
  p += 32;
}
// SSE2 16B tail fallback
#endif
```

**목표**: x86_64에서 citm/gsoc **추가 -15%**

---

## Phase 35 — 멀티스레드 병렬 파싱 (압도 결정타)

**이론 "Depth-Bounded Parallel Tape"**:

```
입력 JSON:
{                            ← main thread: ObjectStart push
  "key1": { ... },           ← Thread 0 → TapeArena_0
  "key2": [ ... ],           ← Thread 1 → TapeArena_1
  "key3": { ... }            ← Thread 2 → TapeArena_2
}                            ← main thread: 병합 + ObjectEnd

구현 단계:
1. Pre-scan: SIMD로 depth=1 key offset 배열 생성 O(n/16)
2. Partition: key 수 기준으로 N스레드에 subtree 할당
3. Parallel parse: 각 스레드 독립 TapeArena에 tape 작성 (lock-free)
4. Merge: main thread가 tape pointer 연결 O(N)
```

> [!IMPORTANT]
> Phase 35는 Phase 31-34 완료 후 시작. Pre-scan 정확성이 전제조건.

**목표**: 4코어 기준 이론 4× → 실측 **2-3×** (오버헤드 감안)  
twitter < 120μs, canada < 950μs — **yyjson 대비 30-40% 압도**

---

## 구현 브랜치 전략

| Phase | 브랜치명 | 우선순위 |
|:---|:---|:---:|
| 31 | `feature/phase31-simd-string-gate` | ★★★★★ |
| 32 | `feature/phase32-action-lut` | ★★★★ |
| 33 | `feature/phase33-swar-float` | ★★★★ |
| 34 | `feature/phase34-avx2-string` | ★★★ |
| 35 | `feature/phase35-parallel-parse` | ★★★★★ |

---

## 매 Phase 검증 기준

```bash
# 빌드
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBEAST_JSON_BUILD_TESTS=ON -DBEAST_JSON_BUILD_BENCHMARKS=ON
cmake --build build -j$(sysctl -n hw.ncpu)

# 1. 테스트 (81개 전부 PASS 필수)
ctest --test-dir build --output-on-failure

# 2. 벤치마크 (이전 Phase 대비 regression 없어야 함)
cd build/benchmarks && ./bench_all --all --iter 50

# 3. Git commit
git add include/beast_json/beast_json.hpp
git commit -m "phase3X: ..."
```

> [!CAUTION]
> canada/gsoc/citm에서 regression 발생 시 해당 Phase revert 후 재검토.
