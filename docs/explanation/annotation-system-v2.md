# Annotation System Redesign

> **Status: implemented.** This is the third shape of the annotation system. v1 identified an annotation
> by inheritance from `base_annotation` and kept dimension membership in a central list; v2 replaced both
> with a specializable `annotation_traits<T>` and reused plain enums (`std::endian`) as annotation values
> directly. v3 — described here — keeps v2's dynamically discovered dimensions and its uniform range API,
> and replaces `annotation_traits` with a single way of *building* an annotation. The sections that remain
> genuinely open (`count`/variable-length fields, annotation scope) are marked as such below.

## Motivation

v2 solved "how do I recognize an annotation" but left "how do I write one" unanswered, and four different
answers had grown in the tree:

| Annotation | v2 shape |
| --- | --- |
| `little` / `big` | a plain `endian::order` enumerator reused as the annotation value |
| `pack` / `align` | a plain `alignment_mode` enumerator |
| `bits(msb, lsb)` | a struct with a payload and a constructor |
| `frame_length`, `fmt` | an anonymous `struct {}` object used as a marker |
| `id` / `id(value)` | a consteval functor object plus a hidden `id_value<T>` template |

Each shape dragged its own `annotation_traits` specialization behind it, spelled
`annotation_traits<std::remove_cvref_t<decltype(rbe::frame_length)>>` for the anonymous ones. Nothing was
wrong with any single one; the problem was that adding a new annotation meant picking a shape with no
criterion, and that the machinery had to keep coping with all of them.

Two more concrete problems came from the same root:

1. **Annotations were handled sometimes as types, sometimes as values.** `annotation_range()` normalized
   everything to *types*, `annotation_values()` preserved *values*, `rbe_annotation_values()` was a third
   variant, and every caller had to know which representation it was holding. Because identity was
   type-based, three annotations that are semantically three values of one thing
   (`frame_length`/`payload_length`/`header_length`) were forced to be three distinct types.
2. **There was no way to ask "which annotation of this kind does this type carry, and what is its
   value?"** — the direction dispatch needs (`framing/dsrl/detail/candidate_list.hpp` was written against
   an API that did not exist). Nothing in the system represented *one whole annotation*.

## Core mechanism: one recipe, `annotation_kind<Tag>`

There is no trait to specialize and no registry. An annotation is built, always, like this:

```cpp
namespace detail {                     // tags and dimensions are not public API: nothing outside
struct endianness_dim {                // RBE ever needs to name them
  static constexpr auto kind          = dimension_kind::exclusive;
  static constexpr auto default_value = endian::order::native;
};

struct order_tag {
  using dimension  = endianness_dim;   // optional: the dimension it belongs to
  using value_type = endian::order;    // optional: absent means "marker only"
};
} // namespace detail

inline constexpr detail::annotation_kind<detail::order_tag> order {}; // [[=rbe::order(endian::order::big)]]

inline constexpr auto big    = order(endian::order::big);             // [[=rbe::big]]
inline constexpr auto little = order(endian::order::little);
```

The public surface of an annotation is therefore exactly three things: the value type it is written
with (`endian::order`, `alignment_mode`, `length_kind`), the factory object, and the named aliases.
The tag and the dimension stay in `rbe::detail`, colocated with the annotation they describe.

`annotation_kind<Tag>` is the object the user spells; calling it produces an `annotation_value<Tag, T>`.
Every named annotation in the library is an alias of such a call. Identity is structural: a type is an RBE
annotation iff it is a specialization of `annotation_kind`, `annotation_value` or `annotations_t` — which
also means `is_complete_type(substitute(...))` and the whole `annotation_traits` apparatus are gone.

A tag may declare four more things, all optional:

| Member | Meaning |
| --- | --- |
| `using dimension = SomeDim;` | the dimension the annotation belongs to; absent ⇒ no correctness ceremony at all (`fmt`) |
| `static constexpr bool marker = true;` | the bare factory object is an annotation too — this is what makes `rbe::id` and `rbe::id(value)` two forms of one tag |
| `static constexpr auto identity = identity_kind::kind;` | how repetitions are counted, see below |
| `static consteval auto check(annotation_info, std::meta::info entity) -> bool;` | the annotation's own correctness rule; it receives the whole annotation, so one `check` covers both forms |

`using value_type = detail::deduced;` means the annotation carries whatever type it is handed, which is
what `rbe::id(42)` and `rbe::id(msg_type::heartbeat)` need.

Dimensions are unchanged from v2 — a tag type with a `kind`, discovered dynamically from the annotations
actually attached to a type, so adding a dimension still requires zero edits to `correctness.hpp`:

```cpp
enum class dimension_kind : std::uint8_t {
  exclusive = 1 << 0, ///< at most one annotation of the dimension within a single annotation range
  unique    = 1 << 1, ///< each annotation of the dimension at most once across the whole (deep) type
};
```

### What "the same annotation" means

Since annotations are compared by value now, `unique` needs to know what counts as a repetition. That is
the one knob `identity_kind` provides:

- `identity_kind::value` (the default) — the annotation *is* its value. `frame_length` and
  `payload_length` are two values of one tag and are independently unique, which is exactly why the three
  length annotations could collapse into a single type.
- `identity_kind::kind` — the value is a payload, not an alternative. `rbe::id(1)` and `rbe::id(2)` are
  one id said twice, so a message carrying both (at any depth) is rejected.

Local duplicate detection is always by value, so `[[=rbe::little, =rbe::little]]` is reported as a
duplicate while `[[=rbe::little, =rbe::big]]` is reported as a dimension conflict — in v2 both came out
as "duplicate", because identity was type-based and the two share a type.

### `derive<...>` is unaffected

`rbe::derive<Args...>` / `pack_le` / `pack_be` / `debug` need no changes: `annotations_t<Args...>` never
depended on how a single annotation is built, and `annotation_value` is a structural type, so it remains
usable as a non-type template parameter.

## One whole annotation: `annotation_info`

`annotation_info` is a strongly typed `std::meta::info` that always wraps an annotation's **value**
(never its type), validated on construction. It is what every range, view and query in the system deals
in, which is what removed the type/value duality:

```cpp
consteval explicit annotation_info(std::meta::info);   // throws unless it is one rbe annotation
reflection() -> std::meta::info     // the value, as written
type()       -> std::meta::info     // annotation_kind<Tag> or annotation_value<Tag, T>
tag()        -> std::meta::info     // what frame_length and payload_length share
dimension()  -> std::meta::info     // null reflection if it belongs to none
has_value()  -> bool
value_type() -> std::meta::info     // the payload's type -- what dispatch needs to recover an id's type
value<T>()   -> std::optional<T>
is(needle)   -> bool                // same annotation as this known one
operator==   -> bool                // same annotation, neither type known statically
identity_equals(other) -> bool      // same annotation for the purposes of `unique`
```

Two mechanisms make the type-erased half work, both reached reflectively:

- `annotation_value<Tag, T>` generates `static consteval payload(info) -> T` and
  `static consteval equals(info, info) -> bool`. `annotation_info` finds them with
  `static_member_function(type, "payload"/"equals")` and calls them through `extract<fn_t>` — the same
  trick `verify_check` already used for an annotation's `check`. No annotation author ever writes them.
- `is(needle)` needs no reflection of the needle at all: the needle's type is known statically at the
  call site, so it is a plain `extract<needle_t>(a) == needle`. This matters because `^^` cannot be
  applied to a non-type template parameter or to a function parameter, which is precisely how needles
  arrive (`contains_annotation<T, Annotation>`, `proxy::field<Annotation>()`).

## One range entry point

`views::annotations` normalizes anything — a scalar annotation, a `derive<...>` list, or the raw output
of `std::meta::annotations_of` — into a flat range of `annotation_info`. On top of it there are exactly
three ranges and two lookups:

```cpp
own_annotations(entity)   -> std::vector<annotation_info>  // written directly on the entity
annotation_range(entity)  -> std::vector<annotation_info>  // REQ-058..061: member's own + its type's own
deep_annotations(entity)  -> std::vector<annotation_info>  // the above, recursively

find_annotation(entity, tag)      -> std::optional<annotation_info>
find_annotation_deep(entity, tag) -> std::optional<annotation_info>
resolve_in_scope<T>(entity)       -> std::optional<T>
```

`own_annotations` is deliberately not called `annotations_of`: ADL on `std::meta::info` would make the
call ambiguous with `std::meta::annotations_of`.

Everything else is an ordinary range algorithm over one of those three: `has_annotations` is an
`any_of`/`all_of`, the exclusivity rule is a `count_if(range, by_dimension(dim)) <= 1`, the uniqueness
rule is a `count_if` with `identity_equals`, duplicate detection is `find` with `operator==`.

`find_annotation` is the direction v2 lacked. Dispatch can now write, generically:

```cpp
auto const id = find_annotation(^^Msg, ^^rbe::detail::id_tag);   // the annotation itself
id->value_type();                                        // the type the id was declared with
id->value<msg_type_t>();                                 // the value it was declared with
```

## `core/detail/context.hpp` is the only value-resolution consumer

`merge_context` remains the single chokepoint where annotation values become behavior:

```cpp
consteval auto merge_context(context const ambient, std::meta::info const entity) -> context {
  context result = ambient;
  if (auto v = resolve_in_scope<endian::order>(entity))   { result.endianness = *v; }
  if (auto v = resolve_in_scope<alignment_mode>(entity))  { result.alignment  = *v; }
  return result;
}
```

`resolve_in_scope<T>` walks `annotation_range(entity)` and returns the first `annotation_info::value<T>()`
that answers — so an annotation of the right dimension but the wrong payload type (`bits` inside the
endianness dimension) is skipped rather than producing a wrong answer, and `context`'s defaults are read
straight off the dimension tags (`endianness_dim::default_value`).

## Empirically verified against GCC 16

Before committing to this design, a set of standalone `.cpp` probes were compiled (`-freflection -fsyntax-only`) against this machine's GCC 16.1.1 to de-risk the reflection API usage, since C++26 reflection is still an experimental, fast-moving feature:

- ✅ `members_of(specialization, ctx)` + `is_type_alias`/`identifier_of` correctly locates a nested `using dimension = ...;`; `static_data_members_of` + `identifier_of` correctly locates `static constexpr kind`/`default_value`. (v2 also verified `is_complete_type(substitute(^^annotation_traits, {T}))`; v3 no longer needs it.)
- ✅ `type_of(annotation_value) == ^^T` (after `remove_cvref`) reliably identifies an annotation's type, including when the annotation is a value of type `T` shared by several distinct annotations (e.g. `little`/`big` both being `endian::order`).
- ✅ `extract<T>(a) == extract<T>(b)` reliably compares two annotation instances **by their real, extracted value**, regardless of how each was spelled at the call site (a named `constexpr` variable vs. a bare enumerator expression).
- ✅ An overloaded `consteval` "factory" function (e.g. two overloads both named `count`, taking different argument types and returning different underlying annotation types) works fine as the operand of `[[=...]]` — useful if a future annotation wants one call-syntax name backed by more than one concrete type.
- ⚠️ **`std::meta::info` equality is *not* reliable for comparing annotation *values* directly.** Two reflections that denote the same constant (e.g. the object `little_v` vs. the bare expression `e_kind::little`) compare **unequal** with plain `==`, even though they print identically in diagnostics. This invalidated the first draft of this design, which planned to normalize annotation instances to a "canonical value" reflection (via `constant_of`) and compare *those* directly. The fix — and the reason the design above never compares value-reflections with `==` — is to always route semantic comparisons through `extract<T>(...) == extract<T>(...)`, comparing the real C++ values, never the reflections. Type-level comparisons (`normalize_type`/`type_of`, used throughout for identity and dimension lookup) remain fully reliable and needed no change.

Re-verified for v3, with a fresh probe (`spike_annotation.cpp`, GCC 16.2.1):

- ✅ `static_member_function(type, "payload"/"equals")` + `extract<fn_t>` finds and calls a static member function of a **class template specialization**, so the payload of an annotation whose concrete type is only known as a reflection can be read back, and two such annotations compared by value. This is what lets `frame_length`/`payload_length`/`header_length` share a single type.
- ✅ `annotation_value<Tag, T>` is a structural type: usable as an NTTP both in `derive<...>` and in `proxy::field<Annotation>()`.
- ✅ `[[=rbe::order(endian::order::big)]]` (the factory called inline in the annotation) and `[[=rbe::big]]` (the named alias) produce annotations that compare equal by value, confirming the `info`-equality hazard above is fully contained.
- ⚠️ **`^^` cannot be applied to a non-type template parameter, to a function parameter, or to an arbitrary expression.** Needles arrive as exactly those (`contains_annotation<T, Annotation>`, `has_annotations(entity, needle)`), so a needle is never reflected: its type is known statically at the call site, which is all `extract<needle_t>(a) == needle` needs.
- ⚠️ **`^^SomeAliasName` compares unequal to a reflection of the type it names.** `^^std::remove_cvref_t<decltype(x)>` reflects the *alias*, so every type-identity check would silently fail for a type spelled through one. `normalize_type` therefore applies `dealias`, and so does every member-alias read (`value_type`, `tag`, `dimension`). This is the type-level sibling of the `annotations_of` alias trap recorded below, and it cost a debugging round before it was found.

This last finding is also why duplicate-detection (`has_duplicates`, used by `verify_no_local_duplications`) could stay type-based for as long as it did: type-level comparison was never the broken part. In v3 it compares whole annotations (`annotation_info::operator==`, routed through `extract`), which is what makes `[[=rbe::little, =rbe::big]]` come out as a dimension conflict rather than as a duplicate.

## Open: annotation scope (struct-level vs. member-level only)

Not yet fully designed. The identity/dimension mechanism above needs one more optional trait member, tentatively:

```cpp
enum class annotation_scope : std::uint8_t { any, member_only, type_only };
// on annotation_traits<T>: static constexpr auto scope = annotation_scope::member_only;
```

defaulting to `any` so no existing annotation needs to change. Verified feasible: it's possible to detect, generically, whether an annotation of a given type showed up directly in `std::meta::annotations_of(a_type)` (struct-level) as opposed to `std::meta::annotations_of(a_member)` (member-level) — the two are already gathered separately at the call sites that matter, they're just unioned together today for dimension-membership purposes. `well_annotated` would gain a third generic check, symmetric to the `kind` check, that rejects an annotation found at a scope it doesn't declare support for.

## N-level annotation propagation — ✅ Implemented

> Originally written up here as an open problem; the section below is kept as-is for the reasoning and the `parent_of` dead-end, since both remain the reason the codebase looks the way it does. The `context`/`merge_context` mechanism described here is now real, shipped code — see `core/detail/context.hpp`, the four-overload split in `srl/serialize.hpp` and `dsrl/deserialize.hpp`, and `dsrl/proxy.hpp`. Regression coverage: `tests/runtime/test_serde.cpp`'s `"N-level propagation: unannotated nested structs inherit an ancestor's endianness"` test case, using the `NestedParent`/`NestedMiddle`/`NestedLeaf` structs in `tests/common/common_structs.hpp`.
>
> Two real bugs turned up only once this was wired into actual serialize/deserialize code, both fixed:
>
> 1. `detail::normalize_endianness<Ctx.endianness>(value)` (one explicit template argument) silently binds to `normalize_endianness`'s *identity-forwarder* overload (`template<endian::order Order> auto normalize_endianness(auto const&)`) instead of the byte-swapping one (`template<T, Order> auto normalize_endianness(T const)`) — with one explicit argument, it binds to the first template parameter of whichever overload's parameter list makes that argument's *position* valid, and the forwarder's abbreviated `auto` parameter happily accepts it. Both `T` and `Order` must be given explicitly at the call site.
> 2. `dsrl::proxy<T, Ctx>` computed its own `context` from `^^value_type` (a member type alias, `using value_type = T;`) instead of `^^T` directly — `std::meta::annotations_of` does not see through a type alias to the annotations on the type it names, so the alias-based lookup silently found nothing. Reflect the template parameter directly, never a same-named alias, when the reflection feeds into annotation lookup.
>
> `resolve<T>(parent, member, dim)` (below) only looked one level up: the member's own scope, then its *immediate* containing struct, then the dimension's default. [REQ-066](requirements.md#nested-structure-handling)–[070](requirements.md#nested-structure-handling) require full transitive propagation through arbitrarily deep nesting — `example/annotations.cpp`'s own worked example spells out the intent explicitly:

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
# Leaf and aggregate are two SEPARATE function definitions, each guarded by what kind of T
# it accepts -- never one function branching internally on "is this a struct". The caller
# below never asks that question either; it just calls serialize() and the language picks
# whichever of the two definitions accepts this particular member's type. This mirrors how
# the real codebase already splits trivially_wirable_primitive / custom_wirable / wirable_class
# into separate overloads today (srl/serialize.hpp) rather than branching inside one function.

function serialize(value: T, out_buffer, context)  -- for T a primitive/leaf:
    write_primitive(value, out_buffer, context.endianness)

function serialize(value: T, out_buffer, context = <dimension defaults>)  -- for T an aggregate:
    local  = merge(context, T)                     # T's own annotations override the inherited ambient
    layout = wire_layout(T, local)
    for (member, member_layout) in layout:
        member_context = merge(local, member)      # one hop further down, never reset
        serialize(value.member, out_buffer at member_layout.offset, member_context)
        # ^ whichever of the two serialize() definitions accepts member's type runs -- no branch here

# ── deserialize: eager (mirror image of serialize, same two-definition split) ────────
function deserialize_eager(in_buffer, T, context)  -> T  -- for T a primitive/leaf:
    return read_primitive(in_buffer, context.endianness)

function deserialize_eager(in_buffer, T, context = <dimension defaults>) -> T  -- for T an aggregate:
    local  = merge(context, T)
    layout = wire_layout(T, local)
    result = new T
    for (member, member_layout) in layout:
        member_context = merge(local, member)
        result.member = deserialize_eager(in_buffer at member_layout.offset, member.type, member_context)
    return result

# ── deserialize: lazy ─────────────────────────────────────────────────
# constructed once, queried many times via field() -- so the context must be REMEMBERED,
# not recomputed from nothing on every access
class lazy_view(in_buffer, T, context = <dimension defaults>):
    local = merge(context, T)                      # resolved ONCE, at construction, not per field()

    function field(name):
        member, member_layout = lookup(wire_layout(T, local), name)
        member_context = merge(local, member)
        # nested-struct fields already collapse to an eager read even under a lazy top-level call
        # in the current code -- keep that shape. field() calls the SAME overloaded
        # deserialize_eager used above and never itself asks "is this member a struct":
        return deserialize_eager(in_buffer at member_layout.offset, member.type, member_context)

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

This pseudocode's shape is now real, shipped code (`core/detail/context.hpp`, `srl/serialize.hpp`, `dsrl/deserialize.hpp`, `dsrl/proxy.hpp`) — see the "✅ Implemented" note at the top of this section for exactly where, plus the two real bugs that only surfaced once it was actually wired in. `context` now threads **two** dimensions the same way: `endianness` and `alignment` (`context::alignment`, an `alignment_mode`) — an unannotated nested aggregate inherits its ambient packing exactly like it inherits ambient endianness, resolved via `resolve_in_scope<alignment_mode>` exactly like endianness (not a bespoke presence check), now that `pack`/`align` are `alignment_mode` values rather than distinct anonymous tag types. Adding `pack` surfaced a third bug, distinct from the two documented above: `wire_size_of`'s packed-recursion assumed every nested type was a genuine aggregate reachable via `nsdm`, which broke the moment packing (previously only ever a per-type presence check, never inherited) started reaching `std::array` members — recursing one hop further into `std::array`'s own internal raw-C-array member and calling `nsdm` on a non-class array type threw. Fixed the same way `is_wirable_class_type` already handled this: check `is_trivially_wirable_primitive(remove_all_extents(info))` as a potential leaf *before* assuming a type has reflectable members to recurse into, since an array of primitives has no inter-element padding to strip regardless of packing and no `nsdm` to walk in the first place.

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

// ── serialize: leaf and aggregate are two SEPARATE overloads, picked by concept -- no
//    `if constexpr` anywhere. This is the same split the real codebase already uses for
//    trivially_wirable_primitive/custom_wirable/wirable_class in srl/serialize.hpp; a full
//    implementation would give custom_wirable its own third overload the same way, calling
//    custom<T>::serialize -- elided here since it's orthogonal to context threading.
template<wirable_primitive T, context Ctx>
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  return serialize_primitive<Ctx.endianness>(out, value); // leaf: context ends here, writes bytes
}

template<wirable_class T, context Ctx = context{}> // top-level calls seed the true dimension defaults
constexpr auto serialize(std::span<std::byte> const out, T const& value) -> std::size_t {
  static constexpr auto local = merge_context(Ctx, ^^T);   // T's own annotations override the inherited ambient
  static constexpr auto wire  = get_wire_layout<T, local>();

  std::size_t written = 0;
  template for (constexpr auto [layout, member] : std::views::zip(wire.members, nsdm(^^T))) {
    using member_type = typename[:type_of(member):];
    // overload resolution alone picks the leaf or aggregate serialize<member_type, ...> above --
    // this call site never asks "is member_type itself a struct"
    written += serialize<member_type, merge_context(local, member)>(
        out.subspan(layout.offset.bytes, layout.size), value.[:member:]);
  }
  return written;
}

// ── deserialize (eager): same two-overload split, exact mirror image ───────────────
template<wirable_primitive T, context Ctx>
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::eager_t) -> T {
  return deserialize_primitive<T, Ctx.endianness>(in);
}

template<wirable_class T, context Ctx = context{}>
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::eager_t) -> T {
  static constexpr auto local = merge_context(Ctx, ^^T);
  static constexpr auto wire  = get_wire_layout<T, local>();

  T value;
  template for (constexpr auto [layout, member] : std::views::zip(wire.members, nsdm(^^T))) {
    using member_type = typename[:type_of(member):];
    value.[:member:] = deserialize<member_type, merge_context(local, member)>(
        in.subspan(layout.offset.bytes, layout.size), dsrl::eager);
  }
  return value;
}

// ── deserialize (lazy): the view has to REMEMBER its resolved context, not just pass it through ──
template<wirable T, context Ctx = context{}>
class proxy {
  static constexpr auto local = merge_context(Ctx, ^^T); // resolved once, baked into proxy<T, Ctx>'s own type
  std::span<std::byte const> data_;

public:
  constexpr explicit proxy(std::span<std::byte const> const data) : data_(data) {}

  template<static_string Name>
  constexpr auto field() const {
    static constexpr auto member     = find_member(^^T, Name);
    static constexpr auto layout     = get_wire_layout<T, local>().members[index_of(member)];
    using member_type = typename[:type_of(member):];

    // today's code already collapses nested-struct fields to an eager recursive call
    // (deserialize_member -> deserialize<T>(..., eager)) rather than a nested proxy<T> -- keep that
    // shape. This calls the SAME overloaded deserialize(..., eager) as above; field() never
    // branches on member_type either, leaf or aggregate is resolved by overload resolution alone.
    return deserialize<member_type, merge_context(local, member)>(
        data_.subspan(layout.offset.bytes, layout.size), dsrl::eager);
  }
};

template<wirable T, context Ctx = context{}>
constexpr auto deserialize(std::span<std::byte const> const in, dsrl::lazy_t) -> proxy<T, Ctx> {
  return proxy<T, Ctx>{in}; // Ctx passed through unresolved -- proxy<T, Ctx> resolves `local` itself, once
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
- This means such types need a second, explicitly runtime code path: `wire_size_of(value)` (taking the actual object, not just `<T>()`), and `srl::serialize`/`dsrl::deserialize` walking members with a *running byte cursor* that accumulates as it goes, instead of looking up a precomputed static offset per member. `dsrl::proxy<T>::field<Index>()` (today true random access, since offsets are constant) would also need to become effectively sequential for any field positioned after a variable-length one.

This is a self-contained follow-up design (touching the `wirable`/`trivially_wirable` concept hierarchy, `core/memory_layout.hpp`, `srl/serialize.hpp`, `dsrl/deserialize.hpp`, and `dsrl/proxy.hpp`), deliberately **out of scope** for the annotation-system redesign described in this document. `count` should be declared (dimension + `member_only` scope) alongside the other annotations when the system above is implemented, but left without a consumer — the same state `id` and the `*_length` annotations are in today.

## Summary of files touched (when this is implemented)

| File | Change |
| --- | --- |
| `annotations/detail/dimension.hpp` | **New.** `annotation_traits<T>`, `dimension_kind`, `is_marked_annotation`, `dimension_of`, `kind_of`, `default_value_of`, `value_of`, `resolve_in_scope`, `resolve`. |
| `annotations/detail/base.hpp` | `base_annotation` removed; `is_rbe_annotation` becomes trait-completeness-based instead of `bases_of`-walking. |
| `annotations/detail/view.hpp`, `detail/utils.hpp` | Unchanged in shape; `has_annotation` collapses to one overload. |
| `annotations/detail/correctness.hpp` | Central `alignment`/`endianness`/`global_unique` lists removed; `well_annotated` becomes a generic loop over the dimensions actually found on a type. |
| `annotations/alignment.hpp`, `endianness.hpp`, `id.hpp`, `length.hpp`, `format.hpp` | Each annotation gains a colocated `annotation_traits<T>` specialization; `endianness.hpp` additionally reuses `std::endian` for `little`/`big`. |
| `annotations/derive.hpp` | No change. |
| `core/memory_layout.hpp` | `endiannes_from_annotation`/`has_endianness_annotation`/`get_member_endianness` collapse into one call to the generic `resolve<endian::order>`. |
| `tests/common/common_structs.hpp`, `tests/static/test_annotations.cpp` | Mechanical fallout: `annotation_a`/`annotation_c` gain a trait specialization instead of inheriting `base_annotation`; a couple of assertions that iterated the old central lists need rewriting against the new discovery mechanism. |
| `example/**`, `test_package/**`, `docs/**` | No changes — all existing usage (`rbe::pack`, `rbe::little`, `derive<...>`, `pack_le`/`pack_be`/`debug`) keeps identical names and `[[=...]]` syntax. |
