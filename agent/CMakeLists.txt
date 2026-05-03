cmake_minimum_required(VERSION 3.20)
project(InfernoAgent LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Pull in shared — makes this file self-sufficient ─────────────────────────
# When built via root CMakeLists, shared is already defined and this is skipped.
# When built in isolation (inside the agent container), this pulls it directly.
if(NOT TARGET shared_lib)
    set(_shared_lib "${CMAKE_CURRENT_SOURCE_DIR}/../shared/build/libshared_lib.a")
    if(EXISTS "${_shared_lib}")
        add_library(shared_lib STATIC IMPORTED GLOBAL)
        set_target_properties(shared_lib PROPERTIES
            IMPORTED_LOCATION "${_shared_lib}"
        )
        target_include_directories(shared_lib INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/../shared/include"
        )
    else()
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/../shared
            ${CMAKE_BINARY_DIR}/shared
        )
    endif()
endif()

# ── Agent library (all logic, no main) ───────────────────────────────────────
add_library(agent_lib STATIC
    src/agent_dispatcher.cpp
    # src/socket/SocketLinux.cpp    ← add as you create files
)

# PRIVATE here because nothing links agent_lib downstream.
# If another target ever needed agent_lib's headers, switch to PUBLIC.
target_include_directories(agent_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link shared_lib as PUBLIC so both agent and tests inherit shared includes.
target_link_libraries(agent_lib PUBLIC
    shared_lib
)

target_compile_options(agent_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Agent executable ──────────────────────────────────────────────────────────
add_executable(agent
    src/main.cpp
)

target_link_libraries(agent PRIVATE agent_lib)

target_compile_options(agent PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
find_package(GTest REQUIRED)

add_executable(agent_tests
    tests/main_test.cpp
    # tests/test_socket_linux.cpp   ← add as you create tests
)

target_include_directories(agent_tests PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(agent_tests PRIVATE
agent_lib           # brings in shared_lib transitively
GTest::gtest
GTest::gmock
GTest::gtest_main
)

target_compile_options(agent_tests PRIVATE
-Wall -Wextra -Wpedantic
)

add_test(NAME agent_tests COMMAND agent_tests)