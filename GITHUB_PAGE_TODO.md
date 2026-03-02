# 🚀 Beast JSON 1.0 Release (GitHub Pages Prep)

Beast JSON을 명실상부한 **세계에서 가장 빠르고 사용하기 편한 C++ JSON 라이브러리**로 GitHub Page에 정식 런칭하기 위한 최종 준비 로드맵입니다.

이 계획의 핵심은 **미사용 Legacy 코드의 완전 삭제**와 **압도적으로 빠른 `beast::lazy` API의 완성**입니다.

---

## 🗑️ 1. Legacy DOM 완전 삭제 (Cleanup)
이전 세대의 느린 DOM 파싱 로직(`beast::json::Value`, `beast::json::Object`, `beast::json::Array`, `beast::json::Parser` 등)을 코드베이스에서 영구히 제거합니다.
- [ ] `include/beast_json/beast_json.hpp`에서 기존 `Value`, `Object`, `Array` 클래스 정의 삭제
- [ ] 구형 `Parser::parse()` 및 관련 스칼라/SIMD 백엔드 코드 일괄 삭제
- [ ] 테스트 코드(`tests/`) 및 벤치마크(`benchmarks/`)에서 기존 DOM을 참조하는 코드 모두 `beast::lazy` 기반으로 마이그레이션 또는 삭제

## 🏗️ 2. Lazy DOM Accessor API 구현 (Core Feature)
단일 헤더 파서인 `beast::lazy::Value`에 사용자가 파싱된 데이터를 손쉽게 꺼내 쓸 수 있는 직관적인 API를 추가합니다. 이 API들은 파싱 시점의 오버헤드를 0으로 유지하고, **접근 시점(On-demand)에만 연산**을 수행합니다.

### 2.1 Primitive Accessors (원시 타입 접근)
- [ ] `as_int64() const` : NumberRaw 테이프 참조에서 정수 파싱 (Fast-path + Russ Cox)
- [ ] `as_double() const` : NumberRaw 테이프 참조에서 부동소수점 파싱 (Beast Float / Russ Cox)
- [ ] `as_bool() const` : BooleanTrue / BooleanFalse 테이프 타입 반환
- [ ] `as_string_view() const` : StringRaw 테이프 참조에서 `std::string_view` 반환

### 2.2 Container Navigation (오브젝트/배열 탐색)
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

## 🧪 3. 1.0 Release Verification
- [ ] `tests/test_lazy_api.cpp`를 신규 작성하여 Accessor들의 정확성 검증 (특히 Russ Cox 플로트 추출 로직의 정확도 보장)
- [ ] 벤치마크 파일 업데이트: 기존 `beast::lazy` 벤치마크는 그대로 유지되는지(회귀 없음) 확인
- [ ] `README.md` 내 Usage(사용법) 섹션을 새로 추가된 "Lazy API 사용법"으로 전면 개편

---
이 TODO 리스트가 모두 완료되면, Beast JSON은 **"가장 빠르면서도 누구나 쉽게 쓸 수 있는"** 완벽한 1.0 릴리즈 상태가 됩니다.
