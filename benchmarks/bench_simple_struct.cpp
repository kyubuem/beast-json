#include "utils.hpp"
#include <beast_json/beast_json.hpp>
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <yyjson.h>

// Simple struct
struct Person {
  std::string name;
  int age;
  std::string email;

  bool operator==(const Person &other) const {
    return name == other.name && age == other.age && email == other.email;
  }
};

// beast_json reflection
BEAST_DEFINE_JSON(Person, name, age, email);

// nlohmann/json serialization
namespace nlohmann {
template <> struct adl_serializer<Person> {
  static void to_json(json &j, const Person &p) {
    j = json{{"name", p.name}, {"age", p.age}, {"email", p.email}};
  }

  static void from_json(const json &j, Person &p) {
    j.at("name").get_to(p.name);
    j.at("age").get_to(p.age);
    j.at("email").get_to(p.email);
  }
};
} // namespace nlohmann

int main() {
  bench::print_header("Simple Struct Benchmark");
  std::cout << "Struct: Person { string name, int age, string email }\n";
  std::cout << "Iterations: 10,000\n\n";
  bench::print_table_header();

  Person original{"Alice Smith", 30, "alice@example.com"};
  constexpr int iterations = 10000;

  std::vector<bench::Result> results;

  // 1. beast_json
  {
    bench::Timer serialize_timer, deserialize_timer;
    std::string json_str;
    Person result;

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
    Person result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      nlohmann::json j = original;
      json_str = j.dump();
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      result = nlohmann::json::parse(json_str).get<Person>();
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    bool correct = (result == original);
    results.push_back({"nlohmann/json", deserialize_ns, serialize_ns, correct});
  }

  // 3. RapidJSON (manual)
  {
    bench::Timer serialize_timer, deserialize_timer;
    std::string json_str;
    Person result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      rapidjson::Document doc;
      doc.SetObject();
      auto &allocator = doc.GetAllocator();

      rapidjson::Value name_val;
      name_val.SetString(original.name.c_str(), original.name.size(),
                         allocator);
      doc.AddMember("name", name_val, allocator);
      doc.AddMember("age", original.age, allocator);

      rapidjson::Value email_val;
      email_val.SetString(original.email.c_str(), original.email.size(),
                          allocator);
      doc.AddMember("email", email_val, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      json_str = buffer.GetString();
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      rapidjson::Document doc;
      doc.Parse(json_str.c_str());
      result.name = doc["name"].GetString();
      result.age = doc["age"].GetInt();
      result.email = doc["email"].GetString();
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    bool correct = (result == original);
    results.push_back({"RapidJSON", deserialize_ns, serialize_ns, correct});
  }

  // 4. yyjson
  {
    bench::Timer serialize_timer, deserialize_timer;
    char *json_str = nullptr;
    size_t json_len = 0;
    Person result;

    serialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
      yyjson_mut_val *root = yyjson_mut_obj(doc);
      yyjson_mut_doc_set_root(doc, root);

      yyjson_mut_obj_add_strcpy(doc, root, "name", original.name.c_str());
      yyjson_mut_obj_add_int(doc, root, "age", original.age);
      yyjson_mut_obj_add_strcpy(doc, root, "email", original.email.c_str());

      if (json_str)
        free(json_str);
      json_str = yyjson_mut_write(doc, 0, &json_len);
      yyjson_mut_doc_free(doc);
    }
    double serialize_ns = serialize_timer.elapsed_ns() / iterations;

    deserialize_timer.start();
    for (int i = 0; i < iterations; ++i) {
      yyjson_doc *doc = yyjson_read(json_str, json_len, 0);
      yyjson_val *root = yyjson_doc_get_root(doc);

      result.name = yyjson_get_str(yyjson_obj_get(root, "name"));
      result.age = yyjson_get_int(yyjson_obj_get(root, "age"));
      result.email = yyjson_get_str(yyjson_obj_get(root, "email"));

      yyjson_doc_free(doc);
    }
    double deserialize_ns = deserialize_timer.elapsed_ns() / iterations;

    if (json_str)
      free(json_str);

    bool correct = (result == original);
    results.push_back({"yyjson", deserialize_ns, serialize_ns, correct});
  }

  // Print results
  for (const auto &r : results) {
    r.print();
  }

  std::cout << "\n✓ Benchmark complete\n";
  return 0;
}
