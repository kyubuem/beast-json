# Beast JSON

**Beast JSON** is a high-performance, single-header C++20 JSON library built around a tape-based lazy DOM. Its design goal is simple: match or beat the world's fastest JSON libraries through aggressive low-level optimization — while remaining practical for real-world use.

> **An AI-Only Generated Library** — every line of code, every optimization, and every benchmark in this repository was designed and written by AI (Claude). It's an ongoing experiment in what AI can achieve in low-level, high-performance C++ systems programming.

---

## Benchmarks

### Linux x86-64

> **Environment**: Linux x86-64, GCC 13.3.0 `-O3 -flto -march=native` + PGO, 150 iterations per file, timings are per-run averages.
> Phase 44–53 applied (Action LUT · AVX-512 string gate · AVX-512 64B WS skip · SWAR-8 pre-gate · PGO · input prefetch · Stage 1+2 two-phase parsing · positions `:,` elimination).
> yyjson compiled with full SIMD enabled. All results verified correct (✓ PASS).

#### twitter.json — 616.7 KB · social graph, mixed types

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **202** | **3.06 GB/s** | **131** |
| yyjson | 248 | 2.49 GB/s | 139 |
| beast::rtsm | 295 | 2.09 GB/s | — |
| nlohmann | 4,415 | 140 MB/s | 1,339 |

> beast::lazy is **23% faster** than yyjson on parse. Two-phase AVX-512 Stage 1+2 parsing with positions array optimizations delivers parse throughput of **3.06 GB/s**.

#### canada.json — 2.2 MB · dense floating-point arrays

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **1,448** | **1.52 GB/s** | **844** |
| beast::rtsm | 1,978 | 1.11 GB/s | — |
| yyjson | 2,734 | 0.80 GB/s | 3,379 |
| nlohmann | 26,333 | 83 MB/s | 6,858 |

> beast::lazy is **89% faster** to parse and **4.0× faster** to serialize than yyjson. AVX-512 64B whitespace skip delivers massive gains on coordinate-heavy JSON.

#### citm_catalog.json — 1.7 MB · event catalog, string-heavy

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 736 | 2.29 GB/s | 257 |
| **beast::lazy** | **757** | **2.23 GB/s** | **347** |
| beast::rtsm | 1,174 | 1.44 GB/s | — |
| nlohmann | 9,353 | 180 MB/s | 1,313 |

> Beast is within **3% of yyjson** — essentially tied. Stage 1+2 two-phase parsing improved this from -32% to near-parity.

#### gsoc-2018.json — 3.2 MB · large object array

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **806** | **4.03 GB/s** | **514** |
| beast::rtsm | 1,018 | 3.19 GB/s | — |
| yyjson | 1,782 | 1.82 GB/s | 1,582 |
| nlohmann | 14,863 | 218 MB/s | 12,231 |

> beast::lazy is **121% faster** to parse and **3.1× faster** to serialize than yyjson. Parse throughput reaches **4.03 GB/s**.

#### Summary

| Benchmark | Beast vs yyjson (parse) | Beast vs yyjson (serialize) |
| :--- | :--- | :--- |
| twitter.json | **Beast 23% faster** ✅ | **Beast 6% faster** |
| canada.json | **Beast 89% faster** ✅ | **Beast 4.0× faster** |
| citm_catalog.json | yyjson 3% faster | yyjson 35% faster |
| gsoc-2018.json | **Beast 121% faster** ✅ | **Beast 3.1× faster** |

Beast **beats yyjson on parse speed for 3 out of 4 files** and is near-tied on the fourth (citm). The **twitter** result (202 μs vs 248 μs) is particularly notable — a file historically dominated by yyjson now falls to Beast by 23%.

#### 1.2× Goal Progress (beat yyjson by ≥20% on all 4 files)

| File | Target | Current | Status |
| :--- | ---: | ---: | :---: |
| twitter.json | ≤219 μs | **202 μs** | ✅ |
| canada.json | ≤2,274 μs | **1,448 μs** | ✅ |
| citm_catalog.json | ≤592 μs | 757 μs | ⬜ |
| gsoc-2018.json | ≤1,209 μs | **806 μs** | ✅ |

---

### macOS (Apple M1 Pro)

> **Environment**: macOS 26.3, Apple Clang 17 (`-O3 -flto`), Apple M1 Pro.
> Phase 31-57 applied (NEON string gate · Action LUT · SWAR float scanner · Pure NEON consolidation).
> All benchmarks: 300 iterations.
> All results verified correct (✓ PASS).

#### twitter.json — 616.7 KB · social graph, mixed types

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 176 | 3.50 GB/s | 101 |
| **beast::lazy** | **246** | **2.51 GB/s** | **116** |
| beast::rtsm | 275 | 2.24 GB/s | — |
| nlohmann | 3,567 | 172 MB/s | 1,353 |

#### canada.json — 2.2 MB · dense floating-point arrays

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 1,441 | 1.52 GB/s | 2,219 |
| **beast::lazy** | **1,845** | **1.19 GB/s** | **883** |
| beast::rtsm | 1,880 | 1.16 GB/s | — |
| nlohmann | 19,489 | 112 MB/s | 6,775 |

> beast::lazy serialize is **2.5× faster** than yyjson.

#### citm_catalog.json — 1.7 MB · event catalog, string-heavy

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| yyjson | 474 | 3.55 GB/s | 163 |
| **beast::lazy** | **627** | **2.69 GB/s** | **262** |
| beast::rtsm | 1,016 | 1.66 GB/s | — |
| nlohmann | 8,132 | 207 MB/s | 1,352 |

#### gsoc-2018.json — 3.2 MB · large object array

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **618** | **5.26 GB/s** | **285** |
| beast::rtsm | 726 | 4.47 GB/s | — |
| yyjson | 1,014 | 3.20 GB/s | 725 |
| nlohmann | 13,624 | 238 MB/s | 11,728 |

> beast::lazy is **63% faster** to parse and **2.5× faster** to serialize than yyjson.

#### Summary

| Benchmark | Beast vs yyjson (parse) | Beast vs yyjson (serialize) |
| :--- | :--- | :--- |
| twitter.json | yyjson 21% faster | yyjson 15% faster |
| canada.json | yyjson 21% faster | **Beast 2.5× faster** |
| citm_catalog.json | yyjson 24% faster | yyjson 60% faster |
| gsoc-2018.json | **Beast 63% faster** ✅ | **Beast 2.5× faster** |

Beast **dominates serialization** on gsoc and canada workloads. On gsoc-2018, beast beats yyjson on parse by **63%**.

> **Phase 57 Discovery**: "Pure NEON" (removing all scalar SWAR gates) is the optimal AArch64 strategy. GPR-SIMD mixing stalls the vector pipeline; a clean NEON-only path reduced `twitter.json` from 260 μs to **246 μs**.

---

### AArch64 Generic (Snapdragon 8 Gen 2 · Android Termux)

> **Environment**: Galaxy Z Fold 5, Android Termux, Clang 21.1.8 (`-O3 -march=armv8.4-a+crypto+dotprod+fp16+i8mm+bf16`), Snapdragon 8 Gen 2.
> CPU: 1×Cortex-X3 (3360 MHz) · 2×Cortex-A715 · 2×Cortex-A710 · 3×Cortex-A510.
> Phase 57+58-A applied (Pure NEON + prefetch distance tuning). Pinned to Cortex-X3 prime core (cpu7), 300 iterations.
> Note: SVE/SVE2 present in hardware but kernel-disabled on this Android build. `-march=armv8.4-a+...` flags required (no SVE).
> All results verified correct (✓ PASS).

#### twitter.json — 616.7 KB · social graph, mixed types

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **243** | **2.54 GB/s** | **171** |
| beast::rtsm | 329 | 1.87 GB/s | — |
| yyjson | 371 | 1.66 GB/s | 177 |
| nlohmann | 5,584 | 110 MB/s | 1,490 |

> beast::lazy is **53% faster** than yyjson.

#### canada.json — 2.2 MB · dense floating-point arrays

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **2,009** | **1.09 GB/s** | **797** |
| beast::rtsm | 2,416 | 0.91 GB/s | — |
| yyjson | 2,761 | 0.80 GB/s | 2,931 |
| nlohmann | 79,130 | 28 MB/s | 8,099 |

> beast::lazy is **37% faster** to parse and **3.7× faster** to serialize than yyjson.

#### citm_catalog.json — 1.7 MB · event catalog, string-heavy

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **639** | **2.64 GB/s** | **441** |
| beast::rtsm | 1,260 | 1.34 GB/s | — |
| yyjson | 973 | 1.73 GB/s | 257 |
| nlohmann | 11,645 | 145 MB/s | 1,559 |

> beast::lazy is **52% faster** to parse than yyjson.

#### gsoc-2018.json — 3.2 MB · large object array

| Library | Parse (μs) | Throughput | Serialize (μs) |
| :--- | ---: | :--- | ---: |
| **beast::lazy** | **659** | **4.93 GB/s** | **735** |
| beast::rtsm | 836 | 3.89 GB/s | — |
| yyjson | 1,742 | 1.87 GB/s | 1,540 |
| nlohmann | 20,574 | 158 MB/s | 12,289 |

> beast::lazy is **164% faster** to parse and **2.1× faster** to serialize than yyjson.

#### Summary

| Benchmark | Beast vs yyjson (parse) | Beast vs yyjson (serialize) |
| :--- | :--- | :--- |
| twitter.json | **Beast 53% faster** ✅ | yyjson 4% faster |
| canada.json | **Beast 37% faster** ✅ | **Beast 3.7× faster** |
| citm_catalog.json | **Beast 52% faster** ✅ | yyjson 42% faster |
| gsoc-2018.json | **Beast 164% faster** ✅ | **Beast 2.1× faster** |

Beast **sweeps all 4 parse benchmarks** on Snapdragon 8 Gen 2 / Cortex-X3 — the standard AArch64 proxy for AWS Graviton 3 and server-class ARM workloads. Optimizations proven on this core transfer directly to cloud AArch64 deployments.

#### 1.2× Goal Progress (beat yyjson by ≥20% on all 4 files)

| File | Target | Current | Status |
| :--- | ---: | ---: | :---: |
| twitter.json | ≤309 μs | **243 μs** | ✅ |
| canada.json | ≤2,301 μs | **2,009 μs** | ✅ |
| citm_catalog.json | ≤811 μs | **639 μs** | ✅ |
| gsoc-2018.json | ≤1,452 μs | **659 μs** | ✅ |

> **Phase 57+58-A note**: Pure NEON strategy (Phase 57) + prefetch distance 192B→256B tuning (Phase 58-A) combined. Cortex-X3 at 3360 MHz delivers **33 cy/tok** — essentially matching M1 Pro for Beast's workloads (twitter: 243 μs vs 246 μs). yyjson costs **50 cy/tok** on Cortex-X3 vs 23 cy/tok on M1 Pro, confirming yyjson's dependency on M1's 576-entry reorder buffer.


---

## How It Works: Under the Hood

Beast JSON is an ongoing laboratory for JSON parsing techniques. Here's what's inside.

### 🧠 Tape-Based Lazy DOM Architecture

Instead of allocating a massive tree of scattered heap nodes (like traditional parsers), Beast writes a **flat, contiguous `TapeNode` array** inside a single pre-allocated memory arena. 

The most extreme optimization we achieved is compressing the entire contextual state of a JSON element into exactly **8 bytes** per node. This drastically reduces the working set size, perfectly aligning with modern CPU L2/L3 cache architectures.

#### The 8-Byte `TapeNode` Layout

```text
 31      24 23     16 15            0
 ┌────────┬─────────┬───────────────┐
 │  type  │   sep   │    length     │  meta (32-bit)
 └────────┴─────────┴───────────────┘
 ┌────────────────────────────────────┐
 │            byte offset             │  offset (32-bit)
 └────────────────────────────────────┘
```

| Field | Specs | Purpose | Performance Impact |
| :--- | :--- | :--- | :--- |
| **`offset`** | 32 bits | Raw byte offset into the original source buffer. | Enables **Zero-Copy** strings. We point directly to the source string instead of allocating memory for it. |
| **`length`** | 16 bits | Byte length of the token string. | O(1) string length lookups. |
| **`sep`** | 8 bits | Pre-computed separator bit (`0`=none, `1`=comma, `2`=colon). | Eliminates the entire separator state-machine overhead during serialization. |
| **`type`** | 8 bits | The node enum (ObjectStart, ArrayEnd, Number, String...). | Single-byte type checking. |

All string data stays in the original input buffer — the library is **100% zero-copy**. Serialization (`dump()`) is a single linear memory scan over the contiguous tape with direct pointer writes into a pre-sized output buffer, bypassing traditional tree-traversal overhead completely.

### Phase D1 — TapeNode Compaction (12 → 8 bytes)

The original `TapeNode` was 12 bytes with separate `type`, `length`, `offset`, and `next_sib` fields. `next_sib` existed to track sibling pointers — but analysis showed it was written and never actually read at runtime. After removing it and packing `type + sep + length` into a single `uint32_t meta`, the struct shrank from 12 → 8 bytes.

**Effect**: Each tape node now fits in 8 bytes instead of 12. For canada.json's 2.32 million float tokens, that's **~9.3 MB less working set** — a major L2/L3 cache improvement, yielding +7.6% parse throughput on canada.json.

### ⚡ SWAR String Scanning — SIMD Without SIMD

String parsing is the bottleneck in every JSON parser. Beast leverages **SWAR (SIMD Within A Register)** — 64-bit bitwise magic that processes 8 bytes at a time for `"` and `\` characters, using zero SIMD intrinsics. This allows Beast to fly even on older architectures.

> [!TIP]
> **The Math Behind The Magic**: `has_byte(v, c)` uses a classic trick: `(v - 0x0101...01 * c) & ~v & 0x8080...80`. It sets the highest bit in each byte position where the target character `c` appears!

```cpp
// Load 8 bytes into a 64-bit GPR
uint64_t v = load_u64(p);

// Broadcast compare: instantaneously find any quotes or backslashes
uint64_t q = has_byte(v, '"');
uint64_t b = has_byte(v, '\\');

if (q && (q < b || !b)) {
    // Quote found BEFORE any escape character!
    // Shift the pointer directly to the end of the string.
    p += ctz(q) / 8;
    goto string_done;
}
```

By adding a **cascaded early exit**, roughly ~36% of strings (like `"id"`, `"text"`) exit within the very first 8-byte chunk, avoiding further looping entirely.

### 🏗️ Pre-flagged Separators (The Ultimate Zero-Cost Abstraction)

This is the single most impactful optimization in the library: **completely eliminating the separator state machine from serialization (`dump()`)**.

Traditional JSON serializers waste cycles tracking state at runtime: *“Are we inside an object? Are we on a key or a value? Is this the first element?”* Every token requires 3–5 bit-stack operations just to decide whether to print a `,` or `:`. 

Beast eliminates this by pre-computing the separator **during parsing** and baking it directly into the `meta` descriptor.

> [!IMPORTANT]
> Because Beast calculates the bit-stacks (using precomputed `depth_mask_`) iteratively during the single pass, the cost is effectively hidden in instruction-level parallelism.

```cpp
/* --- 1. DURING PARSE --- */
const bool in_obj = !!(obj_bits_ & depth_mask_);
const bool is_key = !!(kv_key_bits_ & depth_mask_);
const bool is_val = in_obj & !is_key;

// 2=colon, 1=comma, 0=none (stored right into the TapeNode!)
uint8_t sep = is_val ? 2 : uint8_t(has_elem);

/* --- 2. DURING SERIALIZATION --- */
// One branch replaces 50 lines of complex state-tracking code!
const uint8_t sep = (meta >> 16) & 0xFF;
if (sep) *w++ = (sep == 2) ? ':' : ',';
```

**The Result:** The serialize inner loop becomes a devastatingly tight memory scan. No recursive calls, no state stacks. This delivers a **~29% serialize time reduction** on heavy datasets.

### Phase D4 — Single Meta Read per Iteration

In the `dump()` hot loop, `nd.meta` was accessed multiple times per token — once to get `type`, again for `length`, and again for `sep`. A single cache line miss could stall all three reads.

Fix: read `nd.meta` into a local `const uint32_t meta` once, then derive everything from it with cheap shifts and masks. One memory read replaces three.

**Effect**: twitter serialize -11% on its own; enabled further cleanup in Phase E.

### Phase 50+53 — Two-Phase AVX-512 Parsing (simdjson-style)

The biggest parse-speed breakthrough: a simdjson-inspired two-phase parsing pipeline, customized for Beast's tape architecture.

**Stage 1** (`stage1_scan_avx512`): a single AVX-512 pass over the entire input at 64 bytes/iteration. It uses `_mm512_cmpeq_epi8_mask` to detect quotes, backslashes, and structural characters (`{}[]`), and tracks in-string state via a cross-block XOR prefix-sum (`prefix_xor`). The result is a flat `uint32_t[]` positions array — one entry per structural character, quote, or value-start byte.

**Stage 2** (`parse_staged`): iterates the positions array without touching the raw input for whitespace or string-length scanning. String length becomes `O(1)`: `close_offset − open_offset − 1`. The push() bit-stacks handle key↔value alternation exactly as in single-pass mode — no separator entries needed.

**Phase 53 key insight**: `,` and `:` entries were removed from the positions array entirely. The push() bit-stack already knows whether the current token is a key or value; it doesn't need explicit separator positions. Removing them shrinks the positions array by ~33% (from ~150K to ~100K entries for twitter.json), reducing L2/L3 cache pressure and Stage 2 iteration count.

**Result**: twitter.json 365 μs → **202 μs** (−44.7% vs Phase 48 baseline) with PGO.

A 2 MB size threshold selects the path: files ≤2 MB (twitter, citm) use Stage 1+2; larger number-heavy files (canada, gsoc) fall back to the optimized single-pass parser, where the positions array would exceed L3 capacity.

> **Note on NEON/ARM64**: We implemented an equivalent `stage1_scan_neon` processing 64 bytes per iteration (unrolling 4 × 16-byte `vld1q_u8`). However, benchmark results showed a **~30-45% performance degradation** compared to the single-pass parser. Generating the 64-bit structural bitmasks requires too many shift-and-OR operations (`neon_movemask` per 16B chunk), creating higher overhead than simply scanning line-by-line. AArch64 benefits far more from our highly optimized single-pass linear scans than a two-phase architecture.

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
