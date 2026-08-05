/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file opra.hpp
 * @version 1.0
 * @date 05/08/2026
 * @brief OPRA (Options Price Reporting Authority) Binary Data Recipient
 *        Interface protocol.
 *
 * Message set for the OPRA Binary Data Recipient Interface Specification,
 * version 6.4a (July 1, 2026). OPRA is the US options consolidated feed
 * disseminating trades, quotes and administrative data from all Options
 * Participants over IP multicast.
 *
 * All numeric fields are 1/2/4/8-byte binary integers transmitted in
 * network byte order (BIG-ENDIAN) per spec §3.04. Every IP multicast
 * payload begins with a 21-byte `BlockHeader` (spec §3.05) followed by
 * one or more messages. Each message is prefixed by a 12-byte message
 * `Header` (spec §4.0) carrying the Participant ID, Message Category,
 * Message Type and Message Indicator that together identify the message
 * layout. OPRA does not carry a per-message length on the wire (except
 * for Administrative category-C messages): message length is derived
 * from the (category, indicator) tuple as defined in §5.
 */
#pragma once

#include <rbe/core/annotations.hpp>
#include <rbe/core/custom.hpp> // for rbe::string<N> (see note below)

#include <cstdint>
#include <tuple>

// NOTE: `rbe::string<N>` is documented in the design overview as a
// library-provided fixed-length text type but is not implemented yet;
// this file assumes it will land. Until it does, the messages that
// reference it (all messages carrying a Security Symbol) will not
// compile.

namespace opra {

// ─────────────────────────────────────────────────────────────────────
// Semantic type aliases (spec §3.04, §7)
// ─────────────────────────────────────────────────────────────────────

/// Price / index value with a floating decimal position given by the
/// accompanying Denominator Code (see denominator_code_t). Signed
/// 4-byte integer in all message categories except Short Quote
/// (Category q), which uses `short_price_t`. Represents the whole and
/// decimal portion of a price with the denominator determining the
/// location of the decimal point (spec §7.06, §7.11, §7.22, …).
using price_t = std::int32_t;

/// 2-byte unsigned Short Quote price/size wire type (spec §6.04.2).
using short_price_t = std::uint16_t;

/// 8-byte signed underlying stock price (spec §7.36).
using underlying_price_t = std::int64_t;

/// 4-byte unsigned volume / size / open interest quantity.
using size_t = std::uint32_t;

/// Block sequence number of the first message in a transmission block
/// (spec §3.05.6). Rolls over from 4,294,967,295 to 1.
using block_seq_no_t = std::uint32_t;

using transaction_id_t  = std::uint32_t; ///< Reserved for internal use (spec §4.01.5).
using participant_ref_t = std::uint32_t; ///< Participant reference number (spec §4.01.6).
using trade_id_t        = std::uint32_t; ///< Trade Identifier – for future use (spec §7.33).

/// 32-bit seconds since Unix epoch (spec §3.05.8, first half of Block Timestamp).
using epoch_seconds_t = std::uint32_t;
/// Nanosecond fraction of the Block Timestamp (spec §3.05.8, second half).
using nanoseconds_t = std::uint32_t;

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

/// Participant / Processor identifier (spec §4.01.1).
enum class participant_id_t : std::uint8_t {
  amex    = 'A', ///< NYSE American
  box     = 'B', ///< Boston Options Exchange
  cboe    = 'C', ///< Cboe Options Exchange
  emerald = 'D', ///< MIAX Emerald
  edgx    = 'E', ///< Cboe EDGX Options Exchange
  mx2     = 'G', ///< MX2 Options
  gemx    = 'H', ///< Nasdaq GEMX
  ise     = 'I', ///< Nasdaq ISE
  mrx     = 'J', ///< Nasdaq MRX
  miax    = 'M', ///< Miami International Securities Exchange
  nyse    = 'N', ///< NYSE / NYSE Arca
  opra    = 'O', ///< Options Price Reporting Authority (sent by SIAC)
  pearl   = 'P', ///< MIAX PEARL
  nasd    = 'Q', ///< NASDAQ Options Market
  sphr    = 'S', ///< MIAX Sapphire
  bx      = 'T', ///< NASDAQ BX Options
  memx    = 'U', ///< Members Options Exchange
  iex     = 'V', ///< IEX Options LLC
  c2      = 'W', ///< Cboe C2 Options Exchange
  phlx    = 'X', ///< NASDAQ PHLX
  bats    = 'Z', ///< Cboe BZX Options Exchange
};

/// Message Category (spec §4.01.2). Determines the base message layout.
enum class msg_category_t : std::uint8_t {
  last_sale        = 'a', ///< Equity and Index Last Sale
  open_interest    = 'd', ///< Open Interest
  eod_summary      = 'f', ///< Equity and Index End of Day Summary
  long_quote       = 'k', ///< Long Equity and Index Quote
  short_quote      = 'q', ///< Short Equity and Index Quote
  administrative   = 'C', ///< Administrative (unformatted, variable length)
  control          = 'H', ///< Control (Start of Day, End of Day, etc.)
  series_mapping   = 'R', ///< Series Mapping Message
  underlying_value = 'Y', ///< Underlying Value Message
};

/// Message Type codes for Category `a` (Equity and Index Last Sale)
/// messages (spec §6.01). All mutually exclusive.
enum class last_sale_type_t : std::uint8_t {
  cancel_previous        = 'A', ///< CANC – Cancel a previously reported (non-last, non-opening) trade.
  out_of_sequence        = 'B', ///< OSEQ – Late report, out of sequence.
  cancel_last            = 'C', ///< CNCL – Cancel the last reported trade.
  late_in_sequence       = 'D', ///< LATE – Late report, in correct sequence.
  cancel_opening         = 'E', ///< CNCO – Cancel the opening trade (others followed).
  opening_late_oseq      = 'F', ///< OPEN – Late opening trade, out of sequence.
  cancel_only            = 'G', ///< CNOL – Cancel the only reported trade.
  opening_late_in_seq    = 'H', ///< OPNL – Late opening trade, in sequence.
  auto_execution         = 'I', ///< AUTO – Electronic execution.
  reopening              = 'J', ///< REOP – Reopening after halt.
  intermarket_sweep      = 'S', ///< ISOI – Intermarket Sweep Order execution.
  single_leg_auction     = 'a', ///< SLAN – Single Leg Auction, Non-ISO.
  single_leg_auction_iso = 'b', ///< SLAI – Single Leg Auction, ISO.
  single_leg_cross       = 'c', ///< SLCN – Single Leg Cross, Non-ISO.
  single_leg_cross_iso   = 'd', ///< SCLI – Single Leg Cross, ISO.
  single_leg_floor       = 'e', ///< SLFT – Single Leg Floor Trade.
  multi_leg_electronic   = 'f', ///< MLET – Multi Leg auto-electronic trade.
  multi_leg_auction      = 'g', ///< MLAT – Multi Leg Auction.
  multi_leg_cross        = 'h', ///< MLCT – Multi Leg Cross.
  multi_leg_floor        = 'i', ///< MLFT – Multi Leg floor trade.
  multi_leg_esl          = 'j', ///< MESL – Multi Leg auto-electronic trade against single legs.
  stock_opt_auction      = 'k', ///< TLAT – Stock Options Auction.
  multi_leg_asl          = 'l', ///< MASL – Multi Leg Auction against single legs.
  multi_leg_fsl          = 'm', ///< MFSL – Multi Leg floor trade against single legs.
  stock_opt_electronic   = 'n', ///< TLET – Stock Options auto-electronic trade.
  stock_opt_cross        = 'o', ///< TLCT – Stock Options Cross.
  stock_opt_floor        = 'p', ///< TLFT – Stock Options floor trade.
  stock_opt_esl          = 'q', ///< TESL – Stock Options auto-electronic trade against single legs.
  stock_opt_asl          = 'r', ///< TASL – Stock Options Auction against single legs.
  stock_opt_fsl          = 's', ///< TFSL – Stock Options floor trade against single legs.
  cbmo                   = 't', ///< CBMO – Multi Leg Floor Trade of Proprietary Products.
  mctp                   = 'u', ///< MCTP – Multilateral Compression Trade of Proprietary Products.
  extended_hours         = 'v', ///< EXHT – Extended Hours Trade (to be retired; see Trading Session Identifier).
};

/// Message Type codes for quote categories `k` and `q` (spec §6.04).
enum class quote_type_t : std::uint8_t {
  regular                 = ' ', ///< Regular Trading (space filled).
  non_firm                = 'F', ///< Non-Firm Quote.
  indicative              = 'I', ///< Indicative Value.
  rotation                = 'R', ///< Rotation.
  trading_halted          = 'T', ///< Trading Halted.
  auto_execution_eligible = 'A', ///< Eligible for Automatic Execution.
  bid_customer_interest   = 'B', ///< Bid contains Customer Trading Interest.
  offer_customer_interest = 'O', ///< Offer contains Customer Trading Interest.
  both_customer_interest  = 'C', ///< Both Bid and Offer contain Customer Trading Interest.
  offer_not_firm          = 'X', ///< Offer Side of Quote Not Firm; Bid Side Firm.
  bid_not_firm            = 'Y', ///< Bid Side of Quote Not Firm; Offer Side Firm.
};

/// Message Type codes for Category `H` (Control) messages (spec §6.06).
enum class control_type_t : std::uint8_t {
  start_of_day       = 'C', ///< Start of Day (Participant ID 'O').
  start_of_summary   = 'E', ///< Start of Summary (Participant ID != 'O').
  end_of_summary     = 'F', ///< End of Summary (Participant ID != 'O').
  end_of_day         = 'J', ///< End of Day (Participant ID 'O').
  reset_block_seq_no = 'K', ///< Reset Block Sequence Number (Participant ID 'O').
  start_of_open_int  = 'L', ///< Start of Open Interest (Participant ID 'O').
  end_of_open_int    = 'M', ///< End of Open Interest (Participant ID 'O').
  line_integrity     = 'N', ///< Line Integrity (Participant ID 'O').
  dr_activation      = 'P', ///< Disaster Recovery Data Center Activation (Participant ID 'O').
};

/// Message Type codes for Category `Y` (Underlying Value) messages (spec §6.08).
enum class underlying_value_type_t : std::uint8_t {
  last_sale     = ' ', ///< Index based on Last Sale (space filled).
  bid_and_offer = 'I', ///< Index based on Bid and Offer.
};

/// Message Type code for Category `R` (Series Mapping) messages (spec §6.07).
enum class series_mapping_type_t : std::uint8_t {
  series_mapping = 'A',
};

/// BBO Indicator carried in the Message Indicator field of quote
/// messages (categories k, q). Determines whether a Best Bid / Best
/// Offer appendage follows the quote payload (spec §7.01).
enum class bbo_indicator_t : std::uint8_t {
  no_bid_change_no_offer_change = 'A',
  no_bid_change_contains_offer  = 'B',
  no_bid_change_offer_appendage = 'C',
  no_bid_change_no_offer        = 'D',
  contains_bid_no_offer_change  = 'E',
  contains_bid_contains_offer   = 'F',
  contains_bid_offer_appendage  = 'G',
  contains_bid_no_offer         = 'H',
  no_bid_no_offer_change        = 'I',
  no_bid_contains_offer         = 'J',
  no_bid_offer_appendage        = 'K',
  no_bid_no_offer               = 'L',
  bid_appendage_no_offer_change = 'M',
  bid_appendage_contains_offer  = 'N',
  bid_appendage_offer_appendage = 'O', ///< Double appendage.
  bid_appendage_no_offer        = 'P',
  not_in_bbo                    = ' ', ///< Quote did not meet BBO requirements.
};

/// Data Feed Indicator (spec §3.05.3). Always 'O' for OPRA.
enum class data_feed_indicator_t : std::uint8_t {
  opra = 'O',
};

/// Retransmission Indicator (spec §3.05.4).
enum class retransmission_indicator_t : std::uint8_t {
  original      = ' ', ///< Not a retransmitted block.
  retransmitted = 'V', ///< Retransmitted block.
};

/// Session Indicator in the Block Header (spec §3.05.5) and Message
/// Header field carried in some contexts (spec §7.30).
///
/// Values 1–5 identify the Global Trading Hours (GTH) session for
/// Monday–Friday respectively; 0x00 is Regular Trading Hours; 'X' is
/// used on messages generated by OPRA itself during GTH.
enum class session_indicator_t : std::uint8_t {
  regular       = 0x00,
  gth_monday    = 0x01,
  gth_tuesday   = 0x02,
  gth_wednesday = 0x03,
  gth_thursday  = 0x04,
  gth_friday    = 0x05,
  gth_opra      = 'X',
};

/// Trading Session Identifier (spec §7.34) present in Equity and Index
/// Last Sale messages.
enum class trading_session_id_t : std::uint8_t {
  regular  = 0,
  extended = 1,
};

/// Denominator Code (spec §7.13). Indicates the position of the
/// floating decimal point within the associated price / value field.
enum class denominator_code_t : std::uint8_t {
  d_10        = 'A', ///< 1 decimal place.
  d_100       = 'B', ///< 2 decimal places.
  d_1000      = 'C', ///< 3 decimal places.
  d_10000     = 'D', ///< 4 decimal places.
  d_100000    = 'E', ///< 5 decimal places.
  d_1000000   = 'F', ///< 6 decimal places (not valid for Strike Price).
  d_10000000  = 'G', ///< 7 decimal places (not valid for Strike Price).
  d_100000000 = 'H', ///< 8 decimal places (Underlying Price only).
  no_fraction = 'I', ///< No fraction.
};

/// Expiration Month code (spec §7.14). Identifies both the month and
/// whether the option is a Call ('A'–'L') or a Put ('M'–'X').
enum class expiration_month_t : std::uint8_t {
  call_january   = 'A',
  call_february  = 'B',
  call_march     = 'C',
  call_april     = 'D',
  call_may       = 'E',
  call_june      = 'F',
  call_july      = 'G',
  call_august    = 'H',
  call_september = 'I',
  call_october   = 'J',
  call_november  = 'K',
  call_december  = 'L',
  put_january    = 'M',
  put_february   = 'N',
  put_march      = 'O',
  put_april      = 'P',
  put_may        = 'Q',
  put_june       = 'R',
  put_july       = 'S',
  put_august     = 'T',
  put_september  = 'U',
  put_october    = 'V',
  put_november   = 'W',
  put_december   = 'X',
};

/// Version code of the OPRA binary protocol (spec §3.05.1).
enum class version_t : std::uint8_t {
  v6 = 6, ///< Current version.
};

// ─────────────────────────────────────────────────────────────────────
// Fixed-width text fields (ASCII, left justified, space filled)
// ─────────────────────────────────────────────────────────────────────

/// 5-byte Security Symbol used in all message categories except Short
/// Quote (spec §7.29).
using symbol_t = std::array<char, 5>;

/// 4-byte Security Symbol used in Short Quote (Category q) messages
/// (spec §6.04.2).
using short_symbol_t = std::array<char, 4>;

// ─────────────────────────────────────────────────────────────────────
// Common sub-structures
// ─────────────────────────────────────────────────────────────────────

/// 3-byte Expiration Block: month/day/year (spec §7.14).
struct[[= rbe::pack_be]] ExpirationBlock {
  expiration_month_t month {};
  std::uint8_t day {}; ///< 1–31.
  std::uint8_t year {}; ///< 0–99, starting with year 2000.
};

/// 8-byte Block Timestamp (spec §3.05.8): seconds since Unix epoch
/// followed by nanoseconds within the second. Marks completion of
/// processing for the block.
struct[[= rbe::pack_be]] BlockTimestamp {
  epoch_seconds_t seconds {};
  nanoseconds_t nanoseconds {};
};

// ─────────────────────────────────────────────────────────────────────
// Transmission Block Header (spec §3.05) – prefixes every IP multicast
// payload; 21 bytes.
// ─────────────────────────────────────────────────────────────────────


struct[[= rbe::pack_be]] BlockHeader {
  version_t version {version_t::v6};
  std::uint16_t block_size {}; ///< Total block bytes incl. header, data, pad.
  data_feed_indicator_t data_feed_indicator {data_feed_indicator_t::opra};
  retransmission_indicator_t retransmission_indicator {retransmission_indicator_t::original};
  session_indicator_t session_indicator {session_indicator_t::regular};
  block_seq_no_t block_seq_no {}; ///< Sequence number of the first message in the block.
  std::uint8_t messages_in_block {};
  BlockTimestamp block_timestamp {};
  std::uint16_t block_checksum {}; ///< Lower 16 bits of the 32-bit sum of all bytes.
};

// ─────────────────────────────────────────────────────────────────────
// Message Header (spec §4.0) – 12-byte prefix of every OPRA message.
//
// The `msg_category` field carries the primary discriminator used by
// `rbe::id`. OPRA does not carry a per-message length on the wire
// (Administrative messages carry a separate `msg_data_length` field
// inside the message body); message length is derived from the
// (category, msg_indicator) tuple per §5. As a result no
// `[[= rbe::length]]` is applied here.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] Header {
  participant_id_t participant_id {};
  [[= rbe::id]] msg_category_t msg_category {};
  std::uint8_t msg_type {}; ///< ASCII: last_sale_type_t / quote_type_t / control_type_t / …
  std::uint8_t msg_indicator {}; ///< Space unless quote (then bbo_indicator_t).
  transaction_id_t transaction_id {}; ///< Reserved – ignore (spec §4.01.5).
  participant_ref_t participant_ref {}; ///< Optional Participant Reference Number.
};

// ─────────────────────────────────────────────────────────────────────
// BBO Appendages (spec §6.04.3, §6.04.4). Not standalone messages –
// they trail Long/Short quote payloads when the BBO Indicator so
// requires. Included here for on-wire modelling; not part of the
// `messages` tuple.
// ─────────────────────────────────────────────────────────────────────

/// 10-byte Best Bid or Best Offer appendage (spec §6.04.3).
/// Appended when the BBO Indicator is one of C/G/K (single best offer)
/// or M/N/P (single best bid).
struct[[= rbe::pack_be]] SingleAppendage {
  participant_id_t participant_id;
  denominator_code_t denominator_code;
  price_t price;
  size_t size;
};

/// 20-byte Best Bid and Best Offer appendage (spec §6.04.4).
/// Appended when the BBO Indicator is 'O' (double appendage).
struct[[= rbe::pack_be]] DoubleAppendage {
  participant_id_t best_bid_participant_id;
  denominator_code_t best_bid_denominator_code;
  price_t best_bid_price;
  size_t best_bid_size;
  participant_id_t best_offer_participant_id;
  denominator_code_t best_offer_denominator_code;
  price_t best_offer_price;
  size_t best_offer_size;
};

// ─────────────────────────────────────────────────────────────────────
// Category 'a' – Equity and Index Last Sale (spec §6.01), 43 bytes.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] EquityIndexLastSale {
  Header header {.msg_category = msg_category_t::last_sale};
  symbol_t security_symbol;
  std::uint8_t reserved1 {};
  ExpirationBlock expiration;
  denominator_code_t strike_price_denom;
  price_t strike_price;
  size_t volume;
  denominator_code_t premium_price_denom;
  price_t premium_price;
  trade_id_t trade_identifier {}; ///< For future use – Hex 0x00.
  trading_session_id_t trading_session_id {trading_session_id_t::regular};
  std::uint8_t reserved2[3] {};
};

// ─────────────────────────────────────────────────────────────────────
// Category 'd' – Open Interest (spec §6.02), 30 bytes.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] OpenInterest {
  Header header {.msg_category = msg_category_t::open_interest};
  symbol_t security_symbol;
  std::uint8_t reserved {};
  ExpirationBlock expiration;
  denominator_code_t strike_price_denom;
  price_t strike_price;
  size_t open_interest_volume;
};

// ─────────────────────────────────────────────────────────────────────
// Category 'f' – Equity and Index End of Day Summary (spec §6.03), 72 bytes.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] EquityIndexEodSummary {
  Header header {.msg_category = msg_category_t::eod_summary};
  symbol_t security_symbol;
  std::uint8_t reserved {};
  ExpirationBlock expiration;
  denominator_code_t strike_price_denom;
  price_t strike_price;
  size_t volume;
  size_t open_interest_volume;
  denominator_code_t premium_price_denom;
  price_t open_price;
  price_t high_price;
  price_t low_price;
  price_t last_price;
  price_t net_change; ///< Signed – may be negative.
  denominator_code_t underlying_price_denom;
  underlying_price_t underlying_price;
  price_t bid_price;
  price_t offer_price;
};

// ─────────────────────────────────────────────────────────────────────
// Category 'k' – Long Equity and Index Quote (spec §6.04.1), 43 bytes
// (plus optional 10-byte SingleAppendage or 20-byte DoubleAppendage
// selected by the BBO Indicator in the Message Header).
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] LongQuote {
  Header header {.msg_category = msg_category_t::long_quote};
  symbol_t security_symbol;
  std::uint8_t reserved {};
  ExpirationBlock expiration;
  denominator_code_t strike_price_denom;
  price_t strike_price;
  denominator_code_t premium_price_denom;
  price_t bid_price;
  size_t bid_size;
  price_t offer_price;
  size_t offer_size;
};

// ─────────────────────────────────────────────────────────────────────
// Category 'q' – Short Equity and Index Quote (spec §6.04.2), 29 bytes
// (plus optional appendage). Strike Price Denominator implied 'A',
// Premium Price Denominator implied 'B'.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] ShortQuote {
  Header header {.msg_category = msg_category_t::short_quote};
  short_symbol_t security_symbol;
  ExpirationBlock expiration;
  short_price_t strike_price;
  short_price_t bid_price;
  short_price_t bid_size;
  short_price_t offer_price;
  short_price_t offer_size;
};

// ─────────────────────────────────────────────────────────────────────
// Category 'C' – Administrative (spec §6.05). Fixed prefix is 14 bytes
// (12-byte Message Header + 2-byte Message Data Length); the variable-
// length Message Data field (printable ASCII 32–126, max 200 bytes)
// follows and is not modelled here. Administrative messages are always
// sent one per block.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] Administrative {
  Header header {.msg_category = msg_category_t::administrative};
  std::uint16_t msg_data_length {}; ///< Length of the trailing Message Data field, 0–200.
  // Message Data (variable, printable ASCII) follows on the wire.
};

// ─────────────────────────────────────────────────────────────────────
// Category 'H' – Control messages (spec §6.06). Message Header only,
// 12 bytes. The Type sub-code (control_type_t) discriminates the
// specific control action within the shared layout.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] Control {
  Header header {.msg_category = msg_category_t::control};
};

// ─────────────────────────────────────────────────────────────────────
// Category 'R' – Series Mapping Message (spec §6.07), 155 bytes.
// ─────────────────────────────────────────────────────────────────────

struct[[= rbe::pack_be]] SeriesMapping {
  Header header {
    .msg_category = msg_category_t::series_mapping,
    .msg_type     = static_cast<std::uint8_t>(series_mapping_type_t::series_mapping)
  };
  symbol_t security_symbol;
  ExpirationBlock expiration;
  denominator_code_t strike_price_denom;
  price_t strike_price;
  std::uint16_t multicast_line_number; ///< 1–96; the output line the series is assigned to.
  std::uint8_t reserved[128] {};
};

// ─────────────────────────────────────────────────────────────────────
// Category 'Y' – Underlying Value (spec §6.08).
// ─────────────────────────────────────────────────────────────────────

/// Underlying Value – Last Sale (Message Type ' '), 27 bytes (spec §6.08.1).
struct[[= rbe::pack_be]] UnderlyingValueLastSale {
  Header header {
    .msg_category = msg_category_t::underlying_value,
    .msg_type     = static_cast<std::uint8_t>(underlying_value_type_t::last_sale)
  };
  symbol_t security_symbol;
  std::uint8_t reserved1 {};
  denominator_code_t index_value_denom;
  price_t index_value;
  std::uint8_t reserved2[4] {};
};

/// Underlying Value – Bid and Offer (Message Type 'I'), 27 bytes (spec §6.08.2).
struct[[= rbe::pack_be]] UnderlyingValueBidOffer {
  Header header {
    .msg_category = msg_category_t::underlying_value,
    .msg_type     = static_cast<std::uint8_t>(underlying_value_type_t::bid_and_offer),
  };
  symbol_t security_symbol;
  std::uint8_t reserved {};
  denominator_code_t index_value_denom;
  price_t bid_index_value;
  price_t offer_index_value;
};


// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<opra::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = std::tuple<
    EquityIndexLastSale, OpenInterest, EquityIndexEodSummary, LongQuote, ShortQuote, Administrative, Control,
    SeriesMapping, UnderlyingValueLastSale, UnderlyingValueBidOffer>;

} // namespace opra
