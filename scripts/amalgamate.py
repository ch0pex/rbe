#!/usr/bin/env python3
"""Generates rbe as a single header.

Walks the `#include <rbe/...>` graph from an entry header and inlines every header at its first inclusion, which is what
the preprocessor does with `#pragma once`, so the order (and any include cycle) behaves the same. Standard library
includes are hoisted to the top, sorted and deduplicated.

The default output keeps the path of the umbrella header, so `-I single_include` is a drop-in for `<rbe/rbe.hpp>`.

    scripts/amalgamate.py        # -> single_include/rbe/rbe.hpp
    scripts/amalgamate.py -o build/rbe.hpp --entry rbe/framing.hpp
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

RBE_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"](rbe/[^>"]+)[>"]')
STD_INCLUDE = re.compile(r"^\s*#\s*include\s*<([^>]+)>")
PRAGMA_ONCE = re.compile(r"^\s*#\s*pragma\s+once\b")
VERSION = re.compile(r"project\([^)]*VERSION (\d+\.\d+\.\d+)")
LICENSE_BANNER = re.compile(r"\A\s*/\*{3,}.*?Copyright.*?\*{3,}/\s*", re.DOTALL)


def read_header(name: str) -> str:
    path = SRC / name
    if not path.is_file():
        sys.exit(f"amalgamate: '{name}' is included but not found under {SRC}")
    return path.read_text(encoding="utf-8")


def collect(entry: str) -> tuple[list[str], list[str], list[str]]:
    """Returns (std includes, body lines, inlined header names)."""
    seen: set[str] = set()
    order: list[str] = []
    std_includes: set[str] = set()
    body: list[str] = []

    def visit(name: str) -> None:
        if name in seen:
            return
        seen.add(name)
        order.append(name)

        text = LICENSE_BANNER.sub("", read_header(name), count=1)
        body.append(f"\n// ===== {name} =====\n")
        for line in text.splitlines():
            if PRAGMA_ONCE.match(line):
                continue
            if (match := RBE_INCLUDE.match(line)) is not None:
                visit(match.group(1))
                continue
            if (match := STD_INCLUDE.match(line)) is not None:
                std_includes.add(match.group(1))
                continue
            body.append(line)

    visit(entry)
    return sorted(std_includes), body, order


def version() -> str:
    match = VERSION.search((SRC / "CMakeLists.txt").read_text(encoding="utf-8"))
    return match.group(1) if match else "unknown version"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--entry", default="rbe/rbe.hpp", help="header to start from, relative to src/")
    parser.add_argument("-o", "--output", type=Path, default=ROOT / "single_include" / "rbe" / "rbe.hpp")
    args = parser.parse_args()

    std_includes, body, order = collect(args.entry)

    banner = [
        "/************************************************************************",
        " * Copyright (c) 2026 Alvaro Cabrera Barrio",
        " * This code is licensed under MIT license (see LICENSE.txt for details)",
        " ************************************************************************/",
        f"// rbe {version()}: single header, auto-generated from the library headers ({len(order)} headers).",
        "",
        "#pragma once",
        "",
        *[f"#include <{name}>" for name in std_includes],
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join([*banner, *body]) + "\n", encoding="utf-8")
    print(f"{args.output}: {len(order)} headers, {len(std_includes)} standard includes")


if __name__ == "__main__":
    main()
