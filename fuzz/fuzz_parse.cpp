// fuzz_parse.cpp – libFuzzer target for the beast-json high-level parse() API.
//
// Tests the public-facing parse() function with arbitrary byte sequences.
// Exercises strict (default) and relaxed (lenient) ParseOptions.
// Memory errors and UB are caught by the AddressSanitizer / UBSanitizer
// instrumentation baked in at compile time.
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
// Run with a single file (for reproduction):
//   ./build-fuzz/fuzz/fuzz_parse <crash-input-file>

#include <beast_json/beast_json.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>

using namespace beast::json;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const std::string_view input(reinterpret_cast<const char *>(data), size);

    // ── 1. Strict / default mode ─────────────────────────────────────────────
    try {
        Value v = parse(input);
        (void)v;
    } catch (const ParseError &) {
        // Expected for malformed input – not a bug.
    }

    // ── 2. Relaxed mode (single-quotes, unquoted keys) ───────────────────────
    try {
        ParseOptions opts;
        opts.allow_single_quotes = true;
        opts.allow_unquoted_keys = true;
        Value v = parse(input, {}, opts);
        (void)v;
    } catch (const ParseError &) {
        // Expected.
    }

    // ── 3. Comments + trailing commas enabled ────────────────────────────────
    try {
        ParseOptions opts;
        opts.allow_comments = true;
        opts.allow_trailing_commas = true;
        Value v = parse(input, {}, opts);
        (void)v;
    } catch (const ParseError &) {
        // Expected.
    }

    return 0;
}
