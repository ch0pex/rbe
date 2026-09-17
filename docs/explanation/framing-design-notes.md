# Framing Design Notes

> **Status: working notes.** A running record of the framing design decisions taken while implementing
> `rbe::frame`, `rbe::any` and `rbe::many` on the `feat/framing-v2` branch, meant as the source material
> for the future user-facing framing documentation. Not linked from site navigation — same convention as
> [Message Framing Redesign](framing.md) and [Annotation System Redesign](annotation-system-v2.md).
>
> Where these notes and [framing.md](framing.md) disagree, these notes are newer; the known divergences
> are listed in [Divergences from framing.md](#divergences-from-framingmd).

Each decision is tagged:

- **Implemented** — in the code and covered by tests.
- **Decided** — agreed, not implemented yet.
- **Proposed** — recommended, still pending confirmation.
- **Open** — no answer yet.

---

## 1. Levels: vocabulary types and their lowerings

**Implemented.** The user describes a protocol with *vocabulary types* in `rbe::` — `rbe::frame<H, P>`,
`rbe::any<Ts...>`, `rbe::many<T>`, `rbe::blob`. They carry no bytes; they describe the wire format and
lower to the types that actually view or write bytes:

| Vocabulary (`rbe::`) | `dsrl_type` | `srl_type` |
| --- | --- | --- |
| `frame<H, P>` | `dsrl::frame<H, to_dsrl_t<P>>` | `srl::frame<...>` (TODO) |
| `any<Ts...>` | `dsrl::any<Ts...>` | `srl::any<...>` (TODO) |
| `many<T>` | `dsrl::many<T::dsrl_type>` | `srl::many<...>` (TODO) |
| `blob` | `std::span<std::byte const>` | `std::span<std::byte>` |
| wirable `T` | `T` (the strategy decides the return type) | `T` |

- The header is never lowered, only the payload (`rbe/framing/detail/to_dsrl_type.hpp`).
- Lowering recurses through nesting and is idempotent.

**Implemented. Wire-format properties are level-agnostic.** Classifying how a frame is delimited describes
the format, and `srl` needs it as much as `dsrl` (which length field to back-patch, whether a run of frames
can be split by a reader), so the classification is written once against the *shape* of a frame and holds at
every level: `self_delimiting_frame<msg_frame>` and `self_delimiting_frame<msg_frame::dsrl_type>` are both
valid and give the same answer, because lowering never changes how a frame is delimited. Errors still show up
on the type the user wrote, since `rbe::frame` constrains its own parameters.

### Concept layering

**Implemented.** The concepts used to be written three times, once per level. They are now split along what
actually differs between levels:

| Concept | Where | Holds for |
| --- | --- | --- |
| `frame_header` | `frame_concepts.hpp` | every level — a header is never lowered |
| `is_frame` | `frame_concepts.hpp` | every level — `header_type` + `payload_type`, nothing else |
| `frame_payload` | `frame_concepts.hpp` | `dsrl` and `srl` — the payload categories over the bytes |
| `self_delimiting_frame`, `buffer_delimited_frame`, `dispatch_delimited_frame`, … | `frame_delimiting_concepts.hpp` | every level, via `is_frame` |
| `frame_serder_payload`, `serder_traits`, `frame_serder` | `frame_serder_concepts.hpp` | the vocabulary level |
| `dsrl::is_frame` | `dsrl/frame_concepts.hpp` | the deserializer API |
| `srl::is_frame` | `srl/frame_concepts.hpp` | the serializer API (TODO) |

- `is_frame` is the weakest thing worth calling a frame, and the only thing the delimiting classification
  needs. Everything a level adds on top (traits to lower itself, an API to offer) stays with that level.
- `frame_payload` is spelled over `std::span<std::byte>` only: a payload type constructible from
  `std::span<std::byte const>` is constructible from `std::span<std::byte>` too, which converts to it, so the
  writable span covers both lowerings and the concept does not need to be parameterized by the level.
- The vocabulary payload (`frame_serder_payload`) is *not* that list of categories: at that level a payload
  is either a wirable message or a type that lowers itself (`serder_traits`), which is what `blob`, `many`,
  `any` and nested frames have in common. The short names belong to the shared core; the vocabulary level,
  which is the one that owns the serder traits, keeps the `_serder` prefix.
- `is_any` / `is_many` are detected by a tag base (`rbe::detail::any_tag`, `many_tag`), which both the
  vocabulary type and its lowering inherit **publicly** — `std::derived_from` needs the base to be reachable,
  so a `class` vocabulary type must say `public`. Now that they are real, `serder_traits` could fold into the
  same disjunction and `frame_payload` / `frame_serder_payload` become one concept.

---

## 2. A frame is a view; the buffer may be larger than the frame

**Implemented** in `rbe/framing/dsrl/frame.hpp`.

`dsrl::frame` and `dsrl::proxy` are non-owning views over a `std::span<std::byte const>`. **Both narrow their
span at construction**, so they always view exactly their object (`data_.size() == length()`). Construction
resolves the *layout* (length fields, static sizes, nested frame lengths); header and payload *values* stay
lazy.

**The buffer may be larger than the frame, and this is the normal case**, not a tolerated one:

1. The library itself builds frames over larger buffers: `payload_length()` of a nested frame builds the inner
   frame over *the remainder of the buffer*, because it does not know the inner length yet — that is what it
   is computing.
2. Iterating `many` builds each element over the rest of the buffer, reads its length and advances. The exact
   size is only known *after* building the element.
3. Users rarely know the exact size: they get a datagram, a `recv` buffer, a ring buffer. Demanding the exact
   size would force them to parse the header themselves.

What needs controlling is a buffer that is *too small*, and ambiguity about what the buffer size means (see
[§4](#4-self-delimiting-and-buffer-delimited-frames)).

Accessor contract:

| Accessor | Returns |
| --- | --- |
| `length()` | `data_.size()` — resolved once at construction by `length_of()` |
| `header_length()` | the `header_length` field, otherwise `wire_size_of<header_type>()` |
| `payload_length()` | `length() - header_length()` |
| `header_span()` | `data_.first(header_length())` |
| `payload_span()` | `data_.subspan(header_length())` |
| `as_span()` | `data_` — exactly the frame |
| `data()` | `std::byte const*` to the start of the frame |

Constructor preconditions (documented on `dsrl::frame::frame`):

- `data.size() >= length_of(data)`, checked by `std::span::first()` in hardened builds
- buffer-delimited frames: the span covers exactly the frame
- `wire_size_of<header_type>() <= header_length <= frame_length`

**Implemented — `length_of(buffer)`.** A static `frame::length_of(data)` resolves the length without constructing
the frame, so a partially received buffer can still tell how many bytes it needs (stream reassembly). It only
reads what the length depends on: the fixed-size header prefix for length fields and static sizes, plus the
nested frames' headers for a nested frame payload. For a buffer-delimited frame it returns `data.size()`. The
constructor narrows with it (`data.first(length_of(data))`); members cannot be used there, since they read
`data_` before it is initialized. `dsrl::is_frame` requires it, because a nested frame payload is resolved
through `payload_type::length_of()`.

**Proposed — checked construction.** Keep the constructor unchecked (a contract, checked in hardened builds)
and add a validating factory, used internally by `many`:

```cpp
auto [f, rest] = parse<my_frame>(buf); // from_chars style: the frame plus the bytes after it
```

It would check the fixed header prefix, then `length_of()` against the buffer size (reporting "incomplete"
instead of constructing), then length consistency. The return type
(`optional`, `expected`, a result struct) is **Open**.

**Decided — `proxy` narrows its span at construction.** Unlike `frame`, a `proxy` always views exactly its
object: `data_.size() == length()`.

- Fixed-size `T`: `data.first(wire_size_of<T>())`, which is free.
- Variable-size `T`: the constructor builds the offset table, which yields the length
  ([§8](#offset-table-built-at-construction)).

The span passed in may still be larger than the object; the extra bytes are simply not kept. This makes
`as_span()` just `data_`; `size()`/`size_bytes()` are removed, since `length()` is the object's size. It supersedes the
earlier proposal of storing the whole span and narrowing in the accessors, which only made sense for a lazily
computed layout. Remaining inconsistency:

- `data()` returns a `span` in `proxy` but a `std::byte const*` in `frame`.

---

## 3. Length resolution

**Implemented.** Three length annotations, one per quantity (`rbe/annotations/length.hpp`):
`header_length`, `payload_length` and `frame_length`, related by
`frame_length = header_length + payload_length`.

- **`header_length()`** — the annotated field, otherwise `wire_size_of<header_type>()`. The field is read over
  the *fixed-size header prefix* (`data_.first(wire_size_of<header_type>())`), never through `header_span()`,
  which depends on `header_length()` itself.
- **`length()`** — resolved once at construction by the static `length_of()`, from the payload extent in
  priority order (wire values before static sizes):

  | `payload_extent` | Payload length |
  | --- | --- |
  | `payload_length_field` | the `payload_length` header field |
  | `frame_length_field` | `frame_length` field − `header_length()` |
  | `nested_frame` | the nested frame's own `length_of()` over the remainder of the buffer |
  | `static_size` | `wire_size_of<payload_type>()` — fixed-size wirable payloads only, see [§8](#8-variable-size-wirable-types) |
  | `any_id` *(Decided, not implemented)* | the size of the alternative selected by the header id |
  | `buffer_end` | to the end of the buffer (`blob`, span-constructible, `many`) |

- **`payload_length()`** — `length() - header_length()`, by the identity above.

The classification is a single compile-time function, `rbe::detail::payload_extent_of<H, P>()` in
`rbe/framing/detail/payload_extent.hpp`. `dsrl::frame::length_of()` branches on it with `if constexpr`,
and the concepts in [§4](#4-self-delimiting-and-buffer-delimited-frames) are built on it, so the runtime
resolution and the compile-time classification cannot drift apart.

The classification is *structural*: it only needs `header_type` and `payload_type`
(`rbe::is_frame`), so an `rbe::frame` and its `dsrl::frame` lowering classify identically. That
invariant is static-asserted (`same_extent_when_lowered` in `tests/static/test_framing.cpp`).

**Open — units.** All three annotations count bytes. IPv4's IHL counts 32-bit words, and some protocols count
elements rather than bytes (see [framing.md](framing.md#length-semantics)).

---

## 4. Self-delimiting and buffer-delimited frames

**Implemented** in `rbe/framing/frame_concepts.hpp`:

```cpp
template<typename T>
concept self_delimiting_frame  = is_frame<T> and detail::is_self_delimiting<T>();

template<typename T>
concept buffer_delimited_frame = is_frame<T> and not detail::is_self_delimiting<T>();
```

- **Self-delimiting**: the frame's length is known without looking at the buffer size — from a length field,
  static sizes, a nested self-delimiting frame, or (once implemented) the header id. Such a frame may be read
  from a larger buffer; trailing bytes are padding.
- **Buffer-delimited**: the payload runs to the end of the buffer, so *the buffer size is the frame length*.
  Trailing bytes are taken as payload, never as padding.

Rules, all covered by static asserts:

- **A `payload_length` or `frame_length` field makes the frame self-delimiting, whatever the payload is.**
  Guaranteed by the priority order of `payload_extent_of`: a length field is classified before the payload is
  even looked at. `frame<WithFrameLength, blob_frame>` is self-delimiting even though `blob_frame` is not.
- A nested frame makes the outer frame self-delimiting only if the nested frame is.
- `header_length` alone does not delimit anything: it bounds the header, and the payload still runs to the
  end (`frame<WithHeaderLength, blob>` is buffer-delimited).
- Both concepts only accept `rbe::frame_serder` types, not their lowerings.

Concepts cannot be recursive, so the walk through nested frames lives in the `consteval`
`rbe::detail::is_self_delimiting<F>()`.

**Proposed — explicit `is_self_delimiting`.** Today it returns `extent != buffer_end` for everything except
`nested_frame`, which reads as "depends on the payload" and would silently classify a future enumerator as
self-delimiting. Enumerate each case instead (length fields → `true`, `static_size` → `true`,
`nested_frame` → recurse, `buffer_end` → `false`), so that adding `any_id` forces a deliberate branch.

### Buffer-delimited frames are legitimate

Supporting them is deliberate. The length always exists, **but not always inside the frame itself** — it is
often provided by the enclosing layer:

- **TCP**: the header has a data offset (`header_length`) but no payload length; the segment size comes from
  IP's total length. Exactly `frame<TcpHeader, blob>`.
- **RTP, CoAP, DNS over UDP**, a lot of proprietary telemetry: no length, the payload is the rest of the UDP
  datagram.
- **Ethernet II**: no length field (EtherType); the size comes from the driver.
- Application protocols that send one message per datagram and never needed a length because UDP carries it.

The restriction is **where** they may appear, not whether they exist:

1. **As the outermost frame** — the buffer is exactly what was received.
2. **As the payload of a frame carrying a length** — the outer frame narrows, and the inner one receives
   exactly its `payload_span()`: `frame<IpHeader /* [[=frame_length]] */, frame<TcpHeader, blob>>`.
3. **Never as elements of `many`** — nothing tells where one ends and the next begins.

**Proposed — documentation wording** for `buffer_delimited_frame`:

> Valid only where something else delimits them: as the outermost frame (the buffer is exactly what was
> received), or as the payload of a frame that carries a length. They can never be elements of `rbe::many`.

**The typical failure** is a fixed-size receive buffer (e.g. 1500 bytes) not trimmed to the size `recv`
returned. It is harmless for self-delimiting frames; for buffer-delimited ones the garbage becomes payload.
Types cannot prevent it; the constructor documentation warns about it.

**Out of scope for `frame`**: byte-stuffed protocols (SLIP, COBS, HDLC). Their length comes from a deframer
scanning for a delimiter, not from a header. They still fit as "outermost frame over what the deframer
returns", but deframing is not `frame`'s job.

### `many` requires self-delimiting elements

**Decided, not implemented.** `rbe::many<T>` (and its lowerings) must require `self_delimiting_frame<T>`.

It is blocked on `any_id`: `many<frame<CommonHeader, any<...>>>` (`packet` in `tests/static/test_framing.cpp`)
currently classifies as `buffer_end`, because `any` is still a stub and `CommonHeader` has no length field.
Constrain `many` once `any_id` exists.

---

## 5. `any` with id-implied lengths (`any_id`)

**Implemented (classification).** `rbe::any<Ts...>` counts as self-delimiting when every alternative is
(fixed-size, or itself delimited). `payload_extent_of()` returns `any_id` for an `any` payload and
`rbe::dispatch_delimited_frame` picks those frames out, so callers can tell apart the frames whose `length()`
costs an id dispatch; resolving the id at runtime is still pending. **Open:** the classification does not yet
check that every alternative *does* imply its length, so an `any` holding a variable-size alternative would be
classified `any_id` anyway. Users need `many<frame<Header, any<Msgs...>>>`, and many protocols imply each message's length
from its id rather than a header field. When the header carries *both* a length field and an id, the length
field wins: it is cheaper and tolerates unknown ids (see [§6](#6-unknown-ids)).

### Cost and resolution

Self-delimitation is a compile-time property; the *cost* of computing the length is a separate, runtime axis,
and it is not encoded in the concepts (reading a `payload_length` field has a runtime cost too). The
`payload_extent` enum still lets code pick a strategy at compile time
(`if constexpr (extent == payload_extent::any_id)`).

**Decided — id→index resolution.** C++26 reflection cannot generate a `switch` (no code injection). Use a
`template for` chain of `if` comparisons and let the optimizer turn it into a jump table, bit test or decision
tree; GCC and Clang do so at `-O2` when the pattern is clean. Even a linear scan over ≤30–50 integer
comparisons costs a few nanoseconds, and real message distributions are skewed, so the branch predictor learns
the hot types.

Keep the chain switch-convertible:

```cpp
static constexpr auto ids   = std::array {id_of<Ts>()...};           // from [[=rbe::id(v)]]
static constexpr auto sizes = std::array {rbe::wire_size_of<Ts>()...};

auto const id = /* read the [[=rbe::id]] header field once */;
template for (constexpr auto i : indices) {
  if (id == ids[i]) {
    return i; // only return the constant: no work inside the branch
  }
}
return npos; // unknown id
```

- Read the id once into a local before the chain. If every branch re-reads the buffer, the optimizer no longer
  sees the pattern.
- Branches only return the index. The length comes afterwards from `sizes[i]`, and dispatch is a separate
  step.

Fallbacks, only if profiling calls for them (both generated in `consteval`):

- Dense, small ids (e.g. `uint8_t`): an id→index `static constexpr std::array` with a sentinel. O(1)
  guaranteed.
- Dispatch through a function-pointer table, as `std::visit` implementations do. It is an indirect call,
  though, which blocks inlining the visitor, so the `if` chain likely still wins.

Verify once implemented by checking the generated assembly for the jump table (`objdump -d`, Compiler
Explorer).

### Resolve once, carry the index

`dsrl::frame` is stateless and recomputes on every accessor (`as_span()`, `payload_span()`, `match()` all end
in `payload_length()`). With a length field those are cheap reads; with `any_id` each is a lookup. So the
index is resolved once and carried, **without adding state to `frame`**:

- `dsrl::any` is built from `(index, span)`: `length()` becomes `sizes[index]`, and `match()` an indexed jump.
- `frame<H, any>::payload()` resolves once and builds the resolved `any`.
- The `many` iterator resolves while advancing (it needs the length to step) and keeps the index, so
  dereferencing yields a frame with an already resolved `any`.

Caching the index inside `frame` is rejected: it stops being a trivial, copyable view and needs an "unresolved"
state.

---

## 6. Unknown ids

What sets `any_id` apart is not the cost but that **resolution can fail**:

- **With a length field**, an unknown id is skippable: the stream stays parseable.
- **With `any_id`**, an unknown id has no length: the rest of the buffer is unparseable.

**Proposed**:

- **No exceptions.** The library must build without exceptions (REQ-040/041 in
  [Requirements](requirements.md)). An unknown id is not exceptional either: a newer protocol version adding
  messages is routine. A user who wants to throw can do so from their default handler.
- **No `optional` from `payload_length()`.** It keeps returning `size_type`. For `any_id` it has the precondition
  "the header id is known", checked in hardened builds (REQ-042..044) and assumed in non-hardened ones
  (REQ-046). This is the same kind of contract as "the buffer covers the frame".
- **The failure channel lives in resolution**, as a sentinel index (`npos`, like `std::variant::npos` or a
  `find` returning `end`), not an `optional` wrapper. The two safe paths — `match()` and the `many` iterator —
  have to resolve anyway, so checking the sentinel costs nothing. Only direct misuse (calling `length()` on a
  frame whose id is unknown) can hit the precondition.
- **`match()` sends an unknown id to the default case.** The default case is how the user learns about it, and
  the documentation must make users aware of what happens next.
- **Iteration over `many`**:
  - explicit lengths: the unknown element goes to the default case, and iteration **continues**;
  - id-implied lengths: the unknown element is still yielded (its header is readable), goes to the default
    case, and iteration then **ends**, because the iterator cannot advance past it.

  Same handler, different continuation depending on the payload extent — this is the key point for the user
  documentation.
- **The default handler must receive something safe** for an unknown id: the id and the header proxy, and
  possibly the remaining bytes — never an API whose precondition is already broken (`as_span()`,
  `payload_span()`).

**Open**:

- How a caller tells "the buffer was fully consumed" from "iteration stopped at an unknown id" after the loop.
  Candidates: rely on the default handler having run, or have `many` expose `remainder()`/`consumed()`.
- The contract mechanism for hardened checks (C++26 `pre`/`contract_assert`, or a project macro), and whether
  it is shared with the other framing preconditions.

---

## 7. Iteration model

**Decided.** `many` ships a **pull** model (external iterator, ranges-compatible) first; a **push** model
(`for_each`/`dispatch` with an overload set) is a future opt-in, and no API may block adding it.

- Pull supports ranges, and resuming over partially received buffers.
- With id-implied lengths, pull costs one id resolution to advance plus an indexed jump on match, where push
  would need a single resolution. With explicit lengths pull is cheaper: no resolution to advance, and a lazy
  one on match.

See [§5](#resolve-once-carry-the-index) for how pull pays the id resolution only once.

---

## 8. Variable-size wirable types

> The detailed design — offset table, anchors, nested structs, resolution and a worked example — lives in
> [Variable-Size Layout Notes](variable-size-layout.md). This section keeps the summary and the impact on framing.

**Decided (direction), not designed.** `proxy`, and the rest of `dsrl`/`srl`, will have to support structs whose
wire size is only known at runtime — e.g. a `std::vector<Element>` whose element count comes from another,
earlier member. The annotation side is sketched as `count(field_name)` in
[Annotation System Redesign](annotation-system-v2.md#open-problem-variable-length-fields-count). The count
source must precede the vector on the wire (fixed field order, REQ-051/052).

This does not change the framing model; it pushes the same question one level down, from frames to structs:
*where does the length come from, and can it be known from the object's own bytes?*

Consequences to keep in mind now:

- **`wire_size_of<T>()` stops being total.** Variable-size types need a static minimum (the fixed prefix up to
  the first variable member) plus a runtime length computed over the bytes. Headers keep requiring a fixed
  size: `header_length()` reads its field over the fixed prefix.
- **`proxy::length()` becomes a runtime quantity** for variable-size `T`, but it is known from construction on
  ([below](#offset-table-built-at-construction)), so it stays a plain `const` accessor.
- **Field offsets become runtime past the first variable member.** Today `field<Index>()` slices with static
  offsets (`data_.subspan<offset, size>()`). A member after a vector sits at "static offset + sum of the
  preceding variable parts":
  - with fixed-size elements, that is arithmetic on counts (`count × element size`);
  - with variable-size elements, it is a forward walk.
- **Accessing the vector lazily** yields a range over the element bytes: random access when the element size is
  static, forward-only otherwise — the same categories as `many`. `in_place` stays unavailable, since such a
  type is not trivially wirable.
- **`payload_extent` has to split `static_size`.** `payload_extent_of` classifies every wirable payload as
  `static_size` today. A variable-size wirable payload needs its own case — "the payload computes its own
  length from its bytes", the struct-level counterpart of `nested_frame` — which is self-delimiting when its
  counts live inside it.

### Offset table built at construction

**Decided.** A variable-size `proxy` computes its offset table **in the constructor**, and every member stays
`const`. `field<"vector">()` and every other accessor are used exactly as with a fixed-size `T`, with no extra
step for the user.

- **One class, specialized storage.** A single `proxy` keeps one field-lookup API (`field<"name">`,
  `field<Annotation>`, `field<"a", "b">`, `field<Index>`), which goes through an `offset_of(index)` helper
  instead of reading `member_layout.offset` directly:
  - members before the first variable-size member keep their compile-time offset;
  - the table holds one entry per variable-size member (its end offset), so its size is known at compile time;
    a fixed member between two variable ones sits at that end plus a static delta;
  - for fixed-size `T` the table is an empty type (`[[no_unique_address]]`), so that `proxy` stays exactly as it
    is today.
- **Semantics do not depend on `T`.** Every member means the same for fixed- and variable-size types; only costs
  differ (construction, `sizeof`). A different type is reserved for a different contract, not a different cost.
- **Lazy means values, not layout.** Field values are still decoded on demand, one at a time, with no copy of
  the object (REQ-011..013). For variable-size `T` the *layout* is resolved eagerly: construction reads only the
  counts needed to place each member, never the values. The cost is zero for fixed-size `T`, N count reads with
  fixed-size elements, and a walk over element counts with variable-size elements. `frame` already works this
  way: it reads the header length fields to locate the payload.
- **To document** for variable-size `T`:
  - construction reads the counts and walks the variable-size members, even if only a prefix field is used
    afterwards;
  - the precondition "the buffer covers the object" is hit at construction, which is where the hardened check
    goes;
  - a proxy cannot be built over a partially received object;
  - the layout is frozen at construction: the viewed bytes must not change while the proxy is alive. Changing a
    count afterwards (e.g. reusing the buffer for the next message) leaves stale offsets that can point outside
    the span, whereas `frame` and fixed-size proxies would only observe new values;
  - thread safety is unaffected: construction only reads the shared buffer, and afterwards every member is
    `const` with no `mutable` state, so concurrent calls on the same proxy are safe as long as nobody writes the
    buffer;
  - `sizeof(proxy)` grows by one offset per variable-size member.
- **Nested variable-size structs**: the parent's table gives the child its exact span, but the child builds its
  own table again. A private constructor taking the already computed entries can remove that double walk later,
  if profiling calls for it.

Rejected alternatives:

- **A lazy table in a `mutable` member.** It would spare prefix-only reads the construction cost, but caching
  behind `const` needs `mutable`, which is only justified for synchronization primitives such as a mutex; it
  would also break concurrent `const` access to the same object.
- **Dropping `const` from the accessors** to allow a lazy cache, as `std::ranges::filter_view::begin()` does:
  the accessors do not change the object's observable state.
- **Recomputing on every access plus an explicit `resolve()`**: it pushes an extra step onto the user, and
  variable-size types would no longer be used exactly like fixed-size ones.

**Open**:

- **Counts outside the struct**, e.g. a header field sizing a payload's vector. The payload alone is not
  self-delimiting and its length depends on the header, which crosses levels. This is related to the
  repeat-count annotation for `many` in [framing.md](framing.md#header-requirements).
- **A trailing vector with no count** (sized by the end of the buffer) would make the struct buffer-delimited.
  Probably not allowed at struct level, since `frame<H, blob>` already expresses it.

---

## 9. Uniform field access across strategies

**Decided, not implemented.** Code that is generic over the deserialization strategy — benchmarks comparing
`lazy`, `eager` and `in_place` above all, and strategy-parameterized tests — gets a uniform interface through a
**free accessor**, not through a single `proxy<T, S>` type.

### Why not `proxy<T, S>`

[framing.md](framing.md#consequences-and-open-problems) sketched `proxy<T, S = lazy_t>`, reached through
`deserialize<T>(buf, as_proxy(eager))`, with the same `field<"name">()` interface for every strategy. It mixes
two contracts under one name:

| `proxy<T, S>` | Depends on the buffer after construction |
| --- | --- |
| `lazy_t` | yes: a view over the bytes |
| `in_place_t` | yes: a reference into the bytes |
| `eager_t` | **no**: it would own a materialized `T` |

`return_type<S, T>` already mixes an owned value (`T`), a reference into the buffer (`T const&`) and a view
(`proxy<T>`), and that is fine, because each type states its semantics. The conflict only appears when all three
share the name `proxy`, which promises "I am a view" and would not be one in eager mode. What needs unifying is
**how fields are accessed**, not the type.

### The free accessor

Every strategy keeps returning its natural type, and a free function works on all of them, the way `std::get`
works on both `tuple` and `variant`:

```cpp
namespace rbe {
// T, T&, T const& -> reference to the member (reflection-based access)
// dsrl::proxy<T>  -> forwards to proxy.field<Path...>()
template<static_string... Path>
constexpr decltype(auto) field(auto&& source);

template<unique_annotation auto Annotation>
constexpr decltype(auto) field(auto&& source); // same, looked up by annotation
}
```

```cpp
template<strategy S>
void bench_price(std::span<std::byte const> buf, S strategy) {
  auto&& msg = rbe::deserialize<Order>(buf, strategy); // T, T const& or proxy<T>
  do_not_optimize(rbe::field<"price">(msg));
}
```

- **No mixed contracts.** `proxy` is always a view, `T` always owns its data, `T const&` always refers to the
  buffer.
- **Nothing to invent for eager.** There is no owning `proxy`.
- **No overhead for eager and in_place.** `rbe::field<"price">(msg)` reduces to `msg.price`, so a benchmark does
  not measure an artificial wrapper. Still check it: measure raw access as a baseline, or confirm in the
  assembly that the accessor disappears.
- **Framing gets it for free.** `frame::payload(S)` already returns `return_type<S, T>`, so generic code over
  frames uses the same accessor.
- **`auto&&` covers all three returns**: it extends the lifetime of the eager temporary and binds the in_place
  reference and the proxy directly.
- **To document**: the result type is not identical across strategies. Owned values and references yield a
  reference to the member; a proxy yields a decoded value, or a nested proxy for class-type members.

Hand-written code keeps the raw returns: `msg.price` is still better than `msg.field<"price">()`.

### Eager versus lazy for variable-size types

Which backend wins is workload-dependent, which is exactly what the benchmarks are for. One point to keep in
mind: **eager does not benefit from lazy's offset table**, since it reads sequentially and picks up the counts
while copying. The table is a cost specific to lazy, not a cost eager gets for free. On top of lazy, eager adds:

- **Allocation**: every `std::vector` goes to the heap, which usually dominates in hot paths and conflicts with
  heap-less or freestanding builds;
- **Copying and converting every element**: O(N) at construction, versus O(1) per accessed element for lazy,
  which hands out a zero-copy range.

Eager tends to win when almost every field and element is read and allocation is acceptable. Lazy tends to win
for sparse access, such as filtering network messages on a couple of fields, and whenever the heap is off
limits.

**Rejected alternative — a proxy viewing a materialized `T`** (`rbe::view_of(value)`): no proxy would own
anything, only its lifetime dependency would change (buffer or object). It is more coherent than an owning
eager proxy, but the free accessor solves the same need with fewer pieces.

## 10. Header-only messages

**Open.** Some protocols have messages that are only a header, such as `aquis::Heartbeat` and `opra::Control`.
With the header declared once on the frame, such a message has an empty payload, and an empty struct is not
wirable, so it cannot be an alternative of `rbe::any`. The market examples leave them out of their `messages`
with a TODO.

Candidates:

- Make an empty class wirable, with a wire size of 0, so it participates in `any` like any other alternative.
- Let `rbe::any` accept an id with no payload type.

---

## Divergences from framing.md

[framing.md](framing.md) predates the implementation. Known points where it no longer matches:

| framing.md | Now |
| --- | --- |
| `frame_length()` accessor | `length()` |
| `truncated()`: a short or malformed frame is reported, never a precondition | The frame is an unchecked view with documented preconditions (checked in hardened builds); validation belongs in a checked factory ([§2](#2-a-frame-is-a-view-the-buffer-may-be-larger-than-the-frame)) — still to reconcile with the "partially received buffer stays walkable" goal |
| `as_span()` "clamped to the buffer" | `as_span()` is exactly the frame: the span is narrowed at construction, and covering the frame is a precondition; `length_of()` serves partially received buffers |
| `remainder()` on `frame` | Not implemented; appears in the proposed `parse` factory and possibly on `many` |
| `many` yields an overrunning element with `truncated()` set | Undecided under the precondition model |
| Header length field "required only when the extent can't be determined any other way" | Formalized as `payload_extent` and `self_delimiting_frame` ([§3](#3-length-resolution), [§4](#4-self-delimiting-and-buffer-delimited-frames)) |
| `[[= rbe::id_field]]` locator | `[[=rbe::id]]` on the header field, `[[=rbe::id(value)]]` on the message type |
| `proxy<T, S = lazy_t>` reached through `as_proxy(S)` for strategy-generic code | Rejected: it mixes owning and view contracts; a free `rbe::field<...>(source)` accessor instead ([§9](#9-uniform-field-access-across-strategies)) |

---

## Pending work

- [ ] Add `payload_extent::any_id` and the explicit `is_self_delimiting` branches ([§4](#4-self-delimiting-and-buffer-delimited-frames)).
- [ ] Implement `dsrl::any` built from `(index, span)`, with id resolution as in [§5](#5-any-with-id-implied-lengths-any_id).
- [ ] Constrain `rbe::many` / `dsrl::many` with `self_delimiting_frame`.
- [ ] Settle the unknown-id contract ([§6](#6-unknown-ids)) and the hardened-check mechanism.
- [ ] Reword the `buffer_delimited_frame` `@note` ([§4](#buffer-delimited-frames-are-legitimate)).
- [ ] Checked `parse` factory with remainder; reconcile with `truncated()` in framing.md.
- [ ] `proxy`: narrow the span at construction (`data_.size() == length()`); fix the `data()` inconsistency with `frame`.
- [ ] Variable-size wirable types ([§8](#8-variable-size-wirable-types)): static minimum, offset table built in the
      `proxy` constructor behind `offset_of(index)`, and a separate `payload_extent` case for variable-size payloads.
- [ ] Free `rbe::field<...>(source)` accessor over `T`, `T const&` and `proxy<T>` ([§9](#9-uniform-field-access-across-strategies)), then strategy-parameterized benchmarks.
- [ ] `srl::frame` / `srl::many` on top of the same classification.
- [ ] Length units (words, element counts).
- [ ] Header-only messages ([§10](#10-header-only-messages)), then add `aquis::Heartbeat` and `opra::Control` back to their `messages`.
- [ ] Fold these notes and framing.md into user-facing documentation.
