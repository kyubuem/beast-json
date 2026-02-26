# Beast JSON Optimization - Performance Tracking & TODO (Up to Phase 30)

## Current Status (as of Phase 30)
- **Baseline (Phase 18)**: 298μs (Parse)
- **Current Optimum (Phase 30)**: **236μs** (Parse) 🎉
- **Target (yyjson)**: ~170us (Parse)
- **Current Gap**: ~66μs 

## 🚀 The Massive Breakthroughs (Phase 25-30)

We dropped from 284μs down to an incredible 236μs by fundamentally attacking memory writes, CPU register packing, and SIMD instruction density.

### ✅ What Worked & Brought Us to 236μs
1. **Phase 28: TapeNode Direct Memory Construction (The 15μs Drop)**
   - *Previous*: `*tape_head_++ = TapeNode(t, l, o);` caused Clang to eagerly pack a 128-bit structure in CPU registers before issuing a memory store.
   - *Fix*: Hand-assigned structure members via a pointer (`n->type = t; n->flags = 0;`).
   - *Result*: Allowed the CPU to stream parallel `strb`/`strh`/`str` instructions directly to the store buffer without horizontal ALU sequence stalls. Saved ~15μs globally!

2. **Phase 29: Ultimate NEON Whitespace Scanner (The 27μs Drop)**
   - *Previous*: Complex SWAR algorithm checking exactly ` `, `\n`, `\r`, `\t` costing 14 instructions.
   - *Fix*: Integrated a 3-instruction NEON fallback checking `c > 0x20` using `vld1q_u8` + `vcgtq_u8` + `vmaxvq_u32` (similar to yyjson). If `vmaxvq_u32 != 0`, a tight scalar loop pinpoints the token.
   - *Result*: Utterly obliterated whitespace skip loop overhead. Brought us down from 263μs to an all-time 236μs record!

3. **Phase 25 & 26: Double-Pump Number/String Parsing**
   - *Fix*: Numbers and Strings in JSON are heavily followed by structurally predictable separators (`,`, `:`, `}`). Immediately parsing and consuming these delimiters at the exit point of the node parser prevents the generic loop from returning to the giant `switch`, bypassing the switch overhead entirely for ~50% of the payload.

### ❌ What Failed / Was Reverted
1. **Phase 30: NEON String Parsing Extension**
   - Attempted to apply the `vmaxvq_u32` NEON trick to string fast-paths (for quotes and backslashes).
   - *Why it failed*: Tiny strings (2-6 bytes, abundant in twitter.json) prefer the SWAR 24-byte path because SWAR branches directly to exit without entering an inner loop. NEON incurred a 4-5μs loop startup penalty.

2. **Phase 24: The Big Hack (Recursive Descent Object Loop)**
   - Considered rewriting the object parser to strictly expect "key": value sequences.
   - *Status*: Dropped. The Phase 26 string double-pump implicitly reproduces this behavior inside the flat `switch` without duplicating multi-type value parsing code.

## 🎯 Phase 31+ Roadmap: Bridging the Final 66μs 
We have eliminated almost all fat from the main token extraction. To reach `yyjson`'s ~170μs benchmark, we must focus on architectural and branchless optimizations:

- [ ] **1. Data Layout & Tape Size Tuning (AST Compaction)**
  - *Context*: `yyjson` nodes are 16 bytes, and they achieve extreme cache locality by stripping out backward `prev` pointers.
  - *Action*: Investigate removing `prev` or merging `flags`/`aux` variables inside `beast-json`'s `TapeNode` to shrink the memory footprint and reduce L1 cache misses during large object parsing.
  - *Goal*: Increase nodes-per-cache-line.

- [ ] **2. Zero-Branch Eisel-Lemire Number Parsing**
  - *Context*: Currently, we fall back to a long scalar `while` loop for double/float digit extraction. `simdjson` and `yyjson` use Lemire's `fast_float` or similar exact float math to bypass scalar scans.
  - *Action*: Replace the `std::strtod` fallback and manual scanning loop with a customized, zero-branch fast-path for IEEE 754 floating-point conversion.
  - *Goal*: Eliminate branch mispredictions inside dense number arrays.

- [ ] **3. Static SIMD Structural Dispatch (Jump Tables)**
  - *Context*: We currently rely on a `switch(c)` block per extracted token. `yyjson` pads the file with trailing zeros and reads action characters via a 256-byte static jump table (`[action, ws, error]`).
  - *Action*: Refactor the main `parse()` loop to use a computed goto or branchless `if (!(type = table[c]))` lookup mechanism to guarantee instruction cache hits across unpredictable JSON payloads.
  - *Goal*: Remove CPU pipeline stalls caused by complex switch-case branch predictor misses.

- [ ] **4. Clang vs GCC Compiler Loop Pragma Tuning**
  - *Context*: Apple Clang might be emitting less optimal loops for ARM64 compared to GCC or Clang 18+. 
  - *Action*: Test `#pragma GCC unroll` inside the exact hotspots (like the string SWAR loop) and analyze the generated assembly for `LDR/STR` pair optimizations.

### 🧹 Infrastructure Note
*Legacy DOM tests (`test_json_pointer.cpp`, `test_iterators.cpp`) were permanently removed in Phase 30. All future features and tests will be built exclusively around the zero-copy, streaming `beast::json::lazy::Value` architecture.*
