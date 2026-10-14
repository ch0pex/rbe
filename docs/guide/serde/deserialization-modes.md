# Deserialization Modes

!!! note "Stub"
    Planned contents for this page.

- Eager: decode into a value (`example/dsrl/eager.cpp`)
- Lazy: proxy that decodes fields on access (`example/dsrl/lazy.cpp`, `dsrl/proxy.hpp`)
- In-place: reinterpret the buffer, trivially wirable only, no endianness translation (`example/dsrl/in_place.cpp`)
- Choosing a mode: cost, lifetime and alignment trade-offs
