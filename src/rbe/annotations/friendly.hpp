/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file friendly.hpp
 * @date 21/10/2026
 * @brief The `friendly` annotation: a type's consent to put its non-public members on the wire
 */

#pragma once

// --- Includes ---
#include <rbe/annotations/annotation_concepts.hpp>

// --- STD ---

namespace rbe {

namespace detail {

struct friendly_dim {
  // not `unique`: a type holding two members of one friendly type is not annotated twice
  static constexpr auto kind = dimension_kind::exclusive | dimension_kind::type_only;
};

struct friendly_tag {
  using dimension = friendly_dim;
};

} // namespace detail

/**
 * @brief Grants RBE access to the non-public members of the annotated type.
 *
 * A class is wirable only if every member it declares is public, unless the class itself is annotated
 * `friendly`. Members are private to protect an invariant, and the member-wise decode that makes a class
 * wirable writes them without going through any constructor, so RBE does not take that decision on the
 * author's behalf. The annotation is the author saying that the members are the representation and that an
 * object built from the wire is valid by construction: the role `friend` plays for other serialization
 * libraries, under a name that can be spelled.
 *
 * It is written on the type, never on a member, and it is not inherited by the types of the members: each
 * class with non-public members speaks for itself. It combines freely with annotations of other dimensions.
 */
inline constexpr detail::annotation_kind<detail::friendly_tag> friendly {};

} // namespace rbe
