/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file london.hpp
 * @version 1.0
 * @date 05/08/2026
 * @brief London Stock Exchange Group Ticker Plant (GTP) market data protocol.
 *
 * Message set for the LSE Group Ticker Plant, GTP002 – Technical Guide
 * (London Stock Exchange), issue 25.1 (11 December 2024). All multi-byte
 * integers are little-endian on the wire (spec §3.5) and messages are
 * 1-byte packed.
 *
 * Each UDP packet begins with an 8-byte Unit Header (spec §3.8) followed
 * by one or more back-to-back messages. Every message starts with a
 * 3-byte header carrying its 2-byte length and 1-byte type code. It is
 * expressed here as a nested `Header` field so that `rbe::id` and
 * `rbe::length` are declared once and dispatch/framing work through
 * introspection of the nested struct.
 */
#pragma once

#include <rbe/core/annotations.hpp>
#include <rbe/core/custom.hpp> // for rbe::string<N> (see note below)
#include <rbe/core/message_list.hpp>

#include <cstdint>
#include <tuple>

// NOTE: `rbe::string<N>` is documented in the design overview as a
// library-provided fixed-length text type but is not implemented yet;
// this file assumes it will land. Until it does, messages that reference
// it (Login Request, Instrument Directory, Trade, MiFID II Trade, …) will
// not compile.

namespace lse {

// ─────────────────────────────────────────────────────────────────────
// Semantic type aliases (spec §3.5)
// ─────────────────────────────────────────────────────────────────────

/// Signed 64-bit price with 8 implied decimal places.
using price_t = std::int64_t;

/// Signed 64-bit price with 4 implied decimal places.
using price4_t = std::int64_t;

/// Unsigned 64-bit size with 8 implied decimal places.
using size_t_ = std::uint64_t;

/// Unsigned 64-bit size with 4 implied decimal places.
using size4_t = std::uint64_t;

/// Signed 64-bit quantity with 8 implied decimal places (Int Size).
using int_size_t = std::int64_t;

/// Nanoseconds since Unix epoch (UDT, spec §3.5).
using timestamp_t = std::uint64_t;

using instrument_id_t        = std::uint64_t; ///< GTP instrument identifier.
using order_id_t             = std::uint64_t; ///< Unique order identifier.
using trade_id_t             = std::uint64_t; ///< Unique trade identifier.
using request_id_t           = std::uint32_t; ///< Client-supplied echo value.
using sequence_number_t      = std::uint32_t; ///< Real-time channel sequence number.
using exchange_market_size_t = std::uint64_t; ///< Exchange Market Size (EMS).

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

enum class message_type_t : std::uint8_t {
  // Administrative messages (§3.6)
  login_request                = 0x01,
  login_response               = 0x02,
  replay_request               = 0x03,
  replay_response              = 0x04,
  recovery_request             = 0x81,
  recovery_response            = 0x82,
  replay_and_recovery_complete = 0x83,

  // Application messages (§3.7)
  system_event                  = 0x53,
  instrument_directory          = 0x70,
  instrument_directory_equities = 0x52,
  instrument_status             = 0x48,
  add_order_mbo                 = 0x41,
  add_order_short_mbo           = 0x65,
  add_order_mbp                 = 0x66,
  add_order_short_mbp           = 0x67,
  add_order_incremental         = 0x46,
  order_modify                  = 0x55,
  order_delete                  = 0x44,
  top_of_book                   = 0x69,
  order_book_clear              = 0x79,
  trade                         = 0x50,
  statistics                    = 0x77,
  statistics_update             = 0x6A,
  statistics_snapshot           = 0x6b,
  mifid_trade                   = 0x51,
  trade_summary                 = 0x57,
  analytics                     = 0x61,
};

/// Buy/sell side (Byte 'B'/'S').
enum class side_t : std::uint8_t {
  buy  = 'B',
  sell = 'S',
};

/// Login Response status (§3.10.2).
enum class login_status_t : std::uint8_t {
  accepted                     = 'A',
  compid_inactive_or_suspended = 'a',
  login_limit_reached          = 'b',
  service_unavailable          = 'c',
  max_connections_reached      = 'd',
  failed_other                 = 'e',
  invalid_compid_or_ip         = 'f',
};

/// Replay Response status (§3.10.3).
enum class replay_status_t : std::uint8_t {
  accepted                 = 'A',
  request_limit_reached    = 'D',
  out_of_range             = 'O',
  replay_unavailable       = 'U',
  concurrent_limit_reached = 'c',
  failed_other             = 'e',
};

/// Recovery Response status (§3.10.4).
enum class recovery_status_t : std::uint8_t {
  accepted                    = 'A',
  out_of_range                = 'O',
  invalid_group_or_instrument = 'a',
  request_limit_reached       = 'b',
  concurrent_limit_reached    = 'c',
  invalid_recovery_type       = 'd',
  failed_other                = 'e',
};

/// Recovery Request scope (§3.9.3).
enum class request_level_t : std::uint8_t {
  instrument        = 0,
  group             = 1,
  multicast_channel = 2,
};

/// Recovery Request data category (§3.9.3).
enum class recovery_type_t : std::uint8_t {
  instrument_directory = 0,
  order_book           = 1,
  all_trades           = 2,
  statistics           = 3,
  instrument_status    = 4,
  reserved             = 5,
  system_event         = 6,
};

/// Order-book type (§3.9.3 & §3.11.x).
enum class order_book_type_t : std::uint8_t {
  all                   = 0, ///< Recovery Request only.
  firm_quote_book       = 1,
  off_book              = 2,
  electronic_order_book = 3,
  private_rfq           = 4,
};

/// Source venue identifier (§3.11.x, UInt16).
enum class source_venue_t : std::uint16_t {
  london_stock_exchange = 1,
};

/// System Event code (§3.11.1).
enum class event_code_t : std::uint8_t {
  end_of_day   = 'C',
  start_of_day = 'O',
};

/// Instrument trading status (§3.11.4).
enum class trading_status_t : std::uint8_t {
  halt                              = 'H',
  halt_matching_partition_suspended = 'J',
  halt_system_suspended             = 'K',
  regular_trading                   = 'T', ///< Or Start Trade Reporting.
  halt_regulatory                   = 'P',
  end_trade_reporting               = 't',
  opening_auction_call              = 'a',
  post_close                        = 'b',
  closed                            = 'c',
  closing_auction_call              = 'd',
  aesp_auction_call                 = 'e',
  resume_auction                    = 'f',
  pre_mandatory                     = 'm',
  mandatory                         = 'n',
  post_mandatory                    = 'o',
  edsp_auction_call                 = 'q',
  periodic_auction_call             = 'r',
  inactive                          = '1',
  suspended                         = '2',
  no_active_session                 = 'w',
  end_of_post_close                 = 'x',
  closing_price_crossing_session    = 'u',
};

/// Instrument Status session change reason (§3.11.4).
enum class session_change_reason_t : std::uint8_t {
  scheduled_transition    = 0,
  extended_by_market_ops  = 1,
  shortened_by_market_ops = 2,
  market_order_imbalance  = 3,
  price_outside_range     = 4,
  aesp_or_circuit_breaker = 5,
  unavailable             = 9,
};

/// Add Order Incremental order type (§3.11.9).
enum class order_type_t : std::uint8_t {
  limit_or_firm_quote = 0,
  market              = 1,
  named_limit         = 3,
  named_market        = 4,
  executable_quotes   = 5,
};

/// Clearing settlement mode (§3.11.3).
enum class clearing_type_t : std::uint8_t {
  not_cleared = 0,
  cleared     = 1,
};

/// Trade category (§3.11.14 / §3.11.18).
enum class trade_type_t : std::uint8_t {
  regular                    = 0,
  auction_trade_bulk         = 1,
  auction_trade_individual   = 2,
  on_book_trade_cancellation = 9,
  trade_correction           = 11,
  rfq_trade                  = 22,
  rfq_trade_cancellation     = 23,
  rfq_trade_correction       = 24,
};

/// Auction type (§3.11.14 / §3.11.17 / §3.11.18).
enum class auction_type_t : std::uint8_t {
  none             = 0,
  closing_auction  = 'C',
  opening_auction  = 'O',
  aesp             = 'A',
  edsp             = 'B',
  resume_auction   = 'E',
  periodic_auction = 'F',
};

/// Hidden Execution Indicator (§3.11.14).
enum class hidden_execution_indicator_t : std::uint8_t {
  not_applicable = 0,
  visible        = 1,
  hidden         = 2,
};

/// Trade Qualifier (§3.11.14 / §3.11.18).
enum class trade_qualifier_t : std::uint8_t {
  na                  = ' ',
  closing_price_cross = 'C',
};

/// Statistic type (§3.11.16, UInt16).
enum class statistic_type_t : std::uint16_t {
  indicative_auction_uncrossing = 1,
  official_opening_price        = 2,
  official_closing_price        = 3,
  trade_high_on_book            = 4,
  trade_low_on_book             = 5,
  trade_high_all                = 6,
  trade_low_all                 = 7,
  trade_high_52_week            = 8,
  trade_low_52_week             = 9,
  best_closing_bid              = 10,
  best_closing_ask              = 11,
  static_reference_price        = 16,
  dynamic_reference_price       = 17,
};

/// Statistics Update auction info (§3.11.16).
enum class auction_info_t : std::uint8_t {
  not_applicable                  = 0,
  buy_imbalance                   = 'B',
  no_imbalance                    = 'N',
  insufficient_orders_for_auction = 'O',
  sell_imbalance                  = 'S',
};

/// Statistics opening/closing price indicator (§3.11.16 / §3.11.17).
enum class price_indicator_t : std::uint8_t {
  none           = 0,
  ut             = 'A',
  at             = 'B',
  mid_of_bbo     = 'C',
  last_at        = 'D',
  last_ut        = 'E',
  manual         = 'F',
  previous_close = 'I',
};

/// Statistics Snapshot imbalance direction (§3.11.17).
enum class imbalance_direction_t : std::uint8_t {
  buy_imbalance                   = 'B',
  no_imbalance                    = 'N',
  insufficient_orders_for_auction = 'O',
  sell_imbalance                  = 'S',
};

/// MiFID II Trade market mechanism (§3.11.18).
enum class market_mechanism_t : std::uint8_t {
  central_limit_order_book = 1,
  request_for_quote        = 6,
};

/// MiFID II Trade trading mode (§3.11.18).
enum class trading_mode_t : std::uint8_t {
  scheduled_opening_auction  = 'O',
  scheduled_closing_auction  = 'K',
  scheduled_intraday_auction = 'I',
  unscheduled_auction        = 'U',
  continuous_trading         = '2',
  at_market_close_trading    = '3',
};

// ─────────────────────────────────────────────────────────────────────
// Fixed-width text fields (left-justified ASCII, space padded)
// ─────────────────────────────────────────────────────────────────────

using comp_id_t             = std::array<char, 8>; ///< CompID (username).
using isin_t                = std::array<char, 12>; ///< ISIN (§3.11.2).
using sedol_t               = std::array<char, 8>; ///< SEDOL (§3.11.3).
using currency_t            = std::array<char, 3>; ///< ISO 4217 currency code.
using segment_t             = std::array<char, 6>; ///< Segment identifier.
using symbol_t              = std::array<char, 8>; ///< Instrument symbol.
using description_t         = std::array<char, 40>; ///< Instrument description.
using venue_instrument_id_t = std::array<char, 11>; ///< Source-venue instrument id.
using tick_id_t             = std::array<char, 2>; ///< Tick structure id.
using ex_marker_code_t      = std::array<char, 2>; ///< Ex-marker.
using country_code_t        = std::array<char, 3>; ///< Country of register.
using security_exchange_t   = std::array<char, 11>; ///< Security Exchange (not applicable to LSE).
using participant_t         = std::array<char, 11>; ///< Trading participant identity.
using rfq_id_t              = std::array<char, 10>; ///< Private RFQ identifier.
using group_id_t            = std::array<char, 6>; ///< Recovery Request group id.
using date_ascii_t          = std::array<char, 8>; ///< YYYYMMDD ASCII date.
using time_ascii_t          = std::array<char, 6>; ///< HHMMSS ASCII time.
using date_time_iso_t       = std::array<char, 27>; ///< ISO 8601 date-time string.
using mifid_decimal_t       = std::array<char, 20>; ///< {DECIMAL-n/m} numeric string.
using transaction_id_t      = std::array<char, 52>; ///< MiFID transaction identification code.
using iic_type_t            = std::array<char, 4>; ///< Instrument Identification Code Type (e.g. "ISIN").
using price_notation_t      = std::array<char, 4>; ///< "MONE" or "PERC".
using venue_of_execution_t  = std::array<char, 4>; ///< MIC of execution venue.
using pt_flag_t             = std::array<char, 4>; ///< Post-trade 4-byte alpha flag.

// ─────────────────────────────────────────────────────────────────────
// Unit Header (spec §3.8)
// ─────────────────────────────────────────────────────────────────────

/// Prefix of every UDP payload. Followed by `message_count` back-to-back
/// application or administrative messages. A Unit Header with
/// `message_count == 0` is the server's heartbeat (§3.10.1).
struct[[= rbe::pack_le]] UnitHeader {
  std::uint16_t length {}; ///< Total packet size incl. this header and payload.
  std::uint8_t message_count {}; ///< Number of payload messages that follow.
  std::uint8_t market_data_group {}; ///< Identity of the market data group (Byte).
  sequence_number_t sequence_number {}; ///< Sequence number of the first payload message.
};

// ─────────────────────────────────────────────────────────────────────
// Common message header (§3.9 / §3.11)
// ─────────────────────────────────────────────────────────────────────

/// 3-byte header prefix of every LSE GTP message.
///
/// Each message struct embeds this as its first member and default-
/// initializes `length` and `msg_type` to its own compile-time values.
/// The `rbe::id` and `rbe::length` annotations live here so they are
/// declared exactly once for the whole protocol.
struct[[= rbe::pack_le]] Header {
  [[= rbe::length]] std::uint16_t length {};
  [[= rbe::id]] message_type_t msg_type {};
};

// ─────────────────────────────────────────────────────────────────────
// Administrative messages (spec §3.9 – §3.10)
// ─────────────────────────────────────────────────────────────────────

// clang-format off

/// Client → server: log in to replay or recovery channel (§3.9.1).
struct [[=rbe::pack_le]] LoginRequest {
  Header     header {.length = 11, .msg_type = message_type_t::login_request};
  comp_id_t  username;
};

/// Server → client: response to a login request (§3.10.2).
struct [[=rbe::pack_le]] LoginResponse {
  Header         header {.length = 4, .msg_type = message_type_t::login_response};
  login_status_t status;
};

/// Client → server: request retransmission on the replay channel (§3.9.2).
struct [[=rbe::pack_le]] ReplayRequest {
  Header        header {.length = 15, .msg_type = message_type_t::replay_request};
  std::uint32_t first_message;   ///< Sequence number of first message in range.
  std::uint32_t count;           ///< Number of messages to be resent.
  request_id_t  request_id;      ///< Echoed back in the Replay Response.
};

/// Server → client: response to a replay request (§3.10.3).
struct [[=rbe::pack_le]] ReplayResponse {
  Header          header {.length = 16, .msg_type = message_type_t::replay_response};
  std::uint32_t   first_message; ///< Zero if status != accepted.
  std::uint32_t   count;         ///< Number of messages to follow (excl. completion).
  replay_status_t status;
  request_id_t    request_id;
};

/// Client → server: request a snapshot/reference data set (§3.9.3).
struct [[=rbe::pack_le]] RecoveryRequest {
  Header            header {.length = 30, .msg_type = message_type_t::recovery_request};
  request_level_t   request_level;
  instrument_id_t   instrument;     ///< Only used when request_level == instrument.
  group_id_t        group_id;       ///< Only used when request_level == group.
  order_book_type_t order_book_type;///< Only used when request_level == instrument.
  source_venue_t    source_venue;   ///< Mandatory when request_level == group.
  recovery_type_t   recovery_type;
  sequence_number_t sequence_number;///< Only valid if recovery_type == all_trades.
  request_id_t      request_id;
};

/// Server → client: response to a recovery request (§3.10.4).
struct [[=rbe::pack_le]] RecoveryResponse {
  Header            header {.length = 16, .msg_type = message_type_t::recovery_response};
  sequence_number_t sequence_number; ///< Snapshot sync point on real-time channel.
  std::uint32_t     count;           ///< Number of messages to follow (excl. completion).
  recovery_status_t status;
  request_id_t      request_id;
};

/// Server → client: marks the end of a replay or recovery session (§3.10.5).
struct [[=rbe::pack_le]] ReplayAndRecoveryComplete {
  Header           header {.length = 8, .msg_type = message_type_t::replay_and_recovery_complete};
  request_id_t     request_id;
  trading_status_t trading_status;   ///< Populated only at end of individual order-book snapshot.
};

// ─────────────────────────────────────────────────────────────────────
// Application messages (spec §3.11)
// ─────────────────────────────────────────────────────────────────────

/// Start and end-of-day marker (§3.11.1).
struct [[=rbe::pack_le]] SystemEvent {
  Header         header {.length = 14, .msg_type = message_type_t::system_event};
  timestamp_t    timestamp;
  event_code_t   event_code;
  source_venue_t source_venue;
};

/// Instrument reference data — common set (§3.11.2).
struct [[=rbe::pack_le]] InstrumentDirectory {
  Header              header {.length = 141, .msg_type = message_type_t::instrument_directory};
  timestamp_t         timestamp;
  instrument_id_t     instrument;
  isin_t              isin;
  std::uint8_t        allowed_book_types;         ///< Bit field, see §3.11.2.
  source_venue_t      source_venue;
  venue_instrument_id_t venue_instrument_id;
  tick_id_t           tick_id;
  price_t             reserved_field_1;
  price_t             dynamic_circuit_breaker_tolerances;
  price_t             static_circuit_breaker_tolerances;
  segment_t           segment;
  rbe::string<12>     reserved_field_2;
  rbe::string<11>     reserved_field_3;
  currency_t          currency;
  std::uint8_t        partition_id;               ///< Byte.
  rbe::string<4>      reserved_field_4;
  price4_t            average_daily_turnover;     ///< Not applicable to LSE.
  rbe::string<8>      reserved_field_5;
  std::uint8_t        reserved_field_6;           ///< Bit field.
  price_t             reserved_field_7;
  price_t             reserved_field_8;
};

/// Instrument reference data — equity-specific (§3.11.3).
struct [[=rbe::pack_le]] InstrumentDirectoryEquities {
  Header                  header {.length = 313, .msg_type = message_type_t::instrument_directory_equities};
  timestamp_t             timestamp;
  instrument_id_t         instrument;
  isin_t                  isin;
  sedol_t                 sedol;
  std::uint8_t            allowed_book_types;     ///< Bit field, see §3.11.3.
  source_venue_t          source_venue;
  venue_instrument_id_t   venue_instrument_id;
  segment_t               segment;
  currency_t              currency;
  tick_id_t               tick_id;
  price_t                 previous_day_closing_price;
  price_t                 reserved_field_1;
  price_t                 dynamic_circuit_breaker_tolerances;
  price_t                 static_circuit_breaker_tolerances;
  std::uint8_t            reserved_field_2;       ///< Bit field.
  std::uint8_t            reserved_field_3;
  date_ascii_t            expiration_date;
  date_ascii_t            listing_start_date;
  date_ascii_t            listing_end_date;
  size_t_                 minimum_lot;            ///< Minimum Lot / Minimum Execution Size.
  price_t                 last_price_in_preceding_session;
  date_ascii_t            last_price_in_preceding_session_date;
  std::uint8_t            reserved_field_4;
  date_ascii_t            reserved_field_5;
  date_ascii_t            reserved_field_6;
  ex_marker_code_t        ex_marker_code;
  std::uint8_t            security_type;
  country_code_t          country_of_register;
  exchange_market_size_t  exchange_market_size;
  size_t_                 minimum_peak_size_multiplier;
  price_t                 security_maximum_spread;
  clearing_type_t         clearing_type;
  price_t                 strike_price;
  security_exchange_t     security_exchange;      ///< Not applicable to LSE.
  rbe::string<12>         reserved_field_7;
  std::uint8_t            reserved_field_8;
  std::uint64_t           reserved_field_9;
  size_t_                 reserved_field_10;
  std::uint8_t            partition_id;           ///< Byte.
  size_t_                 reserved_field_11;
  price_t                 reserved_field_12;
  std::uint32_t           reserved_field_13;
  std::uint16_t           reserved_field_14;
  symbol_t                symbol;
  description_t           description;
};

/// Scheduled / unscheduled trading-status change (§3.11.4).
struct [[=rbe::pack_le]] InstrumentStatus {
  Header                  header {.length = 30, .msg_type = message_type_t::instrument_status};
  timestamp_t             timestamp;
  instrument_id_t         instrument;
  source_venue_t          source_venue;
  trading_status_t        trading_status;
  session_change_reason_t session_change_reason;
  time_ascii_t            new_end_time;           ///< Local time (not UTC); spaces if no change.
  order_book_type_t       order_book_type;
};

/// First order of an MBO snapshot side (§3.11.5).
struct [[=rbe::pack_le]] AddOrderMBO {
  Header            header {.length = 67, .msg_type = message_type_t::add_order_mbo};
  timestamp_t       timestamp;
  order_id_t        order_id;
  side_t            side;
  size_t_           size;
  instrument_id_t   instrument;
  price_t           price;
  price_t           reserved_field;
  source_venue_t    source_venue;
  order_book_type_t order_book_type;
  participant_t     participant;
  std::uint8_t      depth;                        ///< Total orders on this side.
};

/// Subsequent order of an MBO snapshot side (§3.11.6).
struct [[=rbe::pack_le]] AddOrderShortMBO {
  Header        header {.length = 46, .msg_type = message_type_t::add_order_short_mbo};
  order_id_t    order_id;
  size_t_       size;
  price_t       price;
  price_t       reserved_field;
  participant_t participant;
};

/// First price point of an MBP snapshot side (§3.11.7).
struct [[=rbe::pack_le]] AddOrderMBP {
  Header            header {.length = 50, .msg_type = message_type_t::add_order_mbp};
  timestamp_t       timestamp;
  side_t            side;
  size_t_           size;
  instrument_id_t   instrument;
  price_t           price;
  price_t           reserved_field;
  source_venue_t    source_venue;
  order_book_type_t order_book_type;
  std::uint16_t     splits;                       ///< Number of orders at this price point.
  std::uint8_t      depth;
};

/// Subsequent price point of an MBP snapshot side (§3.11.8).
struct [[=rbe::pack_le]] AddOrderShortMBP {
  Header        header {.length = 29, .msg_type = message_type_t::add_order_short_mbp};
  size_t_       size;
  price_t       price;
  price_t       reserved_field;
  std::uint16_t splits;
};

/// New displayable order in the retrospective order book (§3.11.9).
struct [[=rbe::pack_le]] AddOrderIncremental {
  Header            header {.length = 77, .msg_type = message_type_t::add_order_incremental};
  timestamp_t       timestamp;
  order_id_t        order_id;
  side_t            side;
  size_t_           size;
  instrument_id_t   instrument;
  price_t           price;
  timestamp_t       transaction_time;
  source_venue_t    source_venue;
  order_book_type_t order_book_type;
  participant_t     participant;
  order_type_t      order_type;
  rfq_id_t          rfq_id;                       ///< Populated only for Private RFQ.
};

/// Order price/size modification (§3.11.10).
struct [[=rbe::pack_le]] OrderModify {
  Header            header {.length = 80, .msg_type = message_type_t::order_modify};
  timestamp_t       timestamp;
  order_id_t        order_id;
  instrument_id_t   instrument;
  side_t            side;
  std::uint8_t      flags;                        ///< Bit 0 – Priority Flag (§3.11.10).
  order_book_type_t order_book_type;
  size_t_           new_quantity;
  price_t           new_price;
  price_t           reserved_field;
  source_venue_t    source_venue;
  price_t           previous_price;
  size_t_           previous_quantity;
  timestamp_t       transaction_time;
};

/// Order removal from the retrospective order book (§3.11.11).
struct [[=rbe::pack_le]] OrderDelete {
  Header            header {.length = 55, .msg_type = message_type_t::order_delete};
  timestamp_t       timestamp;
  order_id_t        order_id;
  instrument_id_t   instrument;
  side_t            side;
  order_book_type_t order_book_type;
  source_venue_t    source_venue;
  price_t           previous_price;
  size_t_           previous_quantity;
  timestamp_t       transaction_time;
};

/// Level-1 top-of-book update (§3.11.12).
struct [[=rbe::pack_le]] TopOfBook {
  Header            header {.length = 87, .msg_type = message_type_t::top_of_book};
  timestamp_t       timestamp;
  instrument_id_t   instrument;
  source_venue_t    source_venue;
  size_t_           bid_market_size;              ///< Aggregated market-order size.
  price_t           bid_limit_price;
  price_t           reserved_field_1;
  size_t_           bid_limit_size;
  size_t_           offer_market_size;
  price_t           offer_limit_price;
  price_t           reserved_field_2;
  size_t_           offer_limit_size;
  order_book_type_t order_book_type;
  std::uint8_t      flags;                        ///< Bit 0 – Bid depth, bit 1 – Offer depth.
};

/// Remove all orders from an instrument's book (§3.11.13).
struct [[=rbe::pack_le]] OrderBookClear {
  Header            header {.length = 22, .msg_type = message_type_t::order_book_clear};
  timestamp_t       timestamp;
  source_venue_t    source_venue;
  instrument_id_t   instrument;
  order_book_type_t order_book_type;
};

/// Executed trade (§3.11.14).
struct [[=rbe::pack_le]] Trade {
  Header                       header {.length = 66, .msg_type = message_type_t::trade};
  timestamp_t                  timestamp;
  timestamp_t                  transaction_time;
  source_venue_t               source_venue;
  size_t_                      executed_size;
  instrument_id_t              instrument;
  price_t                      price;
  price_t                      reserved_field;
  trade_id_t                   trade_id;
  trade_type_t                 trade_type;
  auction_type_t               auction_type;      ///< Only relevant when trade_type == auction_trade_bulk.
  std::uint8_t                 flags;             ///< Bit 0 – Cancellation, bit 1 – Correction.
  hidden_execution_indicator_t hidden_execution_indicator;
  trade_qualifier_t            trade_qualifier;
};

/// Frequently-updated derived statistics (§3.11.15).
struct [[=rbe::pack_le]] Statistics {
  Header          header {.length = 77, .msg_type = message_type_t::statistics};
  timestamp_t     timestamp;
  instrument_id_t instrument;
  source_venue_t  source_venue;
  size4_t         volume;
  size4_t         volume_on_book;
  price4_t        vwap;
  price4_t        vwap_on_book;
  std::uint32_t   number_of_trades;
  std::uint32_t   number_of_trades_on_book;
  price4_t        turnover;
  price4_t        turnover_on_book;
};

/// Infrequently-updated derived statistics (§3.11.16).
struct [[=rbe::pack_le]] StatisticsUpdate {
  Header             header {.length = 50, .msg_type = message_type_t::statistics_update};
  timestamp_t        timestamp;
  instrument_id_t    instrument;
  source_venue_t     source_venue;
  statistic_type_t   statistic_type;
  price_t            statistic_price;
  size_t_            statistic_size;
  auction_type_t     auction_type;                ///< Populated if statistic_type == indicative_auction_uncrossing.
  size_t_            imbalance_quantity;
  auction_info_t     auction_info;                ///< Populated if statistic_type == indicative_auction_uncrossing.
  price_indicator_t  opening_closing_price_indicator; ///< Populated if statistic_type == official_opening/closing_price.
};

/// Full statistics snapshot for recovery (§3.11.17).
struct [[=rbe::pack_le]] StatisticsSnapshot {
  Header                header {.length = 273, .msg_type = message_type_t::statistics_snapshot};
  timestamp_t           timestamp;
  instrument_id_t       instrument;
  source_venue_t        source_venue;
  size4_t               volume;
  size4_t               volume_on_book;
  price4_t              vwap;
  price4_t              vwap_on_book;
  std::uint32_t         number_of_trades;
  std::uint32_t         number_of_trades_on_book;
  price4_t              turnover;
  price4_t              turnover_on_book;
  price_t               official_opening_price;
  price_t               official_closing_price;
  price_t               trade_high_on_book;
  price_t               trade_low_on_book;
  price_t               trade_high;
  price_t               trade_low;
  price_t               trade_high_52_week;
  price_t               trade_low_52_week;
  price_indicator_t     opening_price_indicator;
  price_indicator_t     closing_price_indicator;
  price_t               iau_price;                ///< Last indicative auction crossing price.
  size_t_               iau_paired_size;
  size_t_               imbalance_quantity;
  imbalance_direction_t imbalance_direction;
  price_t               best_closing_bid_price;
  price_t               best_closing_ask_price;
  size_t_               best_closing_bid_size;
  size_t_               best_closing_ask_size;
  price_t               reserved_field_1;
  price_t               reserved_field_2;
  size_t_               reserved_field_3;
  price_t               reserved_field_4;
  auction_type_t        auction_type;
  price_t               last_trade_price;
  size_t_               last_trade_quantity;
  timestamp_t           last_trade_time;
  price_t               static_reference_price;
  price_t               dynamic_reference_price;
};

/// MiFID II compliant trade (§3.11.18).
struct [[=rbe::pack_le]] MiFIDTrade {
  Header              header {.length = 286, .msg_type = message_type_t::mifid_trade};
  timestamp_t         timestamp;
  source_venue_t      source_venue;
  instrument_id_t     instrument;
  transaction_id_t    transaction_identification_code;
  trade_type_t        trade_type;
  auction_type_t      auction_type;               ///< Only relevant when trade_type == auction_trade_bulk.
  mifid_decimal_t     mifid_price;
  mifid_decimal_t     mifid_quantity;
  date_time_iso_t     trading_date_and_time;
  iic_type_t          instrument_identification_code_type;
  isin_t              instrument_identification_code;
  price_notation_t    price_notation;
  currency_t          price_major_currency;
  mifid_decimal_t     notional_amount;
  currency_t          notional_currency;
  venue_of_execution_t venue_of_execution;
  date_time_iso_t     publication_date_and_time;
  pt_flag_t           pt_ref_price_waiver_flag;   ///< Not applicable to LSE.
  pt_flag_t           reserved_field_1;
  pt_flag_t           market_closing_price_flag;  ///< "CLSE" or blank.
  pt_flag_t           pt_algo_trade;              ///< "ALGO" or blank.
  pt_flag_t           pt_cancellation_flag;       ///< "CANC" or blank.
  pt_flag_t           pt_amendment_flag;          ///< "AMND" or blank.
  std::uint8_t        reserved_field_2;
  rbe::string<3>      reserved_field_3;
  mifid_decimal_t     reserved_field_4;
  pt_flag_t           reserved_field_5;
  trade_qualifier_t   trade_qualifier;
  market_mechanism_t  market_mechanism;
  trading_mode_t      trading_mode;
  std::uint8_t        transaction_category;       ///< 'D' Dark, '-' None.
  std::uint8_t        negotiation_indicator;      ///< '-' Not a Negotiated Trade.
  std::uint8_t        agency_cross_indicator;     ///< '-' No Agency Cross Trade.
  std::uint8_t        modification_indicator;     ///< 'C' CANC, 'A' AMND, '-' New.
  std::uint8_t        reference_price_indicator;  ///< '1' CLSE, '-' Not Reference.
  std::uint8_t        special_dividend_indicator; ///< '-' No Special Dividend.
  std::uint8_t        off_book_automated_indicator;///< '-' Unspecified.
  std::uint8_t        price_formation_indicator;  ///< 'P' Plain-Vanilla.
  std::uint8_t        algorithmic_indicator;      ///< 'H' ALGO, '-' Not Algorithmic.
  std::uint8_t        post_trade_deferral_reason; ///< '-' Immediate Publication.
  std::uint8_t        deferral_enrichment_type;   ///< '-' Not Applicable.
  std::uint8_t        duplicative_indicator;      ///< '-' Unique Trade Report.
};

/// Aggregated multi- and single-fill trade summary (§3.11.19).
struct [[=rbe::pack_le]] TradeSummary {
  Header          header {.length = 94, .msg_type = message_type_t::trade_summary};
  timestamp_t     timestamp;
  instrument_id_t instrument;
  source_venue_t  source_venue;
  timestamp_t     transaction_time;
  price_t         far_price;
  size_t_         total_executed_quantity;
  size_t_         total_hidden_executed_quantity;
  size_t_         deleted_order_quantity;         ///< Deleted due to SEP.
  std::uint8_t    side;                           ///< ' ' No side, 'B' Buy, 'S' Sell.
  int_size_t      best_bid_size;                  ///< -1 if top of book not disclosed.
  price_t         best_bid_price;                 ///< -1 if top of book not disclosed, 0 if market order on top.
  int_size_t      best_offer_size;
  price_t         best_offer_price;
};

/// Order-book activity statistics (§3.11.20).
struct [[=rbe::pack_le]] Analytics {
  Header          header {.length = 109, .msg_type = message_type_t::analytics};
  timestamp_t     timestamp;
  instrument_id_t instrument;
  source_venue_t  source_venue;
  timestamp_t     start_time;
  timestamp_t     end_time;
  std::uint32_t   buy_order_count;
  std::uint32_t   sell_order_count;
  size4_t         buy_order_size;
  size4_t         sell_order_size;
  std::uint32_t   buy_order_cancellations;
  std::uint32_t   sell_order_cancellations;
  std::uint32_t   buy_limit_order_cancellations;
  std::uint32_t   buy_market_order_cancellations;
  std::uint32_t   sell_limit_order_cancellations;
  std::uint32_t   sell_market_order_cancellations;
  price_t         bid_ask_spread;
  price_t         vwap_buy;
  price_t         vwap_sell;
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<lse::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::msg_list<
    LoginRequest, LoginResponse, ReplayRequest, ReplayResponse, RecoveryRequest, RecoveryResponse,
    ReplayAndRecoveryComplete, SystemEvent, InstrumentDirectory, InstrumentDirectoryEquities, InstrumentStatus,
    AddOrderMBO, AddOrderShortMBO, AddOrderMBP, AddOrderShortMBP, AddOrderIncremental, OrderModify, OrderDelete,
    TopOfBook, OrderBookClear, Trade, Statistics, StatisticsUpdate, StatisticsSnapshot, MiFIDTrade, TradeSummary,
    Analytics>;

} // namespace lse
