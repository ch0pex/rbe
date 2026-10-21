/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file test_memory_layout.cpp
 * @date 27/06/2026
 * @brief Doctest suite for struct and wire layout computation
 */

// --- Includes ---
#include "common_structs.hpp"

#include <rbe/core/detail/memory_layout.hpp>
#include <rbe/core/fmt.hpp>
#include <rbe/core/trivially_wirable_concepts.hpp>

// --- External dependencies ---
#include "test_macros.hpp"

// --- STD ---

// --- System ---

namespace {

TEST_SUITE_BEGIN("memory_layout");

TEST_CASE("memory_layout - B layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<B>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(B),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^B::m0),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - X layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<X>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(X),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^X::m1),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Y layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Y>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Y),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Y::m2),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Inner layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Inner>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Inner),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Inner::x),
        .size   = sizeof(int),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Inner::y),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Outer layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Outer>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Outer),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Outer::inner),
        .size   = sizeof(Inner),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Outer::z),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Deep layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Deep>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Deep),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Deep::outer),
        .size   = sizeof(Outer),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Deep::w),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Base layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Base>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Base),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Base::m0),
        .size   = sizeof(int),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Base::m1),
        .size   = sizeof(int),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^Base::m2),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - UnnamedMember layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<UnnamedMember>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(UnnamedMember),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^UnnamedMember::_),
        .size   = sizeof(int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Bits layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Bits>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(Bits),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(unsigned int),
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 3},
        .size   = sizeof(unsigned int),
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = sizeof(unsigned int),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - PacketHeader packed layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<PacketHeader>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = 8,
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = 2,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 2, .bits = 0},
        .size   = 1,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 3, .bits = 0},
        .size   = 1,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 4, .bits = 0},
        .size   = 4,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - AddOrder packed layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<AddOrder>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = 27,
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = 1,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = 1,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 2, .bits = 0},
        .size   = 4,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 6, .bits = 0},
        .size   = 4,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 10, .bits = 0},
        .size   = 1,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 11, .bits = 0},
        .size   = 4,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 15, .bits = 0},
        .size   = 8,
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 23, .bits = 0},
        .size   = 4,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - ReduceSize layout (no pack, has padding)") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<ReduceSize>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(ReduceSize),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^ReduceSize::length),
        .size   = sizeof(std::uint8_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^ReduceSize::message_type),
        .size   = sizeof(std::uint8_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^ReduceSize::time_offset),
        .size   = sizeof(std::uint32_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^ReduceSize::order_id),
        .size   = sizeof(std::uint64_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^ReduceSize::cancelled_shares),
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - NoPack layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<NoPack>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(NoPack),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^NoPack::a),
        .size   = sizeof(std::uint8_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^NoPack::b),
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Packed layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Packed>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = 5,
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(std::uint8_t),
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 1, .bits = 0},
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MixedEndian layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<MixedEndian>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MixedEndian),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MixedEndian::a),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::little,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MixedEndian::b),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MixedEndian::c),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - Complex packed layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<Complex>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = 7,
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset     = {.bytes = 0, .bits = 0},
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::little,
      },
      rbe::detail::member_layout {
        .offset     = {.bytes = 2, .bits = 0},
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = {.bytes = 6, .bits = 0},
        .size       = sizeof(std::uint8_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - NonPaddedStruct2 layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<NonPaddedStruct2>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(NonPaddedStruct2),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^NonPaddedStruct2::a),
        .size   = sizeof(int),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^NonPaddedStruct2::b),
        .size   = sizeof(int),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^NonPaddedStruct2::c),
        .size   = sizeof(char),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - CommonHeader layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<CommonHeader>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(CommonHeader),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^CommonHeader::version),
        .size   = sizeof(std::uint32_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^CommonHeader::size),
        .size   = sizeof(std::uint16_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^CommonHeader::type),
        .size   = sizeof(std::uint16_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^CommonHeader::timestamp),
        .size   = sizeof(std::uint64_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MessageWithHeader layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<MessageWithHeader>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MessageWithHeader),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithHeader::header),
        .size   = sizeof(CommonHeader),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithHeader::price),
        .size   = sizeof(std::uint32_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithHeader::volume),
        .size   = sizeof(std::uint32_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithHeader::orders),
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - CommonHeaderPackBe layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<CommonHeaderPackBe>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size       = sizeof(CommonHeaderPackBe),
    .endianness = rbe::endian::order::big,
    .members    = rbe::static_array {
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^CommonHeaderPackBe::version),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^CommonHeaderPackBe::size),
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^CommonHeaderPackBe::type),
        .size       = sizeof(std::uint16_t),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^CommonHeaderPackBe::timestamp),
        .size       = sizeof(std::uint64_t),
        .endianness = rbe::endian::order::big,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MessageWithHeaderPackBe layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<MessageWithHeaderPackBe>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MessageWithHeaderPackBe),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MessageWithHeaderPackBe::header),
        .size       = sizeof(CommonHeaderPackBe),
        .endianness = rbe::endian::order::big,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MessageWithHeaderPackBe::price),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MessageWithHeaderPackBe::volume),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MessageWithHeaderPackBe::orders),
        .size       = sizeof(std::uint32_t),
        .endianness = rbe::endian::order::native,
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MessageWithEnum layout") {
  static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<MessageWithEnum>();
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MessageWithEnum),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithEnum::flags),
        .size   = sizeof(test_flags),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithEnum::number),
        .size   = sizeof(std::uint32_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithEnum::number2),
        .size   = sizeof(std::uint32_t),
      },
    }
  };

  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MessageWithArray nested struct layout") {
  static constexpr auto layout                              = rbe::detail::get_struct_layout(^^MessageWithArray);
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MessageWithArray),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithArray::traderID),
        .size   = sizeof(std::uint16_t),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithArray::senderID),
        .size   = sizeof(MessageWithArray::senderID),
      },
    },
  };
  RBE_CHECK(layout == layout_expected);
}

TEST_CASE("memory_layout - MessageWithArrayBe big-endian array layout") {
  static constexpr auto wire_layout                         = rbe::detail::get_wire_layout(^^MessageWithArrayBe);
  static constexpr auto layout                              = rbe::detail::get_struct_layout(^^MessageWithArrayBe);
  static constexpr rbe::detail::type_layout layout_expected = {
    .size    = sizeof(MessageWithArrayBe),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::detail::member_layout {
        .offset = std::meta::offset_of(^^MessageWithArrayBe::traderID),
        .size   = sizeof(std::uint16_t),
      },
      rbe::detail::member_layout {
        .offset     = std::meta::offset_of(^^MessageWithArrayBe::senderID),
        .size       = sizeof(MessageWithArrayBe::senderID),
        .endianness = rbe::endian::order::little,
      },
    },
  };
  static constexpr rbe::detail::type_layout wire_layout_expected = {
    .size    = sizeof(CommonHeader) + sizeof(std::uint16_t) + sizeof(MessageWithArrayBe::senderID),
    .members = rbe::static_array {
      rbe::detail::member_layout {
        .offset = {.bytes = 0, .bits = 0},
        .size   = sizeof(CommonHeader),
      },
      rbe::detail::member_layout {
        .offset = {.bytes = 16, .bits = 0},
        .size   = sizeof(std::uint16_t),
      },
      rbe::detail::member_layout {
        .offset     = {.bytes = 18, .bits = 0},
        .size       = sizeof(MessageWithArrayBe::senderID),
        .endianness = rbe::endian::order::big,
      },
    },
  };
  RBE_CHECK(layout == layout_expected);
  RBE_CHECK(wire_layout == wire_layout_expected);
}

TEST_SUITE_END();


// clang-format off
static_assert(rbe::detail::get_wire_layout<CommonHeaderPackBe>() != rbe::detail::get_struct_layout<CommonHeaderPackBe>());
static_assert(rbe::detail::get_wire_layout<MessageWithHeaderPackBe>() != rbe::detail::get_struct_layout<MessageWithHeaderPackBe>());

// NOTE: this cases are confusing and should be well defined
// TODO: define the proper behaviour of the library regarding this cases
// The first case for example layouts are different however this structure should be still trivially wirable
// because it only differs in padding bits
static_assert(rbe::detail::get_wire_layout<MessageWithHeaderPack>() != rbe::detail::get_struct_layout<MessageWithHeaderPack>());
static_assert(rbe::detail::get_wire_layout<MessageWithHeaderMemberBe>() == rbe::detail::get_struct_layout<MessageWithHeaderMemberBe>());

static_assert(rbe::trivially_wirable<MessageWithArray>);
static_assert(rbe::trivially_wirable<MessageWithCArray>);
// clang-format on


// TODO: Doubles
// TEST_CASE("Test PaddedStruct layout") {
//   static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<PaddedStruct>();
//   static constexpr rbe::detail::type_layout layout_expected = {
//     .size    = sizeof(PaddedStruct),
//     .members = rbe::static_array {
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^PaddedStruct::a),
//         .size   = sizeof(int),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^PaddedStruct::b),
//         .size   = sizeof(double),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^PaddedStruct::c),
//         .size   = sizeof(char),
//       },
//     }
//   };
//
//   RBE_CHECK(layout == layout_expected);
// }
// TEST_CASE("Test NonPaddedStruct layout") {
//   static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<NonPaddedStruct>();
//   static constexpr rbe::detail::type_layout layout_expected = {
//     .size    = sizeof(NonPaddedStruct),
//     .members = rbe::static_array {
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^NonPaddedStruct::a),
//         .size   = sizeof(int),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^NonPaddedStruct::b),
//         .size   = sizeof(int),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^NonPaddedStruct::c),
//         .size   = sizeof(double),
//       },
//     }
//   };
//
//   RBE_CHECK(layout == layout_expected);
// }

// TEST_CASE("Test NoAggregateCustomSerder layout") {
//   static constexpr rbe::detail::type_layout layout          =
//   rbe::detail::get_wire_layout<NoAggregateCustomSerder>(); static constexpr rbe::detail::type_layout
//   layout_expected = {
//     .size = sizeof(NoAggregateCustomSerder), .members = rbe::static_array {}
//   };
//
//   RBE_CHECK(layout == layout_expected);
// }


// TEST_CASE("Test NoAggregate layout") {
//   static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<NoAggregate>();
//   static constexpr rbe::detail::type_layout layout_expected = {
//     .size    = sizeof(NoAggregate),
//     .members = rbe::static_array {
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^NoAggregate::number),
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^NoAggregate::number2),
//         .size   = sizeof(std::uint32_t),
//       },
//     }
//   };
//
//   RBE_CHECK(layout == layout_expected);
// }

// TODO: Derived
// TEST_CASE("Test AggregateDerived layout") {
//   static constexpr rbe::detail::type_layout layout          = rbe::detail::get_wire_layout<AggregateDerived>();
//   static constexpr rbe::detail::type_layout layout_expected = {
//     .size    = sizeof(AggregateDerived),
//     .members = rbe::static_array {
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^AggregateDerived::number),
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^AggregateDerived::number2),
//         .size   = sizeof(std::uint32_t),
//       },
//       rbe::detail::member_layout {
//         .offset = std::meta::offset_of(^^AggregateDerived::numbers_derived),
//         .size   = sizeof(std::uint32_t),
//       },
//     }
//   };
//
//   RBE_CHECK(layout == layout_expected);
// }

} // namespace
