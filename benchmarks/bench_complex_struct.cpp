#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>
#include <yyjson.h>

// Complex nested structures
struct Address {
  std::string street;
  std::string city;
  std::string country;
  int zip_code;

  bool operator==(const Address &other) const {
    return street == other.street && city == other.city &&
           country == other.country && zip_code == other.zip_code;
  }
};

struct Person {
  std::string name;
  int age;
  std::string email;

  bool operator==(const Person &other) const {
    return name == other.name && age == other.age && email == other.email;
  }
};

struct Company {
  std::string name;
  std::vector<Person> employees;
  std::map<std::string, std::vector<int>> departments;
  std::optional<Address> headquarters;

  bool operator==(const Company &other) const {
    return name == other.name && employees == other.employees &&
           departments == other.departments &&
           headquarters == other.headquarters;
  }
};

// beast_json reflection
BEAST_DEFINE_JSON(Address, street, city, country, zip_code);
BEAST_DEFINE_JSON(Person, name, age, email);
BEAST_DEFINE_JSON(Company, name, employees, departments, headquarters);

// nlohmann/json serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Address, street, city, country, zip_code);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Person, name, age, email);

namespace nlohmann {
template <> struct adl_serializer<Company> {
  static void to_json(json &j, const Company &c) {
    j["name"] = c.name;
    j["employees"] = c.employees;
    j["departments"] = c.departments;
    if (c.headquarters) {
      j["headquarters"] = *c.headquarters;
    } else {
      j["headquarters"] = nullptr;
    }
  }

  static void from_json(const json &j, Company &c) {
    j.at("name").get_to(c.name);
    j.at("employees").get_to(c.employees);
    j.at("departments").get_to(c.departments);
    if (!j["headquarters"].is_null()) {
      c.headquarters = j.at("headquarters").get<Address>();
    } else {
      c.headquarters = std::nullopt;
    }
  }
};
} // namespace nlohmann

// Create test data
Company create_test_company() {
  Company c;
  c.name = "Tech Corp";

  c.employees = {{"Alice", 30, "alice@tech.com"},
                 {"Bob", 25, "bob@tech.com"},
                 {"Charlie", 35, "charlie@tech.com"},
                 {"Diana", 28, "diana@tech.com"},
                 {"Eve", 32, "eve@tech.com"}};

  c.departments = {{"Engineering", {101, 102, 103, 104}},
                   {"Sales", {201, 202}},
                   {"HR", {301}}};

  c.headquarters = Address{"123 Tech St", "San Francisco", "USA", 94105};

  return c;
}

int main() {
  bench::print_header("Complex Struct Benchmark");
  std::cout << "Struct: Company { string, vector<Person>, "
               "map<string,vector<int>>, optional<Address> }\n";
  std::cout << "Iterations: 1,000\n\n";
  bench::print_table_header();

  Company original = create_test_company();
  constexpr int iterations = 1000;

  std::vector<bench::Result> results;

  // 1. beast_json
  {
    bench::Timer serialize_timer, deserialize_timer;
    std::string json_str;
    Company result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      json_str = beast::json::json_of(original);
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      beast::json::parse_into(result, json_str);
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    bool correct = (result == original);
    results.push_back({"beast_json", deserialize_ns, serialize_ns, correct});
  }

  // 2. nlohmann/json
  {
    bench::Timer serialize_timer, deserialize_timer;
    std::string json_str;
    Company result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      nlohmann::json j = original;
      json_str = j.dump();
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      result = nlohmann::json::parse(json_str).get<Company>();
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    bool correct = (result == original);
    results.push_back({"nlohmann/json", deserialize_ns, serialize_ns, correct});
  }

  // Note: RapidJSON and yyjson require significantly more manual coding for
  // complex structs We'll skip them for this benchmark to keep code
  // maintainable

  // Print results
  for (const auto &r : results) {
    r.print();
  }

  std::cout << "\n✓ Benchmark complete\n";
  std::cout << "Note: RapidJSON and yyjson omitted (too much manual "
               "serialization code)\n";
  return 0;
}
