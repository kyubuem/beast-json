# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-02-28 (Phase 41 + AVX-512 감지 버그 수정)
> **현재 최고 기록 (Phase 34, M1 Pro)**: twitter.json 264μs · canada 1,891μs · gsoc 632μs
> **현재 최고 기록 (Phase 41, Linux x86_64 AVX-512)**: twitter 351μs · canada 1,677μs · citm 797μs · gsoc 761μs (4.27 GB/s)
> **목표**: yyjson 압도 (30% 이상 우세)

---

## 압도 플랜 Phase 31-36+

📄 **Full Plan**: [OPTIMIZATION_PLAN.md](./OPTIMIZATION_PLAN.md)

---

## 할 일 목록

### Phase 31 — Contextual SIMD Gate String Scanner ⭐⭐⭐⭐⭐ ✅
- [x] `scan_string_end()` Stage1: 8B SWAR gate (short string early exit)
- [x] `scan_string_end()` Stage2: `#elif BEAST_HAS_SSE2` → SSE2 `_mm_loadu_si128` 16B loop
- [x] `scan_string_end()` Stage2: `#if BEAST_HAS_NEON` → NEON `vld1q_u8` 16B loop + `vgetq_lane_u64` pinpoint
- [x] ctest 81개 PASS
- [x] bench_all 결과: twitter **-4.4%** (276→264μs), gsoc **-11.6%** (715→632μs)
- [x] git commit `a60e265` → merge main

### Phase 32 — 256-Entry constexpr Action LUT ⭐⭐⭐⭐ ✅
- [x] `ActionId` enum + `kActionLut[256]` constexpr `std::array` 추가
- [x] `parse()` hot loop `switch(c)` → `switch(kActionLut[(uint8_t)c])` 변경
- [x] 17 char-literal cases → 11 ActionId cases 통합
- [x] ctest 81개 PASS
- [x] bench_all 결과: 전체 flat (BTB 개선, 열측정 노이즈 범위)
- [x] git commit `d2581d4` → merge main

### Phase 33 — SWAR Float Scanner ⭐⭐⭐⭐ ✅
- [x] float 소수부 scalar `while` 루프 → `BEAST_SWAR_SKIP_DIGITS()` inline macro
- [x] 지수부(`e+/-`) digit scan도 동일 macro 적용
- [x] 람다 방식 regression → macro inline으로 재작성 (zero overhead)
- [x] ctest 81개 PASS
- [x] bench_all 결과: canada **-6.4%** (2021→1891μs)
- [x] git commit `39ca6d9` → merge main

### Phase 34 — AVX2 32B String Scanner (x86_64 전용) ⭐⭐⭐ ✅
- [x] Phase 31의 SSE2 16B를 `#if BEAST_HAS_AVX2` 블록으로 AVX2 32B 업그레이드
- [x] SSE2 16B는 tail fallback으로 유지
- [x] Linux x86-64 환경 전용 (M1에서는 inactive 확인)
- [x] **[완료]** Linux x86_64 (GCC 13.3.0, -mavx2, yyjson SIMD 활성화) 에서 AVX2 가동 테스트 및 벤치마크 검증 완료. ctest 81/81 PASS. canada **-44% vs yyjson**, gsoc **-55% vs yyjson** (gsoc 4.44 GB/s). citm 사실상 타이. README.md 업데이트 완료.
- [x] ctest 81개 PASS 확인
- [x] bench_all 결과: M1은 영향 없음 (정상동작). x86_64 리눅스에서 최대 -15% 기대
- [x] git commit `c5b6b73` → merge main

### Phase 36 — AVX2 Inline String Scan (parse() hot path) ⭐⭐⭐⭐ ✅
- [x] `kActString` case: `do { break }` 패턴 → `if (s+32<=end_)` + `goto str_slow` 로 교체
- [x] `scan_key_colon_next()`: 동일 패턴 적용 (`goto skn_slow`)
- [x] 분기 redundancy 제거: mask==0 및 backslash 케이스가 SWAR-24를 bypass하고 바로 str_slow/skn_slow로 이동
- [x] Phase 37 (AVX2 whitespace skip) 시도 → citm +13% regression 확인 → **revert** (SWAR-32 복원)
- [x] **Phase 37 분석**: skip_to_action은 평균 0-8B 공백 처리 → SWAR-32 4개 병렬 스칼라 연산이 AVX2 XOR+CMPGT+MOVEMASK보다 실제 더 빠름 (포트 경쟁 없음). 보류.
- [x] ctest 81개 PASS
- [x] bench_all 결과: twitter **-4.5%** (332→318μs), canada -1.3% (1519→1501μs), gsoc -0.5%, citm ±2% (noise)
- [x] git commit → push

### Phase 35 — 멀티스레드 병렬 파싱 ⭐⭐⭐⭐⭐ ⏸️ **HOLD**
- [x] Pre-scan: `scan_toplevel_value_offsets()` 구현 완료
- [x] `parse_reuse()` → `parse_parallel(N)` API 추가 및 lock-free 병렬 파싱 실험
- [x] 병합: zero-copy in-place 파싱 및 O(1) memcpy 병합 실험
- [ ] **실험 결과**: 단일 문서 파싱 단위(GB/s) 스케일에서 `std::thread` 생성 및 join, OS 스케줄링 오버헤드가 단일스레드 파싱 시간(수백 μs)보다 커서 오히려 속도 저하 발생.
- [ ] **결론**: 단일 문서 API 수준의 내부 멀티스레딩은 적합하지 않음. 사용자가 문서 여러 개를 멀티스레드 환경에서 각각 단일스레드로 처리하는 아키텍처가 이상적임. **보류**.

### Phase 40 — AVX2 상수 호이스팅 ❌ **REVERTED**
- [x] `parse()` 루프 진입 전 `h_vq`/`h_vbs` `__m256i` 상수 선언 시도
- [x] **결과**: 전 파일 10-14% 회귀 (twitter 318→357μs, canada 1501→1705μs 등)
- [x] **원인 분석**: 두 YMM 레지스터를 parse() 전체 루프 동안 점유 → 숫자/객체/배열 처리 구간에서도 레지스터 압력 증가 → 스택 스필 발생. `vpbroadcastb` latency 1사이클로 충분히 저렴해 재계산 비용 무시 가능.
- [x] **교훈**: SIMD 상수는 사용 지점에 인접하게 선언해야 레지스터 라이프타임이 최소화됨.
- [x] git commit `b1ed9ed` — revert 포함

### Phase 41 — skip_string_from32 (mask==0 fast path) ⭐⭐⭐ ✅
- [x] `skip_string_from32(s)` 헬퍼 추가: s+32부터 AVX2 루프 진입 (SWAR-8 게이트 생략)
- [x] `kActString` mask==0 경로: `goto str_slow` → `skip_string_from32(s)` + 인라인 `push()`
- [x] `scan_key_colon_next()` mask==0 경로: `goto skn_slow` → `skip_string_from32(s)` + `goto skn_found`
- [x] SSE2 16B tail + SWAR-8 scalar tail로 플랫폼-독립적 fallback 처리
- [x] ctest 81개 PASS
- [x] **주의**: Phase 40 호이스팅과 함께 커밋되었다가 Phase 40 revert 시 같이 정리됨. Phase 41 자체는 유지.

### AVX-512 감지 버그 수정 ⭐⭐⭐⭐⭐ ✅
- [x] **버그**: `__AVX512F__` 정의 시 `#if-elif` 체인에서 `BEAST_HAS_AVX2`가 정의되지 않음 → Phase 34/36/41 전체 AVX2 코드가 AVX-512 머신에서 dead code
- [x] **수정**: `BEAST_HAS_AVX512` 블록에 `#define BEAST_HAS_AVX2 1` 추가 (AVX-512 ⊇ AVX2)
- [x] ctest 81개 PASS, objdump로 ymm 명령어 활성화 확인
- [x] git commit `b1ed9ed`

---

## 압도 기준 통과 조건

### Linux x86_64 (AVX-512, Phase 41 기준)

| 파일 | yyjson | Beast | Beast vs yyjson | 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json | 311 μs | 351 μs | yyjson 13% 빠름 | ⬜ |
| canada.json | 2,998 μs | **1,677 μs** | **Beast +44%** | ✅ |
| citm_catalog.json | 795 μs | 797 μs | **사실상 타이** | ✅ |
| gsoc-2018.json | 1,752 μs | **761 μs** (4.27 GB/s) | **Beast +57%** | ✅ |

### M1 Pro (NEON, Phase 34 기준 — 별도 머신)

| 파일 | yyjson | Beast | Beast vs yyjson | 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json (M1) | 178 μs | 264 μs | yyjson 33% 빠름 | ⬜ |
| canada.json (M1) | 1,456 μs | 1,891 μs | yyjson 23% 빠름 | ⬜ |
| citm.json (M1) | 474 μs | 646 μs | yyjson 27% 빠름 | ⬜ |
| gsoc-2018.json (M1) | 982 μs | **632 μs** | **Beast +36%** | ✅ |

**Twitter가 유일한 약점**: 이 머신에서 yyjson 대비 -13% 열세. 아래 Phase 42-45가 목표.

---

## 다음 단계 (Phase 42~45)

### Phase 42 — AVX-512 네이티브 64B 문자열 스캔 ⭐⭐⭐⭐⭐
- [ ] `scan_string_end()`에 `#if BEAST_HAS_AVX512` 블록 추가 (64B/iter, `zmm` 레지스터)
- [ ] `_mm512_cmpeq_epi8_mask` 사용으로 `vpor` 연산 제거 (마스크 직접 OR)
- [ ] 32B AVX2 블록을 tail fallback으로 유지
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 회귀 없음 확인
- **기대 효과**: gsoc/citm 긴 문자열 -10~15%, twitter 단기 효과 제한적

### Phase 43 — kActString 인라인 AVX-512 64B 원샷 스캔 ⭐⭐⭐⭐
- [ ] `kActString`의 인라인 스캔을 AVX2 32B → AVX-512 64B로 확장
- [ ] ≤63자 문자열을 단일 zmm 로드로 처리
- [ ] `scan_key_colon_next()`도 동일 패턴 적용
- [ ] ctest PASS, regression 없음 확인
- **기대 효과**: citm(긴 키) 및 twitter(64자 미만 문자열 원샷화)

### Phase 44 — twitter 병목 프로파일링 ⭐⭐⭐⭐⭐
- [ ] `perf stat -e cycles,instructions,branch-misses,L1-dcache-misses` 로 twitter 병목 정량화
- [ ] yyjson vs beast IPC 비교
- [ ] 분기 예측 미스 집중 지점 파악 → 타겟 최적화 설계
- **목적**: twitter에서 yyjson 대비 -13% 열세의 정확한 원인 파악

### Phase 45 — scan_key_colon_next SWAR-24 중복 경로 제거 ⭐⭐
- [ ] AVX2/AVX-512 인라인 스캔이 커버하는 범위와 SWAR-24 경로의 중복 분석
- [ ] 불필요한 fallthrough 경로 제거로 코드 크기 감소 → I-cache 효율화

---

## 완료된 최적화 기록 (Phase 1-41)

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| D1 | TapeNode 12→8 bytes 컴팩션 | +7.6% |
| Phase 25-26 | Double-pump number/string + 3-way fused scanner | -15μs |
| Phase 28 | TapeNode 직접 메모리 생성 | -15μs |
| Phase 29 | NEON whitespace scanner | -27μs |
| Phase E | Pre-flagged separator (dump bit-stack 제거) | -29% serialize |
| **Phase 31** | **Contextual SIMD Gate (NEON/SSE2 string scanner)** | **twitter -4.4%, gsoc -11.6%** |
| **Phase 32** | **256-entry constexpr Action LUT dispatch** | BTB 개선 (flat on M1 thermals) |
| **Phase 33** | **SWAR-8 inline float digit scanner** | **canada -6.4%** |
| **Phase 34** | **AVX2 32B String Scanner (x86_64 only)** | x86_64 처리량 2배 (M1 inactive) |
| **Phase 36** | **AVX2 Inline String Scan (kActString hot path)** | **twitter -4.5% (332→318μs)** |
| Phase 40 | AVX2 상수 호이스팅 | ❌ 전 파일 +10-14% 회귀 → revert |
| **Phase 41** | **skip_string_from32: mask==0 AVX2 fast path** | SWAR-8 게이트 생략 (≥32자 문자열) |
| **AVX-512 fix** | **BEAST_HAS_AVX2 on AVX-512 machines** | **AVX2 코드 전체 활성화** |

---

## 주의 사항

- 모든 변경은 `ctest --output-on-failure` 완전 통과 후 커밋
- canada/gsoc 등 regression 발생 시 해당 Phase revert 후 아키텍처별 조건부 재검토
- **AVX-512 머신 빌드**: `-mavx2 -march=native` 필수 (`BEAST_HAS_AVX2` 활성화)
- **YMM 레지스터 라이프타임**: SIMD 상수는 사용 지점에 인접 선언 (호이스팅 금지 — Phase 40 교훈)
- 매 Phase는 별도 브랜치로 진행 → merge request
