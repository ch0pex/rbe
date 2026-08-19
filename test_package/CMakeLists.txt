
cmake_minimum_required(VERSION 3.15)
project(test_package CXX)

find_package(rbe REQUIRED CONFIG)

add_executable(test_package src/test_package.cpp)
target_link_libraries(test_package PRIVATE rbe::rbe)
