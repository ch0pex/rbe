/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
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

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/dimension.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <concepts>
#include <meta>
#include <type_traits>

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
