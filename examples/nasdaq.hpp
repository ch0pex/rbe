/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file nasdaq.hpp
 * @version 1.0
 * @date 05/08/2026
 * @brief Nasdaq TotalView-ITCH 5.0 market data protocol.
 *
 * Message set for the Nasdaq TotalView-ITCH 5.0 direct data feed
 * (Nasdaq Stock Market, LLC).  All integer fields are big-endian
 * (network byte order) and 1-byte packed on the wire (spec "Data
 * Types").
 *
 * ITCH itself carries no length field within each message; the feed
 * is delivered over SoupBinTCP, "Compressed SoupBinTCP" or MoldUDP64,
 * each of which prefixes every payload with its own length.  The
 * shared `Header` below models that framed layout: a 2-byte big-endian
 * length prefix (per SoupBinTCP: it excludes the length field itself)
 * followed by the 11-byte common ITCH header (message type + stock
 * locate + tracking number + 6-byte nanosecond timestamp).  Each
 * message struct embeds this as its first member and default-
 * initializes `msg_type` and `length` to its own compile-time values.
 */
#pragma once

#include <rbe/core/annotations.hpp>
#include <rbe/core/custom.hpp> // for rbe::string<N>, rbe::uint48_t (see note below)
#include <rbe/core/message_list.hpp>

#include <cstdint>
#include <tuple>
#include "rbe/core/message_list.hpp"

// NOTE: `rbe::string<N>` and `rbe::uint48_t` are documented in the
// design overview as library-provided fixed-width primitives but are
// not implemented yet; this file assumes they will land. Until they
// do, the messages that reference them (all ITCH messages, since every
// header carries a 6-byte timestamp) will not compile.

namespace nasdaq {

// ─────────────────────────────────────────────────────────────────────
// Semantic type aliases (spec "Data Types")
// ─────────────────────────────────────────────────────────────────────

/// Price with 4 implied decimal places. e.g. 1'462'500 → 146.2500.
/// Maximum value is 200,000.0000 (decimal, 0x77359400 hex).
using price4_t = std::uint32_t;

/// Price with 8 implied decimal places. Used only by the MWCB Decline
/// Level message.
using price8_t = std::uint64_t;

/// Nanoseconds since midnight (Eastern Time). Wire-encoded in 6 bytes.
using timestamp_t = rbe::uint48_t;

using stock_locate_t    = std::uint16_t; ///< Dynamically assigned locate code (0 = not stock-dependent).
using tracking_number_t = std::uint16_t; ///< Nasdaq internal tracking number.
using order_ref_t       = std::uint64_t; ///< Day-unique order reference number.
using match_number_t    = std::uint64_t; ///< Day-unique match number (referenced by Broken Trade).
using shares_t          = std::uint32_t; ///< 32-bit share count field.
using shares64_t        = std::uint64_t; ///< 64-bit share count (Cross Trade, NOII paired/imbalance shares).

// ─────────────────────────────────────────────────────────────────────
// Fixed-width text fields (left-justified ASCII, space-padded)
// ─────────────────────────────────────────────────────────────────────

using stock_t                 = std::array<char, 8>; ///< Stock symbol.
using mpid_t                  = std::array<char, 4>; ///< Nasdaq Market Participant Identifier.
using trading_action_reason_t = std::array<char, 4>; ///< See spec Appendix C.
using issue_sub_type_t        = std::array<char, 2>; ///< See spec Appendix E.

// ─────────────────────────────────────────────────────────────────────
// Enumerations
// ─────────────────────────────────────────────────────────────────────

/// One byte ASCII discriminator carried at offset 0 of every ITCH message.
enum class message_type_t : std::uint8_t {
  system_event                = 'S',
  stock_directory             = 'R',
  stock_trading_action        = 'H',
  reg_sho_restriction         = 'Y',
  market_participant_position = 'L',
  mwcb_decline_level          = 'V',
  mwcb_status                 = 'W',
  ipo_quoting_period_update   = 'K',
  luld_auction_collar         = 'J',
  operational_halt            = 'h',
  add_order                   = 'A',
  add_order_mpid              = 'F',
  order_executed              = 'E',
  order_executed_with_price   = 'C',
  order_cancel                = 'X',
  order_delete                = 'D',
  order_replace               = 'U',
  trade                       = 'P',
  cross_trade                 = 'Q',
  broken_trade                = 'B',
  noii                        = 'I',
  rpii                        = 'N',
  dlcr                        = 'O',
};

/// System event codes (spec §1.1).
enum class system_event_t : std::uint8_t {
  start_of_messages     = 'O',
  start_of_system_hours = 'S',
  start_of_market_hours = 'Q',
  end_of_market_hours   = 'M',
  end_of_system_hours   = 'E',
  end_of_messages       = 'C',
};

/// Listing market / market tier for the issue (spec §1.2.1).
enum class market_category_t : std::uint8_t {
  nasdaq_global_select = 'Q',
  nasdaq_global        = 'G',
  nasdaq_capital       = 'S',
  nyse                 = 'N',
  nyse_american        = 'A',
  nyse_arca            = 'P',
  bats_z               = 'Z',
  iex                  = 'V',
  not_available        = ' ',
};

/// Nasdaq continued-listing compliance status (spec §1.2.1).
enum class financial_status_t : std::uint8_t {
  deficient                         = 'D',
  delinquent                        = 'E',
  bankrupt                          = 'Q',
  suspended                         = 'S',
  deficient_and_bankrupt            = 'G',
  deficient_and_delinquent          = 'H',
  delinquent_and_bankrupt           = 'J',
  deficient_delinquent_and_bankrupt = 'K',
  creations_redemptions_suspended   = 'C',
  normal                            = 'N',
  not_available                     = ' ',
};

enum class round_lots_only_t : std::uint8_t {
  yes = 'Y', ///< Nasdaq system only accepts round lots for this issue.
  no  = 'N',
};

enum class authenticity_t : std::uint8_t {
  live_production = 'P',
  test            = 'T',
};

enum class short_sale_threshold_t : std::uint8_t {
  restricted     = 'Y',
  not_restricted = 'N',
  not_available  = ' ',
};

enum class ipo_flag_t : std::uint8_t {
  yes           = 'Y',
  no            = 'N',
  not_available = ' ',
};

enum class luld_reference_price_tier_t : std::uint8_t {
  tier_1        = '1', ///< NMS Stocks and select ETPs.
  tier_2        = '2', ///< Other NMS Stocks.
  not_available = ' ',
};

enum class etp_flag_t : std::uint8_t {
  yes           = 'Y',
  no            = 'N',
  not_available = ' ',
};

enum class inverse_indicator_t : std::uint8_t {
  yes = 'Y',
  no  = 'N',
};

/// Current trading state disseminated by the Stock Trading Action
/// message (spec §1.2.2).
enum class trading_state_t : std::uint8_t {
  halted            = 'H', ///< Halted across all U.S. equity markets / SROs.
  paused            = 'P', ///< Paused (Nasdaq-listed securities only).
  quotation_only    = 'Q', ///< Quotation only period for cross-SRO halt or pause.
  trading_on_nasdaq = 'T',
};

/// Reg SHO short-sale price-test restriction status (spec §1.2.3).
enum class reg_sho_action_t : std::uint8_t {
  no_price_test        = '0',
  restriction_intraday = '1', ///< Restriction in effect due to an intraday price drop.
  restriction_remains  = '2',
};

enum class primary_market_maker_t : std::uint8_t {
  yes = 'Y',
  no  = 'N',
};

/// Registration status vs SEC Rules 101/104 of Regulation M (spec §1.2.4).
enum class market_maker_mode_t : std::uint8_t {
  normal        = 'N',
  passive       = 'P',
  syndicate     = 'S',
  pre_syndicate = 'R',
  penalty       = 'L',
};

/// Market participant registration status in the issue (spec §1.2.4).
enum class market_participant_state_t : std::uint8_t {
  active            = 'A',
  excused_withdrawn = 'E',
  withdrawn         = 'W',
  suspended         = 'S',
  deleted           = 'D',
};

/// MWCB level breached in the MWCB Status message (spec §1.2.5.2).
enum class mwcb_level_t : std::uint8_t {
  level_1 = '1',
  level_2 = '2',
  level_3 = '3',
};

/// IPO Quotation Release Qualifier (spec §1.2.6).
enum class ipo_quotation_release_qualifier_t : std::uint8_t {
  anticipated = 'A', ///< Anticipated quotation release time.
  cancelled   = 'C', ///< IPO release cancelled / postponed.
};

/// Market centre for the Operational Halt message (spec §1.2.8).
enum class market_code_t : std::uint8_t {
  nasdaq = 'Q',
  bx     = 'B',
  psx    = 'X',
};

/// Operational Halt Action (spec §1.2.8).
enum class operational_halt_action_t : std::uint8_t {
  halted  = 'H', ///< Operationally halted on the identified market.
  trading = 'T', ///< Operational halt lifted and trading resumed.
};

/// Buy/Sell indicator (spec §1.3.1). Note: the Trade (Non-Cross)
/// message always sets this to `buy` regardless of resting side
/// (effective 2014-07-14).
enum class buy_sell_t : std::uint8_t {
  buy  = 'B',
  sell = 'S',
};

/// Printable flag on the Order Executed With Price message (spec §1.4.2).
enum class printable_t : std::uint8_t {
  non_printable = 'N',
  printable     = 'Y',
};

/// Nasdaq cross session for Cross Trade / NOII (spec §1.5.2 / §1.6).
/// The value 'A' (Extended Trading Close) is NOII-only.
enum class cross_type_t : std::uint8_t {
  opening                = 'O',
  closing                = 'C',
  ipo_halt_pause         = 'H',
  extended_trading_close = 'A',
};

/// Market side of the NOII order imbalance (spec §1.6).
enum class imbalance_direction_t : std::uint8_t {
  buy                 = 'B',
  sell                = 'S',
  no_imbalance        = 'N',
  insufficient_orders = 'O',
  paused              = 'P',
};

/// Absolute % deviation of Near Indicative Clearing Price to Current
/// Reference Price (spec §1.6).
enum class price_variation_indicator_t : std::uint8_t {
  less_than_1_pct = 'L',
  pct_1_to_2      = '1',
  pct_2_to_3      = '2',
  pct_3_to_4      = '3',
  pct_4_to_5      = '4',
  pct_5_to_6      = '5',
  pct_6_to_7      = '6',
  pct_7_to_8      = '7',
  pct_8_to_9      = '8',
  pct_9_to_10     = '9',
  pct_10_to_20    = 'A',
  pct_20_to_30    = 'B',
  pct_30_or_more  = 'C',
  not_calculated  = ' ',
};

/// Retail interest side indicator on the RPII message (spec §1.7).
enum class interest_flag_t : std::uint8_t {
  buy_side  = 'B',
  sell_side = 'S',
  both      = 'A',
  none      = 'N',
};

/// DLCR Open Eligibility Status (spec §1.8).
enum class open_eligibility_status_t : std::uint8_t {
  not_eligible = 'N',
  eligible     = 'Y',
};

// ─────────────────────────────────────────────────────────────────────
// Common ITCH message header
// ─────────────────────────────────────────────────────────────────────

/// 13-byte prefix of every framed ITCH message.
///
/// Layout on the wire:
///   [0..2)  length         — SoupBinTCP length prefix (excludes itself)
///   [2..3)  msg_type       — one-byte ASCII message-type code
///   [3..5)  stock_locate   — dynamically assigned locate code (0 = not stock-dependent)
///   [5..7)  tracking_number — Nasdaq internal tracking number
///   [7..13) timestamp      — nanoseconds since midnight (Eastern Time)
///
/// The `rbe::id` and `rbe::length` annotations live here so they are
/// declared exactly once for the whole protocol.
struct[[= rbe::pack_be]] Header {
  [[= rbe::length]] std::uint16_t length {};
  [[= rbe::id]] message_type_t msg_type {};
  stock_locate_t stock_locate {};
  tracking_number_t tracking_number {};
  timestamp_t timestamp {};
};

// ─────────────────────────────────────────────────────────────────────
// Message definitions
// ─────────────────────────────────────────────────────────────────────

// clang-format off

// --- System event (spec §1.1) ----------------------------------------

struct [[=rbe::pack_be]] SystemEvent {
  Header         header {.length = 12, .msg_type = message_type_t::system_event};
  system_event_t event_code;
};

// --- Stock-related messages (spec §1.2) ------------------------------

/// Stock Directory (spec §1.2.1). Disseminated at the start of the day
/// for every active symbol; occasionally intraday for corrections.
struct [[=rbe::pack_be]] StockDirectory {
  Header                      header {.length = 39, .msg_type = message_type_t::stock_directory};
  stock_t                     stock;
  market_category_t           market_category;
  financial_status_t          financial_status;
  std::uint32_t               round_lot_size;
  round_lots_only_t           round_lots_only;
  std::uint8_t                issue_classification;  ///< See spec Appendix D.
  issue_sub_type_t            issue_sub_type;        ///< See spec Appendix E.
  authenticity_t              authenticity;
  short_sale_threshold_t      short_sale_threshold;
  ipo_flag_t                  ipo_flag;
  luld_reference_price_tier_t luld_reference_price_tier;
  etp_flag_t                  etp_flag;
  std::uint32_t               etp_leverage_factor;   ///< Rounded down to nearest integer.
  inverse_indicator_t         inverse_indicator;
};

/// Stock Trading Action (spec §1.2.2).
struct [[=rbe::pack_be]] StockTradingAction {
  Header                  header {.length = 25, .msg_type = message_type_t::stock_trading_action};
  stock_t                 stock;
  trading_state_t         trading_state;
  std::uint8_t            reserved;
  trading_action_reason_t reason;              ///< See spec Appendix C.
};

/// Reg SHO Short Sale Price Test Restricted Indicator (spec §1.2.3).
struct [[=rbe::pack_be]] RegSHORestriction {
  Header           header {.length = 20, .msg_type = message_type_t::reg_sho_restriction};
  stock_t          stock;
  reg_sho_action_t reg_sho_action;
};

/// Market Participant Position (spec §1.2.4).
struct [[=rbe::pack_be]] MarketParticipantPosition {
  Header                     header {.length = 26, .msg_type = message_type_t::market_participant_position};
  mpid_t                     mpid;
  stock_t                    stock;
  primary_market_maker_t     primary_market_maker;
  market_maker_mode_t        market_maker_mode;
  market_participant_state_t market_participant_state;
};

/// MWCB Decline Level Message (spec §1.2.5.1). Stock Locate always 0.
struct [[=rbe::pack_be]] MWCBDeclineLevel {
  Header    header {.length = 35, .msg_type = message_type_t::mwcb_decline_level};
  price8_t  level_1;
  price8_t  level_2;
  price8_t  level_3;
};

/// MWCB Status Message (spec §1.2.5.2). Stock Locate always 0.
struct [[=rbe::pack_be]] MWCBStatus {
  Header       header {.length = 12, .msg_type = message_type_t::mwcb_status};
  mwcb_level_t breached_level;
};

/// IPO Quoting Period Update (spec §1.2.6). Stock Locate always 0.
struct [[=rbe::pack_be]] IPOQuotingPeriodUpdate {
  Header                            header {.length = 28, .msg_type = message_type_t::ipo_quoting_period_update};
  stock_t                           stock;
  std::uint32_t                     ipo_quotation_release_time;  ///< Seconds since midnight; 0 if cancelled.
  ipo_quotation_release_qualifier_t ipo_quotation_release_qualifier;
  price4_t                          ipo_price;                   ///< 0 if quotation cancelled/postponed.
};

/// LULD Auction Collar (spec §1.2.7).
struct [[=rbe::pack_be]] LULDAuctionCollar {
  Header        header {.length = 35, .msg_type = message_type_t::luld_auction_collar};
  stock_t       stock;
  price4_t      auction_collar_reference_price;
  price4_t      upper_auction_collar_price;
  price4_t      lower_auction_collar_price;
  std::uint32_t auction_collar_extension;
};

/// Operational Halt (spec §1.2.8).
struct [[=rbe::pack_be]] OperationalHalt {
  Header                    header {.length = 21, .msg_type = message_type_t::operational_halt};
  stock_t                   stock;
  market_code_t             market_code;
  operational_halt_action_t operational_halt_action;
};

// --- Add Order messages (spec §1.3) ----------------------------------

/// Add Order — No MPID Attribution (spec §1.3.1).
struct [[=rbe::pack_be]] AddOrder {
  Header      header {.length = 36, .msg_type = message_type_t::add_order};
  order_ref_t order_reference_number;
  buy_sell_t  buy_sell_indicator;
  shares_t    shares;
  stock_t     stock;
  price4_t    price;
};

/// Add Order with MPID Attribution (spec §1.3.2).
struct [[=rbe::pack_be]] AddOrderMPID {
  Header      header {.length = 40, .msg_type = message_type_t::add_order_mpid};
  order_ref_t order_reference_number;
  buy_sell_t  buy_sell_indicator;
  shares_t    shares;
  stock_t     stock;
  price4_t    price;
  mpid_t      attribution;
};

// --- Modify Order messages (spec §1.4) -------------------------------

/// Order Executed Message (spec §1.4.1).
struct [[=rbe::pack_be]] OrderExecuted {
  Header         header {.length = 31, .msg_type = message_type_t::order_executed};
  order_ref_t    order_reference_number;
  shares_t       executed_shares;
  match_number_t match_number;
};

/// Order Executed With Price Message (spec §1.4.2). May be marked
/// non-printable when the shares are rolled into a later bulk print.
struct [[=rbe::pack_be]] OrderExecutedWithPrice {
  Header         header {.length = 36, .msg_type = message_type_t::order_executed_with_price};
  order_ref_t    order_reference_number;
  shares_t       executed_shares;
  match_number_t match_number;
  printable_t    printable;
  price4_t       execution_price;
};

/// Order Cancel Message — partial cancellation (spec §1.4.3).
struct [[=rbe::pack_be]] OrderCancel {
  Header      header {.length = 23, .msg_type = message_type_t::order_cancel};
  order_ref_t order_reference_number;
  shares_t    cancelled_shares;
};

/// Order Delete Message — full cancellation (spec §1.4.4).
struct [[=rbe::pack_be]] OrderDelete {
  Header      header {.length = 19, .msg_type = message_type_t::order_delete};
  order_ref_t order_reference_number;
};

/// Order Replace Message (spec §1.4.5). Side, stock and MPID are not
/// carried — firms should retain them from the original Add Order.
struct [[=rbe::pack_be]] OrderReplace {
  Header      header {.length = 35, .msg_type = message_type_t::order_replace};
  order_ref_t original_order_reference_number;
  order_ref_t new_order_reference_number;
  shares_t    shares;
  price4_t    price;
};

// --- Trade messages (spec §1.5) --------------------------------------

/// Trade Message (Non-Cross) (spec §1.5.1). Emitted for non-displayable
/// order matches. `order_reference_number` is always zero (effective
/// 2010-12-06) and `buy_sell_indicator` is always `buy` (effective
/// 2014-07-14).
struct [[=rbe::pack_be]] Trade {
  Header         header {.length = 44, .msg_type = message_type_t::trade};
  order_ref_t    order_reference_number;
  buy_sell_t     buy_sell_indicator;
  shares_t       shares;
  stock_t        stock;
  price4_t       price;
  match_number_t match_number;
};

/// Cross Trade Message (spec §1.5.2). Sent after the Opening, Closing
/// and EMC cross events for every active issue.
struct [[=rbe::pack_be]] CrossTrade {
  Header         header {.length = 40, .msg_type = message_type_t::cross_trade};
  shares64_t     shares;
  stock_t        stock;
  price4_t       cross_price;
  match_number_t match_number;
  cross_type_t   cross_type;
};

/// Broken Trade / Order Execution Message (spec §1.5.3). References
/// the match number of a previous execution or trade.
struct [[=rbe::pack_be]] BrokenTrade {
  Header         header {.length = 19, .msg_type = message_type_t::broken_trade};
  match_number_t match_number;
};

// --- NOII (spec §1.6) ------------------------------------------------

/// Net Order Imbalance Indicator (spec §1.6).
struct [[=rbe::pack_be]] NOII {
  Header                      header {.length = 50, .msg_type = message_type_t::noii};
  shares64_t                  paired_shares;
  shares64_t                  imbalance_shares;
  imbalance_direction_t       imbalance_direction;
  stock_t                     stock;
  price4_t                    far_price;
  price4_t                    near_price;
  price4_t                    current_reference_price;
  cross_type_t                cross_type;
  price_variation_indicator_t price_variation_indicator;
};

// --- RPII (spec §1.7) ------------------------------------------------

/// Retail Price Improvement Indicator (spec §1.7).
struct [[=rbe::pack_be]] RPII {
  Header          header {.length = 20, .msg_type = message_type_t::rpii};
  stock_t         stock;
  interest_flag_t interest_flag;
};

// --- DLCR (spec §1.8) ------------------------------------------------

/// Direct Listing with Capital Raise Price Discovery Message (spec §1.8).
/// Disseminated once per second after the DLCR volatility test passes.
struct [[=rbe::pack_be]] DLCR {
  Header                    header {.length = 48, .msg_type = message_type_t::dlcr};
  stock_t                   stock;
  open_eligibility_status_t open_eligibility_status;
  price4_t                  minimum_allowable_price;   ///< 20% below Registration Statement Lower Price.
  price4_t                  maximum_allowable_price;   ///< 80% above Registration Statement Highest Price.
  price4_t                  near_execution_price;      ///< Current reference price when volatility test passed.
  std::uint64_t             near_execution_time;       ///< Time at which the near execution price was set.
  price4_t                  lower_price_range_collar;  ///< 10% below the Near Execution Price.
  price4_t                  upper_price_range_collar;  ///< 10% above the Near Execution Price.
};

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Type-erased dispatch — use with rbe::any_msg<nasdaq::messages>
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::msg_list<
    SystemEvent, StockDirectory, StockTradingAction, RegSHORestriction, MarketParticipantPosition, MWCBDeclineLevel,
    MWCBStatus, IPOQuotingPeriodUpdate, LULDAuctionCollar, OperationalHalt, AddOrder, AddOrderMPID, OrderExecuted,
    OrderExecutedWithPrice, OrderCancel, OrderDelete, OrderReplace, Trade, CrossTrade, BrokenTrade, NOII, RPII, DLCR>;

} // namespace nasdaq
