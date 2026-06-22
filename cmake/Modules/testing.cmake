function(create_test test_name test_src)
    if (NOT TARGET ${PROJECT_NAME}-tests)
        add_custom_target(${PROJECT_NAME}-tests)
    endif ()

    set(target_exe "test_${PROJECT_NAME}_${test_name}")

    set(options "")
    set(oneValueArgs RESOURCE_LOCK)
    set(multiValueArgs LABELS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${target_exe} ${test_src})
    target_link_libraries(${target_exe} PRIVATE doctest::doctest ${PROJECT_NAME}-lib)

    add_test(NAME ${target_exe} COMMAND ${target_exe})

    add_dependencies(${PROJECT_NAME}-tests ${target_exe})

    if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set_tests_properties(${target_exe} PROPERTIES
                WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif ()

    set(final_labels "${PROJECT_NAME}")
    if (ARG_LABELS)
        list(APPEND final_labels ${ARG_LABELS})
    endif ()

    set_tests_properties(${target_exe} PROPERTIES
            LABELS "${final_labels}"
    )

    if (ARG_RESOURCE_LOCK)
        set_tests_properties(${target_exe} PROPERTIES
                RESOURCE_LOCK "${ARG_RESOURCE_LOCK}"
        )
    endif ()

    if (SANITIZED_BUILD)
        set_tests_properties(${target_exe} PROPERTIES
                ENVIRONMENT "ASAN_OPTIONS=detect_odr_violation=0;LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/cmake/supp/lsan.supp"
        )
    endif ()

endfunction()


function(integration_test test_name test_src)
    if (NOT TARGET ${PROJECT_NAME}-tests)
        add_custom_target(${PROJECT_NAME}-tests)
    endif ()

    set(target_exe "ftest_${PROJECT_NAME}_${test_name}")

    add_executable(${target_exe} ${test_src})
    target_link_libraries(${target_exe} PRIVATE ${PROJECT_NAME}-lib)

    add_dependencies(${PROJECT_NAME}-tests ${target_exe})
endfunction()

# Common testing setup
if (BUILD_TESTING)
    find_package(doctest CONFIG REQUIRED)
    enable_testing()
    find_program(MEMORYCHECK_COMMAND valgrind)
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1 --suppressions=${CMAKE_SOURCE_DIR}/cmake/supp/valgrind.supp")
    include(CTest)
endif ()
