/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_annotations.cpp
 * @date 01/07/2026
 * @brief Static assertions for annotation detection, dimension checks and well_annotated
 */

// --- Includes ---
#include "common_structs.hpp"
#include "rbe/annotations/detail/utils.hpp"

#include <concepts>
#include <rbe/annotations/detail/annotated_nsdm.hpp>
#include <rbe/annotations/detail/annotation.hpp>

#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/annotation_concepts.hpp>
#include <rbe/annotations/bits.hpp>
#include <rbe/annotations/derive.hpp>
#include <rbe/annotations/detail/correctness.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/format.hpp>
#include <rbe/annotations/id.hpp>
#include <rbe/annotations/length.hpp>
#include <rbe/core/detail/throw_check.hpp>

// --- STD ---
#include <array>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {

// clang-format off

// --- is_rbe_annotation ---
static_assert(rbe::detail::is_rbe_annotation(^^rbe::pack));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::align));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::little));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::big));
static_assert(rbe::detail::is_rbe_annotation(^^decltype(rbe::bits(3, 0))));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::id));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::frame_length));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::payload_length));
static_assert(rbe::detail::is_rbe_annotation(^^rbe::header_length));
static_assert(rbe::detail::is_rbe_annotation(^^decltype(rbe::id(test_msg_type_t::heartbeat))));
static_assert(rbe::detail::is_rbe_annotation(^^annotation_a));
static_assert(not rbe::detail::is_rbe_annotation(^^annotation_b));
static_assert(not rbe::detail::is_rbe_annotation(^^int));
// a bare factory is not an annotation: only the value it produces is (rbe::id opts into both forms)
static_assert(not rbe::detail::is_rbe_annotation(^^rbe::order));
static_assert(not rbe::detail::is_rbe_annotation(^^rbe::alignment));
static_assert(not rbe::detail::is_rbe_annotation(^^rbe::length));
static_assert(not rbe::detail::is_rbe_annotation(^^rbe::bits));

// --- is_annotation_list ---
static_assert(rbe::detail::is_annotation_list(^^ rbe::debug));
static_assert(not rbe::detail::is_annotation_list(^^rbe::id));
static_assert(not rbe::detail::is_annotation_list(^^rbe::bits));

// --- is_unique_annotation ---
static_assert(rbe::detail::unique_annotation<decltype(rbe::id)>);
static_assert(rbe::detail::unique_annotation<decltype(rbe::header_length)>);
static_assert(rbe::detail::unique_annotation<decltype(rbe::payload_length)>);
static_assert(rbe::detail::unique_annotation<decltype(rbe::frame_length)>);
static_assert(not rbe::detail::unique_annotation<decltype(rbe::big)>);
static_assert(not rbe::detail::unique_annotation<decltype(rbe::little)>);

// --- has_annotations ---
static_assert(rbe::detail::has_annotations(^^TestLittle, rbe::little));
static_assert(rbe::detail::has_annotations(^^TestBig, rbe::big));
static_assert(rbe::detail::has_annotations(^^TestPack, rbe::pack));
static_assert(rbe::detail::has_annotations(^^TestId, rbe::id));
static_assert(rbe::detail::has_annotations(^^TestDebug, rbe::debug));
static_assert(rbe::detail::has_annotations(^^TestLength, rbe::frame_length));
static_assert(rbe::detail::has_annotations(^^TestDebug, rbe::fmt));
static_assert(rbe::detail::has_annotations(^^AnnotatedStructA, annotation_a));
static_assert(rbe::detail::has_annotations(^^AnnotatedStructB, annotation_a));
static_assert(not rbe::detail::has_annotations(^^WrongAnnotatedStruct, annotation_a));
static_assert(rbe::detail::has_annotations(^^NestedParent, rbe::big));
static_assert(not rbe::detail::has_annotations(^^NestedParent, rbe::frame_length));
static_assert(not rbe::detail::has_annotations(^^NestedParent, rbe::id));
static_assert(not rbe::detail::has_annotations(^^NestedParent, rbe::header_length));
// --- has_annotations_deep ---
static_assert(rbe::detail::has_annotations_deep(^^NestedParent, rbe::big));
static_assert(rbe::detail::has_annotations_deep(^^NestedParent, rbe::frame_length));
static_assert(rbe::detail::has_annotations_deep(^^NestedParent, rbe::id));
static_assert(rbe::detail::has_annotations_deep(^^NestedParent, rbe::header_length));
static_assert(not rbe::detail::has_annotations_deep(^^NestedParent, rbe::payload_length));

// --- contains_annotations ---
static_assert(rbe::contains_annotation<NestedParent, rbe::big>);
static_assert(rbe::contains_annotation<NestedParent, rbe::frame_length>);
static_assert(rbe::contains_annotation<NestedParent, rbe::id>);
static_assert(rbe::contains_annotation<NestedParent, rbe::header_length>);
static_assert(not rbe::contains_annotation<NestedParent, rbe::payload_length>);

// --- views::annotations ---
/// the expected range -- NOT named `annotations_of`: ADL on std::meta::info would pick
/// std::meta::annotations_of instead
consteval auto expected(auto const... anns) -> std::vector<rbe::detail::annotation_info> {
  return {rbe::detail::annotation_info {anns}...};
}

inline constexpr std::array raw_annotations = std::array {^^annotation_a, ^^annotation_c};
inline constexpr std::array raw_with_non_rbe = std::array {^^annotation_a, ^^annotation_b, ^^annotation_c};
static_assert(std::ranges::equal(rbe::detail::views::annotations(raw_annotations), expected(^^annotation_a, ^^annotation_c)));
static_assert(std::ranges::equal(rbe::detail::views::annotations(raw_with_non_rbe), expected(^^annotation_a, ^^annotation_c)));
// a derive<...> list expands to the annotations it carries, values preserved
static_assert(std::ranges::equal(rbe::detail::views::expand_annotation(^^rbe::pack_be), expected(^^rbe::pack, ^^rbe::big)));

// --- annotation_range ---
static_assert(rbe::detail::annotation_range(^^AnnotatedStructA).size() == 1);
static_assert(std::ranges::equal(rbe::detail::annotation_range(^^AnnotatedStructA), expected(^^annotation_a)));
static_assert(rbe::detail::annotation_range(^^AnnotatedStructB).size() == 1);
static_assert(std::ranges::equal(rbe::detail::annotation_range(^^AnnotatedStructB), expected(^^annotation_a))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::annotation_range(^^AnnotatedStructC).size() == 2);
static_assert(std::ranges::equal(rbe::detail::annotation_range(^^AnnotatedStructC), expected(^^annotation_a, ^^annotation_c))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::annotation_range(^^WrongAnnotatedStruct).size() == 0);
static_assert(std::ranges::equal(rbe::detail::annotation_range(^^WrongAnnotatedStruct), std::vector<rbe::detail::annotation_info>{}));
static_assert(rbe::detail::annotation_range(^^Parent::child).size() == 2);
static_assert(std::ranges::equal(rbe::detail::annotation_range(^^Parent::child), expected(^^rbe::pack, ^^rbe::big)));

// --- deep_annotations ---
static_assert(rbe::detail::deep_annotations(^^AnnotatedStructA).size() == 1);
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^AnnotatedStructA), expected(^^annotation_a)));
static_assert(rbe::detail::deep_annotations(^^AnnotatedStructB).size() == 1);
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^AnnotatedStructB), expected(^^annotation_a))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::deep_annotations(^^AnnotatedStructC).size() == 2);
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^AnnotatedStructC), expected(^^annotation_a, ^^annotation_c))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::deep_annotations(^^WrongAnnotatedStruct).size() == 0);
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^WrongAnnotatedStruct), std::vector<rbe::detail::annotation_info>{}));
static_assert(rbe::detail::deep_annotations(^^AnnotatedStructD).size() == 3);
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^AnnotatedStructD), expected(^^annotation_a, ^^annotation_c, ^^annotation_a)));
static_assert(std::ranges::equal(rbe::detail::deep_annotations(^^AnnotatedStructD), rbe::detail::deep_annotations(^^AnnotatedStructWithList)));
static_assert(rbe::detail::deep_annotations(^^LengthAnnotatedTwiceNested).size() == 2); // recursion descends past the first level of members

// --- well_annotated ---

static_assert(rbe::well_annotated<Child>);
static_assert(rbe::well_annotated<Parent>);
static_assert(not rbe::well_annotated<ConflictingAnnotations>);
static_assert(not rbe::well_annotated<DuplicatedAnnotations>);
static_assert(not rbe::well_annotated<BadParent>);

// --- Length annotation correctness ---

static_assert(rbe::well_annotated<AddOrder>); // length annotation is correct
static_assert(not rbe::well_annotated<LengthAnnotatedTwice>); // length is annotated twice, fails dimension check
static_assert(not rbe::well_annotated<LengthAnnotatedTwiceNested>); // uniqueness is global: the duplicate sits two levels down
static_assert(not rbe::well_annotated<LengthNotConvertible>); // length is annotated on a non covertible type to std::size_t, fails check
static_assert(rbe::well_annotated<AllLengths>); // the three lengths are independent, one per field
static_assert(not rbe::well_annotated<LengthsInOneRange>); // two lengths in one annotation range, fails dimension check

// --- id annotation correctness ---
static_assert(rbe::well_annotated<AddOrder>); // id annotation is correct
static_assert(not rbe::well_annotated<IdAnnotatedTwice>); // id is annotated twice, fails dimension check
static_assert(not rbe::well_annotated<IdAnnotatedTwiceNested>); // id is annotated twice in a nested type, fails dimension check
static_assert(not rbe::well_annotated<IdNotEqualityComparable>); // id is annotated on a non equality comparable type, fails check

// --- id(value) annotation correctness ---
static_assert(rbe::well_annotated<MsgWithIdValue>); // id(value) on the type, marker on the nested header
static_assert(not rbe::well_annotated<IdValueAndMarker>); // marker and value share an annotation range, fails dimension check
static_assert(not rbe::well_annotated<TwoIdValues>); // two id values in one annotation range, fails dimension check
static_assert(not rbe::well_annotated<IdValueOnScalar>); // id(value) on a non class type, fails check
static_assert(not rbe::well_annotated<TwoIdValuesNested>); // one id per message whatever its value, fails uniqueness
                                                         //

// --- annotated_nsdm ---
static_assert(rbe::detail::annotated_nsdm(^^AllLengths, rbe::frame_length) == ^^AllLengths::frame);
static_assert(rbe::detail::annotated_nsdm(^^AllLengths, rbe::payload_length) == ^^AllLengths::payload);
static_assert(rbe::detail::annotated_nsdm(^^AllLengths, rbe::header_length) == ^^AllLengths::header);
static_assert(rbe::detail::annotated_nsdm(^^AllLengths, rbe::id) == std::nullopt);

// --- bits annotation correctness ---
static_assert(rbe::well_annotated<BitsField>);
static_assert(not rbe::well_annotated<BitsWiderThanField>); // the range does not fit the annotated field
static_assert(not rbe::well_annotated<BitsOnNonIntegral>); // a bit range needs an integral field
static_assert(not rbe::well_annotated<BitsAndEndianness>); // bits joins the endianness dimension

// a bit range bounds itself by the widest field RBE supports; the tag's check does the rest
static_assert(rbe::detail::no_throw([] { return rbe::bits(63, 0); }));
static_assert(not rbe::detail::no_throw([] { return rbe::bits(64, 0); })); // past the highest bit there is
static_assert(not rbe::detail::no_throw([] { return rbe::bits(-1, 0); })); // would wrap around to bit 255
static_assert(not rbe::detail::no_throw([] { return rbe::bits(0, 3); })); // inverted range

// --- annotation_info ---
inline constexpr auto big_info = rbe::detail::annotation_info {^^rbe::big};
static_assert(big_info.reflection() == ^^rbe::big);
static_assert(big_info.type() == rbe::detail::normalize_type(^^decltype(rbe::big)));
static_assert(big_info.tag() == ^^rbe::detail::order_tag);
static_assert(big_info.dimension() == ^^rbe::detail::endianness_dim);
static_assert(big_info.value_type() == dealias(^^rbe::endian::order)); // rbe::endian::order is an alias of std::endian
static_assert(big_info.has_value());
static_assert(big_info.value<rbe::endian::order>() == rbe::endian::order::big);
static_assert(big_info.value<rbe::alignment_mode>() == std::nullopt); // right annotation, wrong value type

// fmt marker carries nothing and belongs to no dimension
inline constexpr auto fmt_info = rbe::detail::annotation_info {^^rbe::fmt};
static_assert(not fmt_info.has_value());
static_assert(fmt_info.tag() == ^^rbe::detail::fmt_tag);
static_assert(fmt_info.dimension() == std::meta::info {});
static_assert(fmt_info.value_type() == std::meta::info {});
static_assert(fmt_info.value<int>() == std::nullopt);

// id marker carries nothing and but belongs to id_dim dimension
inline constexpr auto id_info = rbe::detail::annotation_info {^^rbe::id};
static_assert(not id_info.has_value());
static_assert(id_info.tag() == ^^rbe::detail::id_tag);
static_assert(id_info.dimension() == ^^rbe::detail::id_dim);
static_assert(id_info.value_type() == std::meta::info {});
static_assert(id_info.value<int>() == std::nullopt);

// is(): the needle's type is known statically, so the comparison never reflects it
static_assert(big_info.is(rbe::big));
static_assert(not big_info.is(rbe::little)); // same type, different value
static_assert(not big_info.is(rbe::pack)); // different type
static_assert(rbe::detail::annotation_info {^^rbe::frame_length}.is(rbe::frame_length));
static_assert(not rbe::detail::annotation_info {^^rbe::frame_length}.is(rbe::payload_length));

// operator==: neither type is known statically, the comparison is still by VALUE
inline constexpr auto big_again = rbe::order(rbe::endian::order::big);
static_assert(^^rbe::big != ^^big_again); // the two reflections differ ...
static_assert(rbe::detail::annotation_info {^^rbe::big} == rbe::detail::annotation_info {^^big_again}); // ... the annotations do not
static_assert(not(big_info == rbe::detail::annotation_info {^^rbe::little}));
static_assert(not(big_info == fmt_info));

// identity_equals: how `unique` counts repetitions
inline constexpr auto id_one = rbe::id(1);
inline constexpr auto id_two = rbe::id(2);
static_assert(not(rbe::detail::annotation_info {^^id_one} == rbe::detail::annotation_info {^^id_two}));
static_assert(rbe::detail::annotation_info {^^id_one}.identity_equals(rbe::detail::annotation_info {^^id_two})); // id_tag: by kind
static_assert(not rbe::detail::annotation_info {^^rbe::frame_length}.identity_equals(
    rbe::detail::annotation_info {^^rbe::payload_length})); // length_tag: by value

// only an actual annotation can be wrapped
static_assert(not rbe::detail::no_throw([](std::meta::info const i) { return rbe::detail::annotation_info {i}; }, ^^int));
static_assert(not rbe::detail::no_throw([](std::meta::info const i) { return rbe::detail::annotation_info {i}; }, ^^rbe::debug)); // a list is not one annotation
static_assert(rbe::detail::no_throw([](std::meta::info const i) { return rbe::detail::annotation_info {i}; }, ^^rbe::big));

// --- by_dimension ---
static_assert(std::ranges::count_if(rbe::detail::annotation_range(^^Parent::child), rbe::detail::by_dimension(^^rbe::detail::endianness_dim)) == 1);
static_assert(std::ranges::count_if(rbe::detail::annotation_range(^^Parent::child), rbe::detail::by_dimension(^^rbe::detail::length_dim)) == 0);

// --- find_annotation: the reverse direction, the annotation itself instead of a yes/no ---
static_assert(rbe::detail::find_annotation(^^AllLengths::frame, ^^rbe::detail::length_tag)->is(rbe::frame_length));
static_assert(rbe::detail::find_annotation(^^AllLengths::payload, ^^rbe::detail::length_tag)->is(rbe::payload_length));
static_assert(rbe::detail::find_annotation(^^AllLengths::frame, ^^rbe::detail::id_tag) == std::nullopt);
static_assert(rbe::detail::find_annotation(^^AllLengths, ^^rbe::detail::length_tag) == std::nullopt); // the members carry them, not the type
static_assert(rbe::detail::find_annotation_deep(^^AllLengths, ^^rbe::detail::length_tag)->is(rbe::frame_length));

// this is what dispatch needs: the id a message type is declared under, and its type
static_assert(rbe::detail::find_annotation(^^MsgWithIdValue, ^^rbe::detail::id_tag).has_value());
static_assert(rbe::detail::find_annotation(^^MsgWithIdValue, ^^rbe::detail::id_tag)->value_type() == ^^test_msg_type_t);
static_assert(rbe::detail::find_annotation(^^MsgWithIdValue, ^^rbe::detail::id_tag)->value<test_msg_type_t>() == test_msg_type_t::heartbeat);

// --- resolve_in_scope ---
static_assert(rbe::detail::resolve_in_scope<rbe::endian::order>(^^TestBig) == rbe::endian::order::big);
static_assert(rbe::detail::resolve_in_scope<rbe::endian::order>(^^TestLittle) == rbe::endian::order::little);
static_assert(rbe::detail::resolve_in_scope<rbe::endian::order>(^^TestPack) == std::nullopt);
static_assert(rbe::detail::resolve_in_scope<rbe::alignment_mode>(^^TestPack) == rbe::alignment_mode::pack);
static_assert(rbe::detail::resolve_in_scope<rbe::endian::order>(^^Parent::child) == rbe::endian::order::big);
static_assert(rbe::detail::resolve_in_scope<rbe::alignment_mode>(^^Parent::child) == rbe::alignment_mode::pack);


// --- identifiable: the type declares the id it answers to ---
static_assert(rbe::identifiable<MsgWithIdValue>);
static_assert(rbe::identifiable<MsgWithInlineId>);
static_assert(rbe::identifiable<MsgWithIntId>);
static_assert(not rbe::identifiable<IdHeader>); // carries the field, declares no value of its own
static_assert(not rbe::identifiable<PlainRecord>);
static_assert(not rbe::identifiable<IdValueOnScalar>); // the value sits on a member, not on the type

// --- identifying: a field of the type's own carries the id ---
static_assert(rbe::identifying<IdHeader>);
static_assert(rbe::identifying<MsgWithInlineId>);
static_assert(not rbe::identifying<MsgWithIdValue>); // the marker is nested one level down, not its own
static_assert(not rbe::identifying<MsgWithIntId>);
static_assert(not rbe::identifying<PlainRecord>);

// --- self_identifying: both halves, so nothing else is needed to recognize it ---
static_assert(rbe::self_identifying<MsgWithInlineId>);
static_assert(not rbe::self_identifying<MsgWithIdValue>); // declares its id, but composes the field
static_assert(not rbe::self_identifying<IdHeader>); // carries the field, but answers to no id
static_assert(not rbe::self_identifying<PlainRecord>);

// --- id_of: the declared value, with the type it was written as ---
static_assert(rbe::id_of<MsgWithIdValue>() == test_msg_type_t::heartbeat);
static_assert(rbe::id_of<MsgWithInlineId>() == test_msg_type_t::add_order);
static_assert(std::same_as<decltype(rbe::id_of<MsgWithIdValue>()), test_msg_type_t>);
static_assert(rbe::id_of<MsgWithIntId>() == 7); // deduced: an id need not be an enum
static_assert(std::same_as<decltype(rbe::id_of<MsgWithIntId>()), int>);

template<typename T>
concept id_of_callable = requires { rbe::id_of<T>(); };

static_assert(id_of_callable<MsgWithIdValue>);
static_assert(not id_of_callable<IdHeader>);
static_assert(not id_of_callable<PlainRecord>);

// --- id_type_of: the type the id was written as, with the value it was written as ---
static_assert(std::same_as<rbe::id_type<MsgWithIdValue>, test_msg_type_t>);
static_assert(std::same_as<rbe::id_type<MsgWithInlineId>, test_msg_type_t>);
static_assert(std::same_as<rbe::id_type<MsgWithIntId>, int>);

} // namespace
