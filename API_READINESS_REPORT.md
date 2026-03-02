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

## 🏗️ PART 2: 아키텍처 레이어링 (Core-Utils-API 분리) 및 사용자 친화적 네이밍
최종 사용자는 내부적으로 파서가 지연 평가(Lazy)를 쓰는지, DOM을 구축하는지 알 필요가 없어야 합니다. 가장 직관적이고 표준적인 네이밍을 제공하면서도, 내부 코드는 철저히 분리된 레이어로 관리되어야 합니다.

전문가 수준의 라이브러리 설계를 위해 다음 3-Tier 아키텍처를 채택합니다.

### Layer 1: The Core Engine (`namespace beast::core`)
JSON을 물리적으로 파싱하고 직렬화하는 절대적인 "엔진부"입니다. 외부(사용자)에선 이 레이어의 존재나 클래스를 직접 조작하지 않습니다.
- **포함 대상**: SIMD/SWAR 스캐너(`simd`, `lookup`), 이스케이프 파서, 숫자 파싱(Russ Cox `PowMantissa`, `Unrounded`), `TapeArena`, `Stage1Index`, 코어 `Parser` 및 `Serializer`.
- **목표**: 100% RFC 8259 준수, 제로 할당, 최대 ILP(Instruction-Level Parallelism) 기반의 처리 속도 확보.

### Layer 2: The Utilities (`namespace beast::utils` 또는 `beast::ext`)
코어 데이터 위에 확장 기능을 부여하는 유틸리티/플러그인 레이어입니다.
- **포함 대상**: C++ 매크로/템플릿 기반 자동 O/R 매퍼(`to_json`, `from_json`, `BEAST_DEFINE_STRUCT`), JSON Pointer (RFC 6901), JSON Patch (RFC 6902) 등.
- **목표**: 코어 엔진의 가벼움을 해치지 않으면서, 필요할 때만 인클루드/사용하여 생산성을 극대화.

### Layer 3: The Public API (`namespace beast`)
사용자가 최종적으로 마주치는 "단일 진입점(Facade)"입니다. 기존의 `lazy`라는 구현 종속적 이름은 내부로 숨기고 표준적인 네이밍을 노출합니다.

- **`beast::Value` (기존 `beast::lazy::Value` 진화형)**:
  - 사용자는 오직 `beast::parse("...")`를 통해 `beast::Value`를 받습니다.
  - 내부적으로는 Tape 참조를 들고 있는 Lazy 객체이지만, 겉으로는 완벽한 DOM 객체처럼 동작합니다.
  - **필수 구현 접근자**: `as_int64()`, `as_double()`, `as_string_view()`, `as_bool()`, `operator[](std::string_view)`, `operator[](size_t)`.
- **`beast::parse()`**: 코어의 `parse_staged()`를 감싸는 래퍼 함수.

### Layer 4: Modern Error Handling (`std::optional`) & Fluent Chaining
기존의 복잡한 예외(Exception) 던지기나 에러 코드 반환 대신, C++17의 `std::optional`을 활용하여 **"안전하고 직관적인 에러 핸들링"** 체계를 구축합니다.

- **안전한 데이터 추출 (Safe Accessors)**:
  - `std::optional<int64_t> get_int64() const` 
  - `std::optional<std::string_view> get_string() const`
  - 값이 존재하지 않거나 타입이 틀릴 경우 `std::nullopt`를 반환하여 사용자가 `if (auto val = obj["key"].get_int64())` 형태로 매우 우아하게 에러를 처리할 수 있게 합니다.

- **세계 최고 수준의 사용자 편의성 (Extreme Usability)**:
  - **1) Fluent Chaining (무예외 연속 탐색)**: `doc["users"][0]["name"]` 처럼 깊은 탐색을 할 때, 중간에 "users"가 배열이 아니거나 "name" 키가 없어도 예외(Throw)를 던지며 프로그램이 죽지 않습니다. 대신 내부적으로 `Null/Error Value` 상태를 전파하여 맨 마지막에 `.get_string().value_or("Unknown")`으로 한 번에 안전하게 폴백(Fallback)할 수 있는 Monadic 기법을 도입합니다.
  - **2) Default Value Getters**: `doc["age"].get_int64(18)` 처럼 값이 없거나 타입이 다를 때 기본값을 즉시 반환하는 직관적인 API 제공.
  - **3) Range-based For Loop & Structured Binding**: `for (auto [key, val] : obj.items()) { ... }` 형태로 C++17 구조적 바인딩을 완벽 지원하여 파이썬 딕셔너리를 순회하는 것과 동일한 극한의 편의성 제공.
## 🏆 PART 4: 타겟 API 벤치마킹 및 "세계 최고의 사용성" 설계
순수 성능뿐만 아니라 "개발자 경험(DX)"에서도 경쟁 라이브러리를 압도해야 합니다. 최근 트렌드인 `nlohmann/json`, `glaze`, `rapidjson`의 장단점을 분석하고 **Beast JSON만의 궁극적인 API**를 설계합니다.

### 4-1. 경쟁 라이브러리 사용성 분석
1. **nlohmann/json ("직관성의 제왕")**
   - **장점**: C++ STL과 100% 동일하게 동작. `json j = "{...}"_json; int age = j["age"];` 방식의 미친 직관성. 언제 어디서든 구조체를 쉽게 변환 가능 (`nlohmann::from_json`).
   - **단점**: 무거운 DOM 생성과 수많은 동적 할당으로 성능이 세계 최하위 수준.
2. **Glaze ("모던 메타프로그래밍의 끝판왕")**
   - **장점**: C++20 `constexpr` 메타프로그래밍을 활용하여 `glz::read<MyStruct>(str)` 단 한 줄로 JSON을 구조체로 곧바로 꽂아넣음. 속도와 사용성 모두 압도적.
   - **단점**: DOM 탐색이 취약하고 전체 매핑에만 집중되어 있어, 구조체를 정의하지 않고 임의의 JSON을 탐색할 때는 불편함.
3. **RapidJSON ("성능의 구세주, 그러나 악몽 같은 API")**
   - **장점**: In-situ 파싱 및 빠르고 가벼운 C스타일 탐색.
   - **단점**: `document["hello"].GetString()` 처럼 낡은 API. 타입 체크를 하지 않으면 바로 `추락(Segfault/Throw)`. 반복자 사용법이 끔찍하게 김.

### 4-2. Beast JSON 1.0의 궁극적 API 설계 (The Ultimate API)
Beast JSON은 **"Glaze의 매법(구조체 파싱 성능) + nlohmann의 우아함(DOM 탐색) + RapidJSON의 Zero 할당 성능"** 을 모두 융합한 단일 API를 제공합니다. 문서 한 줄만 읽어도 사용법을 깨달을 수 있는 수준(Zero Learning Curve)을 지향합니다.

**🔥 기능 1: 암시적 형변환 (Implicit Conversion - Nlohmann Style)**
`get_int64()` 같은 긴 함수명 없이, 할당 연산자로 즉시 값을 꺼내는 마법.
```cpp
beast::Value doc = beast::parse(json_str);

// 1. 타입을 적어주면 알아서 꺼내옵니다. (안전하게!)
int age = doc["user"]["age"]; 
std::string name = doc["user"]["name"];
bool is_admin = doc["is_admin"];

// 2. 키가 없을지도 모르는 상황? std::optional 로 받으면 끝!
std::optional<int> score = doc["score"]; 
```

**🔥 기능 2: 1-Line 구조체 역직렬화 (Glaze Style)**
Glaze처럼, 매크로 하나만 선언하면 JSON 문자열을 구조체로 "직렬 파싱" 해버리는 극한의 편의성.
```cpp
struct User { std::string name; int age; };
BEAST_DEFINE_STRUCT(User, name, age);

// JSON 문자열 -> 구조체 즉시 변환
User u = beast::read<User>(json_str);

// 구조체 -> JSON 문자열 역변환
std::string json = beast::write(u);
```

**🔥 기능 3: Fluent Monadic Chaining**
중간에 키가 없어도 절대 뻗지 않는 안전한 깊은 탐색.
```cpp
// "data"가 없거나, 배열이 아니거나, 첫번째 원소가 없거나, "id" 키가 없어도 죽지 않음.
// 못 찾으면 안전하게 -1 반환.
int id = doc["data"][0]["id"].value_or(-1);
```

이 구조 도입을 위해 내부 구현 단에서 `operator T()` 암시적 형변환 연산자와 `beast::read<T>`, `beast::write<T>` 글로벌 유틸리티 함수를 철저히 구현해야 합니다.

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
