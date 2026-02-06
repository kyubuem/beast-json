#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <glaze/glaze.hpp>
#include <map>
#include <optional>
#include <vector>

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

// beast_json reflection (C++17)
BEAST_DEFINE_JSON(Address, street, city, country, zip_code);
BEAST_DEFINE_JSON(Person, name, age, email);
BEAST_DEFINE_JSON(Company, name, employees, departments, headquarters);

// Glaze reflection (C++23)
template <> struct glz::meta<Address> {
  using T = Address;
  static constexpr auto value =
      object("street", &T::street, "city", &T::city, "country", &T::country,
             "zip_code", &T::zip_code);
};

template <> struct glz::meta<Person> {
  using T = Person;
  static constexpr auto value =
      object("name", &T::name, "age", &T::age, "email", &T::email);
};

template <> struct glz::meta<Company> {
  using T = Company;
  static constexpr auto value =
      object("name", &T::name, "employees", &T::employees, "departments",
             &T::departments, "headquarters", &T::headquarters);
};

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
  bench::print_header("Glaze (C++23) vs beast_json Benchmark");
  std::cout << "Struct: Company { string, vector<Person>, "
               "map<string,vector<int>>, optional<Address> }\n";
  std::cout << "Iterations: 1,000\n\n";
  bench::print_table_header();

  Company original = create_test_company();
  constexpr int iterations = 1000;

  std::vector<bench::Result> results;

  // 1. beast_json (C++17)
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
    results.push_back(
        {"beast_json (C++17)", deserialize_ns, serialize_ns, correct});
  }

  // 2. Glaze (C++23)
  {
    bench::Timer serialize_timer, deserialize_timer;
    std::string json_str;
    Company result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      auto ec = glz::write_json(original, json_str);
      if (ec) {
        std::cerr << "Glaze write error\n";
        break;
      }
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      auto ec = glz::read_json(result, json_str);
      if (ec) {
        std::cerr << "Glaze read error\n";
        break;
      }
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    bool correct = (result == original);
    results.push_back({"Glaze (C++23)", deserialize_ns, serialize_ns, correct});
  }

  // Print results
  for (const auto &r : results) {
    r.print();
  }

  std::cout << "\n✓ Benchmark complete\n";
  return 0;
}
