add_library(rbe INTERFACE)


target_sources(
  rbe
  PUBLIC
  FILE_SET HEADERS
  FILES

)




add_library(rbe::rbe ALIAS rbe)
