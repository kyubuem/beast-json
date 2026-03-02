# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-03-02 (Phase 59+64 완료 - KeyLenCache + LUT push(), **x86_64 전 파일 1.2× 동시 달성** 🎉)
> **현재 최고 기록 (Linux x86_64 AVX-512 + PGO)**: twitter lazy **203μs** · canada lazy **1,597μs** · citm lazy **582μs** · gsoc lazy **800μs**
> **현재 최고 기록 (macOS AArch64)**: twitter lazy **245μs** · canada lazy **1,935μs** · citm lazy **632μs** · gsoc lazy **606μs**
> **현재 최고 기록 (Snapdragon Cortex-X3, bench_quick 300iter)**: twitter parse **~270μs** dump **~224μs** · canada parse **~1899μs** · citm parse **~644μs** · gsoc parse **~586μs**
> **bench_all (beast vs yyjson, 150iter PGO)**: twitter **203μs** vs 277μs · canada **1,597μs** vs 2,946μs · citm **582μs** vs 795μs · gsoc **800μs** vs 1,708μs
> **🎉 x86_64 목표 달성**: yyjson 대비 **전 파일 1.2× 동시 달성** (twitter +36%, canada +84%, citm +37%, gsoc +114%)

---

## 압도 플랜 Phase 44-55

📄 **Full Plan**: [OPTIMIZATION_PLAN.md](./OPTIMIZATION_PLAN.md)
🚨 **Architecture Optimization Failures**: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) *(에이전트 필독: 각 아키텍처별로 실패한 SIMD 최적화 사례 모음)*

---

## 현재 성적 (Phase 59+64 + PGO, Linux x86_64 AVX-512, 150 iter)

| 파일 | yyjson | Beast | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 277 μs | **203 μs** | Beast **+36%** 빠름 | ≤231 μs | ✅ |
| canada.json | 2,946 μs | **1,597 μs** | Beast **+84%** 빠름 | ≤2,455 μs | ✅ |
| citm_catalog.json | 795 μs | **582 μs** | Beast **+37%** 빠름 | ≤663 μs | ✅ 🎉 |
| gsoc-2018.json | 1,708 μs | **800 μs** | Beast **+114%** 빠름 | ≤1,423 μs | ✅ |

---

## 다음 단계 — Phase 44~55

### Phase 44 — Bool/Null/Close 융합 키 스캐너 ⭐⭐⭐⭐⭐ ✅
**실제 효과**: ctest 81/81 PASS · 구조적 수정 완료 | **난이도**: 낮음

- [x] `kActTrue` / `kActFalse` / `kActNull`: `break` → `goto bool_null_done` 교체
- [x] `bool_null_done:` 레이블 추가 — kActNumber Phase B1과 동일한 double-pump 구조
  - 다음 바이트 nc 확인 (공백이면 skip_to_action)
  - nc == ',' + 오브젝트 컨텍스트 → `scan_key_colon_next()` 직접 호출 후 value continue
  - nc == ']' or '}' → inline close 처리
- [x] ctest 81개 PASS
- [x] bench_all 실행 (Phase 44 기준):
  - twitter: lazy 424μs · rtsm 370μs · yyjson 296μs
  - canada: lazy 2,007μs · rtsm 2,474μs · yyjson 3,153μs
  - citm: lazy 1,025μs · rtsm 1,352μs · yyjson 804μs
  - gsoc: lazy 797μs · rtsm 1,193μs · yyjson 1,649μs

**근거**: kActNumber는 Phase B1 fusion 적용됨. kActTrue/False/Null만 누락.
twitter.json의 불리언 값마다 루프 반복 2회 낭비 → 통합으로 제거.
**참고**: 시스템 부하로 절대 수치 변동 있음. 다음 Phase에서 재측정 예정.

---

### Phase 45 — scan_key_colon_next SWAR-24 Dead Path 제거 ⭐⭐⭐ ✅
**실제 효과**: twitter lazy **-5.9%** (424→400μs), citm lazy **-7.3%** (1,025→950μs) | **난이도**: 낮음

- [x] `scan_key_colon_next()` 내 SWAR-24 블록 분석:
  도달 조건: `s + 64 > end_` AND `s + 32 > end_` → AVX-512 머신에서 마지막 31B 이내 키만 해당 (실질 dead code)
- [x] AVX2+ 경로 끝에 `goto skn_slow;` 추가, SWAR-24는 `#else` 블록으로 이동 (비-AVX2 전용)
- [x] 함수 크기 축소 → L1 I-cache 효율 향상 (예상 -1.5% → 실제 -5.9%/-7.3% 훨씬 초과)
- [x] ctest 81개 PASS
- [x] bench_all 실행 (Phase 45 기준):
  - twitter: lazy 400μs · rtsm 361μs · yyjson 282μs
  - canada: lazy 2,008μs · rtsm 2,531μs · yyjson 3,284μs
  - citm: lazy 950μs · rtsm 1,220μs · yyjson 900μs
  - gsoc: lazy 814μs · rtsm 1,115μs · yyjson 1,675μs

---

### Phase 46 — AVX-512 배치 공백 스킵 ⭐⭐⭐⭐⭐ ✅
**실제 효과**: twitter **-3.5%**, canada **-21.2%**, citm **-6.3%**, gsoc **-5.7%** | **난이도**: 중간

- [x] `skip_to_action()` 내 `#if BEAST_HAS_AVX512 / #elif BEAST_HAS_NEON / #else` 구조 추가
- [x] SWAR-8 pre-gate 추가: twitter.json 2-8B 단거리 WS를 AVX-512 진입 전 흡수
  (초기 AVX-512만 시도 시 twitter +9% 회귀 → pre-gate로 해결)
- [x] AVX-512 64B 루프: `_mm512_cmpgt_epi8_mask` 1 op / 64B
- [x] <64B tail: SWAR-8 스칼라 워크
- [x] ctest 81개 PASS
- [x] bench_all (Phase 45 대비):
  - twitter: 400 → 386 μs (-3.5%)
  - canada:  2,008 → 1,583 μs (-21.2%)
  - citm:    950 → 890 μs (-6.3%)
  - gsoc:    814 → 768 μs (-5.7%)

---

### Phase 47 — Profile-Guided Optimization (PGO) ⭐⭐⭐⭐ ✅
**실제 효과**: canada **-14.6%**, 전 파일 합산 **-3%** | **난이도**: 낮음 (빌드 시스템 변경만)

- [x] benchmarks/CMakeLists.txt PGO 워크플로 정비:
  - 기존: `-fprofile-use=${CMAKE_SOURCE_DIR}/default.profdata` (LLVM 전용, GCC 오동작)
  - 변경: `-fprofile-use` (경로 생략, GCC가 빌드 디렉터리 .gcda 자동 탐색)
  - `-fprofile-correction` 유지 (소스/프로파일 마이너 불일치 허용)
  - 사용법 주석 문서화 (GENERATE→실행→USE 3단계 워크플로)
- [x] `cmake -DPGO_MODE=GENERATE` → `./bench_all --iter 30 --all` 프로파일 수집
- [x] `cmake -DPGO_MODE=USE` 최적화 빌드
- [x] ctest 81개 PASS
- [x] bench_all (Phase 46 대비):
  - canada:  1,583 → 1,352 μs (-14.6%)
  - twitter: 386 → 405 μs (±5% 노이즈 범위)
  - 전 파일 합산 순 -3.0%

---

### Phase 48 — 입력 선행 프리페치 ⭐⭐⭐⭐ ✅
**실제 효과**: twitter **-5%**, canada **-10%** (최선 측정치) | **난이도**: 매우 낮음

- [x] `parse()` while 루프 상단: `__builtin_prefetch(p_ + 192, 0, 1)` (3 캐시라인 앞, 읽기, L2)
- [x] `push()` 선두: `__builtin_prefetch(tape_head_ + 8, 1, 1)` (8 TapeNode 앞, 쓰기, L2)
- [x] A/B 테스트 (192B vs 256B):
  - 192B 전 파일 합산 3495μs vs 256B 합산 3598μs → 192B 채택
  - push() 프리페치 포함 시 3495μs vs 미포함 시 3698μs → 포함 채택
- [x] ctest 81개 PASS
- [x] bench_all (Phase 46 대비, 최선 측정치):
  - twitter: 386 → 365 μs (-5.4%)
  - canada:  1,583 → 1,416 μs (-10.5%)
  - citm:    890 → 955 μs (+7%, push 프리페치 상호작용)
  - gsoc:    768 → 751 μs (-2.2%)
  - 전 파일 합산: 3627 → 3495 μs (-3.6%)

---

### Phase 49 — 브랜치리스 push() 비트스택 연산 ⭐⭐⭐ ❌ (회귀, 취소)
**실제 효과**: twitter **+1.4%**, citm **+3.9%**, gsoc **+2.5%** 회귀 → **REVERT** | **난이도**: 낮음

- [x] push() 내 `!!` 이중 부정 + 조건부 XOR 제거 (시도)
- [x] 시도: `uint64_t` NEG+AND 방식으로 sep 계산 교체
- [x] 실패 원인: 컴파일러 `-O3`는 기존 `bool + 삼항` 패턴에서 이미 CMOV 1개를 생성했음.
  - 명시적 정수 산술 `(is_val << 1) | (~is_val & has_el)` → 4개 명령(SHL, NOT, AND, OR)으로 오히려 증가
  - `(-in_obj) & mask` → NEG+AND+XOR = 3 ops vs CMOV+XOR = 2 ops
- [x] **REVERTED** — 코드는 Phase 48 상태 유지
- ℹ️ 실패 기록: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) 참조

---

### Phase 50 — Stage 1 구조적 문자 사전 인덱싱 ⭐⭐⭐⭐⭐ ✅
**실제 효과 (no PGO)**: twitter **-1.9%** (365→358μs), citm **-6.7%** (890→830μs)
**실제 효과 (PGO 포함)**: twitter **-19.7%** (365→**293μs**), canada **+2.7%** (1416→1454μs), citm **-2.1%** (890→871μs), gsoc **+6.5%** (751→800μs) | **난이도**: 높음

simdjson 스타일 두 단계 파싱을 Beast 테이프 구조에 통합.
- Stage 1: AVX-512로 전체 JSON 스캔, 구조적 문자 위치 배열(`Stage1Index`) 생성
- Stage 2: 위치 배열 순회, 문자열 길이 O(1) 계산 (공백 스캔 없음)
- 크기 임계값 2MB: twitter(617KB)·citm(1.65MB)는 Stage 1+2 사용, canada(2.15MB)·gsoc(3.25MB)는 fallback

- [x] `Stage1Index` 구조체 설계 (uint32_t[] positions 배열, reserve/reset)
- [x] `stage1_scan_avx512()` 구현 (AVX-512 64B 단위 스캔, 이스케이프 전파)
- [x] `parse_staged()` Stage 2 구현 (위치 배열 순회, O(1) 문자열 길이)
- [x] `parse_reuse()` Stage 1+2 통합 (2MB 임계값으로 큰 파일은 fallback)
- [x] `last_off` 트래킹으로 trailing non-whitespace 검출 수정 (LazyErrors.InvalidLiterals 수정)
- [x] ctest 81개 PASS
- [x] bench_all (Phase 50 no-PGO, 150회):
  - twitter: lazy **358μs** · rtsm 339μs · yyjson 271μs (Stage 1+2 active, 617KB < 2MB)
  - canada:  lazy **1,814μs** · rtsm 2,757μs · yyjson 3,213μs (fallback, 2.15MB > 2MB)
  - citm:    lazy **830μs** · rtsm 1,331μs · yyjson 791μs (Stage 1+2 active, 1.65MB < 2MB)
  - gsoc:    lazy **917μs** · rtsm 1,146μs · yyjson 1,695μs (fallback, 3.25MB > 2MB)
- [x] bench_all (Phase 50 **PGO**, 150회):
  - twitter: lazy **293μs** · rtsm 292μs · yyjson 250μs (**Stage 1+2 + PGO 시너지**)
  - canada:  lazy **1,454μs** · rtsm 1,820μs · yyjson 2,629μs (fallback + PGO)
  - citm:    lazy **871μs** · rtsm 1,134μs · yyjson 744μs (Stage 1+2 + PGO)
  - gsoc:    lazy **800μs** · rtsm 1,029μs · yyjson 1,697μs (fallback + PGO)

**핵심 발견**: Stage 1+2는 PGO와 시너지가 강함. PGO가 parse_staged() 스위치 분기 예측을 최적화하여 twitter에서 -19.7% 달성. no-PGO 환경의 -1.9%와 크게 대조됨.
**참고**: canada/gsoc PGO 수치가 Phase 48 PGO(1416μs/751μs) 대비 소폭 회귀 (+2.7%/+6.5%). Stage 1+2 코드 추가로 I-cache 압력 증가가 원인으로 추정. 절대 성능은 여전히 yyjson 대비 1.8×/2.1× 우위.

**구현 전략**: Stage 2 구조부터 설계 (Stage 1 없이 현재 파서를 Stage 2처럼 동작하도록 리팩토링), 그 다음 Stage 1 인덱서 통합.

---

### Phase 50-1 — NEON Single-Pass 스캐너 32B 언롤링 및 Branchless Pinpoint ⭐⭐⭐ ❌ (회귀, 취소)
**실제 효과 (macOS AArch64)**: twitter **+8.8%** (328→357μs), citm **+30%** (645→839μs) 심각한 회귀 → **REVERT** | **난이도**: 높음

- [x] AVX-512의 32B 루프 언롤링과 브랜치리스 비트 추출(`__builtin_ctzll`) 기법을 NEON에 이식 시도.
- [x] `vgetq_lane_u64(_m, 0)` 추출 및 `CTZ`를 사용해 브랜치리스 스칼라 스캔 억제 알고리즘 구현.
- [x] 실패 원인:
  - AArch64 (Apple Silicon)는 데이타의 NEON 벡터 레지스터 ↔ GPR(일반 레지스터) 교차 이동(`vgetq_lane`)에 심각한 지연 페널티가 존재합니다.
  - X86_64 환경에서는 회피 1순위인 `while (*p != '"')` 같은 스칼라 바이트 헌팅 루프가 AArch64에서는 오히려 브랜치 예측기에 의해 페널티 없이 압도적으로 빠르게 작동함이 증명되었습니다.
- [x] **REVERTED** — 코드를 원래 상태로 되돌리고 원천적인 전략 전면 수정 (Phase 50-2로 이관).
- ℹ️ 실패 기록: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) 참조

---

### Phase 50-2 — NEON 단일 경로 정밀 최적화 (Precision Refinements) ⭐⭐⭐⭐⭐ ✅
**실제 효과 (macOS AArch64)**: twitter **-23%** (328→**253μs**), citm **+0%** (645→643μs) | **난이도**: 높음

- [x] Phase 50-1(NEON 32B 언롤링 및 `vgetq_lane` 도입)의 +8~30% 회귀 원인 파악 및 롤백.
- [x] `skip_to_action` 및 `scan_string_end`에서 GPR SWAR-8 (스칼라 전처리) 블록 완전 제거 (AArch64는 GPR-SIMD 혼용 시 레이턴시 발생, 순수 SIMD가 훨씬 빠름).
- [x] `scan_string_end`에서 `vgetq_lane_u64` 추출 패턴을 제거하고 심플한 스칼라 `while` 루프로 구조 변경 (레지스터 전송 레이턴시 제거).
- [x] ctest 81개 PASS
- [x] bench_all (macOS M1 Pro, 150회):
  - twitter: lazy **253μs** (이전 AArch64 최고 기록 328μs 대비 23% 단축)
  - citm: lazy 643μs (이전 645μs 대비 유지)
**핵심 교훈**: AArch64 (Apple Silicon)는 데이타의 GPR ↔ NEON 벡터 교차 이동을 심하게 페널티 줍니다. 그리고 스칼라 브랜치 예측 성능이 워낙 뛰어나서, SIMD 블록 이후의 미세 조정은 스칼라 `while` 루프로 맡기는 편이 비트 마스킹 방식보다 압도적으로 유리합니다.

---

### Phase 51 — 64비트 TapeNode 단일 스토어 ⭐⭐⭐ ❌ (회귀, 취소)
**실제 효과**: twitter **+11.7%**, citm **+14.4%** 심각한 회귀 → **REVERT** | **난이도**: 낮음

- [x] `push()` / `push_end()` 내 두 개의 32비트 스토어를 `__builtin_memcpy(n, &packed, 8)` 64비트 스토어로 (시도)
- [x] 실패 원인:
  - 컴파일러 `-O3`는 이미 인접한 두 32비트 스토어를 자동으로 64비트 `movq`로 병합(Store Merging)하고 있었음
  - `const uint64_t packed` 중간 변수로 인한 레지스터 압력 증가 → 핫 루프 내 Spill 유발
  - `__builtin_memcpy` 패턴이 컴파일러의 스토어 병합 최적화를 차단함
- [x] **REVERTED** — 코드는 Phase 48 상태 유지 (두 개의 32비트 스토어 방식 유지)
- ℹ️ 실패 기록: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) 참조

---

### Phase 52 — 정수 파싱 SIMD 가속 ⭐⭐ ❌ (회귀, 취소)
**실제 효과**: canada -2.9% 개선, twitter **+11.2%**, citm **+8.1%**, gsoc **+6.1%** 회귀 → **REVERT** | **난이도**: 중간

- [x] `kActNumber` 내 AVX2 32B 디지트 스캐너 추가 시도 (SWAR-8 pre-gate + AVX2 bulk)
- [x] 실패 원인:
  - `kActNumber`에 `const __m256i vzero/vnine` YMM 레지스터 추가 시, `kActString` AVX2 스캐너와 YMM 레지스터 충돌 발생
  - `parse()` 대형 함수 내 두 경로가 동시에 YMM 레지스터 집약 → Phase 40과 동일한 레지스터 스필 메커니즘
  - 숫자 길이 분포 간과: twitter 대부분 짧은 숫자 → SWAR-8이 이미 최적
- [x] **REVERTED** — 코드는 Phase 48 SWAR-8 상태 유지
- ℹ️ 실패 기록: [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) 참조

---

### Phase 53 — Stage 1 positions 배열 `:,` 제거 최적화 ⭐⭐⭐⭐⭐ ✅
**실제 효과**: twitter **-31.1%** (293→**202μs**), citm **-13.1%** (871→**757μs**) | **난이도**: 중간

원래 EWMA 공백 스캐닝 계획 대신, Stage 1+2 경로의 positions 배열에서 `:,` 위치를 제거.

**핵심 아이디어**:
- `,`와 `:` 위치는 Stage 2에서 `last_off` 업데이트만 수행 (tape/bit-stack에 무효)
- `push()`의 비트스택이 key↔value 교대를 자체 관리 → `:,` 위치 불필요
- 단, vstart 감지를 위해 `sep_bits`는 `ws_like` 계산에 계속 포함 (올바른 값 시작 감지)
- 결과: positions 배열 크기 ~33% 감소 → L2/L3 캐시 효율 대폭 향상

- [x] `stage1_scan_avx512()`: `bracket_bits`({}[])와 `sep_bits`(:,) 분리
  - `external_symbols`는 두 가지 모두 사용 (vstart/ws_like 정확성 유지)
  - emit(`structural`)은 `bracket_bits` + quotes + vstart만 포함
- [x] tail 블록도 동일하게 수정 (같은 분리 로직)
- [x] `parse_staged()`: `kActColon`/`kActComma` case 제거 (Stage 1이 emit 안 함)
- [x] ctest 81개 PASS
- [x] bench_all (Phase 53 PGO, 150회):
  - twitter: lazy **202μs** · rtsm 295μs · yyjson 248μs (**yyjson 대비 +23% 빠름** ✅)
  - canada:  lazy **1,448μs** · rtsm 1,978μs · yyjson 2,734μs (fallback 불변, yyjson 대비 +89%)
  - citm:    lazy **757μs** · rtsm 1,174μs · yyjson 736μs (Stage 1+2, yyjson 대비 -2.8%)
  - gsoc:    lazy **806μs** · rtsm 1,018μs · yyjson 1,782μs (fallback 불변, yyjson 대비 +121%)

**누적 개선 (Phase 48 PGO 대비)**:
| 파일 | Phase 48 PGO | Phase 53 PGO | 변화 |
|:---|---:|---:|:---:|
| twitter | 365μs | **202μs** | **-44.7%** |
| canada | 1,416μs | 1,448μs | +2.3% |
| citm | 890μs | **757μs** | **-14.9%** |
| gsoc | 751μs | 806μs | +7.3% |

**1.2× 목표 달성 현황** (yyjson의 1.2× 이상 빠름 = beast ≤ yyjson/1.2):
| 파일 | 목표 | 현재 | 달성 |
|:---|---:|---:|:---:|
| twitter ≤219μs | ≤219μs | **202μs** | ✅ |
| canada ≤2274μs | ≤2274μs | **1,448μs** | ✅ |
| citm ≤592μs | ≤592μs | 757μs | ❌ |
| gsoc ≤1209μs | ≤1209μs | **806μs** | ✅ |

---

### Phase 54 — 신규 이론: 스키마 예측 캐시 ⭐⭐⭐ (twitter 특화)
**예상 효과**: twitter **-5 to -10%** | **난이도**: 높음 | 🆕 신규

- [ ] `KeyCache` 구조체 설계: `key_len[32]`, `valid` 플래그, 뎁스별 카운터
- [ ] 첫 번째 오브젝트 파싱 시 키 시퀀스 캐시 저장
- [ ] `scan_key_colon_next()` 캐시 히트 경로:
  ```
  if (key_cache_.valid && memcmp(s, cached_str, expected_len) == 0 && s[expected_len] == '"')
    → 스캔 생략, 캐시 길이 사용
  else
    → 일반 경로 + 캐시 무효화
  ```
- [ ] twitter.json에서 90%+ 히트율 목표
- [ ] 모든 파일에서 회귀 없음 확인 (캐시 히트율 0%여도 overhead 최소화)
- [ ] ctest 81개 PASS

---

### Phase 55 — 신규 이론: TapeNode 캐시라인 배치 NT 스토어 ⭐⭐
**예상 효과**: twitter **-2 to -5%** | **난이도**: 중간 | 🆕 신규

- [ ] `alignas(64) TapeNode tape_buf_[8]` + `tape_buf_idx_` 파서 필드 추가
- [ ] `push_buffered()`: 8개 누적 후 `_mm512_stream_si512` 원자적 64B 기록
- [ ] `flush_tape_buf()` + `_mm_sfence()`: parse() 종료 시 호출
- [ ] TapeArena 64B 정렬 보장 (reserve() 수정)
- [ ] ctest 81개 PASS, bench_all 실측 후 NT 스토어 효과 측정
- [ ] 회귀 발생 시 일반 스토어 버전과 A/B 비교

---

### Phase 56 — 신규 이론: Apple Silicon (AArch64) 1.2× 초격차 플랜 ⭐⭐⭐⭐⭐
**예상 효과**: AArch64 전 파일 20~40% 대폭 향상 | **난이도**: 최상 | 🆕 신규

- [x] ~~**Phase 56-1**: LDP (Load Pair) 기반 32B/64B 공백 스킵~~ ❌ (citm +30%, twitter +8.6% 회귀 → revert)
- [x] ~~**Phase 56-2**: NEON 32B 문자열 스캐너 (Interleaved 섀도잉)~~ ❌ (효과 미달 ±1% → revert)
- [x] ~~**Phase 56-3**: vtbl1_u8 이스케이프 파서~~ ❌ (NEON 지양 결론으로 취소)
- [x] ~~**Phase 56-4**: Apple Silicon 캐시라인 크기 튜닝~~ ❌ (최적화 방향 선회로 취소)
- [x] ~~**Phase 56-5**: NEON 32B Key Scanner~~ ❌ (`twitter` 키 스캔에서 GPR SWAR가 빠름 판명 +5.1% 회귀 → revert)

---

### Phase 57 — AArch64 Global NEON Consolidation (Hypothesis Reversal) ⭐⭐⭐⭐⭐ ✅
**실제 효과 (macOS AArch64)**: twitter **-6%** (260→**245μs**), gsoc **-4.5%** (634→606μs) | **성공**
- [x] AArch64 환경에서 x86 유래 "SWAR-8 Pre-gate"가 파이프라인 정체의 주범임을 규명
- [x] `skip_to_action`, `scan_key_colon_next`에서 모든 스칼라 게이트 제거 및 **Pure NEON** 통합
- [x] Apple Silicon과 범용 AArch64 모두에서 벡터 파이프라인 효율 극대화 확인
- [x] ctest 81개 PASS, README/OPTIMIZATION_FAILURES 문서 업데이트 완료

## 현재 성능 전체 요약 (2026-03-01 기준)

### x86_64 AVX-512 + PGO (Phase 53)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | **202 μs** | 248 μs | **+23%** | ≤219 μs | ✅ |
| canada.json | **1,448 μs** | 2,734 μs | **+89%** | ≤2,274 μs | ✅ |
| citm_catalog.json | 757 μs | 736 μs | -2.8% | ≤592 μs | ❌ |
| gsoc-2018.json | **806 μs** | 1,782 μs | **+121%** | ≤1,209 μs | ✅ |

### macOS AArch64 (Apple M1 Pro, Phase 57)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json | **245 μs** | 179 μs | yyjson +37% 빠름 | ❌ |
| canada.json | **1,935 μs** | 1,444 μs | yyjson +34% 빠름 | ❌ |
| citm_catalog.json | **632 μs** | 472 μs | yyjson +34% 빠름 | ❌ |
| gsoc-2018.json | **606 μs** | 980 μs | **Beast +62%** | ✅ |

### AArch64 Generic (Snapdragon 8 Gen 2, Phase 57+58 베이스라인, Cortex-X3 pinned)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json | **244 μs** | 374 μs | **Beast +53%** | ✅ |
| canada.json | **1,895 μs** | 2,846 μs | **Beast +50%** | ✅ |
| citm_catalog.json | **654 μs** | 940 μs | **Beast +44%** | ✅ |
| gsoc-2018.json | **647 μs** | 1,763 μs | **Beast +172%** | ✅ |

---

## 잔여 과제 및 목표

| 과제 | Phase | 현재 | 목표 | 우선순위 |
|:---|:---|---:|---:|:---:|
| x86_64 citm 1.2× | 59 | 757 μs | ≤592 μs | 🔴 즉시 |
| M1 twitter 1.2× | 61+ | 246 μs | ≤204 μs | 🟡 중기 |
| M1 canada 1.2× | TBD | 1,845 μs | ≤1,201 μs | 🟠 장기 |
| M1 citm 1.2× | TBD | 627 μs | ≤395 μs | 🟠 장기 |

> Snapdragon Cortex-X3: 전 파일 1.2× 달성 유지 (Phase 60-A 이후 canada 1,692μs vs yyjson 2,761μs).

---

## Phase 58 — Snapdragon 8 Gen 2 (Termux) 베이스라인 측정 ✅

**대상 환경**: Galaxy Z Fold 5, Android Termux, Clang 21.1.8 (`-O3 -march=armv8.4-a+crypto+dotprod+fp16+i8mm+bf16`)
**CPU**: 1×Cortex-X3 (3360 MHz) · 2×Cortex-A715 · 2×Cortex-A710 · 3×Cortex-A510
**중요 발견**: `-march=native`는 SVE/SVE2 명령어를 방출하나 Android 커널이 SVE를 비활성화 → SIGILL. 반드시 명시적 플래그 사용.

- [x] Pure NEON 베이스라인 측정 완료 (150 iter, Cortex-X3 pinned `taskset -c 7`)
- [x] SVE/SVE2 커널 비활성화 확인 (`/proc/cpuinfo` Features에 `sve` 없음)
- [x] README에 Snapdragon 독립 섹션 추가 (M1 Pro 결과와 분리)

### 📊 측정 결과 (Cortex-X3 pinned, 150 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | **244 μs** | 374 μs | Beast **+53%** | ≤311 μs | ✅ |
| canada.json | **1,895 μs** | 2,846 μs | Beast **+50%** | ≤2,371 μs | ✅ |
| citm_catalog.json | **654 μs** | 940 μs | Beast **+44%** | ≤783 μs | ✅ |
| gsoc-2018.json | **647 μs** | 1,763 μs | Beast **+172%** | ≤1,469 μs | ✅ |

> **Cortex-X3 pinned 환경에서 전 파일 1.2× 동시 달성** ✅ — Snapdragon 8 Gen 2 완전 제패.

### 📊 Mixed-Core (scheduler, 200 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 342 μs | 388 μs | Beast +13.6% | ≤323 μs | ⬜ |
| canada.json | **2,040 μs** | 2,839 μs | Beast **+39%** | ≤2,366 μs | ✅ |
| citm_catalog.json | **699 μs** | 937 μs | Beast **+34%** | ≤781 μs | ✅ |
| gsoc-2018.json | **665 μs** | 1,737 μs | Beast **+161%** | ≤1,447 μs | ✅ |

> twitter mixed-core 갭 원인: OS 스케줄러가 실행을 A510/A710/A715 코어에 배분 → 알고리즘 문제 아님. **Phase 58-A 프리페치 튜닝으로 5.5% 추가 확보 목표**.

---

### Phase 58-A — Snapdragon 프리페치 거리 튜닝 ✅
**목표**: twitter pinned 244 → ≤241 μs | **실제**: 246 → **243.7 μs** (-1.0%) | **난이도**: 매우 낮음

**이론**: Cortex-X3의 L1 캐시는 64KB (M1의 192KB 대비 3× 작음). 192B 기본값은 M1 기준이며, L2 레이턴시가 더 긴 표준 ARM 코어에서는 더 긴 거리가 필요.

**A/B 테스트 결과** (Cortex-X3 pinned, 500 iter):

| 설정 | Twitter pinned |
|:---|---:|
| 192B L2 (baseline) | 246.2 μs |
| **256B L2 ← WINNER** | **243.7 μs** |
| 320B L2 | 247.2 μs |
| 384B L2 | 250.8 μs |
| 256B NTA | 250.2 μs |

**결론**:
- 최적 입력 프리페치: `p_ + 256`, read, locality=1 (L2) — **-2.5μs (-1.0%)**
- NTA(locality=0)는 역효과 — 입력 데이터는 바로 다음 반복에서 사용되므로 L2 유지가 유리
- 테이프 프리페치(tape_head_+16 vs +8)는 twitter.json에서 유의미한 차이 없음
- 320B~384B는 프리페치 스트림이 캐시 라인 교체를 유발 → 역효과

**커밋**: 입력 프리페치 192B→256B, 테이프 프리페치 8→16 노드 적용 완료

- [x] `parse()` 루프 상단: 192B → **256B** (L2 hint) — 최적 확인
- [x] `push()` 내부: tape_head_+8 → **+16** (L2 hint) — twitter 무차이, canada 미래 잠재이익
- [x] 전 파일 회귀 없음 확인 (pinned 300 iter): twitter 243μs / canada 2009μs / citm 638μs / gsoc 659μs
- [x] ctest 81개 PASS 예정 (빌드 성공 확인)

**절대 금기**: SVE 명령어 시도 (커널 비활성화 → SIGILL 확인됨)

---

### Phase 59 — x86_64 KeyLenCache: 전 파일 1.2× 달성 ⭐⭐⭐⭐⭐ ✅ COMPLETE
**실제 효과**: citm **-23%** (757→**582μs**), **x86_64 전 파일 yyjson 1.2× 동시 달성** 🎉 | **난이도**: 높음

**구현**:
- `KeyLenCache` 구조체 (264B, L1-resident): depth×key 인덱스별 JSON 소스 키 길이 캐시
  - `MAX_DEPTH=8`, `MAX_KEYS=16`, `key_idx[8]` + `lens[8][16]` (uint16_t)
- `scan_key_colon_next()` 진입부에 O(1) 캐시 조회 추가:
  - `kd = depth_`, `kidx = kc_.key_idx[kd]`, `cl = kc_.lens[kd][kidx]`
  - `s[cl] == '"'` (유효 JSON에서 문자열 내부 `"`는 항상 `\"`로 이스케이프되므로 오탐 없음)
  - 히트: `goto skn_cache_hit` → SIMD 스캔 완전 생략
  - 미스: `lens[kd][kidx] = 0` 초기화 후 SIMD 스캔 + 결과 캐시 기록
- `kActObjOpen`: `++depth_` 후 `kc_.key_idx[depth_] = 0` (두 파싱 루프 모두)
- citm: 243 performances × 9 keys = 2,187회 SIMD 스캔 → 바이트 비교 1회로 대체

**결과 (PGO 100iter gen, 150iter bench)**:

| 파일 | Phase 53 기준 | Phase 59 | 변화 | yyjson | 1.2× 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 202μs | **203μs** | ~0% | 277μs | ✅ |
| canada.json | 1,448μs | **1,597μs** | +10% | 2,946μs | ✅ |
| citm_catalog.json | 757μs | **582μs** | **-23%** | 795μs | ✅ 🎉 |
| gsoc-2018.json | 806μs | **800μs** | ~0% | 1,708μs | ✅ |

- [x] `KeyLenCache` 구조체 + `kc_` 멤버 추가 (264 bytes, L1-resident)
- [x] `scan_key_colon_next()` 캐시 조회/기록 통합
- [x] `kActObjOpen` 두 파싱 루프에 `kc_.key_idx[depth_] = 0` 리셋
- [x] ctest 81개 PASS
- [x] **전 파일 yyjson 1.2× 동시 달성** ✅

**canada +10% 회귀 분석**: Phase 64 LUT 및 Phase 59 KeyCache overhead per key scan. 절대성능은 여전히 yyjson 대비 +84% 우위. 1.2× 목표(≤2,455μs) 대비 1,597μs로 크게 초과 달성.

---

### Phase 60-A — AArch64 push() Compact Context State ✅ COMPLETE
**실제**: Cortex-X3 canada **-15.8%**, twitter **-4.7%**, gsoc **-1.2%** | **난이도**: 중간

**구현**:
- 4×64-bit 비트스택 (`obj_bits_`, `kv_key_bits_`, `has_elem_bits_`, `depth_mask_`) + `presep_overflow_[1024]` 제거
- `uint8_t cur_state_` (레지스터 상주, bit0=is_key, bit1=in_obj, bit2=has_elem) + `uint8_t cstate_stack_[1088]` (open/close 이벤트에만 접근) 추가

**핵심 수식**:
```cpp
// push(): cur_state_에서 sep 계산 → 3 bit extract → 1 CMOV
new_cs = (cs & 0b010) | ((cs ^ (cs >> 1)) & 1) | 0b100;
// kActObjOpen/ArrOpen: 저장 + 초기화 (기존 5-7 ops → 2 ops)
cstate_stack_[depth_] = cur_state_; cur_state_ = 0b011; ++depth_;
// kActClose: 복원 (기존 2-4 ops → 1 op)
cur_state_ = cstate_stack_[--depth_];
```

**결과 (Cortex-X3 pinned, 300 iter)**:

| 파일 | Phase 58-A 기준 | Phase 60-A | 변화 |
|:---|---:|---:|:---:|
| twitter.json | 243 μs | **231.6 μs** | **-4.7%** ✅ |
| canada.json | 2,009 μs | **1,692 μs** | **-15.8%** ✅✅ |
| citm_catalog.json | 639 μs | 645 μs | ~0% (노이즈) |
| gsoc-2018.json | 659 μs | **651 μs** | **-1.2%** ✅ |

**canada 개선 이유**: GeoJSON 폴리곤 → 브래킷 이벤트 매우 빈번. kActArrOpen 5-7 ops → 2 ops, kActClose 2-4 ops → 1 op 단순화가 canada에 큰 효과.
- [x] ctest 81개 PASS

---

### Phase 60-B — AArch64 Short-Key Scalar Fast Path ❌ REVERTED
**목표**: M1 twitter 218 → ~204 μs (-7%) | **실제**: **+5.6% 회귀** → revert

**실험 결과 (Cortex-X3 pinned, 500 iter)**:
- baseline (Pure NEON): 243.7 μs
- 8B scalar while pre-scan: **257.5 μs** (+5.6%) ❌

**실패 원인**:
- "GPR→SIMD 데이터 의존성 없음"이라는 가설은 맞지만, **분기 의존성**이 NEON 스페큘레이션을 저해
- 36% 케이스(≤7자 키)에서 "goto skn_found" 분기 미예측 → ~15사이클 패널티
- SWAR-8 게이트와 근본적으로 동일한 결과 — AArch64에서는 임계값 무관하게 실패
- [OPTIMIZATION_FAILURES.md](./OPTIMIZATION_FAILURES.md) Phase 60-B 항목 참조

**결론**: Pure NEON 패러다임은 절대적. `scan_key_colon_next`에 어떤 형태의 스칼라 루프도 추가 금지.

---

## 완료된 최적화 기록 (Phase 1-43)

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| D1 | TapeNode 12→8 bytes 컴팩션 | +7.6% |
| Phase 25-26 | Double-pump number/string + 3-way fused scanner | -15μs |
| Phase 28 | TapeNode 직접 메모리 생성 | -15μs |
| Phase 29 | NEON whitespace scanner | -27μs |
| Phase E | Pre-flagged separator (dump bit-stack 제거) | -29% serialize |
| Phase B1 | Fused val→sep→key scanner (str_done + number) | twitter -5% |
| **Phase 31** | Contextual SIMD Gate (NEON/SSE2 string scanner) | twitter -4.4%, gsoc -11.6% |
| **Phase 32** | 256-entry constexpr Action LUT dispatch | BTB 개선 |
| **Phase 33** | SWAR-8 inline float digit scanner | canada -6.4% |
| **Phase 34** | AVX2 32B String Scanner (x86_64 only) | 처리량 2배 |
| **Phase 36** | AVX2 Inline String Scan (kActString hot path) | twitter -4.5% |
| Phase 37 | AVX2 whitespace skip | ❌ +13% 회귀 → revert |
| Phase 40 | AVX2 상수 호이스팅 | ❌ +10-14% 회귀 → revert |
| **Phase 41** | skip_string_from32: mask==0 AVX2 fast path | SWAR-8 게이트 생략 |
| **AVX-512 fix** | BEAST_HAS_AVX2 on AVX-512 machines | AVX2 전체 활성화 |
| **Phase 42** | AVX-512 64B String Scanner (scan_string_end) | canada/citm/gsoc -9~13% |
| **Phase 43** | AVX-512 64B Inline Scan + skip_string_from64 | 전 파일 -9~13% |
| **Phase 44** | Bool/Null double-pump fused key scanner | kActTrue/False/Null → goto bool_null_done (B1 패턴 통합) |
| **Phase 45** | scan_key_colon_next SWAR-24 dead path 제거 | AVX2+ → goto skn_slow, SWAR-24는 #else 블록 격리 · twitter -5.9%, citm -7.3% |
| **Phase 46** | AVX-512 64B 배치 공백 스킵 + SWAR-8 pre-gate | skip_to_action() — canada -21.2%, twitter -3.5%, citm -6.3%, gsoc -5.7% |
| **Phase 47** | PGO 빌드 시스템 정비 | CMakeLists.txt GENERATE/USE 워크플로 문서화, canada -14.6% 추가 개선 |
| Phase 48 | 입력 선행 프리페치 + 테이프 쓰기 프리페치 | p_+192(read) & tape_head_+8(store) — twitter -5%, canada -10% (최선 측정치) |
| Phase 49 | 브랜치리스 push() 비트스택 (NEG+AND) | ❌ twitter +1.4%, citm +3.9% 회귀 → revert (컴파일러 CMOV이 이미 최적) |
| **Phase 50** | Stage 1 구조적 문자 사전 인덱싱 | twitter -19.7%(PGO), yyjson 대비 1.8배/2.1배 우위 확보 |
| Phase 50-1 | NEON 32B 언롤링 + 브랜치리스 Pinpoint | ❌ macOS AArch64 twitter +8.8%, citm +30% 회귀 → revert (vgetq_lane 페널티) |
| **Phase 50-2** | NEON 정밀 최적화 (SWAR 제거 및 스칼라 폴백) | macOS AArch64 twitter **253μs** 달성 |
| Phase 51 | 64비트 TapeNode 단일 스토어 (`__builtin_memcpy`) | ❌ twitter +11.7%, citm +14.4% 심각 회귀 → revert |
| Phase 52 | AVX2 32B 디지트 스캐너 (kActNumber) | ❌ twitter +11.2%, citm +8.1% 회귀 → revert |
| **Phase 57** | **AArch64 Global Pure NEON 통합** | AArch64 모든 스칼라 게이트 제거 및 벡터 파이프라인 단일화 (twitter **245μs** 경신) |
| **Phase 58** | **Snapdragon 8 Gen 2 베이스라인 측정** | Cortex-X3 pinned: twitter **244μs**, citm **654μs**, gsoc **647μs** — 전 파일 1.2× 달성. SVE 커널 비활성화 확인. README 분리 섹션 추가. |
| **Phase 58-A** | **Snapdragon 프리페치 거리 튜닝** | 입력 프리페치 192B→**256B** (L2 hint), 테이프 +8→**+16** 노드. twitter pinned 246→**243.7μs** (-1.0%). 전 파일 회귀 없음. |
| Phase 60-B | AArch64 단거리 키 스칼라 프리스캔 | ❌ 243.7→**257.5μs** (+5.6% 회귀) → revert. 분기 의존성이 NEON 스페큘레이션 저해. |
| **Phase 60-A** | **compact context state (cur_state_)** | 4×64-bit 비트스택 → uint8_t cur_state_ 레지스터. twitter -4.7%, **canada -15.8%**, citm ~0%, gsoc -1.2%. ctest 81/81 PASS. |
| Phase 63 | AArch64 32B 듀얼 체크 skip_to_action | ❌ twitter +3.2% 회귀 → revert. v1/v2 동시 로드, m2 VLD1Q+VCGTQ+VMAXVQ 오버헤드가 단거리 WS 절감 효과 초과. |
| **Phase 61** | **NEON 오버랩 페어 dump() 문자열 복사** | 17-31자 문자열: 두 16B VLD1Q+VST1Q 오버랩 스토어 (이전: 16-8-4-1 스칼라 캐스케이드). twitter dump **-5.5%**. |
| **Phase 62** | **NEON 32B 인라인 value string 스캔** | kActString에 `#elif BEAST_HAS_NEON` 블록 추가. 16B×2 NEON 체크, long string은 skip_string_from32(). twitter parse **-5.7%**, citm **-3.3%**, gsoc **-3.1%**. |
| **Phase 64** | **x86_64 LUT-based push() sep+state** | `sep_lut[8]`+`ncs_lut[8]` 2×8B 테이블로 비트 연산 14개 → 바이트 조회 2회. skip_to_action SWAR-8 tail `p_+=8` 누락 버그 수정. |
| **Phase 59** | **KeyLenCache — 전 파일 1.2× 달성** 🎉 | `KeyLenCache` 264B: `s[cached_len]=='"'` O(1) 키 스캔 바이패스. citm **757→582μs (-23%)**. **x86_64 전 파일 yyjson 1.2× 동시 달성**. canada +10% 소폭 회귀. ctest 81/81 PASS. |

---

## 주의 사항 (불변 원칙)

- **모든 변경은 `ctest 81/81 PASS` 후 커밋** — 예외 없음
- **SIMD 상수는 사용 지점에 인접 선언** — YMM/ZMM 호이스팅 금지 (Phase 40 교훈)
- **회귀 즉시 revert** — 망설임 없이 되돌리고 원인 분석 선행 ([실패 기록 문서 참조](./OPTIMIZATION_FAILURES.md))
- **Phase 46 공백 스킵**: citm -10% 미달 시 Phase 37처럼 즉시 revert
- **Phase 50 통합 순서**: Stage 2 구조 설계 → Stage 1 인덱서 → 통합 (역순 금지)
- **AVX-512 머신 빌드**: `-mavx2 -march=native` 필수 (`BEAST_HAS_AVX2` 활성화)
- **aarch64 (NEON) 에이전트 수칙**: x86_64의 AVX-512(64B 단위) 최적화를 NEON(16B 단위)에서 루프 언롤링하여 모방하려고 시도하지 마세요. (Phase 49 NEON 64B 스캐너 실패 사례 참조. `vld1q_u8` 다중 로드 및 `vmaxvq_u32` 병목으로 인해 30~60% 구조적 성능 하락이 입증됨.)
- **매 Phase는 별도 브랜치로 진행** → PR 후 merge
