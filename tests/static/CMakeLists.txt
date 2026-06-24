
add_library(unit_test_static STATIC
  test_endian.cpp
  test_introspection.cpp
  test_memcpy_constexpr.cpp
)

target_link_libraries(unit_test_static PRIVATE rbe::rbe)
