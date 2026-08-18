/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_layout.cpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

// --- Includes ---

// --- Dependencies ---
#include <rbe/core/memory_layout.hpp>

#include <rbe/core/fmt.hpp>
#include "rbe/concepts/trivially_wirable.hpp"

// --- External dependencies ---
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// --- STD ---
#include "common_structs.hpp"

// --- System ---

namespace {

TEST_CASE("Test B layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<B>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(B),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(B, m0), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test X layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<X>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(X),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(X, m1), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Y layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Y>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Y),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(Y, m2), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Inner layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Inner>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Inner),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(Inner, x), .bits = 0},
        .size   = sizeof(int),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(Inner, y), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Outer layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Outer>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Outer),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(Outer, inner), .bits = 0},
        .size   = sizeof(Inner),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(Outer, z), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Deep layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Deep>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Deep),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(Deep, outer), .bits = 0},
        .size   = sizeof(Outer),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(Deep, w), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Base layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Base>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Base),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(Base, m0), .bits = 0},
        .size   = sizeof(int),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(Base, m1), .bits = 0},
        .size   = sizeof(int),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(Base, m2), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test UnnamedMember layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<UnnamedMember>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(UnnamedMember),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(UnnamedMember, _), .bits = 0},
        .size   = sizeof(int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Bits layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Bits>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(Bits),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(unsigned int),
      },
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 3},
        .size   = sizeof(unsigned int),
      },
      rbe::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = sizeof(unsigned int),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test PacketHeader packed layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<PacketHeader>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = 8,
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = 2,
      },
      rbe::member_layout {
        .offset = {.bytes = 2, .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = 3, .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = 4, .bits = 0},
        .size   = 4,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test AddOrder packed layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<AddOrder>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = 27,
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = 2, .bits = 0},
        .size   = 4,
      },
      rbe::member_layout {
        .offset = {.bytes = 6, .bits = 0},
        .size   = 4,
      },
      rbe::member_layout {
        .offset = {.bytes = 10, .bits = 0},
        .size   = 1,
      },
      rbe::member_layout {
        .offset = {.bytes = 11, .bits = 0},
        .size   = 4,
      },
      rbe::member_layout {
        .offset = {.bytes = 15, .bits = 0},
        .size   = 8,
      },
      rbe::member_layout {
        .offset = {.bytes = 23, .bits = 0},
        .size   = 4,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test ReduceSize layout (no pack, has padding)") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<ReduceSize>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(ReduceSize),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(ReduceSize, length), .bits = 0},
        .size   = sizeof(std::uint8_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(ReduceSize, message_type), .bits = 0},
        .size   = sizeof(std::uint8_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(ReduceSize, time_offset), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(ReduceSize, order_id), .bits = 0},
        .size   = sizeof(std::uint64_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(ReduceSize, cancelled_shares), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test NoPack layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<NoPack>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(NoPack),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(NoPack, a), .bits = 0},
        .size   = sizeof(std::uint8_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(NoPack, b), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Packed layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Packed>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = 5,
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(std::uint8_t),
      },
      rbe::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test MixedEndian layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<MixedEndian>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MixedEndian),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MixedEndian, a), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::little,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MixedEndian, b), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MixedEndian, c), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test Complex packed layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<Complex>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = 7,
    .members = rbe::static_array {
      rbe::member_layout {
        .offset     = {.bytes = 0, .bits = 0},
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::little,
      },
      rbe::member_layout {
        .offset     = {.bytes = 2, .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = 6, .bits = 0},
        .size       = sizeof(std::uint8_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test NonPaddedStruct2 layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<NonPaddedStruct2>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(NonPaddedStruct2),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(NonPaddedStruct2, a), .bits = 0},
        .size   = sizeof(int),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(NonPaddedStruct2, b), .bits = 0},
        .size   = sizeof(int),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(NonPaddedStruct2, c), .bits = 0},
        .size   = sizeof(char),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test CommonHeader layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<CommonHeader>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(CommonHeader),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(CommonHeader, version), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(CommonHeader, size), .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(CommonHeader, type), .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(CommonHeader, timestamp), .bits = 0},
        .size   = sizeof(std::uint64_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test MessageWithHeader layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<MessageWithHeader>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MessageWithHeader),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithHeader, header), .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithHeader, price), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithHeader, volume), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithHeader, orders), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test CommonHeaderPackBe layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<CommonHeaderPackBe>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(CommonHeaderPackBe),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset     = {.bytes = offsetof(CommonHeaderPackBe, version), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(CommonHeaderPackBe, size), .bits = 0},
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(CommonHeaderPackBe, type), .bits = 0},
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(CommonHeaderPackBe, timestamp), .bits = 0},
        .size       = sizeof(std::uint64_t),
        .endianness = rbe::endian::order::big,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test MessageWithHeaderPackBe layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<MessageWithHeaderPackBe>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MessageWithHeaderPackBe),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MessageWithHeaderPackBe, header), .bits = 0},
        .size       = sizeof(CommonHeaderPackBe),
        .endianness = rbe::endian::order::big,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MessageWithHeaderPackBe, price), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MessageWithHeaderPackBe, volume), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MessageWithHeaderPackBe, orders), .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test MessageWithEnum layout") {
  static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<MessageWithEnum>();
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MessageWithEnum),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithEnum, flags), .bits = 0},
        .size   = sizeof(test_flags),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithEnum, number), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithEnum, number2), .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  CHECK(layout == layout_expected);
}

TEST_CASE("Test struct layout - Nested class") {
  static constexpr auto layout                        = rbe::get_struct_layout(^^MessageWithArray);
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MessageWithArray),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithArray, traderID), .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithArray, senderID), .bits = 0},
        .size   = sizeof(MessageWithArray::senderID),
      },
    },
  };
  CHECK(layout == layout_expected);
}

TEST_CASE("Test struct with big endian array") {
  static constexpr auto wire_layout                   = rbe::get_wire_layout(^^MessageWithArrayBe);
  static constexpr auto layout                        = rbe::get_struct_layout(^^MessageWithArrayBe);
  static constexpr rbe::struct_layout layout_expected = {
    .size    = sizeof(MessageWithArrayBe),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::member_layout {
        .offset = {.bytes = offsetof(MessageWithArrayBe, traderID), .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::member_layout {
        .offset     = {.bytes = offsetof(MessageWithArrayBe, senderID), .bits = 0},
        .size       = sizeof(MessageWithArrayBe::senderID),
        .endianness = rbe::endian::order::little,
      },
    },
  };
  static constexpr rbe::struct_layout wire_layout_expected = {
    .size    = sizeof(CommonHeader) + sizeof(std::uint16_t) + sizeof(MessageWithArrayBe::senderID),
    .members = rbe::static_array {
      rbe::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::member_layout {
        .offset = {.bytes = 16, .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::member_layout {
        .offset     = {.bytes = 18, .bits = 0},
        .size       = sizeof(MessageWithArrayBe::senderID),
        .endianness = rbe::endian::order::big,
      },
    },
  };
  CHECK(layout == layout_expected);
  CHECK(wire_layout == wire_layout_expected);
}


static_assert(rbe::get_wire_layout<CommonHeaderPackBe>() != rbe::get_struct_layout<CommonHeaderPackBe>());
static_assert(rbe::get_wire_layout<MessageWithHeaderPackBe>() != rbe::get_struct_layout<MessageWithHeaderPackBe>());

// NOTE: this cases are confusing and should be well defined
// TODO: define the proper behaviour of the library regarding this cases
// The first case for example layouts are different however this structure should be still trivially wirable
// because it only differs in padding bits
static_assert(rbe::get_wire_layout<MessageWithHeaderPack>() != rbe::get_struct_layout<MessageWithHeaderPack>());
static_assert(rbe::get_wire_layout<MessageWithHeaderMemberBe>() == rbe::get_struct_layout<MessageWithHeaderMemberBe>());

static_assert(rbe::trivially_wirable<MessageWithArray>);
static_assert(rbe::trivially_wirable<MessageWithArray>);
static_assert(rbe::trivially_wirable<MessageWithCArray>);


// TODO: Doubles
// TEST_CASE("Test PaddedStruct layout") {
//   static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<PaddedStruct>();
//   static constexpr rbe::struct_layout layout_expected = {
//     .size    = sizeof(PaddedStruct),
//     .members = rbe::static_array {
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(PaddedStruct, a), .bits = 0},
//         .size   = sizeof(int),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(PaddedStruct, b), .bits = 0},
//         .size   = sizeof(double),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(PaddedStruct, c), .bits = 0},
//         .size   = sizeof(char),
//       },
//     }
//   };
//
//   CHECK(layout == layout_expected);
// }
// TEST_CASE("Test NonPaddedStruct layout") {
//   static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<NonPaddedStruct>();
//   static constexpr rbe::struct_layout layout_expected = {
//     .size    = sizeof(NonPaddedStruct),
//     .members = rbe::static_array {
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(NonPaddedStruct, a), .bits = 0},
//         .size   = sizeof(int),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(NonPaddedStruct, b), .bits = 0},
//         .size   = sizeof(int),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(NonPaddedStruct, c), .bits = 0},
//         .size   = sizeof(double),
//       },
//     }
//   };
//
//   CHECK(layout == layout_expected);
// }

// TEST_CASE("Test NoAggregateCustomSerder layout") {
//   static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<NoAggregateCustomSerder>();
//   static constexpr rbe::struct_layout layout_expected = {
//     .size = sizeof(NoAggregateCustomSerder), .members = rbe::static_array {}
//   };
//
//   CHECK(layout == layout_expected);
// }


// TEST_CASE("Test NoAggregate layout") {
//   static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<NoAggregate>();
//   static constexpr rbe::struct_layout layout_expected = {
//     .size    = sizeof(NoAggregate),
//     .members = rbe::static_array {
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(NoAggregate, number), .bits = 0},
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(NoAggregate, number2), .bits = 0},
//         .size   = sizeof(std::uint32_t),
//       },
//     }
//   };
//
//   CHECK(layout == layout_expected);
// }

// TODO: Derived
// TEST_CASE("Test AggregateDerived layout") {
//   static constexpr rbe::struct_layout layout          = rbe::get_wire_layout<AggregateDerived>();
//   static constexpr rbe::struct_layout layout_expected = {
//     .size    = sizeof(AggregateDerived),
//     .members = rbe::static_array {
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(AggregateDerived, number), .bits = 0},
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(AggregateDerived, number2), .bits = 0},
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::member_layout {
//         .offset = {.bytes = offsetof(AggregateDerived, numbers_derived), .bits = 0},
//         .size   = sizeof(std::uint32_t),
//       },
//     }
//   };
//
//   CHECK(layout == layout_expected);
// }

} // namespace
