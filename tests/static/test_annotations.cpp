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
#include <rbe/core/annotations.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---

namespace {

// clang-format off
struct[[=rbe::little]] TestLittle{};
struct[[=rbe::big]]    TestBig{};
struct[[=rbe::pack]]   TestPack{};
struct[[=rbe::id]]     TestId{};
struct[[=rbe::length]] TestLength{};
struct[[=rbe::debug]]  TestDebug{};

inline constexpr struct {} annotation_a {};
inline constexpr struct {} annotation_b {};
struct [[=annotation_a]] AnnotatedStructA {
  int x;
  double y;
};

struct[[=annotation_a, =annotation_b]] AnnotatedStructB {
  int x;
  double y;
};

[[=annotation_a]] struct WrongAnnotatedStruct {
  int x;
  double y;
};

//clang-format on

static_assert(rbe::detail::is_annotation_list(^^rbe::debug));

static_assert(rbe::detail::has_annotation(^^TestLittle, rbe::little));
static_assert(rbe::detail::has_annotation(^^TestBig, rbe::big));
static_assert(rbe::detail::has_annotation(^^TestPack, rbe::pack));
static_assert(rbe::detail::has_annotation(^^TestId, rbe::id));
static_assert(rbe::detail::has_annotation(^^TestLength, rbe::length));
static_assert(rbe::detail::has_annotation(^^TestDebug, rbe::debug));
static_assert(rbe::detail::has_annotation(^^TestDebug, rbe::fmt));

static_assert(rbe::detail::has_annotation(^^AnnotatedStructA, annotation_a));
static_assert(not rbe::detail::has_annotation(^^AnnotatedStructA, annotation_b));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructB, annotation_a));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructB, annotation_b));
static_assert(not rbe::detail::has_annotation(^^WrongAnnotatedStruct, annotation_a));

} // namespace
