# RBE Requirements

## Functional Requirements

### Protocol Declaration
- **REQ-001**: The library must support declaring binary protocols using plain C++ structs
- **REQ-002**: Layout must be expressed through the type system, not annotations
- **REQ-003**: The library must provide struct-level annotations: `=rbe::little`, `=rbe::big`, `=rbe::pack`
- **REQ-004**: The library must provide field-level annotations: `=rbe::id`, `=rbe::length`, and per-field endianness overrides
- **REQ-005**: The library must provide non-standard layout types: `rbe::uint24_t`, `rbe::uint48_t`, `rbe::padding<N>`, `rbe::string<N>`, `rbe::vector<T, N>`

### Serialization
- **REQ-006**: Serialization must be eager (execute immediately on `serialize()` call)
- **REQ-007**: Serialization must support single message: `serialize(msg, out)`
- **REQ-008**: Serialization must support multiple messages: `serialize(header, payload, out)`
- **REQ-009**: Trivially serializable structs must be reduced to `memcpy`
- **REQ-010**: Endianness must be applied per-field during serialization

### Deserialization Modes

The library must support three deserialization modes:

#### Lazy Deserialization
- **REQ-011**: Lazy mode must return a lightweight view over the buffer
- **REQ-012**: Fields must be read on demand, one at a time
- **REQ-013**: No full copy of the message should be made
- **REQ-014**: Must support field access by name: `msg.field("name")`
- **REQ-015**: Missing fields must trigger compile-time errors
- **REQ-016**: Endianness translation must be applied per field at read time

#### Eager Deserialization
- **REQ-017**: Eager mode must materialize the entire message from the buffer
- **REQ-018**: All fields must be read and stored in an in-memory representation
- **REQ-019**: Must be compatible with all types (trivial and non-trivial)
- **REQ-020**: Endianness translation must be applied to all fields upfront

#### In-place Deserialization
- **REQ-021**: In-place mode must interpret the buffer directly as the struct (via bitcast)
- **REQ-022**: Must only work with trivially serializable types (wire layout == C++ struct layout)
- **REQ-023**: Must use zero-copy, zero-allocation approach
- **REQ-024**: Must not perform endianness translation (or only support native byte order)
- **REQ-025**: Compile-time verification must ensure type compatibility

### Trivially Serializable Types
- **REQ-026**: A type is trivially serializable when wire layout equals C++ struct layout
- **REQ-027**: Trivially serializable types can be serialized/deserialized via `memcpy`
- **REQ-028**: All primitive types with fixed byte widths are trivially serializable
- **REQ-029**: RBE wrapper types like `rbe::uint24_t` must define their layout explicitly

### Packet Composition
- **REQ-030**: The library must provide a generic packet composition type for grouping headers and payloads
- **REQ-031**: The `flatten()` utility must decompose packets back into constituent parts

### Type Erasure and Dispatch
- **REQ-032**: Structs with `=rbe::id` annotation must participate in type-erased message dispatch
- **REQ-033**: `any_msg` must hold messages of any registered type
- **REQ-034**: `any_msg` must support `visit`/`match` patterns for generic message processing
- **REQ-035**: Message dispatch must work without knowing the concrete type at the call site

### Debugging
- **REQ-036**: The library must provide binary format debugging (raw byte dump)
- **REQ-037**: The library must provide text format debugging (human-readable field-by-field output)
- **REQ-038**: Text format must use field names derived from reflection

## Robustness and Safety Requirements

### Exception Handling
- **REQ-039**: The library must support compilation with exceptions enabled
- **REQ-040**: The library must support compilation without exceptions (freestanding/embedded)
- **REQ-041**: When exceptions are disabled, errors must be reported through alternative mechanisms (error codes, assertions, or contracts)

### Hardened vs Non-hardened Builds
- **REQ-042**: The library must support a "hardened" build mode with enhanced safety checks
- **REQ-043**: Hardened mode must enforce stricter validation and bounds checking
- **REQ-044**: Hardened mode must provide defensive runtime checks
- **REQ-045**: Non-hardened mode must optimize for performance with minimal runtime overhead
- **REQ-046**: Non-hardened mode may assume inputs are well-formed

## Non-Functional Requirements

### Performance
- **REQ-047**: The library must impose no runtime overhead over hand-written implementations
- **REQ-048**: All serialization and deserialization logic must be derived at compile time
- **REQ-049**: Trivially serializable types must be optimized to `memcpy` operations
- **REQ-050**: In-place deserialization must be zero-copy and zero-allocation

### Design Constraints
- **REQ-051**: The library only supports protocols with fixed field order
- **REQ-052**: Field order on the wire must match the struct declaration order
- **REQ-053**: Complex types (nested structs, pointers, non-trivial members) are not supported
- **REQ-054**: Variable-order protocols are out of scope

### Scope Limitations
- **REQ-055**: Variable-length fields are supported within the fixed-order constraint
- **REQ-056**: Optional fields are supported within the fixed-order constraint
- **REQ-057**: Repeated fields are supported within the fixed-order constraint
