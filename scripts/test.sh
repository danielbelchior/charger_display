#!/bin/sh
# Build and run the host-side unit tests (pure logic from main/, no Arduino).
#
#   scripts/test.sh               # use the host C++ compiler ($CXX or c++)
#   scripts/test.sh wifi          # only tests whose name contains "wifi"
#   CXXFLAGS="-fsanitize=address,undefined" scripts/test.sh
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)

CXX=${CXX:-c++}
OUT="$ROOT/build/host-tests"
mkdir -p "$OUT"

# shellcheck disable=SC2086
"$CXX" -std=c++17 -O1 -g -Wall -Wextra -Werror ${CXXFLAGS:-} \
    -I "$ROOT/main" \
    "$ROOT"/tests/host/*.cpp \
    "$ROOT/main/wifi_policy.cpp" \
    "$ROOT/main/validation.cpp" \
    "$ROOT/main/display_frame.cpp" \
    -o "$OUT/run_tests"

"$OUT/run_tests" "$@"
