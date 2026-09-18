/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file id.hpp
 * @date 07/09/2026
 * @brief Message id annotations
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/detail/annotated_nsdm.hpp>
#include <rbe/annotations/detail/annotation_info.hpp>
#include <rbe/annotations/detail/utils.hpp>
#include <rbe/core/detail/introspection.hpp>
#include <rbe/core/detail/invoke_concept.hpp>

// --- STD ---
#include <concepts>
#include <stdexcept>

namespace rbe {

namespace detail {

/// The `rbe::id(value)` annotation written on `type`, or nullopt -- the bare marker does not count.
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
      return is_class_type(normalize_type(entity))
             and invoke_concept(^^std::equality_comparable, {ann.value_type()});
    }
    // rbe::id: marks the field the id is read from
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
 * @brief The id `T` declares, as the type it was written with.
 *
 * `id_tag` deduces its value type, so the id comes back exactly as spelled in the annotation --
 * enum class and all -- rather than widened to some fixed integer type.
 *
 * @note A static_assert rather than a `requires` clause: it can name the annotation that is missing,
 * where a failed constraint would report only that no overload matched.
 */
template<typename T>
  requires(identifiable<T>)
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
template<typename T>
using id_type_of = std::remove_cvref_t<decltype(id_of<T>())>;

} // namespace rbe
