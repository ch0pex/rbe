include(testing)

create_test_suite(test_rbe_runtime
    test_main.cpp
    test_memcpy_constexpr.cpp
    test_endian.cpp
    test_format.cpp
    test_memory_layout.cpp
    test_no_throw.cpp
    test_serde.cpp
    test_static_array.cpp
    test_static_string.cpp
)

