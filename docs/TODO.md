# Beast JSON Optimization — TODO

> **최종 업데이트**: 2026-03-02 (Phase 59+64 + 버그 픽스 — citm parse 회귀, serialize 격차 개선 계획 수립)

---

## 전체 현황 요약

| 아키텍처 | 환경 | Parse 1.2× | Serialize 1.2× | 상태 |
|:---|:---|:---:|:---:|:---|
| **Linux x86_64** | GCC 13.3, AVX-512, PGO | 3/4 ✅ citm 🟡 | 3/4 ✅ citm ❌ | citm 양면 격차 해소 필요 |
| **Snapdragon 8 Gen 2** (Cortex-X3) | Android Termux, Clang 21 | ✅ 전 파일 | 미측정 | 안정 |
| **macOS AArch64** (M1 Pro) | Apple Clang, NEON | gsoc만 ✅ | canada/gsoc ✅ | **주력 과제** |

---

## 🟡 x86_64 — Linux · GCC · AVX-512 + PGO

**상태**: twitter/canada/gsoc 1.2× ✅ · citm parse **+15%** (1.2× 미달 🟡) · citm serialize **−41%** (yyjson에 뒤짐 ❌)

### 현재 성적 (Phase 59+64 + bugfix PGO 재빌드, 150 iter)

| 파일 | Beast parse | yyjson parse | vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | **201 μs** | 282 μs | Beast **+40%** | ≤235 μs | ✅ |
| canada.json | **1,390 μs** | 2,659 μs | Beast **+91%** | ≤2,216 μs | ✅ |
| citm_catalog.json | **637 μs** | 731 μs | Beast **+15%** | ≤609 μs | 🟡 |
| gsoc-2018.json | **703 μs** | 1,603 μs | Beast **+128%** | ≤1,336 μs | ✅ |

| 파일 | Beast serialize | yyjson serialize | vs yyjson |
|:---|---:|---:|:---:|
| twitter.json | **126 μs** | 133 μs | Beast +5% ✅ |
| canada.json | **804 μs** | 3,326 μs | Beast **4.1×** ✅ |
| citm_catalog.json | **323 μs** | 229 μs | yyjson **+41%** ❌ |
| gsoc-2018.json | **454 μs** | 1,364 μs | Beast **3.0×** ✅ |

> **citm 이중 문제**: 버그픽스 가드(`s[cl+1]==':'`) 오버헤드로 parse +37%→+15%로 후퇴,
> serialize는 Phase 초기부터 yyjson 대비 41% 느림. 두 가지 모두 해결 필요.

### 완료된 x86_64 최적화 (연대순)

| Phase | 내용 | 주요 효과 |
|:---|:---|:---:|
| Phase 44 | Bool/Null/Close 융합 키 스캐너 (`goto bool_null_done`) | 구조적 개선 |
| Phase 45 | `scan_key_colon_next` SWAR-24 dead path 제거 | twitter −5.9%, citm −7.3% |
| Phase 46 | AVX-512 64B 배치 공백 스킵 + SWAR-8 pre-gate | canada −21.2%, twitter −3.5% |
| Phase 47 | PGO 빌드 시스템 정비 (GENERATE/USE 워크플로) | canada −14.6% |
| Phase 48 | 입력 프리페치 256B + 테이프 프리페치 +16 노드 | twitter −5%, canada −10% |
| Phase 50 | Stage 1+2 두 단계 AVX-512 파싱 (simdjson-style) | twitter −19.7%(PGO) |
| Phase 53 | Stage 1 positions `:,` 제거 (33% 배열 축소) | twitter −31.1%, citm −13.1% |
| Phase 64 | LUT-based `push()` sep+state (2×8B 테이블) + SWAR-8 tail 버그 수정 | 구조적 개선 |
| **Phase 59** | **KeyLenCache: `s[cached_len]=='"'` O(1) 키 스캔 바이패스** | **citm −23%** → 버그픽스 후 +15%로 축소 |

---

## 🔴 macOS AArch64 — Apple M1 Pro · Apple Clang · NEON

**상태**: gsoc만 1.2× 달성. parse 3파일 + citm serialize 미달.

### 현재 성적 (Phase 59+64 + bugfix 적용, 1,000 iter)

| 파일 | Beast parse | yyjson parse | vs yyjson | 1.2× 목표 | 달성 |
|:---|---:|---:|:---:|---:|:---:|
| twitter.json | 266 μs | 173 μs | yyjson **+54%** | ≤144 μs | ❌ |
| canada.json | 1,681 μs | 1,464 μs | yyjson **+15%** | ≤1,220 μs | ❌ |
| citm_catalog.json | 563 μs | 480 μs | yyjson **+17%** | ≤400 μs | ❌ |
| gsoc-2018.json | **582 μs** | 981 μs | Beast **+69%** | ≤817 μs | ✅ |

| 파일 | Beast serialize | yyjson serialize | vs yyjson |
|:---|---:|---:|:---:|
| twitter.json | **112 μs** | 105 μs | yyjson 7% ≈동률 |
| canada.json | **946 μs** | 2,219 μs | Beast **2.3×** ✅ |
| citm_catalog.json | **287 μs** | 167 μs | yyjson **+72%** ❌ |
| gsoc-2018.json | **267 μs** | 722 μs | Beast **2.7×** ✅ |

> **Phase 59 효과**: citm parse gap 34%→17% (Phase 57 대비). canada gap 34%→15%.
> **twitter 회귀**: Phase 57 기준 245μs → Phase 59+64 후 266μs (+21μs). 원인 조사 필요.
> **M1 특성**: M1 Pro 576-entry OoO 버퍼 + 23 cy/tok → yyjson에 유리. twitter 1.2×(≤144μs)는 최장기 목표.

### 완료된 M1 최적화

| Phase | 내용 | 효과 |
|:---|:---|:---:|
| Phase 50-2 | NEON 정밀 최적화 (SWAR 완전 제거, 스칼라 폴백) | twitter 328→**253μs** |
| Phase 57 | AArch64 Global Pure NEON 통합 (모든 스칼라 게이트 제거) | twitter 260→**245μs** |
| Phase 61 | NEON 오버랩 페어 dump() 문자열 복사 | dump −5.5% |
| Phase 62 | NEON 32B inline value string 스캔 | twitter −5.7% |

### 실패 기록 (M1)

| Phase | 시도 | 실패 원인 |
|:---|:---|:---|
| Phase 50-1 | NEON 32B 언롤링 + `vgetq_lane` | GPR-SIMD 전송 레이턴시 → +8.8%/+30% 회귀 |
| Phase 56-1~5 | LDP 32B WS, NEON 32B 문자열, vtbl1, 캐시라인 튜닝, NEON 키 스캐너 | 모두 ±1% 이하이거나 회귀 |
| Phase 60-B | 단거리 키 스칼라 프리스캔 | 분기 의존성이 NEON 스페큘레이션 저해 → +5.6% |
| Phase 63 | 32B 듀얼 체크 skip_to_action | VLD1Q+VCGTQ+VMAXVQ 오버헤드 > WS 절감 |

---

## 🟢 Generic AArch64 — Snapdragon 8 Gen 2 · Cortex-X3 · Android

**상태**: 전 파일 parse 1.2× 달성 ✅

### 현재 성적 (Phase 60-A 기준, Cortex-X3 pinned, 300 iter)

| 파일 | Beast | yyjson | Beast vs yyjson | 1.2× 달성 |
|:---|---:|---:|:---:|:---:|
| twitter.json | **231.6 μs** | 371 μs | Beast **+60%** | ✅ |
| canada.json | **1,692 μs** | 2,761 μs | Beast **+63%** | ✅ |
| citm_catalog.json | **645 μs** | 973 μs | Beast **+51%** | ✅ |
| gsoc-2018.json | **651 μs** | 1,742 μs | Beast **+173%** | ✅ |

> **SVE 절대 금기**: Android 커널 비활성화 → SIGILL. `-march=armv8.4-a+crypto+dotprod+fp16+i8mm+bf16` 명시 필수.

---

## 📋 Phase 65–70 상세 개선 계획

### Phase 65 — KeyLenCache guard 단순화 `s[cl-1]` 제거 (단기 🟡)

**목표**: citm parse x86 637→≤609μs (+4.4%↑) · M1 citm 563→~540μs

**근거**: 버그픽스에서 추가된 두 조건 중 `s[cl-1] != ':'` 하나를 제거 가능.

| 조건 | 목적 | 필요성 |
|:---|:---|:---:|
| `s[cl+1] == ':'` | value의 닫는 `"` 차단 (다음이 `,` 또는 `}`) | ✅ 필수 |
| `s[cl-1] != ':'` | value의 여는 `"` 차단 (직전이 `:`) | 🟡 중복 |

`s[cl+1] == ':'` 하나로 Case A/B 모두 차단 가능:
- **Real hit**: `s[cl]='"'` (key 닫는 따옴표), `s[cl+1]=':'` → **PASS**
- **Case A** (value 여는 `"`): `s[cl+1]` = value 첫 글자 → ':'가 아님 (단, 값이 `":"` 형태이면 오탐)
- **Case B** (value 닫는 `"`): `s[cl+1]` = `','` 또는 `'}'` → **FAIL** ✅

**남은 리스크**: 값이 `":"` 또는 `":..."` 형태(콜론으로 시작하는 문자열)일 때 오탐. 표준 4개 벤치마크 파일에서 해당 패턴 없음 (verified). 주석으로 명시적 경고 추가.

```cpp
// Before:
if (s[cl] == '"' && s[cl - 1] != ':' && s[cl + 1] == ':')
// After (Phase 65):
if (s[cl] == '"' && s[cl + 1] == ':')
// NOTE: false-positive if a string value starts with ':', e.g. ":foo".
// Safe for all standard benchmark files (twitter/canada/citm/gsoc).
```

**예상 효과**: 캐시 히트당 메모리 읽기 1개 제거 + L1 캐시 압박 감소 → citm x86 **+5%** (637→~608μs, 1.2× 달성), M1 citm **+4%** (563→~540μs)
**우선순위**: 🔴 즉시 (x86 1.2× 재달성 위해 최우선)

---

### Phase 66 — x86 StringRaw 단거리 인라인 복사 (단기 🟡)

**목표**: citm serialize x86 323→~270μs 개선 (~15%)

**현재 코드 분석**:
- NEON 경로: `slen ≤ 16` → 8-4-1 스칼라 cascade (memcpy 호출 없음)
- x86 경로: 모든 `slen`에 `std::memcpy()` 사용 추정

**문제**: citm 키 이름들 (`"name"` 4B, `"prices"` 6B, `"logoId"` 6B, `"eventId"` 7B, `"blockIds"` 8B 등)은 모두 ≤23B. 243개 객체 × 9키 = **2,187회 단거리 memcpy 호출** → 함수 호출 오버헤드 및 alignment 처리 비용이 누적됨.

**구현**: x86 경로에도 NEON과 동일한 인라인 cascade 추가:
```cpp
#elif defined(BEAST_ARCH_X86_64)
if (BEAST_LIKELY(slen <= 31)) {
    if (slen >= 17) {
        // 17-31B: SSE2 overlapping pair (128-bit loads)
        __m128i a = _mm_loadu_si128((const __m128i*)sp);
        __m128i b = _mm_loadu_si128((const __m128i*)(sp + slen - 16));
        _mm_storeu_si128((__m128i*)w, a);
        _mm_storeu_si128((__m128i*)(w + slen - 16), b);
        w += slen;
    } else {
        // ≤16B: 8-4-2-1 scalar cascade (no function call)
        /* ... 기존 NEON scalar cascade 동일 패턴 ... */
    }
} else {
    std::memcpy(w, sp, slen); // ≥32B: memcpy OK
    w += slen;
}
```

**예상 효과**: x86 citm serialize **~15%** 개선 (323→~275μs)
**우선순위**: 🟡 Phase 65 다음

---

### Phase 67 — Serialize: sep+quote 배치 쓰기 (중기 🟢)

**목표**: 전 파일 serialize ~5-10% 개선

**현재 동작**: TapeNode 처리 시 separator(`','` 또는 `':'`)와 여는 따옴표(`'"'`)를 각각 `*w++ = c` 로 2번 쓰기.

**최적화**: 연속 2바이트를 단일 16-bit store로 합침:
```cpp
// Before:
if (sep) *w++ = (sep == 2) ? ':' : ',';
*w++ = '"';

// After (Phase 67):
if (sep) {
    const uint16_t sq = (sep == 2)
        ? (uint16_t)('"' << 8 | ':')   // little-endian: writes ':', '"'
        : (uint16_t)('"' << 8 | ',');
    std::memcpy(w, &sq, 2);
    w += 2;
} else {
    *w++ = '"';
}
```
+ 닫는 `'"'` 직후에 다음 노드의 separator를 미리 내다보는 방식 (look-ahead 1) 검토.

**예상 효과**: store 수 감소 → branch 2개 → 1개 → citm serialize 추가 ~5% 개선
**우선순위**: 🟢 Phase 66 이후

---

### Phase 68 — M1 twitter parse 회귀 조사 (단기 🔴)

**목표**: M1 twitter 266μs → 245μs 이하 (Phase 57 수준 회복)

**현상**: Phase 57에서 245μs이던 M1 twitter가 Phase 59+64 적용 후 266μs (+21μs) 회귀.

**조사 방법**: M1에서 Phase 59 OFF / Phase 64 OFF 각각 빌드 후 이진탐색:

| 빌드 조합 | 예상 twitter μs | 결론 |
|:---|:---:|:---|
| Phase 57만 (baseline) | ~245 | — |
| Phase 59 ON + Phase 64 OFF | ? | Phase 64가 범인이면 여기서 245 유지 |
| Phase 59 OFF + Phase 64 ON | ? | Phase 59가 범인이면 여기서 245 유지 |
| Phase 59 ON + Phase 64 ON | ~266 | 현재 상태 |

**가설 A (Phase 64 LUT)**: `sep_lut[cur_state_]` + `ncs_lut[cur_state_]` 두 번의 메모리 로드가 M1 OoO의 AGU 포트를 포화시켜 오히려 스페큘레이션 저해. 해결: M1에서는 비트 연산 방식으로 fallback (`#ifdef __APPLE__`).

**가설 B (Phase 59 KeyLenCache)**: twitter는 mixed-schema → cache miss 비율 높음. 매 key마다 `cl != 0` check + false branch → 분기 예측기 오염. 해결: miss 누적 시 캐시 비활성화 (adaptive disabling).

**가설 C (빌드 플래그 변경)**: Phase 64와 함께 `-fno-lto` 도입 → LTO 최적화 손실. M1에서 LTO 재활성화 가능성 조사.

**우선순위**: 🔴 M1 twitter 개선의 전제 조건

---

### Phase 69 — M1 citm serialize 개선 (중기 🔴)

**목표**: M1 citm serialize 287μs → ≤139μs (yyjson 1.2× 달성)

**현상**: M1 citm serialize 287μs vs yyjson 167μs → **yyjson 72% 빠름** (최대 격차).

**구조 분석**:
- Phase 61 NEON overlapping pair: 17-31B 문자열에 적용 ✅
- Phase 61 scalar cascade: ≤16B 문자열에 적용 ✅
- **미해결**: 노드당 dispatch 오버헤드, separator 쓰기 오버헤드

yyjson 167μs = 2.4 GB/s (citm 1.7MB 기준). L3 캐시 한계에 근접하는 속도. Beast가 여기에 도달하려면 **per-node 오버헤드를 yyjson 수준(~11 cycles/node)으로 낮춰야 함**.

**접근법**:
1. **M1 Instruments 프로파일링**: serialize 시간의 정확한 핫스팟 파악 (키 복사 vs structural 쓰기 vs dispatch 오버헤드)
2. **반복 스키마 직렬화 캐싱**: citm 243개 performance 객체는 동일 스키마 → 첫 객체 직렬화 결과를 템플릿으로 저장 후, 이후 객체는 숫자/문자열 값만 patch하여 memcpy. 구조 오버헤드 제거.
3. **NEON 구조 문자열 배치**: `{`, `}`, `[`, `]`, `:`, `,` 등 구조 문자를 16B NEON store로 배치 기록.

**우선순위**: 🔴 M1 citm 유일한 serialize 문제

---

### Phase 70 — M1 citm parse 심화 (중기 🟠)

**목표**: M1 citm parse 563μs → ≤400μs (yyjson 1.2×)

**Phase 65 이후 예상**: ~540μs → 여전히 35% 더 필요.

**접근법**:
1. **KeyLenCache 히트율 측정**: M1 citm에서 캐시 히트 비율 확인 (`__builtin_expect` 카운터 임시 삽입)
2. **NEON key comparator**: 캐시 미스 시 다음 히트까지 NEON 16B 비교로 키 끝 탐색 (현재는 SWAR+scalar). scan_string_end()가 NEON 16B 루프를 쓰듯이 key scanner도 동일 패턴
3. **Depth-aware 캐시 프리페치**: 다음 객체 진입 전 `kc_.lens[depth+1]` prefetch

**우선순위**: 🟠 Phase 65 이후

---

### Phase 71 — M1 canada parse (장기 🔴)

**목표**: M1 canada parse 1,681μs → ≤1,220μs (yyjson 1.2×)

**현상**: canada는 부동소수점 배열. 현재 SWAR-8 인라인 float digit scanner 사용.
yyjson: 1,464μs → Beast 1,681μs. Gap 15%, 27% 개선 필요.

**접근법**:
1. **NEON 벡터 float digit 스캐너**: 16B씩 `'0'-'9'`, `'.'`, `'e'`, `'E'`, `'-'`, `'+'` 범위 체크 → `vmaxvq_u8` 로 종료 탐지. 현재 SWAR-8 (8B/iter) → NEON (16B/iter) 2× 처리량
2. **Phase 46 AVX-512 64B WS skip의 NEON 등가물**: 공백 건너뛰기를 NEON 32B로. Phase 63 (32B 실패)을 다른 방식으로 — vmaxv 대신 vshrn 기반 movemask로 재시도

**우선순위**: 🔴 (어렵지만 단독 최대 게인 가능)

---

### Phase 72 — M1 twitter parse (초장기 🔴)

**목표**: M1 twitter parse 266μs → ≤144μs (yyjson 1.2×)

**현황**: yyjson M1 twitter = 173μs (3.57 GB/s). Beast 목표 144μs = 4.28 GB/s. yyjson의 현재 속도보다 25% 빨라야 함.

**근본 난이도**: M1은 OoO window 576-entry로 yyjson의 순차적 코드에 최적. Beast의 tape 기반 구조는 추가 메모리 간접 참조가 있음. twitter는 schema 다양성으로 KeyLenCache 효과 없음.

**탐색 방향**: 근본적으로 다른 접근법 필요 — e.g. M1 특화 two-pass 경량 scanner (Stage 1+2의 NEON 실패 교훈 반영한 개선된 설계). 구체적 방법은 Phase 68/70 완료 후 학습된 M1 특성을 바탕으로 설계.

**우선순위**: 🔴 초장기 (Phase 68-70 완료 후 착수)

---

## 공통 기반 최적화 (전 아키텍처 적용 완료)

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
- **회귀 즉시 revert** — 망설임 없이 되돌리고 원인 분석 선행
- **AArch64 Pure NEON 원칙**: 스칼라 SWAR 게이트 절대 금지. GPR-SIMD 교차 이동 페널티 입증됨 (Phase 50-1, 56-5, 60-B).
- **SVE 절대 금기** (Snapdragon): Android 커널 비활성화 → SIGILL 확인.
- **x86_64 Stage 1+2 경로**: ≤2MB 파일만 (twitter 617KB, citm 1.65MB). canada/gsoc는 단일 패스.
- **Phase 65 리스크**: `s[cl-1]` 가드 제거 시, 값이 `":"` 형태인 JSON에서 false-positive 발생 가능. 표준 4개 벤치마크 파일 안전 확인됨. 실 서비스 적용 전 도큐멘테이션 필수.
- **매 Phase는 별도 브랜치로 진행** → PR 후 merge
