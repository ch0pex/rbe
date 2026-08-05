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
 * It is expressed as a nested `Header` field so that `rbe::id` and
 * `rbe::length` are declared once and dispatch/framing work through
 * introspection of the nested struct.
 */
#pragma once

#include <rbe/core/annotations.hpp>
#include <rbe/core/custom.hpp> // for rbe::string<N> (see note below)

#include <cstdint>
#include <tuple>

// NOTE: `rbe::string<N>` is documented in the design overview as a
// library-provided fixed-length text type but is not implemented yet;
// this file assumes it will land. Until it does, the messages that
// reference it (Trader/Security/Tick-Table Definitions, Login) will
// not compile.

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
/// Each message struct embeds this as its first member and default-
/// initializes `msg_type` and `length` to its own compile-time values.
/// The `rbe::id` and `rbe::length` annotations live here so they are
/// declared exactly once for the whole protocol.
struct[[= rbe::pack_le]] Header {
  [[= rbe::id]] message_type_t msg_type {};
  [[= rbe::length]] std::uint8_t length {};
  std::uint32_t seq_no {};
};

// ─────────────────────────────────────────────────────────────────────
// Continuous data feed messages (spec §3.3–§3.4)
// ─────────────────────────────────────────────────────────────────────

// clang-format off

struct [[=rbe::pack_le]] Heartbeat {
  Header header {.msg_type = message_type_t::heartbeat, .length = 6};
};

struct [[=rbe::pack_le]] OrderAdd {
  Header        header {.msg_type = message_type_t::order_add, .length = 33};
  security_id_t security_id;
  side_t        side;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};

struct [[=rbe::pack_le]] OrderCancel {
  Header        header {.msg_type = message_type_t::order_cancel, .length = 20};
  security_id_t security_id;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};

struct [[=rbe::pack_le]] OrderModify {
  Header        header {.msg_type = message_type_t::order_modify, .length = 32};
  security_id_t security_id;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
  timestamp_t   timestamp;
};

struct [[=rbe::pack_le]] QuoteCancel {
  Header        header {.msg_type = message_type_t::quote_cancel, .length = 18};
  trader_id_t   trader_id;
  security_id_t security_id;
  timestamp_t   timestamp;
};

struct [[=rbe::pack_le]] Trade {
  Header        header {.msg_type = message_type_t::trade, .length = 41};
  security_id_t security_id;
  trade_type_t  trade_type;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;    ///< Zero unless trade_type == visible.
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;   ///< See spec §3.6.1.
};

struct [[=rbe::pack_le]] TradeReport {
  Header        header {.msg_type = message_type_t::trade_report, .length = 45};
  security_id_t security_id;
  trade_type_t  trade_type;
  std::uint32_t quantity;
  price_t       price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
  timestamp_t   transact_time;
};

/// Emitted after a TradeReportCancel of the original report (spec §3.4.8).
struct [[=rbe::pack_le]] TradeReportModify {
  Header        header {.msg_type = message_type_t::trade_report_modify, .length = 57};
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

struct [[=rbe::pack_le]] TradeReportCancel {
  Header        header {.msg_type = message_type_t::trade_report_cancel, .length = 65};
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

/// Applies only to order-generated trades (spec §3.4.10).
struct [[=rbe::pack_le]] TradeBust {
  Header        header {.msg_type = message_type_t::trade_bust, .length = 36};
  security_id_t security_id;
  std::uint32_t quantity;
  price_t       price;
  trade_ref_t   trade_ref;
  timestamp_t   timestamp;
  std::uint32_t binary_mmt;
};

/// Disseminates opening and closing prices.
struct [[=rbe::pack_le]] SecurityStatistics {
  Header        header {.msg_type = message_type_t::security_statistics, .length = 25};
  security_id_t security_id;
  price_t       price;
  price_type_t  price_type;
  timestamp_t   timestamp;
};

/// Pre-market identification of quote originators.
struct [[=rbe::pack_le]] TraderDefinition {
  Header      header {.msg_type = message_type_t::trader_definition, .length = 24};
  trader_id_t trader_id;
  sender_id_t sender_id;
};

/// Pre-market dynamic tick tables / static ticks driving price increments.
struct [[=rbe::pack_le]] TickTableData {
  Header       header {.msg_type = message_type_t::tick_table_data, .length = 33};
  std::uint8_t tick_table_id;
  name_t       name;
  price_t      threshold;
  price_t      tick_size;
};

/// Pre-market security reference data. Occasionally emitted intraday
/// when a correction is required.
struct [[=rbe::pack_le]] SecurityDefinition {
  Header        header {.msg_type = message_type_t::security_definition, .length = 34};
  security_id_t security_id;
  umtf_t        umtf;         ///< Not applicable to AQSE.
  isin_t        isin;
  currency_t    currency;
  mic_t         mic;
  std::uint8_t  tick_table_id;
};

/// Published when the trading status of a security or its parent market changes.
/// market_flags bit layout (spec §3.4.15):
///   bit 0    Trading: 0 = CT closed / AoD off, 1 = CT open / AoD on
///   bits 1-2 Reserved
///   bit 3    Pre-open/close: 0 = not a pre phase, 1 = pre phase
///   bits 4-7 Reserved
struct [[=rbe::pack_le]] SecurityStatus {
  Header           header {.msg_type = message_type_t::security_status, .length = 18};
  security_id_t    security_id;
  trading_status_t trading_status;
  std::uint8_t     market_flags;
  timestamp_t      timestamp;
};

// ─────────────────────────────────────────────────────────────────────
// Auction On Demand (AoD) feed messages (spec §3.5)
// ─────────────────────────────────────────────────────────────────────

/// First AoDUpdate signals the start of an auction. Subsequent updates
/// publish the indicative price and matched volume.
struct [[=rbe::pack_le]] AoDUpdate {
  Header        header {.msg_type = message_type_t::aod_update, .length = 28};
  security_id_t security_id;
  price_t       indicative_price;
  std::uint32_t match_vol;
  timestamp_t   timestamp;
};

// AoD Trade and AoD Trade Bust reuse the Trade / TradeBust structures
// from §3.4 (spec §3.5.2, §3.5.3).

// ─────────────────────────────────────────────────────────────────────
// Snapshot feed messages (spec §3.7)
// ─────────────────────────────────────────────────────────────────────

/// First message of each snapshot; identifies the continuous-stream
/// seq_no that this snapshot corresponds to.
struct [[=rbe::pack_le]] SnapshotStart {
  Header        header {.msg_type = message_type_t::snapshot_start, .length = 20};
  std::uint32_t stream_seq_no;
  std::uint16_t security_count;
  timestamp_t   timestamp;
};

/// Book status for one security within a snapshot. Followed by `entries`
/// BookEntry messages.
struct [[=rbe::pack_le]] BookStatus {
  Header           header {.msg_type = message_type_t::book_status, .length = 28};
  security_id_t    security_id;
  trading_status_t trading_status;
  std::uint8_t     market_flags;
  std::uint16_t    entries;
  std::uint32_t    closing_buy_qty;
  std::uint32_t    closing_sell_qty;
  price_t          indicative_price;
};

/// One open order in the book snapshot. Published in price-time priority
/// per side.
struct [[=rbe::pack_le]] BookEntry {
  Header        header {.msg_type = message_type_t::book_entry, .length = 25};
  security_id_t security_id;
  side_t        side;
  std::uint32_t quantity;
  price_t       price;
  order_ref_t   order_ref;
};

// ─────────────────────────────────────────────────────────────────────
// Replay service messages — TCP/IP (spec §3.8)
// ─────────────────────────────────────────────────────────────────────
// seq_no in the header is ignored on the replay channel.

struct [[=rbe::pack_le]] Login {
  Header      header {.msg_type = message_type_t::login, .length = 26};
  username_t  username;
  password_t  password;
};

struct [[=rbe::pack_le]] ReplayRequest {
  Header        header {.msg_type = message_type_t::replay_request, .length = 14};
  std::uint32_t begin_seq_no;
  std::uint32_t end_seq_no;
};

struct [[=rbe::pack_le]] ReplayResponse {
  Header          header {.msg_type = message_type_t::replay_response, .length = 7};
  response_code_t response_code;
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<aquis::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = std::tuple<
    Heartbeat, OrderAdd, OrderCancel, OrderModify, QuoteAddReplace, QuoteCancel, Trade, TradeReport, TradeReportModify,
    TradeReportCancel, TradeBust, SecurityStatistics, TraderDefinition, TickTableData, SecurityDefinition,
    SecurityStatus, AoDUpdate, SnapshotStart, BookStatus, BookEntry, Login, ReplayRequest, ReplayResponse>;

} // namespace aquis
