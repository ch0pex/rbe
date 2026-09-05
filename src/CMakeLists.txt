cmake_minimum_required(VERSION 3.31)

project(rbe VERSION 0.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_library(rbe INTERFACE rbe/dsrl/msg.hpp)

target_sources(
        rbe
        PUBLIC
        FILE_SET HEADERS
        FILES

        # annotations
        rbe/annotations/detail/base.hpp
        rbe/annotations/detail/correctness.hpp
        rbe/annotations/detail/dimension.hpp
        rbe/annotations/detail/traits.hpp
        rbe/annotations/detail/utils.hpp
        rbe/annotations/detail/view.hpp
        rbe/annotations/alignment.hpp
        rbe/annotations/derive.hpp
        rbe/annotations/endianness.hpp
        rbe/annotations/format.hpp
        rbe/annotations/metadata.hpp
        rbe/annotations/well_annotated_concepts.hpp

        # Core
        rbe/core/detail/context.hpp
        rbe/core/detail/introspection.hpp
        rbe/core/detail/invoke_concept.hpp
        rbe/core/detail/memcpy_constexpr.hpp
        rbe/core/detail/no_throw.hpp
        rbe/core/detail/normalize.hpp
        rbe/core/overload.hpp
        rbe/core/detail/static_array.hpp
        rbe/core/detail/static_string.hpp
        rbe/core/custom.hpp
        rbe/core/endian.hpp
        rbe/core/fmt.hpp
        rbe/core/memory_layout.hpp
        rbe/core/message_list.hpp
        rbe/core/metadata_layout.hpp
        rbe/core/trivially_wirable_concepts.hpp
        rbe/core/wirable_concepts.hpp
        rbe/core/wirable_primitives.hpp

        # deserialize
        rbe/dsrl/detail/deserialize_impl.hpp
        rbe/dsrl/detail/deserialize_member.hpp
        rbe/dsrl/detail/message_dispatcher_impl.hpp
        rbe/dsrl/detail/return_type_impl.hpp
        rbe/dsrl/any_msg.hpp
        rbe/dsrl/deserialize.hpp
        rbe/dsrl/message_dispatcher_concept.hpp
        rbe/dsrl/msg.hpp
        rbe/dsrl/packet.hpp
        rbe/dsrl/return_type.hpp
        rbe/dsrl/tags.hpp

        # serialize
        rbe/srl/detail/serialize_impl.hpp
        rbe/srl/serialize.hpp

        # Umbrella headers
        rbe/annotations.hpp
        rbe/core.hpp
        rbe/srl.hpp
        rbe/rbe.hpp
        rbe/dsrl.hpp
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
