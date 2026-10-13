#!/usr/bin/env python3
"""Generates an IDE stub of rbe from the single header.

rbe is built on C++26 reflection, which clang (and therefore clangd) does not implement: opening a file that includes
rbe floods the editor with errors. The stub is a drop-in replacement *for tooling only*: it declares the same public
API with every reflection construct taken out, so clangd can parse, complete and navigate code that uses rbe. It is
never meant to run, and it cannot answer the questions reflection answers (what a field is called, its wire size...).

How each declaration is lowered:
  - functions taking or returning `std::meta::info` are removed, they cannot be spelled without `^^` anyway
  - a body with an explicit return type becomes `return rbe::stub::fake<R>();`, a `void` one becomes `{}`
  - a deduced body is kept if it is reflection free, otherwise it returns `rbe::stub::unknown`, a value that converts
    to and compares with anything
  - a namespace scope `std::size_t` function or variable (`wire_size_of`, `static_length`...) yields
    `rbe::stub::size`, an integral that satisfies any comparison, so `static_assert(wire_size_of<T>() == 8)` holds
  - a concept that needs reflection is `true`, an alias that needs it is `rbe::stub::unknown`
  - a requires clause that needs reflection is `true`, or `rbe::stub::reflected<T>` on partial specializations and
    namespace scope operators, where `true` would hijack every type (`std::formatter`, `operator<<`)
  - static_asserts and consteval blocks are removed, and annotations on the library's own declarations are stripped

Annotations cannot be spelled at all under clang: code read through the stub has to hide them behind a macro of its
own, empty under the stub and `[[= ...]]` otherwise.

    scripts/stubgen.py                 # single_include/rbe/rbe.hpp -> single_include/rbe/rbe_stub.hpp
    scripts/stubgen.py --check         # also runs clang -fsyntax-only over the output
"""

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STUB_NS = "::rbe::stub"

# ------------------------------------------------------------------------------------------------------------------
# Tokenizer
# ------------------------------------------------------------------------------------------------------------------

PUNCTUATORS = sorted(
    [
        "...", "<=>", "->*", "<<=", ">>=", "::", "->", "^^", "[:", ":]", "==", "!=", "<=", ">=", "&&", "||", "++",
        "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ".*",
    ],
    key=len,
    reverse=True,
)
IDENT = re.compile(r"[A-Za-z_]\w*")
NUMBER = re.compile(r"\.?\d(?:[eEpP][+-]|['\w.])*")
STRING_PREFIX = re.compile(r'(?:u8|u|U|L)?R?"|(?:u8|u|U|L)?\'')


@dataclass
class Tok:
    text: str
    kind: str  # ident, number, string, punct, pp
    start: int
    end: int


def tokenize(src: str) -> list[Tok]:
    toks: list[Tok] = []
    i, n = 0, len(src)
    line_start = True
    while i < n:
        c = src[i]
        if c == "\n":
            line_start = True
            i += 1
            continue
        if c in " \t\r\f\v":
            i += 1
            continue
        if src.startswith("//", i):
            i = src.find("\n", i)
            i = n if i < 0 else i
            continue
        if src.startswith("/*", i):
            i = src.index("*/", i + 2) + 2
            continue
        if c == "#" and line_start:
            j = i
            while True:
                k = src.find("\n", j)
                k = n if k < 0 else k
                if src[k - 1] == "\\" and k < n:
                    j = k + 1
                    continue
                break
            toks.append(Tok(src[i:k], "pp", i, k))
            i = k
            continue
        line_start = False
        if (m := STRING_PREFIX.match(src, i)) and (c in "\"'" or not IDENT.match(src, i) or m.end() - i <= 3):
            prefix = m.group(0)
            if prefix.endswith('R"'):
                delim_end = src.index("(", m.end())
                delim = src[m.end():delim_end]
                close = src.index(")" + delim + '"', delim_end)
                j = close + len(delim) + 2
            else:
                quote = prefix[-1]
                j = m.end()
                while src[j] != quote:
                    j += 2 if src[j] == "\\" else 1
                j += 1
            toks.append(Tok(src[i:j], "string", i, j))
            i = j
            continue
        if m := IDENT.match(src, i):
            toks.append(Tok(m.group(0), "ident", i, m.end()))
            i = m.end()
            continue
        if m := NUMBER.match(src, i):
            toks.append(Tok(m.group(0), "number", i, m.end()))
            i = m.end()
            continue
        for p in PUNCTUATORS:
            if src.startswith(p, i):
                toks.append(Tok(p, "punct", i, i + len(p)))
                i += len(p)
                break
        else:
            toks.append(Tok(c, "punct", i, i + 1))
            i += 1
    return toks


# ------------------------------------------------------------------------------------------------------------------
# Declaration parser: just enough structure to tell namespaces, classes, functions, variables... apart
# ------------------------------------------------------------------------------------------------------------------

OPEN = {"(": ")", "[": "]", "{": "}", "[:": ":]"}
CLOSE = {v: k for k, v in OPEN.items()}
SPECIFIERS = {"inline", "constexpr", "consteval", "constinit", "static", "friend", "virtual", "explicit", "extern"}


@dataclass
class Decl:
    kind: str
    start: int  # token range [start, end)
    end: int
    children: list["Decl"] = field(default_factory=list)
    body_open: int = -1  # namespace / class / function body braces
    body_close: int = -1
    template_heads: list[tuple[int, int]] = field(default_factory=list)
    requires: list[tuple[int, int]] = field(default_factory=list)  # requires clauses, expression range
    name: str = ""
    head_start: int = -1  # first token after template heads and the leading requires clause
    params: tuple[int, int] = (-1, -1)  # parenthesis tokens of a function
    trailing_return: tuple[int, int] = (-1, -1)
    mem_init: tuple[int, int] = (-1, -1)
    eq: int = -1  # `=` of a variable / alias / concept


class Parser:
    def __init__(self, toks: list[Tok]):
        self.t = toks

    def text(self, i: int) -> str:
        return self.t[i].text if i < len(self.t) else ""

    def match(self, i: int) -> int:
        """Index of the bracket closing the one at i."""
        stack = [OPEN[self.t[i].text]]
        i += 1
        while stack:
            s = self.t[i].text
            if s in OPEN:
                stack.append(OPEN[s])
            elif s in CLOSE:
                if s != stack[-1]:
                    raise SyntaxError(f"unbalanced '{s}' at offset {self.t[i].start}")
                stack.pop()
            i += 1
        return i - 1

    def match_angle(self, i: int) -> int:
        """Index of the '>' closing the '<' at i (template argument / parameter lists only)."""
        depth = 0
        while True:
            s = self.text(i)
            if s in OPEN:
                i = self.match(i)
            elif s == "<":
                depth += 1
            elif s == ">":
                depth -= 1
                if depth == 0:
                    return i
            elif s in (";", "{", "}"):
                raise SyntaxError(f"unterminated template argument list at offset {self.t[i].start}")
            i += 1

    def skip_constraint(self, i: int) -> int:
        """End of a requires clause's constraint expression starting at i."""
        while True:
            while self.text(i) in ("not", "!"):
                i += 1
            if self.text(i) == "(":
                i = self.match(i) + 1
            else:
                if self.text(i) == "::":
                    i += 1
                while True:
                    if self.t[i].kind != "ident":
                        raise SyntaxError(f"cannot parse constraint at offset {self.t[i].start}")
                    i += 1
                    if self.text(i) == "<":
                        i = self.match_angle(i) + 1
                    if self.text(i) == "::":
                        i += 1
                        continue
                    break
            if self.text(i) in ("and", "or", "&&", "||"):
                i += 1
                continue
            return i

    def skip_angles_until(self, i: int, stops: set[str]) -> int:
        """First token in `stops` at bracket depth 0, treating `<...>` after a name as brackets."""
        while True:
            s = self.text(i)
            if s in stops:
                return i
            if s in OPEN:
                i = self.match(i) + 1
                continue
            if s == "<" and i > 0 and (self.t[i - 1].kind == "ident" or self.t[i - 1].text == ">"):
                if self.t[i - 1].text != "operator":
                    i = self.match_angle(i) + 1
                    continue
            if s in ("}", ")"):
                raise SyntaxError(f"unexpected '{s}' at offset {self.t[i].start}")
            i += 1

    def scope(self, i: int, end_text: str | None) -> tuple[list[Decl], int]:
        decls: list[Decl] = []
        while i < len(self.t):
            if self.text(i) == end_text:
                return decls, i
            d = self.decl(i)
            decls.append(d)
            i = d.end
        if end_text is not None:
            raise SyntaxError("unterminated scope")
        return decls, i

    def decl(self, i: int) -> Decl:
        t = self.t
        start = i
        tok = t[i]
        if tok.kind == "pp":
            return Decl("pp", i, i + 1)
        if tok.text == ";":
            return Decl("empty", i, i + 1)
        if tok.text in ("public", "private", "protected") and self.text(i + 1) == ":":
            return Decl("access", i, i + 2, name=tok.text)
        if tok.text == "consteval" and self.text(i + 1) == "{":
            return Decl("consteval_block", i, self.match(i + 1) + 1)
        if tok.text == "static_assert":
            return Decl("static_assert", i, self.match(i + 1) + 2)

        d = Decl("?", start, -1)
        while self.text(i) == "template" and self.text(i + 1) == "<":
            close = self.match_angle(i + 1)
            d.template_heads.append((i, close + 1))
            i = close + 1
        if self.text(i) == "requires":
            j = self.skip_constraint(i + 1)
            d.requires.append((i + 1, j))
            i = j
        d.head_start = i
        s = self.text(i)

        if s == "namespace":
            j = i + 1
            while self.text(j) != "{":
                if self.text(j) in ("=", ";"):  # namespace alias
                    k = self.skip_angles_until(j, {";"})
                    d.kind, d.end = "alias", k + 1
                    return d
                j += 1
            d.kind, d.name = "namespace", "".join(x.text for x in t[i + 1:j])
            d.body_open = j
            d.children, d.body_close = self.scope(j + 1, "}")
            d.end = d.body_close + 1
            return d

        if s == "concept":
            d.kind, d.name, d.eq = "concept", self.text(i + 1), i + 2
            d.end = self.skip_angles_until(i + 2, {";"}) + 1
            return d

        if s == "using":
            d.kind = "alias" if self.text(i + 2) == "=" else "using"
            d.name = self.text(i + 1)
            d.eq = i + 2 if d.kind == "alias" else -1
            d.end = self.skip_angles_until(i, {";"}) + 1
            return d

        if s == "enum":
            d.kind = "enum"
            d.end = self.skip_angles_until(i, {";"}) + 1
            return d

        j = i
        while self.text(j) in SPECIFIERS or self.text(j) == "[":
            j = self.match(j) + 1 if self.text(j) == "[" else j + 1
        if self.text(j) in ("class", "struct", "union"):
            k = self.skip_angles_until(j, {";", "{", "("})
            if self.text(k) == ";":
                d.kind, d.end = "class_decl", k + 1
                return d
            if self.text(k) == "{":
                d.kind = "class"
                name_end = self.skip_angles_until(j + 1, {":", "{", "final"})
                d.name = next(
                    (x.text for x in reversed(t[j + 1:name_end]) if x.kind == "ident" and x.text not in SPECIFIERS),
                    "",
                )
                d.body_open = k
                d.children, d.body_close = self.scope(k + 1, "}")
                d.end = self.skip_angles_until(d.body_close + 1, {";"}) + 1
                return d

        # function or variable
        k = self.skip_angles_until(i, {";", "{", "(", "="})
        if self.text(k) == "(" and self.text(k - 1) == "operator":  # operator()
            k = self.skip_angles_until(self.match(k) + 1, {";", "{", "(", "="})
        if self.text(k) == "(":  # rbe never declares a variable as `T name(args);`
            return self.function(d, k)
        d.kind = "variable"
        d.eq = k if self.text(k) == "=" else -1
        d.end = self.skip_angles_until(k, {";"}) + 1
        return d

    def function(self, d: Decl, lp: int) -> Decl:
        t = self.t
        d.kind = "function"
        d.params = (lp, self.match(lp))
        # operator names: operator==, operator(), operator T
        op = next((x for x in range(d.head_start, lp) if t[x].text == "operator"), -1)
        if op >= 0:
            d.name = "operator" + "".join(x.text for x in t[op + 1:lp])
        else:
            d.name = t[lp - 1].text
            if t[lp - 2].text == "~":
                d.name = "~" + d.name
        i = d.params[1] + 1
        # cv / ref / noexcept / attributes
        while True:
            s = self.text(i)
            if s in ("const", "volatile", "&", "&&", "override", "final", "mutable"):
                i += 1
            elif s == "noexcept":
                i += 1
                if self.text(i) == "(":
                    i = self.match(i) + 1
            elif s == "[":
                i = self.match(i) + 1
            else:
                break
        if self.text(i) == "->":
            r = self.skip_angles_until(i + 1, {"{", ";", "=", "requires", "try"})
            d.trailing_return = (i + 1, r)
            i = r
        if self.text(i) == "requires":
            j = self.skip_constraint(i + 1)
            d.requires.append((i + 1, j))
            i = j
        s = self.text(i)
        try_tok = i if s == "try" else -1
        if try_tok >= 0:
            i += 1
            s = self.text(i)
        if s == ":":  # member initializers: `name(args)` or `name{args}`, comma separated
            j = i + 1
            while True:
                while self.text(j) not in ("(", "{"):
                    j = self.match_angle(j) + 1 if self.text(j) == "<" else j + 1
                j = self.match(j) + 1
                if self.text(j) != ",":
                    break
                j += 1
            d.mem_init = (try_tok if try_tok >= 0 else i, j)
            i = j
            s = self.text(i)
        if s == "{":
            d.body_open, d.body_close = i, self.match(i)
            if try_tok >= 0:  # function-try-block: the handlers are part of the body
                if d.mem_init[0] < 0:
                    d.body_open = try_tok
                while self.text(d.body_close + 1) == "catch":
                    d.body_close = self.match(self.match(d.body_close + 2) + 1)
            d.end = d.body_close + 1
            if self.text(d.end) == ";":
                d.end += 1
        elif s == "=":  # = default / = delete
            d.end = self.skip_angles_until(i, {";"}) + 1
        elif s == ";":
            d.end = i + 1
        else:
            raise SyntaxError(f"cannot parse function '{d.name}' at offset {self.t[i].start}")
        return d


# ------------------------------------------------------------------------------------------------------------------
# Lowering
# ------------------------------------------------------------------------------------------------------------------

REFLECTION_IDENTS = {
    "meta", "define_static_array", "define_static_string", "define_static_object", "define_aggregate",
    "substitute", "extract", "reflect_constant", "reflect_object", "reflect_function",
}

# Bodies that must keep working beyond returning `unknown`: (class, function) -> replacement body
BODY_OVERRIDES = {
    ("universal_formatter", "format"): "{ return fmt_ctx.out(); }",
    ("proxy", "field"): f"{{ return {STUB_NS}::field_value {{}}; }}",
}

# Declarations replaced wholesale, by qualified name.
DECL_OVERRIDES = {
    # a structural type usable as `field<"name">()`: it has to own its characters
    "rbe::static_string": """struct static_string {
  using target_type = std::string_view;
  char value[256] {};
  std::size_t length = 0;
  template<std::size_t N>
  consteval static_string(char const (&s)[N]) : length(N - 1) {
    for (std::size_t i = 0; i < N - 1; ++i)
      value[i] = s[i];
  }
  consteval static_string(std::string_view const s) : length(s.size()) {
    for (std::size_t i = 0; i < s.size(); ++i)
      value[i] = s[i];
  }
  constexpr operator target_type() const { return {value, length}; }
  constexpr auto get() const -> target_type { return {value, length}; }
  constexpr auto operator*() const -> target_type { return get(); }
  friend constexpr auto operator==(static_string const&, static_string const&) -> bool = default;
};

namespace stub {

/// A field read through a proxy: the member's value, or the proxy of a nested class, whichever is asked of it.
struct field_value : unknown {
  using unknown::unknown;
  constexpr auto value() const noexcept -> unknown { return {}; }
  constexpr auto operator*() const noexcept -> unknown { return {}; }
  template<static_string First, static_string... Rest>
  constexpr auto field() const noexcept -> field_value { return {}; }
  template<std::size_t Index>
  constexpr auto field() const noexcept -> field_value { return {}; }
  template<auto Annotation>
    requires(not std::integral<decltype(Annotation)>)
  constexpr auto field() const noexcept -> field_value { return {}; }
  constexpr auto size() const noexcept -> std::size_t { return 0; }
  constexpr auto buffer() const noexcept -> std::span<std::byte const> { return {}; }
  constexpr auto as_span() const noexcept -> std::span<std::byte const> { return {}; }
  constexpr auto data() const noexcept -> std::byte const* { return nullptr; }
};

} // namespace stub
} // namespace rbe

template<>
struct std::formatter<rbe::stub::field_value> : std::formatter<rbe::stub::unknown> { };

namespace rbe {""",
    # how a frame is delimited is read off its annotations: every frame gets a tag that passes for any category
    "rbe::frames::frame_delimiting_tag": f"""template<header HeaderType, typename PayloadType>
using frame_delimiting_tag = {STUB_NS}::delimiting;""",
}

# Concepts whose reflection-free approximation is better than `true`, by qualified name: the new constraint expression.
CONCEPT_OVERRIDES = {
    # wirable: a primitive, or a non empty aggregate (reflection also checks every member, recursively)
    "rbe::trivially_wirable_primitive": "std::integral<T> or std::is_enum_v<T>",
    "rbe::custom_wirable": "not trivially_wirable_primitive<T> and requires { sizeof(custom<T>); }",
    "rbe::wirable": (
        "wirable_primitive<std::remove_all_extents_t<T>> or (std::is_class_v<std::remove_all_extents_t<T>> and "
        "std::is_aggregate_v<std::remove_all_extents_t<T>> and not std::is_empty_v<std::remove_all_extents_t<T>>)"
    ),
    "rbe::introspectable": f"{STUB_NS}::reflected<T>",
    "rbe::frames::delimiting_tag": f"std::derived_from<T, detail::delimiting_tag> or std::same_as<T, {STUB_NS}::delimiting>",
    "rbe::frames::detail::is_tag": (
        "requires { requires std::derived_from<typename T::delimiting_concept, Tag> or "
        f"std::same_as<typename T::delimiting_concept, {STUB_NS}::delimiting>; }}"
    ),
}

PRELUDE = f"""
#include <compare>
#include <concepts>
#include <cstddef>
#include <format>
#include <ostream>
#include <ranges>
#include <type_traits>

namespace rbe::stub {{

/// A value of type T, for bodies that are never evaluated outside the editor.
template<class T>
T declval_impl() noexcept;

template<class T>
constexpr auto fake() noexcept -> T {{
  if constexpr (std::is_void_v<T>) {{
    return;
  }}
  else if constexpr (not std::is_reference_v<T> and std::is_default_constructible_v<T>) {{
    return T {{}};
  }}
  else {{
    return declval_impl<T>();
  }}
}}

/// Whatever reflection would have computed: converts to and compares with anything.
struct unknown {{
  constexpr unknown() = default;
  template<class T>
    requires(not std::same_as<std::remove_cvref_t<T>, unknown>)
  constexpr unknown(T&&) noexcept {{ }}
  template<class T>
  constexpr operator T() const noexcept {{ return fake<T>(); }}
  constexpr auto operator*() const noexcept -> unknown {{ return {{}}; }}
  constexpr auto operator->() const noexcept -> unknown const* {{ return this; }}
  constexpr auto operator[](auto&&...) const noexcept -> unknown {{ return {{}}; }}
  constexpr auto operator()(auto&&...) const noexcept -> unknown {{ return {{}}; }}
  constexpr auto begin() const noexcept -> unknown const* {{ return this; }}
  constexpr auto end() const noexcept -> unknown const* {{ return this + 1; }}
  friend constexpr auto operator==(unknown, auto const&) noexcept -> bool {{ return true; }}
  friend constexpr auto operator<=>(unknown, auto const&) noexcept {{ return std::partial_ordering::equivalent; }}
  friend constexpr auto operator+(unknown, auto const&) noexcept -> unknown {{ return {{}}; }}
  friend constexpr auto operator-(unknown, auto const&) noexcept -> unknown {{ return {{}}; }}
  friend constexpr auto operator*(unknown, auto const&) noexcept -> unknown {{ return {{}}; }}
  friend constexpr auto operator/(unknown, auto const&) noexcept -> unknown {{ return {{}}; }}
}};

/// A size reflection would have computed: an integral that satisfies any comparison.
struct size {{
  constexpr operator std::size_t() const noexcept {{ return 0; }}
  friend constexpr auto operator==(size, std::integral auto) noexcept -> bool {{ return true; }}
  friend constexpr auto operator==(size, size) noexcept -> bool {{ return true; }}
  friend constexpr auto operator<=>(size, std::integral auto) noexcept {{ return std::partial_ordering::equivalent; }}
  friend constexpr auto operator<=>(size, size) noexcept {{ return std::partial_ordering::equivalent; }}
#define RBE_STUB_SIZE_OP(op)                                                                                          \\
  friend constexpr auto operator op(size, size) noexcept -> size {{ return {{}}; }}                                      \\
  friend constexpr auto operator op(size, std::integral auto) noexcept -> size {{ return {{}}; }}                        \\
  friend constexpr auto operator op(std::integral auto, size) noexcept -> size {{ return {{}}; }}
  RBE_STUB_SIZE_OP(+)
  RBE_STUB_SIZE_OP(-)
  RBE_STUB_SIZE_OP(*)
  RBE_STUB_SIZE_OP(/)
  RBE_STUB_SIZE_OP(%)
#undef RBE_STUB_SIZE_OP
}};

/// The delimiting category of a frame, which reflection reads off the header's annotations: passes for any category.
struct delimiting {{ }};

/// What a reflection predicate on a type would accept: an aggregate that is not a range, a user message.
template<class T>
concept reflected = std::is_aggregate_v<std::remove_cvref_t<T>> and not std::ranges::range<T>;

}} // namespace rbe::stub

template<>
struct std::formatter<rbe::stub::unknown> {{
  constexpr auto parse(auto& ctx) {{ return ctx.begin(); }}
  auto format(rbe::stub::unknown, auto& ctx) const {{ return ctx.out(); }}
}};

template<>
struct std::formatter<rbe::stub::size> : std::formatter<std::size_t> {{
  auto format(rbe::stub::size, auto& ctx) const {{ return std::formatter<std::size_t>::format(0, ctx); }}
}};
"""


class Lowering:
    def __init__(self, src: str, toks: list[Tok], decls: list[Decl]):
        self.src, self.t = src, toks
        self.dropped = self.collect_dropped(decls)
        self.used_overrides: set[str | tuple[str, str]] = set()
        self.annotations = self.find_annotations()
        self.stats = {"dropped": 0, "stubbed": 0, "kept": 0, "true": 0}

    # --- classification ----------------------------------------------------------------------------------------

    def find_annotations(self) -> list[tuple[int, int]]:
        """`[[= ...]]` attribute specifiers, removed from everything that is emitted."""
        found = []
        p = Parser(self.t)
        for i in range(len(self.t) - 2):
            if self.t[i].text == "[" and self.t[i + 1].text == "[" and self.t[i + 2].text == "=":
                found.append((i, p.match(i) + 1))
        return found

    def reflective(self, i: int, j: int) -> bool:
        t = self.t
        for k in range(i, j):
            s = t[k].text
            if s in ("^^", "[:", ":]") or (t[k].kind == "ident" and s in REFLECTION_IDENTS):
                return True
            if s == "template" and k + 1 < j and t[k + 1].text == "for":
                return True
            if s == "consteval" and k + 1 < j and t[k + 1].text == "{":
                return True
        return False

    def dirty(self, i: int, j: int, extra: set[str] = frozenset()) -> bool:
        if i < 0 or i >= j:
            return False
        if self.reflective(i, j):
            return True
        names = self.dropped | extra
        return any(self.t[k].kind == "ident" and self.t[k].text in names for k in range(i, j))

    def signature_reflective(self, d: Decl) -> bool:
        """A function that takes or returns a reflection: the whole function goes."""
        ranges = [*d.template_heads, (d.head_start, d.params[0]), (d.params[0], d.params[1] + 1), d.trailing_return]
        return any(self.reflective(a, b) for a, b in ranges if a >= 0)

    def collect_dropped(self, decls: list[Decl]) -> set[str]:
        """Names of the namespace scope entities removed: whatever mentions them needs reflection too. Members are
        tracked per class, see lower_decl."""
        names = set()
        for d in decls:
            if d.kind == "namespace":
                names |= self.collect_dropped(d.children)
            elif d.kind == "function" and self.signature_reflective(d):
                names.add(d.name)
            elif d.kind in ("variable", "alias", "concept") and any(self.reflective(*h) for h in d.template_heads):
                names.add(d.name or self.variable_name(d))
            elif d.kind == "variable" and self.reflective(d.head_start, d.eq if d.eq >= 0 else d.end):
                names.add(self.variable_name(d))
        return names

    def variable_name(self, d: Decl) -> str:
        stop = d.eq if d.eq >= 0 else d.end - 1
        idents = [k for k in range(d.head_start, stop) if self.t[k].kind == "ident"]
        if self.t[stop - 1].text == "}" and d.eq < 0:  # brace initialized: name {...}
            k = stop - 1
            while k > d.head_start and self.t[k].text != "{":
                k -= 1
            idents = [x for x in range(d.head_start, k) if self.t[x].kind == "ident"]
        return self.t[idents[-1]].text if idents else ""

    # --- rendering ---------------------------------------------------------------------------------------------

    def render(self, i: int, j: int, repl: list[tuple[int, int, str]] = ()) -> str:
        """Source text of tokens [i, j), with token ranges replaced and annotations stripped."""
        if i >= j:
            return ""
        edits = sorted([*repl, *((a, b, "") for a, b in self.annotations if a >= i and b <= j)])
        out, pos = [], self.t[i].start
        last = i
        for a, b, text in edits:
            if a < last:
                continue  # nested in an earlier edit
            out.append(self.src[pos:self.t[a].start])
            out.append(text)
            pos = self.t[b - 1].end
            last = b
        out.append(self.src[pos:self.t[j - 1].end])
        return "".join(out)

    def gap(self, d: Decl) -> str:
        """Whitespace and comments right before a declaration: its documentation."""
        prev_end = self.t[d.start - 1].end if d.start > 0 else 0
        return self.src[prev_end:self.t[d.start].start]

    def tokens_text(self, i: int, j: int) -> str:
        return self.src[self.t[i].start:self.t[j - 1].end] if i < j else ""

    def requires_edits(self, d: Decl, generic: bool) -> list[tuple[int, int, str]]:
        edits = []
        for a, b in d.requires:
            if self.dirty(a, b):
                edits.append((a, b, self.reflected_constraint(d) if generic else "(true)"))
        return edits

    def reflected_constraint(self, d: Decl) -> str:
        if not d.template_heads:
            return "(true)"
        a, b = d.template_heads[-1]
        params = [self.t[k + 1].text for k in range(a, b - 1) if self.t[k].text in ("typename", "class")]
        params = params or [self.t[b - 2].text]
        return f"({STUB_NS}::reflected<{params[0]}>)"

    # --- declarations ------------------------------------------------------------------------------------------

    def lower(self, decls: list[Decl], scope: list[str], cls_dropped: set[str] = frozenset()) -> str:
        out = []
        in_class = bool(scope) and scope[-1].startswith("class:")
        for d in decls:
            text = self.lower_decl(d, scope, in_class, cls_dropped)
            if text is None:
                self.stats["dropped"] += 1
                continue
            out.append(self.gap(d) + text)
        return "".join(out)

    def lower_decl(self, d: Decl, scope: list[str], in_class: bool, cls_dropped: set[str]) -> str | None:
        k = d.kind
        if any(self.reflective(a, b) for a, b in d.template_heads):
            return None
        if k in ("static_assert", "consteval_block", "empty"):
            return None
        if k in ("pp", "access", "using", "enum", "class_decl"):
            return self.render(d.start, d.end)
        if k == "namespace":
            inner = self.lower(d.children, [*scope, d.name])
            return self.render(d.start, d.body_open + 1) + inner + self.gap_before(d.body_close) + "}"
        qualified = "::".join([*(s.removeprefix("class:") for s in scope), d.name])
        if k in ("class", "concept", "alias") and qualified in DECL_OVERRIDES:
            self.used_overrides.add(qualified)
            return DECL_OVERRIDES[qualified]
        if k == "class":
            members = {c.name for c in d.children if c.kind == "function" and self.signature_reflective(c)}
            head = self.render(d.start, d.body_open + 1, self.requires_edits(d, generic=True))
            inner = self.lower(d.children, [*scope, f"class:{d.name}"], members)
            return head + inner + self.gap_before(d.body_close) + self.render(d.body_close, d.end)
        if k == "concept":
            if qualified in CONCEPT_OVERRIDES:
                self.used_overrides.add(qualified)
                return self.render(d.start, d.eq + 1) + f" {CONCEPT_OVERRIDES[qualified]};"
            if self.dirty(d.eq + 1, d.end - 1):
                self.stats["true"] += 1
                return self.render(d.start, d.eq + 1) + " true;"
            return self.render(d.start, d.end)
        if k == "alias":
            if d.eq >= 0 and self.dirty(d.eq + 1, d.end - 1):
                return self.render(d.start, d.eq + 1) + f" {STUB_NS}::unknown;"
            return self.render(d.start, d.end)
        if k == "variable":
            return self.lower_variable(d, in_class)
        if k == "function":
            return self.lower_function(d, scope, in_class, cls_dropped)
        raise AssertionError(k)

    def gap_before(self, i: int) -> str:
        return self.src[self.t[i - 1].end:self.t[i].start]

    def lower_variable(self, d: Decl, in_class: bool) -> str | None:
        t = self.t
        type_end = d.eq if d.eq >= 0 else d.end - 1
        if self.reflective(d.head_start, type_end):
            return None
        init_dirty = self.dirty(d.eq + 1, d.end - 1) if d.eq >= 0 else self.dirty(d.head_start, d.end - 1)
        size_typed = not in_class and self.is_size_t(d.head_start, type_end)
        if not init_dirty and not size_typed:
            return self.render(d.start, d.end)
        # rebuild `specifiers type name {}`, an unknown when the type is deduced
        if d.eq >= 0:
            decl_end = d.eq
        else:  # brace initialized: drop the braces
            decl_end = d.end - 1
            if t[decl_end - 1].text == "}":
                decl_end = next(x for x in range(d.head_start, d.end) if t[x].text == "{")
        edits = []
        for x in range(d.head_start, decl_end):
            if t[x].text == "auto":
                edits.append((x, x + 1, f"{STUB_NS}::unknown"))
        if size_typed:
            edits = [(a, b, f"{STUB_NS}::size") for a, b in self.size_t_ranges(d.head_start, decl_end)]
        return self.render(d.start, decl_end, edits) + " {};"

    SIZE_T = ("std", "::", "size_t")

    def size_t_ranges(self, i: int, j: int) -> list[tuple[int, int]]:
        return [(x, x + 3) for x in range(i, j - 2) if tuple(y.text for y in self.t[x:x + 3]) == self.SIZE_T]

    def is_size_t(self, i: int, j: int) -> bool:
        words = [x.text for x in self.t[i:j] if x.text not in SPECIFIERS and x.kind == "ident"]
        return len(self.size_t_ranges(i, j)) == 1 and words[:2] == ["std", "size_t"]

    def lower_function(self, d: Decl, scope: list[str], in_class: bool, cls_dropped: set[str]) -> str | None:
        t = self.t
        if self.signature_reflective(d):
            return None
        owner = scope[-1].removeprefix("class:") if in_class else ""
        edits = self.requires_edits(d, generic=not in_class and d.name.startswith("operator"))
        lead_start = d.head_start
        while t[lead_start].text in SPECIFIERS or t[lead_start].text == "[":
            lead_start = Parser(t).match(lead_start) + 1 if t[lead_start].text == "[" else lead_start + 1
        op = next((x for x in range(lead_start, d.params[0]) if t[x].text == "operator"), -1)
        name_start = op if op >= 0 else d.params[0] - 1
        while name_start > lead_start and t[name_start - 1].text in ("~", "::"):
            name_start -= 2 if t[name_start - 1].text == "::" else 1
        conversion = op >= 0 and t[op + 1].kind == "ident" and t[op + 1].text not in ("new", "delete")
        leading = self.tokens_text(lead_start, name_start).strip()
        trailing = self.tokens_text(*d.trailing_return).strip() if d.trailing_return[0] >= 0 else ""
        ctor = not leading and not conversion and op < 0
        ret = trailing or leading
        if conversion:
            ret = self.tokens_text(op + 1, d.params[0])
        deduced = not ctor and re.search(r"\bauto\b", trailing or leading) is not None and (
            not trailing or trailing in ("auto", "decltype(auto)")
        )

        # signature pieces that need reflection
        if d.trailing_return[0] >= 0 and self.dirty(*d.trailing_return):
            edits.append((*d.trailing_return, f"{STUB_NS}::unknown"))
            ret, deduced = f"{STUB_NS}::unknown", False
        if not in_class and ret in ("std::size_t",):
            for a, b in self.size_t_ranges(lead_start, d.body_open if d.body_open >= 0 else d.end):
                if a < d.params[0] or a >= d.params[1]:
                    edits.append((a, b, f"{STUB_NS}::size"))
            ret = f"{STUB_NS}::size"

        if d.body_open < 0:  # declaration, = default, = delete
            return self.render(d.start, d.end, edits)

        key = (owner, d.name)
        body_range = (d.mem_init[0] if d.mem_init[0] >= 0 else d.body_open, d.body_close + 1)
        if key in BODY_OVERRIDES:
            self.used_overrides.add(key)
            body = BODY_OVERRIDES[key]
        elif (ctor or deduced) and not self.dirty(*body_range, cls_dropped):
            self.stats["kept"] += 1
            return self.render(d.start, d.end, edits)
        elif ctor or ret == "void" or (deduced and not any(t[x].text == "return" for x in range(*body_range))):
            body = "{ }"
        elif deduced:
            body = f"{{ return {STUB_NS}::unknown {{}}; }}"
        elif ret == "bool":  # a predicate, which constraints mostly ask for: say yes
            body = "{ return true; }"
        else:
            body = f"{{ return {STUB_NS}::fake<{ret}>(); }}"
        self.stats["stubbed"] += 1
        edits.append((*body_range, body))
        return self.render(d.start, d.end, edits)


# ------------------------------------------------------------------------------------------------------------------


def generate(src: str) -> tuple[str, dict]:
    toks = tokenize(src)
    decls, _ = Parser(toks).scope(0, None)
    low = Lowering(src, toks, decls)
    body = low.lower(decls, [])
    missing = (set(BODY_OVERRIDES) | set(DECL_OVERRIDES) | set(CONCEPT_OVERRIDES)) - low.used_overrides
    if missing:
        sys.exit(f"stubgen: overrides no longer match anything, update the script: {sorted(missing)}")

    # the standard headers of the real library, minus the reflection one clang does not ship
    body = re.sub(r"^#include <meta>\n", "", body, flags=re.M)
    body = body.replace("#pragma once\n", "", 1)
    banner, sep, rest = body.partition("#include")
    header = [
        banner.rstrip().replace("single header, auto-generated", "IDE stub, generated by scripts/stubgen.py"),
        "// Declarations only, for clangd and other tools without C++26 reflection: never compile code against it.",
        "",
        "#pragma once",
        PRELUDE,
    ]
    return "\n".join(header) + sep + rest, low.stats


def check(path: Path, clang: str) -> int:
    probe = f'#include "{path}"\nint main() {{}}\n'
    cmd = [clang, "-std=c++26", "-fsyntax-only", "-x", "c++", "-"]
    result = subprocess.run(cmd, input=probe, text=True, capture_output=True)
    sys.stderr.write(result.stderr)
    return result.returncode


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-i", "--input", type=Path, default=ROOT / "single_include" / "rbe" / "rbe.hpp")
    parser.add_argument("-o", "--output", type=Path, default=ROOT / "single_include" / "rbe" / "rbe_stub.hpp")
    parser.add_argument("--check", action="store_true", help="run clang -fsyntax-only over the generated stub")
    parser.add_argument("--clang", default=shutil.which("clang++") or "clang++")
    args = parser.parse_args()

    text, stats = generate(args.input.read_text(encoding="utf-8"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8")
    print(
        f"{args.output}: {stats['kept']} bodies kept, {stats['stubbed']} stubbed, {stats['true']} concepts relaxed, "
        f"{stats['dropped']} declarations removed"
    )
    if args.check:
        sys.exit(check(args.output.resolve(), args.clang))


if __name__ == "__main__":
    main()
