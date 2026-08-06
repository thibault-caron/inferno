cmake_minimum_required(VERSION 3.20)
project(InfernoTestSupport LANGUAGES CXX)

find_package(GTest REQUIRED)

add_library(test_support INTERFACE)

target_include_directories(test_support INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_support INTERFACE
    GTest::gtest
    GTest::gmock
)