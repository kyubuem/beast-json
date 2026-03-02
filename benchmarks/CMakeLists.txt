cmake_minimum_required(VERSION 3.14)
project(beast_json_benchmarks)

include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch RapidJSON
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG master
)
FetchContent_MakeAvailable(rapidjson)

# Fetch simdjson
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG v3.10.1
)
FetchContent_MakeAvailable(simdjson)

# Fetch yyjson
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 0.10.0
)
set(YYJSON_DISABLE_SIMD OFF CACHE BOOL "Disable SIMD in yyjson" FORCE)
FetchContent_MakeAvailable(yyjson)

# Fetch fast_float removed

# Fetch Glaze (C++23)
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.9.5
)
FetchContent_MakeAvailable(glaze)

# Download benchmark data
set(BENCHMARK_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(BENCHMARK_FILES
    "twitter.json"
    "canada.json"
    "citm_catalog.json"
    "gsoc-2018.json"
)

foreach(file ${BENCHMARK_FILES})
    if(NOT EXISTS ${BENCHMARK_DATA_DIR}/${file})
        message(STATUS "Downloading ${file}...")
        file(DOWNLOAD 
            https://raw.githubusercontent.com/simdjson/simdjson-data/master/jsonexamples/${file}
            ${BENCHMARK_DATA_DIR}/${file}
            SHOW_PROGRESS
        )
    endif()
endforeach()

# Optimization and PGO
# Note: -fuse-ld=lld is incompatible with GCC LTO (GIMPLE IR).
# Use the default linker (bfd/gold) which understands GCC LTO bitcode.
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-O3 -flto)
    add_link_options(-flto)
endif()

# Phase 47: Profile-Guided Optimization (PGO)
# Usage:
#   Step 1 — instrument build:
#     cmake -DPGO_MODE=GENERATE -B build && cmake --build build -j$(nproc)
#     ./build/benchmarks/bench_all --iter 30 --all   # collects .gcda profiles
#   Step 2 — optimized build:
#     cmake -DPGO_MODE=USE -B build && cmake --build build -j$(nproc)
#
# GCC PGO: .gcda files are written alongside .o files in the build directory.
# -fprofile-use (without a path) reads them from the same location automatically.
# -fprofile-correction tolerates minor profile/source mismatches.
set(PGO_MODE "" CACHE STRING "PGO mode: GENERATE or USE (leave empty for normal build)")
if(PGO_MODE STREQUAL "GENERATE")
    message(STATUS "PGO: instrument build (GENERATE) — run benchmarks to collect profiles")
    add_compile_options(-fprofile-generate)
    add_link_options(-fprofile-generate)
elseif(PGO_MODE STREQUAL "USE")
    message(STATUS "PGO: optimized build (USE) — reading .gcda profiles from build tree")
    add_compile_options(-fprofile-use -fprofile-correction)
    add_link_options(-fprofile-use -fprofile-correction)
endif()

# C++17 Benchmarks
add_executable(bench_file_io bench_file_io.cpp)
target_link_libraries(bench_file_io PRIVATE 
    beast_json::beast_json 
    nlohmann_json::nlohmann_json
    simdjson
    yyjson
)
target_include_directories(bench_file_io PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_compile_features(bench_file_io PRIVATE cxx_std_20)

# Phase 17: Lazy-Manifest DOM Benchmark
add_executable(bench_lazy_dom bench_lazy_dom.cpp)
target_link_libraries(bench_lazy_dom PRIVATE
    beast_json::beast_json
    nlohmann_json::nlohmann_json
    yyjson
)
target_compile_features(bench_lazy_dom PRIVATE cxx_std_20)

# Unified benchmark: beast::lazy + yyjson + nlohmann, all 4 standard files
# Usage: ./bench_all [file.json]  OR  ./bench_all --all
add_executable(bench_all bench_all.cpp)
target_link_libraries(bench_all PRIVATE
    beast_json::beast_json
    nlohmann_json::nlohmann_json
    yyjson
)
target_compile_features(bench_all PRIVATE cxx_std_20)

# beast::json-only microbenchmark — parse + dump, no third-party dependencies.
# Safe to run on Android kernels that do not expose SVE to userspace.
# Usage: ./bench_quick [--all] [--iter N]
add_executable(bench_quick bench_quick.cpp)
target_link_libraries(bench_quick PRIVATE beast_json::beast_json)
target_compile_features(bench_quick PRIVATE cxx_std_20)

# ── SVE safety fix ────────────────────────────────────────────────────────────
# On AArch64 with -mcpu=cortex-x3 (or -march=native), Clang's auto-vectorizer
# generates SVE instructions both at compile time and at LTO link time.
# Android kernel 5.15 does not expose SVE to userspace → SIGILL (exit 132).
#
# Fix: disable the loop auto-vectorizer (-fno-vectorize) and LTO (-fno-lto)
# per-target.  beast_json uses explicit arm_neon.h intrinsics which are
# unaffected by -fno-vectorize; third-party libraries (nlohmann, yyjson,
# simdjson) are constrained to NEON-only auto-vectorization this way.
foreach(_tgt bench_all bench_file_io bench_lazy_dom bench_quick)
    if(TARGET ${_tgt})
        target_compile_options(${_tgt} PRIVATE -fno-lto -fno-vectorize -fno-slp-vectorize)
        target_link_options(${_tgt} PRIVATE -fno-lto)
    endif()
endforeach()
