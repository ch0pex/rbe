/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
// rbe 0.0.1: single header, auto-generated from the library headers (79 headers).

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <meta>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <version>

// ===== rbe/rbe.hpp =====

/**
 * @file rbe.hpp
 * @date 24/06/2026
 * @brief RBE Umbrella header file
 */



// ===== rbe/annotations.hpp =====

/**
 * @file annotations.hpp
 * @date 20/08/2026
 * @brief annotations umbrella header
 */



// ===== rbe/annotations/alignment.hpp =====

/**
 * @file alignment.hpp
 * @date 20/08/2026
 * @brief Memory alignment annotations
 */


// --- Includes ---

// ===== rbe/annotations/detail/annotation.hpp =====

/**
 * @file annotation.hpp
 * @date 17/09/2026
 * @brief The one and only way an RBE annotation is built, and the predicates that recognize one
 *
 * Every RBE annotation is created the same way: a *tag* type declares
 * what the annotation means, `annotation_kind<Tag>` is the object the user spells, and calling it
 * produces an `annotation_value<Tag, T>`. Named annotations are plain aliases of such a call:
 *
 * @code
 * struct order_tag {
 *   using dimension  = endianness_dim;     // optional: the dimension it belongs to
 *   using value_type = endian::order;      // optional: absent means "marker only"
 * };
 *
 * inline constexpr detail::annotation_kind<order_tag> order {};   // [[=rbe::order(endian::order::big)]]
 * inline constexpr auto big = order(endian::order::big);          // [[=rbe::big]]
 * @endcode
 *
 * A tag may additionally declare:
 *   - `static constexpr bool marker = true;`          the bare factory object is an annotation too
 *                                                     (`rbe::id` alongside `rbe::id(value)`)
 *   - `static constexpr auto identity = identity_kind::kind;`   see identity_kind
 *   - `static consteval auto check(annotation_info, std::meta::info entity) -> bool;`
 *                                                     the annotation's own correctness rule
 *
 * Identity is structural: a type is an RBE annotation iff it is a specialization of the templates
 * below. There is no registry and no trait to specialize.
 */


// --- Includes ---

// ===== rbe/annotations/detail/dimension.hpp =====

/**
 * @file dimension.hpp
 * @brief Annotation dimensions: the orthogonal groups annotations belong to, and the correctness
 *        rules each group enforces
 */


// --- Includes ---

// ===== rbe/core/detail/introspection.hpp =====

/**
 * @file introspection.hpp
 * @date 24/06/2026
 * @brief Reflection helpers wrapping std::meta for non-static data member access
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe::detail {

inline constexpr auto default_context = std::meta::access_context::unchecked();

/**
 * @brief The plain, cv-unqualified, alias-free type denoted by `info` -- the type itself if `info` is
 * one, the type of the entity otherwise.
 *
 * `dealias` is not optional: `^^SomeAliasName` reflects the *alias*, and compares unequal to a
 * reflection of the type it names, so every type identity check in the library would silently fail for
 * a type spelled through an alias (e.g. `std::remove_cvref_t<decltype(x)>`).
 */
consteval auto normalize_type(std::meta::info const info) -> std::meta::info {
  return dealias(not is_type(info) ? remove_cvref(type_of(info)) : remove_cvref(info));
};

consteval auto nsdm(std::meta::info info, std::meta::access_context ctx = default_context) {
  return nonstatic_data_members_of(info, ctx);
}

consteval auto nsdm( //
  std::meta::info info,  //
  std::string_view const identifier,  //
  std::meta::access_context ctx = default_context  //
) {
  for (auto [idx, field]: nsdm(info, ctx) | std::views::enumerate) {
    if (has_identifier(field) and identifier_of(field) == identifier)
      return field;
  }

  // Reflecting overload sets is not supported yet, as a work arround to throw
  static constexpr auto nsdm_by_id = [] { };
  throw std::meta::exception("invalid member identifier, no such nonstatic data member", ^^nsdm_by_id);
}

consteval auto nsdm(
    std::meta::info const info, //
    std::size_t const index, //
    std::meta::access_context ctx = default_context //
) {
  if (auto const members = nsdm(info, ctx); index < members.size()) {
    return members[index];
  }

  static constexpr auto nsdm_by_index = [] { };
  throw std::meta::exception("invalid member index", ^^nsdm_by_index);
}

consteval auto nsdm_index( //
  std::meta::info const info,  //
  std::string_view const identifier, //
  std::meta::access_context ctx = default_context //
) -> std::size_t {
  for (auto [idx, field]: nsdm(info, ctx) | std::views::enumerate) {
    if (has_identifier(field) and identifier_of(field) == identifier)
      return static_cast<std::size_t>(idx);
  }
  static constexpr auto nsdm_index = [] { };
  throw std::meta::exception("invalid member, no such nonstatic data member", ^^nsdm_index);
}

consteval auto nsdm_index( //
  std::meta::info const info,  //
  std::meta::info const member,
  std::meta::access_context ctx = default_context //
) -> std::size_t {
  return nsdm_index(info, identifier_of(member), ctx);
}

consteval std::size_t nsdm_count(std::meta::info const info, std::meta::access_context ctx = default_context) {
  return nsdm(info, ctx).size();
}

consteval bool specialization_of(std::meta::info const info, std::meta::info const template_info) {
  if (not has_template_arguments(info)) {
    return false;
  }

  return template_of(info) == template_info;
}

consteval auto bases_of(std::meta::info info, std::meta::access_context ctx = default_context) {
  return std::meta::bases_of(info, ctx);
}

consteval auto static_member_functions_of(std::meta::info const info, std::meta::access_context ctx = default_context) {
  static constexpr auto is_static_member_function = [](std::meta::info const member) -> bool {
    return is_static_member(member) and is_function(member) and not is_special_member_function(member);
  };
  return members_of(info, ctx) | std::views::filter(is_static_member_function) | std::ranges::to<std::vector>();
}

consteval auto static_data_member(
    std::meta::info const info, //
    std::string_view const identifier, //
    std::meta::access_context ctx = default_context //
) -> std::optional<std::meta::info> {
  for (auto const member: static_data_members_of(info, ctx)) {
    if (has_identifier(member) and identifier_of(member) == identifier) {
      return member;
    }
  }
  return std::nullopt;
}

consteval auto static_member_function(
    std::meta::info const info, //
    std::string_view const identifier, //
    std::meta::access_context ctx = default_context //
) -> std::optional<std::meta::info> {
  for (auto const member: static_member_functions_of(info, ctx)) {
    if (has_identifier(member) and identifier_of(member) == identifier) {
      return member;
    }
  }
  return std::nullopt;
}

consteval auto member_aliases_of(std::meta::info const info, std::meta::access_context ctx = default_context)
    -> std::vector<std::meta::info> {
  return members_of(info, ctx) //
         | std::views::filter(std::meta::is_type_alias) //
         | std::ranges::to<std::vector>();
}

/// The member alias named `identifier` (NOT dealiased -- see `member_alias_of`), or nullopt.
consteval auto find_member_alias( //
    std::meta::info const info, //
    std::string_view const identifier, //
    std::meta::access_context ctx = default_context //
) -> std::optional<std::meta::info> {
  for (auto const alias: member_aliases_of(info, ctx)) {
    if (has_identifier(alias) and identifier_of(alias) == identifier) {
      return alias;
    }
  }
  return std::nullopt;
}

consteval auto member_alias_of( //
    std::meta::info const info,  //
    std::string_view const identifier,  //
    std::meta::access_context ctx = default_context //
) -> std::meta::info {
  if (auto const alias = find_member_alias(info, identifier, ctx)) {
    return *alias;
  }

  throw std::meta::exception("invalid member alias, no such member alias", ^^member_alias_of);
}


} // namespace rbe::detail

// --- STD ---

namespace rbe::detail {

/**
 * @brief The set of correctness rules a dimension enforces across an annotated type.
 *
 * The rules are independent and combinable with `|`: a dimension whose annotations are both mutually
 * exclusive locally and non-repeatable globally (e.g. the id dimension, where `id` and `id(value)`
 * may not share an annotation range and neither may repeat across the message) declares
 * `exclusive | unique`.
 */
enum class dimension_kind : std::uint8_t {
  exclusive = 1 << 0, ///< at most one annotation of the dimension may appear within a single annotation range
  unique    = 1 << 1, ///< each annotation of the dimension may independently appear at most once across the whole type
  type_only = 1 << 2, ///< annotation can only be applied to types
};

consteval auto operator|(dimension_kind const lhs, dimension_kind const rhs) -> dimension_kind {
  return static_cast<dimension_kind>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

/// Whether `kind` includes the given `rule`.
consteval auto enforces(dimension_kind const kind, dimension_kind const rule) -> bool {
  return (std::to_underlying(kind) & std::to_underlying(rule)) != 0;
}

/**
 * @brief What makes two annotations produced by the same factory "the same annotation" when counting
 * repetitions for `dimension_kind::unique`.
 *
 * `value` (the default) is the intuitive reading and the one annotations want whenever the factory's
 * values are alternatives rather than payloads: `rbe::frame_length` and `rbe::payload_length` are two
 * values of one annotation type, and each is independently unique. `kind` is for annotations whose
 * value is a payload rather than an alternative -- `rbe::id(1)` and `rbe::id(2)` are the same
 * annotation said twice, and a message carries one id whatever its value.
 */
enum class identity_kind : std::uint8_t { value, kind };

/// Reads the `static constexpr dimension_kind kind` member off a dimension tag type.
consteval auto kind_of(std::meta::info const dim) -> dimension_kind {
  if (auto const member = static_data_member(dim, "kind")) {
    return extract<dimension_kind>(*member);
  }
  throw std::meta::exception("dimension tag type is missing a `static constexpr dimension_kind kind` member", dim);
}

} // namespace rbe::detail

// --- STD ---

namespace rbe::detail {

/// Spelled as a tag's `value_type` when the annotation carries whatever type it is handed
/// (`rbe::id(42)` and `rbe::id(msg_type::heartbeat)` are both ids).
struct deduced { };

template<typename Tag>
concept has_value_type = requires { typename Tag::value_type; };

template<typename Tag>
concept deduced_value_tag = has_value_type<Tag> and std::same_as<typename Tag::value_type, deduced>;

template<typename Tag>
concept fixed_value_tag = has_value_type<Tag> and not deduced_value_tag<Tag>;

/**
 * @brief An annotation carrying a value, produced by calling an `annotation_kind`.
 *
 * `payload` and `equals` exist so that the value can be read back, and two annotations compared,
 * from a `std::meta::info` whose concrete type is not known statically -- they are reached
 * reflectively by `annotation_info`, never called directly.
 */
template<typename Tag, typename T>
struct annotation_value {
  using tag        = Tag;
  using value_type = T;

  T value;

  consteval explicit annotation_value(auto const... args) : value(T(args...)) { }

  consteval auto operator==(annotation_value const&) const -> bool = default;

  /// The value carried by `a`, which must reflect an annotation of this very type.
  static consteval auto payload(std::meta::info const a) -> T { return extract<annotation_value>(a).value; }

  /// Whether `a` and `b`, both annotations of this very type, carry the same value.
  static consteval auto equals(std::meta::info const a, std::meta::info const b) -> bool {
    return extract<annotation_value>(a) == extract<annotation_value>(b);
  }
};

/**
 * @brief The object an annotation is spelled with.
 *
 * Callable when `Tag` names a `value_type`; itself a valid annotation when `Tag` names no
 * `value_type` (a pure marker such as `rbe::fmt`) or opts in with `static constexpr bool marker`.
 */
template<typename Tag>
struct annotation_kind {
  using tag = Tag;

  consteval auto operator==(annotation_kind const&) const -> bool = default;

  consteval auto operator()(auto const... args) const
    requires fixed_value_tag<Tag>
  {
    return annotation_value<Tag, typename Tag::value_type> {args...};
  }

  consteval auto operator()(auto const arg) const
    requires deduced_value_tag<Tag>
  {
    return annotation_value<Tag, std::remove_cvref_t<decltype(arg)>> {arg};
  }
};

/// Groups several annotations into a single value -- see `rbe::derive`.
template<auto... Args>
struct annotations_t { };

// --- Tag queries ----------------------------------------------------------------------------------

/// The tag type of an annotation type, or a null reflection if it is not an annotation type.
consteval auto tag_of(std::meta::info const type) -> std::meta::info {
  auto const alias = find_member_alias(type, "tag");
  return alias ? dealias(*alias) : std::meta::info {};
}

/// The type of the value an annotation type carries, or a null reflection for a marker.
consteval auto value_type_of(std::meta::info const type) -> std::meta::info {
  auto const alias = find_member_alias(type, "value_type");
  return alias ? dealias(*alias) : std::meta::info {};
}

/// The dimension tag an annotation tag belongs to, or a null reflection if it belongs to none.
consteval auto dimension_of_tag(std::meta::info const tag) -> std::meta::info {
  if (tag == std::meta::info {}) {
    return std::meta::info {};
  }
  auto const alias = find_member_alias(tag, "dimension");
  return alias ? dealias(*alias) : std::meta::info {};
}

/// How repetitions of the annotations produced by `tag` are counted -- `identity_kind::value` unless
/// the tag says otherwise.
consteval auto identity_of_tag(std::meta::info const tag) -> identity_kind {
  if (tag == std::meta::info {}) {
    return identity_kind::value;
  }
  auto const member = static_data_member(tag, "identity");
  return member ? extract<identity_kind>(*member) : identity_kind::value;
}

/// The dimension an annotation type belongs to, or a null reflection if it belongs to none.
consteval auto dimension_of(std::meta::info const type) -> std::meta::info { return dimension_of_tag(tag_of(type)); }

/// Whether the bare `annotation_kind<Tag>` object is itself an annotation, and not just a factory.
consteval auto allows_marker(std::meta::info const tag) -> bool {
  if (tag == std::meta::info {}) {
    return false;
  }
  if (value_type_of(tag) == std::meta::info {}) {
    return true; // nothing to carry: the tag can only ever be a marker
  }
  auto const member = static_data_member(tag, "marker");
  return member and extract<bool>(*member);
}

// --- Identity -------------------------------------------------------------------------------------

consteval auto is_annotation_list(std::meta::info const info) -> bool {
  return specialization_of(normalize_type(info), ^^annotations_t);
}

/// An entity is a first-class RBE annotation iff it was built with the templates above.
consteval auto is_rbe_annotation(std::meta::info const info) -> bool {
  auto const type = normalize_type(info);
  if (is_annotation_list(type) or specialization_of(type, ^^annotation_value)) {
    return true;
  }
  return specialization_of(type, ^^annotation_kind) and allows_marker(tag_of(type));
}

template<typename T>
concept annotation = is_rbe_annotation(^^T) and not is_annotation_list(^^T);

template<typename T>
concept annotation_list = is_annotation_list(^^T);

template<typename T>
concept unique_annotation = annotation<T> //
                            and dimension_of(normalize_type(^^T)) != std::meta::info {} and
                            enforces(kind_of(dimension_of(normalize_type(^^T))), dimension_kind::unique);

} // namespace rbe::detail

// --- STD ---

namespace rbe {

namespace detail {

enum class alignment_mode : std::uint8_t { pack, align };

/**
 * At most one alignment-dimension annotation may appear within a single annotation range. The
 * implicit default (no explicit annotation anywhere in scope) is `native` -- regular padded layout.
 */
struct alignment_dim {
  static constexpr auto kind          = dimension_kind::exclusive;
  static constexpr auto default_value = alignment_mode::align;
};

/// Layout semantics: `rbe::alignment(alignment_mode::pack)`, or the `pack`/`align` aliases.
struct alignment_tag {
  using dimension  = alignment_dim;
  using value_type = alignment_mode;
};

} // namespace detail

/// The layout a field can be given on the wire: `pack` (no padding) or `align` (regular C++ layout).
using alignment_mode = detail::alignment_mode;

/**
 * @brief How the annotated struct or member is laid out on the wire.
 *
 * Written on a struct it applies to every member: a member with no alignment of its own inherits
 * it, and a member that states one replaces it for that field. What a member may not do is
 * contradict its own type -- annotate a member whose type already carries an alignment and the two
 * share a single annotation range, so they do not override, they conflict, and compilation fails.
 * An explicitly annotated struct keeps its layout wherever it is used.
 *
 * `pack`/`align` below are the two values named, and are what you normally write -- this spelling
 * is for passing an `alignment_mode` decided elsewhere.
 */
inline constexpr detail::annotation_kind<detail::alignment_tag> alignment {};
inline constexpr auto pack  = alignment(alignment_mode::pack);
inline constexpr auto align = alignment(alignment_mode::align);

} // namespace rbe

// ===== rbe/annotations/annotation_concepts.hpp =====

/**
 * @file annotation_concepts.hpp
 * @date 10/08/2026
 * @brief well_annotated concept verifying the annotation correctness of a type
 */


// --- Includes ---

// ===== rbe/annotations/detail/correctness.hpp =====

/**
 * @file correctness.hpp
 * @version 2.0
 * @date 15/08/2026
 * @brief Annotation dimension definitions and correctness checks used by well_annotated
 */


// ===== rbe/annotations/detail/utils.hpp =====

/**
 * @file utils.hpp
 * @version 2.0
 * @date 15/08/2026
 * @brief Helpers for gathering and querying the RBE annotations attached to a type or member
 *
 * Three ranges, each one a `std::vector<annotation_info>` -- the annotations written on an entity,
 * the annotation range the requirements define for it, and that same range recursively. Everything
 * else is an ordinary range algorithm over one of them.
 */

// --- Includes ---

// ===== rbe/annotations/detail/view.hpp =====

/**
 * @file view.hpp
 * @version 2.0
 * @date 15/08/2026
 * @brief Range adaptor that flattens annotations and annotation lists into a single view
 */

// --- Includes ---

// ===== rbe/annotations/detail/annotation_info.hpp =====

/**
 * @file annotation_info.hpp
 * @date 17/09/2026
 * @brief A single, whole RBE annotation: a strongly typed `std::meta::info`
 *
 * Every query in the annotation system deals in `annotation_info`, never in bare reflections and
 * never in annotation *types*: the wrapped reflection is always the annotation's VALUE, so the value
 * is never lost along the way and there is no second, parallel "values" API to keep in sync.
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

// NOTE: annotation_info is kept as a struct instead of a class
// because it's convinietn to keep it as an structural type so it
// can be used as a template parameter
struct annotation_info {
  /// @throws std::meta::exception if `info` does not reflect a single RBE annotation
  consteval explicit annotation_info(std::meta::info const info) : info_ {info} {
    if (is_annotation_list(info)) {
      throw std::meta::exception("an annotation list is not a single annotation, expand it first", info);
    }
    if (not is_rbe_annotation(info)) {
      throw std::meta::exception("not an rbe annotation", info);
    }
  }

  /// The wrapped reflection: the annotation's value, as written.
  [[nodiscard]] consteval auto reflection() const -> std::meta::info { return info_; }

  /// The annotation's type -- `annotation_kind<Tag>` for a marker, `annotation_value<Tag, T>` otherwise.
  [[nodiscard]] consteval auto type() const -> std::meta::info { return normalize_type(info_); }

  /// The tag the annotation was built from: what `rbe::frame_length` and `rbe::payload_length` share.
  [[nodiscard]] consteval auto tag() const -> std::meta::info { return tag_of(type()); }

  /// The dimension the annotation belongs to, or a null reflection if it belongs to none.
  [[nodiscard]] consteval auto dimension() const -> std::meta::info { return dimension_of(type()); }

  /// The type of the value carried, or a null reflection for a marker annotation.
  [[nodiscard]] consteval auto value_type() const -> std::meta::info { return value_type_of(type()); }

  [[nodiscard]] consteval auto has_value() const -> bool { return value_type() != std::meta::info {}; }

  /// The value carried, if it is a `T` -- nullopt for a marker or for a value of any other type.
  template<typename T>
  [[nodiscard]] consteval auto value() const -> std::optional<T> {
    if (value_type() != normalize_type(^^T)) {
      return std::nullopt;
    }
    auto const payload = static_member_function(type(), "payload");
    using payload_fn   = T (*)(std::meta::info);
    return extract<payload_fn>(*payload)(info_);
  }

  /// Whether this is the annotation `needle` -- same annotation type and same value.
  template<typename A>
  [[nodiscard]] consteval auto is(A const& needle) const -> bool {
    using needle_type = std::remove_cvref_t<A>;
    return type() == normalize_type(^^needle_type) and extract<needle_type>(info_) == needle;
  }

  /// Same annotation type and same value, without either type being known statically.
  [[nodiscard]] consteval auto operator==(annotation_info const& other) const -> bool {
    if (type() != other.type()) {
      return false;
    }
    auto const equals = static_member_function(type(), "equals");
    if (not equals) {
      return true; // a marker carries no value: its type identifies it completely
    }
    using equals_fn = bool (*)(std::meta::info, std::meta::info);
    return extract<equals_fn>(*equals)(info_, other.info_);
  }

  /**
   * @brief Whether both count as "the same annotation" when checking a `unique` dimension.
   *
   * Identical to `operator==` unless the annotation's tag declares `identity_kind::kind`, in which
   * case every value produced by that tag counts as the same annotation -- `rbe::id(1)` and
   * `rbe::id(2)` are one id said twice.
   */
  [[nodiscard]] consteval auto identity_equals(annotation_info const& other) const -> bool {
    return identity_of_tag(tag()) == identity_kind::kind ? type() == other.type() : *this == other;
  }

  std::meta::info info_;
};

/// Filter predicate factory: keep only annotations belonging to dimension `dim`.
consteval auto by_dimension(std::meta::info const dim) {
  return [dim](annotation_info const a) { return a.dimension() == dim; };
}

template<detail::annotation_info Ann>
  requires(Ann.has_value())
consteval auto value_of() {
  using value_type = [:Ann.value_type():];
  return *Ann.value<value_type>();
}

} // namespace rbe::detail

// --- STD ---

namespace rbe::detail::views {

/**
 * @brief The annotations one raw attribute denotes: a `derive<...>` list expands into its
 * constituents (recursively), a plain RBE annotation is itself, anything else vanishes.
 */
consteval auto expand_annotation(std::meta::info const raw) -> std::vector<annotation_info> {
  if (is_annotation_list(raw)) {
    std::vector<annotation_info> expanded;
    for (auto const arg: template_arguments_of(normalize_type(raw))) {
      expanded.append_range(expand_annotation(arg));
    }
    return expanded;
  }
  return is_rbe_annotation(raw) ? std::vector {annotation_info {raw}} : std::vector<annotation_info> {};
}

struct annotations_view_fn : std::ranges::range_adaptor_closure<annotations_view_fn> {
  template<std::ranges::range R>
    requires std::ranges::viewable_range<R>
  consteval auto operator()(R const& r) const {
    return r // all annotations
           | std::views::transform(expand_annotation) // drop non-rbe ones, expand the lists
           | std::views::join; // join back to one single range of annotations
  }
};

inline constexpr annotations_view_fn annotations;

} // namespace rbe::detail::views

// --- STD ---

namespace rbe::detail {

/**
 * The RBE annotations written directly on `entity` (a type or a non-static data member) -- derive<...>
 * lists already expanded, non-RBE attributes already filtered out.
 *
 * Not named `annotations_of`: ADL on `std::meta::info` would make the call ambiguous with
 * `std::meta::annotations_of`.
 */
consteval auto own_annotations(std::meta::info const entity) -> std::vector<annotation_info> {
  if (not is_nonstatic_data_member(entity) and not std::meta::is_type(entity)) {
    throw std::meta::exception("info is not a non-static data member or a type", ^^own_annotations);
  }
  return std::meta::annotations_of(entity) | views::annotations | std::ranges::to<std::vector>();
}

/**
 * @brief The REQ-058..061 "annotation range" for `entity`: for a member, its own annotations unioned
 * with its type's own annotations; for a type, just its own annotations.
 *
 * @param info entity to gather rbe annotations from
 * @return a vector with all the annotations of the entity
 */
consteval auto annotation_range(std::meta::info const info) -> std::vector<annotation_info> {
  auto result = own_annotations(info);
  if (is_nonstatic_data_member(info)) {
    result.append_range(own_annotations(type_of(info)));
  }
  return result;
}

/**
 * @brief annotation_range(entity), recursively unioned with every nested non-static data member's.
 *
 * Recursion is driven by the entity's *type*, so it descends through members of class type at any
 * depth -- a member is not a leaf, its type's members are visited too. Non-class types stop it.
 *
 * @param info reflection of the type (or member) to inspect recursively
 * @return a vector with all the annotations found within the type
 */
consteval auto deep_annotations(std::meta::info const info) -> std::vector<annotation_info> {
  std::vector<annotation_info> result = annotation_range(info);

  auto const type = normalize_type(info);
  if (not is_class_type(type)) {
    return result;
  }

  std::ranges::for_each(nsdm(type), [&](std::meta::info const member) {
    result.append_range(deep_annotations(member));
  });

  return result;
}

/**
 * @param annotations The range of annotations to search within.
 * @param needle The annotation, or `derive<...>` list of annotations, to find.
 *
 * @return true if every annotation the needle denotes is found in the range.
 */
consteval auto has_annotations(std::ranges::range auto const& annotations, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  using needle_type = std::remove_cvref_t<decltype(needle)>;

  if constexpr (annotation_list<needle_type>) {
    return std::ranges::all_of(views::expand_annotation(^^needle_type), [&](annotation_info const one) {
      return std::ranges::contains(annotations, one);
    });
  }
  else {
    return std::ranges::any_of(annotations, [&](annotation_info const one) { return one.is(needle); });
  }
}

/**
 * Single entry point: `needle` may be a plain annotation OR a `derive<...>` list -- both are compared
 * against the same range, so there is exactly one code path.
 */
consteval auto has_annotations(std::meta::info const info, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  return has_annotations(annotation_range(info), needle);
}

consteval auto has_annotations_deep(std::meta::info const info, auto const needle) -> bool
  requires annotation<std::remove_cvref_t<decltype(needle)>> or annotation_list<std::remove_cvref_t<decltype(needle)>>
{
  return has_annotations(deep_annotations(info), needle);
}

/**
 * @brief The annotation built from `tag` that applies to `entity`, if any.
 *
 * The reverse direction of `has_annotations`: instead of asking whether a known annotation is there,
 * it hands back the one that is, so its value can be read (`rbe::id(value)` -> the id a message type
 * is dispatched under).
 */
consteval auto find_annotation(std::meta::info const entity, std::meta::info const tag)
    -> std::optional<annotation_info> {
  auto const range = annotation_range(entity);
  auto const it    = std::ranges::find_if(range, [tag](annotation_info const one) { return one.tag() == tag; });
  return it == std::ranges::end(range) ? std::nullopt : std::optional {*it};
}

/// `find_annotation`, searching the whole (deep) type instead of just the entity's own range.
consteval auto find_annotation_deep(std::meta::info const entity, std::meta::info const tag)
    -> std::optional<annotation_info> {
  auto const range = deep_annotations(entity);
  auto const it    = std::ranges::find_if(range, [tag](annotation_info const one) { return one.tag() == tag; });
  return it == std::ranges::end(range) ? std::nullopt : std::optional {*it};
}

/**
 * Searches entity's REQ-058..061 annotation range (its own annotations, unioned with its type's own
 * annotations for a member) for the first annotation carrying a `T`.
 */
template<typename T>
consteval auto resolve_in_scope(std::meta::info const entity) -> std::optional<T> {
  for (auto const one: annotation_range(entity)) {
    if (auto const value = one.value<T>()) {
      return value;
    }
  }
  return std::nullopt;
}

} // namespace rbe::detail

// --- STD ---

namespace rbe::detail::annotations {

/**
 * @brief Verifies that at most one annotation belonging to `dim` appears in a single annotation range.
 *
 * @param anns annotations range to verify
 * @param dim dimension
 * @return false if 2 or more annotations from the same dimension where found
 */
consteval auto satisfies_dimension(std::ranges::range auto const& anns, std::meta::info const dim) -> bool {
  return std::ranges::count_if(anns, by_dimension(dim)) <= 1;
}

consteval auto verify_dimension_correctness(std::meta::info info, std::meta::info const dim) -> bool {
  if (not satisfies_dimension(annotation_range(info), dim)) {
    return false;
  }
  auto member_check = [dim](std::meta::info const member) { return verify_dimension_correctness(member, dim); };
  return is_class_type(normalize_type(info)) ? std::ranges::all_of(nsdm(normalize_type(info)), member_check) : true;
}

// --- Global unique annotations ---

consteval auto verify_global_unique_dimension(std::meta::info const type, std::meta::info const dim) -> bool {
  auto const in_dim = deep_annotations(type) | std::views::filter(by_dimension(dim)) | std::ranges::to<std::vector>();
  return std::ranges::all_of(in_dim, [&in_dim](annotation_info const one) {
    return std::ranges::count_if(in_dim, [one](annotation_info const other) { //
             return one.identity_equals(other);
           }) <= 1;
  });
}

// --- No duplicated annotations ---

consteval auto has_duplicates(std::ranges::range auto const& elements) -> bool {
  auto it        = std::ranges::begin(elements);
  auto const end = std::ranges::end(elements);

  for (; it != end; ++it) {
    if (std::ranges::find(std::next(it), end, *it) != end) {
      return true;
    }
  }
  return false;
}

consteval auto verify_no_local_duplications(std::meta::info info) -> bool {
  info = normalize_type(info);
  if (has_duplicates(annotation_range(info))) {
    return false;
  }

  return is_class_type(info) ? std::ranges::all_of(nsdm(info), verify_no_local_duplications) : true;
}

// --- Verify annotations scope ---

consteval auto verify_type_only_scope(std::meta::info const type, std::meta::info const dim) {
  // if annotation is not in the type then deep and type_only sizes will be different
  auto deep      = deep_annotations(type) | std::views::filter(by_dimension(dim)) | std::ranges::to<std::vector>();
  auto type_only = annotation_range(type) | std::views::filter(by_dimension(dim)) | std::ranges::to<std::vector>();
  return std::ranges::size(deep) == std::ranges::size(type_only);
}

// --- Generic dispatch: discover which dimensions are actually used, verify each per its kind ---

consteval auto dimensions_used_in(std::meta::info const type) -> std::vector<std::meta::info> {
  std::vector<std::meta::info> dims;
  for (auto const ann: deep_annotations(type)) {
    if (auto const dim = ann.dimension(); dim != std::meta::info {} and not std::ranges::contains(dims, dim)) {
      dims.push_back(dim);
    }
  }
  return dims;
}

consteval auto verify_dimension(std::meta::info const type, std::meta::info const dim) -> bool {
  auto const kind = kind_of(dim);
  if (enforces(kind, dimension_kind::exclusive) and not verify_dimension_correctness(type, dim)) {
    return false;
  }
  if (enforces(kind, dimension_kind::unique) and not verify_global_unique_dimension(type, dim)) {
    return false;
  }
  if (enforces(kind, dimension_kind::type_only) and not verify_type_only_scope(type, dim)) {
    return false;
  }
  return true;
}

// --- Local constraints : verify that every annotation found in `type` satisfies its own correctness rule

/**
 * Runs the annotation's own `check`, declared on its tag, if it declares one. The check is reached
 * reflectively -- `check` is a static member function of a type only known at this point as a
 * reflection.
 */
consteval auto verify_check(annotation_info const ann, std::meta::info const entity) -> bool {
  auto const check = static_member_function(ann.tag(), "check");
  if (not check) {
    return true;
  }

  using check_fn = bool (*)(annotation_info const, std::meta::info const);
  return extract<check_fn>(*check)(ann, entity);
}

consteval auto verify_local_constraints(std::meta::info info) -> bool {
  auto check = std::ranges::all_of(annotation_range(info), [info](annotation_info const ann) { //
    return verify_check(ann, info);
  });

  info = normalize_type(info);
  return check and (is_class_type(info) ? std::ranges::all_of(nsdm(info), verify_local_constraints) : true);
}

/**
 * @brief Verifies that `type` is annotated correctly: no local duplicates, and every dimension found
 * among its (possibly nested) annotations satisfies its own correctness rule.
 *
 * Adding a brand new dimension anywhere in the codebase requires zero edits here -- it is discovered
 * dynamically from the annotations actually attached to `type`.
 */
consteval auto well_annotated(std::meta::info const type) -> bool {
  auto const dimension_check = [type](std::meta::info const dim) { return verify_dimension(type, dim); };

  return verify_no_local_duplications(type) //
         and verify_local_constraints(type) //
         and std::ranges::all_of(dimensions_used_in(type), dimension_check);
}

} // namespace rbe::detail::annotations

// --- STD ---

namespace rbe {

template<typename T>
concept well_annotated = detail::annotations::well_annotated(^^T);

template<typename T, auto Annotation>
concept contains_annotation = detail::has_annotations_deep(^^T, Annotation);

} // namespace rbe

// ===== rbe/annotations/bits.hpp =====

/**
 * @file bits.hpp
 * @date 18/09/2026
 * @brief Explicit bit placement annotation
 */


// --- Includes ---

// ===== rbe/annotations/endianness.hpp =====

/**
 * @file endianness.hpp
 * @date 20/08/2026
 * @brief Endianness annotations
 *
 * The endianness dimension is declared here; `rbe::bits` joins it from `bits.hpp`.
 */


// --- Includes ---

// ===== rbe/core/endian.hpp =====

/**
 * @file endian.hpp
 * @date 27/06/2026
 * @brief Endianness conversion and byte load/store helpers
 */


// --- Includes ---

// ===== rbe/core/detail/memcpy_constexpr.hpp =====

/**
 * @file memcpy_constexpr.hpp
 * @date 24/06/2026
 * @brief Constexpr-safe memcpy and load helpers built on std::bit_cast
 */


// --- Includes ---

// --- STD ---

// --- System ---


namespace rbe::detail {


// NOTE: this function can only copy in compiletime if the src struct
// doesn't have any padding bytes, otherwise it will fail to compile.
template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr void memcpy_constexpr(std::span<std::byte> dst, T const& src) {
  assert(dst.size() >= sizeof(T));
  auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(src);
  std::ranges::copy(bytes, std::ranges::begin(dst));
}

template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr void memcpy_constexpr(std::byte* dst, T const& src) {
  memcpy_constexpr(std::span<std::byte>(dst, sizeof(T)), src);
}

template<typename T>
  requires(std::is_trivially_copyable_v<T>)
constexpr T load(std::span<std::byte const> const source) {
  assert(source.size() >= sizeof(T));
  std::array<std::byte, sizeof(T)> buffer {};
  // copy_n, not copy: the precondition is >=, so source is routinely larger than T -- a view handed a
  // buffer that outlives the value, or a payload whose declared extent exceeds the type it decodes.
  std::ranges::copy_n(std::ranges::begin(source), sizeof(T), std::ranges::begin(buffer));
  return std::bit_cast<T>(buffer);
}

} // namespace rbe::detail

// --- STD ---

// --- System ---

namespace rbe::endian {

using order = std::endian;

template<order From = order::native>
constexpr std::integral auto to_native(std::integral auto value) {
  if constexpr (From == order::native) {
    return value;
  }
  else {
    return std::byteswap(value);
  }
}

// NOTE: This function is an alias for to_native<O>(value) and is provided for convenience.
// it might seem confusing but actually both operate the same way, the difference is in the
// semantics of the function name
template<order To = order::native>
constexpr std::integral auto native_to(std::integral auto value) {
  return to_native<To>(value);
}


template<std::integral T, order O = order::native>
constexpr T load(std::byte const* src) {
  std::array<std::byte, sizeof(T)> bytes {};
  std::ranges::copy_n(src, sizeof(T), std::ranges::begin(bytes));
  return to_native<O>(std::bit_cast<T>(bytes));
}

template<std::integral T, order O = order::native>
constexpr void store(std::byte* dst, T value) {
  detail::memcpy_constexpr(dst, native_to<O>(value));
}

} // namespace rbe::endian

// --- STD ---

namespace rbe {

namespace detail {

struct endianness_dim {
  static constexpr auto kind = dimension_kind::exclusive;

  /// NOTE: little endian is defaulted here becouse it is the most common on the wire
  /// we need determinisitc wire representation accross different machines to avoid
  /// ABI incomppatibilities, with order::native two machines with different endianness would
  /// serialize the same struct differently making it imposible to communcate between them.
  static constexpr auto default_value = endian::order::little;
};

struct order_tag {
  using dimension  = endianness_dim;
  using value_type = endian::order;
};

} // namespace detail

/**
 * @brief The byte order the annotated struct or member is serialized in.
 *
 * Written on a struct it applies to every member: a member with no byte order of its own inherits
 * it, and a member that states one replaces it for that field. What a member may not do is
 * contradict its own type -- annotate a member whose type already carries a byte order and the two
 *
 * share a single annotation range, so they do not override, they conflict, and compilation fails.
 * An explicitly annotated struct keeps its byte order wherever it is used.
 *
 * With no endianness annotation anywhere in scope, fields are little-endian -- a fixed byte order,
 * not the host's, so the same struct reaches the wire the same way from any machine.
 */
inline constexpr detail::annotation_kind<detail::order_tag> order {};
inline constexpr auto little = order(endian::order::little);
inline constexpr auto big    = order(endian::order::big);

} // namespace rbe

// --- STD ---

namespace rbe {

namespace detail {

/// The widest field a bit range can address is 64 bits wide, so 63 is the highest bit there is.
inline constexpr auto max_bit_index = 63;

/// The value `rbe::bits(msb, lsb)` carries.
struct bit_range {
  std::uint8_t msb, lsb;

  // NOTE: int is used here to avoid -Wconversion warnings when passing literals
  consteval bit_range(int const msb, int const lsb) :
    msb {static_cast<std::uint8_t>(msb)}, lsb {static_cast<std::uint8_t>(lsb)} {
    if (lsb < 0 or msb > max_bit_index) {
      throw std::meta::exception("bit index out of range: a bit range lives within [0, 63]", ^^bit_range);
    }
    if (msb < lsb) {
      throw std::meta::exception("inverted bit range: msb must not be below lsb", ^^bit_range);
    }
  }

  consteval auto operator==(bit_range const&) const -> bool = default;
};

/**
 * Explicit bits semantics. It joins the endianness dimension -- `[[=rbe::little, =rbe::bits(3,0)]]`
 * is correctly rejected -- without carrying an `endian::order`, so byte-order resolution simply does
 * not see it.
 */
struct bits_tag {
  using dimension  = endianness_dim;
  using value_type = bit_range;

  /**
   * `bit_range` can only bound itself by the widest field RBE supports; here the annotated field is
   * known, so the range is checked against the bits that field actually has.
   */
  static consteval auto check(annotation_info const ann, std::meta::info const entity) -> bool {
    auto const type = normalize_type(entity);
    if (not is_integral_type(type)) {
      return false;
    }
    auto const range = ann.value<bit_range>();
    return range and static_cast<std::size_t>(range->msb) < 8U * size_of(type);
  }
};

} // namespace detail

/**
 * @brief The bit range, most significant bit first, the annotated field occupies: `rbe::bits(3, 0)`.
 *
 * Both indices are inclusive and must fit in the annotated integral field. It shares the endianness
 * dimension, so it cannot be combined with `little`/`big` on the same field.
 *
 * @note Declared and checked, but not yet consumed by layout computation.
 */
inline constexpr detail::annotation_kind<detail::bits_tag> bits {};

} // namespace rbe

// ===== rbe/annotations/count.hpp =====

/**
 * @file count.hpp
 * @date 06/10/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/core/detail/invoke_concept.hpp =====

/**
 * @file invoke.hpp
 * @date 24/08/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

template<std::meta::reflection_range R = std::initializer_list<std::meta::info>>
consteval auto invoke_concept(std::meta::info const concept_rfl, R&& arguments) -> bool {
  if (not is_concept(concept_rfl)) {
    throw std::meta::exception("reflection is not a concept", ^^invoke_concept);
  }
  return extract<bool>(substitute(concept_rfl, std::forward<R>(arguments)));
}

} // namespace rbe::detail

// ===== rbe/core/detail/static_string.hpp =====

/**
 * @file static_string.hpp
 * @date 16/07/2026
 * @brief String type usable as a non-type template parameter via reflection
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe {


namespace detail {

template<typename T, std::meta::info S>
inline constexpr auto string_object = [] { //
  return std::string_view(extract<char const*>(S), extent(type_of(S)) - 1);
}();

template<typename T, std::meta::info D, std::meta::info S>
inline constexpr auto string_view_object = [] { //
  return std::string_view(extract<char const*>(D), extract<std::size_t>(S));
}();

consteval auto define_static_string(std::string const& s) -> std::string_view const& {
  std::vector<std::meta::info> parts;

  parts.push_back(type_of(^^s));
  parts.push_back(reflect_constant(std::meta::reflect_constant_string(s)));

  auto r = object_of(substitute(^^string_object, parts));

  return extract<std::string_view const&>(r);
}

consteval auto define_static_string(std::string_view const s) -> std::string_view const& {
  std::vector<std::meta::info> parts;
  auto const str = std::string {s};

  parts.push_back(type_of(^^s));
  parts.push_back(reflect_constant(std::meta::reflect_constant_string(str)));

  auto const r = object_of(substitute(^^string_object, parts));

  return extract<std::string_view const&>(r);
}

} // namespace detail

/**
 * @brief string class that can be passed as constant template parameter (nttp)
 *
 * @note This class is inspired by the ctp library from this post:
 * https://brevzin.github.io/c++/2025/08/02/ctp-reflection
 *
 */
struct static_string {
  using target_type = std::string_view;

  target_type const& value;

  consteval static_string(std::string const& s) : value(detail::define_static_string(s)) { }

  consteval static_string(std::string_view const s) : value(detail::define_static_string(s)) { }

  consteval static_string(char const* s) : value(detail::define_static_string(std::string_view {s})) { }

  consteval operator target_type const&() const { return value; }

  consteval auto get() const -> target_type const& { return value; }

  consteval auto operator*() const -> target_type const& { return value; }

  consteval auto operator->() const -> target_type const* { return std::addressof(value); }

  /// Compares the strings, not the objects. `constexpr` rather than `consteval` on purpose: the
  /// conversion above is an immediate function, so it cannot be what a constrained algorithm
  /// compares two of these with.
  friend constexpr auto operator==(static_string const& lhs, static_string const& rhs) -> bool {
    return lhs.value == rhs.value;
  }
};

} // namespace rbe

// --- STD ---

namespace rbe {

namespace detail {

template<typename T>
concept has_compile_time_size = requires {
  { std::tuple_size<std::remove_cvref_t<T>>::value };
};

template<typename T>
concept dynamic_owning_contiguous_range = //
    std::ranges::contiguous_range<T> //
    and not std::ranges::view<T> //
    and not std::is_array_v<std::remove_cvref_t<T>> //
    and not has_compile_time_size<T>; //

struct count_dim {
  static constexpr auto kind = dimension_kind::exclusive;
};

struct count_tag {
  using dimension  = count_dim;
  using value_type = static_string;

  static consteval auto check(annotation_info const annotation, std::meta::info const entity) -> bool try {
    // Conditions:
    //   - The pointed member must exist before the annotated field
    //   - The pointed member's type must be convertible to std::size_t
    //   - The annotated field must be an owning contiguous_range whose length is not fixed at compile time
    auto const parent_type        = normalize_type(parent_of(entity));
    auto const pointed_identifier = annotation.value<static_string>();
    auto const member_idx         = nsdm_index(parent_type, entity);
    auto const pointed_idx        = nsdm_index(parent_type, *pointed_identifier);
    auto const pointed_member     = nsdm(parent_type, pointed_idx);

    return (pointed_idx < member_idx) // clang-format off
      and invoke_concept(^^std::convertible_to, {normalize_type(pointed_member), ^^std::size_t})
      and invoke_concept(^^dynamic_owning_contiguous_range, {normalize_type(entity)});
    // clang-format on
  }
  catch (std::meta::exception const& /**/) {
    return false;
  }
};

} // namespace detail

/**
 * @brief Indicates which field of the annotated type holds the count of elements
 * for a variable-length array or sequence.
 */
inline constexpr detail::annotation_kind<detail::count_tag> count {};

} // namespace rbe

// ===== rbe/annotations/derive.hpp =====

/**
 * @file derive.hpp
 * @date 20/08/2026
 * @brief Annotation grouping facility and builtin annotation presets
 */


// --- Includes ---

// ===== rbe/annotations/format.hpp =====

/**
 * @file format.hpp
 * @date 20/08/2026
 * @brief Debugging annotations
 */


// --- Includes ---

namespace rbe {

namespace detail {

struct fmt_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique | dimension_kind::type_only;
};

struct fmt_tag {
  using dimension = fmt_dim;
};

} // namespace detail

/**
 * @brief Opts the annotated type into RBE's universal formatter.
 *
 * The type becomes printable through `std::format` and `operator<<` (`rbe/core/fmt.hpp`), which
 * walks every member recursively, base classes and bit-fields included. It belongs to no dimension,
 * so it combines freely with any other annotation.
 */
inline constexpr detail::annotation_kind<detail::fmt_tag> fmt {};

} // namespace rbe

namespace rbe {

/**
 * @brief Groups rbe annotations into one variable
 *
 * Users can use rbe::derive in annotations directly:
 * [[=rbe::derive<rbe::pack, rbe::little>]]
 *
 * Or they can use it to create reusable group of annotations:
 * inline constexpr auto packed_le = rbe::derive<rbe::pack, rbe::little>;
 *
 * And then use it like:
 * [[=packed_le]]
 *
 * @tparam Args Annotations to be grouped
 */
template<auto... Args>
inline constexpr detail::annotations_t<Args...> derive {};

// --- Builtin Annotation lists ---

inline constexpr auto pack_le = derive<pack, little>;
inline constexpr auto pack_be = derive<pack, big>;
inline constexpr auto debug   = derive<fmt>;

} // namespace rbe

// ===== rbe/annotations/empty.hpp =====

/**
 * @file empty.hpp
 * @date 22/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe {

namespace detail {

struct empty_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique | dimension_kind::type_only;
};

struct empty_tag {
  using dimension = empty_dim;
};

} // namespace detail

/**
 * @brief Marks a type as empty, which means it has no member variables and no base classes.
 *
 * This annotation is used to indicate that a type is empty, which can be useful for optimization
 * purposes. It is an exclusive annotation, meaning that it cannot be combined with other annotations
 *
 */
inline constexpr detail::annotation_kind<detail::empty_tag> empty {};

/**
 */
template<typename T>
concept explicitly_empty = contains_annotation<T, empty> and std::is_empty_v<T>;

} // namespace rbe

// ===== rbe/annotations/id.hpp =====

/**
 * @file id.hpp
 * @date 07/09/2026
 * @brief Message id annotations
 */


// --- Includes ---

// ===== rbe/annotations/detail/annotated_nsdm.hpp =====

/**
 * @file annotated_nsdm.hpp
 * @date 09/09/2026
 * @brief Lookup of the non-static data member carrying a given unique annotation
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

consteval auto annotated_nsdm(std::meta::info type, unique_annotation auto const ann)
    -> std::optional<std::meta::info> {
  auto const members               = nsdm(type);
  auto const has_unique_annotation = [ann](auto member) { return has_annotations(member, ann); };
  auto const it                    = std::ranges::find_if(members, has_unique_annotation);
  if (it == std::ranges::end(members)) {
    return std::nullopt;
  }
  return *it;
}

} // namespace rbe::detail

// --- STD ---

namespace rbe {

namespace detail {

// NOTE: since rbe::id has rbe::id(value) and rbe::id as marker type_only cannot be used
// in the dimmension kinds. It's checked manually in the check function
struct id_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique;
};

struct id_tag { // clang-format off
  using dimension  = id_dim;
  using value_type = deduced;

  static constexpr auto marker   = true; ///< the bare `rbe::id` is an annotation of its own
  static constexpr auto identity = identity_kind::kind;

  static consteval auto check(annotation_info const ann, std::meta::info const entity) -> bool {
    if (ann.has_value()) { // rbe::id(value): declares the id of a message type
      return is_type(entity) // id::value can only exist on types no member variables
             and invoke_concept(^^std::equality_comparable, {ann.value_type()});
    }
    // rbe::id marker can only be used with equality_comparable types 
    return invoke_concept(^^std::equality_comparable, {normalize_type(entity)});
  }
}; // clang-format on

consteval auto declared_id(std::meta::info const type) -> std::optional<annotation_info> {
  auto const annotation = find_annotation(type, ^^id_tag);
  return annotation and annotation->has_value() ? annotation : std::nullopt;
}

/// The type of the id `type` declares, as a reflection -- a null reflection if it declares none.
consteval auto id_type_of(std::meta::info const type) -> std::meta::info {
  auto const annotation = declared_id(type);
  return annotation ? annotation->value_type() : std::meta::info {};
}

/**
 * @brief The id `type` declares, read as an `Id`.
 *
 * The value type is named by the caller rather than deduced from `type`, which is what lets the type
 * be an ordinary argument: `rbe::id_of<T>()` has to take it as a template parameter because its
 * return type is whatever that one type declared. Use this where the id type is already pinned and
 * the types are being walked -- a list of candidates known to agree on it.
 *
 * @throws std::meta::exception if `type` declares no id, or declares one of another type
 */
template<typename Id>
consteval auto id_of(std::meta::info const type) -> Id {
  auto const annotation = declared_id(type);
  if (not annotation) {
    throw std::meta::exception("type declares no `rbe::id(value)` annotation", type);
  }
  auto const value = annotation->value<Id>();
  if (not value) {
    throw std::meta::exception("the declared id is not of the requested type", type);
  }
  return *value;
}

} // namespace detail

/**
 * @brief An id, in two halves: which id a type answers to, and where that id is read from.
 *
 * Neither half is much use alone -- together they are what lets a run of bytes be matched to the
 * type it should be decoded as. They are written separately because they usually sit on different
 * types: the value on each candidate, the field on the one type composed into all of them.
 *
 *   - `rbe::id(value)` declares the id a type answers to -- see `rbe::identifiable`.
 *   - `rbe::id` marks the field the id is read from -- see `rbe::identifying`.
 *
 * A type may carry both halves itself, and is then `rbe::self_identifying`.
 *
 * @note The field annotated and the value annotated as id must be comparable.
 */
inline constexpr detail::annotation_kind<detail::id_tag> id {};

/**
 * @brief A type that declares which id it answers to: `[[=rbe::id(value)]]` written on the type.
 *
 * This is the side that gets *selected*: given an id read from the bytes, it says which of several
 * candidate types those bytes should be decoded as. See `rbe::id_of` to read that id back.
 */
template<typename T>
concept identifiable = detail::declared_id(^^T).has_value();

/**
 * @brief A type with a field of its own marked `[[=rbe::id]]`: reading it yields the id.
 *
 * This is the side that *tells you* which type you are looking at, before the rest is decoded. It is
 * usually declared once and composed into every candidate, so the lookup is deliberately not deep: a
 * type that merely embeds such a type is not itself the one holding the id.
 */
template<typename T>
concept identifying = detail::annotated_nsdm(^^T, id).has_value();

/**
 * @brief A type that needs nothing else to be recognized: it declares its id and holds a field to read it from.
 *
 * Both halves sit on the type itself, so its bytes can be matched against a list of candidates with
 * no second type involved.
 */
template<typename T>
concept self_identifying = identifiable<T> and identifying<T>;

/**
 * @brief A type that can be compared for equality, which is what an id has to be.
 */
template<typename T>
concept id_like = std::equality_comparable<T> and std::copyable<T>;

/**
 * @brief The id `T` declares, as the type it was written with.
 *
 * `id_tag` deduces its value type, so the id comes back exactly as spelled in the annotation --
 * enum class and all -- rather than widened to some fixed integer type.
 *
 * @note A static_assert rather than a `requires` clause: it can name the annotation that is missing,
 * where a failed constraint would report only that no overload matched.
 */
template<identifiable T>
consteval auto id_of() {
  static constexpr auto id = detail::id_of<typename[:detail::id_type_of(^^T):]>(^^T);
  return id;
}

/**
 * @brief The type of `T`'s id, as written in the annotation.
 *
 * For where an id has to be stored or compared rather than produced: the element type of a table of
 * ids, or the field one is read into. Candidates belong in the same list only if they agree on it.
 */
template<identifiable T>
using id_type_of = std::remove_cvref_t<decltype(id_of<T>())>;

} // namespace rbe

// ===== rbe/annotations/length.hpp =====

/**
 * @file length.hpp
 * @date 07/09/2026
 * @brief Message length annotations
 */


// --- Includes ---

// --- STD ---

namespace rbe {

namespace detail {

/// Which length a field encodes.
enum class length_kind : std::uint8_t {
  frame, ///< total frame length: header + payload
  payload, ///< payload length: frame minus header
  self, ///< self annotated type length
  count ///< the number of elements in a dynamic range, not a length in bytes
};

/**
 * A field encodes exactly one length, so the three annotations may not share an annotation range;
 * across the whole (deep) type each is independently unique -- a message may carry any combination
 * of frame/payload/header lengths, each at most once.
 */
struct length_dim {
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::unique;
};

/**
 * The three lengths are three values of one annotation, not three annotations: they share a dimension,
 * a check and a spelling, and they are told apart by their value alone.
 */
struct length_tag {
  using dimension  = length_dim;
  using value_type = length_kind;

  /// The annotated field must be able to hold a length.
  static consteval auto check(annotation_info const /**/, std::meta::info const entity) -> bool {
    return is_convertible_type(normalize_type(entity), ^^std::size_t);
  }
};

} // namespace detail

/// Which of the three lengths on the wire a field encodes.
using length_kind = detail::length_kind;

/**
 * @brief Marks the field that encodes a length on the wire.
 *
 * A type may carry any combination of the three lengths, each on its own field and each at most
 * once; the annotated field must be convertible to `std::size_t`.
 *
 * Length ssemantics:
 * - self_length: the length of the annotated type itself
 * - frame_length: the total frame length, header + payload
 * - payload_length: the payload length, frame minus header
 * - header_length: the header length (self_length alias)
 * - payload_count: indicates the length by telling the count of payload elements in a frame
 */
inline constexpr detail::annotation_kind<detail::length_tag> length {};
inline constexpr auto self_length    = length(length_kind::self);
inline constexpr auto frame_length   = length(length_kind::frame);
inline constexpr auto payload_length = length(length_kind::payload);
inline constexpr auto header_length = self_length; ///< alias for self_length, convenient for use in a frame header type
inline constexpr auto payload_count = length(length_kind::count);

} // namespace rbe

// ===== rbe/core.hpp =====

/**
 * @file core.hpp
 * @date 20/08/2026
 * @brief core umbrella header
 */



// ===== rbe/core/custom.hpp =====

/**
 * @file custom.hpp
 * @date 17/07/2026
 * @brief Primary template for user-provided custom serialization/deserialization
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe {

template<class T>
struct custom;

} // namespace rbe

// ===== rbe/core/fmt.hpp =====

/**
 * @file fmt.hpp
 * @date 27/06/2026
 * @brief std::format formatter for types annotated with [[=rbe::fmt]]
 */


// --- Includes ---

// ===== rbe/core/wirable_concepts.hpp =====

/**
 * @file wirable_concepts.hpp
 * @date 30/06/2026
 * @brief Wirable concept definition
 */


// --- Includes ---

// ===== rbe/core/detail/static_array.hpp =====

/**
 * @file static_array.hpp
 * @date 07/07/2026
 * @brief Fixed-size array materialized at compile time via std::define_static_array
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe {

namespace detail {

// TODO: move from public API to detail namespace
template<typename R, typename T>
concept compatible_range = std::ranges::input_range<R> and std::convertible_to<std::ranges::range_reference_t<R>, T>;

} // namespace detail


template<typename T>
  requires std::same_as<std::remove_cvref_t<T>, T>
class static_array {
public:
  using underlying_span_type   = std::span<T const>;
  using element_type           = underlying_span_type::element_type;
  using value_type             = underlying_span_type::value_type;
  using size_type              = underlying_span_type::size_type;
  using difference_type        = underlying_span_type::difference_type;
  using const_pointer          = underlying_span_type::const_pointer;
  using const_reference        = underlying_span_type::const_reference;
  using const_iterator         = underlying_span_type::const_iterator;
  using const_reverse_iterator = underlying_span_type::const_reverse_iterator;

  consteval static_array() = default;

  template<detail::compatible_range<T> R>
  consteval static_array(std::from_range_t /**/, R&& range) :
    data_(std::define_static_array(std::forward<R>(range))) { }

  template<std::input_iterator InputIt>
  consteval static_array(InputIt first, InputIt last) : data_(std::define_static_array(first, last)) { }

  consteval static_array(std::initializer_list<T> init) : data_(std::define_static_array(init)) { }

  [[nodiscard]] constexpr const_iterator begin() const { return data_.begin(); }

  [[nodiscard]] constexpr const_iterator end() const { return data_.end(); }

  [[nodiscard]] constexpr const_reverse_iterator rbegin() const { return data_.rbegin(); }

  [[nodiscard]] constexpr const_reverse_iterator rend() const { return data_.rend(); }

  [[nodiscard]] constexpr const_iterator cbegin() const { return data_.cbegin(); }

  [[nodiscard]] constexpr const_iterator cend() const { return data_.cend(); }

  [[nodiscard]] constexpr const_reverse_iterator crbegin() const { return data_.crbegin(); }

  [[nodiscard]] constexpr const_reverse_iterator crend() const { return data_.crend(); }

  [[nodiscard]] constexpr size_type size() const { return data_.size(); }

  [[nodiscard]] constexpr size_type size_bytes() const { return data_.size_bytes(); }

  [[nodiscard]] constexpr bool empty() const { return data_.empty(); }

  [[nodiscard]] constexpr const_reference operator[](std::size_t index) const { return data_[index]; }

  [[nodiscard]] constexpr const_reference at(std::size_t index) const { return data_.at(index); }

  [[nodiscard]] constexpr const_reference front() const { return data_.front(); }

  [[nodiscard]] constexpr const_reference back() const { return data_.back(); }

  [[nodiscard]] constexpr const_pointer data() const { return data_.data(); }

  [[nodiscard]] consteval static_array first(size_type count) const {
    return {std::from_range, std::define_static_array(data_.first(count))};
  }

  [[nodiscard]] consteval static_array last(size_type offset) const {
    return {std::from_range, std::define_static_array(data_.last(offset))};
  }

  [[nodiscard]] consteval static_array subspan(size_type offset, size_type count = std::dynamic_extent) const {
    return {std::from_range, std::define_static_array(data_.subspan(offset, count))};
  }

  [[nodiscard]] constexpr bool operator==(static_array const& other) const {
    return std::ranges::equal(data_, other.data_);
  }

private:
  std::span<T const> data_;
};

template<std::ranges::range R>
static_array(std::from_range_t, R&&) -> static_array<std::ranges::range_value_t<R>>;

template<std::input_iterator InputIt>
static_array(InputIt, InputIt) -> static_array<std::iter_value_t<InputIt>>;

template<typename T>
static_array(std::initializer_list<T>) -> static_array<T>;

} // namespace rbe

// ===== rbe/core/wirable_primitives.hpp =====

/**
 * @file wirable_primitives.hpp
 * @date 02/08/2026
 * @brief Concepts and predicates for wirable primitive and custom-serialized types
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe {

// NOTE: is_array_type is not included here bc remove_all_extents is used when called this function
// I not consider array a trivially_wirable_primitive yet bc we use this concept to dispatch to endian::load/store
// functions. Maybe I should have onne more level of primitive for arrays rather than considering them as
// wirable/trivially_wirable
consteval auto is_trivially_wirable_primitive(std::meta::info const info) -> bool {
  return is_integral_type(info) or is_enum_type(info);
}

consteval auto is_custom_wirable(std::meta::info const info) -> bool { // clang-format off
  return is_complete_type(substitute(^^custom, {info})); // clang-format on
}

consteval auto is_wirable_primitive(std::meta::info const info) -> bool {
  return is_trivially_wirable_primitive(info) or is_custom_wirable(info);
}

template<typename T>
concept trivially_wirable_primitive = is_trivially_wirable_primitive(^^T);

template<typename T>
concept custom_wirable = not trivially_wirable_primitive<T> and is_custom_wirable(^^T);

template<typename T>
concept wirable_primitive = trivially_wirable_primitive<T> or custom_wirable<T>;

} // namespace rbe

// --- STD ---


// --- System ---

namespace rbe {

namespace detail {

consteval auto is_wirable_class_type(std::meta::info type) -> bool {
  type = normalize_type(type);
  if (is_wirable_primitive(remove_all_extents(type))) { // leaf case
    return true;
  }

  if (is_class_type(type) and not is_empty_type(type)) {
    return std::ranges::all_of(nsdm(type), is_wirable_class_type, std::meta::type_of);
  }
  return false;
}

} // namespace detail

template<typename T>
concept introspectable = std::meta::is_enumerable_type(^^T);

/**
 * @brief Concept to determine if a given type is suitable to be transmitted
 *        through the wire using RBE.
 *
 * A type is wirable if it satisfies any of the following:
 *   - It is an arithmetic type (`std::is_arithmetic_v`)
 *   - It is an enumeration type (`std::is_enum_v`)
 *   - It has a custom serder (user-provided or RBE-provided specialization)
 *   - It is a class such that:
 *       - It has at least one member variable
         - Member variables exist in only one class within the inheritance
 *         hierarchy (inheritance is allowed, but fields cannot be split
 *         across multiple levels)
 *       - All member variables are wirable
 *
 * Not supported yet:
 *   - Base classes
 *
 */
template<typename T>
concept wirable = wirable_primitive<T> or detail::is_wirable_class_type(^^T);

template<typename T>
concept wirable_class = std::is_class_v<T> and wirable<T>;

template<typename T>
concept wirable_range = wirable<T> and std::ranges::range<T>;

} // namespace rbe

// --- STD ---


// --- System ---

inline std::size_t& fmt_depth() {
  thread_local std::size_t depth = 0;
  return depth;
}

struct universal_formatter {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }

  template<typename T>
  auto format(T const& t, auto& fmt_ctx) const {
    auto& depth = fmt_depth();
    std::string const pad(depth * 2, ' ');
    std::string const inner((depth + 1) * 2, ' ');

    auto out = std::format_to(fmt_ctx.out(), "{} {{", has_identifier(^^T) ? identifier_of(^^T) : "(unnamed-type)");

    ++depth;

    static constexpr auto ctx     = std::meta::access_context::unchecked();
    static constexpr auto bases   = define_static_array(bases_of(^^T, ctx));
    static constexpr auto members = define_static_array(rbe::detail::nsdm(^^T, ctx));

    template for (constexpr auto base: bases) {
      out = std::format_to(out, "\n{}{},", inner, static_cast<typename[:type_of(base):] const&>(t));
    }

    template for (constexpr auto mem: members) {
      std::string_view mem_label = has_identifier(mem) ? identifier_of(mem) : "(unnamed-member)";
      if (is_bit_field(mem)) {
        out = std::format_to(out, "\n{}.{}:{} = {},", inner, mem_label, bit_size_of(mem), t.[:mem:]);
      }
      else {
        out = std::format_to(out, "\n{}.{} = {},", inner, mem_label, t.[:mem:]);
      }
    }

    if (members.size() > 0 or bases.size() > 0) {
      out = std::format_to(out, "\n{}}}", pad);
    }
    else {
      *out++ = '}';
    }

    --depth;
    return out;
  }
};

template<>
struct std::formatter<std::endian> {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }
  auto format(std::endian e, auto& ctx) const {
    return std::format_to(ctx.out(), "{}", e == std::endian::little ? "little" : "big");
  }
};

template<rbe::introspectable T>
  requires(rbe::detail::has_annotations(^^T, rbe::fmt))
struct std::formatter<T> : universal_formatter { };

template<rbe::introspectable T>
  requires(rbe::detail::has_annotations(^^T, rbe::fmt))
std::ostream& operator<<(std::ostream& os, T const& val) {
  auto const formatted = std::format("{}", val);
  os.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
  return os;
}

// ===== rbe/core/memory_layout.hpp =====

/**
 * @file memory_layout.hpp
 * @date 27/06/2026
 * @brief Public wire size query
 */


// --- Includes ---

// ===== rbe/core/detail/memory_layout.hpp =====

/**
 * @file memory_layout.hpp
 * @date 27/06/2026
 * @brief In-memory and wire struct layout computation via reflection
 */


// --- Includes ---

// ===== rbe/core/detail/context.hpp =====

/**
 * @file context.hpp
 * @brief Ambient annotation context threaded through recursive serialize/deserialize calls
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

/**
 * @brief Aggregates every dimension's resolved value (today: endianness, alignment) so it can be
 * threaded down through recursive serialize/deserialize calls as a single non-type template parameter.
 *
 * A member nested arbitrarily deep under an annotated ancestor, but with no explicit annotation of its
 * own anywhere along the way, must still inherit the ancestor's annotation. `get_wire_layout`/`serialize`/
 * `deserialize` recompute a fresh `context` at every recursive step via `merge_context`, so the value
 * threads down one hop at a time instead of resetting to the dimension's default at each level.
 */
struct context {
  endian::order endianness = endianness_dim::default_value; ///< one field per dimension
  alignment_mode alignment = alignment_dim::default_value; ///< `rbe::pack`/`rbe::align`

  friend constexpr bool operator==(context, context) = default; ///< required: NTTPs must be structural types
};

/**
 * Overwrites `ambient` with whatever `entity` explicitly annotates itself; leaves everything else
 * inherited from `ambient` untouched.
 */
consteval auto merge_context(context const ambient, std::meta::info const entity) -> context {
  context result = ambient;
  if (auto v = resolve_in_scope<endian::order>(entity)) {
    result.endianness = *v;
  }
  if (auto v = resolve_in_scope<alignment_mode>(entity)) {
    result.alignment = *v;
  }
  return result;
}

} // namespace rbe::detail

// --- STD ---

// --- System ---


namespace rbe::detail {

using member_offset = std::meta::member_offset;

struct member_layout {
  member_offset offset {};
  std::size_t size {};
  endian::order endianness {std::endian::little};

  constexpr bool operator==(member_layout const&) const = default;
};

//  TODO: add support for bit fields
struct struct_layout {
  std::size_t size {};
  static_array<member_layout> members {};

  constexpr bool operator==(struct_layout const& /**/) const = default;
};


/**
 * @brief Wire size of `info` given an inherited `ctx` -- an unannotated member's packing falls back
 * to `ctx` (whatever an outer ancestor resolved) instead of always assuming native padding, making
 * `pack` propagate correctly through arbitrarily deep unannotated nesting, exactly like endianness.
 */
consteval auto wire_size_of(std::meta::info const info, context const ctx) -> std::size_t {
  auto const local = merge_context(ctx, info);
  if (local.alignment == alignment_mode::align or is_trivially_wirable_primitive(remove_all_extents(info))) {
    return size_of(info);
  }

  std::size_t result = 0;
  for (auto const member: nsdm(info)) {
    result += wire_size_of(type_of(member), merge_context(local, member));
  }
  return result;
}

/**
 * Context-free overload: no ambient annotation is inherited from anywhere -- the behavior every
 * existing caller already relies on, unchanged.
 */
consteval auto wire_size_of(std::meta::info const info) -> std::size_t {
  return wire_size_of(info, context {});
}

consteval auto get_struct_layout(std::meta::info const info) -> struct_layout {
  auto const members = nsdm(info);

  std::vector<member_layout> member_layouts;

  for (auto const& member: members) {
    member_layouts.emplace_back(
        member_layout {
          .offset     = offset_of(member),
          .size       = size_of(type_of(member)),
          .endianness = endian::order::native,
        }
    );
  }
  return {
    .size    = size_of(info),
    .members = {std::from_range, member_layouts},
  };
}

/**
 * @brief Wire layout of `info` given an inherited `ctx` -- an unannotated member's endianness falls
 * back to `ctx` (whatever an outer ancestor resolved) instead of the dimension's global default,
 * making annotations propagate correctly through arbitrarily deep unannotated nesting.
 */
consteval auto get_wire_layout_padded(std::meta::info const info, context const ctx) -> struct_layout {
  auto const local = merge_context(ctx, info); // info's own annotation overrides the ambient
  std::vector<member_layout> member_layouts;

  for (auto const m: nsdm(info)) {
    auto const member_ctx = merge_context(local, m); // member's own annotation wins over local
    member_layouts.emplace_back(
        member_layout {
          .offset     = offset_of(m),
          .size       = wire_size_of(type_of(m), member_ctx),
          .endianness = member_ctx.endianness,
        }
    );
  }

  return {
    .size    = wire_size_of(info, ctx),
    .members = {std::from_range, member_layouts},
  };
}

consteval auto get_wire_layout_packed(std::meta::info const info, context const ctx) -> struct_layout {
  auto const local   = merge_context(ctx, info);
  auto const members = nsdm(info);

  std::vector<member_layout> member_layouts;
  member_layouts.reserve(members.size());

  // TODO: this implementation do not support bit fields
  member_offset current_offset {};
  for (auto const& member: members) {
    auto const member_ctx  = merge_context(local, member);
    auto const member_size = wire_size_of(type_of(member), member_ctx);
    member_layouts.emplace_back(
        member_layout {
          .offset     = current_offset,
          .size       = member_size,
          .endianness = member_ctx.endianness,
        }
    );
    current_offset.bytes += static_cast<std::ptrdiff_t>(member_size);
  }

  return {
    .size    = static_cast<std::size_t>(current_offset.bytes),
    .members = {std::from_range, member_layouts},
  };
}

/**
 * @brief Wire layout of `info` given an inherited `ctx` -- an unannotated member's packing falls back
 * to `ctx` (whatever an outer ancestor resolved) instead of always assuming native padding, making
 * `pack` propagate correctly through arbitrarily deep unannotated nesting, exactly like endianness.
 */
consteval auto get_wire_layout(std::meta::info const info, context const ctx) -> struct_layout {
  if (merge_context(ctx, info).alignment != alignment_mode::align) {
    return get_wire_layout_packed(info, ctx);
  }
  return get_wire_layout_padded(info, ctx);
}

/**
 * Context-free overload: no ambient annotation is inherited from anywhere -- the behavior every
 * existing caller (is_trivially_wirable, get_wire_layout<T>(), ...) already relies on, unchanged.
 */
consteval auto get_wire_layout(std::meta::info const info) -> struct_layout {
  return get_wire_layout(info, context {});
}

// NOTE: at some point if compile times get really bad maybe we should consider caching the results
// of these functions in static constexpr variables. Referencing to static constexpr however is
// buggy in clang so returning by value is the only safe option for now.
// Related threads:
// - https://github.com/llvm/llvm-project/issues/82994
// - https://github.com/llvm/llvm-project/issues/61425
//
// NOTE: The time has come, so these functions are now cached;
// the compilation times for the market examples have been reduced by 5
//
// The only layout query visible to the user is rbe::wire_size_of<T>() (see rbe/core/memory_layout.hpp)

template<wirable T, context Ctx = context {}>
consteval auto wire_size_of() -> std::size_t {
  static constexpr auto size = wire_size_of(^^T, Ctx);
  return size;
}

template<wirable_primitive T, context Ctx = context {}>
consteval auto wire_size_of() -> std::size_t {
  return sizeof(T);
}

template<typename T, context Ctx = context {}>
  requires(std::is_empty_v<std::remove_cvref_t<T>>)
consteval auto wire_size_of() -> std::size_t {
  return 0;
}

template<wirable_class T>
consteval auto get_struct_layout() -> struct_layout {
  static constexpr auto layout = get_struct_layout(^^T);
  return layout;
}

template<wirable_class T, context Ctx = context {}>
consteval auto get_wire_layout() -> struct_layout {
  static constexpr auto layout = get_wire_layout(^^T, Ctx);
  return layout;
}

} // namespace rbe::detail

// --- STD ---

namespace rbe {

/**
 * @brief Number of bytes `T` occupies on the wire, honoring its annotations (packing, ...).
 */
template<typename T>
consteval auto wire_size_of() -> std::size_t {
  return detail::wire_size_of<T>();
}

} // namespace rbe

// ===== rbe/core/overload_set.hpp =====

/**
 * @file overload_set.hpp
 * @date 15/09/2026
 * @brief Overload set utility for dispatching over multiple callables
 */


// --- Includes ---

// --- STD ---

namespace rbe {

template<typename... Args>
struct overload : Args... {
  using Args::operator()...;
};
template<typename... Args>
overload(Args...) -> overload<Args...>;

} // namespace rbe

// ===== rbe/core/trivially_wirable_concepts.hpp =====

/**
 * @file trivially_wirable_concepts.hpp
 * @date 31/07/2026
 * @brief trivially_wirable concepts for types whose struct layout matches their wire layout
 */


// --- Includes ---

// --- STD ---

namespace rbe {

namespace detail {

template<typename T>
consteval auto is_trivially_wirable_type() -> bool {
  static constexpr auto info = ^^T;
  if constexpr (wirable_class<T>) {
    return is_trivially_copyable_type(info) //
           and is_standard_layout_type(info) //
           and get_struct_layout<T>() == get_wire_layout<T>(); //
  }
  return is_integral_type(info) or is_enum_type(info);
}

} // namespace detail

consteval auto is_trivially_wirable(std::meta::info const info) -> bool {
  return is_trivially_wirable_primitive(remove_all_extents(info)) or
         ( //
             is_class_type(info) //
             and not is_empty_type(info) //
             and detail::get_struct_layout(info) == detail::get_wire_layout(info) //
             and is_trivially_copyable_type(info) //
             and is_standard_layout_type(info) //
             and std::ranges::all_of(detail::nsdm(info), is_trivially_wirable, std::meta::type_of) //
         );
}


/**
 * @brief Concept to check if a type is trivially wirable.
 *
 * A type is considered trivially wirable if its struct layout matches its wire layout.
 * Meaning that the in-memory representation of the type can be directly used for serialization without any additional
 * processing. Therefore, serialization and deserialization can be performed by simply memcpy'ing the data to and from a
 * buffer.
 *
 */
template<typename T>
concept trivially_wirable = wirable<T> and not custom_wirable<T> and is_trivially_wirable(^^T);

template<typename T>
concept trivially_wirable_class = std::is_class_v<T> and trivially_wirable<T>;

template<typename T>
concept trivially_wirable_range = trivially_wirable<T> and std::ranges::contiguous_range<T>;

} // namespace rbe

// ===== rbe/dsrl.hpp =====

/**
 * @file dsrl.hpp
 * @date 08/08/2026
 * @brief Deserialization umbrella header
 */



// ===== rbe/dsrl/deserialize.hpp =====

/**
 * @file deserialize.hpp
 * @date 12/07/2026
 * @brief Deserialization routines for DSRL (Data Serialization and Retrieval Library).
 *
 * Provides eager, lazy, and in-place deserialization strategies for converting
 * serialized byte buffers back into C++ objects.
 */


// --- Includes ---

// ===== rbe/dsrl/detail/deserialize_impl.hpp =====

/**
 * @file deserialize_impl.hpp
 * @date 12/07/2026
 * @brief Context-aware dispatch implementing eager deserialization for wirable types.
 *
 * Internal machinery: fast-path/custom/primitive/aggregate/range overloads threading a `context`
 * through recursive calls so annotations propagate correctly through arbitrarily deep nesting. The
 * public entry point (`rbe::deserialize` for `dsrl::eager_t`, in `rbe/dsrl/deserialize.hpp`) always
 * starts from the default context and dispatches here.
 */


// --- Includes ---

// ===== rbe/core/detail/normalize.hpp =====

/**
 * @file normalize.hpp
 * @date 03/08/2026
 * @brief Endianness normalization helpers for serialization
 */


// --- Includes ---

// --- STD ---

// --- System ---

namespace rbe::detail {

template<trivially_wirable_primitive T>
struct normalize_primitive_impl_t {
  using type = T;
};

template<trivially_wirable_primitive T>
  requires(std::is_enum_v<T>)
struct normalize_primitive_impl_t<T> {
  using type = std::underlying_type_t<T>;
};

template<trivially_wirable_primitive T>
using normalize_primitive_type = typename normalize_primitive_impl_t<T>::type;

/// Identity forwarder for non-enum trivially wirable types.
template<trivially_wirable T>
  requires(not std::is_enum_v<T>)
auto normalize_primitive(T const value) { return value; }

/// Convert an enum to its underlying integral type for byte-swapping.
template<trivially_wirable_primitive T>
  requires(std::is_enum_v<T>)
auto normalize_primitive(T const value) -> std::underlying_type_t<T> {
  return std::to_underlying(value);
}

/// Normalize a trivially wirable primitive to the target endianness.
template<trivially_wirable_primitive T, endian::order Order>
auto normalize_endianness(T const value) -> T {
  return static_cast<T>(endian::native_to<Order>(normalize_primitive(value)));
}

/// Identity forwarder for non-primitive types (no byte-swapping needed).
template<endian::order Order>
auto normalize_endianness(auto const& value) {
  return value;
}

} // namespace rbe::detail

// ===== rbe/dsrl/tags.hpp =====

/**
 * @file tags.hpp
 * @date 12/07/2026
 * @brief Defines tags for eager and lazy deserialization strategies
 */



namespace rbe::dsrl {

namespace detail {

struct base_tag { };

} // namespace detail

/**
 * @brief Eager deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed eagerly,
 * meaning that the entire data structure is deserialized at once.
 * 'deserialize' function will return a fully constructed object of the specified type.
 */
struct eager_t : detail::base_tag { };

/**
 * @brief Lazy deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed lazily,
 * meaning that the data structure is deserialized on-demand, as needed.
 * 'deserialize' function will return a proxy object that will perform the actual deserialization when accessed.
 */
struct lazy_t : detail::base_tag { };

/**
 * @brief In-place deserialization strategy tag.
 *
 * This tag indicates that the deserialization process should be performed in-place,
 * meaning that the data structure is deserialized directly into the provided memory location.
 * 'deserialize' function will return a reference to the deserialized object in the provided memory.
 *
 * @note This strategy is only supported if the wire format and type format are compatible, and
 * if the pointer to the memory satisfies the alignment requirements of the type otherwise the .
 * behavior is undefined.
 */
struct in_place_t : detail::base_tag { };
struct in_place_mut_t : detail::base_tag { };

inline constexpr eager_t eager {};
inline constexpr lazy_t lazy {};
inline constexpr in_place_t in_place {};
inline constexpr in_place_mut_t in_place_mut {};

template<typename T>
concept strategy = std::derived_from<T, detail::base_tag>;

} // namespace rbe::dsrl

// --- STD ---

namespace rbe::detail {

// Forward declarations so every overload below can recurse into any sibling regardless of
// definition order -- e.g. the aggregate overload needs to see the range overload, and vice versa.

template<trivially_wirable T, context Ctx = context {}>
  requires(Ctx == context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<custom_wirable T, context Ctx = context {}>
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<wirable_class T, context Ctx = context {}>
  requires(
      std::is_default_constructible_v<T> and not custom_wirable<T> and not wirable_range<T> and
      (not trivially_wirable<T> or Ctx != context {})
  )
constexpr auto deserialize(std::span<std::byte const>) -> T;

template<wirable_range T, context Ctx = context {}>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto deserialize(std::span<std::byte const>) -> T;

/**
 * @brief Eager deserialization for trivially wirable types with no ambient context forcing anything.
 *
 * Exactly today's fast path: a single direct memory load, no per-member processing.
 *
 * @tparam T The type to deserialize. Must satisfy `trivially_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<trivially_wirable T, context Ctx>
  requires(Ctx == context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return load<T>(input);
}

/**
 * @brief Eager deserialization for types with a custom serder.
 *
 * Delegates to the user-provided `custom<T>::deserialize` implementation. The wire format is
 * entirely user-defined, so the ambient context never applies to it.
 *
 * @tparam T The type to deserialize. Must satisfy `custom_wirable`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<custom_wirable T, context Ctx>
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return rbe::custom<T>::deserialize(input);
}

/**
 * @brief Eager deserialization for a primitive forced to a non-default byte order by an ancestor.
 *
 * @tparam T The primitive type to deserialize. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ctx The ambient context; must not be the default (otherwise the fast path above applies).
 * @param input A span of bytes containing the serialized data.
 * @return A value of type T with bytes swapped to native order.
 */
template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  return T {endian::load<normalize_primitive_type<T>, Ctx.endianness>(input.data())};
}

/**
 * @brief Eager deserialization for non-trivial, non-custom wirable types.
 *
 * Default-constructs the object, then deserializes each non-static data member individually,
 * threading the resolved ambient context one hop further down at each member so annotations
 * propagate correctly through arbitrarily deep unannotated nesting.
 *
 * @tparam T The type to deserialize. Must be default-constructible and wirable.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed object of type T.
 */
template<wirable_class T, context Ctx>
  requires(
      std::is_default_constructible_v<T> and not custom_wirable<T> and not wirable_range<T> and
      (not trivially_wirable<T> or Ctx != context {})
  )
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  using std::ranges::to;

  static constexpr auto local   = merge_context(Ctx, ^^T);
  static constexpr auto wire    = get_wire_layout<T, local>();
  static constexpr auto members = nsdm(^^T) | std::ranges::to<static_array>();

  T value;
  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = [:type_of(member):];
    value.[:member:]  = deserialize<member_type, merge_context(local, member)>(
                         input.subspan<layout.offset.bytes, layout.size>()
                     );
  }
  return value;
}

/**
 * @brief Eager deserialization for a wirable range (`std::array`, a bounded C array, ...).
 *
 * Reached whenever the range's elements aren't uniformly loadable as one block: either they aren't
 * trivially wirable on their own, or an ancestor's context forces a byte order onto them that their
 * in-memory representation doesn't already have. Each element goes through its own dispatch.
 *
 * @tparam T The range type to deserialize. Must satisfy `wirable_range`.
 * @param input A span of bytes containing the serialized data.
 * @return A fully constructed range of type T.
 */
template<wirable_range T, context Ctx>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto deserialize(std::span<std::byte const> const input) -> T {
  using element_type = std::ranges::range_value_t<T>;

  T value;
  auto remaining = input;
  for (auto& element: value) {
    element   = deserialize<element_type, Ctx>(remaining);
    remaining = remaining.subspan(wire_size_of(^^element_type));
  }
  return value;
}

template<wirable T, dsrl::strategy S>
constexpr auto satisfy_preconditions(std::span<std::byte const> const input, S /**/) -> bool {
  // TODO: when variable types are supported will have to change wire_size_of<T>()
  // to a query over the buffer wire_size_of<T>(buffer) ?
  if (input.size() < wire_size_of<T, context {}>()) {
    return false;
  }
  if constexpr (std::same_as<S, dsrl::in_place_t> or std::same_as<S, dsrl::in_place_mut_t>) {
    std::uintptr_t const ptr = reinterpret_cast<std::uintptr_t>(input.data());
    return (ptr & (alignof(T) - 1)) == 0;
  }

  return true;
}


} // namespace rbe::detail

// ===== rbe/dsrl/detail/deserialize_member.hpp =====

/**
 * @file deserialize_member.hpp
 * @date 03/08/2026
 * @brief Helper to populate struct members during deserialization
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

/**
 * @brief Fill a wirable member by recursive deserialization.
 *
 * @tparam T The member type. Must satisfy `wirable`.
 * @tparam Ctx The ambient context resolved for this member, threaded one hop further down into `T`'s
 *          own deserialization so annotations keep propagating through arbitrarily deep nesting.
 * @param input Byte span containing the member's serialized data.
 */
template<wirable T, context Ctx>
constexpr auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T, Ctx>(input);
}

/**
 * @brief Fill a trivially wirable primitive member with endianness handling.
 *
 * @tparam T The primitive member type. Must satisfy `trivially_wirable_primitive`.
 * @tparam Ctx The ambient context; its `endianness` is applied to this member.
 * @param input Byte span containing the member's serialized data.
 */
template<trivially_wirable_primitive T, context Ctx>
constexpr auto deserialize_member(std::span<std::byte const> const input) -> T {
  return deserialize<T, Ctx>(input);
}

template<trivially_wirable_range T, context Ctx>
  requires(Ctx.endianness == endian::order::native or sizeof(std::ranges::range_value_t<T>) == 1)
constexpr auto deserialize_member(std::span<std::byte const> input) -> T {
  return load<T>(input);
}

template<wirable_range T, context Ctx>
constexpr auto deserialize_member(std::span<std::byte const> input) -> T {
  T array;
  using element_type = std::ranges::range_value_t<T>;
  for (auto& e: array) {
    e     = deserialize<element_type, Ctx>(input);
    input = input.subspan(wire_size_of(^^element_type));
  }
  return array;
}

} // namespace rbe::detail

// ===== rbe/dsrl/proxy.hpp =====

/**
 * @file proxy.hpp
 * @date 24/06/2026
 * @brief Lazy deserialization proxy providing field-by-field access into a byte buffer
 */


// --- Includes ---


// --- STD ---

// --- System ---

namespace rbe::dsrl {

template<typename T, rbe::detail::context Ctx = rbe::detail::context {}>
  requires(wirable<T> or explicitly_empty<T>)
class proxy {
  static constexpr auto local = rbe::detail::merge_context(Ctx, ^^T);

public:
  // --- Type traits ---

  using value_type  = T;
  using size_type   = std::size_t;
  using buffer_type = std::span<std::byte const>;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<proxy> {
    // NOTE: specialize in the future when variable size types are supported
    if (data.size() >= rbe::detail::wire_size_of<value_type, local>()) {
      return proxy {data};
    }
    return std::nullopt;
  }

  // --- Constructors ---

  /// precondition: data.size() >= wire_size_of<value_type, local>()
  constexpr explicit proxy(buffer_type const data) : data_(data) { }

  template<static_string First, static_string... Rest>
    requires(wirable_class<value_type>)
  [[nodiscard]] constexpr auto field() const {
    if constexpr (sizeof...(Rest) == 0) {
      static constexpr auto member_index = rbe::detail::nsdm_index(^^value_type, First.get());
      return field<member_index>();
    }
    else {
      return field<First>().template field<Rest...>();
    }
  }

  template<rbe::detail::unique_annotation auto Annotation>
    requires(wirable_class<value_type> and contains_annotation<value_type, Annotation>)
  [[nodiscard]] constexpr auto field() const {
    static constexpr auto direct_member = rbe::detail::annotated_nsdm(^^T, Annotation);
    if constexpr (direct_member) {
      return field<identifier_of(direct_member.value())>();
    }
    else {
      template for (constexpr auto m: rbe::detail::nsdm(^^T) | std::ranges::to<static_array>()) {
        if constexpr (rbe::detail::has_annotations_deep(m, Annotation)) {
          return this->template field<identifier_of(m)>().template field<Annotation>();
        }
      }
    }
  }

  template<std::size_t Index>
    requires(wirable_class<value_type>)
  [[nodiscard]] constexpr auto field() const {
    using member_type                   = [:type_of(rbe::detail::nsdm(^^value_type, Index)):];
    static constexpr auto wire          = rbe::detail::get_wire_layout<T, local>();
    static constexpr auto member_layout = wire.members[Index];
    static constexpr auto member_ctx    = rbe::detail::merge_context(local, rbe::detail::nsdm(^^value_type, Index));

    auto const member_data = data_.subspan<member_layout.offset.bytes, member_layout.size>();
    if constexpr (wirable_class<member_type>) {
      return proxy<member_type, member_ctx>(member_data);
    }
    else {
      return rbe::detail::deserialize_member<member_type, member_ctx>(member_data);
    }
  }

  [[nodiscard]] constexpr auto value() const -> value_type {
    return rbe::detail::deserialize<value_type, local>(data_);
  }

  [[nodiscard]] constexpr auto operator*() const -> value_type { return value(); }

  [[nodiscard]] static constexpr auto fixed_size() -> size_type { return rbe::detail::wire_size_of<value_type, local>(); }

  // NOTE: for now size and fixed_size are the same since variable size types are not supported yet
  [[nodiscard]] constexpr auto size() const -> size_type { return fixed_size(); }

  /// The buffer this view was handed. Not an extent of anything: proxy only requires the buffer to be
  /// at least wire_size_of<T>, so it is routinely larger than the value. Use as_span() for the extent.
  [[nodiscard]] constexpr auto buffer() const -> buffer_type { return data_; }

  /// This value as a span, i.e. exactly the bytes T occupies. Invariant: buffer().first(size()).
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return buffer().first(size()); }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

  [[nodiscard]] friend constexpr auto operator==(proxy const lhs, value_type const& rhs) -> bool
    requires(std::equality_comparable<value_type>)
  {
    return lhs.value() == rhs;
  }

  [[nodiscard]] friend constexpr auto operator==(proxy const lhs, proxy const rhs) -> bool
    requires(std::equality_comparable<value_type>)
  {
    return lhs.value() == rhs.value();
  }

private:
  std::span<std::byte const> data_;
};

} // namespace rbe::dsrl

// ===== rbe/dsrl/return_type.hpp =====

/**
 * @file return_type.hpp
 * @date 03/09/2026
 * @brief Maps a deserialization strategy tag to the type its `deserialize`
 * overload returns.
 */


// --- Includes ---

// ===== rbe/dsrl/detail/return_type_impl.hpp =====

/**
 * @file return_type_impl.hpp
 * @date 03/09/2026
 * @brief Maps a deserialization strategy tag to the type its `deserialize` overload returns.
 */


// --- Includes ---

// --- STD ---

namespace rbe::dsrl::detail {

template<strategy Tag, wirable T>
struct return_type;

template<wirable T>
struct return_type<eager_t, T> {
  using type = T;
};

template<wirable T>
struct return_type<lazy_t, T> {
  using type = dsrl::proxy<T>;
};

template<wirable T>
struct return_type<in_place_t, T> {
  using type = T const&;
};

template<wirable T>
struct return_type<in_place_mut_t, T> {
  using type = T&;
};

} // namespace rbe::dsrl::detail

// --- STD ---

namespace rbe::dsrl {

/**
 * @brief The type `rbe::deserialize(input, Tag{})` returns for a wirable `T`.
 *
 * `type` is:
 * - `T` for `eager_t`.
 * - `dsrl::proxy<T>` for `lazy_t`.
 * - `T const&` for `in_place_t`.
 * - `T&` for `in_place_mut_t`.
 *
 * Kept as an alias over the specializations in detail/return_type_impl.hpp -- rather than exposing
 * them directly -- so that mapping can change without breaking callers, e.g. if a strategy later
 * needs to report failure and starts returning `std::expected<type, error>` instead.
 *
 * @tparam Tag The deserialization strategy tag.
 * @tparam T The wirable type being deserialized.
 */
template<strategy Tag, wirable T>
using return_type = detail::return_type<Tag, T>::type;

} // namespace rbe::dsrl

// --- STD ---

// --- System ---

namespace rbe {

// ──────────────────────────────────────────────────────────────────
// eager deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Eager deserialization for a wirable type.
 *
 *
 * Preconditions:
 * - The input buffer must be sufficiently large to contain a full serialized representation of `T`.
 *
 * If these preconditions are not met, behavior is undefined. For a safer version with bounds
 * checking, use `try_deserialize` instead.
 *
 * @tparam T The type to deserialize. Must satisfy `wirable`.
 * @param input A read-only span of bytes containing the serialized data.
 * @param eager Tag for eager deserialization strategy.
 * @return A fully constructed object of type T.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::eager_t /**/) -> T {
  return detail::deserialize<T, detail::context {}>(input);
}

// ──────────────────────────────────────────────────────────────────
// in-place deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief In-place deserialization returning a const reference (narrow contract).
 *
 * Interprets the buffer as an object of type T without copying data.
 * Only viable when the wire layout matches the in-memory layout.
 *
 * Preconditions:
 * - The input buffer must be at least `sizeof(T)` bytes in size.
 * - The input buffer must be aligned to `alignof(T)`.
 *
 * If these preconditions are not met, behavior is undefined. `try_deserialize` offers a
 * safer version of this function that checks them.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A read-only span of bytes containing the serialized data.
 * @param in_place Tag for in-place deserialization strategy.
 * @return A const reference to the object in the buffer.
 */
template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::in_place_t /**/) -> T const& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

/**
 * @brief In-place deserialization returning a mutable reference (narrow contract).
 *
 * Interprets the writable buffer as an object of type T and returns a reference.
 * No data copying occurs; the caller must ensure the buffer contains valid serialized data.
 *
 * Preconditions:
 * - The input buffer must be at least `sizeof(T)` bytes in size.
 * - The input buffer must be aligned to `alignof(T)`.
 *
 * If these preconditions are not met, behavior is undefined. `try_deserialize` offers a
 * safer version of this function that checks them.
 *
 * @tparam T The type to interpret. Must satisfy `trivially_wirable`.
 * @param input A mutable span of bytes containing the serialized data.
 * @param in_place_mut Tag for in-place mutable deserialization strategy.
 * @return A mutable reference to the object in the buffer.
 */
template<trivially_wirable T>
constexpr auto deserialize(std::span<std::byte> const input, dsrl::in_place_mut_t /**/) -> T& {
  auto* ptr = std::start_lifetime_as<T>(input.data());
  return *ptr;
}

// ──────────────────────────────────────────────────────────────────
// lazy deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Deserializes a buffer into an object lazily (narrow contract).
 *
 * The function takes a span of bytes as input and deserializes it into an object of type dsrl::proxy<T>.
 * The deserialization is performed lazily, meaning that the members of the object are deserialized on-demand when
 * accessed.
 *
 * Preconditions:
 * - The input buffer must be large enough for the proxy to operate on, given `T`'s wire layout.
 *
 * If this precondition is violated, accessing proxy fields will result in undefined behavior.
 * `try_deserialize` offers a safer version of this function that checks it.
 *
 * @tparam T The type of the object to deserialize. Must be a wirable_class.
 * @param input A read-only span of bytes containing the serialized data.
 * @param lazy A tag indicating that the deserialization should be performed lazily.
 * @return A `dsrl::proxy<T>` that deserializes fields on-demand when accessed.
 */
template<wirable T>
constexpr auto deserialize(std::span<std::byte const> const input, dsrl::lazy_t /**/) -> dsrl::proxy<T> {
  return dsrl::proxy<T> {input};
}

// ──────────────────────────────────────────────────────────────────
// safe / hardened deserialization
// ──────────────────────────────────────────────────────────────────

/**
 * @brief Safer wrapper for `deserialize` that checks buffer size and alignment.
 *
 * This function provides a safer alternative to the narrow-contract `deserialize` API by
 * validating, upfront, the preconditions that actually gate the call itself: minimum buffer
 * size and, for strategies that interpret the buffer in place, its alignment relative to
 * `alignof(T)`. If a check fails, it falls back to an empty optional instead of faulting.
 *
 * @note This function remains a narrow contract: the caller is still responsible
 *       for ensuring `input` refers to live, valid memory for its duration.
 *
 * @tparam T The `wirable` target type to deserialize the buffer into.
 * @tparam S The `strategy` applied for deserialization (e.g., zero-copy view, deep copy).
 * @param input A read-only span of bytes representing the binary payload.
 * @param strategy The strategy instance dictating the deserialization semantics.
 * @return std::optional<return_type<T, S>> The deserialized object/view if all preconditions
 *         are satisfied, or std::nullopt if the buffer is invalid or misaligned.
 */
template<wirable T, dsrl::strategy S>
constexpr auto try_deserialize(std::span<std::byte const> const input, S str)
    -> std::optional<dsrl::return_type<S, T>> //
{
  return detail::satisfy_preconditions<T>(input, str) //
             ? std::optional<dsrl::return_type<S, T>> {deserialize<T>(input, str)} //
             : std::nullopt; //
}

} // namespace rbe

// ===== rbe/framing.hpp =====

/**
 * @file framing.hpp
 * @date 11/09/2026
 * @brief Framing module umbrella header
 */


// --- Includes ---


// ===== rbe/framing/any.hpp =====

/**
 * @file any.hpp
 * @date 11/09/2026
 * @brief Short description
 */


// --- Includes ---

// ===== rbe/framing/detail/base_tags.hpp =====

/**
 * @file is_any_detail.hpp
 * @date 17/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

struct any_tag { };

struct many_tag { };

} // namespace rbe::detail

// ===== rbe/framing/dsrl/any.hpp =====

/**
 * @file any.hpp
 * @date 09/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/framing/detail/candidate_list.hpp =====

/**
 * @file candidate_list.hpp
 * @date 05/08/2026
 * @brief Type list and variant/tuple aggregation of the wirable types an id can select
 */


// --- Includes ---

// ===== rbe/core/detail/throw_check.hpp =====

/**
 * @file throw_check.hpp
 * @date 05/09/2026
 * @brief Turns a throwing consteval diagnostic into a bool, so it can double as a concept's requirement
 */


// --- STD ---

namespace rbe::detail {

/**
 * @brief Returns `true` if invoking `diagnose(args...)` doesn't throw, `false` if it does.
 *
 * @param diagnose Any callable, e.g. `diagnose_foo<Args...>` -- passed as an ordinary argument rather than
 * a non-type template parameter, so it isn't limited to plain functions (a capturing lambda works too).
 */
template<class Diagnose, class... Args>
constexpr auto no_throw(Diagnose&& diagnose, Args&&... args) noexcept -> bool try {
  std::invoke(std::forward<Diagnose>(diagnose), std::forward<Args>(args)...);
  return true;
}
catch (...) {
  return false;
}

template<class Diagnose, class... Args>
constexpr auto throws(Diagnose&& diagnose, Args&&... args) noexcept -> bool {
  return not no_throw(std::forward<Diagnose>(diagnose), std::forward<Args>(args)...);
}

} // namespace rbe::detail

// --- STD ---

#if __cpp_lib_constexpr_unordered_set >= 202502L
#endif

// --- System ---

namespace rbe::detail {

/// NOTE: if unique_set constepxr is available it's used, otherwise a vector is
/// used and the search is linear.
/// If the id is not hashable, the vector is used even if unique_set is available.

template<typename Id>
concept hashable = requires(Id const& id) { std::hash<Id> {}(id); };

template<typename Id>
consteval auto insert_unique(std::vector<Id>& seen, Id const& id) -> bool {
  if (std::ranges::contains(seen, id)) {
    return false;
  }
  seen.push_back(id);
  return true;
}

#if __cpp_lib_constexpr_unordered_set >= 202502L
template<typename Id>
consteval auto insert_unique(std::unordered_set<Id>& seen, Id const& id) -> bool {
  return seen.insert(id).second;
}

template<typename Id>
using seen_ids = std::conditional_t<hashable<Id>, std::unordered_set<Id>, std::vector<Id>>;
#else
template<typename Id>
using seen_ids = std::vector<Id>;
#endif

/**
 * @brief Verifies that these types can form a candidate list: all identifiable, all agreeing on the
 * id type, and no id declared twice.
 *
 * @throws std::invalid_argument naming the candidate at fault -- this is what a rejected list reports
 */
template<typename IdType>
consteval auto diagnose_compatible_candidates(std::span<std::meta::info const> types) -> void {
  if (types.size() < 2) {
    throw std::invalid_argument("candidate list must have at least 2 candidates");
  }

  for (auto const candidate: types) {
    if (id_type_of(candidate) == std::meta::info {}) {
      throw std::invalid_argument(
          "all candidates must be identifiable types, but '" + std::string(display_string_of(candidate)) +
          "' declares no id"
      );
    }

    if (id_type_of(candidate) != id_type_of(types[0])) {
      throw std::invalid_argument(
          "all candidates must have the same id type, but '" + std::string(display_string_of(candidate)) +
          "' has a different id type than the first candidate '" + std::string(display_string_of(types[0])) + "'"
      );
    }
  }

  seen_ids<IdType> ids;
  for (auto const candidate: types) {
    if (not insert_unique(ids, id_of<IdType>(candidate))) {
      throw std::invalid_argument(
          "all candidates must have unique ids, but '" + std::string(display_string_of(candidate)) +
          "' has the same id as another candidate"
      );
    }
  }
}

template<typename... T>
concept compatible_candidates = no_throw(diagnose_compatible_candidates<rbe::id_type_of<T...[0]>>, std::array {^^T...});

template<typename T, typename List>
concept belongs_to = [] {
  auto id = rbe::id_of<T>();
  return std::ranges::find(List::ids, id) != std::ranges::end(List::ids);
}();


/**
 * @brief The position a candidate occupies in a candidate_list, or `none` if the list has no such candidate.
 *
 * A type of its own rather than a bare index: ids are very often integers themselves, so on any interface
 * that takes both -- notably any's index constructor, which exists precisely to skip the id lookup -- an
 * index and an id would silently convert into one another.
 */
enum class candidate_index : std::size_t {
  none = std::numeric_limits<std::size_t>::max(), ///< what index_of() reports for an id no candidate declares
};

/**
 * @brief A list of wirable types, each declaring the id it answers to.
 *
 * The primary template is the error path: it carries no members, and its `consteval` block re-runs
 * the diagnosis so instantiating a bad list reports *why*
 */
template<typename... T>
struct candidate_list {
  consteval { diagnose_compatible_candidates<rbe::id_type_of<T...[0]>>(std::array {^^T...}); }
};

template<identifiable... T>
  requires(compatible_candidates<T...>)
struct candidate_list<T...> {
  using id_type = rbe::id_type_of<T...[0]>;

  // The type and canonical id of every candidate
  static constexpr auto types = std::array {^^T...};
  static constexpr auto ids   = std::array {rbe::id_of<T>()...};

  // NOTE: for now rbe only supports fixed-size wirables,
  // so we can store their sizes in a constexpr array
  // However keep in mind that in the future this will change
  static constexpr auto wire_size = std::array {rbe::wire_size_of<T>()...};
  static constexpr auto count     = sizeof...(T);

  // NOTE: For now I believe linear search is fine, this allow us to
  // tell the user to specify first in the list the most common candidates
  // Usually market protocols have a few messages that are much more common
  // than the rest, so this is a reasonable assumption
  // TODO: use if consteval to use a hash map at runtime
  static constexpr auto index_of(id_type const id) -> candidate_index {
    auto const it = std::ranges::find(ids, id);
    return it != std::ranges::end(ids) //
               ? static_cast<candidate_index>(std::ranges::distance(std::ranges::begin(ids), it)) //
               : candidate_index::none; //
  }

  template<belongs_to<candidate_list> U>
  static consteval auto index_of() -> candidate_index {
    return index_of(rbe::id_of<U>());
  }

  /// Whether `index` selects a candidate of this list, i.e. it came from the id of one of them.
  static constexpr auto contains(candidate_index const index) -> bool { return std::to_underlying(index) < count; }
};

} // namespace rbe::detail

// ===== rbe/framing/dsrl/concepts.hpp =====

/**
 * @file concepts.hpp
 * @date 20/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/framing/dsrl/detail/any_dispatcher.hpp =====

/**
 * @file message_dispatcher_impl.hpp
 * @date 05/09/2026
 * @brief Diagnostics and dispatch helpers backing the message_dispatcher concept
 */


// --- Includes ---

// ===== rbe/framing/dsrl/any_unmatched.hpp =====

/**
 * @file unknown.hpp
 * @date 21/09/2026
 * @brief Unknown and unhandled any representation
 */


// --- Includes ---

// --- STD ---

// NOTE: this file is deliberately not in the dsrl namespace,
// not because it's used in both dslr and srl, but because
// it would be too verbose to write rbe::dsrl::unmatched in the match callbacks
// Moreover probably in the future it will be used in other contexts,
// so it is better to keep it in the rbe namespace
namespace rbe {

namespace detail {

template<id_like T>
struct unmatched {
  using id_type     = T;
  using buffer_type = std::span<std::byte const>;

  id_type id;
  bool known_id;
  buffer_type data;
};

} // namespace detail

/**
 * @brief A type that represents an unhandled/unknown candidate in an any match overload set.
 *
 * This must be used as a flallback overload in any match calls,
 * to handle the case where the id of the any does not match any of the known candidates.
 */
template<typename T>
concept unmatched = detail::specialization_of(^^T, ^^detail::unmatched);

} // namespace rbe

// --- STD ---

namespace rbe::dsrl::detail {

// dsrl::proxy<T> requires wirable T, which no empty type ever satisfies can_substitute
// so it needs to be checked first
// clang-format off
consteval auto is_proxy_invocable(std::meta::info candidate, std::meta::info overload) -> bool {
  return can_substitute(^^dsrl::proxy, {candidate})
     and rbe::detail::invoke_concept(^^std::invocable, {overload, substitute(^^dsrl::proxy, {candidate})});
}

consteval auto is_match_invocable(std::meta::info candidate, std::meta::info overload) -> bool {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return is_proxy_invocable(candidate, overload)
      or rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

consteval auto is_ambiguous_match(std::meta::info candidate, std::meta::info overload) -> bool {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return is_proxy_invocable(candidate, overload)
     and rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

consteval auto is_bad_empty_match(std::meta::info candidate, std::meta::info overload) {
  overload  = add_lvalue_reference(overload);
  candidate = rbe::detail::normalize_type(candidate);
  return rbe::detail::invoke_concept(^^rbe::explicitly_empty, {candidate})
     and is_proxy_invocable(candidate, overload)
     and not rbe::detail::invoke_concept(^^std::invocable, {overload, candidate});
}

// clang-format on

consteval auto normalize_overload_type(std::meta::info const type) {
  if (rbe::detail::specialization_of(type, ^^dsrl::proxy)) {
    return rbe::detail::normalize_type(rbe::detail::member_alias_of(type, "value_type"));
  }
  return type;
}

/// The parameter type of `closure`'s call operator, or nullopt if it is a generic (templated) call
/// operator -- e.g. `[](rbe::unmatched auto) {}` matches by concept rather than by a single fixed type,
/// so it has nothing we can check against `CandidateList::types`
consteval auto overload_parameter_type_of(std::meta::info const closure) -> std::optional<std::meta::info> {
  for (auto const member: members_of(closure, rbe::detail::default_context)) {
    if (is_function(member) and not is_function_template(member) and is_operator_function(member) and
        operator_of(member) == std::meta::op_parentheses) {
      if (auto const params = parameters_of(member); params.size() == 1) {
        return type_of(params[0]);
      }
    }
  }
  return std::nullopt;
}

/// The closure types composing `overload` -- its own template arguments if it is a `rbe::overload<Args...>`,
/// or `overload` itself for a lone callback
consteval auto closures_of(std::meta::info const overload) -> std::vector<std::meta::info> {
  if (rbe::detail::specialization_of(overload, ^^rbe::overload)) {
    return template_arguments_of(overload) | std::ranges::to<std::vector>();
  }
  return {overload};
}

template<class Overload, class CandidateList>
consteval auto handled_types() -> std::vector<std::meta::info> {
  return CandidateList::types // all candidates
         | std::views::filter(std::bind_back(is_match_invocable, ^^Overload)) // we filter in matches
         | std::ranges::to<std::vector>(); // to vector
}

/// Throwing diagnostic for why `Overload` fails to be a valid `any_dispatcher` for `CandidateList`
template<typename Overload, typename CandidateList>
consteval auto diagnose_any_dispatcher() -> void {
  using id_type = CandidateList::id_type;

  // every overload set must be able to handle ids outside of `CandidateList`
  if (not std::invocable<Overload&, rbe::detail::unmatched<id_type>>) {
    throw std::invalid_argument(
        "overload set must contain a fallback callback (invocable with the observed id, or with no "
        "arguments at all) to handle unrecognized messages"
    );
  }

  // every non-generic callback must target one of the candidates (or the unmatched fallback) --
  // a callback for a type outside of CandidateList is otherwise silently dead code
  auto overload_types = handled_types<Overload, CandidateList>();
  for (auto const closure: closures_of(^^Overload)) {
    auto const param_type = overload_parameter_type_of(closure);
    if (not param_type) {
      continue; // generic call operator -- nothing fixed to check
    }
    auto const message_type = normalize_overload_type(rbe::detail::normalize_type(*param_type));
    // clang-format off
    if (not rbe::detail::invoke_concept(^^rbe::unmatched, {message_type})
        and not std::ranges::contains(CandidateList::types, message_type)) {
      //clang-format on
      throw std::invalid_argument(
          "overload set contains a callback for '" + std::string(display_string_of(message_type)) +
          "', which is not one of the candidate message types. Check for a typo, or a stale overload "
          "left over after removing it from the candidate list"
      );
    }
  }

  // explicitly empty candidates carry no data to defer, so they must be handled eagerly (T), never
  // through proxy<T>
  auto is_bad_empty_candidate = std::bind_back(is_bad_empty_match, ^^Overload);
  for (auto candidate: overload_types | std::views::filter(is_bad_empty_candidate)) {
    throw std::invalid_argument(
        "message type '" + std::string(display_string_of(candidate)) +
        "' is annotated rbe::empty and is handled through proxy<T> -- an empty type carries no data to "
        "defer, so it must be handled by its eager form (T) instead: replace the proxy<" +
        std::string(display_string_of(candidate)) + "> overload with one taking '" +
        std::string(display_string_of(candidate)) + "' by value"
    );
  }

  // a candidate must be handled by exactly one of its two forms -- invocable with both proxy<T> (lazy)
  // and T (eager) leaves it unclear which one dispatch_matched should pick
  auto is_ambiguous_candidate = std::bind_back(is_ambiguous_match, ^^Overload);
  for (auto candidate: CandidateList::types | std::views::filter(is_ambiguous_candidate)) {
    throw std::invalid_argument(
        "overload set is ambiguous for message type '" + std::string(display_string_of(candidate)) +
        "': it is invocable with both its lazy (proxy<T>) and eager (T) form -- keep only one"
    );
  }
}

template<class Overload, rbe::unmatched T>
constexpr auto dispatch_unmatched(Overload&& overload_set, T&& unmatched) -> decltype(auto) {
  return std::invoke(std::forward<Overload>(overload_set), std::forward<T>(unmatched));
}

/// Called once `CandidateType` is known to be the candidate matching the observed id -- what's left is
/// purely a compile-time overload resolution: `overload_set` may take either `proxy<CandidateType>` (lazy) or
/// `CandidateType` itself (eager), and whichever it's invocable with is used (both can't apply at once --
/// `message_dispatcher` already rules that out), or it falls back to `dispatch_unmatched` if it's
/// invocable with neither.
template<class CandidateType, class Overload>
constexpr auto dispatch_matched(Overload overload_set, std::span<std::byte const> buffer) -> decltype(auto) {
  // NOTE: explicitly_empty needs to be first otherwise dsrl::proxy<CandidateType>  would fail the compilation
  if constexpr (explicitly_empty<CandidateType>) {
    return std::invoke(std::forward<Overload>(overload_set), CandidateType {});
  }
  else if constexpr (std::invocable<Overload&, dsrl::proxy<CandidateType>>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::lazy));
  }
  else if constexpr (std::invocable<Overload&, CandidateType>) {
    return std::invoke(std::forward<Overload>(overload_set), rbe::deserialize<CandidateType>(buffer, dsrl::eager));
  }
}


} // namespace rbe::dsrl::detail

// ===== rbe/framing/frame_concepts.hpp =====

/**
 * @file frame_concepts.hpp
 * @date 11/09/2026
 * @brief Level-agnostic core of the framing concepts
 *
 * A frame exists at three levels: the rbe:: vocabulary type the user writes (rbe::frame, see
 * frame.hpp), and its rbe::dsrl:: / rbe::srl:: lowerings. Lowering only rewrites the payload
 * *representation*, never the shape of a frame nor the categories a payload may fall into, so the shape
 * (rbe::is_frame) and the payload of a lowered frame (rbe::frame_payload) are written once here and reused by
 * every level. What each level adds on top -- the traits it must expose, the API it must offer -- lives with
 * that level: rbe::frame_serder (frame_serder_concepts.hpp), rbe::dsrl::is_frame, rbe::srl::is_frame.
 *
 * Everything that only depends on the shape -- the delimiting classification of
 * frame_delimiting_concepts.hpp and detail/payload_extent.hpp -- is therefore written against
 * rbe::is_frame and applies to a frame and to its lowerings alike.
 */


// --- Includes ---

// --- STD ---

namespace rbe {

// TODO: structural concepts, see rbe::dsrl::any / rbe::dsrl::many
template<typename T>
concept is_any = std::derived_from<T, detail::any_tag>;

template<typename T>
concept is_many = std::derived_from<T, detail::many_tag>;

/**
 * @brief The header of a frame, at any level
 *
 * A header is a fixed-layout message, and lowering never changes its type, so there is a single header
 * concept shared by the vocabulary frame and by both lowerings.
 */
template<typename T>
concept frame_header = wirable<T>;

/**
 * @brief The shape shared by every frame, at every level
 *
 * rbe::frame, rbe::dsrl::frame and rbe::srl::frame all expose the same header_type / payload_type pair.
 * This is the weakest thing worth calling a frame, and the only thing the delimiting classification needs:
 * it says nothing about how the frame is (de)serialized, which is what the per-level concepts add.
 */
template<typename T>
concept is_frame = requires {
  typename T::header_type;
  typename T::payload_type;
  requires frame_header<typename T::header_type>;
};

template<typename T>
concept frame_wirable = wirable<T> or explicitly_empty<T>;

template<typename T>
concept frame_wirable_class = wirable_class<T> or explicitly_empty<T>;

/**
 * @brief The payload of a lowered frame, shared by rbe::dsrl and rbe::srl
 *
 * The payload categories are the same on both sides of the wire, and so is the concept: a deserializer views
 * the payload bytes read-only and a serializer writes into them, but a type constructible from
 * std::span<std::byte const> is also constructible from std::span<std::byte>, which converts to it, so
 * testing the writable span covers both lowerings. The vocabulary counterpart is rbe::frame_serder_payload.
 */
template<typename T>
concept frame_payload = //
    frame_wirable<T> // wirable or explicitly_empty
    or is_frame<T> // a nested frame
    or is_any<T> // a set of alternatives resolved by an id
    or is_many<T> // a sequence of frames
    or std::constructible_from<T, std::span<std::byte>>; // an opaque view over the payload bytes

template<is_frame T>
using frame_header_t = T::header_type;

template<is_frame T>
using frame_payload_t = T::payload_type;

} // namespace rbe

// --- STD ---

namespace rbe::dsrl {

namespace detail {

using std::get; // so `get<I>(frame)` parses as a template-id and is found by ADL

/// `get<I>` the way structured bindings look for it: a member, or a free function found by ADL
template<typename T, std::size_t I>
concept has_get = requires(T const& frame) { frame.template get<I>(); } or requires(T const& frame) { get<I>(frame); };

} // namespace detail

/**
 * @brief A frame deserializer: a non-owning view over a frame's bytes that decodes it lazily
 *
 * On top of the shape, this is the API rbe::dsrl::frame offers and the one generic code (flatten, many)
 * relies on.
 *
 * @note one common mistake that the user might make
 * is that they might mix rbe::dsrl::frame with rbe::frame we could check
 * if payload is a serder to provide them a btter error message. However
 * it's not possible to include here the serder concepts because it would
 * create a circular dependency, so we will have to live with the current error message
 * until we find a better way to do it.
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and requires(T const ct) {
  typename T::buffer_type;
  typename T::size_type;

  { T::make(ct.as_span()) } -> std::same_as<std::optional<T>>;
  { ct.header() } -> std::same_as<typename T::header_return_type>;
  { ct.payload() } -> std::same_as<typename T::payload_return_type>;
  { ct.length() } -> std::same_as<typename T::size_type>;
  { ct.as_span() } -> std::same_as<typename T::buffer_type>;
  { ct.data() } -> std::same_as<std::byte const*>;

  requires std::constructible_from<T, typename T::buffer_type>;
  requires frame_header<typename T::header_type>;
  requires frame_payload<typename T::payload_type>;
};


/// A frame that can be taken apart: `auto [header, payload] = frame`
template<typename T>
concept decomposable_frame = is_frame<T> and detail::has_get<T, 0> and detail::has_get<T, 1>;


/**
 * @brief Checks whether Overload is an unambiguous dispatcher for every message in MsgList
 *
 * Satisfied when, for every message type `T` in MsgList, Overload is invocable with at most one of
 * `T` (eager) or `dsrl::msg<T>` (lazy), and Overload additionally provides exactly one fallback
 * overload for unrecognized messages: either `[](id_type id) {...}` or `[]() {...}`.
 *
 * @code
 * rbe::overload{
 *     [](FooMsg const& foo) { ... },        // eager form for FooMsg
 *     [](dsrl::msg<BarMsg> const& bar) { ... }, // lazy form for BarMsg
 *     [](MsgList::id_type id) { ... },      // fallback for unrecognized ids
 * };
 * @endcode
 *
 * @tparam Overload Candidate dispatcher, typically built with rbe::overload
 * @tparam MsgList Message list the dispatcher must be able to handle
 */
template<typename Overload, typename CandidateList>
concept any_dispatcher = rbe::detail::no_throw(detail::diagnose_any_dispatcher<Overload, CandidateList>);

} // namespace rbe::dsrl

// --- STD ---

namespace rbe::dsrl {

template<frame_wirable_class... Args>
class any : public rbe::detail::any_tag {
public:
  using candidates  = rbe::detail::candidate_list<Args...>;
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;
  using id_type     = candidates::id_type;

  // --- Factory static member function ---

  /**
   * @brief wide-contract counterpart of the constructor, construct an any over `data` after checking it's preconditions
   *
   * Preconditions:
   *   - if the id is known, the candidate fits in the buffer: data.size() >= *length_of(index_of(id), data)
   *
   * @return The any, nullopt if the candidate does not fit in the buffer. If the id is unknown, any is always returned.
   */
  [[nodiscard]] static constexpr auto make(id_type const id, buffer_type const data) -> std::optional<any> {
    auto const index = candidates::index_of(id);
    return length_of(index, data) //
        .transform([&](size_type const length) { return any {id, index, data.first(length)}; });
  }

  // --- Constructors ---

  /**
   * Any constructor takes an id and a span of bytes, and constructs an any object that can be used to dispatch
   * at runtime to the corresponding candidate.
   *
   * Any will interpet the span of bytes as the wire representation of the candidate type corresponding to the given id
   * Providing unkonw ids is allowed and will be dispatched to the fallback overload.
   *
   * @note data is truncated to the wire_size of the candidate type if the id is known,
   * otherwise it is left as-is. This way iteration naturally stops at the end of an unknown
   * candidate for those frames whose length is implicitly extracted from any rather than
   * explicitly annotated in the header.
   *
   * Preconditions are narrow contract, violating either of them is undefined behavior:
   *   - the bytes are the wire representation of the candidate the id selects, not of another one
   *   - the candidate fits in the buffer: data.size() >= *length_of(index_of(id), data)
   *
   * Over a buffer that may not hold a whole candidate yet use make()
   * instead, which reports the violation rather than running into it.
   */
  constexpr any(id_type const id, buffer_type const data) :
    id_(id), //
    index_(candidates::index_of(id)), //
    data_(trim_to(index_, data)) { }

  /**
   * @brief Dispatches the `any` object to the corresponding overload based on its ID.
   *
   * Evaluates the internal ID against the available candidate types, deserializes
   * the matching candidate, and invokes the appropriate overload from the provided set.
   *
   * Overload Set Rules
   * - Unambiguous matching: Eager and lazy overloads can be freely mixed, but must
   *   remain strictly unambiguous. For any given candidate type, exactly one overload
   *   must be invocable (either with the eager type or its lazy proxy).
   *
   * - Mandatory fallback: The overload set must provide a fallback handler constrained
   *   by the `rbe::unhandled` concept.
   *
   * - Unrecognized IDs: To distinguish between known-but-unhandled IDs and completely
   *   unrecognized IDs, an optional fallback constrained by `rbe::unknown` can be provided additionally.
   *
   * - Concept grouping: Custom C++ concepts can be used to group multiple candidate types
   *   into a single overload. However, if a concept is satisfied by both the eager and lazy
   *   forms of the same type, the dispatch will be ambiguous and fail to compile.
   *
   * @tparam T The overload set type. Must satisfy the `any_dispatcher` concept for the candidates of this `any`.
   * @param overload_set The visitor or overload set to dispatch to.
   * @return The result of the invoked overload. All invocable overloads must share the same return type.
   */
  template<any_dispatcher<candidates> T>
  constexpr auto match(T&& overload_set) const -> decltype(auto) {
    using std::ranges::to;
    template for (constexpr auto candidate: detail::handled_types<T, candidates>() | to<static_array>()) {
      using candidate_type = [:candidate:];
      if (candidates::template index_of<candidate_type>() == index_) {
        return detail::dispatch_matched<candidate_type>(std::forward<T>(overload_set), data_);
      }
    }

    return detail::dispatch_unmatched(
        std::forward<T>(overload_set), //
        rbe::detail::unmatched {.id = id_, .known_id = known_id(), .data = data_}
    );
  }

  template<typename... T>
  constexpr auto match(T&&... callbacks) const -> decltype(auto) {
    return match(rbe::overload {std::forward<T>(callbacks)...});
  }

  template<typename T>
    requires(not any_dispatcher<T, candidates>)
  constexpr auto match(T /**/) const -> decltype(auto) {
    detail::diagnose_any_dispatcher<T, candidates>();
  }

  template<rbe::detail::belongs_to<candidates> T>
  [[nodiscard]] constexpr auto is() const -> bool {
    return index_ == candidates::template index_of<T>();
  }

  template<rbe::detail::belongs_to<candidates> T, strategy S = lazy_t>
    requires(wirable_class<T>)
  [[nodiscard]] constexpr auto as(S strategy = lazy) const -> std::optional<return_type<S, T>> {
    return is<T>() ? std::optional<return_type<S, T>>(rbe::deserialize<T>(data_, strategy)) : std::nullopt;
  }

  template<rbe::detail::belongs_to<candidates> T, strategy S = lazy_t>
    requires(explicitly_empty<T>)
  [[nodiscard]] constexpr auto as() const -> std::optional<T> {
    return is<T>() ? std::optional<T>(T {}) : std::nullopt;
  }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto id() const -> id_type { return id_; }

  [[nodiscard]] constexpr auto known_id() const -> bool { return candidates::contains(index_); }

  /// How many bytes this any holds: the candidate's size for a known id, the rest of the buffer for an unknown one
  [[nodiscard]] constexpr auto size() const -> size_type { return data_.size(); }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  using index_type = rbe::detail::candidate_index;

  /// NOTE: maybe I should expose this constructor. For now it's only used by the factory function
  /// however if the id is dense and the user know it he could use it to avoid finding the index.
  /// However this doesn't allow unkown ids, so  maybe it's not the solution.
  /// TODO: A better solution is to label id's as dense by the user actually it's possible to
  /// detect such a property at compile time, but it would be a bit more complex to implement.
  /**
   * @brief Construct from an index already resolved over a span already narrowed to the candidate
   *
   * The factory path holds both, so going through the public constructor would pay a second time for the
   * id lookup and for the narrowing. `index_type` is a type of its own precisely so this overload can never
   * be selected by an id, which is an integer just as often as an index is.
   *
   * The id is carried rather than recovered from candidates::ids, which has no entry for an
   * unrecognized id: index_of() reports candidate_index::none for those, and indexing the table with
   * it reads out of bounds.
   *
   * Preconditions:
   *   - index == candidates::index_of(id)
   *   - data is already narrowed: data.size() == *length_of(index, data)
   */
  constexpr any(id_type const id, index_type const index, buffer_type const data) :
    id_(id), //
    index_(index), //
    data_(data) { }

  /// @return The candidate's wire size for a known id, the rest of the buffer for an unknown one
  [[nodiscard]] static constexpr auto length_of(index_type const index, buffer_type const data)
      -> std::optional<size_type> {
    if (not candidates::contains(index)) {
      return data.size();
    }
    auto const wire_size = candidates::wire_size[std::to_underlying(index)];
    return wire_size <= data.size() ? std::optional<size_type> {wire_size} : std::nullopt;
  }

  /// Narrowing counterpart of length_of(), narrow contract: data must hold the candidate whole
  [[nodiscard]] static constexpr auto trim_to(index_type const index, buffer_type const data) -> buffer_type {
    if (not candidates::contains(index)) {
      return data;
    }
    assert(candidates::wire_size[std::to_underlying(index)] <= data.size());
    return data.first(candidates::wire_size[std::to_underlying(index)]);
  }

  id_type id_;
  index_type index_;
  buffer_type data_;
};

} // namespace rbe::dsrl

// ===== rbe/framing/srl/any.hpp =====

/**
 * @file any.hpp
 * @date 09/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---


namespace rbe::srl {


template<wirable_class... Args>
class any {
public:
private:
};

} // namespace rbe::srl


namespace rbe {

template<frame_wirable_class... Args>
struct any : detail::any_tag {
  using dsrl_type = dsrl::any<Args...>;
  // using srl_type  = srl::any<Args...>;
};

} // namespace rbe

// ===== rbe/framing/blob.hpp =====

/**
 * @file blob.hpp
 * @date 12/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe {

struct blob {
  using dsrl_type = std::span<std::byte const>;
  using srl_type  = std::span<std::byte>;
};

} // namespace rbe

// ===== rbe/framing/deserialize.hpp =====

/**
 * @file deserialize.hpp
 * @date 07/10/2026
 * @brief deserialize and try_deserialize for vocabulary frames
 *
 * A frame is only ever read through its view, so no strategy is asked for: `deserialize<aquis::packet>(buffer)`
 * is the same as building `aquis::packet::dsrl_type` over the buffer.
 */


// --- Includes ---

// ===== rbe/framing/frame_serder_concepts.hpp =====

/**
 * @file frame_serder_concepts.hpp
 * @date 11/09/2026
 * @brief Vocabulary level of the framing concepts
 *
 * Only what the vocabulary level adds on top of the shared core (frame_concepts.hpp) lives here: the traits
 * a type must expose to be lowered (rbe::serder_traits), the payload a user may write
 * (rbe::frame_serder_payload) and the check that both lowerings of a frame are well formed
 * (rbe::frame_serder).
 */


// --- Includes ---

// ===== rbe/framing/srl/concepts.hpp =====

/**
 * @file frame_concepts.hpp
 * @date 16/09/2026
 * @brief Serialization level of the framing concepts
 *
 * Only what the srl lowering adds on top of the shared core (rbe/framing/frame_concepts.hpp) lives here.
 * The shape (rbe::is_frame), the header (rbe::frame_header) and the payload (rbe::frame_payload) are used
 * as-is from the enclosing namespace.
 */


// --- Includes ---

// --- STD ---

namespace rbe::srl {

/**
 * @brief A frame serializer
 *
 * @note only the shape is required for now; the API requirements land with rbe::srl::frame
 */
template<typename T>
concept is_frame = rbe::is_frame<T> and frame_payload<typename T::payload_type>;

} // namespace rbe::srl

// --- STD ---

namespace rbe {

/**
 * @brief A vocabulary type that knows how to lower itself to a (de)serializer
 *
 * rbe::frame, rbe::blob, rbe::many and rbe::any are the vocabulary types the user writes; each one names its
 * dsrl / srl counterpart, which is what rbe::detail::to_dsrl_t and rbe::detail::to_srl_t map through.
 */
template<typename T>
concept serder_traits = requires {
  typename T::dsrl_type;
  // typename T::srl_type;
};

/**
 * @brief The payload of a vocabulary frame
 *
 * The vocabulary level does not spell the payload categories out the way rbe::frame_payload does:
 * a payload is either a plain wirable message, lowered as-is, or a vocabulary type that lowers itself, which
 * is what blob, many, any and nested frames have in common.
 */
template<typename T>
concept frame_serder_payload = wirable<T> or serder_traits<T>;

/**
 * @brief A vocabulary frame: a frame shape whose lowerings are both well formed
 *
 * This is what rbe::frame produces, and what a user writes their protocol with. Its lowerings are checked by
 * the concept of their own level, so a malformed frame is rejected where it is written, not where it is used.
 */
template<typename T>
concept frame_serder = is_frame<T> and serder_traits<T> and requires {
  requires frame_serder_payload<typename T::payload_type>;
  requires dsrl::is_frame<typename T::dsrl_type>;

  // TODO:
  // requires srl::is_frame<typename T::srl_type>;
  // requires value_type_of<typename T::value_type, T>;
};

template<frame_serder T>
using frame_value_t = T::value_type;

template<frame_serder T>
using frame_dsrl_t = T::dsrl_type;

template<frame_serder T>
using frame_srl_t = T::srl_type;

} // namespace rbe

// --- STD ---

namespace rbe {

/**
 * @brief Views a buffer as the frame `T` (narrow contract)
 *
 * Preconditions:
 * - The buffer holds the whole frame, header and payload.
 *
 * If this precondition is violated, reading the frame is undefined behavior. `try_deserialize` checks it.
 *
 * @tparam T A vocabulary frame, e.g. `rbe::frame<Header, Payload>` or a custom serder like `cboe::top::line`.
 * @param input The bytes of the frame, which the view keeps referring to: they must outlive it.
 * @return The `T::dsrl_type` view over `input`.
 */
template<frame_serder T>
[[nodiscard]] constexpr auto deserialize(std::span<std::byte const> const input) -> typename T::dsrl_type {
  return typename T::dsrl_type {input};
}

/**
 * @brief Views a buffer as the frame `T`, or nothing if the buffer does not hold it
 *
 * @tparam T A vocabulary frame.
 * @param input The bytes of the frame, which the view keeps referring to: they must outlive it.
 * @return The `T::dsrl_type` view over `input`, or `std::nullopt` if the header or the payload do not fit in it.
 */
template<frame_serder T>
[[nodiscard]] constexpr auto try_deserialize(std::span<std::byte const> const input)
    -> std::optional<typename T::dsrl_type> //
{
  return T::dsrl_type::make(input);
}

} // namespace rbe

// ===== rbe/framing/dsrl/views.hpp =====

/**
 * @file views.hpp
 * @date 06/10/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/framing/dsrl/many.hpp =====

/**
 * @file many.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/framing/frame_delimiting_concepts.hpp =====

/**
 * @file frame_delimiting_concepts.hpp
 * @date 11/09/2026
 * @brief Compile-time classification of how a frame's length is resolved
 *
 * These concepts only look at the shape of a frame (rbe::is_frame), so they apply to an rbe:: vocabulary
 * frame and to its dsrl:: / srl:: lowerings alike: lowering never changes how a frame is delimited.
 *
 * @note 'delimited' word is used here rather than 'sized' to avoid confusion between length and size semantics
 * in the library: length is the value readen from the wire and size is the hardcoded size of a type in RBE.
 *
 */


// --- Includes ---

// ===== rbe/framing/detail/payload_extent.hpp =====

/**
 * @file payload_extent.hpp
 * @date 14/09/2026
 * @brief Compile-time classification of where a frame resolves its payload length from
 *
 * The classification only looks at the structure of a frame (its header_type and payload_type), so it applies
 * equally to rbe:: vocabulary frames and to their dsrl:: / srl:: lowerings: lowering never changes the extent.
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

/**
 * @brief Where a frame resolves its payload length from, in priority order
 *
 * Wire values take precedence over static sizes. Shared by dsrl::frame::payload_length() and the
 * rbe::self_delimiting_frame / rbe::buffer_delimited_frame concepts, so the runtime resolution and the
 * compile-time classification cannot drift apart.
 */
enum class payload_extent : std::uint8_t {
  payload_length_field, ///< payload_length annotated header field
  frame_length_field, ///< frame_length annotated header field minus header_length
  nested_frame, ///< the nested frame's own length(), self-delimiting only if the nested frame is
  static_size, ///< rbe::wire_size_of<payload_type>()
  any_id, ///< rbe::any whose alternatives imply their length from the id
  buffer_end, ///< the payload extends to the end of the buffer (blob, span constructible, many)
};

template<frame_header HeaderType, typename PayloadType>
[[nodiscard]] consteval auto payload_extent_of() -> payload_extent {
  if (contains_annotation<HeaderType, rbe::payload_length>) {
    return payload_extent::payload_length_field;
  }
  if (contains_annotation<HeaderType, rbe::frame_length>) {
    return payload_extent::frame_length_field;
  }
  if (is_frame<PayloadType>) {
    return payload_extent::nested_frame;
  }
  if (wirable<PayloadType>) {
    return payload_extent::static_size;
  }
  if (is_any<PayloadType>) {
    return payload_extent::any_id;
  }
  return payload_extent::buffer_end;
}

// concepts cannot be recursive, so the walk through nested frames lives here
template<is_frame T>
[[nodiscard]] consteval auto is_self_delimiting() -> bool {
  constexpr auto extent = payload_extent_of<typename T::header_type, typename T::payload_type>();
  if constexpr (extent == payload_extent::nested_frame) {
    return is_self_delimiting<typename T::payload_type>();
  }
  else {
    return extent != payload_extent::buffer_end;
  }
}

template<is_frame T>
[[nodiscard]] consteval auto is_dispatch_delimited() -> bool {
  constexpr auto extent = payload_extent_of<typename T::header_type, typename T::payload_type>();
  if constexpr (extent == payload_extent::nested_frame) {
    return is_dispatch_delimited<typename T::payload_type>();
  }
  else {
    return extent == payload_extent::any_id;
  }
}

} // namespace rbe::detail

// --- STD ---

namespace rbe {

/**
 * @brief A frame whose length can be resolved without looking at the size of the buffer
 *
 * The length comes from a header field (payload_length, frame_length), from static sizes, or from a nested
 * self-delimiting frame (see rbe::detail::payload_extent). Such a frame may be read from a larger buffer,
 * trailing bytes being padding, which is what allows a sequence of frames to share a single buffer.
 */
template<typename T>
concept self_delimiting_frame = is_frame<T> and detail::is_self_delimiting<T>();

/**
 * @brief A frame whose payload extends to the end of the buffer it is read from
 *
 * The buffer size is the frame length, so the buffer must cover exactly the frame: trailing bytes are
 * taken as payload, never as padding. Such a frame can only be the last one in a buffer.
 *
 * @note these frames are not iterable: a sequence of them cannot be split without an external length
 */
template<typename T>
concept buffer_delimited_frame = is_frame<T> and not detail::is_self_delimiting<T>();

/**
 * @brief A frame whose length is explicitly resolved by reading a wire field.
 *
 * This concept requires the frame to be self-delimiting and checks if its
 * header contains an explicit annotation for either the total frame length
 * (`rbe::frame_length`) or the payload length (`rbe::payload_length`).
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept explicitly_delimited_frame = //
    self_delimiting_frame<T> //
    and (contains_annotation<frame_header_t<T>, rbe::frame_length> or
         contains_annotation<frame_header_t<T>, rbe::payload_length>);

/**
 * @brief A frame whose length is resolved by the implicit size of its underlying types.
 *
 * This concept applies to self-delimiting frames that lack explicit length
 * annotations in their header.
 *
 * @note Determining the length of this kind of frame might be slower when the
 * payload is arbitrary (e.g., `std::any`), as it requires dynamic type
 * dispatching to calculate the total size. The `dispatch_delimited_frame` concept
 * is provided for cases where the payload is of type `rbe::any`.
 *
 * @tparam T The frame type to be evaluated.
 */
template<typename T>
concept implicitly_delimited_frame = //
    self_delimiting_frame<T> //
    and not(contains_annotation<frame_header_t<T>, rbe::frame_length> or
            contains_annotation<frame_header_t<T>, rbe::payload_length>);

/**
 * @brief A frame whose length is resolved dynamically at runtime, typically because its payload is of type `rbe::any`.
 */
template<typename T>
concept dispatch_delimited_frame = self_delimiting_frame<T> and detail::is_dispatch_delimited<T>();

} // namespace rbe

// --- STD ---

namespace rbe::dsrl {

template<is_frame T>
  requires self_delimiting_frame<T>
class many : public rbe::detail::many_tag {
public:
  using frame_type  = T;
  using buffer_type = std::span<std::byte const>;
  using size_type   = std::size_t;

  class iterator {
  public:
    /// Holds the frame operator-> hands back: current() returns by value, so there is nothing to point
    /// into, and the pointer operator-> yields must target a frame that outlives the expression.
    class arrow_proxy {
    public:
      [[nodiscard]] constexpr auto operator->() const -> frame_type const* { return std::addressof(frame_); }

    private:
      friend class iterator;
      explicit constexpr arrow_proxy(frame_type frame) : frame_(std::move(frame)) { }

      frame_type frame_;
    };

    using parent_type       = many;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;
    using value_type        = frame_type;
    using difference_type   = std::ptrdiff_t;
    using reference         = frame_type;
    using pointer           = arrow_proxy;

    explicit constexpr iterator(parent_type* parent) : parent_(parent) { }

    [[nodiscard]] constexpr auto operator*() const -> reference {
      assert(parent_->current_.has_value());
      return parent_->current();
    }

    [[nodiscard]] constexpr auto operator->() const -> pointer {
      assert(parent_->current_.has_value());
      return arrow_proxy {parent_->current()};
    }

    constexpr auto operator++() -> iterator& {
      assert(not parent_->done());
      parent_->next();
      return *this;
    }

    constexpr auto operator++(int) -> void { ++*this; }

    [[nodiscard]] constexpr auto operator==(std::default_sentinel_t /**/) const -> bool { return parent_->done(); }

  private:
    parent_type* parent_;
  };

  [[nodiscard]] static constexpr auto
  make(buffer_type const data, size_type count = std::numeric_limits<size_type>::max()) -> std::optional<many> {
    return many {data, count};
  }

  explicit constexpr many(buffer_type const data, size_type count = std::numeric_limits<size_type>::max()) :
    current_(frame_type::make(data)), data_(data), left_(count) { }

  constexpr auto next() {
    assert(not done());
    data_    = data_.subspan(current_->length());
    current_ = frame_type::make(data_);
    --left_;
  }

  [[nodiscard]] constexpr auto current() const -> frame_type {
    assert(not done());
    return current_.value();
  }

  [[nodiscard]] constexpr auto done() const -> bool { return not current_ or left_ == 0; }

  [[nodiscard]] constexpr auto remainder() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto has_seen_partial() const -> bool { return done() and not data_.empty(); }

  [[nodiscard]] constexpr auto begin() -> iterator { return iterator(this); }

  [[nodiscard]] constexpr auto end() -> std::default_sentinel_t { return std::default_sentinel; }

  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return data_; }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return data_.data(); }

private:
  std::optional<frame_type> current_;
  buffer_type data_;
  size_type left_;
};

} // namespace rbe::dsrl

// --- STD ---

namespace rbe::views {

namespace detail {

/// Sign-safe for integers (an id read as int against an unsigned literal), plain == for anything else, enums included
template<typename A, typename B>
[[nodiscard]] constexpr auto same_id(A const& a, B const& b) -> bool {
  if constexpr (std::integral<A> and std::integral<B>) {
    return std::cmp_equal(a, b);
  }
  else {
    return a == b;
  }
}

} // namespace detail

template<dsrl::is_frame T>
  requires self_delimiting_frame<T>
struct many_fn : std::ranges::range_adaptor_closure<many_fn<T>> {

  [[nodiscard]] constexpr auto operator()(std::span<std::byte const> const data) const -> dsrl::many<T> {
    return dsrl::many<T> {data};
  }
};

template<dsrl::is_frame T>
  requires self_delimiting_frame<T>
[[nodiscard]] constexpr auto many() -> many_fn<T> {
  return {};
}

/**
 * @brief Keeps the frames whose header id is any of `ids`
 *
 * A filter-in over std::views::filter, so it composes with the rest of the adaptors:
 * `data | views::many<F>() | views::with_ids(1, 3)`. Unknown ids are matched too, the id is the one on the wire.
 */
template<typename... Ids>
  requires(sizeof...(Ids) > 0)
[[nodiscard]] constexpr auto with_ids(Ids... ids) {
  return std::views::filter([... ids = std::move(ids)](auto const& frame) {
    return (detail::same_id(frame.header().id(), ids) or ...);
  });
}

/// Keeps the frames whose id the code knows, those whose payload is one of the candidates of its any
[[nodiscard]] constexpr auto known_ids() {
  return std::views::filter([](auto const& frame) { return frame.payload().known_id(); });
}

/// Keeps the frames whose id the code does not know, the ones an any would hand to its fallback
[[nodiscard]] constexpr auto unknown_ids() {
  return std::views::filter([](auto const& frame) { return not frame.payload().known_id(); });
}

} // namespace rbe::views

// ===== rbe/framing/frame.hpp =====

/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief Rbe frame
 */


// --- Includes ---

// ===== rbe/framing/detail/to_dsrl_type.hpp =====

/**
 * @file dsrl_type_converter.hpp
 * @date 12/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

template<typename T>
struct to_dsrl {
  using type = T;
};

template<serder_traits T>
struct to_dsrl<T> {
  using type = typename T::dsrl_type;
};

template<typename T>
using to_dsrl_t = typename to_dsrl<T>::type;

} // namespace rbe::detail

// ===== rbe/framing/detail/to_srl_type.hpp =====

/**
 * @file to_srl_type.hpp
 * @date 12/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

template<typename T>
struct to_srl {
  using type = T;
};

template<serder_traits T>
struct to_srl<T> {
  using type = typename T::srl_type;
};

template<typename T>
using to_srl_t = typename to_srl<T>::type;

} // namespace rbe::detail

// ===== rbe/framing/dsrl/frame.hpp =====

/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief The default frame view: base_frame plus the default make() and length()
 */


// --- Includes ---

// ===== rbe/framing/dsrl/base_frame.hpp =====

/**
 * @file base_frame.hpp
 * @date 06/10/2026
 * @brief What every frame view shares: its header and payload, and the operations derived from them
 */


// --- Includes ---

// ===== rbe/framing/dsrl/flatten.hpp =====

/**
 * @file flatten.hpp
 * @date 14/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---


namespace rbe::dsrl {

/// The header of every nested frame followed by the innermost payload: `auto [soup, itch, payload] = flatten(frame)`
template<is_frame T>
constexpr auto flatten(T const& frame) {
  if constexpr (is_frame<typename T::payload_return_type>) {
    return std::tuple_cat(std::make_tuple(frame.header()), flatten(frame.payload()));
  }
  else {
    return std::make_tuple(frame.header(), frame.payload());
  }
}

} // namespace rbe::dsrl

// ===== rbe/framing/dsrl/header.hpp =====

/**
 * @file header.hpp
 * @date 05/10/2026
 * @brief A frame header view: a proxy over H plus the lengths the wire declares
 */


// --- Includes ---

// --- STD ---

namespace rbe::dsrl {

template<frame_header T>
class header : private rbe::dsrl::proxy<T> {
  using base = proxy<T>;
  constexpr explicit header(base const b) : base(b) { }

public:
  // --- Type traits ---

  using value_type  = base::value_type;
  using buffer_type = base::buffer_type;
  using size_type   = base::size_type;

  // --- Constants ---

  static constexpr bool has_header_length  = contains_annotation<T, rbe::header_length>;
  static constexpr bool has_payload_length = contains_annotation<T, rbe::payload_length>;
  static constexpr bool has_frame_length   = contains_annotation<T, rbe::frame_length>;
  static constexpr bool has_id             = contains_annotation<T, rbe::id>;
  static constexpr bool has_count          = contains_annotation<T, rbe::payload_count>;

  /// Whether this header settles where its payload ends, through payload_length or frame_length. When it
  /// does, the field is the one authoritative source for the payload's extent, whatever the payload is.
  static constexpr bool delimits_payload = has_payload_length or has_frame_length;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<header> {
    auto const hdr = base::make(data);
    if (not hdr) {
      return std::nullopt;
    }

    if constexpr (has_header_length) {
      auto declared_length = static_cast<size_type>(hdr->template field<rbe::header_length>());
      if (data.size() < declared_length) {
        return std::nullopt;
      }
    }
    return header {*hdr};
  }

  // --- Constructors ---

  using base::base;

  // --- Member functions ---

  // Everything that is purely about T's layout carries over unchanged so we
  // can directly expose proxy's interface for those members.

  using base::buffer; // the buffer this view was handed, untrimmed -- not an extent of anything
  using base::data;
  using base::field;
  using base::size; // size of T, known to the code, not the length the wire declares
  using base::value;
  using base::operator*;

  [[nodiscard]] constexpr auto id() const
    requires(has_id)
  {
    return this->template field<rbe::id>();
  }

  // NOTE: header length doesn't have to be always the same
  // as size(). Header length mainly exists
  // to improve backwards compatibiility whenn adding new fields to a header
  // so the user could have a reduced version of the header meaning
  // that header_length (value comming through the wire) could be bigger than the
  // size of the header type (size(), known to the code). is_extended() can be used to check
  // this condition, and extension_span() hands back the bytes T does not account for.
  // The behaivour is undefined if the length the wire declares is smaller than the size of T (size()).
  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (has_header_length) {
      assert(this->template field<rbe::header_length>() >= this->size());
      return this->template field<rbe::header_length>();
    }
    else {
      return this->size();
    }
  }

  [[nodiscard]] constexpr auto is_extended() const -> bool { return length() > this->size(); }

  // Payload length can be derived sorted by priority as follows:
  // - payload_length field if present
  // - frame_length - header_length if frame_length field is present
  // if this value is wrong the framing library behaivour is undefined
  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_payload_length)
  {
    return this->template field<rbe::payload_length>();
  }

  // NOTE: same with payload_length as with header_length
  // the payload length the wire declares might differ from the payload size
  // if the payload is a wirable_class.
  // The behaivour is undefined if the payload is a wirable_class
  // and the payload_length is smaller than the size of the payload type.
  [[nodiscard]] constexpr auto payload_length() const -> size_type
    requires(has_frame_length and not has_payload_length)
  {
    assert(this->frame_length() >= this->length());
    return this->frame_length() - this->length();
  }

  // Frame length can be derived sorted by priority as follows:
  // - frame_length field if present
  // - payload_length + header_length if payload_length field is present
  // if this value is wrong the framing library behaivour is undefined
  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_frame_length)
  {
    assert(this->template field<rbe::frame_length>() >= this->length());
    return this->template field<rbe::frame_length>();
  }

  [[nodiscard]] constexpr auto frame_length() const -> size_type
    requires(has_payload_length and not has_frame_length)
  {
    return this->length() + payload_length();
  }

  [[nodiscard]] constexpr auto payload_count() const -> size_type
    requires(has_count)
  {
    return this->template field<rbe::payload_count>();
  }

  // --- Spans ---

  // Three extents over the same data(): the one the wire delimits, the part of it T knows how to
  // decode, and the part it does not. as_span() == known_span() + extension_span().

  /// The header as the wire delimits it: what has to be skipped to reach the payload, and what has to
  /// be re-emitted to reproduce the header verbatim.
  /// precondition: the underlying buffer holds at least length() bytes -- guaranteed when the header
  /// came from make(), the caller's responsibility when it was built from a raw buffer.
  [[nodiscard]] constexpr auto as_span() const -> buffer_type { return buffer_type {this->data(), length()}; }

  /// The bytes T declares fields for, and therefore the only ones field() may read. Never longer than
  /// as_span().
  [[nodiscard]] constexpr auto known_span() const -> buffer_type { return base::as_span(); }

  /// The trailing bytes the wire declared but T has no field for, so there is no way to decode them --
  /// appended by a newer version of the protocol. Empty unless is_extended().
  [[nodiscard]] constexpr auto extension_span() const -> buffer_type {
    return is_extended() ? as_span().subspan(this->size()) : buffer_type {};
  }
};

} // namespace rbe::dsrl

// ===== rbe/framing/dsrl/payload.hpp =====

/**
 * @file payload.hpp
 * @date 06/10/2026
 * @brief The steps a frame takes to reach its payload, as public building blocks
 *
 * Reading a frame is three steps: locate the payload (the header's job, header<H>::make and length()),
 * settle where it ends (payload_extent) and build a view over those bytes (try_construct_payload). dsrl::frame is
 * the canonical composition of the three, and nothing else; they are public so that a frame of the
 * user's own -- one that delimits by a terminator, by a composite discriminant, by a length carried in
 * the body -- composes the same blocks and swaps only the step it needs, instead of copying frame's
 * internals to stay interoperable with any, proxy and nested frames.
 *
 * Each step comes in the two forms the rest of the library uses: a narrow one that assumes its
 * precondition (payload_extent, construct_payload) and a wide one that reports failure as std::optional
 * (try_payload_extent, try_construct_payload).
 */


// --- Includes ---

// --- STD ---

namespace rbe::dsrl {

namespace detail {

template<frame_payload T>
struct normalize_payload {
  using type = T;
};

// NOTE: `not is_frame<T>` because rbe::wirable only inspects a class's own non-static data members: a frame
// view whose state lives in base_frame has none of its own and passes for wirable. That is a gap in
// detail::is_wirable_class_type (bases are ignored), not a property of frames; the guard keeps it out of here.
template<frame_payload T>
  requires(frame_wirable<T> and not is_frame<T>)
struct normalize_payload<T> {
  using type = proxy<T>;
};

template<typename P>
concept has_make = requires(std::span<std::byte const> const bytes) {
  { P::make(bytes) } -> std::same_as<std::optional<P>>;
};

template<frame_header H>
constexpr auto payload_count(H const hdr) -> std::size_t {
  if constexpr (H::has_count) {
    return hdr.count();
  }
  else {
    return std::numeric_limits<std::size_t>::max();
  }
}

} // namespace detail

/// The view a frame hands back for a payload declared as P: proxy<T> for a wirable T, P itself otherwise
template<frame_payload P>
using payload_view_t = typename detail::normalize_payload<P>::type;


/**
 * @brief The bytes the payload declared as P occupies at the start of `rest`, the bytes after the header
 *
 * The extent the header declares (payload_length / frame_length) when it declares one, whatever the
 * payload is; otherwise `rest`, the payload runs to the end of the buffer and the view built over it in
 * step 3 narrows itself if it can. Narrow contract: `rest` holds the extent the header declares.
 * try_payload_extent is the wide form.
 */
template<frame_payload P, frame_header H>
[[nodiscard]] constexpr auto payload_extent([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const rest)
    -> std::span<std::byte const> {
  if constexpr (header<H>::delimits_payload) {
    return rest.first(hdr.payload_length());
  }
  else {
    return rest;
  }
}

/// Wide form of payload_extent: nullopt when `rest` does not hold the extent the header declares
template<frame_payload P, frame_header H>
[[nodiscard]] constexpr auto
try_payload_extent([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const rest)
    -> std::optional<std::span<std::byte const>> {
  if constexpr (header<H>::delimits_payload) {
    return rest.size() >= hdr.payload_length() ? std::optional {rest.first(hdr.payload_length())} : std::nullopt;
  }
  else {
    return rest;
  }
}

/**
 * @brief The view over `bytes`, the payload's extent as already settled by step 2 (or by the caller)
 *
 * The view narrows itself to what it knows: a proxy to T's wire size, an any to its candidate's wire size
 * (the whole of `bytes` for an unknown id), a nested frame to its own length. The bytes a header declared
 * beyond that stay reachable through the frame's payload_span(), not through the view. Narrow contract:
 * the payload fits in `bytes`. try_construct_payload is the wide form.
 */
template<frame_payload P, frame_header H>
[[nodiscard]] constexpr auto
construct_payload([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const bytes) -> payload_view_t<P> {
  using view = payload_view_t<P>;
  if constexpr (is_any<view>) {
    static_assert(
        header<H>::has_id,
        "an any payload selects its candidate by the header's rbe::id field, and this header has none"
    );
    return view {hdr.id(), bytes};
  }
  else if constexpr (is_many<view>) {
    return view {bytes, detail::payload_count(hdr)}; // many
  }
  else {
    return view {bytes};
  }
}

/// Wide form of construct_payload: nullopt when the payload does not fit in `bytes`
template<frame_payload P, frame_header H>
[[nodiscard]] constexpr auto
try_construct_payload([[maybe_unused]] header<H> const hdr, std::span<std::byte const> const bytes)
    -> std::optional<payload_view_t<P>> //
{
  using view = payload_view_t<P>;
  if constexpr (is_any<view>) {
    static_assert(
        header<H>::has_id,
        "an any payload selects its candidate by the header's rbe::id field, and this header has none"
    );
    return view::make(hdr.id(), bytes); // any
  }
  else if constexpr (is_many<view>) {
    return view::make(bytes, detail::payload_count(hdr)); // many
  }
  else if constexpr (detail::has_make<view>) {
    return view::make(bytes); // proxy, a nested frame
  }
  else {
    return std::optional {view {bytes}}; // an opaque span-constructible payload, e.g. blob
  }
}

} // namespace rbe::dsrl

// --- STD ---

namespace rbe::dsrl {

/**
 * @brief The view_interface of frames
 *
 * A derived class supplies the two primitives, `static make(buffer) -> std::optional<D>` and
 * `length() const`, and gets everything derived from them for free. base_frame deliberately has neither
 * primitive: leaving one out is a compile error in many<D> or dsrl::is_frame<D>, not an iteration that
 * silently advances by the base's length. There are no protected resolution helpers either -- a derived
 * class that wants the default resolution calls the same public building blocks dsrl::frame calls
 * (header<H>::make, payload_extent, try_construct_payload), which serve a frame that holds a frame as a member
 * just as well as one that derives.
 *
 * The derived operations dispatch to the most derived type (deducing this), so as_span(), payload_span()
 * and flatten() follow a custom length().
 */
template<frame_header HeaderType, frame_payload PayloadType>
class base_frame {
public:
  // --- Type traits ---

  using header_type         = HeaderType;
  using payload_type        = PayloadType;
  using header_return_type  = rbe::dsrl::header<header_type>; // qualified: header() below would change its meaning
  using payload_return_type = payload_view_t<payload_type>;
  using buffer_type         = std::span<std::byte const>;
  using size_type           = std::size_t;

  // --- Member functions ---

  [[nodiscard]] constexpr auto header() const -> header_return_type { return header_; }

  [[nodiscard]] constexpr auto payload() const -> payload_return_type { return payload_; }

  /// Tuple-like access so a frame decomposes into its two parts: `auto [header, payload] = frame`
  template<std::size_t I>
    requires(I < 2)
  [[nodiscard]] constexpr auto get() const {
    if constexpr (I == 0) {
      return header();
    }
    else {
      return payload();
    }
  }

  [[nodiscard]] constexpr auto data() const -> std::byte const* { return header_.data(); }

  [[nodiscard]] constexpr auto buffer() const -> buffer_type { return header_.buffer(); }

  // --- Spans, all derived from length() ---

  [[nodiscard]] constexpr auto as_span(this auto const& self) -> buffer_type {
    return buffer_type {self.data(), self.length()};
  }

  [[nodiscard]] constexpr auto header_span() const -> buffer_type { return header_.as_span(); }

  [[nodiscard]] constexpr auto payload_span(this auto const& self) -> buffer_type {
    return self.as_span().subspan(self.header().length());
  }

  [[nodiscard]] constexpr auto flatten(this auto const& self) {
    return rbe::dsrl::flatten(self);
  }

protected:
  constexpr base_frame(header_return_type const hdr, payload_return_type const payload) :
    header_(hdr), payload_(payload) { }

private:
  header_return_type header_;
  payload_return_type payload_;
};

} // namespace rbe::dsrl

// A frame is a pair of header and payload, which is what makes `for (auto [header, payload] : many)` work. Any
// frame qualifies, the library's or a user's, as long as it provides get<0> (header) and get<1> (payload): base_frame
// does, a frame of the user that does not derive from it supplies its own.
template<rbe::dsrl::decomposable_frame T>
struct std::tuple_size<T> : std::integral_constant<std::size_t, 2> { };

template<std::size_t I, rbe::dsrl::decomposable_frame T>
  requires(I < 2)
struct std::tuple_element<I, T> {
  using type = std::conditional_t<I == 0, typename T::header_return_type, typename T::payload_return_type>;
};

// ===== rbe/framing/dsrl/detail/payload_size.hpp =====

/**
 * @file payload_size.hpp
 * @date 29/09/2026
 * @brief Customization point for the byte extent of a frame's payload
 */


// --- STD ---

namespace rbe::dsrl::detail {

template<typename T>
concept member_size = requires(T const& payload) {
  { payload.size() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept member_length = requires(T const& payload) {
  { payload.length() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept adl_payload_size = requires(T const& payload) {
  { payload_size(payload) } -> std::convertible_to<std::size_t>;
};

struct payload_size_fn {
  template<member_length T>
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.length();
  }

  template<typename T>
    requires(not member_length<T> and member_size<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload.size();
  }

  template<typename T>
    requires(not member_length<T> and not member_size<T> and adl_payload_size<T>)
  [[nodiscard]] constexpr auto operator()(T const& payload) const -> std::size_t {
    return payload_size(payload); // found by ADL in T's own namespace
  }
};

inline constexpr payload_size_fn payload_length {};

} // namespace rbe::dsrl::detail

// --- STD ---

namespace rbe::dsrl {

/**
 * @brief A frame view over a header and the payload it describes
 *
 * frame adds to base_frame the two primitives in their default form. make() is the canonical composition
 * of the three building blocks -- locate the payload (header<H>::make), settle its extent
 * (try_payload_extent) and build the view (try_construct_payload) -- and length() follows the same precedence as
 * the extent: a header that declares a length is the authority, otherwise header plus payload.
 */
template<frame_header HeaderType, frame_payload PayloadType>
class frame : public base_frame<HeaderType, PayloadType> {
  using base = base_frame<HeaderType, PayloadType>;

public:
  // --- Type traits ---

  using typename base::buffer_type;
  using typename base::header_return_type;
  using typename base::header_type;
  using typename base::payload_return_type;
  using typename base::payload_type;
  using typename base::size_type;

  // --- Factory static member function ---

  [[nodiscard]] static constexpr auto make(buffer_type const data) -> std::optional<frame> {
    // safely constructs the header
    auto const hdr = header_return_type::make(data);
    if (not hdr.has_value()) {
      return std::nullopt;
    }

    // trims the paylaod to the extent the header declares
    auto const payload_extent = try_payload_extent<payload_type>(*hdr, data.subspan(hdr->length()));
    if (not payload_extent.has_value()) {
      return std::nullopt;
    }

    // builds the payload
    return try_construct_payload<payload_type>(*hdr, *payload_extent).transform([&](payload_return_type const payload) {
      return frame {*hdr, payload};
    });
  }

  // --- Constructors ---

  /// precondition: data holds the whole frame, i.e. make(data) would succeed
  constexpr explicit frame(buffer_type const data) : frame(resolve(data)) { }

  // --- Member functions ---

  [[nodiscard]] constexpr auto length() const -> size_type {
    if constexpr (buffer_delimited_frame<frame>) {
      return this->buffer().size();
    }
    else if constexpr (header_return_type::delimits_payload) {
      return this->header().frame_length();
    }
    else {
      return this->header().length() + detail::payload_length(this->payload());
    }
  }

private:
  constexpr frame(header_return_type const hdr, payload_return_type const payload) : base(hdr, payload) { }

  /// The narrow counterpart of make(): the same three steps, assuming their preconditions instead of checking them
  [[nodiscard]] static constexpr auto resolve(buffer_type const data) -> frame {
    auto const hdr   = header_return_type {data};
    auto const bytes = payload_extent<payload_type>(hdr, data.subspan(hdr.length()));
    return frame {hdr, construct_payload<payload_type>(hdr, bytes)};
  }
};

} // namespace rbe::dsrl

// ===== rbe/framing/srl/frame.hpp =====

/**
 * @file frame.hpp
 * @date 08/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::srl {

template<frame_header HeaderType, frame_payload PaylaodType>
class frame {
  // TODO:
};


} // namespace rbe::srl

// ===== rbe/framing/value_type.hpp =====

/**
 * @file value_type.hpp
 * @date 11/09/2026
 * @brief Value type converter from frame
 */


// --- Includes ---

// --- STD ---


namespace rbe {

// TODO:
template<frame_header HeaderType, frame_serder_payload PayloadType>
using value_type = void;

} // namespace rbe

// --- STD ---

namespace rbe {

/**
 * @brief Frame serder builder class
 *
 * This empty class's sole purpose is to offer a single type with both serialization
 * and deserialization type aliases following serder_traits. This way, the user can define
 * their frame only once and extract both srl and dsrl types from it.
 */
template<frame_header HeaderType, frame_serder_payload PayloadType>
struct frame {
  using header_type  = HeaderType;
  using payload_type = PayloadType;
  using dsrl_type    = dsrl::frame<header_type, detail::to_dsrl_t<payload_type>>;
  // using srl_type     = srl::frame<header_type, detail::to_srl_t<payload_type>>;
  // using value_type   = rbe::value_type<header_type, payload_type>;
};

} // namespace rbe

// ===== rbe/framing/many.hpp =====

/**
 * @file many.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// ===== rbe/framing/srl/many.hpp =====

/**
 * @file many.hpp
 * @date 11/09/2026
 * @brief Short description
 *
 * Longer description
 */


// --- Includes ---

// --- STD ---

namespace rbe::srl {

template<typename T>
class many { };

} // namespace rbe::srl

// --- STD ---

namespace rbe {

template<frame_serder T>
struct many : detail::many_tag {
  using dsrl_type = dsrl::many<typename T::dsrl_type>;
  // TODO: using srl_type  = srl::many<typename T::srl_type>;
};

} // namespace rbe

// ===== rbe/srl.hpp =====

/**
 * @file srl.hpp
 * @date 08/08/2026
 * @brief Serialization umbrella header
 */



// ===== rbe/srl/serialize.hpp =====

/**
 * @file serialize.hpp
 * @date 02/07/2026
 * @brief Serialization routines for writing wirable types to byte buffers.
 *
 * Provides overloads for trivially wirable types, custom-wirable types,
 * integral primitives, and general wirable aggregates.
 */


// --- Includes ---

// ===== rbe/srl/detail/serialize_impl.hpp =====

/**
 * @file serialize_impl.hpp
 * @date 02/07/2026
 * @brief Context-aware dispatch implementing serialization for wirable types.
 *
 * Internal machinery: fast-path/custom/primitive/aggregate/range overloads threading a `context`
 * through recursive calls so annotations propagate correctly through arbitrarily deep nesting. The
 * public entry point (`rbe::serialize`, in `rbe/srl/serialize.hpp`) always starts from the default
 * context and dispatches here.
 */


// --- Includes ---

// --- STD ---

namespace rbe::detail {

// Forward declarations so every overload below can recurse into any sibling regardless of
// definition order -- e.g. an aggregate containing an array member needs to see the range
// overload, and a range of aggregates needs to see the aggregate overload right back.

template<trivially_wirable T, context Ctx = context {}>
  requires(Ctx == context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<custom_wirable T, context Ctx = context {}>
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<wirable_class T, context Ctx = context {}>
  requires(not custom_wirable<T> and not wirable_range<T> and (not trivially_wirable<T> or Ctx != context {}))
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

template<wirable_range T, context Ctx = context {}>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto serialize(std::span<std::byte>, T const&) -> std::size_t;

/**
 * Fast path: nothing has forced a non-default context onto this member, so it's safe to serialize
 * with a single direct memory copy -- exactly today's behavior/optimization, unchanged.
 */
template<trivially_wirable T, context Ctx>
  requires(Ctx == context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  memcpy_constexpr(out, value);
  return sizeof(value);
}

/**
 * Serializes a custom-wirable type via its `custom<T>::serialize` specialization. The wire format is
 * entirely user-defined, so the ambient context never applies to it.
 */
template<custom_wirable T, context Ctx>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return rbe::custom<std::remove_cvref_t<decltype(value)>>::serialize(out, value);
}

/**
 * A primitive that would otherwise be memcpy-able alone, but an ancestor's annotation forces a
 * specific byte order onto it -- write it with that order applied explicitly.
 */
template<trivially_wirable_primitive T, context Ctx>
  requires(Ctx != context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  memcpy_constexpr(out, normalize_endianness<T, Ctx.endianness>(value));
  return sizeof(value);
}

/**
 * @brief Serializes a wirable aggregate into a buffer member-by-member.
 *
 * Iterates over each non-static data member, threading the resolved ambient context one hop further
 * down at each member (`merge_context`) so annotations propagate correctly through arbitrarily deep
 * unannotated nesting, and recursively serializes it into the corresponding offset within the output
 * buffer. Reached whenever `T` isn't trivially wirable on its own, or an ancestor's context forces
 * something even though `T` would otherwise have taken the fast path above.
 *
 * @tparam T The aggregate type to serialize. Must satisfy `wirable_class`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer, including any trailing padding -- matching `T`'s
 *         wire size exactly, the same way the memcpy fast path's `sizeof(value)` always does.
 */
template<wirable_class T, context Ctx>
  requires(not custom_wirable<T> and not wirable_range<T> and (not trivially_wirable<T> or Ctx != context {}))
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using std::ranges::to;

  static constexpr auto local   = merge_context(Ctx, ^^T);
  static constexpr auto wire    = get_wire_layout<T, local>();
  static constexpr auto members = nsdm(^^T) | to<static_array>();

  template for (constexpr auto [layout, member]: std::views::zip(wire.members, members)) {
    using member_type = [:type_of(member):];
    serialize<member_type, merge_context(local, member)>(
        out.subspan<layout.offset.bytes, layout.size>(), value.[:member:]
    );
  }

  return wire.size;
}

/**
 * @brief Serializes a wirable range (`std::array`, a bounded C array, ...) element-by-element.
 *
 * Reached whenever the range's elements aren't uniformly memcpy-able as one block: either they
 * aren't trivially wirable on their own, or an ancestor's context forces a byte order onto them that
 * their in-memory representation doesn't already have. Each element goes through its own dispatch, so
 * a range of aggregates or of primitives needing a byte swap both work correctly.
 *
 * @tparam T The range type to serialize. Must satisfy `wirable_range`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The range to serialize.
 * @return Number of bytes written to the buffer.
 */
template<wirable_range T, context Ctx>
  requires(not trivially_wirable_range<T> or Ctx != context {})
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  using element_type = std::ranges::range_value_t<T>;

  std::size_t bytes_written = 0;
  for (auto const& element: value) {
    bytes_written += serialize<element_type, Ctx>(out.subspan(bytes_written), element);
  }

  return bytes_written;
}

} // namespace rbe::detail

// --- STD ---

namespace rbe {

/**
 * @brief Serializes a wirable value into a buffer.
 *
 * This is the single public entry point for serialization: it always starts from the default
 * (empty) ambient context and dispatches internally, via `detail::serialize`, to whichever of the
 * fast-path/custom/primitive/aggregate implementations applies to `T`.
 *
 * Preconditions:
 *   - The output buffer must be large enough to hold the serialized data
 *
 * @tparam T The type to serialize. Must satisfy `wirable`.
 * @param out Output buffer large enough to hold the serialized data.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer.
 */
template<wirable T>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return detail::serialize<T>(out, value);
}

/**
 * @brief Serializes a wirable value into a buffer if the buffer is large enough.
 *
 * Serializes a wirable value into a buffer if the buffer is large enough to hold
 * the serialized data.
 *
 * @tparam T The type to serialize. Must satisfy `wirable`.
 * @param out Output buffer.
 * @param value The object to serialize.
 * @return Number of bytes written to the buffer if successful, or std::nullopt if the buffer is too small.
 */
template<wirable T>
constexpr auto try_serialize(std::span<std::byte> const out, T const& value) -> std::optional<std::size_t> {
  return out.size() >= wire_size_of<T>() ? std::optional<std::size_t>(serialize(out, value)) : std::nullopt;
}

} // namespace rbe
