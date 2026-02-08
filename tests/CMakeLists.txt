
# tests/CMakeLists.txt

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Fetch fast_float (needed for beast_json)
FetchContent_Declare(
    fast_float
    URL https://github.com/fastfloat/fast_float/archive/refs/tags/v6.1.1.tar.gz
)
FetchContent_MakeAvailable(fast_float)

# Legacy test function (keeps own main)
function(add_beast_legacy_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE beast_json::beast_json fast_float)
    target_compile_features(${name} PRIVATE cxx_std_17)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# GTest function (uses gtest_main)
function(add_beast_gtest name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE beast_json::beast_json gtest_main)
    target_compile_features(${name} PRIVATE cxx_std_17)
    
    # Enable coverage
    target_compile_options(${name} PRIVATE -O0 -g --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(${name} PRIVATE --coverage)

    gtest_discover_tests(${name})
endfunction()

# Migrated tests
add_beast_gtest(test_relaxed)
add_beast_gtest(test_unicode)
add_beast_gtest(test_strict_number)
add_beast_gtest(test_control_char)
add_beast_gtest(test_json_pointer)
add_beast_gtest(test_comments)
add_beast_gtest(test_trailing_commas)
add_beast_gtest(test_utf8_validation)
add_beast_gtest(test_duplicate_keys)
add_beast_gtest(test_json_patch)
add_beast_gtest(test_merge_patch)
add_beast_gtest(test_fluent)
add_beast_gtest(test_reflection)
add_beast_gtest(test_iterators)
add_beast_gtest(test_serializer)
add_beast_gtest(test_diagnostics)
add_beast_gtest(test_string_structure)
add_beast_gtest(test_depth)
add_beast_gtest(test_quote_mask)
add_beast_gtest(test_bitmap)
add_beast_gtest(test_bitmap_offsets)
add_beast_gtest(test_simple_debug)
add_beast_gtest(test_parallel_debug)
add_beast_legacy_test(test_tape_features)
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

# Copy test data to build directory
file(COPY ${PROJECT_SOURCE_DIR}/data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Benchmark Executable

