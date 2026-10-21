# Header Map

Every public header under `src/rbe/`, and the umbrella that includes it. `#include <rbe/rbe.hpp>` brings in
everything; the umbrellas bring in one layer; the granular headers one thing. With the single header,
everything comes from `<rbe/rbe.hpp>`.

Directories named `detail/` and namespaces named `detail` (`rbe::detail`, `rbe::frames::detail`, ...) are
private: user code never needs to name them, and they change without notice.

## Umbrellas

| Header | Includes |
|---|---|
| `rbe/rbe.hpp` | The five below. |
| `rbe/core.hpp` | Concepts, `wire_size_of`, byte order, formatter, `custom`, `overload`. |
| `rbe/annotations.hpp` | Every annotation, `well_annotated`, `contains_annotation`. |
| `rbe/srl.hpp` | `serialize`, `try_serialize`. |
| `rbe/dsrl.hpp` | `deserialize`, `try_deserialize`, the strategy tags, `proxy`, `return_type`. |
| `rbe/frames.hpp` | Frames: vocabulary types, concepts, views, adaptors, `deserialize` for frames. |

## Core

| Header | Provides |
|---|---|
| `rbe/core/wirable_concepts.hpp` | `introspectable`, `wirable`, `wirable_class`, `wirable_range` |
| `rbe/core/trivially_wirable_concepts.hpp` | `trivially_wirable`, `trivially_wirable_class`, `trivially_wirable_range`, `is_trivially_wirable` |
| `rbe/core/wirable_primitives.hpp` | `trivially_wirable_primitive`, `custom_wirable`, `wirable_primitive` and their `is_*` predicates |
| `rbe/core/memory_layout.hpp` | `wire_size_of` |
| `rbe/core/endian.hpp` | `rbe::endian::order`, `to_native`, `native_to`, `load`, `store` |
| `rbe/core/fmt.hpp` | The `std::formatter` and `operator<<` for `=rbe::fmt` types |
| `rbe/core/custom.hpp` | `rbe::custom<T>`, the primary template of the planned customization point |
| `rbe/core/overload_set.hpp` | `rbe::overload` |

## Annotations

| Header | Provides |
|---|---|
| `rbe/annotations/endianness.hpp` | `order`, `little`, `big` |
| `rbe/annotations/bits.hpp` | `bits` |
| `rbe/annotations/alignment.hpp` | `alignment`, `alignment_mode`, `pack`, `align` |
| `rbe/annotations/id.hpp` | `id`, `identifiable`, `identifying`, `self_identifying`, `id_like`, `id_of`, `id_type_of` |
| `rbe/annotations/length.hpp` | `length`, `length_kind`, `self_length`, `header_length`, `frame_length`, `payload_length`, `payload_count` |
| `rbe/annotations/count.hpp` | `count` |
| `rbe/annotations/empty.hpp` | `empty`, `explicitly_empty` |
| `rbe/annotations/format.hpp` | `fmt` |
| `rbe/annotations/friendly.hpp` | `friendly` |
| `rbe/annotations/derive.hpp` | `derive`, `pack_le`, `pack_be`, `debug` |
| `rbe/annotations/annotation_concepts.hpp` | `well_annotated`, `contains_annotation` |

## Serialization

| Header | Provides |
|---|---|
| `rbe/srl/serialize.hpp` | `serialize`, `try_serialize` |

## Deserialization

| Header | Provides |
|---|---|
| `rbe/dsrl/deserialize.hpp` | `deserialize`, `try_deserialize` for wirable types |
| `rbe/dsrl/tags.hpp` | `eager`, `lazy`, `in_place`, `in_place_mut`, their types, `strategy` |
| `rbe/dsrl/proxy.hpp` | `rbe::dsrl::proxy` |
| `rbe/dsrl/return_type.hpp` | `rbe::dsrl::return_type` |

## Frames

| Header | Provides |
|---|---|
| `rbe/frames/frame.hpp` | `rbe::frames::frame` |
| `rbe/frames/any.hpp` | `rbe::frames::any` |
| `rbe/frames/many.hpp` | `rbe::frames::many` |
| `rbe/frames/blob.hpp` | `rbe::frames::blob` |
| `rbe/frames/deserialize.hpp` | `deserialize`, `try_deserialize` for frames |
| `rbe/frames/frame_concepts.hpp` | `is_frame`, `is_any`, `is_many`, `header`, `payload`, `wirable_payload`, `header_t`, `payload_t`, `many_element_t` |
| `rbe/frames/frame_serder_concepts.hpp` | `frame_serder`, `serder_payload`, `serder_traits`, `dsrl_t` |
| `rbe/frames/frame_delimiting_tags.hpp` | The delimiting tags, `delimiting_tag`, `declares_delimiting_concept` |
| `rbe/frames/frame_delimiting_concepts.hpp` | `self_delimited`, `buffer_delimited`, `explicitly_delimited`, `length_delimited`, `count_delimited`, `static_stride_delimited`, `implicitly_delimited`, `static_delimited`, `static_length`, `static_stride`, `dynamic_stride`, `delimiting_concept_t`, `frame_delimiting_tag` |
| `rbe/frames/length.hpp` | `rbe::frames::length`, `distance`, `constant_length` |
| `rbe/frames/value_type.hpp` | 🚧 `value_type`, a placeholder |
| `rbe/frames/dsrl/frame.hpp` | `rbe::frames::dsrl::frame` |
| `rbe/frames/dsrl/base_frame.hpp` | `rbe::frames::dsrl::base_frame`, tuple protocol for frames |
| `rbe/frames/dsrl/header.hpp` | `rbe::frames::dsrl::header` |
| `rbe/frames/dsrl/payload.hpp` | `payload_view_t`, `payload_extent`, `try_payload_extent`, `construct_payload`, `try_construct_payload`, `counted_extent` |
| `rbe/frames/dsrl/any.hpp` | `rbe::frames::dsrl::any` |
| `rbe/frames/dsrl/any_unmatched.hpp` | `rbe::unmatched` |
| `rbe/frames/dsrl/many.hpp` | `rbe::frames::dsrl::many` |
| `rbe/frames/dsrl/flatten.hpp` | `rbe::frames::dsrl::flatten` |
| `rbe/frames/dsrl/views.hpp` | `rbe::views::many`, `with_ids`, `known_ids`, `unknown_ids` |
| `rbe/frames/dsrl/concepts.hpp` | `rbe::frames::dsrl::is_frame`, `decomposable_frame`, `any_dispatcher` |
| `rbe/frames/srl/*.hpp` | 🚧 Frame serialization, placeholders |
