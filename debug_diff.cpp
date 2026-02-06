#include <beast_json/beast_json.hpp>
#include <fstream>
#include <iostream>

int main() {
  // Read twitter.json
  std::ifstream file("twitter.json");
  std::string json_content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

  std::cout << "Original size: " << json_content.size() << " bytes\n";

  // Parse
  beast::json::FastArena arena(json_content.size() * 2);
  beast::json::tape::Document doc(&arena);
  beast::json::tape::Parser parser(doc, json_content.data(),
                                   json_content.size());
  parser.parse();

  // Serialize
  std::string output;
  output.reserve(json_content.size());
  beast::json::tape::TapeSerializerExtreme serializer(doc, output);
  serializer.serialize();

  std::cout << "Serialized size: " << output.size() << " bytes\n";
  std::cout << "Match: " << (output == json_content ? "YES" : "NO") << "\n\n";

  if (output != json_content) {
    // Find first difference
    size_t min_len = std::min(output.size(), json_content.size());
    for (size_t i = 0; i < min_len; i++) {
      if (output[i] != json_content[i]) {
        std::cout << "First difference at position " << i << ":\n";

        // Show context (50 chars before and after)
        size_t start = (i > 50) ? i - 50 : 0;
        size_t end = std::min(i + 50, min_len);

        std::cout << "Original: ...";
        for (size_t j = start; j < end; j++) {
          char c = json_content[j];
          if (c == '\n')
            std::cout << "\\n";
          else if (c == '\t')
            std::cout << "\\t";
          else if (c == ' ')
            std::cout << "·";
          else
            std::cout << c;
        }
        std::cout << "...\n";

        std::cout << "Output:   ...";
        for (size_t j = start; j < end; j++) {
          char c = output[j];
          if (c == '\n')
            std::cout << "\\n";
          else if (c == '\t')
            std::cout << "\\t";
          else if (c == ' ')
            std::cout << "·";
          else
            std::cout << c;
        }
        std::cout << "...\n";

        std::cout << "\nOriginal char: '" << json_content[i] << "' (0x"
                  << std::hex << (int)(unsigned char)json_content[i] << ")\n";
        std::cout << "Output char:   '" << output[i] << "' (0x" << std::hex
                  << (int)(unsigned char)output[i] << ")\n";
        break;
      }
    }

    if (output.size() != json_content.size()) {
      std::cout << "\nSize mismatch: " << output.size() << " vs "
                << json_content.size() << "\n";
    }
  }

  return 0;
}
