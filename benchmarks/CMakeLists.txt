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
FetchContent_MakeAvailable(yyjson)

# Fetch fast_float (ultra-fast number parsing)
FetchContent_Declare(
    fast_float
    URL https://github.com/fastfloat/fast_float/archive/refs/tags/v6.1.1.tar.gz
)
FetchContent_MakeAvailable(fast_float)

# Fetch Glaze (C++23)
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.9.5
)
FetchContent_MakeAvailable(glaze)

# Download benchmark data
set(BENCHMARK_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
if(NOT EXISTS ${BENCHMARK_DATA_DIR}/twitter.json)
    message(STATUS "Downloading twitter.json...")
    file(DOWNLOAD 
        https://raw.githubusercontent.com/simdjson/simdjson/master/jsonexamples/twitter.json
        ${BENCHMARK_DATA_DIR}/twitter.json
        SHOW_PROGRESS
    )
endif()

# Optimization and PGO
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(-O3 -march=native -flto)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(PGO_MODE "" CACHE STRING "PGO mode: GENERATE or USE")
if(PGO_MODE STREQUAL "GENERATE")
    add_compile_options(-fprofile-generate)
    add_link_options(-fprofile-generate)
elseif(PGO_MODE STREQUAL "USE")
    add_compile_options(-fprofile-use=${CMAKE_SOURCE_DIR}/default.profdata -fprofile-correction)
    add_link_options(-fprofile-use=${CMAKE_SOURCE_DIR}/default.profdata -fprofile-correction)
endif()

# C++17 Benchmarks
add_executable(bench_file_io bench_file_io.cpp)
target_link_libraries(bench_file_io PRIVATE fast_float
 
    beast_json::beast_json 
    nlohmann_json::nlohmann_json
    simdjson
    yyjson
)
target_include_directories(bench_file_io PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_compile_features(bench_file_io PRIVATE cxx_std_17)

add_executable(bench_simple_struct bench_simple_struct.cpp)
target_link_libraries(bench_simple_struct PRIVATE fast_float
 
    beast_json::beast_json 
    nlohmann_json::nlohmann_json
    yyjson
)
target_include_directories(bench_simple_struct PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_compile_features(bench_simple_struct PRIVATE cxx_std_17)

add_executable(bench_complex_struct bench_complex_struct.cpp)
target_link_libraries(bench_complex_struct PRIVATE fast_float
 
    beast_json::beast_json 
    nlohmann_json::nlohmann_json
    yyjson
)
target_include_directories(bench_complex_struct PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_compile_features(bench_complex_struct PRIVATE cxx_std_17)

# C++23 Glaze Benchmark
add_executable(bench_glaze bench_glaze.cpp)
target_link_libraries(bench_glaze PRIVATE fast_float
 
    beast_json::beast_json
    glaze::glaze
    fast_float
)
target_compile_features(bench_glaze PRIVATE cxx_std_23)

# Profiling Benchmark (Multi-file)
add_executable(bench_profiling bench_profiling.cpp)
target_link_libraries(bench_profiling PRIVATE fast_float
 
    beast_json::beast_json 
    nlohmann_json::nlohmann_json
    simdjson
    yyjson
)
target_include_directories(bench_profiling PRIVATE ${rapidjson_SOURCE_DIR}/include)
target_compile_features(bench_profiling PRIVATE cxx_std_17)
