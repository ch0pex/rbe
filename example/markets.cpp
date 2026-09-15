/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file markets.cpp
 * @date 06/08/2026
 * @brief Central include point aggregating the market data protocol headers
 */

// --- Includes ---

#include "markets/aquis.hpp"
#include "markets/cboe.hpp"
#include "markets/london.hpp"
#include "markets/nasdaq.hpp"
#include "markets/opra.hpp"

// --- STD ---

// --- System ---

// Every protocol composes into frames, classified at compile time by how they are delimited.
static_assert(rbe::self_delimiting_frame<aquis::message>);
static_assert(rbe::buffer_delimited_frame<aquis::packet>); // PacketHeader has no length: the UDP datagram bounds it
static_assert(rbe::self_delimiting_frame<cboe::pitch::message>);
static_assert(rbe::self_delimiting_frame<cboe::pitch::packet>);
static_assert(rbe::buffer_delimited_frame<cboe::top::message>); // TODO: self-delimiting once rbe::any resolves implied lengths
static_assert(rbe::self_delimiting_frame<lse::message>);
static_assert(rbe::self_delimiting_frame<lse::packet>);
static_assert(rbe::buffer_delimited_frame<nasdaq::message>); // bounded by the SoupBinTCP packet
static_assert(rbe::self_delimiting_frame<nasdaq::packet>);
static_assert(rbe::buffer_delimited_frame<opra::message>); // TODO: self-delimiting once implied lengths are expressible

int main() {
  // This file does not contain any executable logic: it includes the market data protocol headers and checks at
  // compile time that they compose into frames.
  return 0;
}
