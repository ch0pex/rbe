function(add_example example_name executable)
  add_executable(${example_name} ${executable})
  target_link_libraries(${example_name} PRIVATE rbe::rbe)
endfunction()

# Complete examples
add_example(markets markets.cpp)

# How-to examples, one per page under docs/examples/howto/
add_example(howto_annotations howto/annotations.cpp)
add_example(howto_serialize howto/serialize.cpp)
add_example(howto_deserialization_modes howto/deserialization_modes.cpp)
add_example(howto_lazy_fields howto/lazy_fields.cpp)
add_example(howto_wide_contracts howto/wide_contracts.cpp)
add_example(howto_any_match howto/any_match.cpp)
add_example(howto_many howto/many.cpp)
add_example(howto_flatten howto/flatten.cpp)
add_example(howto_format howto/format.cpp)

# Snippets included by the documentation (docs/), kept here so every documented example compiles
add_example(docs_home docs/home.cpp)
add_example(docs_overview docs/overview.cpp)
add_example(docs_installation docs/installation.cpp)
add_example(docs_reference_annotations docs/reference_annotations.cpp)
add_example(docs_reference_serialization docs/reference_serialization.cpp)
add_example(docs_reference_deserialization docs/reference_deserialization.cpp)
add_example(docs_reference_core docs/reference_core.cpp)
add_example(docs_reference_frames docs/reference_frames.cpp)
