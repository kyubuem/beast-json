# 🚀 Beast JSON 1.0 Release (GitHub Pages Prep)

Beast JSON을 명실상부한 **세계에서 가장 빠르고 사용하기 편한 C++ JSON 라이브러리**로 GitHub Page에 정식 런칭하기 위한 최종 준비 로드맵입니다.

이 계획의 핵심은 **미사용 Legacy 코드의 완전 삭제**와 **압도적으로 빠른 `beast::lazy` API의 완성**입니다.

---

## 🗑️ 1. 구조 개편 및 Legacy DOM 완전 삭제 (Architecture & Cleanup)
이전 세대의 느린 DOM 파싱 로직(`beast::json::Value` 등)을 영구히 제거하고, **핵심 속도 엔진(Core)과 사용자 편의 기능(Utils)을 논리적/물리적으로 완전히 분리**합니다.

- [ ] `include/beast_json/beast_json.hpp`에서 기존 `Value`, `Object`, `Array` 클래스 및 파서 백엔드 일괄 삭제.
- [ ] 파일 및 네임스페이스 구조 개편: 
  - **Core Layer**: 순수 고속 파싱/직렬화 엔진 (`tape`, `Parser`, `simd` 등)
  - **Utils/API Layer**: `lazy::Value` 접근자 및 매크로 기반 편의 기능
- [ ] 테스트 코드(`tests/`) 및 벤치마크(`benchmarks/`)를 순수 `beast::lazy` 기반으로 마이그레이션.

## 🏗️ 2. Lazy DOM Accessor API 구현 (Core Feature)
단일 헤더 파서인 `beast::lazy::Value`에 사용자가 파싱된 데이터를 손쉽게 꺼내 쓸 수 있는 직관적인 API를 추가합니다. 이 API들은 파싱 시점의 오버헤드를 0으로 유지하고, **접근 시점(On-demand)에만 연산**을 수행합니다.

### 2.1 The Ultimate API (Implicit Conversions - nlohmann style)
사용자가 게터 이름을 외울 필요 없이 값을 바로 꺼낼 수 있는 마법 같은 직관성을 제공합니다. 내부적으로는 안전한 타입 체킹을 병행합니다.
- [ ] `operator int64_t() const`, `operator double() const`, `operator bool() const`, `operator std::string_view() const` 형태의 암시적 형변환 지원 (`int age = doc["age"];`)
- [ ] Safe Accessors 백업 (`std::optional<T> get<T>()` 호환 지원)

### 2.2 Extreme Usability API (세계 최고 수준 사용자 편의성)
- [ ] **Fluent Chaining (Monadic Error Handling)**: `doc["users"][0]["name"]` 접근 시 중간 키가 없어도 Throw 없이 `Null` 객체를 반환하여 무예외 연속 탐색 지원.
- [ ] **Default Value Getters**: `get_string("Unknown")` 처럼 키가 없을 때 지정된 기본값을 반환하는 API 제공.
- [ ] **Range-based For Loop & Structured Binding**: `for (auto [key, val] : obj.items())` 문법 지원.

### 2.3 Container Navigation (오브젝트/배열 탐색)
- [ ] `operator[](std::string_view key) const` : Object 타입에서 Key를 선형 탐색(SIMD 활용 가능)하여 매칭되는 Value 반환
- [ ] `operator[](size_t index) const` : Array 타입에서 n번째 요소 반환 (테이프 스킵 최적화 알고리즘 적용)
- [ ] `size() const` : Array 또는 Object의 요소 개수 반환
- [ ] `contains(std::string_view key) const` : 키 존재 여부 확인

### 2.3 Type Checking (타입 확인)
- [ ] `is_null() const`
- [ ] `is_number() const`
- [ ] `is_string() const`
- [x] `is_object() const` (기존재)
- [x] `is_array() const` (기존재)

## 🧬 3. Auto Serialize/Deserialize (Glaze Style Meta-programming)
- [ ] `glz::read<T>` 처럼 1줄 만에 JSON 문자열을 C++ 구조체로 곧바로 꽂아넣는 `beast::read<T>(json)` 함수 구현.
- [ ] 매크로 지원: `BEAST_DEFINE_STRUCT(StructName, field1, field2)` 로 사용자 데이터 매핑 단순화.
- [ ] `std::vector`, `std::map`, `std::optional` 등 기본 C++ STL 컨테이너 컬렉션 완벽 자동 대응.

## 📜 4. 전 세계 모든 JSON 표준(RFC) 100% 완벽 준수 검증
오픈소스 라이브러리로서의 "절대적 신뢰성"을 위해, 필수(Mandatory) 사양뿐만 아니라 선택적(Optional) 기능과 확장 표준 체계까지 세상에 존재하는 모든 JSON 관련 RFC를 빠짐없이 지원해야 합니다.

- [ ] **코어 문법의 무결성 (RFC 8259, RFC 7159, RFC 4627)**:
  - JSON Test Suite (NST, JSONChecker 등 300+개 테스트셋) 통합 및 자동화 빌드(`ctest`) 검증.
  - 깊이 제한 방어 (Stack Overflow 방지), UTF-8 / Surrogate Pair 이스케이프 완벽 처리, 부동소수점(`NaN`, `Infinity`, 언더플로우/오버플로우) 거부 및 수용 옵션 등 코너 케이스 완벽 제어.
- [ ] **확장 표준 연산자 (Optional RFCs)**:
  - **JSON Pointer (RFC 6901)**: `doc["/foo/bar/0"]` 기반의 경로 탐색 기능 구현.
  - **JSON Patch (RFC 6902)**: 두 JSON 객체의 병합, 수정, 삭제 프로토콜 지원.
  - **JSON Merge Patch (RFC 7396)**: 간소화된 부분 업데이트 사양 구현.
- [ ] **Strict & Relaxed Parser Options**: 
  - 기본값은 초고속 + 완벽한 RFC 8259 엄격성(Strictness) 유지.
  - 사용자 옵션을 통해 Trailing Comma 허용, C-style 주석(`//`, `/* */`) 허용 등 범용적인 비표준 확장(Relaxed Mode) 지원 완료 확인.

## 🧪 5. 1.0 Release Verification
- [ ] `tests/test_lazy_api.cpp` 작성 (Accessor 정확성 및 Russ Cox 검증)
- [ ] `tests/test_auto_serialize.cpp` 작성 (구조체 자동 변환 검증)
- [ ] 벤치마크 파일 업데이트: 회귀(Regression) 없음 확인
- [ ] `README.md` 전면 개편: Core/Utils 구조 분리 채택 설명 및 Lazy API / 자동 변환 사용법 추가

---
이 TODO 리스트가 모두 완료되면, Beast JSON은 **"가장 빠르면서도 누구나 쉽게 쓸 수 있는"** 완벽한 1.0 릴리즈 상태가 됩니다.
