function(create_test_suite target_name)
    add_executable(${target_name} ${ARGN})
    target_link_libraries(${target_name} PRIVATE doctest::doctest rbe::rbe)
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/tests/common)

    # if (SANITIZED_BUILD)
    #     set(doctest_discover_tests_extra_args
    #             PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_odr_violation=0;LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/cmake/supp/lsan.supp"
    #     )
    # endif ()

    doctest_discover_tests(${target_name}
            WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            PROPERTIES LABELS "rbe"
            ${doctest_discover_tests_extra_args}
    )
endfunction()


find_package(doctest CONFIG REQUIRED)
enable_testing()
include(doctest)
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1 --suppressions=${CMAKE_SOURCE_DIR}/cmake/supp/valgrind.supp")
include(CTest)
