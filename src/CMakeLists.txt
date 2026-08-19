add_library(rbe INTERFACE
        rbe/dsrl/msg.hpp)

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
