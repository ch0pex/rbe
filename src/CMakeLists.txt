add_library(rbe INTERFACE)

target_sources(
  rbe
  PUBLIC
  FILE_SET HEADERS
  FILES
  rbe/rbe.hpp

  # Core
  rbe/core/endian.hpp
  rbe/core/memory_layout.hpp
  rbe/core/literals.hpp
  rbe/core/annotations.hpp

  # detail
  rbe/detail/introspection.hpp
  rbe/detail/memcpy_constexpr.hpp

)

add_library(rbe::rbe ALIAS rbe)
