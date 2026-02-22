// fuzz_tape.cpp – libFuzzer target for the beast-json low-level tape::Parser.
//
// The tape parser is the performance-critical core of beast-json.  It exposes
// two separate code paths (bitmap-accelerated and standard) that are tested
// independently here.  Both share the same FastArena and Document to keep
// per-invocation overhead minimal.
//
// Build / run instructions are identical to fuzz_parse.cpp – just swap the
// target name:
//   cmake --build build-fuzz --target fuzz_tape
//   ./build-fuzz/fuzz/fuzz_tape fuzz/corpus/ -max_len=65536

#include <beast_json/beast_json.hpp>
#include <cstddef>
#include <cstdint>

using namespace beast::json;

// Allocate a shared arena once and reuse it across invocations to avoid the
// overhead of repeated heap allocation.  libFuzzer is single-threaded by
// default, so static storage is safe here.
static FastArena g_arena(4 * 1024 * 1024); // 4 MB
static tape::Document g_doc(&g_arena);

static void reset_doc() {
    g_arena.reset();
    g_doc.tape.clear();
    g_doc.string_buffer.clear();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    const char *input = reinterpret_cast<const char *>(data);

    // ── 1. Standard (non-bitmap) mode ────────────────────────────────────────
    reset_doc();
    {
        tape::ParseOptions opts;
        opts.use_bitmap = false;
        tape::Parser p(g_doc, input, size, opts);
        (void)p.parse();
    }

    // ── 2. Bitmap mode (SIMD two-stage path) ─────────────────────────────────
    reset_doc();
    {
        tape::ParseOptions opts;
        opts.use_bitmap = true;
        tape::Parser p(g_doc, input, size, opts);
        (void)p.parse();
    }

    // ── 3. Bitmap + trailing commas + comments ────────────────────────────────
    reset_doc();
    {
        tape::ParseOptions opts;
        opts.use_bitmap = true;
        opts.allow_trailing_commas = true;
        opts.allow_comments = true;
        tape::Parser p(g_doc, input, size, opts);
        (void)p.parse();
    }

    return 0;
}
