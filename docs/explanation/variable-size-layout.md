# Variable-Size Layout Notes

> **Status: working notes, not implemented.** Design of wirable types whose wire size is only known at runtime
> (e.g. a `std::vector<T>` sized by another member), and of the runtime offset table that lets `dsrl::proxy`
> access them. Meant as source material for the future documentation. Not linked from site navigation — same
> convention as [Framing Design Notes](framing-design-notes.md) and
> [Annotation System Redesign](annotation-system-v2.md).
>
> Related: the `count` annotation is an open problem in
> [Annotation System Redesign](annotation-system-v2.md#open-problem-variable-length-fields-count); the impact on
> framing is summarized in [Framing Design Notes §8](framing-design-notes.md#8-variable-size-wirable-types).

Each decision is tagged:

- **Decided** — agreed, not implemented yet.
- **Proposed** — recommended, still pending confirmation.
- **Open** — no answer yet.

---

## 1. Terminology

| Term | Meaning |
| --- | --- |
| **Fixed-size type** | Every member has a compile-time wire size. Everything that is wirable today. |
| **Variable-size type** | At least one member, directly or nested, has a runtime wire size. |
| **Variable-size member** | A `std::vector<T>`, or a nested variable-size struct. |
| **Fixed prefix** | The members before the first variable-size member. Their offsets are still compile-time. |
| **Anchor** | The point a member's offset is measured from: the start of the struct, or the end of a previous variable-size member. |
| **Delta** | The static distance from the anchor to the member. |
| **Offset table** | Runtime array holding the end of every variable-size member of a type, nested structs included. |

---

## 2. `count` and field order

**Decided.** The count of a `std::vector<T>` comes from another member, which must precede the vector on the wire
(fixed field order, REQ-051/052 in [Requirements](requirements.md)). The count source lives at the same nesting
level as the vector.

**Open — the annotation spelling.**

- **On the vector, naming its count** — the shape sketched in `annotation-system-v2.md`:

  ```cpp
  struct Adios {
    int count2;
    [[=rbe::count("count2")]] std::vector<int> numbers2;
    int adios;
    int count3;
    [[=rbe::count("count3")]] std::vector<int> numbers3;
    int sayonara;
  };
  ```

  A struct can hold several vectors, each knowing where its size comes from.
- **On the count member** (`[[=rbe::count]] int count;`): shorter, but it does not say which vector it sizes. As a
  unique dimension it would allow a single vector per struct.

**Open — counts outside the struct**, e.g. a header field sizing a payload's vector. The payload alone is not
self-delimiting and its length depends on the header, which crosses levels (see
[Framing Design Notes §8](framing-design-notes.md#8-variable-size-wirable-types)).

**Open — a trailing vector with no count** (sized by the end of the buffer) would make the struct
buffer-delimited. Probably not allowed at struct level, since `frame<H, blob>` already expresses it.

---

## 3. `proxy` semantics

**Decided.** There is a single `dsrl::proxy<T>` for fixed- and variable-size types.

- **Every member means the same for both**; only the costs differ (construction, `sizeof`, access complexity).
  `field<"vector">()` and every other accessor are used exactly as with a fixed-size `T`.
- **A different type is reserved for a different contract, not a different cost.** `std::span<T, N>` versus
  `std::span<T>` is the precedent; `std::vector<bool>` is the counterexample to avoid.
- **Members that only make sense for fixed-size types** (static-extent spans, `in_place`) are gated with
  `requires` instead of silently changing meaning.

**Decided — `proxy` narrows its span at construction**, so it always views exactly its object:
`data_.size() == length()`.

- Fixed-size `T`: `data.first(wire_size_of<T>())`, which is free.
- Variable-size `T`: the offset table built by the constructor yields the length.

The span passed in may be larger than the object; the extra bytes are not kept. `as_span()` is just `data_`, and
`size()`/`size_bytes()` are removed, since `length()` is the object's size.

**Decided — lazy means values, not layout.** Field values are still decoded on demand, one at a time, with no
copy of the object (REQ-011..013). For variable-size `T` the *layout* is resolved eagerly: construction reads only
the counts needed to place each member, never the values. The cost is zero for fixed-size `T`, one read per
count with fixed-size elements, and a walk over element counts with variable-size elements. `frame` already
works this way: it reads the header length fields to locate the payload.

**Proposed — REQ-012 wording.** "Fields must be read on demand, one at a time" should say that field *values* are
decoded on demand, while layout fields may be read at construction.

---

## 4. Offset table built at construction

**Decided.** A variable-size `proxy` computes its offset table **in the constructor**, and every member stays
`const`.

Rejected alternatives:

- **A lazy table in a `mutable` member.** It would spare prefix-only reads the construction cost, but caching
  behind `const` needs `mutable`, which is only justified for synchronization primitives such as a mutex; it
  would also break concurrent `const` access to the same object.
- **Dropping `const` from the accessors** to allow a lazy cache, as `std::ranges::filter_view::begin()` does: the
  accessors do not change the object's observable state.
- **Recomputing on every access plus an explicit `resolve()`**: it pushes an extra step onto the user, and
  variable-size types would no longer be used exactly like fixed-size ones.

---

## 5. Static layout: anchor and delta

With a variable-size member, `get_wire_layout`'s `{offset, size}` per member can no longer be fully static:
everything after the vector moves depending on the bytes. The layout stays **compile-time and shallow** (one
entry per direct member, like today), but an offset becomes:

> "**delta bytes after** where the last previous variable-size member ended"

The anchor is that previous member; where it ended is only known at runtime and lives in the table. The delta
spans only fixed-size members, so it is static. Directions like "3 metres after the second tree" are the
analogy: the tree's position goes in the table, the 3 metres in the static layout.

**Proposed — extending `member_layout` / `struct_layout`** (`rbe/core/memory_layout.hpp`) with defaulted fields,
so the fixed-size case stays exactly as it is:

```cpp
struct member_layout {
  member_offset offset {};                   // relative to anchor: the delta
  std::size_t size {};                       // static wire size; meaningless for variable-size members
  endian::order endianness {std::endian::little};
  std::optional<std::size_t> anchor {};      // table entry the offset is measured from; empty = start of the struct
  std::optional<std::size_t> end_entry {};   // variable-size members: table entry holding their end
  table_slice slice {};                      // nested variable-size structs: their entries inside this table

  constexpr bool operator==(member_layout const&) const = default;
};

struct struct_layout {
  std::size_t size {};                       // static wire size; for variable-size types, see end below
  static_array<member_layout> members {};
  std::size_t table_entries {};              // 0 for fixed-size types
  // variable-size types: where the struct ends, as {anchor, delta} -- the same shape as a member offset
};
```

- **Fixed-size types**: `anchor` and `end_entry` are empty and `table_entries == 0`, so `offset` means exactly what
  it means today, and `proxy::field<Index>()` keeps slicing with static offsets.
- **Existing tests keep passing**: `tests/runtime/test_memory_layout.cpp` builds expected layouts with
  designated initializers (`.offset = ...`) and compares them with `==`; defaulted fields keep those valid.
- **An empty `optional`, not anchor 0**: 0 is a valid table index (e0, the end of the first variable-size member).
  A sentinel value would only be needed if the layout were ever used as a non-type template parameter.

**Fixed-size type versus fixed-size member.** In a fixed-size *type* every member is measured from the start. A
fixed-size *member* inside a variable-size type may still have an anchor: what decides it is whether a
variable-size member comes *before* it, not the member's own size.

---

## 6. The runtime table

**What it stores**: only the numbers that cannot be known at compile time — **the end of each variable-size
member** (vectors and nested variable-size structs), with nested structs' entries included (§7). It is not a
layout; the static layout says which entry each member is measured from.

**Where it lives**: inside the `proxy` object, as an array whose size is known at compile time
(`std::array<size_type, table_entries>`). For fixed-size `T` it is an empty type held with `[[no_unique_address]]`,
so a fixed-size `proxy` takes exactly the space it takes today.

**How offsets are recovered**, giving what `get_wire_layout` would return if it could:

```
offset(i) = (anchor(i) ? table[*anchor(i)] : 0) + delta(i)
size(i)   = end_entry(i) ? table[*end_entry(i)] - offset(i) : static_size(i)
```

`field<Index>()` goes through an `offset_of(index)` helper instead of reading `member_layout.offset` directly.
Members of the fixed prefix keep their compile-time offset.

---

## 7. Nested variable-size structs

**Proposed — one table for the whole tree of nested structs.** To compute where a nested variable-size member
ends, the parent's constructor must compute its length, which means computing every end inside it — which *is*
the nested struct's table. Keeping those values instead of discarding them costs nothing extra.

- **The root's table has one entry per variable-size member of the whole tree**, numbered in preorder. Struct
  nesting is static, so the count is known at compile time.
- **Each nested struct owns a contiguous slice** of that table, also known at compile time: the child's entries
  come first, followed by the entry holding the child's own end.
- **Offsets inside a slice are relative to the start of that struct**, so a slice can be copied to the child as is.
- **`field<"child">()` builds the child `proxy` through a private constructor** taking its exact span and a copy
  of its slice. It walks nothing.
- **Same type either way**: a `proxy<Child>` built directly computes its table; one built from a parent receives
  it.

**Why the child copies its slice instead of pointing into the parent's table**: a pointer would tie the child to
the parent's lifetime.

```cpp
auto const adios = rbe::deserialize<Hola>(buf, lazy).field<"adios">(); // the proxy<Hola> temporary dies here
adios.field<"adios">();                                                 // would read a destroyed table
```

With a copy, the child only depends on the buffer, like any `proxy`. Repeated paths through the same intermediate
(`field<"adios", "adios">()`, then `field<"adios", "sayonara">()`) build a temporary each time, which costs a
span plus a few offsets and reads no count; inlining usually removes the temporary altogether.

**Proposed, deferred — flat path resolution.** The path of `field<"a", "b">()` is compile-time, so the final
offset can be computed directly from the root table (e.g. `hola.adios.sayonara` = `table[0] + table[2] + 0`),
with no intermediate proxy. Only worth doing if a benchmark shows the compiler does not remove the temporaries.

**Storing the intermediate view** still reads best when several fields of the same place are used:

```cpp
auto const adios = view.field<"adios">();
adios.field<"adios">();
adios.field<"sayonara">();
```

---

## 8. Resolution: shallow per level

The runtime resolution follows the same model as the compile-time code: `get_wire_layout` is shallow, and
recursion only appears to compute nested lengths, as in `wire_size_of`.

Compile time:

```cpp
// entries a type needs: its direct variable-size members plus the slices of its nested structs (recursive,
// like wire_size_of)
consteval auto table_entries(std::meta::info type, detail::context ctx) -> std::size_t;
```

Runtime, one pass per level:

```cpp
template<wirable T, detail::context Ctx>
constexpr auto resolve(std::span<std::byte const> data, std::span<size_type> table) -> size_type {
  size_type cursor = 0;
  template for (constexpr auto i : member_indices<T>) {
    auto const start = /* anchor(i) ? table[*anchor(i)] + delta(i) : delta(i) (+ alignment, see §10) */;
    auto const length =
        /* nested variable-size struct       */ resolve<Member, MemberCtx>(data.subspan(start), table.subspan(slice(i)))
        /* vector of fixed-size elements     */ count * element_size
        /* vector of variable-size elements  */ walk over the elements (their tables are not kept, §9)
        /* anything else                     */ static size;
    cursor = start + length;
    if (/* end_entry(i) */) table[*end_entry(i)] = cursor;
  }
  return cursor; // this level's length
}
```

- Each call only goes over its direct members.
- The `count` is read at the same level (§2), so it is shallow too.
- **`wire_size_of<T>()` stops being total** for variable-size types: they have a static minimum (the fixed
  prefix) plus a runtime length, the struct-level counterpart of `resolve`'s return value. Headers keep
  requiring a fixed size.

---

## 9. Vectors of variable-size elements

**Decided — the boundary of the table.** The number of elements is only known at runtime, and each element has
its own table, so element tables are not kept. The root's table still stores where the whole vector ends.

- Lazily accessing a vector yields a range over the element bytes: random access when the element size is static,
  forward-only otherwise — the same categories as `rbe::many`.
- Each element is resolved while iterating, as with `many`.
- `in_place` is unavailable: a variable-size type is not trivially wirable.

---

## 10. Native alignment

`get_wire_layout_padded` takes member offsets from the C++ layout (`offset_of(m)`). Past a `std::vector` member
that is meaningless: the C++ offset of the next member depends on `sizeof(std::vector)` (24 bytes on x86-64), not
on the vector's wire size.

**Open**, two options:

- **Accumulate and round up at runtime** (`align_up(cursor, alignment)`), reproducing native padding rules on the
  wire.
- **Require `pack` for variable-size types**, which is what protocols with variable-size fields do in practice.

**Proposed**: require `pack` first, and add native alignment when something needs it.

---

## 11. Worked example

```cpp
struct [[=rbe::pack]] Adios {
  [[=rbe::count]] int count;   // see §2: which vector it sizes is still open
  std::vector<int> numbers2;
  int adios;
  std::vector<int> numbers3;
  int sayonara;
};

struct [[=rbe::pack]] Hola {
  [[=rbe::count]] int count;
  std::vector<int> numbers;
  Adios adios;
  int hola;
};
```

### Static layout

`Adios` (local entries e0, e1):

| Member | Anchor (measured from) | Delta | Size |
| --- | --- | --- | --- |
| `count` | start | 0 | 4 |
| `numbers2` | start | 4 | variable → end stored in e0 |
| `adios` | end of `numbers2` (e0) | 0 | 4 |
| `numbers3` | end of `numbers2` (e0) | 4 | variable → end stored in e1 |
| `sayonara` | end of `numbers3` (e1) | 0 | 4 |
| *(end of Adios)* | end of `numbers3` (e1) | 4 | — |

`Hola` (4 entries in total):

| Member | Anchor (measured from) | Delta | Size |
| --- | --- | --- | --- |
| `count` | start | 0 | 4 |
| `numbers` | start | 4 | variable → end stored in e0 |
| `adios` | end of `numbers` (e0) | 0 | variable-size struct: slice e1..e2, end stored in e3 |
| `hola` | end of `adios` (e3) | 0 | 4 |
| *(end of Hola)* | end of `adios` (e3) | 4 | — |

### Runtime

With `Hola::count = 2`, `Adios::count = 3`, and `numbers3` holding 1 element:

```
Hola:  | count | numbers (2) | ─────────────── adios (28) ─────────────── | hola |
       0       4             12                                           40     44

Adios:         | count | numbers2 (3) | adios | numbers3 (1) | sayonara |
               0       4              16      20             24         28   (relative to Adios)
```

The constructor of `proxy<Hola>` does the only walk:

1. Reads `Hola::count` → `numbers` ends at 12 → `table[0] = 12`.
2. `adios` starts at `table[0]`. Resolves `Adios` relative to its own start:
   - reads `Adios::count` → `numbers2` ends at 16 → `table[1] = 16`;
   - `int adios` sits at `table[1]`, so `numbers3` starts at `table[1] + 4`;
   - `numbers3` ends at 24 → `table[2] = 24`;
   - `sayonara` sits at `table[2]`, so the length of `Adios` is `table[2] + 4` = 28.
3. `table[3] = table[0] + 28 = 40`. `hola` sits at `table[3]`.

```
table = [ 12 | 16 | 24 | 40 ]
          │    └ slice ┘    └ e3: end of adios (relative to Hola)
          │     of Adios
          └ e0: end of numbers (relative to Hola)
```

Applying `offset = table[anchor] + delta` recovers what `get_wire_layout` would return:

| `Hola` | Offset | Size | | `Adios` (relative) | Offset | Size |
| --- | --- | --- | --- | --- | --- | --- |
| `count` | 0 | 4 | | `count` | 0 | 4 |
| `numbers` | 4 | 8 | | `numbers2` | 4 | 12 |
| `adios` | 12 | 28 | | `adios` | 16 + 0 = 16 | 4 |
| `hola` | 40 + 0 = 40 | 4 | | `numbers3` | 16 + 4 = 20 | 24 − 20 = 4 |
| | | | | `sayonara` | 24 + 0 = 24 | 4 |

Only 4 numbers are stored instead of 9 offsets and 9 sizes; everything else is static.

### Accesses

```cpp
view.field<"adios", "adios">();    // proxy<Adios> temporary: copies slice [16, 24]
                                   // offset = slice[0] + 0 -> reads the int
view.field<"adios", "sayonara">(); // another temporary with the same 2-offset copy
                                   // offset = slice[1] + 0 = 24 -> 12 + 24 = 36 in the buffer
view.field<"adios", "numbers3">(); // start = slice[0] + 4, end = slice[1] -> zero-copy range
```

None of them reads a count again.

---

## 12. Consequences to document

For variable-size `T`:

- **Construction reads the counts** and walks the variable-size members, even if only a prefix field is used
  afterwards.
- **The precondition "the buffer covers the object" is hit at construction**, which is where the hardened check
  goes.
- **A proxy cannot be built over a partially received object.**
- **`sizeof(proxy)` grows** by one offset per variable-size member of the whole nested tree.
- **The layout is frozen at construction**: the viewed bytes must not change while the proxy is alive. Changing a
  count afterwards (e.g. reusing the buffer for the next message) leaves stale offsets that can point outside the
  span, whereas `frame` and fixed-size proxies would only observe new values.
- **Thread safety is unaffected**: construction only reads the shared buffer, and afterwards every member is
  `const` with no `mutable` state, so concurrent calls on the same proxy are safe as long as nobody writes the
  buffer.

---

## 13. Impact elsewhere

- **Framing**: `payload_extent` has to split `static_size` into fixed-size payloads and payloads that compute their
  own length from their bytes, the struct-level counterpart of `nested_frame`
  ([Framing Design Notes §8](framing-design-notes.md#8-variable-size-wirable-types)).
- **Eager versus lazy**: eager does not benefit from lazy's offset table, since it reads sequentially and picks up
  the counts while copying; it adds allocation and O(N) copies. See
  [Framing Design Notes §9](framing-design-notes.md#9-uniform-field-access-across-strategies).

---

## Pending work

- [ ] Settle the `count` annotation spelling (§2).
- [ ] Extend `member_layout` / `struct_layout` with `anchor`, `end_entry`, `slice`, `table_entries` and the struct end (§5).
- [ ] `table_entries` and the per-level `resolve` (§8).
- [ ] `proxy`: narrow at construction, offset table behind `offset_of(index)`, private constructor for nested slices (§3, §6, §7).
- [ ] Lazy ranges over vectors, random access or forward depending on the element size (§9).
- [ ] Decide native alignment versus requiring `pack` (§10).
- [ ] Split `payload_extent::static_size` (§13).
- [ ] Reword REQ-012 (§3).
