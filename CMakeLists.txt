cmake_minimum_required(VERSION 3.26)
project(rbe LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
option(BUILD_TESTING "Build the testing tree" OFF)
option(BUILD_EXAMPLES "Build the project examples" OFF)

add_subdirectory(src)


if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

# Install the library target
install(TARGETS rbe DESTINATION lib)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
