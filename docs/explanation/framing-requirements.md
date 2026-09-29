# Framing Requirements

Normative requirements for the three framing views: `any`, `frame` and `many`.

This document continues the requirement numbering of [requirements.md](requirements.md). It states *what*
the framing types guarantee, never how they achieve it: rationale, cost analysis and the alternatives that
were rejected live in [framing-design-notes.md](framing-design-notes.md).

Requirements are worded so that a violation is observable by a user of the library. Where a requirement
names a precondition, violating it is undefined behaviour unless the requirement says otherwise.

Requirement ids are append-only: a new requirement takes the next free id and is placed in the section it
belongs to, so ids need not read in order down the document. Existing ids are never renumbered or reused.

## Common Requirements

These apply to `any`, `frame` and `many` alike.

- **REQ-080**: A framing view must be a non-owning view over a buffer of bytes. It must never own, copy or
  allocate the bytes it describes, and the buffer must outlive the view
- **REQ-081**: Copying a framing view must cost the same regardless of how large the viewed buffer is and of
  how many elements, candidates or fields it describes
- **REQ-082**: Every framing operation must be usable in constant evaluation whenever the data it reads is
  itself a constant expression
- **REQ-083**: Resolving the length of a framing view must never require decoding payload field values
- **REQ-084**: Every operation that can fail because of the data it reads must offer a checked form that
  reports the failure as `std::optional`. Constructors must not report data failures: they carry narrow
  contracts and must never throw
- **REQ-085**: The preconditions of a narrow contract must be verifiable in hardened builds and assumed in
  non-hardened ones (see REQ-042 to REQ-046)
- **REQ-086**: A buffer larger than the entity a view describes must be accepted. Everything the view reports
  must refer to the entity, never to the buffer it was constructed over

## `any` Requirements

### Definition and candidates

- **REQ-087**: An `any` must be a type-erased view over a buffer, interpretable as exactly one of the N
  candidate types given as template arguments, selected at runtime by an identifier
- **REQ-088**: A candidate list must declare at least two candidates, every candidate must declare an id,
  all ids must share one type, and no id may be declared twice. Violating any of these must be a compile
  error that names the offending candidate
- **REQ-089**: Candidates may be of fixed or variable size, and may be explicitly empty. Every candidate must
  be deserializable by rbe

### Construction

- **REQ-090**: An `any` must be constructible from an identifier and a buffer
- **REQ-091**: An `any` must be constructible from a buffer and a candidate named statically as a template
  parameter, without supplying its identifier
- **REQ-092**: An `any` must be constructible from an identifier that no candidate declares. An unknown
  identifier is a legitimate runtime value, not an error
- **REQ-093**: The buffer must be at least as large as the stored candidate. A larger buffer must be accepted
  and the bytes beyond the candidate must not be part of the `any`
- **REQ-094**: `make(id, buffer)` must be the checked form of construction: it must yield an `any` when the
  candidate fits in the buffer, and `nullopt` when it does not
- **REQ-095**: `parse_length(id, buffer)` must report the length of the candidate the identifier selects
  without constructing the `any`, reading only the bytes the length depends on, so that it is usable over a
  partially received buffer. It must report `nullopt` when the buffer is too short to determine the length

### Unknown identifiers

- **REQ-096**: An `any` holding an unknown identifier has no candidate length, and must therefore extend to
  the end of the buffer it was constructed over
- **REQ-097**: An `any` holding an unknown identifier must remain fully usable: it must report its
  identifier, report that the identifier is unknown, expose its bytes, and dispatch to the sink overload

### Dispatch

- **REQ-098**: `match(overload_set)` must dispatch to the overload matching the stored candidate, passing it
  the deserialized candidate
- **REQ-099**: `match` must be exhaustive. An overload set that does not handle every candidate must provide
  a sink overload constrained by `rbe::unmatched`; an overload set without one must be a compile error
- **REQ-100**: An overload set must be able to distinguish an identifier that is known but unhandled from one
  that no candidate declares, by providing an additional overload for the latter
- **REQ-101**: All overloads reachable for a given `any` must share one return type
- **REQ-102**: Eager and lazy overloads must be mixable within one overload set, but the mapping from
  candidate to overload must stay unambiguous. An ambiguous overload set must be a compile error

### Accessors

- **REQ-103**: `is<T>()` must report whether the stored identifier selects `T`. Naming a type that is not a
  candidate must be a compile error, not a `false` result
- **REQ-104**: `as<T>(strategy)` must yield the deserialized candidate when `is<T>()` holds and `nullopt`
  otherwise, honouring the requested deserialization strategy
- **REQ-105**: `length()` must report the length of the stored candidate. It must be semantically equivalent
  to `match(get_length)`, including for an unknown identifier, where the sink reports the remaining buffer
- **REQ-106**: `id()` must report the stored identifier as given at construction, known or not
- **REQ-107**: `known_id()` must report whether the stored identifier belongs to the candidate list
- **REQ-108**: `as_span()` must yield the bytes of the stored candidate, delimited by `length()`

## `frame` Requirements

### Definition and preconditions

- **REQ-109**: A frame must be a non-owning view over a buffer, composed of a header and a payload
- **REQ-110**: The buffer must be at least as large as the frame
- **REQ-111**: The lengths a frame reads off the wire must be consistent with each other:
  `wire_size_of<header>() <= header_length <= frame_length`, and, when the header declares both,
  `frame_length == header_length + payload_length`
- **REQ-112**: When the header declares both a payload length and a frame length, the two must agree
- **REQ-113**: `make(buffer)` must be the checked form of construction: it must yield a frame when the frame
  fits in the buffer, and `nullopt` when it does not
- **REQ-114**: `parse_length(buffer)` must report the length of the frame without constructing it, reading
  only the bytes the length depends on, so that it is usable over a partially received buffer. It must
  report `nullopt` when the buffer is too short to determine the length

### Composition

- **REQ-115**: Frames must be recursively composable: a payload may itself be a frame, to any depth
- **REQ-116**: The header must be of fixed size. It may declare its own length on the wire, which may exceed
  its static size so that fields can be added without breaking compatibility; the payload must then begin at
  the declared header length
- **REQ-117**: A payload must be able to be a wirable type of fixed or variable size, an explicitly empty
  type, a type constructible from a span of bytes, an `any`, a `many`, or another frame
- **REQ-118**: `flatten()` must decompose a frame into its constituent headers and payload

### Delimitation

- **REQ-119**: Every frame must be classified at compile time as either self-delimiting or buffer-delimited.
  A self-delimiting frame knows its length from the bytes it holds; a buffer-delimited frame is exactly as
  long as the buffer it was constructed over
- **REQ-120**: A self-delimiting frame must be classified as either explicitly or implicitly delimited. It is
  explicitly delimited when the header declares a payload length or a frame length, and implicitly delimited
  when its length follows from the header length plus the payload length
- **REQ-121**: Explicit delimitation must take precedence over implicit delimitation
- **REQ-122**: An implicitly delimited frame has no length of its own: it is exactly its header plus its
  payload. It is therefore self-delimiting only if its payload can report its own length, and a payload that
  cannot — a blob, a `many`, or a nested frame that is itself buffer-delimited — makes the enclosing frame
  buffer-delimited. Asking a nested frame for its length is the same question one level down, so the
  classification of a frame may depend on a frame several levels below it, and stops depending on it at the
  first header that declares a length field. A frame whose own header declares one is explicitly delimited
  whatever its payload turns out to be (REQ-121)
- **REQ-123**: A buffer-delimited payload nested in a self-delimiting frame must be delimited by the
  enclosing frame: the payload is exactly the bytes the enclosing frame assigns to it, which is what lets a
  frame that cannot delimit itself be carried inside one that can
- **REQ-124**: A frame whose payload is an `any` must be delimited by the identifier only when its header
  declares no length field. Such a frame is dispatch-delimited
- **REQ-125**: A dispatch-delimited frame carrying an unknown identifier must extend to the end of the
  buffer. A frame that is self-delimiting by classification therefore degrades to buffer-delimited at
  runtime for that frame only (see REQ-096)

### Access

- **REQ-126**: The header must be deserializable both lazily and eagerly, at the user's choice
- **REQ-127**: The payload must be deserializable both lazily and eagerly, at the user's choice. A payload
  that is not a wirable type is currently restricted to lazy deserialization
- **REQ-128**: The user must be able to obtain the bytes of the header, of the payload, and of the whole
  frame, as `header_span()`, `payload_span()` and `as_span()`
- **REQ-129**: The user must be able to obtain the length of the frame, of its header and of its payload, as
  `length()`, `header_length()` and `payload_length()`

## `many` Requirements

### Definition

- **REQ-130**: A `many` must be a non-owning view over a buffer holding a sequence of frames of one type
- **REQ-131**: The element type must be self-delimiting. Instantiating a `many` over a buffer-delimited
  element type must be a compile error, since the first element would consume the whole buffer
- **REQ-132**: A `many` must be an input range of its element type, usable with range adaptors and with a
  classic loop alike. Iteration is single-pass
- **REQ-151**: Constructing a `many` must not be able to fail, so it must not offer the checked form of
  REQ-084: its extent is the buffer it is given, and malformed content is reported while iterating
  (REQ-134 to REQ-136) rather than at construction. Its constructor is the only wide one of the three views
  (REQ-143), and a `make()` that can never report failure would oblige every caller to handle a case that
  cannot arise

### Iteration and truncation

- **REQ-133**: A `many` must accept truncated buffers: the last element it holds may be incomplete
- **REQ-134**: Iteration must stop at the first element the buffer does not hold whole
- **REQ-135**: Iteration must stop at an element that is dispatch-delimited and carries an unknown
  identifier, because that element extends to the end of the buffer and no further element can be delimited
  after it (see REQ-125)
- **REQ-136**: Iteration must make progress: every step must advance by at least one byte. An element that
  reports a length of zero is malformed and must terminate the iteration rather than be iterated over again
- **REQ-137**: The user must be able to access the element the iteration currently sits on
- **REQ-138**: The user must be able to access the bytes the iteration has not consumed, at any point and
  not only once it has stopped. The element the iteration currently sits on counts as unconsumed, so while
  iterating these are the bytes from the current element to the end of the buffer, and once iteration has
  stopped they are the remainder a reassembler carries over to the next read
- **REQ-139**: The user must be able to tell apart the three ways an iteration can end: the buffer was
  consumed exactly, it stopped on an element that is incomplete, or it stopped on one that is malformed. An
  empty remainder distinguishes the first, and the other two must be distinguishable from each other, since
  a reassembler waits for more bytes in one case and must drop the stream in the other

### Composition

- **REQ-140**: A `many` must be usable as a frame payload. Such a frame is buffer-delimited unless its
  header declares a length field, in which case the `many` spans exactly the payload the enclosing frame
  assigns to it (REQ-121, REQ-123)

## Construction From a Known Extent

A framing view constructed over a buffer that is larger than itself has to find its own end (REQ-086). A
caller that already knows where it ends would be paying for that resolution twice, and the enclosing frame
is such a caller: an explicitly delimited frame knows the exact extent of its payload before constructing
it, whether that payload is an `any`, a `many` or another frame.

- **REQ-141**: A framing view must offer a form of construction for a caller that already knows the exact
  extent of what is being constructed, which assumes that extent instead of resolving it. Its precondition
  is stronger than REQ-093 and REQ-110: the buffer must be exactly the entity, not merely large enough, so
  that `buffer.size() == *parse_length(buffer)`
- **REQ-142**: Composing framing views must not resolve the same length twice. Constructing the payload of a
  frame that already delimits it must not re-derive that payload's length
- **REQ-143**: The shape of a call must tell which class of contract it carries: constructors are narrow and
  `make()` is the wide path that reports failure as `std::optional` (REQ-084). The form of REQ-141 is
  therefore a constructor, and it must be marked at every call site by a trailing `rbe::exact_length` tag,
  never as an unmarked overload and never by defaulting the tag, since two narrow contracts of different
  strength would otherwise be spelled identically. The tag names the precondition it asserts, not the work
  it skips
- **REQ-144**: Assuming an extent must not weaken what the resulting view reports. A view built this way
  must be indistinguishable from one built by resolution whenever its precondition holds

## Range Integration

- **REQ-145**: `many` must model `std::ranges::view`, so that it composes with range adaptors through `|`.
  It is a range factory over a buffer rather than an adaptor over another range
- **REQ-146**: Modelling a view must not weaken REQ-132: `many` remains an input range and stays single-pass

## Trust Model

The wire is the authority. rbe decodes the protocol it is told to decode; it does not audit it.

- **REQ-147**: Lengths read off the wire must take precedence over lengths implied by the types. When a
  header declares a length that disagrees with the static size of the type it describes, the declared length
  is what the frame uses. This is what makes a header able to grow without breaking its readers (REQ-116)
- **REQ-148**: rbe must not validate that the wire is well formed. The consistency stated in REQ-111 and
  REQ-112 is a precondition the protocol must satisfy, not a condition rbe reports on: a message whose type
  implies 50 bytes and whose header declares 49 is a corrupt protocol, and the resulting behaviour is
  undefined
- **REQ-149**: The precedence in REQ-147 must never be exercised silently as a fallback. A frame must not
  choose between two disagreeing lengths at runtime; which length governs is fixed by the frame's
  classification (REQ-119 to REQ-125) and is therefore known before any byte is read
- **REQ-150**: A hardened build may check the preconditions of REQ-111 and REQ-112 and report a violation
  (REQ-085). Such a check must never be required for correctness in a non-hardened build
