/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file introspection.cpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---

// --- Dependencies ---
#include <rbe/detail/introspection.hpp>

// --- External dependencies ---

// --- STD ---
#include <meta>

// --- System ---

namespace {

// ============================================================
// Test types
// ============================================================

struct EmptyStruct { };

struct TestStruct {
  int a;
  double b;
  char c;
};

struct SingleMember {
  float x;
};

struct WithStatic {
  int x;
  static int s;
  double y;
};

struct Inner {
  int val;
  float coeff;
};

struct Outer {
  Inner inner;
  int id;
  bool flag;
};

class AccessClass {
public:
  int pub;

protected:
  double prot;

private:
  char priv;
};

struct ManyMembers {
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
  int g;
  int h;
};

inline constexpr struct {
} annotation_a {};

inline constexpr struct {
} annotation_b {};

struct[[= annotation_a]] AnnotatedStructA {
  int x;
  double y;
};

struct[[ = annotation_a, = annotation_b ]] AnnotatedStructB {
  int x;
  double y;
};

[[= annotation_a]] struct WrongAnnotatedStruct {
  int x;
  double y;
};


// ============================================================
// Helpers
// ============================================================

consteval bool test_nsdm_name(std::meta::info info, std::string_view identifier, std::size_t expected_index) try {
  return rbe::detail::nsdm(info, identifier) ==
         nonstatic_data_members_of(info, rbe::detail::default_context)[expected_index];
}
catch (std::meta::exception const& e) {
  return false;
}

consteval bool test_nsdm_idx(std::meta::info info, std::size_t input_index, std::size_t expected_index) try {
  return rbe::detail::nsdm(info, input_index) ==
         nonstatic_data_members_of(info, rbe::detail::default_context)[expected_index];
}
catch (std::meta::exception const& e) {
  return false;
}

consteval bool test_nsdm_index(std::meta::info info, std::string_view identifier, std::size_t expected_index) try {
  return rbe::detail::nsdm_index(info, identifier) == expected_index;
}
catch (std::meta::exception const& e) {
  return false;
}


// ============================================================
// TestStruct
// ============================================================

static_assert(
    rbe::detail::nsdm(^^TestStruct) == nonstatic_data_members_of(^^TestStruct, std::meta::access_context::unchecked())
);

static_assert(test_nsdm_name(^^TestStruct, "a", 0));
static_assert(test_nsdm_name(^^TestStruct, "b", 1));
static_assert(test_nsdm_name(^^TestStruct, "c", 2));
static_assert(not test_nsdm_name(^^TestStruct, "d", 0)); // No such member

static_assert(test_nsdm_idx(^^TestStruct, 0, 0));
static_assert(test_nsdm_idx(^^TestStruct, 1, 1));
static_assert(test_nsdm_idx(^^TestStruct, 2, 2));
static_assert(not test_nsdm_idx(^^TestStruct, 3, 0)); // Out of bounds

static_assert(test_nsdm_index(^^TestStruct, "a", 0));
static_assert(test_nsdm_index(^^TestStruct, "b", 1));
static_assert(test_nsdm_index(^^TestStruct, "c", 2));
static_assert(not test_nsdm_index(^^TestStruct, "d", 0)); // No such member

// ============================================================
// EmptyStruct
// ============================================================

static_assert(rbe::detail::nsdm(^^EmptyStruct).empty());
static_assert(rbe::detail::nsdm(^^EmptyStruct).size() == 0);

static_assert(not test_nsdm_name(^^EmptyStruct, "a", 0));
static_assert(not test_nsdm_idx(^^EmptyStruct, 0, 0)); // Out of bounds
static_assert(not test_nsdm_index(^^EmptyStruct, "d", 0)); // No such member

// ============================================================
// SingleMember
// ============================================================

static_assert(rbe::detail::nsdm(^^SingleMember).size() == 1);
static_assert(test_nsdm_name(^^SingleMember, "x", 0));
static_assert(not test_nsdm_name(^^SingleMember, "y", 0)); // No such member
static_assert(test_nsdm_idx(^^SingleMember, 0, 0));
static_assert(not test_nsdm_idx(^^SingleMember, 1, 0)); // Out of bounds

// ============================================================
// WithStatic — static members must NOT appear in NSDM
// ============================================================

static_assert(rbe::detail::nsdm(^^WithStatic).size() == 2); // x and y, not s
static_assert(test_nsdm_name(^^WithStatic, "x", 0));
static_assert(test_nsdm_name(^^WithStatic, "y", 1));
static_assert(not test_nsdm_name(^^WithStatic, "s", 0)); // static member excluded

// ============================================================
// Outer — nested struct as member type
// ============================================================

static_assert(rbe::detail::nsdm(^^Outer).size() == 3);
static_assert(test_nsdm_name(^^Outer, "inner", 0));
static_assert(test_nsdm_name(^^Outer, "id", 1));
static_assert(test_nsdm_name(^^Outer, "flag", 2));
static_assert(test_nsdm_idx(^^Outer, 0, 0));
static_assert(test_nsdm_idx(^^Outer, 1, 1));
static_assert(test_nsdm_idx(^^Outer, 2, 2));
static_assert(not test_nsdm_idx(^^Outer, 3, 0)); // Out of bounds

static_assert(test_nsdm_index(^^Outer, "inner", 0));
static_assert(test_nsdm_index(^^Outer, "id", 1));
static_assert(test_nsdm_index(^^Outer, "flag", 2));

// ============================================================
// AccessClass — pub/prot/priv all visible under unchecked()
// ============================================================

static_assert(rbe::detail::nsdm(^^AccessClass).size() == 3);
static_assert(test_nsdm_name(^^AccessClass, "pub", 0));
static_assert(test_nsdm_name(^^AccessClass, "prot", 1));
static_assert(test_nsdm_name(^^AccessClass, "priv", 2));
static_assert(test_nsdm_idx(^^AccessClass, 0, 0));
static_assert(test_nsdm_idx(^^AccessClass, 1, 1));
static_assert(test_nsdm_idx(^^AccessClass, 2, 2));
static_assert(not test_nsdm_idx(^^AccessClass, 3, 0)); // Out of bounds

static_assert(test_nsdm_index(^^AccessClass, "pub", 0));
static_assert(test_nsdm_index(^^AccessClass, "prot", 1));
static_assert(test_nsdm_index(^^AccessClass, "priv", 2));

// Current access context is public, so only public members should be visible
static_assert(rbe::detail::nsdm(^^AccessClass, std::meta::access_context::current()).size() == 1);

// ============================================================
// ManyMembers — boundary / last-index exhaustion
// ============================================================

static_assert(rbe::detail::nsdm(^^ManyMembers).size() == 8);
static_assert(test_nsdm_name(^^ManyMembers, "a", 0));
static_assert(test_nsdm_name(^^ManyMembers, "h", 7));
static_assert(test_nsdm_idx(^^ManyMembers, 0, 0));
static_assert(test_nsdm_idx(^^ManyMembers, 7, 7));
static_assert(not test_nsdm_idx(^^ManyMembers, 8, 0)); // Out of bounds

static_assert(test_nsdm_index(^^ManyMembers, "a", 0));
static_assert(test_nsdm_index(^^ManyMembers, "h", 7));
static_assert(not test_nsdm_index(^^ManyMembers, "z", 0)); // No such member


// --- Annotation tests ---
static_assert(rbe::detail::has_annotation(^^AnnotatedStructA, annotation_a));
static_assert(not rbe::detail::has_annotation(^^AnnotatedStructA, annotation_b));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructB, annotation_a));
static_assert(rbe::detail::has_annotation(^^AnnotatedStructB, annotation_b));
static_assert(not rbe::detail::has_annotation(^^WrongAnnotatedStruct, annotation_a));

} // namespace
