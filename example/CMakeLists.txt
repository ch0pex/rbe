function(add_example example_name executable)
  add_executable(${example_name} ${executable})
  target_link_libraries(${example_name} PRIVATE rbe::rbe)
endfunction()

add_example(eager_deserialization dsrl/eager.cpp)
add_example(lazy_deserialization dsrl/lazy.cpp)
add_example(in_place dsrl/in_place.cpp)
add_example(markets markets.cpp)
add_example(annotations annotations.cpp)
# add_example(custom custom.cpp)
# add_example(round_trip round_trip.cpp)


# Snippets included by the documentation (docs/), kept here so every documented example compiles
add_example(docs_home docs/home.cpp)
add_example(docs_quick_start docs/quick_start.cpp)
add_example(docs_reference_annotations docs/reference_annotations.cpp)
add_example(docs_reference_serialization docs/reference_serialization.cpp)
add_example(docs_reference_deserialization docs/reference_deserialization.cpp)
