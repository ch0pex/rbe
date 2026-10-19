# CLAUDE.md

Claude Code loads this file automatically. The shared, vendor-neutral guidance for this repository lives in
[AGENTS.md](AGENTS.md) (also read by other agent tools) and is imported below. Personal, machine-specific notes go in
`CLAUDE.local.md` (gitignored), which Claude also loads.

@AGENTS.md

## Working with the maintainer

- **Decide, do not interrogate.** For implementation choices, pick a sensible default, state it and its rationale in
  the plan, and flag the reversible ones. Do not stop to ask a batch of questions with `AskUserQuestion`; ask only when
  going either way would waste real work.
- **Open design forks are the exception.** When the question is the design itself, do not offer a two-option dialog:
  write the full option space as a proposal document (see *Design proposals* in AGENTS.md).
- **Verify with the build, then stop.** After changing headers under `src/`, `cmake --build` plus `ctest` is the
  check. The single header and the formatting belong to the pre-commit hook.

## Memory

Keep in memory only what the repository cannot tell you: decisions reached in conversation, feedback on how to work,
measurements. Anything that is a rule for every contributor belongs in AGENTS.md, and any design decision belongs in
the design notes under `docs/explanation/`, not only in memory.
