#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT
compiler="${CXX:-c++}"
flags=(-std=c++17 -O1 -g -Wall -Wextra -Werror)
if [[ "${SANITIZE:-0}" == 1 ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
for source in tests/physics.cpp tests/settings.cpp tests/regressions.cpp; do
    executable="$test_dir/$(basename "$source" .cpp)"
    "$compiler" "${flags[@]}" "$source" -o "$executable"
    "$executable"
done
echo "Physics, settings and regression tests passed."
