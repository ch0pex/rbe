/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotation_info.hpp
 * @date 17/09/2026
 * @brief A single, whole RBE annotation: a strongly typed `std::meta::info`
 *
 * Every query in the annotation system deals in `annotation_info`, never in bare reflections and
 * never in annotation *types*: the wrapped reflection is always the annotation's VALUE, so the value
 * is never lost along the way and there is no second, parallel "values" API to keep in sync.
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotation.hpp>
#include <rbe/core/detail/introspection.hpp>

// --- STD ---
#include <meta>
#include <optional>
#include <type_traits>

namespace rbe::detail {

class annotation_info {
public:
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
    if (not payload) {
      return std::nullopt;
    }
    using payload_fn = T (*)(std::meta::info);
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

private:
  std::meta::info info_;
};

/// Filter predicate factory: keep only annotations belonging to dimension `dim`.
consteval auto by_dimension(std::meta::info const dim) {
  return [dim](annotation_info const a) { return a.dimension() == dim; };
}

} // namespace rbe::detail
