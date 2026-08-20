/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file cboe.hpp
 * @version 1.0
 * @date 05/08/2026
 * @brief Cboe market data protocols: European Multicast PITCH and US TOP.
 *
 * Two distinct protocols are exposed through nested namespaces:
 *
 *  - `cboe::pitch` — Cboe Titanium Europe Multicast PITCH v6.67
 *    (2 April 2026). Binary, little-endian, delivered over UDP with
 *    each frame prefixed by a Sequenced Unit Header wrapping one or
 *    more back-to-back sequenced messages. Every message starts with
 *    a 2-byte header: `length` (byte 0) then `msg_type` (byte 1).
 *
 *  - `cboe::top` — Cboe US Equities TOP v1.3.6 (6 September 2024).
 *    Fixed-length, line-oriented ASCII messages delivered over TCP.
 *    Each message begins with a 1-byte ASCII message type and is
 *    terminated by an LF (0x0A). There is no on-wire length field —
 *    the wire size is fixed per message type.
 *
 * Each message struct embeds a protocol-specific `Header` as its
 * first member so `rbe::id` (and `rbe::length` where applicable) are
 * declared once per protocol.
 */
#pragma once

#include <rbe/rbe.hpp>

#include <cstdint>
#include <tuple>

namespace cboe {

// =====================================================================
// Cboe Titanium Europe — Multicast PITCH v6.67 (spec §4)
// =====================================================================

namespace pitch {

// ─────────────────────────────────────────────────────────────────────
// Semantic type aliases (spec §2.2)
// ─────────────────────────────────────────────────────────────────────

/// Unsigned little-endian 8-byte price with 4 implied decimal places on
/// BXE/CXE (denominator 10 000) or 6 implied decimal places on TRF
/// (denominator 1 000 000). Spec §2.2 "Binary Long Price".
using long_price_t = std::uint64_t;

/// Unsigned little-endian 2-byte price with 2 implied decimal places
/// (denominator 100). Spec §2.2 "Binary Short Price".
using short_price_t = std::uint16_t;

/// Nanosecond offset from the most recent Time message for the unit.
using time_offset_t = std::uint32_t;

/// Whole seconds since midnight London time (spec §4.1).
using time_t = std::uint32_t;

/// Elapsed nanoseconds since the Unix epoch (spec §4.9.4).
using timestamp_t = std::uint64_t;

using order_id_t     = std::uint64_t; ///< Day-specific order identifier.
using execution_id_t = std::uint64_t; ///< Cboe day-unique execution id (12-char base-36 packed in 8 bytes; spec §2.5).
using trade_id_t     = std::uint64_t; ///< Cboe trade identifier, unique for ≥ 7 calendar days (spec §4.9.4).
using sequence_t     = std::uint32_t;

// ─────────────────────────────────────────────────────────────────────
// Fixed-width text fields (left-justified ASCII, space padded)
// ─────────────────────────────────────────────────────────────────────

using symbol_t         = std::array<char, 8>; ///< Standard PITCH symbol, spec §4.3.1.
using symbol_short_t   = std::array<char, 6>; ///< Symbol carried by Short-form messages, spec §4.3.2.
using isin_t           = std::array<char, 12>; ///< 12-char ISIN used by Trade – Unknown Symbol, spec §4.9.5.
using currency_t       = std::array<char, 3>; ///< ISO 4217 currency, spec §4.9.4.
using mic_t            = std::array<char, 4>; ///< Execution venue MIC, spec §4.9.4.
using participant_id_t = std::array<char, 4>; ///< Systematic Internaliser attribution, spec §4.3.3.
using index_ticker_t   = std::array<char, 10>; ///< Index ticker code, spec §4.16.1.

using execution_flags_t      = std::array<char, 4>; ///< 4-char MMT flags on Order Executed messages, spec §4.4.1.
using trade_flags_t          = std::array<char, 5>; ///< 5-char MMT flags on non-Extended Trade messages, spec §4.9.3.
using extended_trade_flags_t = std::array<char, 14>; ///< 14-char MMT flags on Trade – Extended, spec §4.9.6.

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

/// PITCH 2.X, GRP and Spin Server message identifiers (spec Appendix A).
/// Note that GRP and Spin Server share `login` (0x01) and `login_response`
/// (0x02); they are dispatched on distinct TCP channels.
enum class message_type_t : std::uint8_t {
  // Gap Request Proxy / Spin Server (spec §3, §5)
  login                = 0x01,
  login_response       = 0x02,
  gap_request          = 0x03,
  gap_response         = 0x04,
  spin_image_available = 0x80,
  spin_request         = 0x81,
  spin_response        = 0x82,
  spin_finished        = 0x83,

  // PITCH 2.X market data (spec §4)
  time                         = 0x20,
  unit_clear                   = 0x97,
  add_order_long               = 0x40,
  add_order_short              = 0x22,
  add_order_expanded           = 0x2F,
  order_executed               = 0x23,
  order_executed_at_price_size = 0x24,
  reduce_size_long             = 0x25,
  reduce_size_short            = 0x26,
  modify_order_long            = 0x27,
  modify_order_short           = 0x28,
  delete_order                 = 0x29,
  trade_long                   = 0x41,
  trade_short                  = 0x2B,
  trade_break                  = 0x2C,
  end_of_session               = 0x2D,
  trading_status               = 0x31,
  trade_extended               = 0x32,
  trade_unknown_symbol         = 0x35,
  statistics                   = 0x34,
  transaction_begin            = 0xBC,
  transaction_end              = 0xBD,
  auction_summary              = 0x96,
  auction_update               = 0xAC,
  index_quote                  = 0xD8,
  index_quote_edsp             = 0xD9,
};

/// Buy/Sell side indicator (ASCII, spec §4.3).
enum class side_t : std::uint8_t {
  buy  = 'B',
  sell = 'S',
};

/// Login/Spin login status (spec §3.2, §5.2).
enum class login_status_t : std::uint8_t {
  accepted        = 'A',
  not_authorised  = 'N',
  session_in_use  = 'B',
  invalid_session = 'S',
};

/// Gap request response status (spec §3.4).
enum class gap_status_t : std::uint8_t {
  accepted               = 'A',
  out_of_range           = 'O',
  daily_limit_exhausted  = 'D',
  minute_limit_exhausted = 'M',
  second_limit_exhausted = 'S',
  count_limit_exceeded   = 'C',
  invalid_unit           = 'I',
};

/// Spin Response status (spec §5.5).
enum class spin_status_t : std::uint8_t {
  accepted         = 'A',
  out_of_range     = 'O',
  spin_in_progress = 'S',
};

/// Trading status for a security (spec §4.13).
enum class trading_status_code_t : std::uint8_t {
  trading                    = 'T',
  off_book_reporting         = 'R',
  closed                     = 'C',
  suspension                 = 'S',
  no_reference_price         = 'N',
  volatility_interruption    = 'V', ///< Static collar.
  volatility_auction         = 'I',
  opening_auction            = 'O',
  closing_auction            = 'E',
  halt                       = 'H', ///< Reserved for future use.
  market_order_imbalance_ext = 'M',
  price_monitoring_extension = 'P',
  closing_cross              = 'U',
  halt_auction               = 'Y',
};

/// Auction type (spec §4.15).
enum class auction_type_t : std::uint8_t {
  opening       = 'O',
  closing       = 'C',
  halt          = 'H',
  volatility    = 'V',
  periodic      = 'P',
  closing_cross = 'U',
};

/// Auction Update — tolerance vs Cboe EBBO collar (spec §4.15.1).
enum class outside_tolerance_t : std::uint8_t {
  outside       = 'O',
  inside        = 'I',
  not_specified = '-',
};

/// Auction Update — whether primary-market quotes are in the EBBO used
/// to collar this update (spec §4.15.1).
enum class includes_primary_t : std::uint8_t {
  includes      = 'P',
  excludes      = 'N',
  not_specified = '-',
};

/// Statistics Message price kind (spec §4.14).
enum class statistic_type_t : std::uint8_t {
  closing          = 'C',
  high             = 'H',
  low              = 'L',
  opening          = 'O',
  previous_closing = 'P',
};

/// Statistics Message price provenance (spec §4.14).
enum class price_determination_t : std::uint8_t {
  normal = '0',
  manual = '1', ///< Adjusted by market supervision.
};

/// Index Quote status (spec §4.16.1).
enum class index_status_t : std::uint8_t {
  normal     = 'N',
  indicative = 'I',
  closing    = 'C',
};

// clang-format off

// ─────────────────────────────────────────────────────────────────────
// Cboe Sequenced Unit Header (spec §2.4)
// ─────────────────────────────────────────────────────────────────────

/// 8-byte UDP frame prefix that wraps `count` sequenced or unsequenced
/// PITCH messages. A `count` of zero indicates a heartbeat frame.
struct[[=rbe::pack_le]] SequencedUnitHeader {
  std::uint16_t length; ///< Length of entire block including this header.
  std::uint8_t count; ///< Number of messages that follow.
  std::uint8_t unit; ///< Unit that applies to the enclosed messages.
  sequence_t sequence; ///< Sequence of the first enclosed sequenced message.
};

// ─────────────────────────────────────────────────────────────────────
// Common PITCH message header (spec §2.1)
// ─────────────────────────────────────────────────────────────────────

/// 2-byte header prefix of every PITCH / GRP / Spin message. Cboe puts
/// `length` FIRST (offset 0), then the message type (offset 1). Each
/// message struct embeds this as its first member and defaults both
/// fields to its compile-time values; `rbe::id` and `rbe::length` are
/// declared exactly once for the whole protocol here.
struct[[=rbe::pack_le]] Header {
  [[= rbe::length]] std::uint8_t length {};
  [[= rbe::id]] message_type_t msg_type {};
};

// ─────────────────────────────────────────────────────────────────────
// Gap Request Proxy messages — TCP (spec §3)
// ─────────────────────────────────────────────────────────────────────


/// GRP / Spin Server login (spec §3.1, §5.1).
struct [[=rbe::pack_le]] Login {
  Header          header {.length = 22, .msg_type = message_type_t::login};
  std::array<char,4>  session_sub_id;
  std::array<char,4>  username;
  std::array<char,2>  filler;        ///< Space filled.
  std::array<char,10> password;
};

/// Response to a Login (spec §3.2, §5.2).
struct [[=rbe::pack_le]] LoginResponse {
  Header         header {.length = 3, .msg_type = message_type_t::login_response};
  login_status_t status;
};

/// Request retransmission of a sequenced range (spec §3.3).
struct [[=rbe::pack_le]] GapRequest {
  Header        header {.length = 9, .msg_type = message_type_t::gap_request};
  std::uint8_t  unit;
  sequence_t    sequence; ///< Lowest sequence in the requested range.
  std::uint16_t count;
};

/// Reply to a GapRequest (spec §3.4).
struct [[=rbe::pack_le]] GapResponse {
  Header        header {.length = 10, .msg_type = message_type_t::gap_response};
  std::uint8_t  unit;
  sequence_t    sequence;
  std::uint16_t count;
  gap_status_t  status;
};

// ─────────────────────────────────────────────────────────────────────
// Spin Server messages — TCP (spec §5)
// ─────────────────────────────────────────────────────────────────────

/// Advertises the highest sequence for which a spin is currently available (spec §5.3).
struct [[=rbe::pack_le]] SpinImageAvailable {
  Header     header {.length = 6, .msg_type = message_type_t::spin_image_available};
  sequence_t sequence;
};

/// Request a spin at a previously advertised sequence (spec §5.4).
struct [[=rbe::pack_le]] SpinRequest {
  Header     header {.length = 6, .msg_type = message_type_t::spin_request};
  sequence_t sequence;
};

/// Response to a SpinRequest (spec §5.5).
struct [[=rbe::pack_le]] SpinResponse {
  Header        header {.length = 11, .msg_type = message_type_t::spin_response};
  sequence_t    sequence;
  std::uint32_t order_count; ///< Number of Add Order messages that will follow. 0 on reject.
  spin_status_t status;
};

/// End-of-spin marker; not sent if the SpinRequest was rejected (spec §5.6).
struct [[=rbe::pack_le]] SpinFinished {
  Header     header {.length = 6, .msg_type = message_type_t::spin_finished};
  sequence_t sequence;
};

// ─────────────────────────────────────────────────────────────────────
// PITCH 2.X market data messages (spec §4)
// ─────────────────────────────────────────────────────────────────────

/// Whole-second timestamp base for the unit (spec §4.1).
struct [[=rbe::pack_le]] Time {
  Header header {.length = 6, .msg_type = message_type_t::time};
  time_t time;
};

/// Instructs feed recipients to clear all orders for the Cboe book of
/// the enclosing Sequenced Unit (spec §4.2).
struct [[=rbe::pack_le]] UnitClear {
  Header        header {.length = 6, .msg_type = message_type_t::unit_clear};
  time_offset_t time_offset;
};

/// Newly accepted visible order — long form (spec §4.3.1).
struct [[=rbe::pack_le]] AddOrderLong {
  Header        header {.length = 35, .msg_type = message_type_t::add_order_long};
  time_offset_t time_offset;
  order_id_t    order_id;
  side_t        side;
  std::uint32_t quantity;
  symbol_t      symbol;
  long_price_t  price;
};

/// Newly accepted visible order — short form (spec §4.3.2).
struct [[=rbe::pack_le]] AddOrderShort {
  Header         header {.length = 25, .msg_type = message_type_t::add_order_short};
  time_offset_t  time_offset;
  order_id_t     order_id;
  side_t         side;
  std::uint16_t  quantity;
  symbol_short_t symbol;
  short_price_t  price;
};

/// Newly accepted quote/order carrying attribution — used on the Cboe
/// Systematic Internaliser platform (spec §4.3.3).
struct [[=rbe::pack_le]] AddOrderExpanded {
  Header           header {.length = 40, .msg_type = message_type_t::add_order_expanded};
  time_offset_t    time_offset;
  order_id_t       order_id;
  side_t           side;
  std::uint32_t    quantity;
  symbol_t         symbol;
  long_price_t     price;
  std::uint8_t     add_flags; ///< Bit 1 = SI Quote; bits 0, 2-7 reserved.
  participant_id_t participant_id;
};

/// Visible order executed at its resting price (spec §4.4).
struct [[=rbe::pack_le]] OrderExecuted {
  Header             header {.length = 30, .msg_type = message_type_t::order_executed};
  time_offset_t      time_offset;
  order_id_t         order_id;
  std::uint32_t      executed_shares;
  execution_id_t     execution_id;
  execution_flags_t  execution_flags;
};

/// Visible order executed at a price different from the resting price (spec §4.5).
struct [[=rbe::pack_le]] OrderExecutedAtPriceSize {
  Header            header {.length = 42, .msg_type = message_type_t::order_executed_at_price_size};
  time_offset_t     time_offset;
  order_id_t        order_id;
  std::uint32_t     executed_shares;
  std::uint32_t     remaining_shares; ///< 0 → order fully removed from the book.
  execution_id_t    execution_id;
  long_price_t      price;
  execution_flags_t execution_flags;
};

/// Partial visible-order cancel — long form (spec §4.6.1).
struct [[=rbe::pack_le]] ReduceSizeLong {
  Header        header {.length = 18, .msg_type = message_type_t::reduce_size_long};
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint32_t cancelled_shares;
};

/// Partial visible-order cancel — short form (spec §4.6.2).
struct [[=rbe::pack_le]] ReduceSizeShort {
  Header        header {.length = 16, .msg_type = message_type_t::reduce_size_short};
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint16_t cancelled_shares;
};

/// Visible order modification — long form (spec §4.7.1).
struct [[=rbe::pack_le]] ModifyOrderLong {
  Header        header {.length = 26, .msg_type = message_type_t::modify_order_long};
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint32_t shares;
  long_price_t  price;
};

/// Visible order modification — short form (spec §4.7.2).
struct [[=rbe::pack_le]] ModifyOrderShort {
  Header        header {.length = 18, .msg_type = message_type_t::modify_order_short};
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint16_t shares;
  short_price_t price;
};

/// Complete visible-order cancel (spec §4.8).
struct [[=rbe::pack_le]] DeleteOrder {
  Header        header {.length = 14, .msg_type = message_type_t::delete_order};
  time_offset_t time_offset;
  order_id_t    order_id;
};

/// Hidden-order or routed execution — long form (spec §4.9.1).
struct [[=rbe::pack_le]] TradeLong {
  Header         header {.length = 48, .msg_type = message_type_t::trade_long};
  time_offset_t  time_offset;
  order_id_t     order_id; ///< Obfuscated by default (spec §4.9).
  side_t         side;     ///< Always 'B' for hidden trades.
  std::uint32_t  shares;
  symbol_t       symbol;
  long_price_t   price;
  execution_id_t execution_id;
  trade_flags_t  trade_flags;
};

/// Hidden-order or routed execution — short form (spec §4.9.2).
struct [[=rbe::pack_le]] TradeShort {
  Header         header {.length = 38, .msg_type = message_type_t::trade_short};
  time_offset_t  time_offset;
  order_id_t     order_id;
  side_t         side;
  std::uint16_t  shares;
  symbol_short_t symbol;
  short_price_t  price;
  execution_id_t execution_id;
  trade_flags_t  trade_flags;
};

/// Extended trade details, only used on the Cboe European platform (spec §4.9.4).
struct [[=rbe::pack_le]] TradeExtended {
  Header                    header {.length = 68, .msg_type = message_type_t::trade_extended};
  time_offset_t             time_offset;
  std::uint64_t             shares;
  symbol_t                  symbol;
  long_price_t              price;         ///< 0 if price pending (Level 3.8 = N).
  trade_id_t                trade_id;
  timestamp_t               trade_timestamp;
  mic_t                     execution_venue;
  currency_t                currency;
  std::uint8_t              cboe_trade_flags;    ///< 1-char alphanumeric, see spec §4.9.7.
  extended_trade_flags_t    extended_trade_flags;
};

/// Trade reported on an ISIN not known to Cboe — TRF only (spec §4.9.5).
struct [[=rbe::pack_le]] TradeUnknownSymbol {
  Header                 header {.length = 72, .msg_type = message_type_t::trade_unknown_symbol};
  time_offset_t          time_offset;
  std::uint64_t          shares;
  isin_t                 symbol;   ///< ISIN in place of local symbol.
  long_price_t           price;
  trade_id_t             trade_id;
  timestamp_t            trade_timestamp;
  mic_t                  execution_venue;
  currency_t             currency;
  std::uint8_t           cboe_trade_flags;
  extended_trade_flags_t extended_trade_flags;
};

/// Break of an order-generated trade — carries only the execution id
/// of the broken trade (spec Appendix A / Appendix B, Trade Break).
struct [[=rbe::pack_le]] TradeBreak {
  Header         header {.length = 14, .msg_type = message_type_t::trade_break};
  time_offset_t  time_offset;
  execution_id_t execution_id;
};

/// End-of-session marker for the unit (spec §4.10).
struct [[=rbe::pack_le]] EndOfSession {
  Header        header {.length = 6, .msg_type = message_type_t::end_of_session};
  time_offset_t time_offset;
};

/// Start-of-transaction marker; subsequent messages up to the matching
/// TransactionEnd belong to the same transaction block (spec §4.11).
struct [[=rbe::pack_le]] TransactionBegin {
  Header        header {.length = 6, .msg_type = message_type_t::transaction_begin};
  time_offset_t time_offset;
};

/// End-of-transaction marker (spec §4.12).
struct [[=rbe::pack_le]] TransactionEnd {
  Header        header {.length = 6, .msg_type = message_type_t::transaction_end};
  time_offset_t time_offset;
};

/// Change in trading status for a security (spec §4.13).
struct [[=rbe::pack_le]] TradingStatus {
  Header                header {.length = 18, .msg_type = message_type_t::trading_status};
  time_offset_t         time_offset;
  symbol_t              symbol;
  trading_status_code_t status;
  std::array<char,3>        reserved;
};

/// Disseminates opening / closing / high / low statistics prices — Cboe
/// European platform only (spec §4.14).
struct [[=rbe::pack_le]] Statistics {
  Header                header {.length = 24, .msg_type = message_type_t::statistics};
  time_offset_t         time_offset;
  symbol_t              symbol;
  long_price_t          price;
  statistic_type_t      statistic_type;
  price_determination_t price_determination;
};

/// Indicative price / size during a call or extension phase (spec §4.15.1).
struct [[=rbe::pack_le]] AuctionUpdate {
  Header              header {.length = 37, .msg_type = message_type_t::auction_update};
  time_offset_t       time_offset;
  symbol_t            symbol;
  auction_type_t      auction_type;
  long_price_t        reference_price;
  long_price_t        indicative_price;
  std::uint32_t       indicative_shares;
  outside_tolerance_t outside_tolerance;
  includes_primary_t  includes_primary;
};

/// Post-uncross auction result (spec §4.15.2).
struct [[=rbe::pack_le]] AuctionSummary {
  Header         header {.length = 27, .msg_type = message_type_t::auction_summary};
  time_offset_t  time_offset;
  symbol_t       symbol;
  auction_type_t auction_type;
  long_price_t   price;
  std::uint32_t  shares;
};

/// Real-time index quote (spec §4.16.1) — XIC/XID/XIE feeds only.
struct [[=rbe::pack_le]] IndexQuote {
  Header         header {.length = 29, .msg_type = message_type_t::index_quote};
  std::uint64_t  timestamp;    ///< Nanoseconds since midnight.
  index_ticker_t index_ticker;
  long_price_t   price;
  index_status_t index_status;
};

/// End-of-day exchange delivery settlement price for an index (spec §4.16.2).
struct [[=rbe::pack_le]] IndexQuoteEDSP {
  Header         header {.length = 28, .msg_type = message_type_t::index_quote_edsp};
  std::uint64_t  timestamp;
  index_ticker_t index_ticker;
  long_price_t   price;
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<cboe::pitch::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = std::tuple<
    Login, LoginResponse, GapRequest, GapResponse, SpinImageAvailable, SpinRequest, SpinResponse, SpinFinished, Time,
    UnitClear, AddOrderLong, AddOrderShort, AddOrderExpanded, OrderExecuted, OrderExecutedAtPriceSize, ReduceSizeLong,
    ReduceSizeShort, ModifyOrderLong, ModifyOrderShort, DeleteOrder, TradeLong, TradeShort, TradeExtended,
    TradeUnknownSymbol, TradeBreak, EndOfSession, TransactionBegin, TransactionEnd, TradingStatus, Statistics,
    AuctionUpdate, AuctionSummary, IndexQuote, IndexQuoteEDSP>;

} // namespace pitch

// =====================================================================
// Cboe US Equities — TOP v1.3.6 (spec §1–§10)
// =====================================================================
//
// TOP is a fixed-length, line-oriented ASCII protocol. Every message
// starts with a single-byte ASCII message type at offset 0 and ends
// with an LF (0x0A). There is no on-wire length field — the wire size
// is fixed per message type — so the shared `Header` here declares
// only `rbe::id` and each message carries an explicit terminating
// `newline` byte defaulted to '\n'.

namespace top {

// ─────────────────────────────────────────────────────────────────────
// ASCII text field aliases (spec §1.3)
// ─────────────────────────────────────────────────────────────────────

using timestamp_t    = std::array<char, 8>; ///< 8-digit ms past midnight, Eastern Time.
using seconds_t      = std::array<char, 5>; ///< 5-digit seconds past midnight.
using milliseconds_t = std::array<char, 3>; ///< 3-digit ms past last Seconds message.
using symbol_short_t = std::array<char, 4>; ///< 4-char symbol.
using symbol_long_t  = std::array<char, 6>; ///< 6-char symbol.
using symbol_wide_t  = std::array<char, 8>; ///< 8-char symbol (ISRA / expanded).

using price_short_t    = std::array<char, 5>; ///< 3+2 short-form price.
using price_long_t     = std::array<char, 10>; ///< 6+4 long-form price.
using price_extended_t = std::array<char, 14>; ///< 8+6 extended-form price.

using qty_short_t    = std::array<char, 5>; ///< Short-form quantity.
using qty_long_t     = std::array<char, 6>; ///< Long/expanded/extended quantity.
using volume_t       = std::array<char, 9>; ///< Cumulative volume (short trade uses 7).
using volume_short_t = std::array<char, 7>;

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

/// Every TOP message begins with one of these ASCII bytes (spec §4–§10).
enum class message_type_t : std::uint8_t {
  logon                     = 'L',
  logon_accepted            = 'C',
  logon_rejected            = 'J',
  expanded_spin             = 's',
  extended_spin             = 'S',
  spin_done                 = 'D',
  server_heartbeat          = 'H',
  client_heartbeat          = 'R',
  seconds                   = 'T',
  milliseconds              = 'M',
  extended_bid_update       = 'N',
  expanded_bid_update       = 'E',
  long_bid_update           = 'B',
  short_bid_update          = 'b',
  extended_ask_update       = 'n',
  expanded_ask_update       = 'e',
  long_ask_update           = 'A',
  short_ask_update          = 'a',
  expanded_two_sided_update = 'F',
  long_two_sided_update     = 'U',
  short_two_sided_update    = 'u',
  extended_two_sided_update = 'd',
  extended_trade            = 'r',
  expanded_trade            = 'f',
  long_trade                = 'V',
  short_trade               = 'v',
  trading_status            = 't',
};

/// Boolean flag encoding on the wire.
enum class boolean_t : std::uint8_t {
  yes = 'Y',
  no  = 'N',
};

/// Logon Rejected reason (spec §4.3).
enum class reject_reason_t : std::uint8_t {
  auth_problem = 'A',
};

/// Halt status for Spin (spec §5.1.1) — Trading or Halted.
enum class halt_status_spin_t : std::uint8_t {
  halted  = 'H',
  trading = 'T',
};

/// Halt status for Trading Status message (spec §10.1).
enum class halt_status_t : std::uint8_t {
  accepting_for_queuing = 'A',
  halted                = 'H',
  quote_only            = 'Q',
  suspension            = 'S',
  trading               = 'T',
};

/// Reg SHO short-sale price-test flag (spec §5.1.1, §10.1).
enum class reg_sho_action_t : std::uint8_t {
  no_test   = '0',
  in_effect = '1',
};

// ─────────────────────────────────────────────────────────────────────
// Common TOP message header
// ─────────────────────────────────────────────────────────────────────

/// TOP messages have no on-wire length field, only a 1-byte ASCII type.
/// Each message struct embeds this as its first member; `rbe::id`
/// dispatches on `msg_type` and the wire size is fixed per struct.
struct[[= rbe::pack_le]] Header {
  [[= rbe::id]] message_type_t msg_type {};
};

// ─────────────────────────────────────────────────────────────────────
// Session / logon messages (spec §4)
// ─────────────────────────────────────────────────────────────────────

// clang-format off

/// Client → server logon (spec §4.1).
struct [[=rbe::pack_le]] Logon {
  Header          header {.msg_type = message_type_t::logon};
  std::array<char,6>  username;
  std::array<char,10> password;
  boolean_t       spin_flag; ///< 'Y' → send a spin of current top of book.
  std::uint8_t    newline = '\n';
};

/// Server → client acceptance (spec §4.2).
struct [[=rbe::pack_le]] LogonAccepted {
  Header       header {.msg_type = message_type_t::logon_accepted};
  std::uint8_t newline = '\n';
};

/// Server → client rejection (spec §4.3).
struct [[=rbe::pack_le]] LogonRejected {
  Header          header {.msg_type = message_type_t::logon_rejected};
  reject_reason_t reject_reason;
  std::uint8_t    newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Spin messages (spec §5)
// ─────────────────────────────────────────────────────────────────────

/// Per-symbol snapshot delivered during a spin, expanded form for ISRA
/// symbol sizes (spec §5.1.1).
struct [[=rbe::pack_le]] ExpandedSpin {
  Header             header {.msg_type = message_type_t::expanded_spin};
  timestamp_t        timestamp;
  symbol_wide_t      symbol;
  price_long_t       bid_price;
  qty_long_t         bid_quantity;
  price_long_t       ask_price;
  qty_long_t         ask_quantity;
  timestamp_t        last_trade_time;
  price_long_t       last_trade_price;
  qty_long_t         last_trade_size;
  volume_t           cumulative_volume;
  halt_status_spin_t halt_status;
  reg_sho_action_t   reg_sho_action;
  std::uint8_t       reserved_1;
  std::uint8_t       reserved_2;
  std::uint8_t       newline = '\n';
};

/// Per-symbol snapshot with extended (14-char) prices (spec §5.1.2).
struct [[=rbe::pack_le]] ExtendedSpin {
  Header             header {.msg_type = message_type_t::extended_spin};
  timestamp_t        timestamp;
  symbol_wide_t      symbol;
  price_extended_t   bid_price;
  qty_long_t         bid_quantity;
  price_extended_t   ask_price;
  qty_long_t         ask_quantity;
  timestamp_t        last_trade_time;
  price_extended_t   last_trade_price;
  qty_long_t         last_trade_size;
  volume_t           cumulative_volume;
  halt_status_spin_t halt_status;
  reg_sho_action_t   reg_sho_action;
  std::uint8_t       reserved_1;
  std::uint8_t       reserved_2;
  std::uint8_t       newline = '\n';
};

/// End-of-spin marker (spec §5.2).
struct [[=rbe::pack_le]] SpinDone {
  Header       header {.msg_type = message_type_t::spin_done};
  std::uint8_t newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Heartbeat messages (spec §6)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le]] ServerHeartbeat {
  Header       header {.msg_type = message_type_t::server_heartbeat};
  std::uint8_t newline = '\n';
};

struct [[=rbe::pack_le]] ClientHeartbeat {
  Header       header {.msg_type = message_type_t::client_heartbeat};
  std::uint8_t newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Time update messages (spec §7)
// ─────────────────────────────────────────────────────────────────────

/// Seconds past midnight, Eastern (spec §7.1).
struct [[=rbe::pack_le]] Seconds {
  Header       header {.msg_type = message_type_t::seconds};
  seconds_t    seconds;
  std::uint8_t newline = '\n';
};

/// Milliseconds since the last Seconds message (spec §7.2).
struct [[=rbe::pack_le]] Milliseconds {
  Header         header {.msg_type = message_type_t::milliseconds};
  milliseconds_t milliseconds;
  std::uint8_t   newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Bid / Ask update messages (spec §8.1)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le]] ExtendedBidUpdate {
  Header           header {.msg_type = message_type_t::extended_bid_update};
  symbol_wide_t    symbol;
  price_extended_t bid_price;
  qty_long_t       bid_quantity;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le]] ExpandedBidUpdate {
  Header        header {.msg_type = message_type_t::expanded_bid_update};
  symbol_wide_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] LongBidUpdate {
  Header        header {.msg_type = message_type_t::long_bid_update};
  symbol_long_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] ShortBidUpdate {
  Header         header {.msg_type = message_type_t::short_bid_update};
  symbol_short_t symbol;
  price_short_t  bid_price;
  qty_short_t    bid_quantity;
  std::uint8_t   newline = '\n';
};

struct [[=rbe::pack_le]] ExtendedAskUpdate {
  Header           header {.msg_type = message_type_t::extended_ask_update};
  symbol_wide_t    symbol;
  price_extended_t ask_price;
  qty_long_t       ask_quantity;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le]] ExpandedAskUpdate {
  Header        header {.msg_type = message_type_t::expanded_ask_update};
  symbol_wide_t symbol;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] LongAskUpdate {
  Header        header {.msg_type = message_type_t::long_ask_update};
  symbol_long_t symbol;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] ShortAskUpdate {
  Header         header {.msg_type = message_type_t::short_ask_update};
  symbol_short_t symbol;
  price_short_t  ask_price;
  qty_short_t    ask_quantity;
  std::uint8_t   newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Two-sided update messages (spec §8.2)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le]] ExpandedTwoSidedUpdate {
  Header        header {.msg_type = message_type_t::expanded_two_sided_update};
  symbol_wide_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] LongTwoSidedUpdate {
  Header        header {.msg_type = message_type_t::long_two_sided_update};
  symbol_long_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] ShortTwoSidedUpdate {
  Header         header {.msg_type = message_type_t::short_two_sided_update};
  symbol_short_t symbol;
  price_short_t  bid_price;
  qty_short_t    bid_quantity;
  price_short_t  ask_price;
  qty_short_t    ask_quantity;
  std::uint8_t   newline = '\n';
};

struct [[=rbe::pack_le]] ExtendedTwoSidedUpdate {
  Header           header {.msg_type = message_type_t::extended_two_sided_update};
  symbol_wide_t    symbol;
  price_extended_t bid_price;
  qty_long_t       bid_quantity;
  price_extended_t ask_price;
  qty_long_t       ask_quantity;
  std::uint8_t     newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Trade messages (spec §9)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le]] ExtendedTrade {
  Header           header {.msg_type = message_type_t::extended_trade};
  symbol_wide_t    symbol;
  price_extended_t last_price;
  qty_long_t       last_quantity;
  volume_t         cumulative_volume;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le]] ExpandedTrade {
  Header        header {.msg_type = message_type_t::expanded_trade};
  symbol_wide_t symbol;
  price_long_t  last_price;
  qty_long_t    last_quantity;
  volume_t      cumulative_volume;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] LongTrade {
  Header        header {.msg_type = message_type_t::long_trade};
  symbol_long_t symbol;
  price_long_t  last_price;
  qty_long_t    last_quantity;
  volume_t      cumulative_volume;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le]] ShortTrade {
  Header         header {.msg_type = message_type_t::short_trade};
  symbol_short_t symbol;
  price_short_t  last_price;
  qty_short_t    last_quantity;
  volume_short_t cumulative_volume; ///< Short trade uses a 7-digit cumulative volume.
  std::uint8_t   newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Administrative messages (spec §10)
// ─────────────────────────────────────────────────────────────────────

/// Change in a security's trading state (spec §10.1).
struct [[=rbe::pack_le]] TradingStatus {
  Header           header {.msg_type = message_type_t::trading_status};
  symbol_wide_t    symbol;
  halt_status_t    halt_status;
  reg_sho_action_t reg_sho_action;
  std::uint8_t     reserved_1;
  std::uint8_t     reserved_2;
  std::uint8_t     newline = '\n';
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<cboe::top::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::msg_list<
    Logon, LogonAccepted, LogonRejected, ExpandedSpin, ExtendedSpin, SpinDone, ServerHeartbeat, ClientHeartbeat,
    Seconds, Milliseconds, ExtendedBidUpdate, ExpandedBidUpdate, LongBidUpdate, ShortBidUpdate, ExtendedAskUpdate,
    ExpandedAskUpdate, LongAskUpdate, ShortAskUpdate, ExpandedTwoSidedUpdate, LongTwoSidedUpdate, ShortTwoSidedUpdate,
    ExtendedTwoSidedUpdate, ExtendedTrade, ExpandedTrade, LongTrade, ShortTrade, TradingStatus>;

} // namespace top

} // namespace cboe
