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
 * each of which prefixes every payload with its own length.  That
 * layout is modelled as two nested frames: `SoupHeader`, a 2-byte
 * big-endian length prefix that excludes itself (`rbe::payload_length`),
 * wraps one ITCH message, which is the 11-byte `ItchHeader` (message
 * type + stock locate + tracking number + 6-byte nanosecond timestamp)
 * followed by the message selected by its type. See `message` and
 * `packet` at the end.
 */
#pragma once

#include <rbe/rbe.hpp>

#include <cstdint>
#include <tuple>

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
/// TODO: support 48-bit unsigned integer type
// TODO: use rbe::uint48_t once it exists; until then the 6 bytes are kept raw.
// using timestamp_t = rbe::uint48_t;
using timestamp_t = std::array<std::uint8_t, 6>;

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

/// SoupBinTCP length prefix of every framed ITCH message: the length of
/// the ITCH message that follows, excluding this prefix.
struct[[= rbe::pack_be]] SoupHeader {
  [[= rbe::payload_length]] std::uint16_t length {};
};

/// 11-byte common header of every ITCH message.
///
/// Layout on the wire:
///   [0..1)  msg_type        — one-byte ASCII message-type code
///   [1..3)  stock_locate    — dynamically assigned locate code (0 = not stock-dependent)
///   [3..5)  tracking_number — Nasdaq internal tracking number
///   [5..11) timestamp       — nanoseconds since midnight (Eastern Time)
///
/// Declared once for the whole protocol and composed with the message set
/// in `message`: `msg_type` selects the message whose `rbe::id(value)` matches.
struct[[= rbe::pack_be]] ItchHeader {
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

struct [[=rbe::pack_be, =rbe::id(message_type_t::system_event)]] SystemEvent {
  system_event_t event_code;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<SystemEvent>() == 12);

// --- Stock-related messages (spec §1.2) ------------------------------

/// Stock Directory (spec §1.2.1). Disseminated at the start of the day
/// for every active symbol; occasionally intraday for corrections.
struct [[=rbe::pack_be, =rbe::id(message_type_t::stock_directory)]] StockDirectory {
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
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<StockDirectory>() == 39);

/// Stock Trading Action (spec §1.2.2).
struct [[=rbe::pack_be, =rbe::id(message_type_t::stock_trading_action)]] StockTradingAction {
  stock_t                 stock;
  trading_state_t         trading_state;
  std::uint8_t            reserved;
  trading_action_reason_t reason;              ///< See spec Appendix C.
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<StockTradingAction>() == 25);

/// Reg SHO Short Sale Price Test Restricted Indicator (spec §1.2.3).
struct [[=rbe::pack_be, =rbe::id(message_type_t::reg_sho_restriction)]] RegSHORestriction {
  stock_t          stock;
  reg_sho_action_t reg_sho_action;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<RegSHORestriction>() == 20);

/// Market Participant Position (spec §1.2.4).
struct [[=rbe::pack_be, =rbe::id(message_type_t::market_participant_position)]] MarketParticipantPosition {
  mpid_t                     mpid;
  stock_t                    stock;
  primary_market_maker_t     primary_market_maker;
  market_maker_mode_t        market_maker_mode;
  market_participant_state_t market_participant_state;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<MarketParticipantPosition>() == 26);

/// MWCB Decline Level Message (spec §1.2.5.1). Stock Locate always 0.
struct [[=rbe::pack_be, =rbe::id(message_type_t::mwcb_decline_level)]] MWCBDeclineLevel {
  price8_t  level_1;
  price8_t  level_2;
  price8_t  level_3;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<MWCBDeclineLevel>() == 35);

/// MWCB Status Message (spec §1.2.5.2). Stock Locate always 0.
struct [[=rbe::pack_be, =rbe::id(message_type_t::mwcb_status)]] MWCBStatus {
  mwcb_level_t breached_level;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<MWCBStatus>() == 12);

/// IPO Quoting Period Update (spec §1.2.6). Stock Locate always 0.
struct [[=rbe::pack_be, =rbe::id(message_type_t::ipo_quoting_period_update)]] IPOQuotingPeriodUpdate {
  stock_t                           stock;
  std::uint32_t                     ipo_quotation_release_time;  ///< Seconds since midnight; 0 if cancelled.
  ipo_quotation_release_qualifier_t ipo_quotation_release_qualifier;
  price4_t                          ipo_price;                   ///< 0 if quotation cancelled/postponed.
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<IPOQuotingPeriodUpdate>() == 28);

/// LULD Auction Collar (spec §1.2.7).
struct [[=rbe::pack_be, =rbe::id(message_type_t::luld_auction_collar)]] LULDAuctionCollar {
  stock_t       stock;
  price4_t      auction_collar_reference_price;
  price4_t      upper_auction_collar_price;
  price4_t      lower_auction_collar_price;
  std::uint32_t auction_collar_extension;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<LULDAuctionCollar>() == 35);

/// Operational Halt (spec §1.2.8).
struct [[=rbe::pack_be, =rbe::id(message_type_t::operational_halt)]] OperationalHalt {
  stock_t                   stock;
  market_code_t             market_code;
  operational_halt_action_t operational_halt_action;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OperationalHalt>() == 21);

// --- Add Order messages (spec §1.3) ----------------------------------

/// Add Order — No MPID Attribution (spec §1.3.1).
struct [[=rbe::pack_be, =rbe::id(message_type_t::add_order)]] AddOrder {
  order_ref_t order_reference_number;
  buy_sell_t  buy_sell_indicator;
  shares_t    shares;
  stock_t     stock;
  price4_t    price;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<AddOrder>() == 36);

/// Add Order with MPID Attribution (spec §1.3.2).
struct [[=rbe::pack_be, =rbe::id(message_type_t::add_order_mpid)]] AddOrderMPID {
  order_ref_t order_reference_number;
  buy_sell_t  buy_sell_indicator;
  shares_t    shares;
  stock_t     stock;
  price4_t    price;
  mpid_t      attribution;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<AddOrderMPID>() == 40);

// --- Modify Order messages (spec §1.4) -------------------------------

/// Order Executed Message (spec §1.4.1).
struct [[=rbe::pack_be, =rbe::id(message_type_t::order_executed)]] OrderExecuted {
  order_ref_t    order_reference_number;
  shares_t       executed_shares;
  match_number_t match_number;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OrderExecuted>() == 31);

/// Order Executed With Price Message (spec §1.4.2). May be marked
/// non-printable when the shares are rolled into a later bulk print.
struct [[=rbe::pack_be, =rbe::id(message_type_t::order_executed_with_price)]] OrderExecutedWithPrice {
  order_ref_t    order_reference_number;
  shares_t       executed_shares;
  match_number_t match_number;
  printable_t    printable;
  price4_t       execution_price;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OrderExecutedWithPrice>() == 36);

/// Order Cancel Message — partial cancellation (spec §1.4.3).
struct [[=rbe::pack_be, =rbe::id(message_type_t::order_cancel)]] OrderCancel {
  order_ref_t order_reference_number;
  shares_t    cancelled_shares;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OrderCancel>() == 23);

/// Order Delete Message — full cancellation (spec §1.4.4).
struct [[=rbe::pack_be, =rbe::id(message_type_t::order_delete)]] OrderDelete {
  order_ref_t order_reference_number;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OrderDelete>() == 19);

/// Order Replace Message (spec §1.4.5). Side, stock and MPID are not
/// carried — firms should retain them from the original Add Order.
struct [[=rbe::pack_be, =rbe::id(message_type_t::order_replace)]] OrderReplace {
  order_ref_t original_order_reference_number;
  order_ref_t new_order_reference_number;
  shares_t    shares;
  price4_t    price;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<OrderReplace>() == 35);

// --- Trade messages (spec §1.5) --------------------------------------

/// Trade Message (Non-Cross) (spec §1.5.1). Emitted for non-displayable
/// order matches. `order_reference_number` is always zero (effective
/// 2010-12-06) and `buy_sell_indicator` is always `buy` (effective
/// 2014-07-14).
struct [[=rbe::pack_be, =rbe::id(message_type_t::trade)]] Trade {
  order_ref_t    order_reference_number;
  buy_sell_t     buy_sell_indicator;
  shares_t       shares;
  stock_t        stock;
  price4_t       price;
  match_number_t match_number;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<Trade>() == 44);

/// Cross Trade Message (spec §1.5.2). Sent after the Opening, Closing
/// and EMC cross events for every active issue.
struct [[=rbe::pack_be, =rbe::id(message_type_t::cross_trade)]] CrossTrade {
  shares64_t     shares;
  stock_t        stock;
  price4_t       cross_price;
  match_number_t match_number;
  cross_type_t   cross_type;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<CrossTrade>() == 40);

/// Broken Trade / Order Execution Message (spec §1.5.3). References
/// the match number of a previous execution or trade.
struct [[=rbe::pack_be, =rbe::id(message_type_t::broken_trade)]] BrokenTrade {
  match_number_t match_number;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<BrokenTrade>() == 19);

// --- NOII (spec §1.6) ------------------------------------------------

/// Net Order Imbalance Indicator (spec §1.6).
struct [[=rbe::pack_be, =rbe::id(message_type_t::noii)]] NOII {
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
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<NOII>() == 50);

// --- RPII (spec §1.7) ------------------------------------------------

/// Retail Price Improvement Indicator (spec §1.7).
struct [[=rbe::pack_be, =rbe::id(message_type_t::rpii)]] RPII {
  stock_t         stock;
  interest_flag_t interest_flag;
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<RPII>() == 20);

// --- DLCR (spec §1.8) ------------------------------------------------

/// Direct Listing with Capital Raise Price Discovery Message (spec §1.8).
/// Disseminated once per second after the DLCR volatility test passes.
struct [[=rbe::pack_be, =rbe::id(message_type_t::dlcr)]] DLCR {
  stock_t                   stock;
  open_eligibility_status_t open_eligibility_status;
  price4_t                  minimum_allowable_price;   ///< 20% below Registration Statement Lower Price.
  price4_t                  maximum_allowable_price;   ///< 80% above Registration Statement Highest Price.
  price4_t                  near_execution_price;      ///< Current reference price when volatility test passed.
  std::uint64_t             near_execution_time;       ///< Time at which the near execution price was set.
  price4_t                  lower_price_range_collar;  ///< 10% below the Near Execution Price.
  price4_t                  upper_price_range_collar;  ///< 10% above the Near Execution Price.
};
static_assert(rbe::wire_size_of<ItchHeader>() + rbe::wire_size_of<DLCR>() == 48);

// clang-format on

// ─────────────────────────────────────────────────────────────────────
// Framing
// ─────────────────────────────────────────────────────────────────────

using messages = rbe::any<
    SystemEvent, StockDirectory, StockTradingAction, RegSHORestriction, MarketParticipantPosition, MWCBDeclineLevel,
    MWCBStatus, IPOQuotingPeriodUpdate, LULDAuctionCollar, OperationalHalt, AddOrder, AddOrderMPID, OrderExecuted,
    OrderExecutedWithPrice, OrderCancel, OrderDelete, OrderReplace, Trade, CrossTrade, BrokenTrade, NOII, RPII, DLCR>;

/// One ITCH message: `ItchHeader` followed by the message selected by `msg_type`.
/// ITCH carries no length of its own: it is bounded by the enclosing `packet`.
using message = rbe::frame<ItchHeader, messages>;

/// One SoupBinTCP packet: `SoupHeader` followed by exactly one ITCH message.
using packet = rbe::frame<SoupHeader, message>;

} // namespace nasdaq
