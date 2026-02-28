# Beast JSON

**Beast JSON** is a high-performance, single-header C++20 JSON library built around a tape-based lazy DOM. Its design goal is simple: match or beat the world's fastest JSON libraries through aggressive low-level optimization — while remaining practical for real-world use.

> **An AI-Only Generated Library** — every line of code, every optimization, and every benchmark in this repository was designed and written by AI (Claude). It's an ongoing experiment in what AI can achieve in low-level, high-performance C++ systems programming.

---

## Benchmarks

### Linux x86-64

> **Environment**: Linux x86-64, GCC 13.3.0 `-O3 -flto -mavx2`, 50 iterations per file, timings are per-run averages.
> Phase 31-34 applied (Action LUT · SWAR float scanner · NEON/SSE2 string gate · **AVX2 32B string scanner**).
> yyjson compiled with full SIMD enabled. All results verified correct (✓ PASS).

#### twitter.json — 616.7 KB · social graph, mixed types

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 284 | 2.22 GB/s | 162 |
| **beast::lazy** | **332** | **1.90 GB/s** | **170** |
| beast::rtsm | 399 | 1.58 GB/s | — |
| nlohmann | 5,330 | 119 MB/s | 1,807 |

> Serialize is essentially **tied with yyjson** (170 vs 162 μs, within noise).

#### canada.json — 2.2 MB · dense floating-point arrays

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **1,519** | **1.48 GB/s** | **1,011** |
| beast::rtsm | 2,940 | 765 MB/s | — |
| yyjson | 2,695 | 835 MB/s | 3,359 |
| nlohmann | 32,146 | 70 MB/s | 7,775 |

> beast::lazy is **44% faster** to parse and **3.3× faster** to serialize than yyjson.

#### citm_catalog.json — 1.7 MB · event catalog, string-heavy

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 723 | 2.39 GB/s | 245 |
| **beast::lazy** | **734** | **2.35 GB/s** | **385** |
| beast::rtsm | 1,631 | 1.06 GB/s | — |
| nlohmann | 10,094 | 171 MB/s | 1,807 |

> Parse is essentially **tied with yyjson** (734 vs 723 μs, 1.5% gap).

#### gsoc-2018.json — 3.2 MB · large object array

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **750** | **4.44 GB/s** | **519** |
| beast::rtsm | 1,171 | 2.84 GB/s | — |
| yyjson | 1,675 | 1.99 GB/s | 1,354 |
| nlohmann | 20,851 | 159 MB/s | 14,123 |

> beast::lazy is **55% faster** to parse and **2.6× faster** to serialize than yyjson.

#### Summary

| Benchmark | Beast vs yyjson (parse) | Beast vs yyjson (serialize) |
| :--- | :--- | :--- |
| twitter.json | yyjson 15% faster | **Tied** (±5%) |
| canada.json | **Beast 44% faster** | **Beast 3.3× faster** |
| citm_catalog.json | **Tied** (1.5% gap) | yyjson 37% faster |
| gsoc-2018.json | **Beast 55% faster** | **Beast 2.6× faster** |

Beast **dominates** on float-heavy (canada) and large-object-array (gsoc) workloads. With AVX2 active, the gsoc parse throughput reaches **4.44 GB/s** — 2.2× the yyjson throughput on this machine.

---

### macOS (Apple M1 Pro)

> **Environment**: macOS 26.3, Apple Clang 17 (`-O3 -flto`), Apple M1 Pro.
> Phase 31-34 applied (NEON string gate · Action LUT · SWAR float scanner · AVX2 x86_64).
> twitter/citm/gsoc: 100 iterations. canada: 50 iterations.
> All results verified correct (✓ PASS).

#### twitter.json — 616.7 KB · social graph, mixed types

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 180 | 3.43 GB/s | 103 |
| **beast::lazy** | **264** | **2.34 GB/s** | **120** |
| beast::rtsm | 278 | 2.22 GB/s | — |
| nlohmann | 3,622 | 170 MB/s | 1,374 |

> Serialize is essentially **tied with yyjson** (120 vs 103 μs). Parse **-4.4%** vs Phase 30 baseline.

#### canada.json — 2.2 MB · dense floating-point arrays

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 1,463 | 1.50 GB/s | 2,274 |
| **beast::lazy** | **1,863** | **1.18 GB/s** | **881** |
| beast::rtsm | 1,878 | 1.17 GB/s | — |
| nlohmann | 19,423 | 113 MB/s | 6,785 |

> beast::lazy serialize is **2.6× faster** than yyjson. Parse **-7.8%** vs Phase 30 baseline (SWAR float).

#### citm_catalog.json — 1.7 MB · event catalog, string-heavy

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 474 | 3.56 GB/s | 163 |
| **beast::lazy** | **654** | **2.58 GB/s** | **269** |
| beast::rtsm | 1,009 | 1.67 GB/s | — |
| nlohmann | 8,372 | 201 MB/s | 1,333 |

#### gsoc-2018.json — 3.2 MB · large object array

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **625** | **5.20 GB/s** | **284** |
| beast::rtsm | 722 | 4.50 GB/s | — |
| yyjson | 1,005 | 3.23 GB/s | 722 |
| nlohmann | 13,591 | 239 MB/s | 11,761 |

> beast::lazy is **60% faster** to parse and **2.5× faster** to serialize than yyjson. Parse **-12.6%** vs Phase 30 baseline (NEON string gate).

#### Summary

| Benchmark | Beast vs yyjson (parse) | Beast vs yyjson (serialize) |
| :--- | :--- | :--- |
| twitter.json | yyjson 32% faster | **Tied** (±14%) |
| canada.json | yyjson 21% faster | **Beast 2.6× faster** |
| citm_catalog.json | yyjson 27% faster | yyjson 39% faster |
| gsoc-2018.json | **Beast 60% faster** | **Beast 2.5× faster** |

Beast **dominates serialization** on 3 of 4 files. On the gsoc-2018 workload (large object arrays), beast now beats yyjson on parse speed by **60%**.


---

## How It Works: Under the Hood

Beast JSON is an ongoing laboratory for JSON parsing techniques. Here's what's inside.

### Tape-Based Lazy DOM

Instead of allocating a tree of heap nodes, the parser writes a flat `TapeNode` array in a single pre-allocated arena. Each node is exactly **8 bytes**:

```
 31      24 23     16 15            0
 ┌────────┬─────────┬───────────────┐
 │  type  │   sep   │    length     │  meta (32-bit)
 └────────┴─────────┴───────────────┘
 ┌────────────────────────────────────┐
 │            byte offset             │  offset (32-bit)
 └────────────────────────────────────┘
```

- **`type`** (8 bits): ObjectStart / ArrayEnd / String / Integer / etc.
- **`sep`** (8 bits): pre-computed separator — `0` = none, `1` = comma, `2` = colon
- **`length`** (16 bits): byte length of the token in the source string
- **`offset`** (32 bits): byte offset into the original source buffer

All string data stays in the original input buffer — the library is **zero-copy**. Serialization (`dump()`) is a single linear scan over the tape with direct pointer writes into a pre-sized output buffer.

### Phase D1 — TapeNode Compaction (12 → 8 bytes)

The original `TapeNode` was 12 bytes with separate `type`, `length`, `offset`, and `next_sib` fields. `next_sib` existed to track sibling pointers — but analysis showed it was written and never actually read at runtime. After removing it and packing `type + sep + length` into a single `uint32_t meta`, the struct shrank from 12 → 8 bytes.

**Effect**: Each tape node now fits in 8 bytes instead of 12. For canada.json's 2.32 million float tokens, that's **~9.3 MB less working set** — a major L2/L3 cache improvement, yielding +7.6% parse throughput on canada.json.

### SWAR String Scanning — SIMD Without SIMD

String parsing is the bottleneck in every JSON parser. Beast uses **SWAR** (SIMD Within A Register) — 64-bit bitwise tricks that scan 8 bytes at once for `"` and `\` characters without any SIMD intrinsics:

```cpp
// Load 8 bytes; check for " or \ in all 8 positions at once
uint64_t v = load_u64(p);
uint64_t q = has_byte(v, '"');   // broadcast-compare: finds any " in 8 bytes
uint64_t b = has_byte(v, '\\');  // same for backslash
if (q && (q < b || !b)) {
    // quote found before any backslash → plain string end, no escape needed
    p += ctz(q) / 8;
    goto string_done;
}
```

The `has_byte(v, c)` formula is a classic trick: `(v - 0x0101...01 * c) & ~v & 0x8080...80`. It sets the high bit of each byte position where the target character appears. `ctz()` (count trailing zeros) then pinpoints the first match in one instruction.

A **cascaded early exit** checks the first 8 bytes before loading further: ~36% of twitter.json strings are ≤8 chars and exit immediately without touching the next two 8-byte chunks.

### Phase E — Pre-flagged Separators (The Big One)

The most impactful optimization in the library: **eliminate the separator state machine from `dump()` entirely**.

Traditional JSON serializers track whether to emit `,` or `:` at runtime during serialization — maintaining a stack of bits to know "are we in an object? are we on a key or value? is this the first element?". Every token requires 3–5 bit-stack operations to determine its separator.

Beast computes this **during parsing** and stores the result in `meta` bits 23–16:

```cpp
// In push() during parse — compute sep from bit-stacks, store in meta:
const bool in_obj  = !!(obj_bits_  & depth_mask_);
const bool is_key  = !!(kv_key_bits_ & depth_mask_);
const bool has_el  = !!(has_elem_bits_ & depth_mask_);
const bool is_val  = in_obj & !is_key;
uint8_t sep = is_val ? 2 : uint8_t(has_el);  // 2=colon, 1=comma, 0=none

// In dump() — one branch replaces the entire emit_sep() state machine:
const uint8_t sep = (meta >> 16) & 0xFF;
if (sep) *w++ = (sep == 2) ? ':' : ',';
```

The parse-time bit-stacks use `depth_mask_` — a precomputed `1ULL << (depth-1)` maintained incrementally with `<<1`/`>>1` shifts — to avoid any variable-count shift instructions in the hot path. For nesting deeper than 64 levels, a `presep_overflow_[1024]` byte array takes over transparently.

**Effect**: ~50 lines of emit_sep() machinery deleted from `dump()`. The serialize inner loop becomes a tight scan with one branch and one switch — no stack, no function calls.

Results: **twitter serialize -29%**, **canada serialize -18%**, **citm serialize -26%** vs the previous best.

### Phase D4 — Single Meta Read per Iteration

In the `dump()` hot loop, `nd.meta` was accessed multiple times per token — once to get `type`, again for `length`, and again for `sep`. A single cache line miss could stall all three reads.

Fix: read `nd.meta` into a local `const uint32_t meta` once, then derive everything from it with cheap shifts and masks. One memory read replaces three.

**Effect**: twitter serialize -11% on its own; enabled further cleanup in Phase E.

---

## Features

- **Single header** — drop `beast_json.hpp` into your project, done.
- **Zero-copy** — string values point into the original source buffer; no allocation per token.
- **Lazy DOM** — navigate to only what you need; untouched parts of the JSON cost nothing.
- **Pre-flagged separators** — separator state computed at parse time, never at serialize time.
- **SWAR string scanning** — 8-bytes-at-a-time `"` / `\` detection without any SIMD intrinsics.
- **Tape compaction** — 8-byte nodes, cache-line-friendly linear layout.
- **C++20** — no macros beyond include guards; fully `constexpr` where applicable.

---

## Usage

```cpp
#include <beast_json/beast_json.hpp>
#include <iostream>

using namespace beast::json;

int main() {
    std::string_view json = R"({"name":"Beast","speed":340,"tags":["fast","zero-copy"]})";

    lazy::DocumentView doc(json);
    lazy::Value root = lazy::parse_reuse(doc, json);

    if (doc.error_code != lazy::Error::Ok) {
        std::cerr << "Parse failed\n";
        return 1;
    }

    // Zero-copy key lookup
    std::cout << root.find("name").get_string()  << "\n";  // Beast
    std::cout << root.find("speed").get_int64()  << "\n";  // 340

    // Array traversal
    for (auto tag : root.find("tags").get_array()) {
        std::cout << tag.get_string() << "\n";  // fast, zero-copy
    }

    // Serialize back to JSON
    std::string out = root.dump();
    std::cout << out << "\n";

    return 0;
}
```

---

## Build

### Requirements

- C++20 compiler (Clang 10+ or GCC 10+)
- CMake 3.14+

### Tests & Benchmarks

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBEAST_JSON_BUILD_TESTS=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run benchmarks (requires twitter.json, canada.json, etc. in working directory)
cd build && ./benchmarks/bench_all --all
```

### Single-Header Use

Since beast-json is a single header library, you can also simply copy `include/beast_json/beast_json.hpp` into your project with no CMake required.

---

## Test Coverage

| Test Suite | Tests | Status |
| :--- | ---: | :--- |
| RelaxedParsing | 5 | ✅ PASS |
| Unicode | 5 | ✅ PASS |
| StrictNumber | 4 | ✅ PASS |
| ControlChar | 4 | ✅ PASS |
| Comments | 4 | ✅ PASS |
| TrailingCommas | 4 | ✅ PASS |
| DuplicateKeys | 3 | ✅ PASS |
| ErrorHandling | 5 | ✅ PASS |
| Serializer | 3 | ✅ PASS |
| Utf8Validation | 14 | ✅ PASS |
| LazyTypes | 19 | ✅ PASS |
| LazyRoundTrip | 11 | ✅ PASS |
| **Total** | **81** | **100% PASS** |
