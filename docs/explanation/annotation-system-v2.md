# Annotation System Redesign

> **Status: designed, not implemented.** This document records a design that has been worked out and partially verified against the real GCC 16 reflection implementation, but no code has been written yet. It is not linked from the site navigation. Once implemented, the relevant parts should move into [Design Overview](design-overview.md) and the [Annotations Reference](../reference/annotations.md), and this file should either be deleted or trimmed down to the parts that remain useful as rationale.

## Motivation

The current annotation system (`rbe/annotations/`) identifies an "RBE annotation" by requiring its type to derive from `rbe::detail::base_annotation`, and records which annotations conflict with which ("dimensions") in a central list in `annotations/detail/correctness.hpp`, physically disconnected from where the annotations themselves are defined.

This has three concrete problems:

1. **Base-class identity excludes enums.** `rbe::endian` conversion in `core/memory_layout.hpp` (`endiannes_from_annotation`) is a hand-written `if`/`else` chain mapping each endianness annotation *type* to an `endian::order` *value*. The natural fix — making `rbe::little`/`rbe::big` literally be `std::endian::little`/`std::endian::big` — is impossible today, because `std::endian` cannot derive from `base_annotation`.
2. **Dimension membership is centrally registered, not colocated.** Adding a new annotation to an existing dimension means editing two files: the annotation's own header, and `correctness.hpp`'s hand-maintained `types_list(...)` for that dimension.
3. **No uniform "range of annotations" entry point.** Call sites juggle two concepts (`annotation` vs `annotation_list`) and `has_annotation` has two overloads instead of one.

Two more requirements surfaced while designing the fix:

4. **Some dimensions have a default value when nothing is explicitly annotated** (e.g. endianness defaults to native byte order). Today that default is hard-coded inside `core/memory_layout.hpp`, not declared anywhere near the dimension itself.
5. **Some annotations only make sense in one syntactic position** — e.g. a hypothetical `count(...)` annotation (see [Open problem: variable-length fields](#open-problem-variable-length-fields) below) only makes sense on a struct member, never on a type declaration.

## Core mechanism: `annotation_traits<T>`

Replace `base_annotation` inheritance with a **specializable trait**, mirroring the customization-point idiom this codebase already uses for `rbe::custom<T>` (`core/custom.hpp`, detected via `is_complete_type(substitute(^^custom, {info}))`):

```cpp
template<class T>
struct annotation_traits; // primary template, intentionally incomplete
```

A type `T` is a first-class RBE annotation **iff `annotation_traits<T>` has been specialized** — completeness of the specialization is the identity check, not inheritance. This works identically for empty tag structs, payload-carrying structs, and plain enumerations, because template specialization doesn't care what kind of type `T` is.

The same specialization optionally carries additional members, each answered by a small reflection-based accessor:

| Member (optional) | Meaning | Accessor |
|---|---|---|
| `using dimension = SomeDimTag;` | Which dimension `T` belongs to. Absent ⇒ a "free" annotation with zero correctness ceremony (e.g. `fmt`). | `dimension_of(type) -> std::meta::info` |
| — (on the dimension tag itself) `static constexpr dimension_kind kind` | How the dimension is enforced (see below). Mandatory on every dimension tag. | `kind_of(dim) -> dimension_kind` |
| — (on the dimension tag itself) `static constexpr auto default_value` | The value assumed when no annotation of this dimension is present anywhere in scope. Optional; only meaningful for value-bearing dimensions. | `default_value_of<T>(dim) -> T` |
| — (on `annotation_traits<T>` itself) `static constexpr auto scope` | Where `T` is syntactically allowed: struct-level, member-level, or both (default). | *(new, not yet designed in full — see below)* |

```cpp
enum class dimension_kind : std::uint8_t {
  exclusive, ///< at most one annotation of the dimension may appear within a single annotation range
  unique,    ///< each annotation of the dimension may independently appear at most once across the whole (deep) type
};
```

`exclusive` is the current `alignment`/`endianness` rule (`[[=rbe::little, =rbe::big]]` conflicts). `unique` is the current `id`/`length` rule — `id` and `length` are *not* mutually exclusive with each other, but each must not repeat across the whole (possibly nested) message.

### Adding an annotation to a dimension becomes one colocated block

```cpp
// alignment.hpp
struct alignment_dim { static constexpr auto kind = detail::dimension_kind::exclusive; };

inline constexpr struct {} pack {};
inline constexpr struct {} align {};

template<> struct rbe::detail::annotation_traits<decltype(rbe::pack)>  { using dimension = rbe::alignment_dim; };
template<> struct rbe::detail::annotation_traits<decltype(rbe::align)> { using dimension = rbe::alignment_dim; };
```

No edit to any other file is needed — `correctness.hpp` no longer hand-lists which annotations exist per dimension; `well_annotated` discovers, for a given type, *which dimensions are actually used among its attached annotations*, and checks each one generically by its `kind`. A brand new dimension (not just a new annotation within an existing one) therefore requires **zero** edits to `well_annotated`/`correctness.hpp` — it falls out of the generic loop automatically.

### `derive<...>` is unaffected

`rbe::derive<Args...>` / `pack_le` / `pack_be` / `debug` need no changes: `annotations_t<Args...>` never depended on `base_annotation` for its own mechanics (`is_annotation_list` already checks `template_of(info) == ^^annotations_t` directly).

## Solving pain point 1: reusing `std::endian` directly

```cpp
// endianness.hpp
struct endianness_dim {
  static constexpr auto kind          = detail::dimension_kind::exclusive;
  static constexpr auto default_value = endian::order::native; // resolves to little or big at compile
                                                                 // time, conditionally on the target platform
};

struct bits { // payload-carrying: msb/lsb, NOT reducible to a plain byte order
  std::uint8_t msb, lsb;
  constexpr explicit bits(std::uint8_t const msb, std::uint8_t const lsb) : msb(msb), lsb(lsb) {}
};

inline constexpr auto little = endian::order::little; // literally std::endian::little
inline constexpr auto big    = endian::order::big;    // literally std::endian::big

template<> struct rbe::detail::annotation_traits<rbe::endian::order> { using dimension = rbe::endianness_dim; };
template<> struct rbe::detail::annotation_traits<rbe::bits>          { using dimension = rbe::endianness_dim; };
```

**`rbe::native` is removed entirely** rather than kept as a distinct tag. The "no explicit endianness annotation anywhere in scope" case already resolves through the dimension's `default_value = endian::order::native`, which is resolved conditionally, at compile time, to whichever of `little`/`big` matches the target platform (`std::endian::native`'s own standard-mandated behavior — no runtime branch needed). A dedicated marker type was never actually necessary for that: it existed in the first draft of this design only so the *default* could be represented distinctly from `little`/`big`'s shared `endian::order` type, out of a concern that sharing the type would make the dimension's exclusivity check platform-dependent. That concern doesn't hold up: exclusivity (`dimension_kind::exclusive`) is enforced by *counting how many annotations of the dimension's type are present*, never by comparing their values — `[[=rbe::little, =rbe::native]]` would have been rejected as two `endian::order`-typed annotations on every platform, identically, whether or not `native` happened to equal `little` on the machine compiling it. So there was no real hazard, just unnecessary ceremony.

**Trade-off worth flagging explicitly:** this removes the ability to *write* `=rbe::native` at all. [REQ-062](requirements.md#endianness-and-packing) currently lists `=rbe::native` alongside `=rbe::little`/`=rbe::big` as a required explicit annotation, and [REQ-078](requirements.md#implicit-annotations) implies an explicit-endianness-required safety mode where every member must carry *some* endianness annotation — under this change, a member that wants native order would have no explicit spelling available to satisfy that mode with, and would be forced to rely on the implicit default instead. Either REQ-062/REQ-078 need revising to drop the explicit-`native` requirement, or `rbe::native` needs to come back as an explicit, writable annotation of type `endian::order` (same type as `little`/`big`, just always evaluating to `std::endian::native` — which, per the paragraph above, is equally safe to share the type with them). Flagging this rather than silently resolving it either way.

**Why `bits` doesn't break this:** it joins the dimension (participates in exclusivity: `[[=rbe::little, =rbe::bits(3,0)]]` is correctly rejected) via one trait line, but it simply isn't the type asked for when extracting an `endian::order` value — see `value_of<T>` below, which returns "not found" rather than a wrong answer for annotations of the wrong type. This is the proof that the design isn't "make every dimension member an enum" — heterogeneous representations within one dimension are the normal case, not a special case.

### Generic value extraction and default resolution

```cpp
template<typename T>
consteval auto value_of(std::meta::info const a) -> std::optional<T> {
  if (remove_cvref(type_of(a)) != ^^T) return std::nullopt;
  return extract<T>(a);
}

template<typename T>
consteval auto default_value_of(std::meta::info const dim) -> T {
  for (auto const member : static_data_members_of(dim, default_context))
    if (has_identifier(member) and identifier_of(member) == "default_value") return extract<T>(member);
  throw std::meta::exception("dimension has no default_value for the requested type", dim);
}

// Searches entity's REQ-058..061 annotation range (its own annotations, unioned with its type's
// own annotations for a member -- see annotation_range() in the range-API section below) for the
// first annotation that yields a T.
template<typename T>
consteval auto resolve_in_scope(std::meta::info const entity) -> std::optional<T> {
  for (auto const a : annotation_range(entity))
    if (auto v = value_of<T>(a)) return v;
  return std::nullopt;
}

// Replaces endiannes_from_annotation + has_endianness_annotation + get_member_endianness in one
// function, generically, for ANY value-bearing dimension -- not just endianness.
template<typename T>
consteval auto resolve(std::meta::info const parent, std::meta::info const member, std::meta::info const dim) -> T {
  if (auto v = resolve_in_scope<T>(member)) return *v;
  if (auto v = resolve_in_scope<T>(parent)) return *v;
  return default_value_of<T>(dim);
}
```

`core/memory_layout.hpp` then collapses to:

```cpp
consteval auto get_member_endianness(std::meta::info const parent, std::meta::info const member) -> endian::order {
  return detail::resolve<endian::order>(parent, member, ^^endianness_dim);
}
```

This is the mechanism that answers pain point 4 (per-dimension defaults) at the same time as pain point 1 (no more hand-written conversion chain): the default lives declared on the dimension, `resolve`/`resolve_in_scope`/`value_of` are the *only* three functions in the whole system that ever deal with extracting a semantic value from a reflection, and any future value-bearing dimension reuses them for free.

## Solving pain point 3: one range entry point

`views::rbe_annotations` (the existing `range_adaptor_closure`) stays the low-level piece that normalizes *anything* — a scalar annotation, a `derive<...>` list, or the raw output of `std::meta::annotations_of` — into the flat range of concrete annotation instances it denotes.

On top of it, a single function-call entry point replaces every hand-written recursive traversal in the system (`annotation_types_of`, `deep_annotation_types_of`, and the `resolve_in_scope` walk from above all become 1–3 line compositions of this plus ordinary `std::ranges`/`std::views` algorithms):

```cpp
namespace rbe::detail {

/// The RBE annotations written directly on `entity` (a type or a non-static data member) --
/// derive<...> lists already expanded, non-RBE attributes already filtered out.
consteval auto rbe_annotations(std::meta::info const entity) -> std::vector<std::meta::info> {
  return std::meta::annotations_of(entity) | views::rbe_annotations | std::ranges::to<std::vector>();
}

/// The REQ-058..061 "annotation range": for a member, its own annotations unioned with its
/// type's own annotations; for a type, just its own annotations.
consteval auto annotation_range(std::meta::info const entity) -> std::vector<std::meta::info> {
  auto result = rbe_annotations(entity);
  if (is_nonstatic_data_member(entity)) {
    result.append_range(rbe_annotations(type_of(entity)));
  }
  return result;
}

/// annotation_range(entity), recursively unioned with every nested non-static data member's.
consteval auto deep_annotations(std::meta::info const entity) -> std::vector<std::meta::info> {
  auto result = annotation_range(entity);
  if ((is_type(entity) and not is_class_type(entity)) or is_nonstatic_data_member(entity)) {
    return result;
  }
  for (auto const member : nsdm(entity)) {
    result.append_range(deep_annotations(member));
  }
  return result;
}

/// Filter predicate factory: keep only annotations belonging to dimension `dim`.
consteval auto by_dimension(std::meta::info const dim) {
  return [dim](std::meta::info const a) { return dimension_of(normalize_type(a)) == dim; };
}

} // namespace rbe::detail
```

Every correctness check and value-resolution step is then genuinely one expression built from `std::ranges`/`std::views` over these three ranges, instead of a bespoke recursive function per concern:

```cpp
// has_annotation: single code path for a scalar annotation OR a derive<...> list
consteval auto has_annotation(std::meta::info const entity, auto const value) -> bool
  requires annotation<decltype(value)> or annotation_list<decltype(value)>
{
  auto const haystack = annotation_range(entity);
  return std::ranges::all_of(views::rbe_annotations(value), [&](std::meta::info const needle) {
    return std::ranges::find(haystack, needle) != haystack.end();
  });
}

// exclusive dimension: at most one match within the annotation range
consteval auto satisfies_dimension(std::meta::info const entity, std::meta::info const dim) -> bool {
  return std::ranges::count_if(annotation_range(entity), by_dimension(dim)) <= 1;
}

// unique dimension: every dimension-matching annotation in the deep range appears at most once
consteval auto satisfies_uniqueness(std::meta::info const entity, std::meta::info const dim) -> bool {
  auto const in_dim = deep_annotations(entity) | std::views::filter(by_dimension(dim)) | std::ranges::to<std::vector>();
  return std::ranges::all_of(in_dim, [&](std::meta::info const a) { return std::ranges::count(in_dim, a) <= 1; });
}

```

`resolve_in_scope<T>` (in the endianness section above) is itself one more example of this: it searches `annotation_range(entity)` for the first annotation `value_of<T>` recognizes, which is exactly what let it drop its own hand-written member→type recursion once `annotation_range` already does that union.

`by_dimension` composes with `std::views::filter` the same way any other predicate would (`rbe_annotations(^^T) | std::views::filter(by_dimension(^^endianness_dim))`), and presence checks compose with `std::ranges::find`/`std::ranges::contains` directly on the ranges above — the annotation-query surface becomes ordinary range algorithms end to end, not a growing set of bespoke functions.

## Empirically verified against GCC 16.1.1

Before committing to this design, a set of standalone `.cpp` probes were compiled (`-freflection -fsyntax-only`) against this machine's GCC 16.1.1 to de-risk the reflection API usage, since C++26 reflection is still an experimental, fast-moving feature:

- ✅ `is_complete_type(substitute(^^annotation_traits, {T}))` correctly detects specialization for both a plain struct and an `enum class`, and correctly reports "incomplete" for an unspecialized type.
- ✅ `members_of(traits_specialization, ctx)` + `is_type_alias`/`identifier_of` correctly locates a nested `using dimension = ...;`; `static_data_members_of` + `identifier_of` correctly locates `static constexpr kind`/`default_value`.
- ✅ `type_of(annotation_value) == ^^T` (after `remove_cvref`) reliably identifies an annotation's type, including when the annotation is a value of type `T` shared by several distinct annotations (e.g. `little`/`big` both being `endian::order`).
- ✅ `extract<T>(a) == extract<T>(b)` reliably compares two annotation instances **by their real, extracted value**, regardless of how each was spelled at the call site (a named `constexpr` variable vs. a bare enumerator expression).
- ✅ An overloaded `consteval` "factory" function (e.g. two overloads both named `count`, taking different argument types and returning different underlying annotation types) works fine as the operand of `[[=...]]` — useful if a future annotation wants one call-syntax name backed by more than one concrete type.
- ⚠️ **`std::meta::info` equality is *not* reliable for comparing annotation *values* directly.** Two reflections that denote the same constant (e.g. the object `little_v` vs. the bare expression `e_kind::little`) compare **unequal** with plain `==`, even though they print identically in diagnostics. This invalidated the first draft of this design, which planned to normalize annotation instances to a "canonical value" reflection (via `constant_of`) and compare *those* directly. The fix — and the reason the design above never compares value-reflections with `==` — is to always route semantic comparisons through `extract<T>(...) == extract<T>(...)`, comparing the real C++ values, never the reflections. Type-level comparisons (`normalize_type`/`type_of`, used throughout for identity and dimension lookup) remain fully reliable and needed no change.

This last finding is also why duplicate-detection (`has_duplicates`, used by `verify_no_local_duplications`) does **not** need to change: it already only ever compared annotation *types* (via `normalize_type`), never values, and type-level comparison was never the broken part. Two different-valued annotations sharing a type (`little`/`big` both being `endian::order`) will still be correctly rejected when both appear together — not because "duplicate" catches it, but because the dimension's `exclusive` check (which only ever needed to count *how many* annotations of the dimension's *type* are present, never which values) already does.

## Open: annotation scope (struct-level vs. member-level only)

Not yet fully designed. The identity/dimension mechanism above needs one more optional trait member, tentatively:

```cpp
enum class annotation_scope : std::uint8_t { any, member_only, type_only };
// on annotation_traits<T>: static constexpr auto scope = annotation_scope::member_only;
```

defaulting to `any` so no existing annotation needs to change. Verified feasible: it's possible to detect, generically, whether an annotation of a given type showed up directly in `std::meta::annotations_of(a_type)` (struct-level) as opposed to `std::meta::annotations_of(a_member)` (member-level) — the two are already gathered separately at the call sites that matter, they're just unioned together today for dimension-membership purposes. `well_annotated` would gain a third generic check, symmetric to the `kind` check, that rejects an annotation found at a scope it doesn't declare support for.

## Open problem: N-level annotation propagation

`resolve<T>(parent, member, dim)` (above) only looks one level up: the member's own scope, then its *immediate* containing struct, then the dimension's default. [REQ-066](requirements.md#nested-structure-handling)–[070](requirements.md#nested-structure-handling) require full transitive propagation through arbitrarily deep nesting — `example/annotations.cpp`'s own worked example spells out the intent explicitly:

```cpp
struct [[=rbe::pack]] MiddleNode {
  Leaf leaf;             // Implicit native endianness, derived pack
  std::uint32_t valor2;  // Implicit native endianness
};

struct [[=rbe::big]] ParentNode {
  // MiddleNode and Leaf annotations under ParentNode context would look like:
  // struct Leaf [[=rbe::pack, =rbe::big]] { ... };       // Leaf inherits big, transitively, through MiddleNode
  // struct MiddleNode [[=rbe::pack, =rbe::big]] { ... }; // MiddleNode inherits big directly
  MiddleNode node;        // Implicit big, explicit pack
  std::uint32_t valor3;   // Implicit big, implicit align
};
```

`Leaf` and `MiddleNode` have no endianness annotation of their own; the comment documents that they should still end up resolving to `big`, inherited transitively from `ParentNode`, two and one levels up respectively. **The current shipped code does not do this** — `core/memory_layout.hpp`'s `get_wire_layout<T>()` is computed once per *type*, in isolation (`static constexpr auto wire = get_wire_layout<T>();`), with no notion of "what struct am I embedded in this time." `srl/detail/serialize_impl.hpp` (currently dead code, not `#include`d anywhere) already has a `// TODO: With 2 deep structures this is not gonna work, should I propagate toppest endianness to bottom?` marking exactly this gap.

**Why this is genuinely hard, not just a missing loop:** the effective annotation of an unannotated nested field is a property of the *specific usage path* from wherever resolution starts down to that field — not of the nested type alone. The same `Leaf` could be embedded in one context that implies `big` and, elsewhere in the same program, in a context that implies `little`; `get_wire_layout<Leaf>()` cannot be a single cached answer if `Leaf` ever relies on an inherited (rather than explicit) endianness.

`std::meta::parent_of(r)` was suggested as a tool here and is worth being precise about. Verified against GCC 16.1.1:

- ✅ `parent_of(member)`, given a *non-static data member* reflection, reliably returns the class it's declared in (`parent_of(nonstatic_data_members_of(^^MiddleNode, ctx)[0]) == ^^MiddleNode`). This is genuinely useful: `resolve`'s explicit `parent` parameter becomes redundant and can be derived from `member` instead, so the two-argument `(parent, member)` signatures throughout collapse to one.
- ⚠️ `parent_of`, given a *type* reflection, returns that type's **lexical declaration scope** (the namespace/class it was *written* in), not any struct that merely *uses* it as a field's type: `parent_of(^^Leaf)` is a namespace, not `^^MiddleNode`, confirmed by `static_assert`. So `parent_of` cannot, by itself, answer "what is `Leaf` embedded in this time" — it does not solve the path-dependence problem above. Concrete counter-example, also verified: embed the *same* `MiddleNode` in a second, unrelated struct (`struct OtherOuter { MiddleNode also_here; };`). Walking `parent_of` up from either embedding's member, through the type, lands on the exact same result (`MiddleNode`'s declaration namespace) regardless of which outer struct — or how many — actually embed it. `parent_of(^^MiddleNode)` genuinely cannot distinguish "embedded in `ParentNode`" from "embedded in `OtherOuter`" from "embedded nowhere," because that information was never recorded on the type in the first place — composition (HAS-A) isn't visible to a query about lexical declaration scope.

The path *is* already being walked, though — by the existing recursive traversal in `serialize`/`deserialize`/`get_wire_layout` itself, which visits `ParentNode` → `node` → `MiddleNode` → `leaf` → `Leaf` in order as it lays a message out. `parent_of` being ruled out above confirms the original idea for this problem is the right one: the recursion itself, not a reflection query, has to carry the answer down as it descends — each recursive step should receive whatever was already resolved by its caller one level up and fall back to *that* first, instead of starting its lookup over from scratch and falling straight to the dimension's global default the moment its *own* immediate scope has nothing explicit.

**Where the context lives, and why the public API doesn't need to change.** `rbe::serialize(buffer, value)`, called directly by a user, is always the *root* of its own operation — `T` genuinely has no ambient context at that point (as the `OtherOuter` counter-example above shows, the same `T` can be embedded with different implied annotations in several unrelated outer structs, so "the" context of a bare type isn't even well-defined outside of one specific usage path). The public signature stays exactly as it is today. What needs the ambient value is only the *internal* recursive step, when `serialize`'s own body reaches a member that is itself a `wirable_class` and needs to lay *that* out too — and that's an ordinary compile-time template argument, not a value threaded through a runtime function parameter, so "can a reflection be passed to a non-`consteval` function" never comes up.

Concretely, one **context value aggregating every value-bearing dimension** (`endianness` today, any future one later) flows down through the recursion, refined at each level. In pseudocode:

```
# context: one field per value-bearing dimension, e.g. { endianness }

function merge(ambient_context, entity):
    result = copy of ambient_context
    if entity has its own explicit endianness annotation:
        result.endianness = that annotation's value
    return result                          # otherwise, keep whatever was inherited

function wire_layout(T, context):
    for each member of T:
        member_context = merge(context, member)
        emit { offset, size, endianness: member_context.endianness } for that member

# ── serialize ──────────────────────────────────────────────────────
function serialize(value: T, out_buffer, context = <dimension defaults>):
    local  = merge(context, T)                     # T's own annotations override the inherited ambient
    layout = wire_layout(T, local)
    for (member, member_layout) in layout:
        if member.type is itself a struct:
            member_context = merge(local, member)  # one hop further down, never reset
            serialize(value.member, out_buffer at member_layout.offset, member_context)
        else:
            write_primitive(value.member, out_buffer at member_layout.offset, member_layout.endianness)

# ── deserialize: eager (mirror image of serialize) ───────────────────
function deserialize_eager(in_buffer, T, context = <dimension defaults>) -> T:
    local  = merge(context, T)
    layout = wire_layout(T, local)
    result = new T
    for (member, member_layout) in layout:
        if member.type is itself a struct:
            member_context = merge(local, member)
            result.member = deserialize_eager(in_buffer at member_layout.offset, member.type, member_context)
        else:
            result.member = read_primitive(in_buffer at member_layout.offset, member_layout.endianness)
    return result

# ── deserialize: lazy ─────────────────────────────────────────────────
# constructed once, queried many times via field() -- so the context must be REMEMBERED,
# not recomputed from nothing on every access
class lazy_view(in_buffer, T, context = <dimension defaults>):
    local = merge(context, T)                      # resolved ONCE, at construction, not per field()

    function field(name):
        member, member_layout = lookup(wire_layout(T, local), name)
        if member.type is itself a struct:
            member_context = merge(local, member)
            # nested-struct fields already collapse to an eager read even under a lazy top-level
            # call in the current code -- keep that shape, just resolve ITS context too:
            return deserialize_eager(in_buffer at member_layout.offset, member.type, member_context)
        else:
            return read_primitive(in_buffer at member_layout.offset, member_layout.endianness)

# ── deserialize: in_place ──────────────────────────────────────────────
# context only matters for deciding WHETHER T qualifies at all: T's wire layout, computed with
# the correct inherited context, must equal its in-memory layout. Once that check has passed,
# the actual read is a raw reinterpretation -- no context, no per-field transform, ever:
function deserialize_in_place(in_buffer, T) -> reference to T:
    assert is_trivially_wirable(T)     # this is where context-aware layout resolution happens
    return reinterpret in_buffer as T
```

The point that generalizes beyond endianness: every recursive step resolves its *own* `local` context by overriding whatever it inherited with its *own* explicit annotations, then hands that `local` — never the original global default — to whatever it recurses into next. Nothing ever "gives up" and falls back to the dimension's global default except the very first, outermost call.

The point specific to lazy deserialization: a `lazy_view` is constructed once and then queried across possibly many separate `field(name)` calls, so its context has to be resolved once and stored as part of the view itself at construction time — recomputing it fresh on every field access would be wasteful and, if the recursion state weren't captured anywhere, impossible to do correctly for fields reached through a struct-typed field's own nested fields.

Two things worth being precise about, since the pseudocode above illustrates a *shape*, not verified code: the concrete `context`/`merge` mechanism it's built on **was** compiled and checked against GCC 16.1.1 (a plain aggregate struct usable as a non-type template parameter, refined level by level — see the two-outer-structs and three-level `ParentNode`/`MiddleNode`/`Leaf` checks above); wiring it through the *actual* `serialize`/`deserialize`/`get_wire_layout`/`dsrl::msg` C++ signatures, and extending it to the alignment dimension alongside endianness, is real implementation work, deliberately **out of scope** here — this pseudocode is the recorded shape for that future work, not a substitute for it.

The same shape, closer to real C++ syntax (illustrative — element/offset computation, `find_member`/`index_of`, and the `serialize_primitive`/`deserialize_primitive` helpers are elided or simplified; only the `context`/`merge_context` declaration itself, not the functions built on top of it, was what actually got compiled):

```cpp
struct context {
  endian::order endianness = endian::order::native; // one field per value-bearing dimension
  friend constexpr bool operator==(context, context) = default; // required: NTTPs must be structural types
};

// Overwrite ambient with whatever `entity` explicitly annotates itself; leave everything else inherited.
consteval auto merge_context(context const ambient, std::meta::info const entity) -> context {
  context result = ambient;
  if (auto v = resolve_in_scope<endian::order>(entity)) result.endianness = *v;
  return result;
}

// get_wire_layout gains the same parameter, since a member's resolved offset/size/endianness
// now depends on what was inherited, not just on T in isolation.
template<wirable_class T, context Ctx>
consteval auto get_wire_layout() -> struct_layout {
  std::vector<member_layout> members;
  for (auto const member : nsdm(^^T)) {
    auto const member_ctx = merge_context(Ctx, member); // member's own annotations win over Ctx
    members.push_back({.offset = ..., .size = ..., .endianness = member_ctx.endianness});
  }
  return {.size = ..., .members = {std::from_range, members}};
}

// ── serialize: context flows DOWN as an explicit template argument ────────────────
template<wirable_class T, context Ctx = context{}> // top-level calls seed the true dimension defaults
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  static constexpr auto local = merge_context(Ctx, ^^T);   // T's own annotations override the inherited ambient
  static constexpr auto wire  = get_wire_layout<T, local>();

  std::size_t written = 0;
  template for (constexpr auto [layout, member] : std::views::zip(wire.members, nsdm(^^T))) {
    using member_type = typename[:type_of(member):];
    if constexpr (wirable_class<member_type>) {
      // nested struct: pass the ambient ONE hop further down, don't reset to the global default
      written += serialize<member_type, merge_context(local, member)>(
          out.subspan(layout.offset.bytes, layout.size), value.[:member:]);
    } else {
      written += serialize_primitive<layout.endianness>(out.subspan(layout.offset.bytes, layout.size), value.[:member:]);
    }
  }
  return written;
}

// ── deserialize (eager): exact mirror image ────────────────────────────────────────
template<wirable_class T, context Ctx = context{}>
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::eager_t) -> T {
  static constexpr auto local = merge_context(Ctx, ^^T);
  static constexpr auto wire  = get_wire_layout<T, local>();

  T value;
  template for (constexpr auto [layout, member] : std::views::zip(wire.members, nsdm(^^T))) {
    using member_type = typename[:type_of(member):];
    if constexpr (wirable_class<member_type>) {
      value.[:member:] = deserialize<member_type, merge_context(local, member)>(
          in.subspan(layout.offset.bytes, layout.size), dsrl::eager);
    } else {
      value.[:member:] = deserialize_primitive<member_type, layout.endianness>(in.subspan(layout.offset.bytes, layout.size));
    }
  }
  return value;
}

// ── deserialize (lazy): the view has to REMEMBER its resolved context, not just pass it through ──
template<wirable T, context Ctx = context{}>
class msg {
  static constexpr auto local = merge_context(Ctx, ^^T); // resolved once, baked into msg<T, Ctx>'s own type
  std::span<std::byte const> data_;

public:
  constexpr explicit msg(std::span<std::byte const> const data) : data_(data) {}

  template<static_string Name>
  constexpr auto field() const {
    static constexpr auto member     = find_member(^^T, Name);
    static constexpr auto layout     = get_wire_layout<T, local>().members[index_of(member)];
    using member_type = typename[:type_of(member):];

    if constexpr (wirable_class<member_type>) {
      // nested struct field: today's code already collapses this to an eager recursive call
      // (deserialize_member -> deserialize<T>(..., eager)) rather than a nested msg<T> -- keep
      // that shape, just resolve ITS context too instead of letting it reset to the default:
      return deserialize<member_type, merge_context(local, member)>(
          data_.subspan(layout.offset.bytes, layout.size), dsrl::eager);
    } else {
      return deserialize_primitive<member_type, layout.endianness>(data_.subspan(layout.offset.bytes, layout.size));
    }
  }
};

template<wirable T, context Ctx = context{}>
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::lazy_t) -> msg<T, Ctx> {
  return msg<T, Ctx>{in}; // Ctx passed through unresolved -- msg<T, Ctx> resolves `local` itself, once
}

// ── in_place: context still decides WHETHER T qualifies, but never touches the actual read ──
// trivially_wirable<T> already requires get_struct_layout<T>() == get_wire_layout<T>() -- computing
// the wire-layout side of that comparison needs the correctly resolved context (an unannotated T
// embedded somewhere that implies non-native endianness must NOT be trivially_wirable, even though
// T's own bytes match its own struct layout in isolation). Once a type has passed that check, the
// bitcast itself carries no annotations and needs no context at all:
template<trivially_wirable T> // the concept check is where context-aware layout resolution already happened
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::in_place_t) -> T const& {
  return *std::start_lifetime_as<T>(in.data()); // pure bitcast
}
```

## Open problem: variable-length fields (`count`)

While designing the above, the need for a `count(field_name)` annotation came up — marking a `contiguous_range` member (e.g. `std::vector<T>`) as sized by the runtime value of another, earlier field, rather than by a fixed template parameter (an earlier idea of a bespoke `rbe::vector<T, N>` wrapper type was dropped once it became clear a plain `std::vector<T>` plus `count` covers the same need, and fixed-size sequences are already better expressed as `std::array<T, N>` per REQ-002 — layout belongs in the type system, not in annotations).

`count` itself fits the annotation-dimension mechanism above trivially: one member-scope-only annotation, one trait specialization. The problem is **not** annotation identity — it's that `count` cannot actually be *acted upon* without a bigger change elsewhere:

- `core/memory_layout.hpp`'s `struct_layout`/`member_layout`/`wire_size_of<T>()` are all `consteval`, computed once per *type* and cached as a static table of `{offset, size, endianness}` triples. This is only valid when every member's size is knowable from its type alone.
- With a `count`-driven member, the wire size of the type as a whole can only be known **at runtime**: for serialization, from the live object's `.size()`; for deserialization, only after the count-source field has actually been read off the buffer (which is why it must appear *before* the variable-length field on the wire — a natural consequence of REQ-051/052, fixed field order).
- This means such types need a second, explicitly runtime code path: `wire_size_of(value)` (taking the actual object, not just `<T>()`), and `srl::serialize`/`dsrl::deserialize` walking members with a *running byte cursor* that accumulates as it goes, instead of looking up a precomputed static offset per member. `dsrl::msg<T>::field<Index>()` (today true random access, since offsets are constant) would also need to become effectively sequential for any field positioned after a variable-length one.

This is a self-contained follow-up design (touching the `wirable`/`trivially_wirable` concept hierarchy, `core/memory_layout.hpp`, `srl/serialize.hpp`, `dsrl/deserialize.hpp`, and `dsrl/msg.hpp`), deliberately **out of scope** for the annotation-system redesign described in this document. `count` should be declared (dimension + `member_only` scope) alongside the other annotations when the system above is implemented, but left without a consumer — the same state `id`/`length` are in today.

## Summary of files touched (when this is implemented)

| File | Change |
|---|---|
| `annotations/detail/dimension.hpp` | **New.** `annotation_traits<T>`, `dimension_kind`, `is_marked_annotation`, `dimension_of`, `kind_of`, `default_value_of`, `value_of`, `resolve_in_scope`, `resolve`. |
| `annotations/detail/base.hpp` | `base_annotation` removed; `is_rbe_annotation` becomes trait-completeness-based instead of `bases_of`-walking. |
| `annotations/detail/view.hpp`, `detail/utils.hpp` | Unchanged in shape; `has_annotation` collapses to one overload. |
| `annotations/detail/correctness.hpp` | Central `alignment`/`endianness`/`global_unique` lists removed; `well_annotated` becomes a generic loop over the dimensions actually found on a type. |
| `annotations/alignment.hpp`, `endianness.hpp`, `metadata.hpp`, `format.hpp` | Each annotation gains a colocated `annotation_traits<T>` specialization; `endianness.hpp` additionally reuses `std::endian` for `little`/`big`. |
| `annotations/derive.hpp` | No change. |
| `core/memory_layout.hpp` | `endiannes_from_annotation`/`has_endianness_annotation`/`get_member_endianness` collapse into one call to the generic `resolve<endian::order>`. |
| `tests/common/common_structs.hpp`, `tests/static/test_annotations.cpp` | Mechanical fallout: `annotation_a`/`annotation_c` gain a trait specialization instead of inheriting `base_annotation`; a couple of assertions that iterated the old central lists need rewriting against the new discovery mechanism. |
| `example/**`, `test_package/**`, `docs/**` | No changes — all existing usage (`rbe::pack`, `rbe::little`, `derive<...>`, `pack_le`/`pack_be`/`debug`) keeps identical names and `[[=...]]` syntax. |
