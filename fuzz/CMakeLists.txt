# fuzz/CMakeLists.txt – libFuzzer targets for beast-json
#
# Prerequisites
# ─────────────
#   • Clang (any version with libFuzzer support; tested with 18)
#   • libclang-rt-dev   (Ubuntu: `sudo apt install libclang-rt-18-dev`)
#
# Quick-start
# ───────────
#   cmake -B build-fuzz \
#         -DBEAST_JSON_BUILD_FUZZ=ON \
#         -DBEAST_JSON_BUILD_TESTS=OFF \
#         -DBEAST_JSON_BUILD_BENCHMARKS=OFF \
#         -DCMAKE_CXX_COMPILER=clang++
#   cmake --build build-fuzz
#
#   # Run indefinitely (Ctrl-C to stop, crashes saved as crash-*)
#   ./build-fuzz/fuzz/fuzz_parse  fuzz/corpus/ -max_len=65536
#   ./build-fuzz/fuzz/fuzz_tape   fuzz/corpus/ -max_len=65536
#
#   # Reproduce a specific crash
#   ./build-fuzz/fuzz/fuzz_parse  <crash-file>

# ── Compiler guard ────────────────────────────────────────────────────────────
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "BEAST_JSON_BUILD_FUZZ requires Clang (pass -DCMAKE_CXX_COMPILER=clang++). "
        "Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ── Dependencies ──────────────────────────────────────────────────────────────
# fast_float may already have been fetched by tests/ or benchmarks/; the
# FetchContent machinery deduplicates automatically.
include(FetchContent)
FetchContent_Declare(
    fast_float
    URL https://github.com/fastfloat/fast_float/archive/refs/tags/v6.1.1.tar.gz
)
FetchContent_MakeAvailable(fast_float)

# Expose fast_float to beast_json (idempotent if already done by tests/).
target_link_libraries(beast_json INTERFACE fast_float)

# ── Sanitizer flags ───────────────────────────────────────────────────────────
set(FUZZ_SANITIZERS "fuzzer,address,undefined")

# ── Helper function ───────────────────────────────────────────────────────────
function(add_fuzz_target name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE beast_json::beast_json)
    target_compile_features(${name} PRIVATE cxx_std_17)
    target_compile_options(${name} PRIVATE
        -g
        -O1
        -fsanitize=${FUZZ_SANITIZERS}
        -fno-omit-frame-pointer   # Better stack traces in crash reports
    )
    target_link_options(${name} PRIVATE
        -fsanitize=${FUZZ_SANITIZERS}
    )
endfunction()

# ── Fuzz targets ──────────────────────────────────────────────────────────────
add_fuzz_target(fuzz_parse)   # High-level beast::json::parse()
add_fuzz_target(fuzz_tape)    # Low-level tape::Parser (bitmap + standard)
