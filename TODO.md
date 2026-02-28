# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-02-28  
> **현재 최고 기록 (Phase 30)**: twitter.json 236μs (Linux x86-64)  
> **Mac (M1 Pro) 최고 기록**: twitter.json 276μs  
> **목표**: yyjson 압도 (30% 이상 우세)

---

## 압도 플랜 Phase 31-35

📄 **전체 플랜 문서**: [PHASE31_35_PLAN.md](./PHASE31_35_PLAN.md)

---

## 할 일 목록

### Phase 31 — Contextual SIMD Gate String Scanner ⭐⭐⭐⭐⭐
- [ ] `scan_string_end()` Stage1: 8B SWAR gate 추가 (short string early exit)
- [ ] `scan_string_end()` Stage2: `#if BEAST_HAS_SSE2` → `_mm_loadu_si128` 16B loop
- [ ] `scan_string_end()` Stage2: `#elif BEAST_HAS_NEON` → `vld1q_u8` 16B loop
- [ ] `scan_key_colon_next()` 동일 SIMD gate 적용
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 측정: twitter `-20%` 목표 (276→220μs)
- [ ] git commit (`feature/phase31-simd-string-gate`)

### Phase 32 — 256-Entry constexpr Action LUT ⭐⭐⭐⭐
- [ ] `namespace lazy` 상단에 `kActionLut[256]` constexpr 추가
- [ ] `parse()` hot loop `switch(c)` → `switch(kActionLut[(uint8_t)c])` 변경
- [ ] 17 cases → 11 ActionId cases로 통합
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 측정: 전체 `-8%` 목표
- [ ] git commit (`feature/phase32-action-lut`)

### Phase 33 — SWAR Float Scanner ⭐⭐⭐⭐
- [ ] `parse()` number case float 소수부 `while` 스칼라 루프 → SWAR-8 대체
- [ ] 지수부(`e+/-`) 뒤 digit scan도 동일하게 SWAR-8 적용
- [ ] ctest 81개 PASS 확인
- [ ] bench_all 측정: canada `-20%` 목표 (2021→1600μs)
- [ ] git commit (`feature/phase33-swar-float`)

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

| 파일 | yyjson | 목표 | 달성 |
|:---|---:|---:|:---:|
| twitter.json (M1) | 176 μs | **< 120 μs** | ⬜ |
| canada.json (M1) | 1,426 μs | **< 950 μs** | ⬜ |
| citm.json (M1) | 465 μs | **< 320 μs** | ⬜ |
| gsoc-2018.json (M1) | 978 μs | **< 500 μs** | ⬜ |

---

## 완료된 최적화 기록 (Phase 1-30)

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| D1 | TapeNode 12→8 bytes 컴팩션 | +7.6% |
| Phase 25-26 | Double-pump number/string + 3-way fused scanner | -15μs |
| Phase 28 | TapeNode 직접 메모리 생성 | -15μs |
| Phase 29 | NEON whitespace scanner | -27μs |
| Phase E | Pre-flagged separator (dump bit-stack 제거) | -29% serialize |

---

## 주의 사항

- 모든 변경은 `ctest --output-on-failure` 완전 통과 후 커밋
- canada/gsoc 등 regression 발생 시 해당 Phase revert 후 아키텍처별 조건부 재검토
- Phase 35 병렬 파싱은 Phase 31-34 완료 후 시작
- 매 Phase는 별도 브랜치로 진행 → merge request
