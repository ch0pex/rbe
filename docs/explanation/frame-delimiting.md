# RBE Feature Proposal: Frame Delimiting and the Extent Customization Point

### Summary

Give the frame-delimiting model one vocabulary and one extension seam.

Today rbe answers *how does a frame find its end?* structurally, with `payload_extent_of<H, P>`, and
answers *what does it cost to know its length?* implicitly — `length()` exists on every frame and
means something different per category. The two questions are independent, and conflating them is
what makes a counted frame unrepresentable, a custom frame classify by accident, and `many<T>`
constrained on the wrong property.

The proposal has three parts:

1. name the two axes — **mechanism** (structural, compile time) and **cost** (how `length()` is
   obtained);
2. turn step 2 of frame construction (*where does the payload end?*) into a real customization point
   with a stated contract, so a terminator-delimited, composite-discriminant or body-length protocol
   swaps one step instead of rewriting `make()`;
3. decide what `length()` means for each category, and what the library checks versus assumes.

Related: [Framing Design Notes](framing-design-notes.md) §3 (length resolution), §4 (self-delimiting
and buffer-delimited frames), §5 (`any_id`), §6 (unknown ids) and §11 (custom frames), which this
document supersedes on the points listed in the Decision Log.

### Motivation

`rbe::frame<H, P>` covers the protocols whose end is in the header or in the type. Three shapes in the
repo today are outside that premise, and each one is already paying for it:

- **Terminator-delimited** (cboe TOP): the end is found by scanning for a sentinel. Supported only by
  writing a whole frame view by hand — `example/markets/cboe.hpp:975` and a near-identical copy in
  `tests/runtime/test_dsrl_frame.cpp:294` both re-implement steps 1 and 3 in order to change step 2.
- **Counted** (`frame<HeaderWithCount, many<U>>`): the header says how many elements follow. The frame
  *does* know where it ends, yet it classifies as `buffer_delimited` and its `length()` returns the
  whole buffer — `tests/runtime/test_dsrl_frame.cpp:461` carries the open TODO and navigates with
  `payload.remainder()` precisely because `length()` cannot be used.
- **Composite / in-body discriminants** (OPRA): the extent follows from two header fields, or from the
  first two bytes of the payload.

The underlying claim, against the structural premise of notes §3/§4: *a frame's delimiting is not
always a function of `header_type` and `payload_type`.* Once that is accepted, the classification
needs an input it does not have today, and that input is the customization point.

#### Inconsistencies in the code today

| # | Where | What |
| --- | --- | --- |
| 1 | `dsrl/frame.hpp:77-87`, `dsrl/concepts.hpp:62` | `length()` is required of **every** frame but means two different things: the frame's true length, or `buffer().size()` — an echo of the caller's span. `as_span()` / `payload_span()` / `flatten()` inherit whichever applies, silently. |
| 2 | `dsrl/many.hpp:31` | `many<T>` is constrained on `self_delimiting_frame<T>` (mechanism) when what `next()` needs is a length that is O(1) and non-zero (cost). They coincide today only because the one shape where they differ — counted — is misclassified. |
| 3 | `tests/runtime/test_dsrl_frame.cpp:461-465` | A counted frame is `buffer_delimited`, so `length()` over-reports, `as_span()` / `payload_span()` are unusable, and it cannot nest in `many`. Documented as an open question in the test itself. |
| 4 | `detail/payload_extent.hpp:65-73`, `tests/runtime/test_dsrl_frame.cpp:346` | The structural classification cannot see a custom rule. `self_delimiting_frame<line_view>` is `true` **by accident** (its payload is an `any`, i.e. `any_id`); the same frame over a `blob` would be rejected by `many`. The `self_delimiting` opt-in is *Decided* in notes §11 and not implemented. |
| 5 | `detail/payload_extent.hpp:65-73` | `is_self_delimiting()` is `extent != buffer_end`, so any new enumerator is silently self-delimiting (notes §4, *Proposed*). |
| 6 | `detail/payload_extent.hpp:44-61` | The classification tests `wirable<PayloadType>` before `is_any`, and never tests `is_many`: a `many` payload reaches `buffer_end` only because a type with base classes is excluded from `wirable` (680d76f). Load-bearing accident. |
| 7 | `dsrl/payload.hpp:83-105` | Step 2 (`payload_extent<P>(hdr, rest)`) ignores its `P` parameter and branches only on `header<H>::delimits_payload`. It is "the extent the header declares, or everything" — not the extent, and no seam. |
| 8 | `detail/payload_extent.hpp:34` vs `dsrl/payload.hpp:84` | Two entities named `payload_extent`: the enum (*where the length comes from*) and the function (*the bytes*). |
| 9 | `dsrl/many.hpp:111` | `has_seen_partial()` is `done() and not remainder().empty()`. After 5c59426 `done()` has three causes (count reached / clean end / parse failure) and one bool to report them, so "count satisfied, next frame's bytes follow" is reported as partial — the very idiom the new test uses. |
| 10 | `header::make`, `frame::make`, `any::make`, `many::current_` | `std::optional` cannot tell *incomplete* ("wait for more bytes") from *malformed* ("resync"). In a line protocol the first is routine and the second is an error; `line_view::make` documents only the first while returning both. |
| 11 | `dsrl/many.hpp:95-100`, `core/detail/split_span.hpp:58` | Nothing enforces `length() > 0`: a rule returning `0` makes `next()` not advance and iterate forever (`advance(0)` passes its assert). Nothing bounds a user-provided extent either, so `rest.first(n)` with `n > rest.size()` is UB. |
| 12 | `example/markets/cboe.hpp:1003`, `tests/runtime/test_dsrl_frame.cpp:325` | The LF scan absorbs the run of LFs after the terminator, so the extent of a prefix changes when more bytes arrive (`"A\n"` → 2, `"A\n\n"` → 3): TCP segmentation changes the parse. The law it breaks is written nowhere. |
| 13 | `framing/many.hpp:25` | `rbe::many<T>` (vocabulary) carries no delimiting constraint, so the error surfaces inside `dsrl::many` on use, not where the user wrote the type — against the stated goal "a malformed frame is rejected where it is written". |
| 14 | `dsrl/many.hpp:117-119` | `many::as_span()` / `data()` mean *the remainder*, while `as_span()` means *exactly this object* on `frame`, `proxy` and `any`. `many` also has no length of its own, which is what blocks the counted case from being measurable. |
| 15 | `example/markets/cboe.hpp:996`, `tests/runtime/test_dsrl_frame.cpp:314` | `explicit line_view(buffer_type) : line_view(*make(buf))` dereferences an empty optional with no assert, and is not narrower than `make()` (the scan already happened) — unlike `frame::resolve`, which is a genuine narrow path. It is the pattern users will copy. |

Two invariants the design below leans on, confirmed in the code and written down nowhere:

- `base_frame::payload()` returns by value, so a stored `many` hands out a fresh cursor on every
  call. Payload re-traversal — what an O(n) `distance()` would need — is therefore already sound.
- `header<H>::delimits_payload` and `explicitly_delimited_frame<F>` are equivalent *given* that the
  `buffer_delimited` case is handled first (a length field always wins in `payload_extent_of`), which
  is why the swap at `dsrl/frame.hpp:81` is behaviour-preserving.

### Design

#### The two axes

**Axis A — mechanism** (compile time; structural, or declared where structure cannot say). *How is the
end found?*

| Category | How | Example |
| --- | --- | --- |
| `buffer_delimited` | nothing says; the caller's span **is** the answer | `frame<TcpHeader, blob>` |
| `explicitly_delimited` | a dedicated wire field (`frame_length` / `payload_length`) | aquis, SBE |
| `implicitly_delimited` (static) | every field sized by the type | `frame<WithHeaderLength, Message>` |
| `implicitly_delimited` (dynamic) | a type-fixed walk over fields whose sizes are runtime values | variable-size payload (notes §8) |
| `dispatch_delimited` | the id implies the alternative's size | nasdaq, opra, cboe pitch |
| **`counted`** *(new)* | a count field plus the elements' own lengths | `frame<HeaderWithCount, many<U>>` |
| **`custom`** *(new)* | a rule the user supplies: terminator, composite discriminant, length in the body | cboe TOP, opra `Administrative` |

**Axis B — cost.** *Given a constructed frame, what does `length()` cost?* The criterion is **not**
"calculable from the type in a number of reads fixed by the type" — that wording excludes
`line_view`, which stores a scanned length and is perfectly O(1) afterwards. The criterion is:

> **the constructed frame knows its length without traversing anything further.**

That admits two ways of getting there — *eager* (pay the scan in `make()` and store the result: cboe
TOP) and *cheap* (read a field, add static sizes) — and excludes exactly one shape: a counted payload
of variable-size elements, whose length is only knowable by walking the elements.

| Mechanism | `length()` O(1)? | Walk needed? |
| --- | --- | --- |
| explicit / static / dynamic / dispatch | yes | no |
| custom, extent settled in `make()` | yes | the scan, once, in `make()` |
| counted, elements of static stride | yes (`count × wire_size_of<U>`) | no |
| counted, elements variable-size | **no** | O(n) over the elements |
| buffer_delimited | n/a — the span is the answer | no |

The scan in the eager case is unavoidable, not hidden cost: the payload is handed an already trimmed
span, so somebody has to find the boundary first. The total cost stays linear in the bytes consumed,
so there is no per-element O(n) smuggled into iteration.

#### Bifurcation 1 — where the extent customization point lives

| | Option | How a cboe TOP line is written | Pros | Cons |
| --- | --- | --- | --- | --- |
| **1a** | **Extent policy as a third template parameter**: `dsrl::frame<H, P, Extent = header_extent>`; the policy computes the extent and declares its own `self_delimiting` | `using dsrl_type = dsrl::frame<Header, messages::dsrl_type, lf_terminated>;` — `line_view` disappears | one seam, explicit at the type; steps 1 and 3 are never duplicated; rbe can ship `terminated_by<B>`, `counted_extent`, `body_length<Off, N>`; the policy can carry state and declare its own axis-A answer, which closes fork 6 for free; `many`'s constraint becomes checkable | one more template parameter on the most visible type in the library; a rule needing more than `(hdr, rest)` — the previous frame, a session state — still does not fit |
| **1b** | **CPO**, in the style of `detail::payload_size`: a member `static extent(hdr, rest)` or an ADL `frame_extent(...)` on the vocabulary type | add a `static extent(...)` to `struct line` | `dsrl::frame`'s signature is untouched; the opt-in is invisible when unused | dispatch is implicit and hard to diagnose; no state; the classification would have to find the vocabulary type from the lowered type, which it cannot do today |
| **1c** | **Status quo (notes §11)**: a hand-written view on `base_frame`, plus a `static constexpr bool self_delimiting` opt-in | today's `line_view`, unchanged, plus the opt-in | smallest diff; already implemented and tested; a frame that abandons the header-plus-payload shape needs this path anyway | every custom frame copies steps 1 and 3 — already duplicated twice in the repo — and hand-writes the stored length; nothing enforces the contract |
| **1d** | **Richer payload contract** (evaluated and shelved in notes §11): every payload view gets `make(hdr, bytes)`, `{bytes, exact_length}` and `length()` | the payload, not the frame, finds the line end | removes the `is_any` / `is_many` branches from step 3; fixes the `size()` / `length()` asymmetry | puts a contract on *every* payload to serve cases a frame-level seam covers; and the LF is framing, not payload |
| **1e** | **Declarative annotation** on the header type, e.g. `[[=rbe::terminated_by('\n')]]` | `struct [[=rbe::pack_le, =rbe::terminated_by('\n')]] Header` | the classification stays purely structural — no opt-in, no policy, nothing to keep in sync; reads like the rest of rbe | only covers rules expressible as an annotation value (a terminator, a stride); OPRA's composite discriminant and in-body lengths still need 1a/1b/1c |

These are not all mutually exclusive: **1e composes with any of the others** (the annotation lowers to
a library-provided policy), and **1c survives under 1a** for frames that abandon the shape.
Recommendation, if a single answer is wanted: **1a**, with **1e** layered on later for the terminator
case, and 1c kept as the escape hatch.

#### Bifurcation 2 — what `length()` means, and how axis B is expressed

| | Option | Consequences |
| --- | --- | --- |
| **2a** | **Settle the extent in `make()` and store it.** `length()` becomes `header().length() + payload_extent_`, uniformly. | `make()` already computes the extent and throws it away. Storing it makes `length()` O(1) and single-meaning for every category, deletes the three-branch body at `dsrl/frame.hpp:77`, and reduces `detail::payload_size` to one call inside `make()` instead of one per `length()`. Matches the principle already adopted in notes §8 (*layout eager, values lazy*) and the standing preference for computing in the constructor over caching behind `const`. Costs `sizeof(size_t)` per frame; a buffer-delimited frame stores `rest.size()`, i.e. today's answer, now by an explicit rule. Does **not** contradict notes §5 (which rejected a cached `any` index), since an extent has no unresolved state. |
| **2b** | **`measurable_frame` + `distance()`** — the two-concept model. `length()` exists iff it is O(1); buffer-delimited frames lose it; `rbe::distance(frame)` walks when needed; `many<T>` requires `measurable_frame<T>`. | Most faithful to the two axes, and the only option that lets a counted-variable frame exist *without* an eager scan. Touches `dsrl::is_frame` (drops `length()`), `as_span()` (becomes `buffer()` for buffer-delimited), `views::many_fn`, and the static asserts in `tests/static/test_framing.cpp` and `example/markets.cpp`. Needs payload re-traversal, which is already sound. |
| **2c** | **Keep today's semantics, add the concept for the constraint only**: `length()` stays on every frame, `measurable_frame` exists purely so that `many` and `views::many` constrain on the right axis. | Minimal diff; fixes #2 and the constraint half of #3; leaves #1 (`length()` with two meanings) standing. |
| **2d** | **`length()` returns `optional` / `expected`** for the categories that cannot answer. | Honest, and viral: every caller of `as_span()`, `payload_span()`, `flatten()` and `many::next()` grows a failure path. Against the precondition model of notes §6. |
| **2e** | **2a with conditional storage**: `[[no_unique_address]] Extent::state`, empty for the default policy and `size_type` for a scanning one. | `sizeof(frame)` unchanged for every protocol that uses the default; more machinery, and `length()` stops being one expression. |

2a and 2b are the real fork: *is the extent always resolved eagerly, and therefore always O(1), or is
"non-measurable" a first-class state the library supports?* The answer also decides fork 3.

#### Bifurcation 3 — the counted case

| | Option | `length()` | `many<counted_frame>` |
| --- | --- | --- | --- |
| **3a** | **Today**: the count only limits iteration (5c59426); the frame stays buffer-delimited | the buffer size | no; navigate with `payload.remainder()` |
| **3b** | Self-delimiting **when the stride is static**: `hdr.length() + count × wire_size_of<U>` | O(1) | yes |
| **3c** | Self-delimiting via an **eager walk** at construction (a `counted_extent` policy, opt-in) | O(1) after an O(n) `make()` | yes |
| **3d** | Self-delimiting but **non-measurable**: only `distance()` can answer | — | no (requires 2b) |

3b and 3c compose: take the arithmetic when `U` has a static stride and the walk otherwise, both
behind one `counted_extent`. 3a is a legitimate default — the user pays no scan they did not ask for —
as long as it is *documented as a choice* rather than left as the only option.

Sub-fork: **who consumes `payload_count`?** Today the frame reads it in step 3 and injects it into
`many`'s constructor (`dsrl/payload.hpp:127,148`). Under 3b/3c the count is also extent information,
so either (i) the extent rule reads it and `many` keeps being told as well, or (ii) the count becomes
purely extent information and `many` simply stops at the extent it was handed. (ii) is cleaner but
changes the behaviour the new test pins down.

#### Bifurcation 4 — the result type of step 2

| | Option | Notes |
| --- | --- | --- |
| **4a** | `std::optional<size_type>` (today) | cannot say *incomplete* versus *malformed* (#10) |
| **4b** | `std::expected<size_type, extent_error>`, `enum class extent_error { incomplete, malformed }` | what a line protocol wants; the question is how far it propagates — step 2 only, or also `frame::make`, `header::make`, `any::make` and `many`'s stop reason (fork 5). Must stay exception-free (REQ-040/041) |
| **4c** | Sentinel sizes (`npos` for incomplete, `0` for malformed) | consistent with the `candidate_index::none` precedent of notes §6; conflicts with a legitimate extent of `0` (an `rbe::empty` payload) |
| **4d** | `optional` plus a queryable reason on the frame / on `many` | keeps today's signatures; two sources of truth |

#### Bifurcation 5 — `many`'s stop reason

`done()` has three causes and `has_seen_partial()` one bit (#9).

- **5a** keep it and document the count caveat (status quo bug).
- **5b** an enum — `consumed` / `count_reached` / `incomplete` / `malformed` — with
  `has_seen_partial()` kept as `stop() == incomplete`.
- **5c** two predicates: `pending()` (the count is not satisfied) and `truncated()` (bytes are left
  that do not parse), with `has_seen_partial()` as either of them.

Either of 5b/5c also needs **bytes consumed** (`consumed()`, or a `length()` on `many`, which #14
argues for anyway): that is what a `counted_extent` rule (3c) calls, and what would make `many` a
measurable payload.

#### Bifurcation 6 — how the classification learns about a custom rule

- **6a** a `static constexpr bool self_delimiting` on the vocabulary type (notes §11, *Decided*).
- **6b** the extent policy declares it — free under 1a.
- **6c** infer it from the view's API. Brittle: `line_view` already looks self-delimiting by accident.
- **6d** a `payload_extent::custom` enumerator, so the walk in `is_self_delimiting()` has a branch to
  land on.

Independently of the fork: make `is_self_delimiting()` **enumerate every case** instead of
`extent != buffer_end` (#5, notes §4), and reorder `payload_extent_of` to test `is_any` / `is_many`
before `wirable` (#6).

#### Bifurcation 7 — who validates the extent

- **7a** the library, at the call site: report `n > rest.size()`, reject a zero-length frame, refuse a
  non-advancing `many::next()`.
- **7b** the rule's narrow contract, asserted in hardened builds only.
- **7c** both: the wide path (`make`) reports and the narrow path (the constructor) asserts — the
  split the rest of the library already uses.

7c is the house style. 7a's `many::next()` guard is needed either way, since a non-advancing step is
an infinite loop rather than a wrong answer.

#### The contract for an extent rule

Whatever fork 1 resolves to, the rule is a function of `(header view, bytes after the header)`, and
the compiler cannot check any of this. Hence the laws:

- **L1 — Purity.** The result depends only on the header's bytes and on `rest`'s bytes. Two calls over
  equal bytes give equal results. Required because `make()` and the narrow constructor must agree, and
  because a re-traversal (`distance()`, a second `payload()`) must reach the same boundary.
- **L2 — Prefix stability.** If the rule yields `n` for `rest`, it yields `n` for every buffer that
  *extends* `rest`; and if it yields *incomplete* for `rest`, it yields *incomplete* for every prefix
  of `rest`. Appending bytes never moves a boundary that was already found, and truncating never
  completes one. This is the law #12 breaks, and the reason the LF-run scan is a bug rather than a
  design choice: the answer must not depend on how TCP split the stream.
- **L3 — Range.** `0 <= n <= rest.size()`, and `header.length() + n > 0`. Zero is a valid *payload*
  extent (an `rbe::empty` payload) but never a valid *frame* length — a zero-length frame is what
  makes `many` loop forever.
- **L4 — Three states, with meaning.** *incomplete* = these bytes are a prefix of a frame, wait for
  more; *malformed* = no continuation can make this a frame, resync; otherwise, the extent.
  *incomplete* must not be returned once `rest` already covers the bound the rule itself declares.
- **L5 — The extent is the authority.** The frame's length is `header.length() + n`. The payload view
  built over those bytes may narrow *itself* within `n` — a proxy to its wire size, an `any` to its
  candidate — so `payload_span()` is the full `n` while `payload().as_span()` may be shorter. The LF
  padding of a TOP line lives in exactly that gap.
- **L6 — Independence from the payload** (recommended, not required). A rule that ignores the id is
  what lets iteration continue past an id this build does not know; a rule that depends on the id
  inherits `dispatch_delimited`'s failure mode (notes §6).
- **L7 — The declared axis A must match the rule.** A rule that consults `rest.size()` to decide the
  boundary is not self-delimiting, whatever it declares. Unverifiable, and the one that silently
  breaks `many`.

What the library does enforce (fork 7): the range check at the call site, the non-advancing guard in
`many::next()`, and the narrow-path asserts in hardened builds.

### Alternatives Considered

- **Make `payload_extent_of` recognise custom frames structurally.** It cannot: the delimiting of a
  custom frame is not in its types. That is what forces fork 6 to exist at all.
- **Derive a frame from `dsrl::frame` and shadow `make` / `length`.** Forgetting `length()` iterates
  wrongly in silence; `base_frame` without primitives makes it a compile error (notes §11).
- **A `protected resolve()` on `base_frame`.** It only customizes `length()` in practice; replacing a
  step still means copying `frame`'s private code.
- **Caching the resolved extent lazily behind `const`** (a `mutable` member). Rejected on the same
  grounds as the `any` index in notes §5 and the lazy offset table in §8: compute it in the
  constructor.
- **Deframing inside `frame`** — byte-stuffed protocols (SLIP, COBS, HDLC). Still out of scope: their
  boundary comes from a scanner over the stream, not from a frame's own bytes, and they fit as
  "outermost frame over what the deframer returned". Note that an extent rule does bring the plain
  *terminator* case (no escaping, no stuffing) inside `frame`, which is a narrower claim than general
  deframing.
- **`rbe::any_frame<H, Msgs...>`** for the composite-discriminant case. The id is just one header
  field; a rule that needs another one reads it off `header()`.

### Open Questions

1. Fork 1: which seam, and does `1e` (a `terminated_by` annotation) come now or later?
2. Fork 2: eager-and-stored (`2a`) or `measurable_frame` + `distance()` (`2b`)? This is the one
   decision the rest hang off.
3. Fork 3: is a counted frame self-delimiting by default (3b/3c) or only on opt-in (3a)? And does
   `many` keep being handed the count, or stop at the extent it was given?
4. Fork 4: does `expected<size_type, extent_error>` stay inside step 2, or does the
   incomplete/malformed distinction propagate to `frame::make`, `header::make`, `any::make` and the
   deserialize entry points? It is the same hole as `has_seen_partial()`.
5. Does `many` gain a length of its own (`consumed()` / `length()`)? Required by 3c, and it would make
   `many::as_span()` mean what it means everywhere else (#14).
6. Naming: the enum `payload_extent` versus the function `payload_extent` (#8). Candidates:
   `extent_source` / `delimiting_kind` for the enum, `payload_bounds` / `extent_of` for the function.
7. Does the vocabulary `rbe::many<T>` get the delimiting constraint, so that the diagnostic lands
   where the user wrote the type (#13)?
8. Do the delimiting concepts keep applying to lowered types, now that a policy (1a) or an opt-in (1c)
   lives at one level only? The "lowering never changes delimiting" invariant
   (`same_extent_when_lowered`) has to be restated.
9. Length units — words, element counts — still unresolved (notes §3). A counted extent is the first
   case where the unit is not bytes.

### Decision Log

- Already **implemented**: `payload_extent_of<H, P>` as the single compile-time classification;
  `self_delimiting` / `buffer_delimited` / `explicitly_delimited` / `implicitly_delimited` /
  `dispatch_delimited`; public building blocks for steps 1–3; `base_frame` with no primitives;
  `dsrl::many` constrained on `self_delimiting_frame`; `payload_count` limiting `many`'s iteration
  (5c59426); `many::remainder()` as the forward-navigation idiom for buffer-delimited frames.
- Already **decided, not implemented** (notes §4, §11): `many` requiring self-delimiting elements at
  the vocabulary level too; an explicit per-enumerator `is_self_delimiting()`; the `self_delimiting`
  opt-in; rewriting `cboe::top::line` as a vocabulary serder.
- **This proposal adds**: the two axes as named vocabulary; `counted` and `custom` as first-class
  mechanisms; the extent rule as a customization point with laws L1–L7; the incomplete/malformed
  distinction; library-side extent validation.
- **Confirmed by the code, not by the docs**: `payload()` returning by value already makes payload
  re-traversal sound, which is what an O(n) `distance()` needs.
