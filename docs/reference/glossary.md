# Glossary

The words the documentation uses with a precise meaning.

| Term | Meaning |
|---|---|
| **Annotation** | A C++26 construct, `[[=value]]`, attaching a constant to a declaration. RBE reads its own annotations back through reflection; they have no effect on the struct. |
| **Annotation range** | The set of annotations one rule is checked against: a struct's own, or a member's own together with those of the member's type. At most one annotation per dimension per range. |
| **Dimension** | A group of annotations that answer one question (byte order, alignment, id, length, ...). One per range; some dimensions add a rule across the whole type. |
| **Wirable** | A type RBE can write to and read from the wire: integers, enumerations, arrays of them, aggregates of wirable members with no base class, and types with an `rbe::custom` specialization. |
| **Trivially wirable** | A wirable type whose in-memory layout is its wire layout, so that every operation on it is a `memcpy` or a reinterpretation. |
| **Wire layout, struct layout** | Where each member sits and how wide it is on the wire, after annotations; and in memory, as the compiler lays it out. Equal for a trivially wirable type. |
| **Size, length** | *Size* is what the code knows: `rbe::wire_size_of<T>()`, `sizeof`. *Length* is what the wire says: a `frame_length` field, a header's `length()`. They agree unless the protocol has moved on. |
| **Serde** | Serialization and deserialization, taken together. `rbe/srl` and `rbe/dsrl` are the two halves. |
| **Narrow contract, wider contract** | A function has a narrow contract when some inputs it accepts have undefined behavior (`serialize`, `deserialize` with a short buffer). The `try_` forms have a wider one: they check the preconditions that can be checked, size and alignment, and report instead; what depends on a pointer being valid stays assumed. |
| **Strategy** | How a value is read: eager (decode everything), lazy (a view, decode on access), in place (reinterpret the bytes). The tag passed to `rbe::deserialize`. |
| **Proxy, view** | An object that borrows a buffer and decodes parts of it on demand. `rbe::dsrl::proxy<T>` for a struct, the `dsrl::` types for frames. |
| **Frame** | A header followed by a payload. The unit frames compose: `rbe::frames::frame<Header, Payload>`. |
| **Header** | Any wirable type at the front of a frame. Its length and id annotations tell the frame where the payload ends and what it is. |
| **Payload** | What follows the header: a wirable type, an `any`, a `many`, a `blob` or another frame. |
| **Candidate** | One of the types an `rbe::frames::any` may hold. Each declares the id it answers to. |
| **Id** | The value that selects a candidate: a header field marked `=rbe::id`, matched against each candidate's `=rbe::id(value)`. An *unknown* id is one no candidate answers to; *unmatched* is the `match` callback that receives unknown ids and unhandled candidates. |
| **Delimiting** | How a frame knows where it ends. *Self-delimited*: from its own contents, either *explicitly* (a length or count field) or *implicitly* (the types in the payload). *Buffer-delimited*: it ends where the buffer does. |
| **Extent** | The bytes a frame or payload occupies on the wire, as a span. |
| **Stride** | The extent of one element of a run. *Static* when it is a compile-time constant. |
| **Constant length** | A frame that answers its extent in O(1), by carrying `length()`. The alternative is a walk (`rbe::frames::distance`). |
| **Vocabulary type** | One of `rbe::frames::frame`, `any`, `many`, `blob`, or a user's equivalent: an empty type that names the shape and lowers to a view (`dsrl_type`) that has the API. |
