#!/bin/sh
# Format the firmware and host-test sources with clang-format (.clang-format).
#
#   scripts/format.sh          # rewrite files in place
#   scripts/format.sh --check  # exit 1 if any file needs formatting
#   CLANG_FORMAT=/path/to/clang-format scripts/format.sh
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CF=${CLANG_FORMAT:-clang-format}
cd "$ROOT"

# http_server_index.h is generated from control-panel/index.html.
FILES=$(ls main/*.cpp main/*.h main/*.ino tests/host/*.cpp tests/host/*.h | grep -v '^main/http_server_index.h$')

if [ "${1:-}" = --check ]; then
    # shellcheck disable=SC2086
    "$CF" --style=file --dry-run -Werror $FILES
else
    # shellcheck disable=SC2086
    "$CF" --style=file -i $FILES
fi
