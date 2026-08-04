cmake_minimum_required(VERSION 3.20)
project(InfernoAgent LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Windows configuration ─────────────────────────────────────────────────────
if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    add_compile_definitions(WINVER=0x0600 _WIN32_WINNT=0x0600)
endif()

# ── OpenSSL ───────────────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)

# ── Transport library ─────────────────────────────────────────────────────────

if(NOT TARGET transport_lib)
    set(_transport_lib "${CMAKE_CURRENT_SOURCE_DIR}/../transport/build/libtransport_lib.a")
    if(EXISTS "${_transport_lib}")
        add_library(transport_lib STATIC IMPORTED GLOBAL)
        set_target_properties(transport_lib PROPERTIES
            IMPORTED_LOCATION        "${_transport_lib}"
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

# ── Platform-specific sources ─────────────────────────────────────────────────
if(WIN32)
    set(PLATFORM_METRICS_SRC  src/metrics/windows_metrics_scrapper.cpp)
    set(PLATFORM_MONITOR_SRC  src/system_monitor/windows_system_monitor.cpp)
else()
    set(PLATFORM_METRICS_SRC  src/metrics/linux_metrics_scrapper.cpp)
    set(PLATFORM_MONITOR_SRC  src/system_monitor/linux_system_monitor.cpp)
endif()

# ── Sources ───────────────────────────────────────────────────────────────────
set(DISPATCHER_SOURCES
    src/dispatcher/agent_dispatcher.cpp
)

set(METRICS_SOURCES
    src/metrics/metrics_controller.cpp
    src/metrics/metrics_scrapper_factory.cpp
    ${PLATFORM_METRICS_SRC}
)

set(MONITOR_SOURCES
    src/system_monitor/system_monitor_factory.cpp
    ${PLATFORM_MONITOR_SRC}
)

# ── Agent library (all logic, no main) ───────────────────────────────────────
add_library(agent_lib STATIC
    src/agent_session.cpp
    src/agent_loop.cpp
    src/time/time_converter.cpp
    ${DISPATCHER_SOURCES}
    ${METRICS_SOURCES}
    ${MONITOR_SOURCES}
)

target_include_directories(agent_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# PUBLIC: agent and tests both inherit transport includes transitively.
target_link_libraries(agent_lib PUBLIC
    transport_lib
)

if(WIN32)
    target_link_libraries(agent_lib PRIVATE
        pdh       # PdhOpenQuery, PdhAddCounter, …
        iphlpapi  # GetAdaptersAddresses
        psapi     # GetProcessMemoryInfo
        ws2_32    # inet_ntop, socket helpers
    )
endif()

target_compile_options(agent_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Agent executable ──────────────────────────────────────────────────────────
add_executable(agent
    src/main.cpp
)

target_link_libraries(agent PRIVATE agent_lib)

if(WIN32)
    # Bundle runtime libs so the .exe runs without MinGW installed. (libs copied inside the .exe)
    target_link_options(agent PRIVATE -static-libgcc -static-libstdc++)
    target_link_libraries(agent PRIVATE -static)
endif()

target_compile_options(agent PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests (Linux only — no GTest on MinGW cross-build) ───────────────────────
if(NOT WIN32)
    enable_testing()
    find_package(GTest REQUIRED)

    if(NOT TARGET test_support)
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/../test_support
            ${CMAKE_CURRENT_BINARY_DIR}/test_support
        )
    endif()

    add_executable(agent_tests
        tests/agent_dispatcher_test.cpp
        tests/system_monitor/system_monitor_test.cpp
        tests/metrics/linux_metrics_scrapper_test.cpp
    )

    target_include_directories(agent_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../transport/tests
    )

    target_link_libraries(agent_tests PRIVATE
        agent_lib
        test_support
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )

    target_compile_options(agent_tests PRIVATE
        -Wall -Wextra -Wpedantic
    )

    add_test(NAME agent_tests COMMAND agent_tests)
endif()


