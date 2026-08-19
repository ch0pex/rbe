
function(rbe_enable_reflection target scope)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
    # -Wno-attributes is required because it reports annoations as attribute ignored
    # causing compilation to fail
    target_compile_options(${target} ${scope} -freflection -Wno-attributes)
  else()
    message(FATAL_ERROR
      "rbe requires C++26 reflection support, which your compiler doesn't support yet. "
      "Detected: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
      "Currently only GCC >= 16 supports reflection.")
  endif()
endfunction()

