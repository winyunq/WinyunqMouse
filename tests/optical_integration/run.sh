#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM
for swap in 0 1; do
  for invert_x in 0 1; do
    for invert_y in 0 1; do
      "${CC:-cc}" -std=c99 -Wall -Wextra -Werror \
        -fsanitize=undefined -fno-sanitize-recover=all \
        -Itests/optical_integration/stubs -Isrc/include -Isrc/Profile/include \
        -DPAW3805EK_SWAP_XY="$swap" -DPAW3805EK_INVERT_X="$invert_x" -DPAW3805EK_INVERT_Y="$invert_y" \
        tests/optical_integration/test_mouse.c -o "$tmpdir/test_mouse"
      "$tmpdir/test_mouse"
    done
  done
done
