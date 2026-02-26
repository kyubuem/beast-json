// fuzz_parse.cpp – libFuzzer target for the beast::json high-level parse() API.
//
// Tests the public-facing parse() function with arbitrary byte sequences.
// Exercises strict (default) and relaxed ParseOptions.
// AddressSanitizer + UBSanitizer catch memory errors and undefined behaviour
// at compile time (injected by fuzz/CMakeLists.txt).
//
// Build:
//   cmake -B build-fuzz \
//         -DBEAST_JSON_BUILD_FUZZ=ON \
//         -DBEAST_JSON_BUILD_TESTS=OFF \
//         -DBEAST_JSON_BUILD_BENCHMARKS=OFF \
//         -DCMAKE_CXX_COMPILER=clang++
//   cmake --build build-fuzz --target fuzz_parse
//
// Run (indefinitely):
//   ./build-fuzz/fuzz/fuzz_parse fuzz/corpus/ -max_len=65536
//
// Reproduce a crash:
//   ./build-fuzz/fuzz/fuzz_parse <crash-file>

#include <beast_json/beast_json.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace beast::json;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const std::string_view input(reinterpret_cast<const char *>(data), size);

    // ── 1. Default options ───────────────────────────────────────────────────
    // parse() throws std::runtime_error on failure (ParseError ⊂ runtime_error)
    try {
        Value v = parse(input);
        (void)v;
    } catch (const std::runtime_error &) {
        // Expected for malformed input.
    }

    // ── 2. Relaxed: single-quotes + unquoted keys ────────────────────────────
    try {
        ParseOptions opts;
        opts.allow_single_quotes = true;
        opts.allow_unquoted_keys = true;
        Value v = parse(input, {}, opts);
        (void)v;
    } catch (const std::runtime_error &) {}

    // ── 3. Strict: duplicates / trailing-commas / comments all forbidden ─────
    try {
        ParseOptions opts;
        opts.allow_trailing_commas = false;
        opts.allow_comments       = false;
        opts.allow_duplicate_keys = false;
        Value v = parse(input, {}, opts);
        (void)v;
    } catch (const std::runtime_error &) {}

    return 0;
}
