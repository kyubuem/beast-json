# 📊 Beast JSON 1.0 Release — Technical API Blueprint

이 문서는 차기 에이전트들이 **"Beast JSON 1.0 정식 릴리즈"** 를 완벽하게 수행할 수 있도록, 삭제해야 할 Legacy 코드의 물리적 식별자와 새로 구축해야 할 `beast::lazy` API 아키텍처를 상세히 기술한 **Technical Blueprint(기술 설계서)** 입니다.

현재 `main` 브랜치 소스코드(`beast_json.hpp`) 기준으로 분석되었습니다.

---

## 🗑️ PART 1: Legacy 하위 호환 코드 삭제 명세서 
가장 먼저 수행해야 할 작업은 구세대 파서 로직(DOM 및 스칼라 파서)의 완전한 궤멸입니다. 이를 통해 바이너리 크기를 줄이고 API 가독성을 확보합니다.

### 1-1. 삭제 대상 핵심 클래스 및 구조체 (DOM 표현)
이 클래스들은 느린 메모리 할당(동적 할당) 기반의 DOM 객체들입니다. 모두 제거하십시오.
- `class Value` (Line ~2119 부근 시작)
  - 관련된 모든 `as_string`, `as_int`, `operator[]` 등 멤버 함수 일절.
  - 관련 `from_json`, `to_json` 오버로딩 전면 삭제.
- `class Object` 및 `class Array`
- `struct JsonMember`
- `class Document` (DOM Document)
- `class StringBuffer` / `class TapeSerializer` 
  - (단, `beast::lazy::Value::dump()`에서 쓰이는 신규 Serializer 로직은 보존해야 함에 유의)

### 1-2. 삭제 대상 파서 백엔드 (구문 분석 로직)
`beast::lazy` 타겟은 `TapeArena` 기반의 2-Pass `parse_staged` 등 특수 최적화 파서를 씁니다. 구형 문자열 토크나이저는 삭제 대상입니다.
- `class Parser` 내의 구형 구조:
  - Phase 50 이전의 `void parse()`, `parse_string()`, `parse_number()`, `parse_object()`, `parse_array()` 등 DOM 객체(`Value`)를 직접 리턴하거나 구성하는 파서 메소드 전부.
  - `parse_string_swar()`, `skip_whitespace_swar()`, `vgetq_lane` 류의 레거시 스칼라/벡터 fallback 함수(현재 Tape 기반 Lazy Parser가 사용하지 않는 함수 100% 식별 후 제거).

---

## 🏗️ PART 2: `beast::lazy` 아키텍처 개편 및 Core-Utils 분리
파일의 네임스페이스 구조를 `beast::json` (전면 삭제) -> `beast::core` (엔진) / `beast::utils` (사용자 API) / `beast::lazy` (외부 노출 인터페이스) 로 논리적 재구축해야 합니다.

### 2-1. `beast::lazy::Value` Zero-Overhead Accessors (필수)
현재 `dump()`, `is_object()`, `is_array()`만 존재하는 이 클래스에 생명을 불어넣어야 합니다.

- **원시타입 추출기 (Primitive Extractors)**
  - `std::string_view as_string_view() const` : `TapeNodeType::StringRaw`를 검사하고, 테이프 오프셋에서 원본 메모리 `string_view` 반환.
  - `int64_t as_int64() const`
  - `double as_double() const` : **[핵심]** 파서 타임에 실행되던 Russ Cox / Beast Float 부동소수점 스캐너 로직(128-bit PowMantissa)을 파서에서 잘라내어 이 `as_double()` 내부로 이식(Lazy Evaluation)해야 파싱 속도가 200μs 대로 유지됩니다.

- **컨테이너 네비게이션 (Container Navigate)**
  - `Value operator[](std::string_view key) const` : 현재 오브젝트의 시작 오프셋부터 배열 길이만큼 테이프를 선형 스캔하며 키 문자열 비교 (SIMD 활용 권장).
  - `Value operator[](size_t index) const` : 배열 요소의 O(1) 또는 SIMD 스킵 기반 접근.

### 2-2. Utils: Auto Serialize / Deserialize 
사용자 C++ 구조체를 맵핑하기 위한 템플릿/매크로 메타프로그래밍 유틸리티를 추가합니다.

- **C++ STL 컬렉션 호환성**: `std::vector<T>`, `std::map<std::string, T>`, `std::optional<T>`가 `to_json` / `from_json` 패턴이나 `BEAST_DEFINE_STRUCT()` 매크로에 의해 `beast::lazy::Value`와 완벽히 상호 호환되어야 함.

---

## 📜 PART 3: 전 세계 JSON 표준 (RFC) 100% 준수
GitHub 릴리즈 시 개발자들에게 강력히 어필할 "무결성"을 달성하기 위한 구현 디테일.

1. **RFC 8259 무결성 (코어 레벨 방어)**
   - 스택 오버플로우 공격을 막기 위한 **심도 제한(Max Depth Limit)** 하드코딩 또는 옵션 처리.
   - 유니코드 Surrogate Pair (e.g. `\uD83D\uDE00`) 디코딩 무결성 지원. (기존 코드가 지원하는지 필히 테스트 보강)
2. **Optional RFC 지원 (Utils 레벨 구현)**
   - **JSON Pointer (RFC 6901)** : `doc.at("/users/0/name")` 형태의 액세서 구축.
   - **JSON Patch (RFC 6902)** : 성능 저하 없이 DOM 부분 수정을 가능하게 하는 인터페이스 확장.

이 문서를 챗봇 / 에이전트의 프롬프트 컨텍스트로 활용하여 순차적으로 1.0 릴리즈 코딩을 시작하십시오.
