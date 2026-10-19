# AGENTS.md

Orientation for AI coding agents working on RBE, and a quick map for humans. This file is deliberately short: it says
how an agent should behave here, then points to the real documentation instead of copying it. The full contributor
guide is [CONTRIBUTING.md](CONTRIBUTING.md); the design docs live in [`docs/`](docs/).

## If you are an AI agent, read this first

RBE is a header-only C++26 library built on static reflection (P2996), a feature only GCC 16+ implements today. Models
are weakest exactly there: reflection, `consteval`, `template for`, annotations and pack indexing are newer than most
training data. The bar for a contribution does not drop because a machine wrote it.

- **You are the author.** Understand and be able to defend every line. A change nobody can explain is not ready.
- **Build and test it yourself** before saying it works. Tests encode the intended behavior, not whatever the code
  currently does.
- **Keep the diff minimal.** No unrequested reformatting, renaming, or file-wide rewrites.
- **Distrust confident reflection code.** If you are not sure a `std::meta` function exists or behaves the way you
  think, compile a small probe with the project's GCC before building on it.
- **Propose before you build.** For anything touching the annotation vocabulary, a public concept, the framing model or
  the shape of the public API, write down the options first (see [Design proposals](#design-proposals)).

## Build, test, and conventions

All of this is in [CONTRIBUTING.md](CONTRIBUTING.md); do not reinvent it. The short version (GCC 16 and C++26 are
mandatory; the recipe rejects any other compiler):

```bash
conan install . -b missing -c user.rbe.build:all=True \
    -s compiler=gcc -s compiler.version=16 -s compiler.cppstd=26 -s build_type=Debug
cmake --preset conan-default
cmake --build --preset conan-debug
ctest --preset conan-debug --output-on-failure
```

Tests live in two places, and a change usually touches both:

- `tests/static/`: compile-time tests. Nothing runs; a successful compile is the pass.
- `tests/runtime/`: doctest cases. Use the `RBE_CHECK` / `RBE_REQUIRE` macros from `tests/common/test_macros.hpp`, which
  check the same function both in a constant expression and at runtime.
- `tests/common/common_structs.hpp`: annotated structs shared by both. Reuse one before declaring a new one.

New files are listed explicitly: headers in `src/CMakeLists.txt` and in the right umbrella header (`src/rbe/frames.hpp`,
`src/rbe/core.hpp`, ...), test translation units in their directory's `CMakeLists.txt`.

**Do not run `scripts/amalgamate.py` and do not edit `single_include/rbe/rbe.hpp`.** The single header is produced at
commit time by the `single-header` pre-commit hook, which regenerates and stages it in the same commit; a manual run while
working only churns the working tree. If the hooks are not installed (no `.git/hooks/pre-commit`), install them with
`pre-commit install` rather than running the script by hand. Formatting is also the hook's job (pinned clang-format),
so do not hand-format files you did not otherwise change.

## Understanding the design

Read these before changing core behavior:

- [Design overview](docs/explanation/design-overview.md) and [Concepts](docs/explanation/concepts.md): the core model.
- [Requirements](docs/explanation/requirements.md): the REQ-xxx list. In particular REQ-040/041 (must build without
  exceptions), REQ-042..046 (hardened vs non-hardened checks) and REQ-047..049 (zero overhead, everything derived at
  compile time, trivially wirable types become `memcpy`).
- [Framing design notes](docs/explanation/framing-design-notes.md) and
  [variable-size layout](docs/explanation/variable-size-layout.md): decisions about frames and variable-size types,
  tagged Implemented / Decided / Proposed / Open. Check them before redesigning anything in `frames/`.
- [Glossary](docs/reference/glossary.md): the project's vocabulary (e.g. *size* is what the code knows, *length* is
  what the wire says).

## Architecture at a glance

Everything lives under `src/rbe/`, one directory per layer, each with an umbrella header next to it:

- `core/` (`core.hpp`): reflection helpers, memory layout, the wirable / trivially wirable concepts, endianness.
- `annotations/` (`annotations.hpp`): the `[[=rbe::...]]` annotations and the rules that validate them.
- `srl/` (`srl.hpp`): serialization.
- `dsrl/` (`dsrl.hpp`): deserialization in its three modes: eager, lazy (proxy) and in-place.
- `frames/` (`frames.hpp`): framing composition (`frame`, `many`, `any`, ...), with `srl/` and `dsrl/` subdirectories.
- `rbe.hpp`: includes everything.

Any `detail/` directory and the `rbe::detail` (or `rbe::<layer>::detail`) namespaces are private: users must not need
to name them, and docs must not tell them to.

## Conventions that are easy to get wrong

- **Domain-neutral vocabulary in `src/`.** RBE is a general binary encoding library, not a messaging or market-data one.
  In library code and its comments say *type*, *candidate type*, *bytes*, *buffer*; never *message*, *header packet* or
  *protocol*. Domain words belong in `example/` and in tests.

- **Diagnose with a throwing `consteval` function, not a hard `static_assert`.** A `static_assert` inside an
  instantiation hard-errors and cannot be probed by a concept. Write a `consteval auto diagnose_xxx(...) -> void` that
  throws a descriptive error, turn it into a constraint with `rbe::detail::no_throw` (`core/detail/throw_check.hpp`),
  and call it in a `consteval { ... }` block where you want the readable error. `frames/detail/candidate_list.hpp` is the
  canonical example.

- **Compile time is a cost you must measure, not guess.** Reflection-heavy code is evaluated by GCC's constexpr
  interpreter, and an innocent refactor once doubled build times. Precomputing in a constructor can make things
  *slower* when the object is built in bulk inside `consteval` vectors. If a change may affect compile time, compare
  `-ftime-report` on a heavy TU (e.g. `tests/runtime/test_serde.cpp`) before and after, with `CCACHE_DISABLE=1`.
  `-ftime-trace` does not exist in GCC.

- **Header boilerplate.** Every file starts with the copyright block and a Doxygen `@file` / `@date` / `@brief` header,
  then `#pragma once`, then `// --- Includes ---` (project headers, as `<rbe/...>`) and `// --- STD ---` sections.
  Copy it from a neighboring file.

## Documentation

The docs follow [Diátaxis](https://diataxis.fr/) (guide, reference, explanation) and are built with zensical. The
navigation is explicit in `zensical.toml`, so a new page must be added there. Code shown in the docs is compiled from
`example/docs/`; change the example, not a copy pasted into the page.

If a change alters an annotation, a concept or the framing vocabulary, update the matching page under
`docs/reference/` or `docs/explanation/` in the same change.

### Design proposals

An open design question gets a document under `docs/explanation/`, following the shape of
[variable-size.md](docs/explanation/variable-size.md): `### Summary`, `### Motivation`, `### Design` (one `####`
subsection per fork, each with its options, pros/cons and a recommendation), `### Alternatives Considered`,
`### Open Questions`, `### Decision Log`. Enumerate every fork; do not present only the option you prefer. Once a
decision settles, record it in the relevant design notes file.

## Git and GitHub

- Conventional commit subjects: `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `ci:`, `example:`, with `!` for a
  breaking change (`refactor!:`). One logical change per commit.
- Close issues from the commit body (`Resolves #XXX`), not by hand.
- Issue and PR bodies are GitHub-flavored Markdown with hard line breaks: write one paragraph per line, do not
  source-wrap prose there. Checked-in `.md` files may wrap.
