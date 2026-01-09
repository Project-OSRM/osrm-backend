#!/usr/bin/env bash

# This script checks if any tracked files have been modified.
# It is used in CI to verify that generated files (formatting, documentation, etc.)
# are up to date with their sources.
#
# Usage: ./scripts/error_on_dirty.sh [hint_message]
#
# If files have been modified, the script prints:
#   - The list of modified files
#   - The diff showing what changed
#   - An optional hint message explaining how to fix the issue
#
# Exit codes:
#   0 - No modified files (success)
#   1 - Modified files detected (failure)

set -o errexit
set -o pipefail
set -o nounset

# Optional hint message passed as first argument
HINT_MSG="${1:-}"

dirty=$(git ls-files --modified)

if [[ $dirty ]]; then
    echo "Error: The following files have been modified:"
    echo "$dirty"
    echo ""
    git diff
    echo ""
    if [[ -n "$HINT_MSG" ]]; then
        echo "Hint: $HINT_MSG"
    fi
    exit 1
else
    exit 0
fi
