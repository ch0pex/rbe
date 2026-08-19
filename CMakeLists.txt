cmake_minimum_required(VERSION 3.31)
project(rbe-dev LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/cmake")

include(warnings)
set_warnings()

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_subdirectory(example)

