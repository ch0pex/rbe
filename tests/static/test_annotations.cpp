/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_annotations.cpp
 * @date 01/07/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---
#include "common_structs.hpp"

// --- Dependencies ---

#include <rbe/annotations/alignment.hpp>
#include <rbe/annotations/derive.hpp>
#include <rbe/annotations/detail/annotations_correctness.hpp>
#include <rbe/annotations/endianness.hpp>
#include <rbe/annotations/format.hpp>
#include <rbe/annotations/metadata.hpp>
#include <rbe/annotations/well_annotated_concepts.hpp>

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace {

// clang-format off

// --- is_rbe_annotation ---
static_assert(std::ranges::all_of(rbe::detail::annotations::all, rbe::detail::is_rbe_annotation));
static_assert(rbe::detail::is_rbe_annotation(^^annotation_a));
static_assert(not rbe::detail::is_rbe_annotation(^^annotation_b));
static_assert(not rbe::detail::is_rbe_annotation(^^int));

// --- is_annotation_list ---
static_assert(rbe::detail::is_annotation_list(^^ rbe::debug));
static_assert(not rbe::detail::is_annotation_list(^^rbe::id));
static_assert(not rbe::detail::is_annotation_list(^^rbe::bits));

// --- has_annotation ---
static_assert(rbe::detail::has_annotation(^^TestLittle, rbe::little));
static_assert(rbe::detail::has_annotation(^^TestBig, rbe::big));
static_assert(rbe::detail::has_annotation(^^TestPack, rbe::pack));
static_assert(rbe::detail::has_annotation(^^TestId, rbe::id));
static_assert(rbe::detail::has_annotation(^^TestDebug, rbe::debug));
static_assert(rbe::detail::has_annotation(^^TestLength, rbe::length));
static_assert(rbe::detail::has_annotation(^^TestDebug, rbe::fmt));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructA, annotation_a));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructB, annotation_a));
static_assert(not rbe::detail::has_annotation(^^WrongAnnotatedStruct, annotation_a));

// --- views::rbe_annotations ---
inline constexpr std::array rbe_annotations = rbe::detail::types_list(^^annotation_a, ^^annotation_c);
inline constexpr std::array not_all_rbe_annotations = rbe::detail::types_list(^^annotation_a, ^^annotation_b, ^^annotation_c);
static_assert(std::ranges::equal(rbe::detail::views::rbe_annotations(rbe_annotations), rbe_annotations));
static_assert(std::ranges::equal(rbe::detail::views::rbe_annotations(not_all_rbe_annotations), rbe_annotations));

// --- annotations_types_of ---
static_assert(rbe::detail::annotation_types_of(^^AnnotatedStructA).size() == 1);
static_assert(std::ranges::equal(rbe::detail::annotation_types_of(^^AnnotatedStructA), rbe::detail::types_list(^^annotation_a)));
static_assert(rbe::detail::annotation_types_of(^^AnnotatedStructB).size() == 1);
static_assert(std::ranges::equal(rbe::detail::annotation_types_of(^^AnnotatedStructB), rbe::detail::types_list(^^annotation_a))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::annotation_types_of(^^AnnotatedStructC).size() == 2);
static_assert(std::ranges::equal(rbe::detail::annotation_types_of(^^AnnotatedStructC), rbe::detail::types_list(^^annotation_a, ^^annotation_c))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::annotation_types_of(^^WrongAnnotatedStruct).size() == 0);
static_assert(std::ranges::equal(rbe::detail::annotation_types_of(^^WrongAnnotatedStruct), std::vector<std::meta::info>{}));
static_assert(rbe::detail::annotation_types_of(^^Parent::child).size() == 2);
static_assert(std::ranges::equal(rbe::detail::annotation_types_of(^^Parent::child), rbe::detail::types_list(^^rbe::pack, ^^rbe::big)));

// --- deep_annotations_types_of ---
static_assert(rbe::detail::deep_annotation_types_of(^^AnnotatedStructA).size() == 1);
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^AnnotatedStructA), rbe::detail::types_list(^^annotation_a)));
static_assert(rbe::detail::deep_annotation_types_of(^^AnnotatedStructB).size() == 1);
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^AnnotatedStructB), rbe::detail::types_list(^^annotation_a))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::deep_annotation_types_of(^^AnnotatedStructC).size() == 2);
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^AnnotatedStructC), rbe::detail::types_list(^^annotation_a, ^^annotation_c))); // Annotation B is ignored, not an rbe annotation
static_assert(rbe::detail::deep_annotation_types_of(^^WrongAnnotatedStruct).size() == 0);
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^WrongAnnotatedStruct), std::vector<std::meta::info>{}));
static_assert(rbe::detail::deep_annotation_types_of(^^AnnotatedStructD).size() == 3);
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^AnnotatedStructD), rbe::detail::types_list(^^annotation_a, ^^annotation_c, ^^annotation_a)));
static_assert(std::ranges::equal(rbe::detail::deep_annotation_types_of(^^AnnotatedStructD), rbe::detail::deep_annotation_types_of(^^AnnotatedStructWithList)));

// --- well_annotated ---

static_assert(rbe::well_annotated<Child>);
static_assert(rbe::well_annotated<Parent>);
static_assert(not rbe::well_annotated<DuplicatedAnnotations>);
static_assert(not rbe::well_annotated<ConflictingAnnotations>);
static_assert(not rbe::well_annotated<BadParent>);

} // namespace
