*[annotation]: A C++26 [[=value]] attached to a struct or member; RBE reads it back through reflection. It does not change the struct.
*[annotations]: C++26 [[=value]] constructs attached to a struct or member; RBE reads them back through reflection. They do not change the struct.
*[annotation range]: The annotations one rule is checked against: a struct's own, or a member's own plus its type's.
*[dimension]: A group of annotations answering one question (byte order, alignment, id, length...). At most one per annotation range.
*[dimensions]: Groups of annotations each answering one question (byte order, alignment, id, length...). At most one per annotation range.
*[wirable]: A type RBE can write to and read from the wire: integers, enumerations, arrays of them, classes of public wirable members with no base class, or classes annotated rbe::friendly.
*[trivially wirable]: A wirable type whose in-memory layout is its wire layout: every operation is a memcpy or a reinterpretation.
*[wire layout]: Where each member sits and how wide it is on the wire, after annotations.
*[serde]: Serialization and deserialization, taken together.
*[narrow contract]: Some inputs the function accepts have undefined behavior: a partial function.
*[wider contract]: The checkable preconditions (size, alignment) are checked and reported; a pointer being valid stays assumed.
*[strategy]: How a value is read: eager, lazy or in place. The tag passed to rbe::deserialize.
*[proxy]: A view that borrows a buffer and decodes fields on demand: rbe::dsrl::proxy<T>.
*[frame]: A header followed by a payload; the unit frames compose.
*[frames]: Headers each followed by a payload; the units a protocol is composed of.
*[header]: The wirable type at the front of a frame. Its length and id annotations say where the payload ends and what it is.
*[payload]: What follows a header: a wirable type, an any, a many, a blob or another frame.
*[candidate]: One of the types an any may hold; each declares the id it answers to.
*[candidates]: The types an any may hold; each declares the id it answers to.
*[delimiting]: How a frame knows where it ends: from its own contents (self-delimited) or where the buffer ends (buffer-delimited).
*[self-delimited]: A frame that finds its own end, explicitly (a length or count field) or implicitly (the payload's types).
*[buffer-delimited]: A frame that ends where the buffer it was handed ends.
*[extent]: The bytes a frame or payload occupies on the wire.
*[stride]: The extent of one element of a run; static when it is a compile-time constant.
*[constant length]: A frame that answers its extent in O(1) by carrying length(); otherwise rbe::frames::distance walks.
*[vocabulary type]: An empty type naming a frame's shape (frame, any, many, blob) that lowers to the view with the API.
*[unmatched]: The mandatory match callback: receives unknown ids and candidates no other callback handles.
*[UB]: Undefined behavior.
