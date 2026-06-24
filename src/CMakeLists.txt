add_library(rbe INTERFACE)

target_sources(
  rbe
  PUBLIC
  FILE_SET HEADERS
  FILES
  rbe/rbe.hpp

  # Core
  rbe/core/annotations.hpp
  rbe/core/literals.hpp

  # detail
  rbe/detail/endian.hpp
  rbe/detail/introspection.hpp

)

add_library(rbe::rbe ALIAS rbe)
