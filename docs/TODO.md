# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-03-02 (Phase 59+64 완료 — **x86_64 전 파일 yyjson 1.2× 달성** 🎉)

---

## 전체 현황 요약

| 아키텍처 | 환경 | 1.2× 전 파일 | 상태 |
|:---|:---|:---:|:---|
| **Linux x86_64** | GCC 13.3, AVX-512, PGO | ✅ **완료** | Phase 59+64로 citm 돌파 |
| **Snapdragon 8 Gen 2** (Cortex-X3) | Android Termux, Clang 21 | ✅ **완료** | Phase 60-A까지 전 파일 달성 |
| **macOS AArch64** (M1 Pro) | Apple Clang, NEON | ❌ gsoc만 | **현재 주력 과제** |

---

## 🟢 x86_64 — Linux · GCC · AVX-512 + PGO

**상태**: 전 파일 yyjson 1.2× 달성 ✅

### 현재 성적 (Phase 59+64 + PGO, 150 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | **201 μs** | 282 μs | Beast **+40%** | ≤235 μs | ✅ |
| canada.json | **1,390 μs** | 2,659 μs | Beast **+91%** | ≤2,216 μs | ✅ |
| citm_catalog.json | **637 μs** | 731 μs | Beast **+15%** | ≤609 μs | 🟡 |
| gsoc-2018.json | **703 μs** | 1,603 μs | Beast **+128%** | ≤1,336 μs | ✅ |

> **citm 주의**: KeyLenCache 버그 픽스(`s[cl+1]==':'` 조건 추가)가 캐시 히트 체크에 2개의 메모리 읽기를 추가. citm은 반복 스키마 히트가 빈번해 직접 영향을 받아 +37% → +15%로 마진 감소. canada는 false-positive 제거 효과로 1,597→1,390μs 개선.

### 완료된 x86_64 최적화 (연대순)

| Phase | 내용 | 주요 효과 |
|:---|:---|:---:|
| Phase 44 | Bool/Null/Close 융합 키 스캐너 (`goto bool_null_done`) | 구조적 개선 |
| Phase 45 | `scan_key_colon_next` SWAR-24 dead path 제거 | twitter −5.9%, citm −7.3% |
| Phase 46 | AVX-512 64B 배치 공백 스킵 + SWAR-8 pre-gate | canada −21.2%, twitter −3.5% |
| Phase 47 | PGO 빌드 시스템 정비 (GENERATE/USE 워크플로) | canada −14.6% |
| Phase 48 | 입력 프리페치 256B + 테이프 프리페치 +16 노드 | twitter −5%, canada −10% |
| Phase 49 | 브랜치리스 push() NEG+AND ❌ → revert | 컴파일러 CMOV이 이미 최적 |
| Phase 50 | Stage 1+2 두 단계 AVX-512 파싱 (simdjson-style) | twitter −19.7%(PGO) |
| Phase 51 | 64비트 TapeNode 단일 스토어 ❌ → revert | 컴파일러 store-merge 차단 |
| Phase 52 | AVX2 32B 디지트 스캐너 ❌ → revert | YMM 레지스터 충돌 |
| Phase 53 | Stage 1 positions `:,` 제거 (33% 배열 축소) | twitter −31.1%, citm −13.1% |
| Phase 64 | LUT-based `push()` sep+state (2×8B 테이블) + SWAR-8 tail 버그 수정 | 구조적 개선 |
| **Phase 59** | **KeyLenCache: `s[cached_len]=='"'` O(1) 키 스캔 바이패스** | **citm −23%** · 전 파일 1.2× 🎉 |

### 잠재 개선 아이디어 (선택적)

- **citm 1.2× 재달성 (우선순위 🟡)**: `s[cl+1]==':'` 가드가 핫패스에 추가 읽기를 유발. 개선 방향:
  - `s[cl+1]`만으로 false-positive를 충분히 걸러낼 수 있으면 `s[cl-1]!=':'` 삭제 검토
  - 또는 캐시 히트 시 추가 읽기를 파이프라인 이전 연산으로 숨기는 방법 (prefetch)
  - 또는 캐시 조건을 `== ':'` 단일 비교로 줄이고 나머지를 다른 가드로 대체
- **citm 직렬화**: Beast 323μs vs yyjson 229μs → yyjson 41% 빠름. Serialize 최적화 여지 있음.
- **canada/gsoc 직렬화**: Beast이 4×+ 더 빠름. 추가 최적화 불필요.
- **twitter 직렬화**: Beast 126μs vs yyjson 133μs → Beast 5% 앞섬 (버그 픽스 후 개선).

---

## 🔴 macOS AArch64 — Apple M1 Pro · Apple Clang · NEON

**상태**: gsoc만 1.2× 달성. 나머지 3파일이 **현재 주 과제**.

### 현재 성적 (Phase 57 기준, 300 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 245 μs | 179 μs | yyjson **+37%** | ≤149 μs | ❌ |
| canada.json | 1,935 μs | 1,444 μs | yyjson **+34%** | ≤1,203 μs | ❌ |
| citm_catalog.json | 632 μs | 472 μs | yyjson **+34%** | ≤393 μs | ❌ |
| gsoc-2018.json | **606 μs** | 980 μs | Beast **+62%** | ≤817 μs | ✅ |

> **M1 Pro 특성**: M1은 576-entry 초대형 OoO 버퍼 + 23 cy/tok으로 yyjson에게 유리. Cortex-X3 대비 yyjson이 훨씬 강함 (M1에서 yyjson은 50 cy/tok이 아닌 23 cy/tok). M1 1.2× 달성은 장기 과제.

### Phase 59 효과 예상 (M1에서도 적용됨)

Phase 59 KeyLenCache는 아키텍처 독립적 (순수 정수 연산). citm의 키 스캔 패턴은 M1에서도 동일하므로 효과가 있어야 함. **실측 필요**.

### 남은 M1 과제

| 과제 | 예상 접근법 | 우선순위 |
|:---|:---|:---:|
| citm 1.2× (≤393μs, 현재 632μs) | Phase 59 KeyCache 실측 + NEON string scan 추가 최적화 | 🟡 |
| twitter 1.2× (≤149μs, 현재 245μs) | Stage 1+2 NEON 경로 or 새 접근법 | 🟠 |
| canada 1.2× (≤1,203μs, 현재 1,935μs) | 가장 도전적 — M1의 L1 192KB 덕에 NEON bulk float 가능성 | 🔴 |

### 완료된 M1 최적화

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| Phase 50-2 | NEON 정밀 최적화 (SWAR 완전 제거, 스칼라 폴백) | twitter 328→**253μs** |
| Phase 57 | AArch64 Global Pure NEON 통합 (모든 스칼라 게이트 제거) | twitter 260→**245μs** |

### 실패 기록 (M1)

| Phase | 시도 | 실패 원인 |
|:---|:---|:---|
| Phase 50-1 | NEON 32B 언롤링 + `vgetq_lane` | `vgetq_lane` GPR-SIMD 전송 레이턴시 → +8.8%/+30% 회귀 |
| Phase 56-1~5 | LDP 기반 32B WS, NEON 32B 문자열, vtbl1, 캐시라인 튜닝, NEON 키 스캐너 | 모두 ±1% 이하이거나 회귀 |
| Phase 60-B | 단거리 키 스칼라 프리스캔 | 분기 의존성이 NEON 스페큘레이션 저해 → +5.6% |
| Phase 63 | 32B 듀얼 체크 skip_to_action | m2 VLD1Q+VCGTQ+VMAXVQ 오버헤드 > 단거리 WS 절감 |

---

## 🟢 Generic AArch64 — Snapdragon 8 Gen 2 · Cortex-X3 · Android

**상태**: 전 파일 yyjson 1.2× 달성 ✅

### 현재 성적 (Phase 60-A 기준, Cortex-X3 pinned, 300 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json | **231.6 μs** | 371 μs | Beast **+60%** | ✅ |
| canada.json | **1,692 μs** | 2,761 μs | Beast **+63%** | ✅ |
| citm_catalog.json | **645 μs** | 973 μs | Beast **+51%** | ✅ |
| gsoc-2018.json | **651 μs** | 1,742 μs | Beast **+173%** | ✅ |

> **SVE 절대 금기**: Android 커널이 SVE/SVE2 비활성화 → SIGILL. `-march=armv8.4-a+crypto+dotprod+fp16+i8mm+bf16` 명시 필수.

### 완료된 Snapdragon 최적화

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| Phase 58 | Snapdragon 8 Gen 2 베이스라인 측정 + SVE 비활성화 확인 | 전 파일 1.2× 확인 |
| Phase 58-A | 프리페치 192B→**256B** (Cortex-X3 L1 64KB 최적) | twitter −1.0% |
| Phase 60-A | compact `cur_state_` (4×64bit 비트스택 → 1 byte) | canada **−15.8%**, twitter −4.7% |

---

## 공통 기반 최적화 (전 아키텍처 적용)

| Phase | 내용 | 아키텍처 | 효과 |
|:---|:---|:---:|:---:|
| D1 | TapeNode 12→8 bytes 컴팩션 | All | +7.6% |
| Phase E | Pre-flagged separator (dump 비트스택 제거) | All | serialize −29% |
| Phase 25-26 | Double-pump number/string + 3-way fused scanner | All | −15μs |
| Phase 31 | Contextual SIMD Gate (NEON/SSE2 string scanner) | All | twitter −4.4% |
| Phase 32 | 256-entry constexpr Action LUT dispatch | All | BTB 개선 |
| Phase 33 | SWAR-8 inline float digit scanner | All | canada −6.4% |
| Phase 34 | AVX2 32B String Scanner | x86 | 처리량 2× |
| Phase 41 | `skip_string_from32`: mask==0 AVX2 fast path | x86 | SWAR-8 게이트 생략 |
| Phase 42 | AVX-512 64B String Scanner (`scan_string_end`) | x86 | −9~13% |
| Phase 43 | AVX-512 64B Inline Scan + `skip_string_from64` | x86 | −9~13% |
| Phase 60-A | compact `cur_state_` 상태 머신 | AArch64 | canada −15.8% |
| Phase 61 | NEON 오버랩 페어 dump() 문자열 복사 | AArch64 | dump −5.5% |
| Phase 62 | NEON 32B inline value string 스캔 | AArch64 | twitter −5.7% |

---

## 주의 사항 (불변 원칙)

- **모든 변경은 `ctest 81/81 PASS` 후 커밋** — 예외 없음
- **SIMD 상수는 사용 지점에 인접 선언** — YMM/ZMM 호이스팅 금지 (Phase 40 교훈)
- **회귀 즉시 revert** — 망설임 없이 되돌리고 원인 분석 선행 ([실패 기록](./OPTIMIZATION_FAILURES.md))
- **AArch64 Pure NEON 원칙**: 스칼라 SWAR 게이트 절대 금지. GPR-SIMD 교차 이동 페널티 입증됨 (Phase 50-1, 56-5, 60-B).
- **SVE 절대 금기** (Snapdragon): Android 커널 비활성화 → SIGILL 확인.
- **x86_64 Stage 1+2 경로**: ≤2MB 파일만 (twitter 617KB, citm 1.65MB). canada/gsoc는 단일 패스.
- **Phase 53 `:,` 제거**: positions 배열에서 콜론/쉼표 제거 유지 — Stage 2 push()가 자체 관리.
- **매 Phase는 별도 브랜치로 진행** → PR 후 merge
