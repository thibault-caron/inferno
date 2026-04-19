cmake_minimum_required(VERSION 3.20)
project(InfernoShared LANGUAGES CXX)

# ── Library ───────────────────────────────────────────────────────────────────
# STATIC = compiled once into a .a archive, then linked into agent and server.
# No Dockerfile, no executable — this is purely a library.
add_library(shared_lib STATIC
    src/protocol/protocol_parser.cpp
    src/protocol/protocol_serializer.cpp
    src/socket/linux_socket.cpp
    src/socket/socket_factory.cpp
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

    set(PARSER_TESTS
        tests/protocol/parser/protocol_parser_command_test.cpp
        tests/protocol/parser/protocol_parser_data_test.cpp
        tests/protocol/parser/protocol_parser_error_test.cpp
        tests/protocol/parser/protocol_parser_register_test.cpp
        tests/protocol/parser/protocol_parser_response_test.cpp
        tests/protocol/parser/protocol_parser_test.cpp
    )

    set(SERIALIZER_TESTS
        tests/protocol/serializer/protocol_serializer_command_test.cpp
        tests/protocol/serializer/protocol_serializer_data_test.cpp
        tests/protocol/serializer/protocol_serializer_register_test.cpp
        tests/protocol/serializer/protocol_serializer_response_test.cpp
        tests/protocol/serializer/protocol_serializer_test.cpp
    )

    set(SOCKET_TESTS
        tests/socket/socket_integration_test.cpp
        )

    add_executable(shared_tests
        tests/convert_endian_test.cpp
        tests/protocol/protocol_roundtrip_test.cpp
        ${PARSER_TESTS}
        ${SERIALIZER_TESTS}
        ${SOCKET_TESTS}
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