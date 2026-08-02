
add_library(unit_test_static STATIC
  test_introspection.cpp
  test_annotations.cpp
  test_concepts.cpp
)

target_link_libraries(unit_test_static PRIVATE rbe::rbe)
target_include_directories(unit_test_static PUBLIC ../common)
