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
 * Each protocol declares its `Header` once and composes it with its
 * message set through `rbe::frame`: `rbe::id` on the header field selects
 * the message annotated with the matching `rbe::id(value)`. See `message`
 * (and `packet` for PITCH) at the end of each namespace.
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
  [[=rbe::frame_length]] std::uint16_t length; ///< Length of entire block including this header.
  std::uint8_t count; ///< Number of messages that follow.
  std::uint8_t unit; ///< Unit that applies to the enclosed messages.
  sequence_t sequence; ///< Sequence of the first enclosed sequenced message.
};

// ─────────────────────────────────────────────────────────────────────
// Common PITCH message header (spec §2.1)
// ─────────────────────────────────────────────────────────────────────

/// 2-byte header prefix of every PITCH / GRP / Spin message. Cboe puts
/// `length` FIRST (offset 0), then the message type (offset 1). Declared
/// once for the whole protocol and composed with the message set in
/// `message`; `length` covers the header plus the message.
struct[[=rbe::pack_le]] Header {
  [[= rbe::frame_length]] std::uint8_t length {};
  [[= rbe::id]] message_type_t msg_type {};
};

// ─────────────────────────────────────────────────────────────────────
// Gap Request Proxy messages — TCP (spec §3)
// ─────────────────────────────────────────────────────────────────────


/// GRP / Spin Server login (spec §3.1, §5.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::login)]] Login {
  std::array<char,4>  session_sub_id;
  std::array<char,4>  username;
  std::array<char,2>  filler;        ///< Space filled.
  std::array<char,10> password;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Login>() == 22);

/// Response to a Login (spec §3.2, §5.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::login_response)]] LoginResponse {
  login_status_t status;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<LoginResponse>() == 3);

/// Request retransmission of a sequenced range (spec §3.3).
struct [[=rbe::pack_le, =rbe::id(message_type_t::gap_request)]] GapRequest {
  std::uint8_t  unit;
  sequence_t    sequence; ///< Lowest sequence in the requested range.
  std::uint16_t count;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<GapRequest>() == 9);

/// Reply to a GapRequest (spec §3.4).
struct [[=rbe::pack_le, =rbe::id(message_type_t::gap_response)]] GapResponse {
  std::uint8_t  unit;
  sequence_t    sequence;
  std::uint16_t count;
  gap_status_t  status;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<GapResponse>() == 10);

// ─────────────────────────────────────────────────────────────────────
// Spin Server messages — TCP (spec §5)
// ─────────────────────────────────────────────────────────────────────

/// Advertises the highest sequence for which a spin is currently available (spec §5.3).
struct [[=rbe::pack_le, =rbe::id(message_type_t::spin_image_available)]] SpinImageAvailable {
  sequence_t sequence;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SpinImageAvailable>() == 6);

/// Request a spin at a previously advertised sequence (spec §5.4).
struct [[=rbe::pack_le, =rbe::id(message_type_t::spin_request)]] SpinRequest {
  sequence_t sequence;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SpinRequest>() == 6);

/// Response to a SpinRequest (spec §5.5).
struct [[=rbe::pack_le, =rbe::id(message_type_t::spin_response)]] SpinResponse {
  sequence_t    sequence;
  std::uint32_t order_count; ///< Number of Add Order messages that will follow. 0 on reject.
  spin_status_t status;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SpinResponse>() == 11);

/// End-of-spin marker; not sent if the SpinRequest was rejected (spec §5.6).
struct [[=rbe::pack_le, =rbe::id(message_type_t::spin_finished)]] SpinFinished {
  sequence_t sequence;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<SpinFinished>() == 6);

// ─────────────────────────────────────────────────────────────────────
// PITCH 2.X market data messages (spec §4)
// ─────────────────────────────────────────────────────────────────────

/// Whole-second timestamp base for the unit (spec §4.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::time)]] Time {
  time_t time;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Time>() == 6);

/// Instructs feed recipients to clear all orders for the Cboe book of
/// the enclosing Sequenced Unit (spec §4.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::unit_clear)]] UnitClear {
  time_offset_t time_offset;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<UnitClear>() == 6);

/// Newly accepted visible order — long form (spec §4.3.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::add_order_long)]] AddOrderLong {
  time_offset_t time_offset;
  order_id_t    order_id;
  side_t        side;
  std::uint32_t quantity;
  symbol_t      symbol;
  long_price_t  price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AddOrderLong>() == 35);

/// Newly accepted visible order — short form (spec §4.3.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::add_order_short)]] AddOrderShort {
  time_offset_t  time_offset;
  order_id_t     order_id;
  side_t         side;
  std::uint16_t  quantity;
  symbol_short_t symbol;
  short_price_t  price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AddOrderShort>() == 25);

/// Newly accepted quote/order carrying attribution — used on the Cboe
/// Systematic Internaliser platform (spec §4.3.3).
struct [[=rbe::pack_le, =rbe::id(message_type_t::add_order_expanded)]] AddOrderExpanded {
  time_offset_t    time_offset;
  order_id_t       order_id;
  side_t           side;
  std::uint32_t    quantity;
  symbol_t         symbol;
  long_price_t     price;
  std::uint8_t     add_flags; ///< Bit 1 = SI Quote; bits 0, 2-7 reserved.
  participant_id_t participant_id;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AddOrderExpanded>() == 40);

/// Visible order executed at its resting price (spec §4.4).
struct [[=rbe::pack_le, =rbe::id(message_type_t::order_executed)]] OrderExecuted {
  time_offset_t      time_offset;
  order_id_t         order_id;
  std::uint32_t      executed_shares;
  execution_id_t     execution_id;
  execution_flags_t  execution_flags;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<OrderExecuted>() == 30);

/// Visible order executed at a price different from the resting price (spec §4.5).
struct [[=rbe::pack_le, =rbe::id(message_type_t::order_executed_at_price_size)]] OrderExecutedAtPriceSize {
  time_offset_t     time_offset;
  order_id_t        order_id;
  std::uint32_t     executed_shares;
  std::uint32_t     remaining_shares; ///< 0 → order fully removed from the book.
  execution_id_t    execution_id;
  long_price_t      price;
  execution_flags_t execution_flags;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<OrderExecutedAtPriceSize>() == 42);

/// Partial visible-order cancel — long form (spec §4.6.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::reduce_size_long)]] ReduceSizeLong {
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint32_t cancelled_shares;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ReduceSizeLong>() == 18);

/// Partial visible-order cancel — short form (spec §4.6.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::reduce_size_short)]] ReduceSizeShort {
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint16_t cancelled_shares;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ReduceSizeShort>() == 16);

/// Visible order modification — long form (spec §4.7.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::modify_order_long)]] ModifyOrderLong {
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint32_t shares;
  long_price_t  price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ModifyOrderLong>() == 26);

/// Visible order modification — short form (spec §4.7.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::modify_order_short)]] ModifyOrderShort {
  time_offset_t time_offset;
  order_id_t    order_id;
  std::uint16_t shares;
  short_price_t price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<ModifyOrderShort>() == 18);

/// Complete visible-order cancel (spec §4.8).
struct [[=rbe::pack_le, =rbe::id(message_type_t::delete_order)]] DeleteOrder {
  time_offset_t time_offset;
  order_id_t    order_id;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<DeleteOrder>() == 14);

/// Hidden-order or routed execution — long form (spec §4.9.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_long)]] TradeLong {
  time_offset_t  time_offset;
  order_id_t     order_id; ///< Obfuscated by default (spec §4.9).
  side_t         side;     ///< Always 'B' for hidden trades.
  std::uint32_t  shares;
  symbol_t       symbol;
  long_price_t   price;
  execution_id_t execution_id;
  trade_flags_t  trade_flags;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeLong>() == 48);

/// Hidden-order or routed execution — short form (spec §4.9.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_short)]] TradeShort {
  time_offset_t  time_offset;
  order_id_t     order_id;
  side_t         side;
  std::uint16_t  shares;
  symbol_short_t symbol;
  short_price_t  price;
  execution_id_t execution_id;
  trade_flags_t  trade_flags;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeShort>() == 38);

/// Extended trade details, only used on the Cboe European platform (spec §4.9.4).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_extended)]] TradeExtended {
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
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeExtended>() == 68);

/// Trade reported on an ISIN not known to Cboe — TRF only (spec §4.9.5).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_unknown_symbol)]] TradeUnknownSymbol {
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
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeUnknownSymbol>() == 72);

/// Break of an order-generated trade — carries only the execution id
/// of the broken trade (spec Appendix A / Appendix B, Trade Break).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trade_break)]] TradeBreak {
  time_offset_t  time_offset;
  execution_id_t execution_id;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradeBreak>() == 14);

/// End-of-session marker for the unit (spec §4.10).
struct [[=rbe::pack_le, =rbe::id(message_type_t::end_of_session)]] EndOfSession {
  time_offset_t time_offset;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<EndOfSession>() == 6);

/// Start-of-transaction marker; subsequent messages up to the matching
/// TransactionEnd belong to the same transaction block (spec §4.11).
struct [[=rbe::pack_le, =rbe::id(message_type_t::transaction_begin)]] TransactionBegin {
  time_offset_t time_offset;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TransactionBegin>() == 6);

/// End-of-transaction marker (spec §4.12).
struct [[=rbe::pack_le, =rbe::id(message_type_t::transaction_end)]] TransactionEnd {
  time_offset_t time_offset;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TransactionEnd>() == 6);

/// Change in trading status for a security (spec §4.13).
struct [[=rbe::pack_le, =rbe::id(message_type_t::trading_status)]] TradingStatus {
  time_offset_t         time_offset;
  symbol_t              symbol;
  trading_status_code_t status;
  std::array<char,3>        reserved;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<TradingStatus>() == 18);

/// Disseminates opening / closing / high / low statistics prices — Cboe
/// European platform only (spec §4.14).
struct [[=rbe::pack_le, =rbe::id(message_type_t::statistics)]] Statistics {
  time_offset_t         time_offset;
  symbol_t              symbol;
  long_price_t          price;
  statistic_type_t      statistic_type;
  price_determination_t price_determination;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<Statistics>() == 24);

/// Indicative price / size during a call or extension phase (spec §4.15.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::auction_update)]] AuctionUpdate {
  time_offset_t       time_offset;
  symbol_t            symbol;
  auction_type_t      auction_type;
  long_price_t        reference_price;
  long_price_t        indicative_price;
  std::uint32_t       indicative_shares;
  outside_tolerance_t outside_tolerance;
  includes_primary_t  includes_primary;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AuctionUpdate>() == 37);

/// Post-uncross auction result (spec §4.15.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::auction_summary)]] AuctionSummary {
  time_offset_t  time_offset;
  symbol_t       symbol;
  auction_type_t auction_type;
  long_price_t   price;
  std::uint32_t  shares;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<AuctionSummary>() == 27);

/// Real-time index quote (spec §4.16.1) — XIC/XID/XIE feeds only.
struct [[=rbe::pack_le, =rbe::id(message_type_t::index_quote)]] IndexQuote {
  std::uint64_t  timestamp;    ///< Nanoseconds since midnight.
  index_ticker_t index_ticker;
  long_price_t   price;
  index_status_t index_status;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<IndexQuote>() == 29);

/// End-of-day exchange delivery settlement price for an index (spec §4.16.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::index_quote_edsp)]] IndexQuoteEDSP {
  std::uint64_t  timestamp;
  index_ticker_t index_ticker;
  long_price_t   price;
};
static_assert(rbe::wire_size_of<Header>() + rbe::wire_size_of<IndexQuoteEDSP>() == 28);

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Framing
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::any<
    Login, LoginResponse, GapRequest, GapResponse, SpinImageAvailable, SpinRequest, SpinResponse, SpinFinished, Time,
    UnitClear, AddOrderLong, AddOrderShort, AddOrderExpanded, OrderExecuted, OrderExecutedAtPriceSize, ReduceSizeLong,
    ReduceSizeShort, ModifyOrderLong, ModifyOrderShort, DeleteOrder, TradeLong, TradeShort, TradeExtended,
    TradeUnknownSymbol, TradeBreak, EndOfSession, TransactionBegin, TransactionEnd, TradingStatus, Statistics,
    AuctionUpdate, AuctionSummary, IndexQuote, IndexQuoteEDSP>;

/// One PITCH / GRP / Spin message: `Header` followed by the message selected by `msg_type`.
using message = rbe::frame<Header, messages>;

/// One UDP frame: `SequencedUnitHeader` followed by `count` back-to-back messages
/// (a `count` of zero is a heartbeat).
using packet = rbe::frame<SequencedUnitHeader, rbe::many<message>>;

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
/// Declared once for the whole protocol and composed with the message set
/// in `message`; `msg_type` selects the message and the wire size is fixed
/// per message type.
struct[[= rbe::pack_le]] Header {
  [[= rbe::id]] message_type_t msg_type {};
};

// ─────────────────────────────────────────────────────────────────────
// Session / logon messages (spec §4)
// ─────────────────────────────────────────────────────────────────────

// clang-format off

/// Client → server logon (spec §4.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::logon)]] Logon {
  std::array<char,6>  username;
  std::array<char,10> password;
  boolean_t       spin_flag; ///< 'Y' → send a spin of current top of book.
  std::uint8_t    newline = '\n';
};

/// Server → client acceptance (spec §4.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::logon_accepted)]] LogonAccepted {
  std::uint8_t newline = '\n';
};

/// Server → client rejection (spec §4.3).
struct [[=rbe::pack_le, =rbe::id(message_type_t::logon_rejected)]] LogonRejected {
  reject_reason_t reject_reason;
  std::uint8_t    newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Spin messages (spec §5)
// ─────────────────────────────────────────────────────────────────────

/// Per-symbol snapshot delivered during a spin, expanded form for ISRA
/// symbol sizes (spec §5.1.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::expanded_spin)]] ExpandedSpin {
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
struct [[=rbe::pack_le, =rbe::id(message_type_t::extended_spin)]] ExtendedSpin {
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
struct [[=rbe::pack_le, =rbe::id(message_type_t::spin_done)]] SpinDone {
  std::uint8_t newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Heartbeat messages (spec §6)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le, =rbe::id(message_type_t::server_heartbeat)]] ServerHeartbeat {
  std::uint8_t newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::client_heartbeat)]] ClientHeartbeat {
  std::uint8_t newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Time update messages (spec §7)
// ─────────────────────────────────────────────────────────────────────

/// Seconds past midnight, Eastern (spec §7.1).
struct [[=rbe::pack_le, =rbe::id(message_type_t::seconds)]] Seconds {
  seconds_t    seconds;
  std::uint8_t newline = '\n';
};

/// Milliseconds since the last Seconds message (spec §7.2).
struct [[=rbe::pack_le, =rbe::id(message_type_t::milliseconds)]] Milliseconds {
  milliseconds_t milliseconds;
  std::uint8_t   newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Bid / Ask update messages (spec §8.1)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le, =rbe::id(message_type_t::extended_bid_update)]] ExtendedBidUpdate {
  symbol_wide_t    symbol;
  price_extended_t bid_price;
  qty_long_t       bid_quantity;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::expanded_bid_update)]] ExpandedBidUpdate {
  symbol_wide_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::long_bid_update)]] LongBidUpdate {
  symbol_long_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::short_bid_update)]] ShortBidUpdate {
  symbol_short_t symbol;
  price_short_t  bid_price;
  qty_short_t    bid_quantity;
  std::uint8_t   newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::extended_ask_update)]] ExtendedAskUpdate {
  symbol_wide_t    symbol;
  price_extended_t ask_price;
  qty_long_t       ask_quantity;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::expanded_ask_update)]] ExpandedAskUpdate {
  symbol_wide_t symbol;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::long_ask_update)]] LongAskUpdate {
  symbol_long_t symbol;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::short_ask_update)]] ShortAskUpdate {
  symbol_short_t symbol;
  price_short_t  ask_price;
  qty_short_t    ask_quantity;
  std::uint8_t   newline = '\n';
};

// ─────────────────────────────────────────────────────────────────────
// Two-sided update messages (spec §8.2)
// ─────────────────────────────────────────────────────────────────────

struct [[=rbe::pack_le, =rbe::id(message_type_t::expanded_two_sided_update)]] ExpandedTwoSidedUpdate {
  symbol_wide_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::long_two_sided_update)]] LongTwoSidedUpdate {
  symbol_long_t symbol;
  price_long_t  bid_price;
  qty_long_t    bid_quantity;
  price_long_t  ask_price;
  qty_long_t    ask_quantity;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::short_two_sided_update)]] ShortTwoSidedUpdate {
  symbol_short_t symbol;
  price_short_t  bid_price;
  qty_short_t    bid_quantity;
  price_short_t  ask_price;
  qty_short_t    ask_quantity;
  std::uint8_t   newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::extended_two_sided_update)]] ExtendedTwoSidedUpdate {
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

struct [[=rbe::pack_le, =rbe::id(message_type_t::extended_trade)]] ExtendedTrade {
  symbol_wide_t    symbol;
  price_extended_t last_price;
  qty_long_t       last_quantity;
  volume_t         cumulative_volume;
  std::uint8_t     newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::expanded_trade)]] ExpandedTrade {
  symbol_wide_t symbol;
  price_long_t  last_price;
  qty_long_t    last_quantity;
  volume_t      cumulative_volume;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::long_trade)]] LongTrade {
  symbol_long_t symbol;
  price_long_t  last_price;
  qty_long_t    last_quantity;
  volume_t      cumulative_volume;
  std::uint8_t  newline = '\n';
};

struct [[=rbe::pack_le, =rbe::id(message_type_t::short_trade)]] ShortTrade {
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
struct [[=rbe::pack_le, =rbe::id(message_type_t::trading_status)]] TradingStatus {
  symbol_wide_t    symbol;
  halt_status_t    halt_status;
  reg_sho_action_t reg_sho_action;
  std::uint8_t     reserved_1;
  std::uint8_t     reserved_2;
  std::uint8_t     newline = '\n';
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Framing
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::any<
    Logon, LogonAccepted, LogonRejected, ExpandedSpin, ExtendedSpin, SpinDone, ServerHeartbeat, ClientHeartbeat,
    Seconds, Milliseconds, ExtendedBidUpdate, ExpandedBidUpdate, LongBidUpdate, ShortBidUpdate, ExtendedAskUpdate,
    ExpandedAskUpdate, LongAskUpdate, ShortAskUpdate, ExpandedTwoSidedUpdate, LongTwoSidedUpdate, ShortTwoSidedUpdate,
    ExtendedTwoSidedUpdate, ExtendedTrade, ExpandedTrade, LongTrade, ShortTrade, TradingStatus>;

/// One TOP message: `Header` followed by the message selected by `msg_type`.
/// TODO: the length is implied by `msg_type`; until `rbe::any` resolves it
///       (payload_extent::any_id) the payload runs to the end of the buffer.
using message = rbe::frame<Header, messages>;

} // namespace top

} // namespace cboe
