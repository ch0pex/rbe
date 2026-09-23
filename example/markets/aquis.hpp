/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file aquis.hpp
 * @version 1.0
 * @date 05/08/2026
 * @brief Aquis Stock Exchange multicast market data protocol.
 *
 * Message set for the AQSE Market Data Feed, technical specification
 * v1.2.3 (April 2024). All integers are little-endian and 1-byte packed
 * on the wire (spec §3.1).
 *
 * Every market data message starts with a 6-byte header carrying the
 * message type, its total wire length, and the stream sequence number.
 * The header is declared once as `Header` and composed with the message
 * set through `rbe::frame`: `rbe::id` on the header field selects the
 * message annotated with the matching `rbe::id(value)`, and
 * `rbe::frame_length` bounds it. See `message` and `packet` at the end.
 */
#pragma once

#include <rbe/rbe.hpp>

#include <cstdint>
#include "rbe/annotations/empty.hpp"
#include "rbe/core/memory_layout.hpp"

namespace aquis {

// ─────────────────────────────────────────────────────────────────────
// Semantic type aliases (spec §3.1)
// ─────────────────────────────────────────────────────────────────────

/// Price with 5 implied decimal places. e.g. 1'462'500 → 14.625.
using price_t = std::uint64_t;

/// Elapsed nanoseconds since Unix epoch (00:00 UTC, 1970-01-01).
/// AQSE clock resolution is μs; the value is scaled to ns (×1000).
using timestamp_t = std::uint64_t;

using security_id_t = std::uint16_t; ///< Numeric identifier of a security.
using trader_id_t   = std::uint16_t; ///< Numeric identifier of a market maker.
using order_ref_t   = std::uint32_t; ///< Unique order reference for the day.
using trade_ref_t   = std::uint32_t; ///< Trade reference (not unique per day).

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

enum class message_type_t : std::uint8_t {
  heartbeat           = 1,
  order_add           = 2,
  order_cancel        = 3,
  order_modify        = 4,
  trade               = 5,
  trade_bust          = 6,
  tick_table_data     = 7,
  security_definition = 8,
  security_status     = 9,
  snapshot_start      = 10,
  book_status         = 11,
  book_entry          = 12,
  login               = 13,
  replay_request      = 14,
  replay_response     = 15,
  aod_update          = 17,
  quote_add_replace   = 20,
  quote_cancel        = 21,
  security_statistics = 22,
  trader_definition   = 23,
  trade_report        = 25,
  trade_report_modify = 26,
  trade_report_cancel = 27,
};

enum class side_t : std::uint8_t {
  buy  = 1,
  sell = 2,
};

enum class trading_status_t : std::uint8_t {
  active    = 1,
  halted    = 2,
  suspended = 3,
};

/// Trade category (spec §3.6.2). Other codes may be added.
enum class trade_type_t : std::uint8_t {
  visible              = 1, ///< Trade against visible book quantity.
  hidden               = 2, ///< Trade against hidden or reserve quantity.
  auction_on_demand    = 6,
  trade_capture_report = 8,
};

enum class price_type_t : std::uint8_t {
  opening = 2,
  closing = 5,
};

enum class response_code_t : std::uint8_t {
  login_successful = 0,
  bad_begin_seq_no = 1,
  bad_end_seq_no   = 2,
};

// ─────────────────────────────────────────────────────────────────────
// Fixed-width text fields (left-justified ASCII, 0x00 padded)
// ─────────────────────────────────────────────────────────────────────

using umtf_t      = std::array<char, 6>;
using isin_t      = std::array<char, 12>;
using currency_t  = std::array<char, 3>;
using mic_t       = std::array<char, 4>;
using name_t      = std::array<char, 10>;
using sender_id_t = std::array<char, 16>;
using username_t  = std::array<char, 10>;
using password_t  = std::array<char, 10>;

// ─────────────────────────────────────────────────────────────────────
// Multicast packet header (spec §3.2)
// ─────────────────────────────────────────────────────────────────────

/// Prefix of every UDP payload. Followed by `count` back-to-back messages.
struct[[= rbe::pack_le]] PacketHeader {
  std::uint8_t count;
};

// ─────────────────────────────────────────────────────────────────────
// Common market data message header (spec §3.3.1)
// ─────────────────────────────────────────────────────────────────────

/// 6-byte header prefix of every market data message.
///
/// Declared once for the whole protocol and composed with the message set
/// in `message`: `msg_type` selects the message whose `rbe::id(value)`
/// matches, and `length` covers the header plus the message.
struct[[= rbe::pack_le]] Header {
  [[= rbe::id]] message_type_t msg_type {};
  [[= rbe::frame_length]] std::uint8_t length {};
  std::uint32_t seq_no {};
};

// ─────────────────────────────────────────────────────────────────────
// Continuous data feed messages (spec §3.3–§3.4)
// ─────────────────────────────────────────────────────────────────────

// clang-format off

// Empty messages are supported by explicitly annotating them as empty
struct [[=rbe::pack_le, =rbe::id(message_type_t::heartbeat), =rbe::empty]] Heartbeat {};
static_assert(rbe::explicitly_empty<Heartbeat>);
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Heartbeat>() == rbe::wire_size_of<Header>());

struct [[=rbe::pack_le, =rbe::id(message_type_t::order_add)]] OrderAdd {
  security_id_t security_id;
  side_t        side;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<OrderAdd>() == 33);

struct [[=rbe::pack_le, =rbe::id(message_type_t::order_cancel)]] OrderCancel {
  security_id_t security_id;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<OrderCancel>() == 20);

struct [[=rbe::pack_le, =rbe::id(message_type_t::order_modify)]] OrderModify {
  security_id_t security_id;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<OrderModify>() == 32);

struct [[=rbe::pack_le, =rbe::id(message_type_t::quote_add_replace)]] QuoteAddReplace {
  trader_id_t   trader_id;
  security_id_t security_id;
  std::uint32_t bid_quantity;
  price_t       bid_price;
  std::uint32_t offer_quantity;
  price_t       offer_price;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<QuoteAddReplace>() == 42);

struct [[=rbe::pack_le, =rbe::id(message_type_t::quote_cancel)]] QuoteCancel {
  trader_id_t   trader_id;
  security_id_t security_id;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<QuoteCancel>() == 18);

struct [[=rbe::pack_le, =rbe::id(message_type_t::trade)]] Trade {
  security_id_t security_id;
  trade_type_t  trade_type;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;    ///< Zero unless trade_type == visible.
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;   ///< See spec §3.6.1.
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Trade>() == 41);

struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_report)]] TradeReport {
  security_id_t security_id;
  trade_type_t  trade_type;
  std::uint32_t quantity;
  price_t       price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
  timestamp_t   transact_time;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeReport>() == 45);

/// Emitted after a TradeReportCancel of the original report (spec §3.4.8).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_report_modify)]] TradeReportModify {
  security_id_t security_id;
  trade_type_t  trade_type;
  std::uint32_t quantity;
  price_t       price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
  timestamp_t   transact_time;
  trade_ref_t   orig_trade_ref;
  timestamp_t   orig_timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeReportModify>() == 57);

struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_report_cancel)]] TradeReportCancel {
  security_id_t orig_security_id;
  trade_type_t  orig_trade_type;
  std::uint32_t orig_quantity;
  price_t       orig_price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
  timestamp_t   transact_time;
  trade_ref_t   orig_trade_ref;
  timestamp_t   orig_timestamp;
  timestamp_t   orig_transact_time;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeReportCancel>() == 65);

/// Applies only to order-generated trades (spec §3.4.10).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_bust)]] TradeBust {
  security_id_t security_id;
  std::uint32_t quantity;
  price_t       price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeBust>() == 36);

/// Disseminates opening and closing prices.
struct [[=rbe::pack_le, =rbe::id(message_type_t::security_statistics)]] SecurityStatistics {
  security_id_t security_id;
  price_t       price;
  price_type_t  price_type;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SecurityStatistics>() == 25);

/// Pre-market identification of quote originators.
struct [[=rbe::pack_le, =rbe::id(message_type_t::trader_definition)]] TraderDefinition {
  trader_id_t trader_id;
  sender_id_t sender_id;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TraderDefinition>() == 24);

/// Pre-market dynamic tick tables / static ticks driving price increments.
struct [[=rbe::pack_le, =rbe::id(message_type_t::tick_table_data)]] TickTableData {
  std::uint8_t tick_table_id;
  name_t       name;
  price_t      threshold;
  price_t      tick_size;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TickTableData>() == 33);

/// Pre-market security reference data. Occasionally emitted intraday
/// when a correction is required.
struct [[=rbe::pack_le, =rbe::id(message_type_t::security_definition)]] SecurityDefinition {
  security_id_t security_id;
  umtf_t        umtf;         ///< Not applicable to AQSE.
  isin_t        isin;
  currency_t    currency;
  mic_t         mic;
  std::uint8_t  tick_table_id;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SecurityDefinition>() == 34);

/// Published when the trading status of a security or its parent market changes.
/// market_flags bit layout (spec §3.4.15):
///   bit 0    Trading: 0 = CT closed / AoD off, 1 = CT open / AoD on
///   bits 1-2 Reserved
///   bit 3    Pre-open/close: 0 = not a pre phase, 1 = pre phase
///   bits 4-7 Reserved
struct [[=rbe::pack_le, =rbe::id(message_type_t::security_status)]] SecurityStatus {
  security_id_t    security_id;
  trading_status_t trading_status;
  std::uint8_t     market_flags;
  timestamp_t      timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SecurityStatus>() == 18);

// ─────────────────────────────────────────────────────────────────────
// Auction On Demand (AoD) feed messages (spec §3.5)
// ─────────────────────────────────────────────────────────────────────

/// First AoDUpdate signals the start of an auction. Subsequent updates
/// publish the indicative price and matched volume.
struct [[=rbe::pack_le, =rbe::id(message_type_t::aod_update)]] AoDUpdate {
  security_id_t security_id;
  price_t       indicative_price;
  std::uint32_t match_vol;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AoDUpdate>() == 28);

// AoD Trade and AoD Trade Bust reuse the Trade / TradeBust structures
// from §3.4 (spec §3.5.2, §3.5.3).

// ─────────────────────────────────────────────────────────────────────
// Snapshot feed messages (spec §3.7)
// ─────────────────────────────────────────────────────────────────────

/// First message of each snapshot; identifies the continuous-stream
/// seq_no that this snapshot corresponds to.
struct [[=rbe::pack_le, =rbe::id(message_type_t::snapshot_start)]] SnapshotStart {
  std::uint32_t stream_seq_no;
  std::uint16_t security_count;
  timestamp_t   timestamp;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SnapshotStart>() == 20);

/// Book status for one security within a snapshot. Followed by `entries`
/// BookEntry messages.
struct [[=rbe::pack_le, =rbe::id(message_type_t::book_status)]] BookStatus {
  security_id_t    security_id;
  trading_status_t trading_status;
  std::uint8_t     market_flags;
  std::uint16_t    entries;
  std::uint32_t    closing_buy_qty;
  std::uint32_t    closing_sell_qty;
  price_t          indicative_price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<BookStatus>() == 28);

/// One open order in the book snapshot. Published in price-time priority
/// per side.
struct [[=rbe::pack_le, =rbe::id(message_type_t::book_entry)]] BookEntry {
  security_id_t security_id;
  side_t        side;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<BookEntry>() == 25);

// ─────────────────────────────────────────────────────────────────────
// Replay service messages — TCP/IP (spec §3.8)
// ─────────────────────────────────────────────────────────────────────
// seq_no in the header is ignored on the replay channel.

struct [[=rbe::pack_le, =rbe::id(message_type_t::login)]] Login {
  username_t  username;
  password_t  password;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Login>() == 26);

struct [[=rbe::pack_le, =rbe::id(message_type_t::replay_request)]] ReplayRequest {
  std::uint32_t begin_seq_no;
  std::uint32_t end_seq_no;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ReplayRequest>() == 14);

struct [[=rbe::pack_le, =rbe::id(message_type_t::replay_response)]] ReplayResponse {
  response_code_t response_code;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ReplayResponse>() == 7);

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Framing
// ─────────────────────────────────────────────────────────────────────


using messages = rbe::any<
    Heartbeat, OrderAdd, OrderCancel, OrderModify, QuoteAddReplace, QuoteCancel, Trade, TradeReport, TradeReportModify,
    TradeReportCancel, TradeBust, SecurityStatistics, TraderDefinition, TickTableData, SecurityDefinition,
    SecurityStatus, AoDUpdate, SnapshotStart, BookStatus, BookEntry, Login, ReplayRequest, ReplayResponse>;

/// One market data message: `Header` followed by the message selected by `msg_type`.
using message = rbe::frame<Header, messages>;

/// One multicast UDP payload: `PacketHeader` followed by `count` back-to-back messages.
/// `PacketHeader` carries no length, so the packet runs to the end of the datagram.
using packet = rbe::frame<PacketHeader, rbe::many<message>>;

} // namespace aquis
