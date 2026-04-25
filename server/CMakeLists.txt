cmake_minimum_required(VERSION 3.20)
project(InfernoServer LANGUAGES CXX)
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

# ── Server library (all logic, no main) ──────────────────────────────────────
add_library(server_lib STATIC
    src/tcp_server.cpp
    src/client_session.cpp 
    src/dispatcher.cpp
    # src/client_manager.cpp        ← add as you create files
    # src/command_dispatcher.cpp
)

target_include_directories(server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_lib PUBLIC
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
    tests/client_session_test.cpp
    tests/dispatcher_test.cpp
    tests/server_integration_test.cpp
    tests/tcp_server_test.cpp
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