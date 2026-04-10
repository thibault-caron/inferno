cmake_minimum_required(VERSION 3.20)
project(INFERNO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(shared)   # MUST be first — agent and server depend on it
add_subdirectory(agent)
add_subdirectory(server)