cmake_minimum_required(VERSION 3.25)
project(rbe-dev LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/cmake")

include(warnings)
include(amalgamate)
include(ccache)

set_warnings()
enable_ccache(MODE DIRECT_PREPROCESSOR BASE_DIR "${PROJECT_SOURCE_DIR}")

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_subdirectory(example)

