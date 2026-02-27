# Beast JSON — twitter.json 성능 역전 개발계획서

> **작성일**: 2026-02-27
> **현황**: beast::lazy 314μs vs yyjson 250μs (twitter.json 기준)
> **목표**: twitter.json에서 yyjson 동등 또는 추월

---

## 1. 근본 원인 진단

실측 데이터 기반의 정확한 병목 분석 결과:

| 특성 | twitter.json | gsoc-2018.json | canada.json |
|:---|:---:|:---:|:---:|
| 공백 비율 | **29.6%** (pretty-print) | 18.7% | 0% |
| 중간 문자열 길이 | 15 bytes | 19 bytes | 7 bytes |
| ≤16bytes 문자열 비율 | **50.9%** | 29.9% | 75% |
| 객체당 평균 키 수 | **10.6** | 5.0 | 2.0 |
| 평균 중첩 깊이 | 4.4 | 2.5 | 6.7 |
| 토큰 종류 다양성 | strings+numbers+bool+null+`{[` | strings만 | numbers만 |

**핵심 결론**: beast-json이 yyjson에 지는 이유는 파서 알고리즘이 약해서가 아니다.
twitter.json이 pretty-print + 토큰 다양성이 높아서 **`skip_to_action()` 호출 빈도가 극도로 높아지는 구조 문제**다.

- gsoc-2018, canada, citm_catalog에서는 beast::lazy가 yyjson을 압도 (3/4 승리)
- twitter.json의 167KB 공백(전체의 29.6%)이 `skip_to_action()` 반복 호출을 폭발적으로 증가시킴
- beast-json에는 이미 Stage1 bitmap 인프라가 존재하지만 **parse_reuse 경로와 연결되지 않은 것**이 유일한 구조적 약점

---

## 2. 전체 로드맵 요약

| 우선순위 | Phase | 작업 | 예상 개선 | 난이도 |
|:---:|:---:|:---|:---:|:---:|
| 1 | A1 | Stage1 bitmap → parse_reuse 통합 | ~15-20% | 높음 |
| 2 | A2 | 32-byte SWAR quad-pump whitespace skip | ~5-8% | 낮음 |
| 3 | B1 | Value→Separator→Key 3-way 융합 스캐너 | ~5-7% | 중간 |
| 4 | B2 | ≤8byte 인라인 문자열 tape 내장 | 조회/직렬화 ~10% | 중간 |
| 5 | C1 | next_sib 기반 멀티스레드 분할 파싱 | ~40%+ (멀티코어) | 매우 높음 |
| 보조 | C2 | TLAB 직결 파싱 (tape 사전 할당 완전 제거) | ~2-3% | 중간 |

---

## 3. Phase A — 즉시 임팩트 (bitmap 기반 재설계)

### A1. Stage1 bitmap을 parse_reuse 핫 경로에 통합

**목표**: twitter.json의 167KB 공백 처리에서 `skip_to_action()` 호출을 0으로 줄인다.

**현재 구조의 문제**:

```
현재 beast-json 두 경로가 완전히 분리됨:

beast::lazy (parse_reuse → Phase 19 Parser)
  → byte-by-byte 스캔
  → skip_to_action() 반복 호출
  → 29.6% 공백마다 루프 오버헤드 발생

Stage1 fill_bitmap (별도 경로)
  → 64바이트 블록 단위로 모든 구조적 위치를 한 번에 인덱싱
  → SWAR prefix-XOR로 문자열 내부 추적
  → 구조적 문자 위치를 비트맵에 수집
  → 현재 parse_reuse에서 활용 안 됨
```

**목표 구조**:

```
현재: token → skip_ws → switch → token → skip_ws → switch (반복)

목표: [pre-scan: fill_bitmap → position array 생성]
       → [walk positions array: switch만, skip_ws 없음]
```

**구현 단계**:

1. `fill_bitmap()` 출력에서 압축 위치 배열 `uint32_t positions[]` 생성
2. Phase 19 Parser가 raw bytes 대신 positions 배열을 순회하도록 수정
3. `skip_to_action()` 호출 경로 제거 (positions 배열이 이미 공백을 건너뜀)
4. positions 배열은 스택 또는 TLAB에서 할당 (heap 할당 없음)

**예상 효과**: twitter.json `skip_to_action()` 호출 제거 → **~15-20% 단축**

---

### A2. 32-byte SWAR quad-pump whitespace skip (SIMD 없이)

**목표**: A1 구현 전/후 fallback 경로에서도 공백 처리 처리량 4배 향상.

**현재 코드 (8바이트씩)**:

```cpp
// 현재 (8바이트씩)
while (p_ + 8 <= end_) {
    uint64_t am = swar_action_mask(load64(p_));
    if (am) { p_ += CTZ(am) >> 3; return *p_; }
    p_ += 8;
}
```

**목표 코드 (32바이트 4-pump)**:

```cpp
// 목표 (32바이트씩, SWAR 4-pump)
while (p_ + 32 <= end_) {
    uint64_t a0 = swar_action_mask(load64(p_));
    uint64_t a1 = swar_action_mask(load64(p_ + 8));
    uint64_t a2 = swar_action_mask(load64(p_ + 16));
    uint64_t a3 = swar_action_mask(load64(p_ + 24));
    if (a0 | a1 | a2 | a3) {
        // 첫 번째 hit 위치를 정확히 탐색
        if (a0) { p_ += CTZ(a0) >> 3; return *p_; }
        if (a1) { p_ += 8 + (CTZ(a1) >> 3); return *p_; }
        if (a2) { p_ += 16 + (CTZ(a2) >> 3); return *p_; }
        p_ += 24 + (CTZ(a3) >> 3); return *p_;
    }
    p_ += 32;
}
// 기존 8바이트 fallback...
```

**구현 포인트**:
- `a0|a1|a2|a3`로 OR 병합 → 분기 1회로 32바이트 체크
- 현대 CPU OOO(Out-of-Order) 실행으로 4개 load64가 파이프라인에서 병렬 처리됨
- SIMD 없이 x86-64 / ARM64 모두 동작

**예상 효과**: pretty-print JSON 공백 처리량 4배 → twitter.json **~5-8% 단축**

---

## 4. Phase B — 중간 임팩트 (fused scanner 확장)

### B1. Value→Separator→Key 3-way 융합 스캐너

**목표**: 객체당 10.6쌍인 twitter.json에서 반복적인 함수 호출 오버헤드 제거.

**현재 경로**:

```
[string 파싱]
  → [double-pump: , 소비]
  → [skip_to_action]
  → [" 확인]
  → [scan_key_colon_next]
  (→ 다음 쌍으로 반복)
```

**목표 경로**:

```
[string 파싱]
  → [fused_val_sep_key: , 즉시 탐지 → 키 스캔 → : 소비]
  → 단일 함수에서 완결
```

**구현 핵심**:
- `scan_key_colon_next()`를 확장하여 앞선 값의 종료 구분자(`,` 또는 `}`)까지 처리
- twitter.json 객체 10.6쌍 × 함수 호출 절감 = 반복 오버헤드 대폭 감소
- `skip_to_action()` 중복 호출 경로 통합 제거

**예상 효과**: twitter.json 객체 내 순회 함수 호출 제거 → **~5-7% 단축**

---

### B2. ≤8byte 인라인 문자열 tape 직접 내장

**목표**: twitter.json 38.7% 문자열(≤8bytes)에 대한 캐시 미스 제거.

**현재 TapeNode 구조**:

```cpp
struct TapeNode {
    TapeNodeType type;   // 1 byte
    uint8_t flags;       // 1 byte
    uint16_t length;     // 2 bytes
    uint32_t offset;     // 4 bytes  ← 원본 JSON 위치 포인터 (캐시 미스 원인)
    uint32_t next_sib;   // 4 bytes
    uint32_t aux;        // 4 bytes  ← 현재 미사용
};  // 16 bytes total
```

**목표 구조 (inline-string 모드 추가)**:

```cpp
// flags 비트 정의
constexpr uint8_t INLINE_STR = 0x01;

// ≤8bytes 문자열: next_sib(4) + aux(4) = 8바이트에 직접 내용 복사
// 파싱 시:
if (node.length <= 8) {
    node.flags |= INLINE_STR;
    memcpy(&node.next_sib, src_ptr, node.length);
    // offset은 0으로 (unused)
}

// 조회 시:
inline std::string_view get_string_sv() const {
    if (flags & INLINE_STR) {
        return std::string_view(
            reinterpret_cast<const char*>(&next_sib), length);
    }
    return std::string_view(base_ptr + offset, length);
}
```

**구현 포인트**:
- 파싱 시 `memcpy` 8바이트 → 컴파일러가 단일 `STR` 명령으로 최적화
- `find()`, `get_string()` 호출 시 원본 JSON 메모리 접근(캐시 미스) 없이 tape만으로 완결
- 직렬화(dump) 경로도 동일하게 캐시 미스 감소

**예상 효과**: 단순 키-값 조회 및 직렬화에서 캐시 미스 감소 → **~10% 향상**

---

## 5. Phase C — 고유 혁신 (업계 미존재 기법)

### C1. next_sib를 이용한 투기적 병렬 파싱

**목표**: beast-json의 고유 next_sib 링크 구조를 멀티스레드 분할 파싱에 활용.

**원리**:

```
{                    ← next_sib 링크로 파싱 완료 위치 즉시 접근 가능
  "key1": <subtree1> ← 스레드 1 담당
  "key2": <subtree2> ← 스레드 2 담당
  "key3": <subtree3> ← 스레드 3 담당
}
```

**전제 조건**:
- Phase A1의 Stage1 bitmap pre-scan 완성 → 상위 깊이 오브젝트 키 위치 사전 파악
- TLAB가 스레드별 독립 할당 지원 → 합병 오버헤드 최소화

**구현 단계**:
1. Stage1 bitmap에서 depth=1 키 위치 목록 추출 (O(n) pre-scan)
2. 키 개수 기준으로 스레드 작업 분할
3. 각 스레드가 독립 TLAB에서 subtree 파싱
4. 메인 스레드에서 next_sib 링크로 결과 합병

**예상 효과**: 멀티코어 활용 → **~40%+ 단축** (4코어 기준)

---

### C2. TLAB 직결 파싱 (tape 사전 할당 완전 제거)

**목표**: `parse_reuse` 내 malloc/realloc 판단 로직 및 경계 체크 제거.

**현재**:
```
parse_reuse → tape.base 크기 확인 → 불충분하면 realloc
             → tape_head_ 설정 → arena->head 경계 체크
```

**목표**:
```
TLAB 공간 직결 → tape_head_++ → arena 내부에서만 동작
               → 경계 체크 불필요 (TLAB 크기가 JSON보다 항상 크게 사전 설정)
```

**구현 포인트**:
- TLAB를 `json.size() * 2` 기준으로 사전 예약 (twitter.json: ~1.2MB)
- `tape_head_`가 TLAB 내부 포인터로 직접 동작
- realloc path 코드 제거 → 분기 감소

**예상 효과**: 파싱 초기화 오버헤드 제거 → **~2-3% 단축**

---

## 6. 구현 순서 및 마일스톤

```
Week 1: Phase A2 (32-byte SWAR quad-pump)
  → 난이도 낮음, 즉시 twitter.json 5-8% 개선
  → 기존 NEON fallback과 공존 가능

Week 2-3: Phase A1 (Stage1 bitmap → parse_reuse 통합)
  → 핵심 구조 변경, 충분한 테스트 필요
  → 완성 시 twitter.json 15-20% 추가 개선
  → 81개 기존 테스트 전체 통과 유지 필수

Week 4: Phase B1 (3-way 융합 스캐너)
  → A1 완성 후 시너지 극대화
  → scan_key_colon_next() 확장

Week 5-6: Phase B2 (인라인 문자열 tape 내장)
  → TapeNode 레이아웃 변경 → 하위 호환성 검토
  → find() / get_string() / dump() 전체 수정

Week 7+: Phase C1 (멀티스레드 분할 파싱)
  → A1 완성이 전제 조건
  → 설계 → 프로토타입 → 스레드 안전성 검증
```

---

## 7. 테스트 및 검증 기준

### 회귀 방지
- 모든 변경 후 `ctest --output-on-failure` 통과 필수 (81개 테스트)
- canada.json, citm_catalog.json, gsoc-2018.json 성능 회귀 없어야 함

### 성능 목표

| 마일스톤 | twitter.json 목표 | 기준 |
|:---|:---:|:---|
| A2 완료 | ≤300μs | 현재 314μs |
| A1 완료 | ≤265μs | yyjson 250μs에 근접 |
| A1+B1 완료 | ≤250μs | yyjson 동등 |
| A1+B1+B2 완료 | ≤230μs | yyjson 추월 |
| C1 완료 (4코어) | ≤150μs | yyjson 압도 |

### 벤치마크 환경
- 현재 환경: Linux x86-64, GCC 13.3.0 `-O3`, 300 iterations
- A1 이후: ARM64 NEON 경로도 별도 측정
- C1 이후: 코어 수별 스케일링 측정 추가

---

## 8. 핵심 메시지

> **A1 하나만 제대로 구현해도 twitter.json에서 yyjson와 동등하거나 앞설 가능성이 높다.**

beast-json은 이미 bitmap 인프라, TLAB, next_sib 링크라는 세 가지 고유 강점을 보유하고 있다.
이 강점들이 아직 parse_reuse 핫 경로와 연결되지 않은 것이 현재의 유일한 구조적 약점이다.

연결이 완성되면:
- **단기 (A 완료)**: twitter.json에서 yyjson 추월
- **중기 (B 완료)**: 모든 벤치마크에서 yyjson 압도
- **장기 (C 완료)**: 멀티코어 기준 세계 최속 JSON 파서 달성
