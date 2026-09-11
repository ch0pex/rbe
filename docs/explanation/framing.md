# Message Framing Redesign

> **Status: design, not implemented.** Supersedes the "Packet Composition" and "Type Erasure and
> Message Dispatch" sections of [Design Overview](design-overview.md), and replaces `msg_list`,
> `any_msg` and the unfinished `packet`. Not linked from site navigation yet — same convention as
> [Annotation System Redesign](annotation-system-v2.md).

## Motivation

Every message in `example/markets/*.hpp` embeds a `Header header` as its first member, copy-pasted
across every message of every protocol. `msg_list` already requires all its candidates to share an
identical `id`/`length` layout (`message_concepts.hpp:40-55`), so that header is redundant per-message:
it should be declared once, against the *set* of messages. Doing that collapses `msg_list`, `any_msg`
and `packet` into a single primitive — which also matches how framed protocols nest in general
(Ethernet → IP → TCP → payload), as `london.hpp:360-380` already does with `UnitHeader` wrapping
per-message `Header`s.

## `frame<Header, T...>`

```cpp
template<frame_header Header, frame_payload Payload>
class frame {
public:
  using header_type = Header;
  using buffer_type = std::span<std::byte const>;

  /// Precondition: buffer.size() >= wire_size_of<Header>(). Nothing about the payload is validated
  /// here -- a short or malformed payload is reported by truncated(), never thrown, as msg/any_msg
  /// already do (so a partially-received stream buffer stays usable).
  constexpr explicit frame(buffer_type buffer);

  template<strategy S> constexpr auto header(S) const -> return_type<S, Header>;

  // payload accessors -- which ones exist depends on the payload shape, see the table below
  template<strategy S> constexpr auto payload(S) const;   // wirable payload
  constexpr auto payload() const;                         // bytes-constructed payload
  template<message_dispatcher Overload> constexpr auto match(Overload) const;  // candidate set

  constexpr auto header_length() const -> std::size_t;  // see Length semantics below
  constexpr auto payload_length() const -> std::size_t;
  constexpr auto frame_length() const -> std::size_t;   // header + payload
  constexpr auto truncated() const -> bool;       // frame_length() exceeds the buffer
  constexpr auto as_span() const -> buffer_type;  // this frame's bytes, clamped to the buffer
  constexpr auto remainder() const -> buffer_type;// bytes past this frame
};
```

### Payload shapes

The payload is always exactly *one* type, and only two rules govern what it may be:

- **`wirable`** — deserialized on access with any strategy (`eager`/`lazy`/`in_place`/`in_place_mut`),
  returning `return_type<S, T>` exactly like the free `rbe::deserialize`: `payload(lazy)` yields
  `proxy<T>` (today's `msg<T>`, see below).
- **constructible from the payload bytes** (`std::span<std::byte const>`) — handed those bytes and
  returned as-is, via a plain `payload()`. This is already spelled `packet_payload` in today's
  `packet.hpp:33`. A payload whose meaning depends on the header takes a second constructor parameter,
  `proxy<Header>`, and `frame` passes it when the type accepts it.

Every shape below is a consequence of those two rules, not a special case in `frame`: a span, a nested
`frame`, `any<...>` and `many<...>` are all just types satisfying the second one — and so is any type
*you* write.

The header parameter is what keeps `any<...>` out of the special-case list. With the discriminant moved
onto `Header`, payload bytes alone no longer identify a candidate, so a payload that dispatches on the
header has to see it — and passing the whole `proxy<Header>` rather than just the id costs nothing and
reaches further: OPRA's second discriminant (`msg_indicator`, see Scope) and any hand-written parser that
needs a header field are the same shape, not new rules.

| Shape | Spelled as | Accessor |
| --- | --- | --- |
| Candidate set | `frame<H, any<AddOrder, ReduceSize, Trade>>` | `match(overload)`, `as_variant()` |
| One wirable | `frame<H, LoginRequest>` | `payload(S) -> return_type<S, LoginRequest>` |
| Nested frame | `frame<UnitHeader, frame<H, any<A, B>>>` | `payload() -> frame<H, any<A, B>>` |
| Repeated frames | `frame<UnitHeader, many<frame<H, any<A, B>>>>` | `payload()` -> range of `frame<H, any<A, B>>` |
| Anything over bytes | `frame<H, std::span<std::byte const>>`, `frame<H, YourType>` | `payload() -> T` |

The last row is the escape hatch: a payload RBE can't describe declaratively — a hand-written parser, a
compressed or encrypted blob, or plain bytes you slice yourself — still gets framed by RBE (header
fields, bounds, truncation, iteration over a unit) and handed its bytes.

`any<T...>` names the semantics ("the payload is one of these") without promising storage: it is not
a `std::variant` and needn't be implemented as one. Naming a concrete representation in the type would
prejudge how a candidate resolves, which differs by header — with an explicit length field the bounds
are known before the id is even read, while with no length field at all the id must resolve the
candidate first and *its* wire size supplies the bounds (today's `explicit_length`/`implicit_length`
split, `message_concepts.hpp:60-68`).

Its responsibility is exactly one thing: **map a discriminant value to one of `T...` and view the bytes
as it.** It is today's `any_msg` with the framing half removed. What that means concretely:

- **The id is an input, not something `any` reads.** `frame` reads it from the header and hands it over
  at construction; `any` never locates a field. `is<U>()`, `as<U>(strategy)`, `as_variant()` and
  `match(...)` all keep working off that id, so `match` stays on `any` — mapping id to type *is* its
  job, and it is the only thing that knows the candidates.
- **Lengths, bounds and truncation leave it.** `length()`, `truncated()`, `as_span()` and `remainder()`
  are framing, and `frame` already has them. All `any` still answers about size is its *resolved
  candidate's* wire size — which is precisely what `frame::payload_length()` falls back to when the
  header carries no length annotation.
- **So the `explicit_length`/`implicit_length` specialization disappears** (`any.hpp:174,232`). It only
  ever existed because `any_msg` had to produce a `length()` two different ways; with that question
  answered by `frame`, `base_any` plus its two specializations collapse into one class.
- **`frame::match(...)` is a forwarder** to `payload().match(...)`, kept because `frame` is the object
  callers hold. `frame::payload()` still hands out the `any` itself for anyone who wants to keep it.
- **Candidate ids come from `rbe::id(value)` on the candidate** rather than
  `default_annotation_value<rbe::id, U>()` reading an embedded `Header` — the same
  `msg_list::ids` array (`message_list.hpp:42-49`), sourced differently. The candidates' common id type
  must match the header's `id_field` type; that is checkable where `frame` is instantiated.

`many<T>` is the same kind of name in the same slot, and makes the same non-promise: it says the payload
is a *run* of `T`, not how the run is stored — and nothing is stored, since it is a view over the payload
bytes. `payload()` yields a range of `T`, walked lazily: each step advances by the element's own
`frame_length()`, which is exactly the TLV walk the length annotations define. That walk fixes the
range's category, so it is worth stating rather than discovering:

- **Forward, not random-access.** Element *n+1*'s offset is only known after reading element *n*'s
  length. (The exception is a fixed-size element type — no length field, one compile-time stride — where
  random access falls out for free.)
- **Sized only when the header carries a count** (`UnitHeader::message_count`): then `size()` is that
  field, with no walk. With no count the run ends at the payload bounds, `size()` would cost a full walk,
  and it is deliberately not a `sized_range`.
- **Iteration stops at whichever bound comes first**, count exhausted or payload bounds reached. An
  element whose `frame_length()` overruns those bounds is still yielded, with its own `truncated()`
  reporting it — the same "report, never throw" contract `msg`/`any_msg` already keep, so a
  partially-received buffer stays walkable.

A candidate set has no single return type, so it gets `any_msg`'s dispatch instead, unchanged in shape:

```cpp
frame.match(
  [](proxy<AddOrder> m) { /* lazy */ },
  [](ReduceSize m)      { /* eager -- whichever the overload is invocable with */ },
  [](auto id)           { /* fallback for an unrecognized id */ }
);
```

Shapes compose, so a two-level protocol is one declaration:

```cpp
using packet = rbe::frame<
  UnitHeader, // Packet header
  rbe::many< // Packet payload: a sequence of messages, each with its own header
    rbe::frame<
      MsgHeader, // Message header
      rbe::any<AddOrder, ReduceSize, Trade> // Message payload: one of the candidate types
    >
  >
>;

rbe::deserialize<packet>(buffer, lazy) -> packet::dsrl_type;

rbe::deserialize<packet>(buffer, eager) -> packet::value_type;

rbe::serialize<packet>(buffer, packet::value_type {
  .header = {},
  .payload = {
    {hdr, msg1},
    {hdr, msg2},
    {hdr, msg3},
  },
});

packet::srl_type builder{buffer}
  .header(unit_header)
  .payload()
    .append(hdr1, msg1) // maybe we can assert here that hdr1 id and msg id_value match 
    .append(hdr2, msg2)
    .append(hdr3, msg3)

```

### Header requirements

- Fixed compile-time size, `wirable`, and `well_annotated`.
- `[[= rbe::id_field]]` field — required iff the payload is an `any<...>`.
- A length field (`payload_length` / `frame_length`, see below) — required only when the payload's
  extent can't be determined any other way. It can be, in two cases: the payload has a fixed
  compile-time size (for an `any<...>`, that means *per candidate*, after `id` resolves which one it is —
  what `any_msg<implicit_length T>` already does — not one size shared by the set); or the payload is a
  `many<...>` whose count is known and whose elements each carry their own extent, so the run can be
  walked. Aquis is the second case: its `PacketHeader` is a bare `count` with no length field at all
  (`aquis.hpp:123-125`), and each enclosed message's own `Header` carries the length.
- For a bytes-constructed payload, the length bounds it if present; otherwise the payload is the whole
  remainder of the buffer.
- For a `many<...>` payload, a field giving the repetition count (`UnitHeader::message_count` in london).

### Length semantics

A length field says nothing until it says *what it counts*, so the single `rbe::length` splits into
three annotations, one per quantity:

| Annotation | The field counts |
| --- | --- |
| `[[= rbe::header_length]]` | the header's own bytes |
| `[[= rbe::payload_length]]` | the payload's bytes, header excluded |
| `[[= rbe::frame_length]]` | header + payload — the frame's whole extent |

One identity relates them:

```
frame_length = header_length + payload_length
```

so a header declares only the quantity it actually transmits, and `frame` derives the other two:

- **`header_length()`** — the annotated field when present, otherwise `wire_size_of<Header>()`. Header
  length is a quantity in its own right precisely so it can be *either*: implicit is the common case, but
  a protocol that transmits it can say so, and this is what the payload offset is read from.
- **`payload_length()`** — the annotated field when present; otherwise `frame_length() - header_length()`;
  otherwise the payload's own extent (fixed size per candidate, a walked `many<...>`, or the remainder of
  the buffer, per the rules above).
- **`frame_length()`** — the annotated field when present; otherwise `header_length() + payload_length()`.

TLV is the composed case rather than a special one: a header carrying `id` + `payload_length` yields its
frame extent as `header_length() + payload_length()`, with the header half of that sum implicit.

Against the examples: cboe's `SequencedUnitHeader::length` ("entire block including this header",
`cboe.hpp:240`), london's `UnitHeader::length` and the per-message `Header::length`s of cboe, aquis and
london (all defaulted to header + body, e.g. `.length = 22` on a 2 + 20 byte `Login`) are all
`frame_length`. Nasdaq's SoupBinTCP prefix, documented as "excludes itself" (`nasdaq.hpp:314`), is a
`payload_length`: "excludes itself" is what payload length reads like when the framing header is a lone
length field, and modelling it as one — `frame<SoupHeader, frame<ItchHeader, any<...>>>` — makes it
exactly that, instead of a third length convention.

`rbe::length` goes away rather than surviving as an alias for `frame_length`. An unqualified "length" is
the ambiguity the split exists to remove, and keeping it would let a protocol declare the common case
without ever stating which convention it meant. The migration is mechanical: one annotation each in
cboe, aquis, nasdaq and london.

Two consequences worth naming:

- **Explicit `header_length` reaches part of the variable-size-header gap below.** With the payload
  offset read from the wire rather than from `wire_size_of<Header>()`, a fixed `Header` describing the
  mandatory prefix plus trailing options in the gap up to `header_length()` is expressible — IPv4's IHL
  in shape, though not yet in units (see below). A `header_length()` *smaller* than `wire_size_of<Header>()`
  is a malformed frame, reported like truncation rather than thrown; exactly which predicate reports it
  is left with the truncation API.
- **A proxy exposes whichever of the three its own type carries**, each behind a `requires` — the same
  rule `id()` already follows — so `proxy<Header>` gets `header_length()`/`payload_length()`/`frame_length()`
  as annotated, and the *derivation* lives only in `frame`, which is the only thing that knows both
  halves. This replaces today's unconstrained, convention-free `msg::length()`.

Still open: **units**. All three annotations above count bytes; IPv4's IHL counts 32-bit words and some
protocols count elements rather than octets, which needs a scale modifier on the annotation
(`[[= rbe::header_length(rbe::in_words<4>)]]` in shape). Also open, smaller: whether declaring all three
on one header is rejected as over-determined or accepted with the redundant field checked against the
identity at runtime.

### Scope

The escape hatch makes the *payload* unconstrained, but `Header` still has to be a fixed-size, byte-
aligned binary struct. That covers the exchange and market-data protocols RBE targets; it is not "any
network protocol". Known outside the model:

- **Bit-level headers** (IPv4 version/IHL, TCP flags). `memory_layout.hpp` doesn't support bit-fields
  yet (`// TODO: add support for bit fields`), so the Ethernet → IP → TCP chain this design borrows as
  a mental model isn't actually parseable by RBE today.
- **Variable-size headers** (IPv4 IHL, TCP data offset). `Header` must have a compile-time size; the
  workaround is a fixed `Header` plus the options landing in the region between `wire_size_of<Header>()`
  and an explicit `header_length` (see Length semantics) — expressible in shape, but not for IHL itself
  until the annotation gains units.
- **Self-delimiting text protocols** — a message whose framing lives in ASCII fields rather than a fixed
  binary prefix has nothing for `Header` to bind to, so it isn't a `frame` at all. Under a binary
  session or transpor header it is, and its body is the last row above.
- **Open — trailers.** `frame` is header + payload, with no third region for an Ethernet FCS or a
  trailing CRC/checksum. Today they'd have to be absorbed into the payload.
- **Open — composite discriminants, and length fields inside the body.** `any<...>` assumes a single
  `id` field. OPRA needs two: its `Header` has no length, and a message's extent depends on
  `msg_category` (the id) *and* `msg_indicator`, which appends an optional 10- or 20-byte BBO appendage
  to a quote (`opra.hpp:354-391`). Separately, its `Administrative` message carries `msg_data_length`
  inside the payload rather than the header (`opra.hpp:496-500`). OPRA is the one example protocol this
  design does not yet cover.

## Candidate ids move onto the candidates

With no embedded `Header` member there's nowhere to hang the default member initializer that today
carries a candidate's id value (`default_annotation_value<rbe::id, U>()` reading
`Header header {.msg_category = ...}`). It moves to the candidate type itself:

```cpp
struct[[= rbe::pack_be, = rbe::id(msg_category_t::long_quote)]] LongQuote {
  symbol_t security_symbol;  // no Header member
  // ...
};
```

The two halves get separate names because they play different roles: `[[= rbe::id_field]]` on a *header
field* locates where the id lives, and `[[= rbe::id(value)]]` on a *candidate type* claims which value
it answers to. The value form takes the shorter spelling because it's written once per message (34
times in opra, 27 in cboe TOP) while the locator is written once per protocol. Accessor-style names
(`id::get`/`id::set`) were considered and dropped: neither annotation reads or writes anything — one
locates, the other declares — and nesting would force `id` to become a scope for no semantic gain.

The call syntax also extends to candidates claiming several ids at once — `rbe::id(C, G, K, M, N, P)` —
which is what OPRA needs (see Scope). That requires relaxing the `unique` rule the `id` dimension has in
`annotation-system-v2.md`, or having the single annotation take a list.

## Consequences and open problems

- `is_msg_list_type`'s cross-candidate layout comparison (`message_concepts.hpp:40-55`) becomes
  unnecessary rather than reimplemented: with `id`/`length` declared once on `Header`, there is nothing
  left to compare.
- **Padding between header and payload is never inferred — it's declared.** There is no implicit padding
  on the wire: bytes are there or they aren't, and implicit padding is a property of C++ in-memory
  layout, not of a wire format. A protocol with gap bytes between header and body says so, and the
  examples already do (`filler` in `cboe.hpp:270`, `reserved` in `opra.hpp:419`). Declared as a header
  field, it falls out of `wire_size_of<Header>()` and the payload offset needs no special case — so
  `frame` deliberately never tries to reconstruct boundary padding from `rbe::align`.
- **Open:** the repeat-count annotation's name and scope, and whether it can reuse the member-level
  `count` still open in `annotation-system-v2.md`.
- **`msg<T>` is renamed `proxy<T, S = lazy_t>` and needs no owning-`Header` parameter.** It was never
  message-specific — it's the lazy deserializer, whose real getter is `field<"name">()`, and under this
  design it views headers as often as messages. `proxy` is already this codebase's own word for it
  (`any_msg.hpp:8`, `tags.hpp:37`), and defaulting the strategy leaves room for a future uniform
  `proxy<T, eager_t>`/`proxy<T, in_place_t>` — same `field<"name">()` interface, differing only in *when*
  the bytes are read — without a second rename. (`lazy<T>` is unavailable regardless: `rbe::dsrl::lazy`
  is already the strategy tag, `tags.hpp:56`.) Those would be reached through a strategy *modifier* —
  `deserialize<T>(buf, as_proxy(eager))` — rather than by naming `proxy<T, S>` directly in a `frame`
  payload, which would otherwise give two spellings for the same thing (`frame<H, Login>` +
  `payload(eager)` vs `frame<H, proxy<Login, eager_t>>` + `payload()`). One modifier covers every
  strategy, present and future, where a per-strategy tag set (`proxy_eager`, `proxy_in_place`, …) grows
  with each one; `as_proxy(lazy)` is just `lazy`, since lazy has no non-proxy form. A piped spelling
  (`eager | as_proxy`) can forward to that same function later, if a second modifier ever makes the
  composition worth its machinery — with only one modifier it's an `operator|`, a composed tag type and
  a widened `strategy` concept to say what a one-line function already says. The raw returns stay raw —
  `return_type<eager_t, T>` is still `T`, because for hand-written code `msg.price` beats
  `msg.field<"price">()`. Defer all of it until something generic over strategies actually needs it;
  benchmarks and strategy-parameterized tests are the plausible first consumers, and adding the modifier
  later touches nothing that exists today.
- **`id()` and the length accessors need no threading either.** A proxy exposes them iff *its own* `T`
  carries the annotations, which is what `msg::id()` already does
  (`requires(rbe::identificable<value_type>)`). So `frame::header(lazy)` yields a `proxy<Header>` with
  `id()` and whichever of `header_length()`/`payload_length()`/`frame_length()` the header declares, the
  payload's `proxy<Payload>` has none of them, and nothing passes between them. Today's
  `msg::length()` — unconstrained, and naming a quantity it never defined — is replaced by those three,
  each behind the `requires` its `id()` counterpart already has.

## Files touched

| File | Change |
| --- | --- |
| `rbe/core/message_list.hpp` | `msg_list<T...>` replaced by `any<T...>` (no per-candidate header, no cross-candidate checks). |
| `rbe/core/message_concepts.hpp` | Cross-candidate comparison dropped; `frame_header`/`frame_payload` concepts for the five shapes. |
| `rbe/dsrl/any_msg.hpp` | Renamed `any.hpp`; keeps `match`/`as_variant`/`is`/`as` but takes the id as a constructor input, drops the framing accessors, and loses the `explicit_length`/`implicit_length` split. |
| `rbe/dsrl/packet.hpp` | Removed; superseded by the single-wirable, nested and `many<...>` shapes. |
| `rbe/dsrl/proxy.hpp` | `msg<T>` renamed `proxy<T, S = lazy_t>`; `length()` becomes `header_length()`/`payload_length()`/`frame_length()`, each `requires` its annotation. |
| `rbe/core/metadata_layout.hpp` | `get_length_layout` becomes one lookup per length annotation; `frame` derives the missing quantities from the identity. |
| `rbe/annotations/id.hpp`, `length.hpp` | `id` splits into `id_field` (locator) and `id(value)` (claim); `length` splits into `header_length`/`payload_length`/`frame_length`; new repeat-count annotation. |
| `example/markets/*.hpp` | Messages drop their `Header` member and gain `id(value)`; `length` becomes `frame_length` (`payload_length` for nasdaq's Soup prefix); each protocol gains a `frame<...>` alias. |
