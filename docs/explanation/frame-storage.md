# RBE Feature Proposal: What a Frame View Stores

### Summary

A frame view has to keep enough state to answer `length()`, `header()` and `payload()` after
construction. Two designs are on the table, and they differ only in **how much of the work `make()`
already did survives it**:

- **Design A — the resolved payload view** (what `base_frame` does today): store the header view and
  the constructed payload view. Nothing is ever resolved twice, and `sizeof` grows with the nesting.
- **Design B — the header plus a cached frame length**: store the header view and one `size_type`.
  `sizeof` is a constant 24 bytes and `length()` is a single load; `payload()` rebuilds its view on
  each call.

Both are *eager*: in neither of them can `make()` avoid constructing the payload, because that
construction **is** the precondition check. The choice is what to keep afterwards, not when to do it.

Related: [Framing Design Notes](framing-design-notes.md) §2 (a frame is a view, the buffer may be
larger), §5 (resolve once, carry the index), §8 (layout eager, values lazy) and §11 (`base_frame`
primitives versus derived operations).

### Motivation

`dsrl::is_frame` requires `length()`, `header()`, `payload()`, `as_span()` and `data()` of every
frame (`dsrl/concepts.hpp:55-69`), and `base_frame` derives the spans from `length()`
(`dsrl/base_frame.hpp:110-121`). Today all of that is served by storing both views:

```cpp
header_return_type header_;    // header<H>, i.e. proxy<H>: the untrimmed buffer span, 16 bytes
payload_return_type payload_;  // payload_view_t<P>: proxy, span, any, many or a nested frame
```

That makes `sizeof(frame)` a function of the frame's composition rather than a constant. Measured over
the market examples (GCC 16, Debug):

| | bytes |
| --- | --- |
| `header<H>` | 16 |
| `any<...>` (aquis, nasdaq, and every other candidate set) | 32 |
| `frame<H, any<...>>` — every protocol's `message` | **48** |
| `many<message>` | 80 |
| `frame<PacketHeader, many<message>>` — aquis / cboe pitch / lse `packet` | **96** |
| `frame<SoupHeader, message>` — nasdaq's frame of frame | 64 |
| `cboe::top::line_view` (custom, stores its own length) | 56 |
| `many<line_view>` | 88 |
| design B, any composition | **24** |

Frames travel **by value** during iteration — `many::current()`
returns one, and so do `operator*` and `operator->` through `arrow_proxy`
(`dsrl/many.hpp:63-71,100-103`) — so the width is paid per element, and the whole nested chain is
built even for a caller that only reads a header field.

#### The invariant both designs share

It is tempting to state the fork as *eager versus lazy payload construction*. It is not, and this is
the point that decides the shape of both designs:

> In a wide-contract `make()`, constructing the payload **is** the precondition check.

`try_construct_payload<P>` dispatches to `proxy<T>::make` (does `T`'s wire size fit?),
`any::make(id, bytes)` (does the candidate the id selects fit?), the nested frame's own `make()`
(recursively), or the span itself (`dsrl/payload.hpp:135-157`). There is no cheaper oracle for *do
these bytes hold this payload*: a separate validation pass would have to repeat exactly what those
constructors already do. A design that skipped construction in `make()` would not be lazy, it would
be unchecked.

So `make()` always constructs. What differs is what it hands to the frame it returns.

### Design

#### Design A — the resolved payload view (implemented today)

```cpp
header_  : header<H>              // 16
payload_ : payload_view_t<P>      // 16 .. 56
```

- `length()` branches at compile time on the delimiting category (`dsrl/frame.hpp:77-87`): the buffer
  size for a buffer-delimited frame, the header's `frame_length()` when the header declares one, and
  otherwise `header().length()` plus the payload's own extent through the `detail::payload_length`
  CPO (`dsrl/detail/payload_size.hpp:53`, which resolves `.length()` → `.size()` → ADL).
- `payload()` is free: the view is already built.
- `length()` over a nested chain is O(depth), but in *reads*, never in re-resolution.
- `as_span()`, `payload_span()` and `flatten()` dispatch through deducing `this`, so a derived class
  that supplies its own `length()` drives them (`dsrl/base_frame.hpp:110-121`).
- `length()` and `make()` are the two primitives a derived frame must supply; `base_frame`
  deliberately has neither, so forgetting one is a compile error rather than an iteration that
  silently advances by the wrong length (notes §11).

What it buys, precisely: **nothing is ever resolved twice.** That matters for the frames whose length
comes from the payload instead of from a header field — a dispatch-delimited frame costs an id lookup
to know its length, and `length()` is what `as_span()`, `payload_span()` and `many`'s iteration all
end up calling. It is also what makes notes §5 ("resolve once, carry the index") true today: the
iterator's `current_` holds a frame whose `any` is already resolved, so dereferencing does not look
the id up again.

What it costs: the width above, and building a chain the caller may never descend.

#### Design B — the header plus a cached frame length

```cpp
header_ : header<H>   // 16: the untrimmed buffer span
length_ : size_type   //  8: the frame length, resolved once
```

Everything else derives:

| | |
| --- | --- |
| `length()` | `length_` — one load, for every delimiting category |
| `buffer()` | `header_.buffer()` — unchanged, still the untrimmed buffer (`header.hpp:70`) |
| `as_span()` | `buffer().first(length_)` |
| `header_span()` | `header_.as_span()` — unchanged |
| `payload_span()` | `as_span().subspan(header_.length())` |
| `payload()` | `construct_payload<P>(header_, payload_span())` — the narrow form, rebuilt per call |

Cache the **frame** length rather than the payload extent: `length()` is the hot path — `many::next()`,
`as_span()` and `payload_span()` all route through it — so it should be the load, and the subtraction
should be paid by `payload_span()`, not the other way round.

Two consequences beyond the size:

- **`length()` stops being a primitive of the derived class.** The base can serve it, because the
  derived class handed the extent to the base constructor; the derived class's only primitive is
  `make()`. The deducing-`this` machinery in `as_span()` / `payload_span()` becomes unnecessary, and
  the failure mode moves from "forgot `length()`" (a compile error, the argument of notes §11) to
  "passed the wrong extent" (a wrong value). A custom frame gets smaller: `line_view` loses both its
  `line_length_` member and its `length()` override, keeping only the `n` it already computes.
- **`payload()` re-reads.** A frame whose underlying bytes change between two calls would observe the
  new ones, whereas design A freezes the payload's layout at construction — which is the rule notes §8
  states for a variable-size `proxy` ("the layout is frozen at construction"). The two rules must not
  contradict each other in the documentation.

#### Side by side

| | A — resolved view (today) | B — header + cached length |
| --- | --- | --- |
| `sizeof` | 16 + the payload view: 48 per message, 96 per packet | 24, constant |
| `length()` | O(depth) reads, no re-resolution | one load |
| `payload()` | free | rebuilt: free for a `proxy` or a span; one id lookup for an `any`; recursive for a nested frame; a first-element parse for a `many` (its constructor calls `frame_type::make`, `dsrl/many.hpp:90-91`) |
| `buffer()` | kept | kept |
| spans | via deducing `this` on `length()` | plain `const` members |
| derived-class primitives | `make()` + `length()` | `make()` only |
| staleness | layout frozen at construction | re-read on each `payload()` |
| `make()` | construct, store the view | construct, keep the number |
| narrow constructor | construct (narrow forms), store the view | resolve the extent: build-and-discard, or a size-only resolver (below) |

B dominates A on everything except `payload()`, and A's advantage there is confined to the payload
categories that cost something to rebuild.

#### What design B needs before it can land

1. **A size-only length resolver**, so the narrow constructor does not have to build and discard a
   payload view just to learn the extent. The extent is free from the header for an explicit length, a
   static size and `buffer_end`; it is the payload that is authoritative for a nested frame, for an
   `any` and for a custom scan. For an `any` the resolver already exists but is private
   (`any::length_of(index, data)`, `dsrl/any.hpp:195`, over `candidates::wire_size`); for a nested
   frame it is the static `parse_length(buffer)` that notes §2 lists as **Implemented** and which is
   **not in the code** — `grep parse_length src/` finds nothing, it was lost in the refactor to public
   building blocks. Making both public also delivers what §2 wanted the resolver for in the first
   place: telling how many bytes a partially received buffer still needs, without constructing a
   frame. A custom scan stays irreducible, which is exactly the case that is already paying the scan
   once instead of per access.
2. **A decision on the `any` index.** Notes §5 keeps the resolved index inside the payload view, so
   under B a dispatch-delimited element pays the id lookup twice per iteration step: once to resolve
   the length in `next()`, once again when the user calls `payload()`. Candidates: accept it (a lookup
   is an `if` chain over a handful of integers); have the `many` iterator carry the index next to the
   frame (which is what §5 actually says — "the iterator resolves while advancing and keeps the
   index"); or let the resolved index travel in the frame, which costs the bytes B is trying to save.
3. **Documentation of the staleness rule** alongside notes §8, as above.

### Alternatives Considered

- **Store nothing but the buffer** (the frame is just the header view, a constant 16 bytes). `make()`
  checks its preconditions and throws the result away, and the narrow constructor is free: it keeps
  the span and nothing else. The price is that `length()` recomputes too, so for an implicitly
  delimited chain one level is rebuilt per level on *every* call — and `as_span()`, `payload_span()`
  and `many::next()` all call `length()`. Rejected: it moves cost into the hottest accessor in the
  library to save the 8 bytes that make that accessor O(1).
- **A `split_span` trimmed to the exact frame, with `middle` at the end of the header**
  (`core/detail/split_span.hpp`). Also 24 bytes, and it gives `as_span()`, `header_span()` and
  `payload_span()` as pure accessors (`left()` / `right()`), caching the header length as well. It
  loses `buffer()`: once the span is trimmed to the frame there is no record of where the caller's
  buffer ended, which `frame::length():79` and three test assertions
  (`tests/runtime/test_dsrl_frame.cpp:246,247,364`) rely on, and `header()` built from `left()` would
  report only the header bytes as its buffer. Design B keeps `buffer()` for the same 24 bytes, at the
  cost of re-reading the `header_length` field in `payload_span()` — a field read against losing an
  accessor. Note also that `split_span`'s API is cursor-flavoured (`advance`, `retreat`), which is the
  shape `many` needs and not the shape a fixed header/payload split needs.
- **Lazy construction without the validation step.** Not an option: see *The invariant both designs
  share*. It would not be a lazy frame, it would be an unchecked one.
- **Caching the payload view behind `mutable`** so the first `payload()` resolves and later calls do
  not. Rejected on the standing rule that a cache behind `const` needs `mutable`, which is reserved
  for synchronization primitives; compute in the constructor instead (notes §8, rejected
  alternatives).

### Open Questions

1. Is the `any` double lookup under B acceptable, or does the index travel in the iterator?
2. Does `base_frame` serving `length()` weaken the §11 guarantee enough to matter? A derived class can
   no longer forget the primitive, but it can pass a wrong extent, and that is not a compile error.
3. Should `payload()` under B return the narrow construction unconditionally, or should the frame keep
   a way to say "this frame was built by the narrow constructor, so the payload's preconditions were
   never checked"? Today both paths produce the same type.
4. Does B change the measured *time* in the direction the sizes suggest? The 48-against-24 per element
   is the argument for it; the counter-argument is that B pays two id lookups per element where A pays
   one (`make()` validates by building the `any`, then `payload()` builds it again), and that is
   unmeasured. A benchmark over `many<frame<H, any<...>>>` — the shape of all six protocols — would
   settle it.
5. If B lands, does `dsrl::is_frame` still require `length()` of a frame, or does it require the
   extent-carrying constructor instead?

### Decision Log

- **Implemented**: design A. `base_frame` stores `header_` + `payload_`; `make()` and the narrow
  `resolve()` both construct the payload (`dsrl/frame.hpp:51-97`); `length()` branches on the
  delimiting category; the spans follow a derived `length()` through deducing `this`.
- **Established here**: `make()` must construct the payload in both designs, because construction is
  the precondition check. The fork is what survives `make()`, not when the work happens.
- **Established here**: caching the frame length next to `header<H>` costs 24 bytes flat, keeps
  `buffer()`, and makes `length()` a load — it dominates both the buffer-only variant and the
  `split_span` variant.
- **Open**: whether to move from A to B, pending the resolver of point 1, the `any` index decision and
  a measurement.
