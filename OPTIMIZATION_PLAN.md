# Beast JSON Optimization Master Plan

**Goal**: Establish `beast-json` as the undisputed #1 high-performance JSON library.
**Target Performance**: > 5 GB/s Parsing, > 5 GB/s Serialization (ARM64/x64).
**Standard**: C++20 (GCC 10+, Clang 10+, MSVC 2019+).

## Part 1: Research Summary & Techniques
*Incorporated from `simdjson` and academic research.*

### 1. Structural Indexing (Two-Stage Parsing)
**Concept**: "Simpson's Split". Separate the parsing into two pipelined stages to massive reduce branch mispredictions.
- **Stage 1 (Structural Classifier)**: Use SIMD (NEON/AVX2) to scan 32/64 bytes at once. Create a bitmap index of structural characters (`{`, `}`, `[`, `]`, `:`, `,`) and strings.
- **Stage 2 (Tape Generation)**: Walk the bitmap to build the Tape. Logic becomes linear and validation is front-loaded.

### 2. On-Demand & Filter Push-Down
- Borrow ideas from "Mison" to support partial parsing where users only extract specific fields, using the structural index to skip entire sub-trees efficiently.

### 3. SWAR (SIMD Within A Register)
- For short strings or fallback paths, use 64-bit integer bit-manipulation (`(x - 0x01) & ~x ...`) to find quotes or delimiters without full SIMD overhead.

---

## Part 2: Phased Implementation Roadmap

### Phase 1: Foundation, Regression Fix & Baseline
**Objective**: Stabilize current base, fix regressions, and prepare C++20 environment.
- [ ] **Fix Parsing Regression**: Restore performance to > 1.2 GB/s. Profile `parse_number`.
- [ ] **Int64/Uint64 Support**: Native storage on Tape to prevent double precision loss.
- [ ] **Automated Benchmark Harness**: Standardize comparison against `glaze`, `yyjson`.
- [ ] **C++20 Migration**: Update build system. Use `[[likely]]` and Concepts.

### Phase 2: Micro-Optimizations (The "Beast" Core)
**Objective**: Implement the "Structural Indexing" engine.
- [ ] **Stage 1 Implementation**: Implement SIMD Structural Identifier (NEON for ARM64 first).
- [ ] **Stage 2 Implementation**: Refactor Tape builder to use the Structural Bitmap.
- [ ] **String Parsing**: Implement NEON/AVX `vld1q_u8` scan-then-copy for strings.
- [ ] **Table-Driven State Machine**: Use `constexpr` generated lookup tables.

### Phase 3: Memory & Layout Architecture
**Objective**: Cache locality and allocation speed.
- [ ] **Tape Alignment**: Force 64-bit alignment for all Tape nodes.
- [ ] **Tape Compression**: Use 32-bit offsets/indices where applicable to pack more nodes into L1 cache.
- [ ] **FastArena Tuning**: `__builtin_prefetch` and inline allocation paths.

### Phase 4: Parallel Processing
**Objective**: > 10 GB/s on massive files.
- [ ] **Chunking**: Heuristic scanning to find safe split points (e.g. top-level commas).
- [ ] **Parallel Merge**: Parse chunks independently and zero-copy merge Tapes.

### Phase 5: Verification & Compliance
- [ ] **Fuzz Testing**: LLVM `libFuzzer` 24h runs.
- [ ] **Compliance**: pass 100% of [JSONTestSuite](https://github.com/nst/JSONTestSuite).
