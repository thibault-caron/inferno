cmake_minimum_required(VERSION 3.20)
project(InfernoAgent LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Configuration spécifique à Windows ──────────────────────────────────────
if(WIN32)
    # 1. Force OpenSSL à chercher les bibliothèques statiques (.a / .lib)
    set(OPENSSL_USE_STATIC_LIBS TRUE)

    # 2. Définit la version de Windows cible (Vista / Win 7+) et nettoie windows.h
    add_compile_definitions(WINVER=0x0600 _WIN32_WINNT=0x0600)
endif()

# ── OpenSSL ───────────────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)

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

# ── Agent library (all logic, no main) ───────────────────────────────────────

# ── Sources communes à toutes les plateformes ────────────────────────────────
set(METRICS_SOURCES
    src/metrics/metrics_controller.cpp
    src/metrics/metrics_scrapper_factory.cpp
)

set(SYSTEM_MONITOR_SOURCES
    src/system_monitor/system_monitor_factory.cpp
)

# ── Sources et dépendances spécifiques au système ─────────────────────────
if(WIN32)
    list(APPEND METRICS_SOURCES
        src/metrics/windows_metrics_scrapper.cpp
    )
    list(APPEND SYSTEM_MONITOR_SOURCES
        src/system_monitor/windows_system_monitor.cpp
    )
else()
    list(APPEND METRICS_SOURCES
        src/metrics/linux_metrics_scrapper.cpp
    )
    list(APPEND SYSTEM_MONITOR_SOURCES
        src/system_monitor/linux_system_monitor.cpp
    )
endif()

add_library(agent_lib STATIC
    src/dispatcher/agent_dispatcher.cpp
    src/agent_session.cpp
    src/agent_loop.cpp
    ${METRICS_SOURCES}
    ${SYSTEM_MONITOR_SOURCES}
    src/time/time_converter.cpp
)

# PRIVATE here because nothing links agent_lib downstream.
# If another target ever needed agent_lib's headers, switch to PUBLIC.
target_include_directories(agent_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link transport_lib as PUBLIC so both agent and tests inherit transport includes.
target_link_libraries(agent_lib PUBLIC
    transport_lib
)

# bibliothèques système Windows
if(WIN32)
    target_link_libraries(agent_lib PRIVATE
        pdh         # Pour PDH (PdhOpenQuery, etc.)
        iphlpapi    # Pour GetAdaptersAddresses (IP/MAC)
        psapi       # GetProcessMemoryInfo
        ws2_32      # Pour les sockets Windows (inet_ntop, etc.)
    )
endif()

target_compile_options(agent_lib PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Agent executable ──────────────────────────────────────────────────────────
add_executable(agent
    src/main.cpp
)

if(WIN32)
    # 1. Intègre libgcc et libstdc++ directement dans le .exe
    target_link_options(agent PRIVATE -static-libgcc -static-libstdc++)
    
    # 2. Intègre la gestion des threads (winpthread) et les autres libs système C en statique
    target_link_libraries(agent PRIVATE -static)

    # 3. Forcer OpenSSL en statique (si tu utilises OpenSSL via find_package)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

target_link_libraries(agent PRIVATE agent_lib)

target_compile_options(agent PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ─────────────────────────────────────────────────────────────────────
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
    # tests/test_socket_linux.cpp   ← add as you create tests
)

    target_include_directories(agent_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../transport/tests
    )

    target_link_libraries(agent_tests PRIVATE
        agent_lib           # brings in shared_lib transitively
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