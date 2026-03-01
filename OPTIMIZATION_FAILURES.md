# Architecture Optimization Failure Log

> **마지막 업데이트**: 2026-03-01 (Phase 49/51/52 x86_64 회귀 사례 추가)
> 이 문서는 Beast JSON 라이브러리 개발 중 각 아키텍처(x86_64, aarch64 등)별로 시도되었으나 실패(성능 회귀, Regression)한 최적화 사례와 그 원인을 면밀히 분석한 자료입니다.
> 에이전트는 새로운 SIMD 최적화를 시도하기 전에 **반드시 이 문서를 숙지**하여 동일한 실수를 반복하지 않도록 해야 합니다.

---

## 1. aarch64 (NEON) 최적화 회귀 사례

### ❌ [Phase 49 시도] NEON 64B (4×16B) 구조적 스캐너 및 공백 스킵 처리
* **목표**: x86_64의 AVX-512 64B 청크 단위 스캐닝(`_mm512_loadu_si512` 등) 성능 개선을 벤치마킹하여, NEON에서도 16바이트(`uint8x16_t`) 4개를 한 번에 언롤링(Unrolling)하여 64B 단위로 처리.
* **적용 대상**: `skip_to_action()`, `scan_string_end()`, `scan_key_colon_next()`
* **실패 결과 (성능 대폭 하락)**: 
  * `twitter.json`: 269μs -> 360μs (+33% 회귀)
  * `citm_catalog.json`: 639μs -> 1037μs (+62% 회귀)
  * `gsoc-2018.json`: 627μs -> 885μs (+41% 회귀)
* **근본 원인 분석**:
  1. **명령어 집합의 근본적 차이**: AVX-512는 64바이트를 '단 하나의 명령어'(`__m512i`)로 로드/비교/비트마스크 변환이 가능합니다. 하지만 NEON에는 64바이트 단일 벡터 레지스터가 존재하지 않습니다. 
  2. **레지스터 스필(Register Spill) 및 중첩 연산 과부하**: 로드(`vld1q_u8`) × 4번, 비교(`vcgtq_u8`, `vceqq_u8`) × 4번 이상, 합산(`vorrq_u8`) 연속 3번 이상이 핫 루프에 추가되면서 컴파일러의 레지스터 할당 한계를 초과하여 CPU 파이프라인이 심각하게 스톨(Stall)되었습니다.
  3. **비트마스크 병목**: AVX는 `_mm512_cmpgt_epi8_mask` 하나로 64비트 정수(mask)를 바로 뽑아내어 `__builtin_ctzll`로 분기가 빠릅니다. 반면 NEON에는 기본 `movemask` 명령이 없어 `vmaxvq_u32`로 1차 판별 후 `neon_movemask()` 유틸리티 함수(시프트, 곱 연산, 리덕션 트리 사용)를 추가 호출해야 하므로 4개의 16B 마스크를 OR로 모아서 처리하는 과정의 연산 비용이 루프를 4번 도는 비용보다 훨씬 컸습니다.
  4. **진입 비용(Startup Overhead) 극대화**: `twitter.json` 등은 문자열이나 공백의 런(run) 스팬이 매우 짧습니다. 64B 크기를 뭉텅이로 처리하기 전 진입하는 셋업/조건 비용 자체가 커서, 오히려 16바이트로 빠르게 스킵하고 즉각 탈출하는 것이 aarch64 아키텍처에서 가장 효율적인 파레토 최적 지점이었습니다.
* **가이드라인 (에이전트 수칙)**:
  * **AArch64 에이전트**: NEON에서 32바이트 이상을 루프 언롤링하여 억지로 처리하려 하지 마세요. NEON 환경은 `uint8x16_t`를 1~2개 처리하는 수준에서 분기를 최소화하는 루프가 최적입니다. x86_64의 AVX-512 극단적 기법을 NEON에 1:1로 이식하려 시도하지 마세요.

### ❌ [Phase 50 시도] NEON Stage 1 구조적 문자 사전 인덱싱 (Two-Phase Parsing)
* **목표**: AVX-512에서 45% (365μs → 202μs) 파싱 속도 향상을 입증한 `stage1_scan_avx512`를 모방하여, NEON에서도 `stage1_scan_neon`을 구현하고 `parse_staged`로 데이터 처리 구조 변경. 64B 단위로 `vld1q_u8` 4회 언롤링 스코프 사용.
* **적용 대상**: `beast_json.hpp` 내 `stage1_scan_neon` 및 `parse_reuse` 라우팅 로직
* **실패 결과 (성능 회귀)**: 
  * `twitter.json`: 328μs → 478μs (**+45% 회귀**)
  * `citm_catalog.json`: ~645μs → 933μs (**+44% 회귀**)
* **근본 원인 분석**:
  1. **Neon_movemask의 과도한 오버헤드**: AVX-512에서는 `_mm512_cmp*_mask` 명령 하나로 64비트 정수 마스크가 바로 도출됩니다. 반면 NEON에서는 16바이트 청크마다 `vshrq_n_u8`, `vmulq_u8`, `vpaddlq_*` 등의 리덕션 트리를 타고 내려와야 하는 고비용의 소프트웨어 에뮬레이션(`neon_movemask`)이 필요합니다.
  2. **1루프 당 20개 이상의 movemask 호출 비용 발생**: 루프 1번당 4번의 16B 청크를 처리하고, 청크 하나당 Quote, Backslash, Brackets, Separators, Whitespace의 5개 마스크를 생성해야 하므로 총 20개의 `neon_movemask` 호출이 64B를 스캔할 때마다 발생합니다.
  3. **비효율적 아키텍처 매핑**: AArch64 (M1 등) 프로세서는 파이프라인과 비순차 실행(Out-of-order execution) 및 분기 예측(Branch Prediction) 성능이 매우 뛰어납니다. 64B 단위로 무거운 산술 연산을 욱여넣는 "SIMD 내 인덱싱" 기법보다, 짧은 바이트를 빠르게 읽고 루프로 바로 건너뛰는 기존 "Single Pass 선형 파서"가 더 오버헤드 없이 잘 동작합니다.
* **가이드라인 (에이전트 수칙)**:
  * **AArch64 에이전트**: SIMD "마스킹" 기반의 Two-Phase 처리 (미리 구조적 문자의 인덱스를 뽑고 나중에 파싱하는 simdjson 류의 방식)는 ARM64 환경에 적합하지 않습니다. NEON에는 Native MoveMask가 없다는 점을 항상 기억하고, 단일 경로 선형 파서 최적화에 집중하세요.

### ❌ [Phase 50-1 시도] NEON Single-Pass 스캐너 32B 언롤링 및 Branchless Pinpoint
* **목표**: `skip_to_action()`과 `scan_string_end()`의 NEON 루프에서 16B 단위 처리를 32B 단위(2× `vld1q_u8`)로 언롤링하고, 발견 시 `while`문을 통한 스칼라 스캔이 아닌 `vgetq_lane_u64(_m, 0)` 추출 및 `CTZ`를 사용해 브랜치리스(Branchless)하게 오프셋을 계산.
* **적용 대상**: `beast_json.hpp` 내 `skip_to_action()`, `scan_string_end()` 의 `#elif BEAST_HAS_NEON` 조건부 컴파일 블록.
* **실패 결과 (성능 회귀)**: 
  * `twitter.json`: 328μs → 357μs (**+8.8% 회귀**)
  * `citm_catalog.json`: 645μs → 839μs (**+30% 회귀**)
* **근본 원인 분석**:
  1. **AArch64의 브랜치 친화성**: x86_64 환경에서 극단적으로 회피해야 할 `while` 루프(스칼라 바이트 비교)가, 오히려 AArch64에서는 거대한 패널티 없이 아주 효율적으로 동작했습니다.
  2. **NEON Lane Extraction(vgetq_lane)의 고비용**: NEON 레지스터(`uint8x16_t` / `uint64x2_t`)에서 일반 범용 레지스터(GPR)로 `vgetq_lane_u64`를 사용해 데이터를 넘기는 과정(Cross-register file transfer)은 지연 속도(Latency)가 높습니다. `vmaxvq_u32`로 히트를 감지한 뒤 `vgetq_lane` 두 번과 `CTZ`를 호출하는 브랜치리스 코드가, 차라리 바이트 포인터 연산을 1바이트씩 N번 루프 도는 것보다 현격하게 느렸습니다.
  3. **과도한 루프 언롤링**: 32B 언롤링은 16개 이상의 레지스터를 여유 있게 사용할 때 좋지만, 문자열 스캐닝 특성 상 16B 만에 바로 탈출조건이 만족되는 경우가 대다수입니다. 필요 없이 16B를 추가 로드(`load` / `ceq` / `orr`) 하는 작업이 오히려 16B 이내 탈출 패턴인 일반 JSON 구조를 지연시켰습니다.
* **가이드라인 (에이전트 수칙)**:
  * **AArch64 에이전트**: NEON 스캐너 구현 시 '히트(조건 만족)' 감지 이후 위치를 좁혀야 할 때, `vgetq_lane`을 이용해 마스크를 빼내어 `CTZ`를 돌리지 마세요. 차라리 스칼라 `while` 루프로 해당 청크(16B) 내부를 바이트 단위로 찾는 것이 AArch64에서는 압도적으로 빠릅니다. 또한 핫 패스의 문자열/공백 찾기는 16B(`vld1q_u8` 1회) 단위로 가장 기민하게 처리하는 것이 최고 효율을 냅니다.

---

## 2. x86_64 (AVX2 / AVX-512) 최적화 회귀 사례

### ❌ [Phase 37] AVX2 공백 스킵 스캐닝 (Whitespace Skip)
* **목표**: `skip_to_action()` 함수에서 SWAR-32/SWAR-8 대신 `__m256i` 기반 AVX2 최적화를 도입하여 공백 무시 속도를 극대화.
* **실패 결과**: 전체적으로 **+13% 파싱 시간 증가 (성능 하락)**.
* **근본 원인 분석**:
  * 공백 길이에 대한 JSON 데이터의 '분포'를 간과했습니다. 대부분의 JSON 파일(특히 `twitter.json`)에서 토큰 사이의 공백은 고작 1~8바이트 수준에 불과합니다.
  * AVX2 레지스터에 값을 로딩(set)하고 `_mm256_movemask_epi8` 결과를 뽑아내어 `ctz`를 수행하는 진입 지연(Setup/Teardown Overhead)이, 고작 평균 2~3바이트의 공백을 스킵하는 이득보다 수 배 이상 컸습니다.
* **해결법 (Phase 46)**: AVX-512 64B 스킵퍼 도입 시, **반드시 직전에 SWAR-8(8바이트 체크) Pre-gate를 배치**하여, 8바이트 미만의 공백은 SIMD 레지스터를 깨우지 않고 스칼라단에서 즉시 처리 후 탈출하게 만들었습니다. 

### ❌ [Phase 40] AVX2 벡터 상수 호이스팅 (Constant Hoisting)
* **목표**: SIMD 스캐너 내부에서 매 루프 선언하는 상수 벡터(`_mm256_set1_epi8('"')` 등)를 루프 바깥 혹은 클래스 멤버로 호이스팅(끌어올림)하여 생성 오버헤드를 줄이려는 시도.
* **실패 결과**: 의도와 정반대로 **+10~14% 성능 회귀** 발생.
* **근본 원인 분석**:
  * 현대 x86 컴파일러(GCC, Clang)는 SIMD 함수 내 상수 선언을 보면, 레지스터 할당 스크래치패드를 분석해 완벽한 위치에 `vbroadcast` 시키거나 `.rodata` 참조로 최적화해냅니다.
  * 프로그래머가 이를 억지로 루프 바깥으로 끄집어내면 레지스터 압력이 증가(Register Pressure)하여 오히려 컴파일러가 해당 레지스터 값을 스택에 Spill/Reload 하는 최악의 코드를 생성합니다.
* **가이드라인 (에이전트 수칙)**:
  * **x86_64 에이전트**: SIMD 상수는 **항상 사용 지점(스코프)에 가장 가깝게(인접 선언)** `const __m256i v = _mm256_set1...` 형태로 선언할 것. 컴파일러의 레지스터 프로모션을 절대 방해하지 마세요. (AVX-512도 동일)

---

### ❌ [Phase 49] 브랜치리스 push() 비트스택 연산 (Branchless Bit-Stack in push())
* **목표**: `push()` 함수 내 `bool` 타입 + 삼항 연산자(CMOV) 패턴을, 순수 정수 산술(NEG+AND)로 대체하여 분기를 완전 제거.
  ```cpp
  // 기존 (컴파일러가 CMOV 생성)
  const bool in_obj = !!(obj_bits_ & mask);
  sep = is_val ? uint8_t(2) : uint8_t(has_el);
  kv_key_bits_ ^= (in_obj ? mask : uint64_t(0));

  // 시도 (NEG+AND 방식)
  const uint64_t in_obj = (obj_bits_ & mask) != 0;
  sep = static_cast<uint8_t>((is_val << 1) | (~is_val & has_el));
  kv_key_bits_ ^= (-in_obj) & mask;
  ```
* **적용 대상**: `push()` 함수 (line ~5863)
* **실패 결과 (성능 회귀)**:
  * `twitter.json`: 365μs → 370μs (+1.4%)
  * `citm_catalog.json`: 955μs → 992μs (+3.9%)
  * `gsoc-2018.json`: 751μs → 770μs (+2.5%)
  * `canada.json`: 1,416μs → 1,497μs (+5.7%) ← 최대 회귀
* **근본 원인 분석**:
  1. **CMOV은 이미 최적**: GCC/Clang `-O3`는 `bool + 삼항` 패턴에서 단일 `cmov` 명령을 생성합니다. 이것이 곧 branchless 코드이므로, 명시적 정수 산술로 바꿔도 이점이 없습니다.
  2. **명령어 수 증가**: `(is_val << 1) | (~is_val & has_el)` 는 4개 명령(SHL, NOT, AND, OR)을 생성합니다. 컴파일러의 CMOV는 단 1개 명령(`cmovne`)입니다.
  3. **NEG+AND vs CMOV**: `(-in_obj) & mask`는 NEG+AND+XOR = 3 ops, 원래 `(in_obj ? mask : 0) ^= kv_key_bits_`는 CMOV+XOR = 2 ops로 오히려 더 효율적이었습니다.
* **가이드라인 (에이전트 수칙)**:
  * **x86_64 에이전트**: `bool` + 삼항 연산자 패턴을 NEG+AND 방식의 `uint64_t` 정수 산술로 대체하지 마세요. 컴파일러는 이미 최적의 CMOV를 생성하며, 명시적 정수 산술은 명령어 수를 오히려 늘립니다. 특히 `push()` 같은 hotpath 함수에서 컴파일러의 최적화를 간섭하지 마세요.

---

### ❌ [Phase 51] 64비트 TapeNode 단일 스토어 (Single 64-bit Store)
* **목표**: `push()` / `push_end()` 에서 두 개의 32비트 필드 스토어를 단일 `uint64_t` packed 스토어(`__builtin_memcpy`)로 통합하여 store 횟수를 절반으로 줄이려는 시도.
  ```cpp
  // 기존 (32비트 ×2)
  n->meta   = (uint32_t(t) << 24) | (uint32_t(sep) << 16) | uint32_t(l);
  n->offset = o;

  // 시도 (64비트 ×1)
  const uint64_t packed = static_cast<uint64_t>(meta_val) | (static_cast<uint64_t>(o) << 32);
  TapeNode *n = tape_head_++;
  __builtin_memcpy(n, &packed, 8);
  ```
* **적용 대상**: `push()` (line ~5895), `push_end()` (line ~5910)
* **실패 결과 (심각한 성능 회귀)**:
  * `twitter.json`: 365μs → 408μs (**+11.7%**)
  * `citm_catalog.json`: 955μs → 1,093μs (**+14.4%**)
  * `gsoc-2018.json`: 751μs → 811μs (+8.0%)
  * `canada.json`: 1,416μs → 1,502μs (+6.1%)
* **근본 원인 분석**:
  1. **컴파일러 스토어 병합(Store Merging) 방해**: GCC/Clang `-O3`는 인접한 두 32비트 스토어를 자동으로 단일 64비트 스토어(`movq`)로 병합합니다. 이 최적화는 이미 기존 코드에 적용되어 있었습니다.
  2. **중간 변수로 인한 레지스터 압력 증가**: `const uint64_t packed = ...`를 위해 중간 계산 결과를 별도 레지스터에 담아야 했고, 이 추가 레지스터 사용이 핫 루프 내 레지스터 스필(Spill)을 유발했습니다.
  3. **`__builtin_memcpy` 패턴의 부작용**: 컴파일러 입장에서 `__builtin_memcpy(n, &packed, 8)`은 임의의 포인터 복사로 해석될 여지가 있어, 이전 스토어 병합 최적화 기회를 오히려 차단했습니다.
* **가이드라인 (에이전트 수칙)**:
  * **x86_64 에이전트**: TapeNode 같은 `struct {uint32_t meta; uint32_t offset}` 구조에서 두 필드를 개별 대입으로 쓰는 것은 컴파일러가 이미 스토어 병합으로 최적화합니다. `uint64_t packed + __builtin_memcpy` 패턴으로 직접 병합을 강제하지 마세요. 이는 컴파일러 최적화를 방해하고 레지스터 압력을 증가시킵니다.

---

### ❌ [Phase 52] 정수 파싱 AVX2 SIMD 가속 (AVX2 Digit Scanner in kActNumber)
* **목표**: `kActNumber` 처리 경로에서 기존 SWAR-8(8바이트 스칼라 워드) 대신 AVX2 32B 벡터 스캐너를 추가하여 긴 숫자(8자리 초과) 파싱 속도 향상.
  ```cpp
  // 시도: SWAR-8 pre-gate → AVX2 32B bulk
  const __m256i vzero = _mm256_set1_epi8('0');
  const __m256i vnine = _mm256_set1_epi8(9);
  while (p_ + 32 <= end_) {
    __m256i v       = _mm256_loadu_si256(...);
    __m256i shifted = _mm256_sub_epi8(v, vzero);
    __m256i lt0     = _mm256_cmpgt_epi8(_mm256_setzero_si256(), shifted);
    __m256i gt9     = _mm256_cmpgt_epi8(shifted, vnine);
    uint32_t mask   = _mm256_movemask_epi8(_mm256_or_si256(lt0, gt9));
    if (mask) { p_ += __builtin_ctz(mask); goto num_done; }
    p_ += 32;
  }
  ```
* **적용 대상**: `kActNumber` case (line ~6309)
* **실패 결과**:
  * `canada.json`: 1,416μs → 1,374μs (**-2.9%** ← 유일한 개선)
  * `twitter.json`: 365μs → 406μs (**+11.2%** 회귀)
  * `citm_catalog.json`: 955μs → 1,033μs (**+8.1%** 회귀)
  * `gsoc-2018.json`: 751μs → 797μs (**+6.1%** 회귀)
* **근본 원인 분析**:
  1. **YMM 레지스터 충돌**: `kActNumber` 내 `const __m256i vzero/vnine`을 추가하면 `parse()` 함수 전체에서 YMM 레지스터 압력이 급격히 증가합니다. `kActString` 경로의 AVX2 스캐너(`vq`, `vbs` 등 YMM 레지스터)와 충돌하여 컴파일러가 스택 Spill/Reload를 유발했습니다.
  2. **Phase 40과 동일한 메커니즘**: Phase 40 (상수 호이스팅) 실패와 완전히 같은 레지스터 압력 문제입니다. `parse()` 함수 내 두 서로 다른 SIMD 처리 경로가 YMM 레지스터를 공유하면, 컴파일러의 레지스터 할당 예산을 초과합니다.
  3. **숫자 길이 분포 간과**: `twitter.json`의 숫자(18자리 ID)는 드문 케이스입니다. 대부분 짧은 숫자들(<8자리)이 SWAR-8로 빠르게 처리됩니다. AVX2 진입 비용이 실질적 이익보다 큽니다.
* **가이드라인 (에이전트 수칙)**:
  * **x86_64 에이전트**: `parse()` 같은 대형 함수에서 `kActString` 과 `kActNumber` 두 경로 모두 YMM 레지스터 집약 코드를 추가하지 마세요. 두 경로가 동시에 활성화되면 YMM 레지스터 예산(16개)을 초과하여 Spill이 발생합니다. 숫자 파싱 가속이 필요하다면 별도 함수로 분리하여 레지스터 스코프를 격리하는 방식을 검토하세요.
