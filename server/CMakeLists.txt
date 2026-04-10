cmake_minimum_required(VERSION 3.20)
project(InfernoServer LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Pull in shared — makes this file self-sufficient ─────────────────────────
# When built via root CMakeLists, shared is already defined and this is skipped.
# When built in isolation (inside the agent container), this pulls it directly.
if(NOT TARGET shared_lib)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
        ${CMAKE_BINARY_DIR}/shared
    )
endif()

# ── Server library (all logic, no main) ──────────────────────────────────────
add_library(server_lib STATIC
    src/placeholder.cpp
    # src/client_manager.cpp        ← add as you create files
    # src/command_dispatcher.cpp
)

target_include_directories(server_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_lib PRIVATE
    shared_lib
)

target_compile_options(server_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Server executable ─────────────────────────────────────────────────────────
add_executable(server
    src/main.cpp
)

target_link_libraries(server PRIVATE server_lib)

target_compile_options(server PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
find_package(GTest REQUIRED)

add_executable(server_tests
    tests/main_test.cpp
    # tests/test_command_dispatcher.cpp
)

target_include_directories(server_tests PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(server_tests PRIVATE
server_lib
GTest::gtest
GTest::gmock
GTest::gtest_main
)

target_compile_options(server_tests PRIVATE
-Wall -Wextra -Wpedantic
)

add_test(NAME server_tests COMMAND server_tests)