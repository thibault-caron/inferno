cmake_minimum_required(VERSION 3.20)
project(InfernoShared LANGUAGES CXX)

# ── Library ───────────────────────────────────────────────────────────────────
# STATIC = compiled once into a .a archive, then linked into agent and server.
# No Dockerfile, no executable — this is purely a library.
add_library(shared_lib STATIC
    src/protocol/protocol_parser.cpp
    src/protocol/protocol_parser_command.cpp
    src/protocol/protocol_parser_data.cpp
    src/protocol/protocol_parser_error.cpp
    src/protocol/protocol_parser_register.cpp
    src/protocol/protocol_parser_response.cpp
    src/protocol/protocol_serializer.cpp
    # src/protocol/serializer.cpp
)

# PUBLIC = these include dirs are inherited by any target that links shared_lib.
# This is what lets agent and server write #include "ISocket.hpp" without paths.
target_include_directories(shared_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(shared_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(CMAKE_PROJECT_NAME STREQUAL "InfernoShared")
    enable_testing()
    find_package(GTest REQUIRED)

    add_executable(shared_tests
        tests/convert_endian_test.cpp
        tests/protocol/protocol_parser_command_test.cpp
        tests/protocol/protocol_parser_data_test.cpp
        tests/protocol/protocol_parser_error_test.cpp
        tests/protocol/protocol_parser_register_test.cpp
        tests/protocol/protocol_parser_response_test.cpp
        tests/protocol/protocol_parser_test.cpp
        tests/protocol/protocol_serializer_test.cpp
    )

    target_include_directories(shared_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(shared_tests PRIVATE
        shared_lib
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )

    target_compile_options(shared_tests PRIVATE
        -Wall -Wextra -Wpedantic
    )
    
    add_test(NAME shared_tests COMMAND shared_tests)
    # include(GoogleTest)
    # gtest_discover_tests(shared_tests DISCOVERY_MODE PRE_TEST)

endif()