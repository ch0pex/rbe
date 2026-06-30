/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file fmt.hpp
 * @date 27/06/2026
 * @brief Short description
 *
 * Longer description
 */

#pragma once

// --- Includes ---
#include <rbe/core/layout.hpp>
#include <rbe/detail/introspection.hpp>

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---
#include <format>
#include <ostream>
#include <ranges>
#include <string>

// --- System ---

namespace rbe {

template<rbe::introspectable T>
  requires(not std::is_array_v<T>)
std::ostream& operator<<(std::ostream& os, T const& val) {
  auto const formatted = std::format("{}", val);
  os.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
  return os;
}

} // namespace rbe

inline std::size_t& fmt_depth() {
  thread_local std::size_t depth = 0;
  return depth;
}

struct universal_formatter {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }

  template<typename T>
  auto format(T const& t, auto& fmt_ctx) const {
    auto& depth = fmt_depth();
    std::string const pad(depth * 2, ' ');
    std::string const inner((depth + 1) * 2, ' ');

    auto out = std::format_to(fmt_ctx.out(), "{} {{", has_identifier(^^T) ? identifier_of(^^T) : "(unnamed-type)");

    ++depth;

    static constexpr auto ctx     = std::meta::access_context::unchecked();
    static constexpr auto bases   = define_static_array(bases_of(^^T, ctx));
    static constexpr auto members = define_static_array(rbe::detail::nsdm(^^T, ctx));

    template for (constexpr auto base: bases) {
      out = std::format_to(out, "\n{}{},", inner, static_cast<typename[:type_of(base):] const&>(t));
    }

    template for (constexpr auto mem: members) {
      std::string_view mem_label = has_identifier(mem) ? identifier_of(mem) : "(unnamed-member)";
      out                        = std::format_to(out, "\n{}.{} = {},", inner, mem_label, t.[:mem:]);
    }

    if (members.size() > 0 or bases.size() > 0) {
      out = std::format_to(out, "\n{}}}", pad);
    }
    else {
      *out++ = '}';
    }

    --depth;
    return out;
  }
};

template<>
struct std::formatter<std::endian> {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }
  auto format(std::endian e, auto& ctx) const {
    return std::format_to(ctx.out(), "{}", e == std::endian::little ? "little" : "big");
  }
};

template<rbe::introspectable T>
  requires(not std::ranges::range<T>)
struct std::formatter<T> : universal_formatter { };
