cmake_minimum_required(VERSION 3.31)

project(rbe VERSION 0.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_library(rbe INTERFACE rbe/dsrl/msg.hpp)

target_sources(
        rbe
        PUBLIC
        FILE_SET HEADERS
        FILES
        rbe/rbe.hpp
        rbe/srl.hpp
        rbe/dsrl.hpp
        rbe/concepts.hpp
        rbe/core.hpp

        # Core
        rbe/core/annotations.hpp
        rbe/core/endian.hpp
        rbe/core/fmt.hpp
        rbe/core/literals.hpp
        rbe/core/memory_layout.hpp
        rbe/core/custom.hpp
        rbe/core/static_array.hpp
        rbe/core/static_string.hpp

        # detail
        rbe/detail/annotation_base.hpp
        rbe/detail/annotations_correctness.hpp
        rbe/detail/annotations_utils.hpp
        rbe/detail/annotations_view.hpp
        rbe/detail/deserialize_fwd.hpp
        rbe/detail/deserialize_member.hpp
        rbe/detail/introspection.hpp
        rbe/detail/memcpy_constexpr.hpp
        rbe/detail/normalize.hpp
        rbe/detail/passkey.hpp

        # concepts
        rbe/concepts/trivially_wirable.hpp
        rbe/concepts/wirable.hpp
        rbe/concepts/wirable_primitives.hpp

        # deserialize
        rbe/dsrl/deserialize.hpp
        rbe/dsrl/msg.hpp
        rbe/dsrl/tags.hpp

        # serialize
        rbe/srl/serialize.hpp
)

add_library(rbe::rbe ALIAS rbe)
target_compile_features(rbe INTERFACE cxx_std_26)

include(enable_reflection)
rbe_enable_reflection(rbe INTERFACE)

install(TARGETS rbe
    EXPORT rbeTargets
    FILE_SET HEADERS
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_file(cmake/rbeConfig.cmake.in rbeConfig.cmake @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(rbeConfigVersion.cmake COMPATIBILITY SameMajorVersion)
install(EXPORT rbeTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rbe NAMESPACE rbe::)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/rbeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/rbeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rbe
)
