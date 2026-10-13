#!/usr/bin/env bash
# Formats every C/C++ source in the repository with the clang-format version pinned in .pre-commit-config.yaml, so
# the result matches the pre-commit hook and the CI check regardless of the clang-format installed locally.
#
# Usage: scripts/format.sh
# Requires pre-commit (`pip install pre-commit`).
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if ! command -v pre-commit >/dev/null 2>&1; then
  echo "error: pre-commit is not installed; run 'pip install pre-commit'" >&2
  exit 1
fi

# pre-commit exits non-zero when the hook modifies files; here that is the expected outcome, not a failure
if pre-commit run clang-format --all-files; then
  echo "All files were already formatted."
else
  status=$?
  if [[ -n "$(git status --porcelain)" ]]; then
    echo "Formatted the files listed above; review and commit them."
  else
    exit "${status}"
  fi
fi
