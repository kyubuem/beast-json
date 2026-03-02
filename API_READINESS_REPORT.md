# 📊 Beast JSON — Public API Readiness Report

GitHub Page/오픈소스 정식 릴리즈를 앞두고 `beast_json.hpp`의 Public API 성숙도를 검토했습니다. 결론부터 말씀드리면, **"성능은 세계 1위이지만, 사용성(API)은 아직 미완성"** 상태입니다.

## 🟢 완벽한 부분 (Ready)

1. **표준 DOM API (`beast::json::Value`)**
   - 사용자 친화적인 nlohmann/json 스타일 완벽 지원.
   - `operator[]`, `as_int()`, `as_string()`, 반복자(`begin()`, `end()`) 등 직관적이고 완벽한 API를 갖추고 있습니다.
   - 하지만 이 DOM은 객체 생성 오버헤드가 있어 우리의 "세계 최고 속도"를 뽐내는 주인공이 아닙니다.

2. **단일 헤더(Single Header) 구조**
   - 복잡한 빌드 과정 없이 `#include "beast_json.hpp"` 하나로 즉시 사용 가능한 구조는 오픈소스 배포에 매우 적합합니다.

3. **Parser / Serializer 성능**
   - `beast::lazy::parse()` 및 `doc.dump()`는 압도적인 벤치마크 데이터를 갖추고 있어 홍보 포인트로 완벽합니다.

---

## 🔴 치명적인 결함 (Blockers before Release)

우리의 주력 병기이자 가장 빠른 **`beast::lazy::Value` API의 기능 누락**이 가장 큽니다.

현재 `beast::lazy::parse(json)`을 호출하면 `beast::lazy::Value`가 반환됩니다. 하지만 이 클래스를 열어보면 다음과 같은 기능밖에 없습니다.
```cpp
// 현재 beast::lazy::Value의 모든 기능
bool is_object() const;
bool is_array() const;
std::string dump() const;
```

**[누락된 필수 API]**
- **값 추출 불가**: 숫자를 `int`나 `double`로 꺼내는 `as_int()`, `as_double()`이 없습니다. Tape 오프셋만 가지고 있을 뿐, 실제 사용자가 값을 읽을 방법이 없습니다.
- **문자열 추출 불가**: `as_string_view()` 가 없습니다. 
- **오브젝트/배열 탐색 불가**: `operator[](const char* key)`, `operator[](size_t index)` 배열/오브젝트 항목 접근 기능이 없습니다. `for (auto& item : obj)`와 같은 이터레이터도 없습니다.

> 사용자가 245μs 만에 JSON을 파싱해도, 특정 키의 값을 읽을 수 있는 API가 제공되지 않아 사실상 "파싱 후 직렬화" 작업 외에는 아무것도 할 수 없는 상태입니다.

---

## 🛠️ 해결 방안 (Action Items)

GitHub Page에 라이브러리로써 당당히 올리기 위해서는 **다음 페이즈에서 Lazy DOM Accessor API를 구현**해야 합니다.

1. **Lazy Value Accessors**
   - `as_int64()`, `as_double()`, `as_bool()`, `as_string_view()`
   - 빠른 파싱을 위해 지연(Lazy)된 숫자/문자열 구조를 실제 C++ 타입으로 변환해주는 함수 구현 (기존 Russ Cox 로직을 파싱 시점이 아닌 접근 시점으로 이동)

2. **Lazy Object/Array Navigation**
   - `Value operator[](std::string_view key)` (오브젝트 키 검색 - 선형 탐색 기반)
   - `Value operator[](size_t index)` (배열 인덱스 접근)
   - `ArrayView`, `ObjectView` 형태의 래퍼(Wrapper) 클래스를 통한 Iterator 제공

### 결론
지금 당장 GitHub Page를 런칭하기에는 "파싱은 빠르지만, 파싱한 데이터를 쓸 수 없는" 상태입니다. Phase 59 (또는 다음 페이즈)의 목표는 **"Lazy 파서의 압도적 속도를 깎아먹지 않는, 제로 오버헤드 Lazy Accessor API 개발"**이 되어야 합니다. 이 작업이 끝나면 자신 있게 1.0 버전을 런칭할 수 있습니다!
