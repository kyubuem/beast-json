# bindings/c/CMakeLists.txt — Beast JSON C Shared Library

add_library(beast_json_c SHARED beast_json_c.cpp)
target_link_libraries(beast_json_c PRIVATE beast_json::beast_json)
target_compile_features(beast_json_c PRIVATE cxx_std_20)
target_include_directories(beast_json_c
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

set_target_properties(beast_json_c PROPERTIES
    VERSION   1.0.0
    SOVERSION 1
    PUBLIC_HEADER beast_json_c.h)

# Install targets
install(TARGETS beast_json_c
    LIBRARY       DESTINATION lib
    PUBLIC_HEADER DESTINATION include)

message(STATUS "beast_json_c shared library target configured")
