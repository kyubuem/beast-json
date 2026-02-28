# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-02-28 (Phase 33 완료)
> **현재 최고 기록 (Phase 33, M1 Pro)**: twitter.json 264μs · canada 1,891μs · gsoc 632μs
> **목표**: yyjson 압도 (30% 이상 우세)

---

## 압도 플랜 Phase 31-35

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

### Phase 34 — AVX2 32B String Scanner (x86_64 전용) ⭐⭐⭐
- [ ] Phase 31의 SSE2 16B를 `#if BEAST_HAS_AVX2` 블록으로 AVX2 32B 업그레이드
- [ ] SSE2 16B는 tail fallback으로 유지
- [ ] Linux x86-64 CI에서 검증 (M1에서는 inactive)
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 측정: x86_64 citm/gsoc `-15%` 추가 목표
- [ ] git commit (`feature/phase34-avx2-string`)

### Phase 35 — 멀티스레드 병렬 파싱 ⭐⭐⭐⭐⭐
- [ ] Pre-scan: SIMD로 depth=1 key offset 배열 생성 (O(n/16))
- [ ] 독립 `TapeArena` per-thread 설계
- [ ] `parse_reuse()` → `parse_parallel(N)` API 추가
- [ ] lock-free subtree 분배 로직
- [ ] 병합: main thread tape pointer 연결
- [ ] 스레드 안전성 검증 (sanitizer)
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 측정: twitter `<120μs`, canada `<950μs` 목표
- [ ] git commit (`feature/phase35-parallel-parse`)

---

## 압도 기준 통과 조건

| 파일 | yyjson | 목표 | **현재 (Phase 33)** | 달성 |
|:---|---:|---:|---:|:---:|
| twitter.json (M1) | 178 μs | **< 120 μs** | 264 μs | ⬜ |
| canada.json (M1) | 1,456 μs | **< 950 μs** | 1,891 μs | ⬜ |
| citm.json (M1) | 474 μs | **< 320 μs** | 646 μs | ⬜ |
| gsoc-2018.json (M1) | 982 μs | **< 500 μs** | **632 μs** | ✅ |

---

## 완료된 최적화 기록 (Phase 1-30)

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

---

## 주의 사항

- 모든 변경은 `ctest --output-on-failure` 완전 통과 후 커밋
- canada/gsoc 등 regression 발생 시 해당 Phase revert 후 아키텍처별 조건부 재검토
- Phase 35 병렬 파싱은 Phase 31-34 완료 후 시작
- 매 Phase는 별도 브랜치로 진행 → merge request
