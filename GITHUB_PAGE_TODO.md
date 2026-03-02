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

### 2.2 Extreme Usability API (독점적 사용자 편의성 혁신)
- [ ] **Pipe Operator Fallback (`|`)**: `doc["users"][0]["age"] | 18` 처럼 에러 상태를 단 한 줄로 우아하게 막아내는 무예외(Throw-less) 폴백 연산자 구현.
- [ ] **Zero-Allocation Typed Views**: 배열 순회 시 동적 할당 없이 즉시 타입 캐스팅하며 순환하는 `as_array<T>()` 구현 (`for(int id : doc["ids"].as_array<int>())`).
- [ ] **C++20 Compile-Time JSON Pointer**: 런타임 탐색 비용을 0으로 만드는 컴파일 타임 문자열 탐색 `doc.at<"/api/config/timeout">()` 지원.
- [ ] **Type-Safe Matcher**: 스키마리스 JSON 값을 안전하게 분기할 수 있는 Rust 스타일 판별기 `doc["val"].match([](int v){}, [](string s){})` 구현.
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

- [x] **코어 문법 완벽 파싱 검증 완료 (Phase 48)**: 81개의 포괄적인 오프라인 테스트 케이스(UTF-8, String escapes, Numbers, Structures) 전면 통과.
- [ ] **최종 JSON Test Suite 통합 검증**: `NST` (Nicolas Seriot's JSON Test Suite) 및 역대 표준 `JSONChecker` 스위트를 통합하여 Stack Overflow 방어, 쌍둥이 키(Duplicate keys), Surrogates 유효성, IEEE754 한계치 처리 등 RFC 8259 엣지 케이스 100% 방어망 구축.
- [ ] **확장 RFC 1: JSON Pointer (RFC 6901)** 지원: `beast::utils::json_pointer`를 통한 문자열 경로 기반 DOM 탐색 및 수정 지원.
- [ ] **확장 RFC 2: JSON Patch (RFC 6902)** 지원: 배열/객체에 대한 `add`, `remove`, `replace`, `move`, `copy`, `test` 등 상태 변이 추적 알고리즘 구현.
- [ ] **확장 RFC 3: JSON Merge Patch (RFC 7396)** 지원: HTTP PATCH 메서드 등에 활용 가능한 문서간 병합 알고리즘 지원.
- [ ] **기능 지원: 통제된 파서 옵션 (Parse Options)**: RFC 8259 엄격 모드(Strict)와 후행 쉼표(Trailing commas) 및 주석(Comments)을 허용하는 이완 모드(Relaxed) 간의 완벽한 분리 및 지원 체계 구현.

## 🌐 5. Foreign Language Bindings (타 언어 확장)
세계에서 가장 빠른 C++ 엔진을 타 언어 생태계에서도 누릴 수 있도록 바인딩 기초를 다집니다.
- [ ] 순수 C API (`extern "C"`) Export 모듈 작성 (FFI 연동용).
- [ ] **Python Binding**: `pybind11` 또는 `ctypes`를 활용한 Python 모듈 제공 준비. 파이썬 기본 `json` 모듈을 대체할 수 있는 직관성 확보.
- [ ] **Node.js Binding**: `N-API` 기반의 Node.js 네이티브 애드온 제공 준비.

## 🧪 6. 1.0 Release Verification
- [ ] `tests/test_lazy_api.cpp` 작성 (Accessor 정확성 및 Russ Cox 검증)
- [ ] `tests/test_auto_serialize.cpp` 작성 (구조체 자동 변환 검증)
- [ ] 벤치마크 파일 업데이트: 회귀(Regression) 없음 확인
- [ ] `README.md` 전면 개편: Core/Utils 구조 분리 채택 설명 및 Lazy API / 자동 변환 사용법 추가

---
이 TODO 리스트가 모두 완료되면, Beast JSON은 **"가장 빠르면서도 누구나 쉽게 쓸 수 있는"** 완벽한 1.0 릴리즈 상태가 됩니다.
