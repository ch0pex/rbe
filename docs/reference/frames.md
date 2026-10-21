# Frames Reference

Header: `rbe/frames.hpp`

A frame is a header followed by a payload. The payload may be a wirable type, one of several chosen by the
header's id (`any`), a run of frames (`many`), opaque bytes (`blob`) or another frame. Frames are read
through views over the buffer, like the lazy strategy, and a whole protocol is one type.

## Vocabulary types

Headers: `rbe/frames/frame.hpp`, `rbe/frames/any.hpp`, `rbe/frames/many.hpp`, `rbe/frames/blob.hpp`

```cpp
--8<-- "example/docs/reference_frames.cpp:types"
```

| Type | Parameters | Constraints |
|---|---|---|
| `rbe::frames::frame<Header, Payload>` | `Header`: any wirable type. `Payload`: a wirable or `empty` type, an `any`, a `many`, a `blob` or a frame. | |
| `rbe::frames::any<T...>` | Two or more candidates. | Every candidate is `wirable_class` or `explicitly_empty` and declares `[[=rbe::id(value)]]`; all ids have the same type and no two are equal. The header of the enclosing frame must have a field marked `[[=rbe::id]]`. A violation is a `consteval` diagnostic naming the rule. |
| `rbe::frames::many<F>` | One frame type. | `F` must be `self_delimited` and `constant_length`: it knows where it ends, in O(1). |
| `rbe::frames::blob` | | Opaque bytes: whatever the header or the buffer bounds. |

The vocabulary types are empty: they declare the shape and the associated types, and the view they lower to
has the API.

```cpp
--8<-- "example/docs/reference_frames.cpp:associated_types"
```

| Associated type | Meaning |
|---|---|
| `F::header_type`, `F::payload_type` | The two parameters. |
| `F::delimiting_concept` | The [delimiting category](#delimiting), deduced from the shape. |
| `F::dsrl_type` | The view `rbe::deserialize<F>` returns: `rbe::frames::dsrl::frame<Header, P>` with `P` lowered to its own view (`dsrl::any<T...>`, `dsrl::many<F::dsrl_type>`, `std::span<std::byte const>` for a blob). |

Concepts over the shape, in `rbe/frames/frame_concepts.hpp` and `rbe/frames/frame_serder_concepts.hpp`:
`rbe::frames::is_frame<T>` (has `header_type`, `payload_type` and a delimiting concept), `is_any<T>`,
`is_many<T>`, `header<T>` (`wirable`), `payload<T>`, `frame_serder<T>` (a frame that lowers to a view),
and the aliases `header_t<F>`, `payload_t<F>`, `many_element_t<M>`, `dsrl_t<F>`.

## Deserializing a frame

Header: `rbe/frames/deserialize.hpp`

```cpp
--8<-- "example/docs/reference_frames.cpp:deserialize"
```

| Function | Returns | Contract |
|---|---|---|
| `rbe::deserialize<F>(buffer)` | `F::dsrl_type` | Narrow: the buffer holds the whole frame. |
| `rbe::try_deserialize<F>(buffer)` | `std::optional<F::dsrl_type>` | Wider: `nullopt` if the header does not fit, or the payload the header announces does not. The memory is still assumed live. |

There is **no strategy argument**: a frame is read through its view, and each part of the view is lazy. The
view borrows the buffer, which must outlive it. There is no eager whole-frame decode; see
[Planned](#planned).

## Frame view

Header: `rbe/frames/dsrl/frame.hpp`, `rbe/frames/dsrl/base_frame.hpp`

`rbe::frames::dsrl::frame<H, P>` holds a header view and a payload view, built when the frame is.

```cpp
--8<-- "example/docs/reference_frames.cpp:frame_view"
```

| Member | Description |
|---|---|
| `static make(buffer) -> std::optional<frame>` | The checked constructor `try_deserialize` calls. |
| `explicit frame(buffer)` | Narrow. |
| `header()` | The [header view](#header-view). |
| `payload()` | The payload view, by value: a `proxy<T>` for a wirable payload, a `dsrl::any`, a `dsrl::many`, a `std::span<std::byte const>` for a blob, a nested frame view. |
| `get<0>()`, `get<1>()` | The same two, so `auto [header, payload] = frame;` works. |
| `flatten()` | A `std::tuple` of every header on the way down and the innermost payload. Stops at a payload that is not a frame. |
| `length()` | The frame's extent in bytes. Exists only for a [`constant_length`](#length-and-distance) frame. |
| `as_span()`, `payload_span()` | The `length()` bytes from `data()`, and those after the header. Only for a `constant_length` frame. |
| `header_span()` | The header's bytes as the wire delimits them. |
| `buffer()`, `data()` | The buffer the view was built over, untrimmed, and its first byte. |
| `header_type`, `payload_type`, `header_return_type`, `payload_return_type`, `buffer_type`, `size_type` | The shape and the two view types. |

## Header view

Header: `rbe/frames/dsrl/header.hpp`

`rbe::frames::dsrl::header<H>` is a [proxy](deserialization.md#proxy) over `H` plus what the
[length annotations](annotations.md#length) on `H` declare.

```cpp
--8<-- "example/docs/reference_frames.cpp:header_view"
```

| Member | Requires | Description |
|---|---|---|
| `field<...>()`, `value()`, `operator*`, `size()`, `buffer()`, `data()` | | From the proxy. `size()` is `wire_size_of<H>()`, what the code knows. |
| `id()` | `has_id` | The field marked `rbe::id`. |
| `length()` | | The header's length on the wire: the `header_length` field if there is one, else `size()`. |
| `frame_length()` | `has_frame_length`, or `has_payload_length` | Header plus payload, read or computed. |
| `payload_length()` | `has_payload_length`, or `has_frame_length` | The payload alone, read or computed. |
| `payload_count()` | `has_count` | The `payload_count` field. |
| `is_extended()` | | `length() > size()`: the wire declared header bytes `H` has no fields for. |
| `as_span()`, `known_span()`, `extension_span()` | | The header as the wire delimits it; the part `H` has fields for; the rest. `as_span()` is the two together. |
| `static make(buffer)` | | Checks that `H` fits and, with `header_length`, that the declared length fits too. |

The `static constexpr bool` flags `has_header_length`, `has_payload_length`, `has_frame_length`, `has_id`,
`has_count` and `delimits_payload` (`has_payload_length or has_frame_length`) say which of these exist.

## Any view

Header: `rbe/frames/dsrl/any.hpp`, `rbe/frames/dsrl/any_unmatched.hpp`

`rbe::frames::dsrl::any<T...>` is an id and the bytes that follow it. For a known id the bytes are trimmed
to the candidate's wire size; for an unknown one they run to the end of what the frame handed over.

```cpp
--8<-- "example/docs/reference_frames.cpp:any_view"
```

| Member | Description |
|---|---|
| `match(callbacks...)`, `match(overload_set)` | Decode the candidate the id selects and call the callback that takes it. See the rules below. |
| `is<T>()` | Whether the id selects `T`. |
| `as<T>(strategy = rbe::dsrl::lazy)` | `std::optional<return_type<S, T>>`: the candidate as a proxy or a value, `nullopt` unless `is<T>()`. For an `empty` candidate, `std::optional<T>`. |
| `id()`, `known_id()` | The id on the wire, with the header field's type; whether a candidate answers to it. |
| `size()`, `as_span()`, `data()` | The candidate's wire size for a known id, the rest of the buffer for an unknown one. |
| `static make(id, buffer)`, `any(id, buffer)` | Checked and narrow constructors; the frame calls them. `make` fails only when the id is known and its candidate does not fit. |
| `candidates`, `id_type`, `buffer_type`, `size_type` | |

Rules for the callbacks of `match`:

- A candidate is taken **by value** (`T` or `T const&`), decoded eagerly, or as **`rbe::dsrl::proxy<T>`**,
  decoded on access. A callback set that could take the same candidate both ways is ambiguous: compile
  error.
- An `empty` candidate is taken by value.
- A generic callback (`auto`, or a constrained `template`) handles every candidate it is invocable with.
  Constrain it so it is not invocable with both the value and the proxy of one candidate.
- A callback whose parameter is none of the candidates is a compile error.
- **One callback constrained by `rbe::unmatched`** is mandatory. It receives an object with `id` (the value
  on the wire), `known_id` (`true` when a candidate answers to it but no callback took it) and `data` (the
  bytes). `rbe::unmatched<T>` is the concept that matches that object's type.
- Every callback returns the same type; `match` returns it.

## Many view

Header: `rbe/frames/dsrl/many.hpp`

`rbe::frames::dsrl::many<F>` walks a buffer as consecutive `F` views. `F` must be `self_delimited` and
`constant_length`: each element says where it ends, and saying it is O(1).

```cpp
--8<-- "example/docs/reference_frames.cpp:many_view"
```

| Member | Description |
|---|---|
| `begin()`, `end()` | An input range: `begin()` is not `const`, and iterating advances the `many`. Elements are `F` views, by value. The range ends at the buffer's end, after `count` elements, or at the first element that is not whole. |
| `current()`, `next()`, `done()` | Manual stepping over the same walk. |
| `remainder()`, `as_span()`, `data()` | The bytes not yet consumed, the current element included. |
| `has_seen_partial()` | Iteration stopped on an incomplete element: more bytes are needed to read it. |
| `static make(buffer, count = max)`, `many(buffer, count = max)` | Never fails: an empty or incomplete buffer is a `many` that is already `done()`. |
| `frame_type`, `buffer_type`, `size_type`, `iterator` | |

A `many` is single-pass. `frame.payload()` returns a fresh one each time it is called, and
`rbe::views::many<F>()` builds one from bytes: start each pass from one of those.

## Range adaptors

Header: `rbe/frames/dsrl/views.hpp`, namespace `rbe::frames::views`, aliased as `rbe::views`

```cpp
--8<-- "example/docs/reference_frames.cpp:adaptors"
```

| Adaptor | Description |
|---|---|
| `rbe::views::many<F>()` | A range adaptor closure: piping `bytes` into it gives `rbe::frames::dsrl::many<F>{bytes}`. `F` is a frame view (`record::dsrl_type`), `self_delimited`. |
| `rbe::views::with_ids(ids...)` | `std::views::filter` on `frame.header().id()` being any of `ids`. Integers compare sign-safely; unknown ids match too. |
| `rbe::views::known_ids()`, `rbe::views::unknown_ids()` | `std::views::filter` on `frame.payload().known_id()`. |

They compose with every `std::views` adaptor.

## Delimiting

Headers: `rbe/frames/frame_delimiting_tags.hpp`, `rbe/frames/frame_delimiting_concepts.hpp`

Who knows where a frame ends, and what it costs to find out. Every frame declares a **tag** as its
`delimiting_concept`; the library deduces it from the header's annotations and the payload's kind, and a
[custom frame](#custom-frames) states its own.

```
(root)
├── buffer_delimited_tag              the buffer it was handed bounds it           O(1)
└── self_delimited_tag                the frame finds its own end
    ├── explicitly_delimited_tag      a header field says so
    │   ├── length_delimited_tag      a byte length                                O(1)
    │   └── count_delimited_tag       an element count
    │       └── static_stride_delimited_tag   over constant-size elements          O(1), otherwise a walk
    └── implicitly_delimited_tag      the payload's types say so: an id, an extended header   O(1)
        └── static_delimited_tag      a compile-time constant
```

| Concept | A frame whose tag is... |
|---|---|
| `rbe::frames::self_delimited<F>`, `buffer_delimited<F>` | one of the two roots, or a refinement. |
| `explicitly_delimited<F>`, `length_delimited<F>`, `count_delimited<F>`, `static_stride_delimited<F>` | the explicit branch and its leaves. |
| `implicitly_delimited<F>`, `static_delimited<F>` | the implicit branch and its leaf. |
| `rbe::frames::constant_length<F>` | The cost axis: the frame answers its extent in O(1), by carrying `length()`. Every category above does except `count_delimited` over variable-size elements. |
| `rbe::frames::static_length<F>` | `std::size_t`: the extent of a `static_delimited` frame. |
| `rbe::frames::static_stride<M>`, `dynamic_stride<M>` | For a `many` `M`: whether its element is `static_delimited`. |
| `rbe::frames::delimiting_tag<T>`, `declares_delimiting_concept<T>` | `T` is a tag; `T` has a `delimiting_concept` that is one. |
| `rbe::frames::delimiting_concept_t<F>`, `frame_delimiting_tag<H, P>` | A frame's tag; the tag the deduction gives a header and payload. |

How the deduction reads a frame, in priority order; the first row that applies wins:

| Header | Payload | Tag |
|---|---|---|
| has `payload_length` or `frame_length` | anything | `length_delimited` |
| has `payload_count` | a `many` of `static_delimited` elements | `static_stride_delimited` |
| has `payload_count` | a `many` of anything else | `count_delimited`, not `constant_length` |
| | a frame | the nested frame's category: `buffer_delimited` stays, `static_delimited` stays unless the header has `header_length`, anything else is `implicitly_delimited` |
| | an `any` | `implicitly_delimited` |
| | a `many` without a count | `buffer_delimited` |
| | a wirable or `empty` type | `static_delimited`, or `implicitly_delimited` with `header_length` |
| | a `blob` or any span-constructible view | `buffer_delimited` |

```cpp
--8<-- "example/docs/reference_frames.cpp:delimiting"
```

## Length and distance

Header: `rbe/frames/length.hpp`

| Symbol | Description |
|---|---|
| `rbe::frames::length(f)` | `f.length()`: the extent in bytes, O(1). Exists only for a `constant_length` frame. |
| `rbe::frames::distance(f)` | The same number for any frame, walking when it has to: `f.length()`, else `f.distance()`, else an ADL `frame_distance(f)`, else the header plus every element of the run. |

```cpp
--8<-- "example/docs/reference_frames.cpp:length_distance"
```

A frame opts into the O(1) axis by carrying a `length()` member; there is nothing else to declare. A frame
that cannot answer cheaply supplies `distance()` or `frame_distance` instead, or nothing, and
`rbe::frames::distance` walks.

## Custom frames

Header: `rbe/frames/dsrl/base_frame.hpp`, `rbe/frames/dsrl/payload.hpp`

A protocol that ends a message in a way no tag names, a terminator byte for instance, needs a view of its
own. `rbe::frames::dsrl::base_frame<H, P>` is the shared part: it stores the header and payload views and
derives `header()`, `payload()`, `get<I>()`, `flatten()`, `as_span()`, `payload_span()`, `header_span()`,
`buffer()` and `data()` from them, dispatching to the most derived type so they follow a custom `length()`.
It deliberately has no `make`, no `delimiting_concept` and no `length()`: the derived class supplies them.

| The derived class provides | Why |
|---|---|
| `static make(buffer) -> std::optional<Derived>` | The checked constructor `try_deserialize` and `many` call. |
| `using delimiting_concept = <tag>;` | The category, which the library cannot deduce for an extent it does not compute. `self_delimited_tag` for a frame that finds its own end. |
| `length() const -> size_type` | Optional; makes the frame `constant_length`, which a `many` requires of its element. |
| a protected constructor call `base_frame(header_view, payload_view)` | |

The building blocks `dsrl::frame` is made of are public, so a custom `make` composes them and replaces only
the step that differs:

| Function | Description |
|---|---|
| `rbe::frames::dsrl::header<H>::make(buffer)` | Step 1, the header. |
| `payload_extent<P>(header, rest)`, `try_payload_extent<P>(header, rest)` | Step 2, the payload's bytes: `rest.first(payload_length)` when the header delimits it, `count * stride` for a counted static run, `rest` otherwise. |
| `construct_payload<P>(header, bytes)`, `try_construct_payload<P>(header, bytes)` | Step 3, the payload view: an `any` from the header's id, a `many` with the header's count, a proxy or nested frame otherwise. |
| `counted_extent<P>(header)` | `payload_count() * static_length` of the element, for a counted run of constant-size elements. |
| `payload_view_t<P>` | The view a payload declared as `P` lowers to. |

A vocabulary type then names the view: `header_type`, `payload_type`, `delimiting_concept` and `dsrl_type`
are all `rbe::frames::many<line>` and `rbe::deserialize<line>` need.

```cpp
--8<-- "example/markets/cboe.hpp:line"
```

The whole example, a line-terminated ASCII feed, is taken apart in
[Write a custom frame](../examples/howto/custom-frame.md). The concepts a view has to satisfy are in
`rbe/frames/dsrl/concepts.hpp`: `rbe::frames::dsrl::is_frame<T>` (the members above plus `header()`,
`payload()`, `data()` and construction from a buffer) and `decomposable_frame<T>` (`get<0>` and `get<1>`).

## Planned

🚧 Not implemented yet, tracked in the [feature matrix](features.md#frames):

- `F::value_type`, a struct generated from the frame's parameters, and `rbe::serialize` of one;
- `F::srl_type`, a writer that appends header and payload into a buffer;
- an eager whole-frame decode. Today a frame is read through its view, and `any::as<T>(rbe::dsrl::eager)`
  or a by-value `match` callback decode one payload.

Writing a frame today is [serializing its parts in order](../examples/howto/serialize.md#several-values-back-to-back).
