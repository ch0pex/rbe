/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file rbe.hpp
 * @date 24/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace rbe {


namespace cboeu {

// clang-format off
//
// [[=rbe::little, =rbe::packing]]
// struct PacketHeader {
//     std::uint16_t length;
//     std::uint8_t  count;
//     std::uint8_t  unit;
//     std::uint32_t sequence;
// };
//
// [[=rbe::little, =rbe::packing]]
// struct AddOrder {
//     [[=rbe::length]] 
//     std::uint8_t  length;
//     [[=rbe::id]] 
//     std::uint8_t  message_type;
//     std::uint32_t time_offset;
//     std::uint32_t order_id;
//     std::uint8_t  side_indicator;
//     std::uint32_t quantity;
//     std::uint64_t symbol;
//     std::uint32_t price;
// };
//
// [[=rbe::little, =rbe::packing]]
// struct ReduceSizeA {
//     [[=rbe::length]]   std::uint8_t  length;
//     [[=rbe::id]]       std::uint8_t  message_type;
//
//     // 24-bits
//     [[=rbe::size(24)]] 
//     std::uint32_t time_offset;
//
//     std::uint64_t order_id;
//     std::uint32_t cancelled_shares;
// };
//
// [[=rbe::size(24)]] using std::uint24_t = std::uint32_t;
// [[=rbe::size(48)]] using std::uint48_t = std::uint32_t;
//
// [[=rbe::little, =rbe::packing]]
// struct ReduceSizeB {
//     [[=rbe::length]] std::uint8_t  length;
//     [[=rbe::id]] std::uint8_t  message_type;
//
//     // 24-bits
//     rbe::uint24_t time_offset;
//
//     std::uint64_t order_id;
//     std::uint32_t cancelled_shares;
// };

}

// clang-format on


} // namespace rbe
