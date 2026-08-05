function(add_example example_name executable)
  add_executable(${example_name} ${executable})
  target_link_libraries(${example_name} PRIVATE rbe::rbe)
endfunction()

add_example(eager_deserialization dsrl/eager.cpp)
add_example(lazy_deserialization dsrl/lazy.cpp)
add_example(in_place dsrl/in_place.cpp)
add_example(markets markets.cpp)
# add_example(custom custom.cpp)
# add_example(round_trip round_trip.cpp)

