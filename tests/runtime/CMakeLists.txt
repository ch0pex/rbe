include(testing)

create_test(memcpy_constexpr test_memcpy_constexpr.cpp)
create_test(endian test_endian.cpp)
create_test(format test_format.cpp)
create_test(memory_layout test_memory_layout.cpp)
create_test(serde test_serde.cpp)
create_test(static_array test_static_array.cpp)
create_test(static_string test_static_string.cpp)

