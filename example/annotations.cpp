/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file annotations.cpp
 * @version 1.0
 * @date 12/08/2026
 * @brief Examples of annotation inheritance and propagation across struct and member levels
 */

#include <rbe/annotations.hpp>

namespace {
// Members derive parent type annotations
struct[[ = rbe::pack, = rbe::little ]] StructLevelAnnotations { // Default annotations for all members
  std::uint16_t length; // Derived pack and little
  std::uint8_t count; // Derived pack and little
  std::uint8_t unit; // Derived pack and little
  std::uint32_t sequence; // Derived pack and little
};

// Explicit member annotations overwrite derived annotations from struct
struct[[ = rbe::little, = rbe::pack ]] ExplicitMemberAnnotation {
  [[= rbe::big]] std::uint16_t length; // Derived pack, explicit big
  [[= rbe::big]] std::uint8_t count; // Derived pack, explicit big
  [[= rbe::big]] std::uint8_t unit; // Derived pack, explicit big
  std::uint32_t sequence; // Derived pack and little
};


// I believe option A to be appropriate and handy but might be dangerous if the lib is used for network communication
struct NoAnnotation { //
  std::uint16_t
      length; // Option A: implicit endian::native, align; Option B: compile error don't support implicit endiannes
  std::uint8_t
      count; // Option A: implicit endian::native, align; Option B: compile error don't support implicit endianness
  std::uint8_t
      unit; // Option A: implicit endian::native, align; Option B: compile error don't support implicit endianness
  std::uint32_t
      sequence; // Option A: implicit endian::native, align; Option B: compile error don't support implicit endianness
};

// Lets consider that we support implicit annotations bc its convenient
struct NoDefaultEndiannes {
  [[= rbe::big]] std::uint16_t length; // Implicit align, explicit big;
  [[= rbe::big]] std::uint8_t count; // Implicit align, explicit big;
  [[= rbe::big]] std::uint8_t unit; // Implicit align, explicit big;
  std::uint32_t sequence; // Implicit align and native endianness;
};

struct[[ = rbe::pack, = rbe::big ]] Header {
  std::uint8_t id;
  std::uint16_t length;
};

struct[[= rbe::little]] Msg {
  Header hdr; // Header remains big and packed
  std::uint32_t payload; // Derived Little endian
};

struct Msg2 {
  [[= rbe::little]] Header hdr; // Option A: Explicit annotation ignored Header remains big and packed; Option B:
                                // Compilation error, annotations conflict
  std::uint32_t payload; // Implicit native endianness and align
};


// Things get complicated when structure nesting is present:

struct Leaf {
  std::uint32_t valor; // Implicit native endianness and align
  [[= rbe::little]] std::uint64_t leaf_valor; // Explicit little, implicit align
};

struct[[= rbe::pack]] MiddleNode {
  Leaf leaf; // Implicit native endianness, derived pack (leaf don't have any explicit requirement related to aligment)
  std::uint32_t valor2; // Implicit native endianness
};

struct[[= rbe::big]] ParentNode {
  // The following example composing Leaf, MiddleNode y ParentNode demonstrates how annotations are propagated down
  // preserving explicit type/member annotations such as pragma pack behaves with explicitly aligned members

  // MiddleNode and Leaf annotations under ParentNode context would look like:
  // struct Leaf [[=rbe::pack, =rbe::big]] {
  //    std::uint32_t valor;                       // Implicit pack and big
  //    [[=rbe::little]] std::uint64_t leaf_valor; // Implicit pack, Explicit little remains
  // };

  // struct [[=rbe::pack, =rbe::big]] MiddleNode {
  //    Leaf leaf;             // Implicit big, derived pack (leaf don't have any explicit requirement related to
  //    alignment std::uint32_t valor2;  // Implicit big
  // };

  MiddleNode node; // Implicit big, explicit pack
  std::uint32_t valor3; // Implicit big, implicit align
};

} // namespace


int main() { return 0; }
