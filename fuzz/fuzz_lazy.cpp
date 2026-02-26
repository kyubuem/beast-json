// fuzz_lazy.cpp – libFuzzer target for beast::json::lazy::parse_reuse().
//
// Exercises the zero-copy, tape-based lazy parser introduced in Phase 19+.
// A single static DocumentView is reused across invocations so that the
// hot-path (tape.reset()) and the cold-path (tape.reserve()) are both hit.
//
// Three passes per input:
//   1. parse_reuse()          – hot path (tape already allocated)
//   2. dump()                 – re-serialize from tape (covers Value traversal)
//   3. is_object() / is_array() – cover simple accessor paths
//
// Build / run: see fuzz_parse.cpp header comment; swap target name to fuzz_lazy.

#include <beast_json/beast_json.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace beast::json::lazy;

// One DocumentView per fuzzing process. The tape buffer grows to fit the
// largest input seen and is then reused (head = base) for every subsequent
// invocation — zero allocation overhead on the hot path.
// libFuzzer is single-threaded by default, so static storage is safe.
static DocumentView g_doc;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const std::string_view input(reinterpret_cast<const char *>(data), size);

    // ── 1. Parse ─────────────────────────────────────────────────────────────
    // parse_reuse() stores a string_view into `data`; all accesses to source
    // happen within this function call, so the pointer remains valid.
    try {
        Value root = parse_reuse(g_doc, input);

        // ── 2. Serialise (dump) ───────────────────────────────────────────────
        // Walks the tape and reconstructs JSON from raw source offsets.
        (void)root.dump();

        // ── 3. Type accessors ────────────────────────────────────────────────
        (void)root.is_object();
        (void)root.is_array();

    } catch (const std::runtime_error &) {
        // parse_reuse() throws std::runtime_error on invalid JSON – expected.
    }

    return 0;
}
