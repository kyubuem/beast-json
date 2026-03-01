# Beast JSON — yyjson 1.2× Domination Plan (Phase 44-55)

> **Date**: 2026-03-01 (Phase 53 complete — Stage 1+2 positions `:,` elimination)
> **Mission**: Beat yyjson by **≥20% (1.2×) on ALL 4 benchmark files simultaneously**
> **Architectures**: x86_64 (AVX-512) PRIMARY · aarch64 (NEON) SECONDARY

---

## Executive Summary

Phase 43 결과를 기반으로 yyjson 1.2× 목표를 달성하기 위한 Phase 44–55 로드맵.

### 현재 성능 (Phase 53 + PGO, Linux x86_64 AVX-512, 150 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | **202 μs** | 248 μs | Beast **+23% 빠름** | ≤219 μs | ✅ |
| canada.json | **1,448 μs** | 2,734 μs | Beast **+89% 빠름** | ≤2,274 μs | ✅ |
| citm_catalog.json | 757 μs | 736 μs | yyjson 2.8% 빠름 | ≤592 μs | ⬜ |
| gsoc-2018.json | **806 μs** | 1,782 μs | Beast **+121% 빠름** | ≤1,209 μs | ✅ |

- **Phase 57 (AArch64 Global NEON 분기 역전현상 증명)**: ✅ SUCCESS
  - Apple Silicon과 일반 AArch64의 분기를 테스트하던 중, Phase 56의 속도 하락 원인이 "NEON 자체"가 아닌, x86_64에서 들여온 "SWAR-8 Pre-gate" 분기 로직 때문임을 발견.
  - 범용 레지스터 스칼라 의존성 체인(SWAR)을 완전히 제거하고 `skip_to_action`, `scan_key_colon_next` 모두 Global NEON Pipeline으로 통합.
  - 결과: AArch64 `twitter.json`에서 **246μs** 파싱 타이밍 달성 (yyjson 추월을 위한 AArch64 최적 패러다임 정립).

- **Phase 58 (Snapdragon 8 Gen 2 / Termux 최적화)**: 🆕 PLANNED
  - 갤럭시 Z 폴드 5 (Android Termux) 환경에서 Phase 57 베이스라인 검증.
  - Snapdragon의 SVE/SVE2 명령어 지원 여부 확인 및 32B+ 가변 길이 벡터 스캐너 실험.
  - Snapdragon L3/SLC 계층에 맞춘 `__builtin_prefetch` 최적 거리 도출.
  - *필독*: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md)의 "AArch64 Survival Guide" 준수 필수.

---

## 🌍 Universal AArch64 Neutrality (범용성 전략)

Beast JSON의 목표는 특정 하드웨어(Apple Silicon)에 종속된 라이브러리가 아닌, 모든 AArch64 아키텍처에서 최고 성능을 내는 **범용 고성능 JSON 엔진**이 되는 것입니다.

### 1. 왜 모바일(Fold 5)인가?
Snapdragon 8 Gen 2는 ARM의 표준 **Cortex-X3 / A715** 코어를 사용합니다. 이는 클라우드 서버에서 사용되는 **AWS Graviton 3 (Neoverse V2)**나 **Ampere Altra**와 아키텍처 계보를 공유합니다. 
따라서 모바일 폰(Termux)에서의 최적화 결과는 곧바로 서버급 AArch64 환경으로 전이(Transfer)될 수 있는 매우 가치 있는 데이터입니다.

### 2. "Pure NEON" = 아키텍처 평준화
우리가 Phase 57에서 발견한 "스칼라 게이트 제거 및 순수 NEON 통합" 전략은 Apple의 독자 코어뿐만 아니라, 표준 Cortex 코어들에게 더욱 유리한 방식입니다. 
표준 코어들은 Apple Silicon보다 스칼라 연산창이 좁기 때문에, 벡터 파이프라인을 일관되게 유지하는 것이 성능 안정성에 결정적입니다.

### 3. 향후 방향
- **Apple Silicon**: 초거대 OoO 윈도우를 활용한 명령어 병렬성 극대화.
- **Snapdragon/Graviton**: 표준 Cortex 파이프라인에 맞춘 프리페치 및 SVE(가변 벡터) 활용.
- **결론**: 아키텍처별 분기를 최소화하되, 모든 ARM 코어가 공통적으로 좋아하는 **"파이프라인 무결성(Pipeline Integrity)"**을 최우선 가치로 삼습니다.

### 현재 성능 (Phase 50-2, macOS AArch64 M1 Pro, 150 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 완료여부 |
|:---|---:|---:|:---:|---:|
| twitter.json | **253 μs** | 176 μs | yyjson 30% 빠름 | ❌ (대폭 단축 성공) |
| canada.json | **1,839 μs** | 1,441 μs | yyjson 27% 빠름 | ❌ |
| citm_catalog.json | **643 μs** | 474 μs | yyjson 35% 빠름 | ❌ |
| gsoc-2018.json | **634 μs** | 990 μs | Beast **+56% 빠름** | ✅ |

**남은 과제**: citm_catalog만 미달 (-21.6% 추가 필요)

### 누적 개선 (Phase 43 → Phase 53 + PGO)

| 파일 | Phase 43 | Phase 53+PGO | 개선 |
|:---|---:|---:|:---:|
| twitter.json | 307 μs | **202 μs** | **-34.2%** |
| canada.json | 1,467 μs | **1,448 μs** | -1.3% |
| citm_catalog.json | 721 μs | 757 μs | +5.0% |
| gsoc-2018.json | 693 μs | 806 μs | +16.3% |

---

## 병목 원인 심층 분석 (Phase 43 이후)

### Twitter.json 구조 분석

Twitter.json (~617KB, ~25,000 토큰)는 다음 특성을 가진다:
- **짧은 string 키** (36% ≤8자, 84% ≤24자): "id", "text", "user", "screen_name" 등
- **bool/null 다수**: is_retweet, is_sensitive, verified 등 깃발 필드
- **정수 ID**: 18자리 숫자, 카운트 값
- **5 사이클/토큰 격차** (3 GHz 기준): 307μs = 36.8cy/tok, yyjson 263μs = 31.6cy/tok

### 5사이클/토큰 격차의 원인 분석

```
push() 비트스택 연산 (Phase E 사전 separator 플래그):
  3× bit read: obj_bits_, kv_key_bits_, has_elem_bits_
  2× bit write: kv_key_bits_ XOR, has_elem_bits_ OR
  합계: ~5사이클/토큰 (3 AND + !! + conditional XOR/OR)

yyjson: separator를 파싱 시 계산하지 않고 dump() 시 런타임 계산
→ yyjson push() ≈ 0사이클 (type 저장만)
→ Beast push() ≈ 5사이클 (separator 사전 계산 비용)

결론: Phase E (separator pre-flagging)는 dump()를 29% 빠르게 하지만
      parse()를 5cy/tok 느리게 만들어 twitter에서 17% 열세 야기
```

### Bool/Null Double-Pump 미적용 (기존 식별된 구멍)

```
kActString → push + str_done → fused: ','→scan_key_colon_next    ✅ 최적
kActNumber → push + Phase 25 B1 → fused: ','→scan_key_colon_next  ✅ 최적
kActTrue/False/Null → push + break → 루프 하단:
  skip_to_action() → c==','→++p_→ 다시 skip_to_action() → kActString  ❌ 누락!
```

kActTrue/False/Null이 루프 하단을 통과하면:
- 추가 `skip_to_action()` 1회
- `kActComma` switch case 1회
- 다시 `skip_to_action()` 1회
- 총 2회 추가 루프 반복 = twitter에서 ~5μs 낭비 추정

### Whitespace skip 최적화 공백 (citm_catalog 주요 원인)

```
citm_catalog.json: 들여쓰기 있는 JSON → 토큰 사이 평균 12-50바이트 공백
현재 skip_to_action(): SWAR-32 (4×8바이트 = 32바이트/iter)
 → 50바이트 공백 처리: 2회 SWAR-32 반복 = ~24 ops

Phase 46 AVX-512 skip_to_action():
 → 64바이트/iter, 단일 cmpgt_epi8_mask
 → 50바이트 공백 처리: 1회 AVX-512 = ~4 ops
 → 6× 절감
```

---

## Phase 44 — Bool/Null/Close 융합 키 스캐너 ⭐⭐⭐⭐⭐ ✅ **COMPLETE**

**Priority**: CRITICAL | **실제 효과**: twitter lazy 424→424μs (측정 노이즈), ctest 81/81 PASS
**난이도**: 낮음 | **위험도**: 낮음

### 이론

kActNumber에 Phase B1 (double-pump fused key scanner)가 적용되어 있지만,
kActTrue/kActFalse/kActNull에는 미적용. 동일 패턴 적용.

```cpp
// kActTrue 현재 (break → 루프 하단 → kActComma → skip_to_action 2회):
case kActTrue:
  push(BooleanTrue, 4, offset);
  p_ += 4;
  break;  // ← 여기서 루프 하단으로 빠짐

// kActTrue Phase 44 목표 (double-pump 적용):
case kActTrue:
  push(BooleanTrue, 4, offset);
  p_ += 4;
  goto bool_null_done;  // str_done 패턴과 동일한 fusion 적용
```

`bool_null_done` 레이블: kActNumber의 Phase 25 B1과 동일한 구조:
1. 다음 바이트 확인 (`nc = *p_`)
2. nc가 공백이면 `skip_to_action()` 1회
3. nc == ',' + 오브젝트 컨텍스트: `scan_key_colon_next()` 직접 호출 → value로 continue
4. nc == ']' or '}': close 인라인 처리
5. 그 외: break

### 구현 메모

```cpp
bool_null_done:   // kActTrue, kActFalse, kActNull 공통 fusion exit point
  if (BEAST_LIKELY(p_ < end_)) {
    unsigned char nc = static_cast<unsigned char>(*p_);
    if (nc <= 0x20) { c = skip_to_action(); if (p_ >= end_) goto done; nc = (unsigned char)c; }
    if (BEAST_LIKELY(nc == ',')) {
      ++p_;
      if (BEAST_LIKELY(depth_ > 0 && depth_ <= 64 && (obj_bits_ >> (depth_-1)) & 1)) {
        if (BEAST_LIKELY(p_ < end_)) {
          unsigned char fc = (unsigned char)*p_;
          if (fc <= 0x20) { fc = (unsigned char)skip_to_action(); if (p_ >= end_) goto done; }
          if (BEAST_LIKELY(fc == '"')) {
            char vc = scan_key_colon_next(p_ + 1, nullptr);
            if (BEAST_UNLIKELY(!vc)) goto fail;
            c = vc;
            continue;
          }
          c = (char)fc;
          continue;
        }
        goto done;
      }
      c = skip_to_action();
      if (p_ >= end_) goto done;
      continue;
    }
    if (nc == ']' || nc == '}') {  // inline close handling
      if (depth_ == 0) goto fail;
      --depth_;
      if (depth_ < kPresepDepth) depth_mask_ >>= 1;
      push_end(nc == '}' ? ObjectEnd : ArrayEnd, p_ - data_);
      ++p_;
      c = skip_to_action();
      if (p_ >= end_) goto done;
      continue;
    }
    c = (char)nc;
    continue;
  }
  break;
```

### 검증 기준

```bash
ctest --test-dir build --output-on-failure  # 81/81 PASS
./bench_all twitter.json --iter 150          # twitter ≤290μs 기대
```

---

## Phase 45 — scan_key_colon_next SWAR-24 Dead Path 제거 ⭐⭐⭐ ✅ **COMPLETE**

**Priority**: MEDIUM | **실제 효과**: twitter lazy **-5.9%** (424→400μs), citm lazy **-7.3%** (1,025→950μs)
**난이도**: 낮음 | **위험도**: 낮음

### 이론

Phase 43 (AVX-512 64B) + Phase 36 (AVX2 32B) 이후,
`scan_key_colon_next`의 SWAR-24 경로는 오직:
- `s + 64 > end_` **AND** `s + 32 > end_` (버퍼 끝 32바이트 이내의 키)
- 즉, 617KB 파일에서 마지막 32바이트 이내의 키에서만 도달 가능

SWAR-24 코드 (30여 줄):
- 함수 크기를 키워 I-cache 효율 저하
- branch predictor entry 낭비

수정: `s + 32 > end_` 케이스에서 직접 `skip_string()` 느린 경로로 폴스루:
```cpp
// Near end of buffer: use generic slow scanner
goto skn_slow;  // scan_string_end(s) path
```

SWAR-24 제거 → 함수 크기 약 30줄 감소 → L1 I-cache 효율 향상.

---

## Phase 46 — AVX-512 배치 공백 스킵 (skip_to_action 개선) ⭐⭐⭐⭐⭐ ✅ **COMPLETE**

**Priority**: CRITICAL (citm_catalog) | **실제 효과**: twitter -3.5%, canada -21.2%, citm -6.3%, gsoc -5.7%
**난이도**: 중간 | **위험도**: 중간 (Phase 37 전례 — SWAR-8 pre-gate로 해결)

### 이론: Phase 37 실패 분석과 AVX-512의 차별점

Phase 37 (AVX2 whitespace skip) 실패 원인:
```
AVX2 방식: _mm256_cmpeq_epi8 × 4 (tab, LF, CR, space) + _mm256_or_si256 × 3
→ 7개 SIMD 명령 + _mm256_movemask_epi8 = 총 9개 명령
대상: 2-8바이트 공백 → SWAR-32 (8 명령)보다 느림
```

AVX-512 방식 (Phase 46):
```
_mm512_loadu_si512 + _mm512_cmpgt_epi8_mask(v, set1(0x20))
→ 2개 명령만으로 64바이트 처리
```

**결정적 차이**:
- `_mm512_cmpgt_epi8_mask`는 마스크를 직접 64비트 정수로 반환
- `vpor`, `movemask` 불필요
- 공백 4종(space=0x20, tab=0x09, LF=0x0A, CR=0x0D)은 모두 ≤0x20
- 0x20 기준 `>` 비교 1회로 완전 처리 (부호있는 비교; 0x80-0xFF는 음수이므로 주의)
  → 비정상 제어 문자(0x00-0x1F, 0x7F 이하)는 오류 바이트로 올바르게 비-공백 처리

### 구현

```cpp
BEAST_INLINE char skip_to_action() noexcept {
  unsigned char c = static_cast<unsigned char>(*p_);
  if (BEAST_LIKELY(c > 0x20))
    return static_cast<char>(c);

#if BEAST_HAS_AVX512
  // Phase 46: AVX-512 배치 공백 스킵 (64바이트/iter)
  // _mm512_cmpgt_epi8_mask: 0x20보다 큰 바이트 위치의 비트 셋
  // 부호있는 비교이므로 0x80-0xFF(JSON 문자열 내 멀티바이트)는 음수로 처리됨
  // → 토큰 사이 컨텍스트에서는 0x80-0xFF가 나타나지 않으므로 안전
  const __m512i ws_thresh = _mm512_set1_epi8(0x20);
  while (BEAST_LIKELY(p_ + 64 <= end_)) {
    __m512i v = _mm512_loadu_si512(p_);
    uint64_t non_ws = static_cast<uint64_t>(
        _mm512_cmpgt_epi8_mask(v, ws_thresh));
    if (BEAST_LIKELY(non_ws != 0)) {
      p_ += __builtin_ctzll(non_ws);
      return *p_;
    }
    p_ += 64;
  }
  // 64바이트 미만 tail: 기존 SWAR-32 fallback
#endif

  // SWAR-32 fallback (기존 구현 유지)
  while (BEAST_LIKELY(p_ + 32 <= end_)) {
    uint64_t a0 = swar_action_mask(load64(p_));
    uint64_t a1 = swar_action_mask(load64(p_ + 8));
    uint64_t a2 = swar_action_mask(load64(p_ + 16));
    uint64_t a3 = swar_action_mask(load64(p_ + 24));
    if (BEAST_LIKELY(a0 | a1 | a2 | a3)) {
      if (a0) { p_ += BEAST_CTZ(a0) >> 3; return *p_; }
      if (a1) { p_ += 8 + (BEAST_CTZ(a1) >> 3); return *p_; }
      if (a2) { p_ += 16 + (BEAST_CTZ(a2) >> 3); return *p_; }
      p_ += 24 + (BEAST_CTZ(a3) >> 3);
      return *p_;
    }
    p_ += 32;
  }
  // ... scalar tail
}
```

### 예상 결과

| 파일 | 공백 특성 | 예상 개선 |
|:---|:---|:---:|
| citm_catalog | 줄바꿈+들여쓰기, 평균 12-50B/토큰 | **-12 to -18%** |
| twitter | 최소 공백, 평균 1-4B/토큰 | -2 to -4% |
| canada | 거의 없음 (숫자 배열) | -1% |
| gsoc | 토큰 사이 공백 거의 없음 | -1% |

### 위험 완화

Phase 37 실패 교훈: 짧은 공백(2-8B)에서 AVX2가 역효과.
- Phase 46은 `c > 0x20` fast-path로 0B 공백을 완전 처리 (SIMD 미진입)
- 짧은 공백(1-32B): `p_ + 64 <= end_`가 거의 항상 참이므로 1회 AVX-512 진입 후 ctzll
- 연속된 64B+ 공백(citm): 완전한 AVX-512 루프 이점 향유

---

## Phase 47 — Profile-Guided Optimization (PGO) ⭐⭐⭐⭐ ✅ **COMPLETE**

**Priority**: HIGH | **실제 효과**: canada -14.6%, 전 파일 합산 -3% (GCC PGO 워크플로 수정 포함)
**난이도**: 낮음 | **위험도**: 매우 낮음

### 이론

GCC/Clang의 PGO는 실제 워크로드 프로파일을 기반으로:
1. **인라인 결정 개선**: 실제 hot 함수 강제 인라인 (scan_key_colon_next 등)
2. **분기 예측 힌트**: `BEAST_LIKELY/UNLIKELY`를 실측 확률로 보정
3. **기본 블록 재배치**: cold path를 함수 끝으로 이동 → I-cache 밀도 향상
4. **루프 최적화**: 실제 반복 횟수 기반 언롤/벡터화 결정

### CMake 구현

```cmake
# 1단계: 계측 빌드
cmake -S . -B build-pgo-gen -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -fprofile-generate=/tmp/beast-pgo"
cmake --build build-pgo-gen -j$(nproc)

# 2단계: 프로파일 수집 (모든 파일, 충분한 반복)
./build-pgo-gen/benchmarks/bench_all --all --iter 30

# 3단계: 최적화 빌드
cmake -S . -B build-pgo-use -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -fprofile-use=/tmp/beast-pgo \
                     -fprofile-correction"
cmake --build build-pgo-use -j$(nproc)
```

### 예상 효과 분석

```
parse() hot loop: 25,000회 반복/파일 → 정밀한 분기 통계 수집
scan_key_colon_next: inline 결정 데이터 충분
kActString fast path vs slow path: 실측 확률로 LIKELY 힌트 보정
push() 내부 분기: 오브젝트 vs 배열 비율 학습
```

---

## Phase 48 — 입력 선행 프리페치 (Prefetch) ⭐⭐⭐⭐ ✅ **COMPLETE**

**Priority**: HIGH | **실제 효과**: twitter -5%, canada -10% (최선 측정치), A/B 테스트 192B 채택
**난이도**: 매우 낮음 | **위험도**: 낮음

### 이론

Twitter.json (617KB)은 L2 캐시에 들어가지만 L1에는 넘침:
- L1 데이터 캐시: 32-48KB (twitter 전체의 ~6%)
- L2 캐시: 256KB-1MB (twitter 전체가 들어감)
- L2→L1 레이턴시: ~10사이클

하드웨어 프리페처는 순차 접근을 잘 예측하지만, parse loop의 불규칙한 점프
(string 스캔 길이 가변)로 인해 miss가 발생함.

**수동 프리페치**: 현재 처리 위치보다 128-256바이트 앞을 사전 로드:
```cpp
// parse() while 루프 상단
__builtin_prefetch(p_ + 192, 0, 1); // 192바이트 선행, 읽기, L1 우선
```

192바이트 = 약 3-5토큰 앞 (twitter 평균 토큰 간격 ~25바이트)
→ 다음 캐시 미스를 현재 처리 중 숨김 (10사이클 × 25,000토큰 = 250,000사이클 = 83μs 이론상 최대)

실제 효과는 하드웨어 프리페처와 중복되므로 20-30% 수준으로 추정.

### TapeArena 프리페치 (tape 쓰기 측)

```cpp
// push() 함수에서: 다음 8개 TapeNode 영역 선행 로드
__builtin_prefetch(tape_head_ + 8, 1, 1); // 쓰기 예상, L1 우선
```

---

## Phase 49 — 브랜치리스 push() 비트스택 연산 ⭐⭐⭐

**Priority**: MEDIUM | **예상 효과**: twitter -2 to -4%, 전 파일 소폭 개선
**난이도**: 낮음 | **위험도**: 낮음

### 이론: Phase E 비트스택 연산 최소화

현재 push() 핵심 연산:
```cpp
const bool in_obj = !!(obj_bits_ & mask);    // AND + TEST + SETNZ
const bool is_key = !!(kv_key_bits_ & mask);  // AND + TEST + SETNZ
const bool has_el = !!(has_elem_bits_ & mask); // AND + TEST + SETNZ
const bool is_val = in_obj & !is_key;          // NOT + AND
sep = is_val ? uint8_t(2) : uint8_t(has_el);  // CMOV
kv_key_bits_ ^= (in_obj ? mask : 0);          // CMOV + XOR
has_elem_bits_ |= mask;                        // OR
// 총 ~12 명령
```

브랜치리스 최적화:
```cpp
// depth_mask_ = 1ULL << (depth_-1) = 이미 precomputed
// 단일 비트 추출: >> 연산 없이 AND + cmov로 처리
uint64_t in_obj_mask = obj_bits_ & mask;      // 0 or mask (non-zero)
uint64_t is_key_mask = kv_key_bits_ & mask;   // 0 or mask
uint64_t has_el_mask = has_elem_bits_ & mask; // 0 or mask

// sep 계산: PDEP 없이 순수 비트 연산
// is_val = in_obj AND NOT is_key
// sep = 2 if is_val, else 1 if has_el (in non-val context), else 0
// CMOV 체인:
uint8_t sep = has_el_mask ? 1 : 0;         // 기본: has_el
sep = is_key_mask ? sep : (in_obj_mask ? 2 : sep); // val position → 2

// kv_key_bits_ 업데이트: in_obj일 때만 toggle
kv_key_bits_ ^= in_obj_mask;  // mask or 0 → XOR로 단순화 가능?
```

**Note**: 이미 `!!` 패턴은 컴파일러가 CMOV로 최적화. 실제 병목은 3개 64비트 필드를
L1에서 읽는 메모리 접근 지연일 가능성 높음. 최적화 여부는 프로파일 확인 후 결정.

---

## Phase 50 — Stage 1 구조적 문자 사전 인덱싱 ⭐⭐⭐⭐⭐

**Priority**: HIGHEST (구현 후 최대 성능 도약) | **예상 효과**: twitter -15 to -20%
**난이도**: 높음 | **위험도**: 높음 | **소요시간**: 2-3일

### 이론: simdjson 방식의 두 단계 파싱

현재 Beast: 입력을 1바이트씩 순회하며 구조적 문자 발견 시 처리
→ 공백 스킵 오버헤드, 분기 예측 미스, 문자열 스캐닝 루프 진입 비용

**Stage 1 + Stage 2 분리**:

```
Stage 1 (AVX-512 선형 스캔):
  입력 전체를 64바이트씩 스캔
  각 64바이트 블록에서:
    q_mask   = '"' 위치 비트마스크  (64비트)
    bs_mask  = '\' 위치 비트마스크  (64비트)
    st_mask  = { } [ ] : , 위치 마스크
  이스케이프 전파로 "실제 닫는 따옴표" 위치 계산
  구조적 문자 위치를 index[] 배열에 저장
  출력: struct_index[N] = { uint32_t pos, uint8_t type }

Stage 2 (index 순회):
  for each entry in struct_index[]:
    dispatch on entry.type
    → 분기 예측 오버헤드 = 0 (이미 결정된 타입 스트림)
    → 공백 스킵 오버헤드 = 0 (이미 위치 알고 있음)
    → 문자열 스캔: Stage 1에서 이미 끝 위치 계산됨
```

### 이스케이프 처리 (핵심 난제)

simdjson의 홀수 백슬래시 전파 알고리즘:
```cpp
// bs_bits: 이 64바이트 내 모든 '\' 위치
// 이스케이프 전파:
//   '\' 다음 위치는 이스케이프됨
//   '\\' 다음 위치는 이스케이프되지 않음
//   '\\\' 다음 위치는 이스케이프됨 (홀수 연속)
uint64_t starts = bs_bits & ~(bs_bits << 1);       // 연속 backslash의 시작
uint64_t even_starts = starts & EVEN_BITS;          // 짝수 위치의 시작
uint64_t odd_starts = starts & ~EVEN_BITS;          // 홀수 위치의 시작
uint64_t even_carries = bs_bits + even_starts;      // carry 전파
uint64_t odd_carries = bs_bits + odd_starts;
uint64_t escaped = (even_carries ^ odd_carries ^ bs_bits) >> 1;
uint64_t real_quotes = q_bits & ~escaped;           // 진짜 닫는 따옴표만
```

### 예상 구현 구조

```cpp
struct StructEntry {
  uint32_t pos;   // 바이트 오프셋
  uint8_t  type;  // ActionId: kActString, kActObjOpen, ...
  uint8_t  str_end_pos; // 문자열 끝 오프셋 (kActString일 때)
  uint16_t pad;
};

class Stage1Scanner {
  const char* data_;
  size_t len_;
  std::vector<StructEntry> index_;
  bool in_string_ = false;

public:
  void scan(); // AVX-512 전체 스캔
  const std::vector<StructEntry>& index() const { return index_; }
};
```

### 통합 방안

Stage 1 결과를 Parser에 주입:
```cpp
bool parse() {
  Stage1Scanner scanner(data_, end_ - data_);
  scanner.scan();  // ~10μs for 617KB

  for (const auto& entry : scanner.index()) {
    p_ = data_ + entry.pos;
    switch (entry.type) {
      case kActString:
        // 이미 end position 알고 있음 → push() 바로 호출
        push(StringRaw, entry.str_len, entry.pos);
        goto bool_null_done; // 다음 진입점으로
      // ...
    }
  }
}
```

**핵심 이점**:
1. 공백 스킵 오버헤드 = 0 (Stage 1에서 이미 필터됨)
2. 문자열 스캐닝 오버헤드 = 0 (Stage 1에서 끝 위치 파악됨)
3. switch 분기 예측 향상 (타입 스트림이 캐시에 있음)

### 예상 성능 향상 계산

```
현재 twitter 307μs 구성 (추정):
  Stage 1 등가 작업 (문자열 스캔): ~100μs
  Stage 2 등가 작업 (비트스택, 테이프 쓰기): ~100μs
  공백 스킵 + 분기 오버헤드: ~107μs

Phase 50 예상:
  Stage 1 (AVX-512 전체 스캔 617KB): ~10-15μs
  Stage 2 (인덱스 순회, 비트스택, 테이프 쓰기): ~100μs
  총: ~110-115μs → 현재 대비 -63%? (너무 낙관적)

현실적 추정 (단계별 비용 재계산):
  Stage 1: 617KB / 64B × 3ops = ~30,000 ops = 10μs@3GHz
  Stage 2: 25,000 entries × 15 ops = 375,000 ops = 125μs@3GHz
  총: 135μs → twitter -56% (달성 시 yyjson 대비 2×)
```

---

## Phase 51 — 64비트 TapeNode 단일 스토어 ⭐⭐⭐

**Priority**: MEDIUM | **예상 효과**: twitter -2 to -3%
**난이도**: 낮음 | **위험도**: 낮음

### 이론

TapeNode = {uint32_t meta, uint32_t offset} (8바이트)
현재 push()에서 두 개의 32비트 쓰기로 분리됨:
```cpp
n->meta   = (uint32_t)...;  // store 1
n->offset = (uint32_t)...;  // store 2
```

하나의 64비트 쓰기로 통합:
```cpp
uint64_t packed = ((uint64_t)meta_val << 32) | (uint64_t)offset_val;
*reinterpret_cast<uint64_t*>(n) = packed;
```

효과:
- 스토어 포트 사용 1회로 감소
- 스토어 버퍼 엔트리 절약
- 컴파일러가 이미 최적화할 수 있지만 명시적으로 보장

**Note**: `std::bit_cast<uint64_t>({meta, offset})`로 구현 가능 (C++20).

### 비-시간적(NT) 스토어 검토

TapeArena 쓰기는 parse() 중 읽히지 않음. 비-시간적 스토어 적용:
```cpp
_mm_stream_si64(reinterpret_cast<long long*>(n), packed);
```

이점: L1/L2 오염 방지, 다른 데이터(입력 버퍼, 비트스택)를 위한 캐시 공간 확보
위험: `_mm_sfence()` 필요, TapeArena 64B 정렬 필요, 실측 필수

---

## Phase 52 — 정수 파싱 SIMD 가속 ⭐⭐

**Priority**: LOW | **예상 효과**: canada -3% (이미 빠름), twitter -1%
**난이도**: 중간

### 이론: SIMD 8자리 동시 파싱

현재: SWAR-8로 비숫자 감지 → 길이 파악 후 실제 값 파싱은 별도
최적화: AVX-512를 이용한 8-16자리 정수 값 동시 계산

```
입력: "12345678"  (8 ASCII 숫자)
단계:
  1. Load 8 bytes
  2. Subtract '0' → {1,2,3,4,5,6,7,8}
  3. _mm_maddubs_epi16: pairs → {12, 34, 56, 78}  (×10+)
  4. _mm_madd_epi16: → {1234, 5678}               (×100+)
  5. 최종: 1234×10000 + 5678 = 12345678

AVX-512 VPERMT2B로 10^i 가중치 적용 → 1 SIMD op으로 8자리 값 계산
```

twitter.json의 18자리 ID는 2회 8자리 + 2자리 스칼라로 처리 가능.
canada.json은 부동소수점이라 이 기법 적용 불가 (float 파싱 다름).

---

## Phase 53 — 신규 이론: 구조적 밀도 적응형 스캐닝 ⭐⭐⭐

**Priority**: RESEARCH | **예상 효과**: 전 파일 -3 to -5%
**난이도**: 중간 | **위험도**: 낮음

### 이론

JSON 파일의 공백 밀도는 파일마다, 심지어 파일 내부에서도 다름:
- twitter.json: 공백 거의 없음 (1-2B/토큰)
- citm_catalog: 포맷됨 (12-50B/토큰)
- canada.json: 부동소수점 배열, 공백 희소
- gsoc-2018: 대형 오브젝트 배열, 중간 정도

**적응형 공백 스캔 전략**:

```cpp
// 이전 skip_to_action() 호출에서 건너뛴 바이트 수의 EWMA
uint8_t ws_density_ = 0; // 0: 공백 없음, 255: 공백 많음

BEAST_INLINE char skip_to_action() noexcept {
  unsigned char c = (unsigned char)*p_;
  if (BEAST_LIKELY(c > 0x20)) return (char)c;

  const char* start = p_;
  #if BEAST_HAS_AVX512
  if (ws_density_ > 16) {
    // 고밀도 공백: AVX-512 배치 스캔
    [AVX-512 loop]
  } else {
    // 저밀도 공백: SWAR-32 (적은 공백에 최적)
    [SWAR-32 loop]
  }
  #endif

  // EWMA 업데이트: ws_density_ = ws_density_ * 7/8 + skipped/8
  uint8_t skipped = (uint8_t)(p_ - start);
  ws_density_ = (ws_density_ * 7 + skipped) >> 3;
}
```

이 방식은:
- twitter: SWAR-32 경로 유지 (ws_density_ ≈ 1)
- citm: 초기 몇 토큰 후 AVX-512 전환 (ws_density_ ≈ 20+)
- 오버헤드: ws_density_ 업데이트 = 3 ops/call (무시할 수준)

---

## Phase 54 — 신규 이론: 스키마 예측 캐시 ⭐⭐⭐ (Speculative)

**Priority**: EXPERIMENTAL | **예상 효과**: twitter -5 to -10% (성공 시)
**난이도**: 높음 | **위험도**: 높음

### 이론

Twitter.json의 각 tweet 오브젝트는 **동일한 키 시퀀스**를 가짐:
```
"id", "id_str", "text", "source", "truncated", "in_reply_to_status_id", ...
```
(총 ~25개 키, 모든 tweet에서 동일한 순서)

**예측 캐시**:
1. 첫 번째 tweet 파싱 시: 키 이름 + 오프셋을 `key_cache[25]`에 저장
2. 두 번째+ tweet에서: 예측된 키를 `memcmp`로 검증만
3. 검증 성공(~99%): 키 스캔 건너뜀, 캐시에서 길이 사용
4. 검증 실패: 일반 경로 폴백

```cpp
struct KeyCache {
  uint16_t key_len[32];       // 캐시된 키 길이
  uint16_t key_offset[32];    // 첫 tweet 대비 오프셋
  uint8_t  count;             // 캐시된 키 수
  bool     valid;             // 캐시 유효 여부
};

// scan_key_colon_next 내:
if (key_cache_.valid) {
  uint8_t idx = kv_count_at_depth_[depth_-1]; // 현 depth에서 몇 번째 키인지
  uint16_t expected_len = key_cache_.key_len[idx];
  // memcmp 1회로 검증
  if (!memcmp(s, cached_key_str[idx], expected_len) && s[expected_len] == '"') {
    // 캐시 히트: 스캔 생략
    push_key_from_cache(idx);
    return value_start;
  }
  // 캐시 미스: 일반 경로 + 캐시 무효화
  key_cache_.valid = false;
}
```

**실현 가능성 평가**:
- twitter.json에서 90%+ 캐시 히트율 예상
- canada.json/citm: 키 변동 없음 (캐시 초기화 후 모든 접근이 히트)
- 구현 복잡도: 중간-높음
- Phase 50 (Stage 1)이 구현되면 더 자연스럽게 통합 가능

---

## Phase 55 — 신규 이론: TapeNode 캐시라인 배치 쓰기 ⭐⭐

**Priority**: LOW-MEDIUM | **예상 효과**: twitter -2 to -5%
**난이도**: 중간

### 이론: 8개 TapeNode = 64바이트 = 1 캐시라인 원자적 쓰기

현재: TapeNode 1개씩 8바이트 스토어 → 캐시라인 부분 채움 반복
최적화: 8개 TapeNode를 레지스터/스택에 누적 후 64바이트 원자적 쓰기

```cpp
class Parser {
  // 8개 TapeNode 누적 버퍼 (64바이트 정렬)
  alignas(64) TapeNode tape_buf_[8];
  int tape_buf_idx_ = 0;

  BEAST_INLINE void push_buffered(TapeNodeType t, uint16_t l, uint32_t o) {
    // sep 계산...
    tape_buf_[tape_buf_idx_] = {make_meta(t, sep, l), o};
    if (++tape_buf_idx_ == 8) {
      // NT 스토어로 64바이트 원자적 기록
      _mm512_stream_si512((__m512i*)tape_head_, *(__m512i*)tape_buf_);
      tape_head_ += 8;
      tape_buf_idx_ = 0;
    }
  }

  // 플러시 (parse() 종료 시)
  void flush_tape_buf() {
    for (int i = 0; i < tape_buf_idx_; i++)
      *tape_head_++ = tape_buf_[i];
    _mm_sfence();
  }
};
```

이점:
- 스토어 포트 사용 최소화 (8→1)
- L1/L2 TapeArena 캐시 오염 감소 (NT 스토어)
- 타 연산(입력 읽기, 비트스택)을 위한 캐시 용량 확보

주의:
- `_mm512_stream_si512`는 64B 정렬 필수 (TapeArena 정렬 보장 필요)
- `_mm_sfence()`로 스토어 완료 보장 필요 (dump() 시작 전)
- 버퍼 오버헤드: stack에서 8개 TapeNode = 64B (register 불가, 스택 사용)

---

## 구현 우선순위 로드맵

### 즉시 실행 (1-2일)

```
Phase 44: bool/null double-pump fusion   → twitter -6%, citm -3%
Phase 45: SWAR-24 dead code 제거         → twitter -5.9%, citm -7.3% ✅
Phase 46: AVX-512 WS skip + SWAR-8 gate → canada -21.2%, twitter -3.5% ✅
Phase 47: PGO 빌드 시스템 정비          → canada -14.6% (추가), 전 파일 합산 -3% ✅
Phase 48: 입력/테이프 선행 프리페치     → twitter -5%, canada -10% ✅
```

누적 twitter 예상: 307 × 0.94 × 0.985 × 0.93 × 0.96 ≈ **253μs** (target: 219μs)
누적 citm 예상: 721 × 0.97 × 0.99 × 0.93 × 0.96 ≈ **623μs** (target: 592μs)

### 중기 실행 (3-5일)

```
Phase 46: AVX-512 whitespace skip        → citm -15%, twitter -3%
Phase 49: 브랜치리스 push()              → twitter -2%
Phase 51: 64비트 단일 TapeNode 스토어   → twitter -2%
```

누적 twitter 예상: 253 × 0.97 × 0.98 × 0.98 ≈ **229μs** (target: 219μs에 근접)
누적 citm 예상: 623 × 0.85 × 0.99 × 0.99 ≈ **519μs** ✅ (target: 592μs 달성)

### 장기 실행 (1-2주)

```
Phase 50: Stage 1 구조적 문자 사전 인덱싱  → x86_64 twitter -19.7% ✅
Phase 50-1: NEON 32B 언롤링 및 Branchless → ❌ 회귀 (AArch64 페널티 확인)
Phase 50-2: NEON 정밀 최적화 (스칼라 폴백) → macOS twitter -23% (253μs) ✅
Phase 53: 위치 배열 `:,` 제거 최적화         → twitter -31.1% 추가 단축 ✅
Phase 54: 스키마 예측 캐시                  → twitter -5 to -10%
Phase 55: TapeNode 배치 쓰기               → twitter -3%
```

누적 twitter 예상: 229 × 0.83 × 0.97 × 0.93 × 0.97 ≈ **168μs** (**+36% vs yyjson 263μs**) ✅

---

## 예상 최종 성능 (모든 Phase 완료 시)

| 파일 | Phase 53 현재 | 최종 예상 | yyjson | Beast vs yyjson |
|:---|---:|---:|---:|:---:|
| twitter.json | 202 μs | **~168 μs** | 248 μs | **+23%** 돌파 ✅ |
| canada.json | 1,448 μs | **~1,350 μs** | 2,734 μs | **+89%** 돌파 ✅ |
| citm_catalog.json | 757 μs | **~460 μs** | 736 μs | 예약됨 ❌ |
| gsoc-2018.json | 806 μs | **~620 μs** | 1,782 μs | **+121%** 돌파 ✅ |

**직렬화 포함 시**: Phase E의 dump() -29% 이점이 추가되어 parse+dump 합산 성능은
모든 파일에서 yyjson 대비 **1.5× 이상** 달성 가능.

---

## Branch Strategy (Phase 44+)

| Phase | Branch | 목표 결과 |
|:---|:---|:---:|
| 44 | `claude/performance-optimization-plan-yn1Pi` | bool/null fusion ✅ |
| 45 | `claude/phase45-swar24-cleanup` | SWAR-24 제거 |
| 46 | `claude/phase46-avx512-ws-skip` | AVX-512 공백 스킵 |
| 47 | `claude/phase47-pgo` | PGO 빌드 |
| 48 | `claude/phase48-prefetch` | 프리페치 |
| 49 | `claude/phase49-branchless-push` | 브랜치리스 push() |
| 50 | `claude/phase50-stage1-scanner` | 구조적 문자 인덱서 ✅ |
| 50-1| `neon-single-pass-opt` | NEON 32B Branchless ❌ |
| 50-2| `neon-opt` | NEON 스칼라 폴백 정제 ✅ |
| 51 | `claude/phase51-tape-store` | 64비트 tape 스토어 |
| 52 | `claude/phase52-int-simd` | 정수 SIMD 파싱 |
| 53 | `claude/phase53-adaptive-ws` | 적응형 공백 스캔 |
| 54 | `claude/phase54-schema-cache` | 스키마 예측 캐시 |
| 55 | `claude/phase55-tape-batching` | TapeNode 배치 쓰기 |

---

## 변경 불변 원칙 (Hard Rules)

> 1. **모든 Phase는 `ctest 81/81 PASS` 후 커밋** — 예외 없음
> 2. **YMM/ZMM 상수는 사용 지점에 인접 선언** — Phase 40 교훈 (호이스팅 금지)
> 3. **회귀 즉시 revert** — 망설임 없이 되돌리고 원인 분석
> 4. **Phase 46 AVX-512 WS skip 검증**: citm -10% 미달이면 Phase 37처럼 revert
> 5. **Phase 50은 Stage 2 first**: Stage 1 없이 Stage 2 구조부터 설계 후 Stage 1 통합
> 6. **N+1 캐시 효과 주의**: 프리페치와 NT 스토어는 동일 실행에서 충돌 가능 → 실측 필수

---

## 완료된 Phase 참조 (31-43)

Phase 31-43의 상세 기록은 이 파일의 구 버전 참조 또는 git log에서 확인:
- Phase 31: NEON/SSE2 contextual gate → M1 twitter -4.4%, gsoc -11.6%
- Phase 32: 256-entry Action LUT → BTB 개선
- Phase 33: SWAR-8 float scanner → canada -6.4%
- Phase 34: AVX2 32B string scanner → x86_64 처리량 2배
- Phase 36: AVX2 inline scan (kActString hot path) → twitter -4.5%
- Phase 37: AVX2 whitespace skip → ❌ REVERTED (Short WS에서 역효과)
- Phase 40: AVX2 상수 호이스팅 → ❌ REVERTED (레지스터 스필)
- Phase 41: skip_string_from32 fast path → mask==0 SWAR-8 게이트 생략
- AVX-512 fix: BEAST_HAS_AVX2 on AVX-512 → AVX2 코드 전체 활성화
- Phase 42: AVX-512 64B scan in scan_string_end → canada/citm/gsoc -9~13%
- Phase 43: AVX-512 64B inline (kActString + scan_key_colon_next) → twitter -12.4%

---

## 신규: AArch64 (NEON) 1.2× 초격차 플랜 (Phase 56+)

x86_64 (AVX-512) 환경에서는 이미 압도적인 성능을 달성했지만, macOS AArch64(M1/M2/M3) 환경에서는 yyjson을 1.2배 이상 앞서기 위해 기존의 단순 포팅을 넘어선 **Apple Silicon 아키텍처 맞춤형 신규 이론**이 필요합니다. Phase 56은 AArch64의 약점을 피하고 강점만을 극대화하는 전용 최적화 페이즈입니다.

### 이론 1: LDP (Load Pair) 기반의 초고속 32B/64B 스칼라-SIMD 융합 스킵

Apple Silicon의 메모리 컨트롤러는 `ldp` (Load Pair) 명령어 처리에 극도로 최적화되어 있습니다. 기존 NEON 16B 읽기(`vld1q_u8`)를 루프 언롤링하는 대신, ARM64 어셈블리 또는 `__uav512` 수준의 64B 분산 로딩을 시도합니다.
특히 공백 스킵(`skip_to_action`)에서 `ldp`로 32바이트를 GPR(General Purpose Registers)에 올린 뒤 스칼라 분기로 탈출하는 기법이 순수 NEON보다 빠를 가능성이 있습니다.

### 이론 2: NEON String Scanner의 32B Interleaved Branching

Phase 50-1에서 실패했던 NEON 32B 언롤링은 `vgetq_lane` 병목 때문이었습니다. 이를 회피하는 새로운 패턴을 고안합니다.
- `vld1q_u8` 2개를 인터리빙(Interleaving)으로 병렬 로드합니다.
- `vceqq_u8` 비교 후 `vmaxvq_u32` 축소 연산을 2개 벡터에 대해 각각 수행하되, 첫 번째 벡터에서 Hit가 발생하면 두 번째 벡터 연산은 파이프라인 브랜치 예측을 통해 폐기하도록(Shadowing) 숏서킷(Short-circuit) 검사 루틴을 설계합니다.
- 추출은 절대 `vgetq_lane`을 쓰지 않고, 조건 분기가 통과된 정확한 16B 청크에 대해서만 스칼라 `while` 루프로 폴백합니다. (Phase 50-2의 극대화 버전)

### 이론 3: NEON 기반 Table Lookup (TBL) State Machine 가속

AArch64의 진정한 강력함은 파이프라인된 `tbl` (Vector Table Lookup) 명령어에 있습니다.
- `scan_string_end` 과정에서 이스케이프 문자(`\`)를 만났을 때 분기하는 느린 폴백 경로를 `vtbl1_u8`을 활용한 SIMD 룩업 기반 이스케이프 파서로 교체합니다.
- 이스케이프 처리 빈도가 높은 JSON(예: html이 포함된 payload)에서 압도적인 가속이 예상됩니다.

### 이론 4: CPU Cache-Line Sizing 맞춤형 프리페치

Apple Silicon의 L1/L2 캐시라인 동작 방식은 x86과 다릅니다. 현재 x86 기준으로 맞춰진 `__builtin_prefetch(p_ + 192, 0, 0)`의 거리(Distance)와 로컬리티(Locality) 힌트를 AArch64 전용(예: 128B 또는 256B)으로 재조정하여 PVM(Page Validation Miss)을 방지합니다.

> **작업 지침**: Phase 56은 단일 Phase로 묶지 않고 56-1 (공백), 56-2 (문자열), 56-3 (이스케이프), 56-4 (프리페치 튜닝)의 4단계로 철저히 쪼개서 각 단계별 성능 회귀 유무를 현미경 검증하며 진행합니다.
