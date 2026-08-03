cmake_minimum_required(VERSION 3.20)
project(InfernoServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── OpenSSL ───────────────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
# set(CMAKE_FIND_DEBUG_MODE ON)
find_package(libpqxx REQUIRED)   
# set(CMAKE_FIND_DEBUG_MODE OFF)

# ── Pull in shared — makes this file self-sufficient ─────────────────────────
# When built via root CMakeLists, shared is already defined and this is skipped.
# When built in isolation (inside the agent container), this pulls it directly.
if(NOT TARGET transport_lib)
    set(_transport_lib "${CMAKE_CURRENT_SOURCE_DIR}/../transport/build/libtransport_lib.a")
    if(EXISTS "${_transport_lib}")
        add_library(transport_lib STATIC IMPORTED GLOBAL)
        set_target_properties(transport_lib PROPERTIES
            IMPORTED_LOCATION "${_transport_lib}"
            INTERFACE_LINK_LIBRARIES "OpenSSL::SSL;OpenSSL::Crypto"
        )
        target_include_directories(transport_lib INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/../transport/include"
        )
    else()
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/../transport
            ${CMAKE_BINARY_DIR}/transport
        )
    endif()
endif()

set(REPOSITORY_SOURCES
    src/repository/database_connection.cpp
    src/repository/agent_repository.cpp
    src/repository/command_repository.cpp
    src/repository/response_repository.cpp
    src/repository/metrics_repository.cpp
)

set(SERVICE_SOURCES
    src/service/agent_service.cpp
    src/service/command_service.cpp
    src/service/metrics_service.cpp
    src/service/response_service.cpp
)


# ── Server library (all logic, no main) ──────────────────────────────────────
add_library(server_lib STATIC
    src/tcp_server.cpp
    src/dispatcher/server_dispatcher.cpp
    src/reactor.cpp
    src/agent_connection.cpp
    src/poller/epoller.cpp
    src/session_manager.cpp
    ${REPOSITORY_SOURCES}
    ${SERVICE_SOURCES}
    # src/agent_manager.cpp        ← add as you create files
    # src/command_dispatcher.cpp
)

target_include_directories(server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_lib PUBLIC
    transport_lib
    libpqxx::pqxx
    
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

if(NOT TARGET test_support)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../test_support
        ${CMAKE_CURRENT_BINARY_DIR}/test_support
    )
endif()

add_executable(server_tests
    # tests/agent_session_test.cpp
    tests/server_dispatcher_test.cpp
    tests/epoller_test.cpp
    tests/reactor_integration_test.cpp
    tests/tcp_server_test.cpp
)

target_include_directories(server_tests PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/tests
${CMAKE_CURRENT_SOURCE_DIR}/../transport/tests
)

target_link_libraries(server_tests PRIVATE
server_lib
test_support
GTest::gtest
GTest::gmock
GTest::gtest_main
)

target_compile_options(server_tests PRIVATE
-Wall -Wextra -Wpedantic
)

add_test(NAME server_tests COMMAND server_tests)