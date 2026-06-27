
add_library(unit_test_static STATIC
  test_introspection.cpp
)

target_link_libraries(unit_test_static PRIVATE rbe::rbe)
