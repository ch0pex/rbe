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
        rbe/core/concepts.hpp
        rbe/core/endian.hpp
        rbe/core/fmt.hpp
        rbe/core/literals.hpp
        rbe/core/memory_layout.hpp
        rbe/core/serder.hpp
        rbe/core/static_array.hpp
        rbe/core/static_string.hpp

        # detail
        rbe/detail/introspection.hpp
        rbe/detail/memcpy_constexpr.hpp
        rbe/detail/passkey.hpp

        rbe/dsrl/deserialize.hpp
        rbe/dsrl/msg.hpp
        rbe/dsrl/tags.hpp

        rbe/srl/serialize.hpp
)

add_library(rbe::rbe ALIAS rbe)
