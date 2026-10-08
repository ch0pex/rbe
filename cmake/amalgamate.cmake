
# Single header version of the library: `cmake --build <dir> --target amalgamate`, written to
# <dir>/single_include/rbe/rbe.hpp so that `-I <dir>/single_include` replaces the include path of the library
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  file(GLOB_RECURSE RBE_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/rbe/*.hpp")
  add_custom_target(amalgamate
      COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/scripts/amalgamate.py"
              --output "${PROJECT_BINARY_DIR}/single_include/rbe/rbe.hpp"
      DEPENDS "${PROJECT_SOURCE_DIR}/scripts/amalgamate.py" ${RBE_HEADERS}
      COMMENT "Generating the rbe single header"
      VERBATIM)
endif()
