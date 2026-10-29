#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
cc=${CC:-cc}

"$cc" -std=c99 -Wall -Wextra -Werror -Isrc/include -Itests/PAW3805EK/stubs \
    tests/PAW3805EK/driver_test.c -o "$build_dir/driver_test"
"$build_dir/driver_test"
"$cc" -std=c99 -Wall -Wextra -Werror -Isrc/include \
    tests/PAW3805EK/unconfigured_test.c -o "$build_dir/unconfigured_test"
"$build_dir/unconfigured_test"

for invalid in duplicate reserved range; do
    case "$invalid" in
        duplicate) ncs=0; clk=0; sdio=2 ;;
        reserved) ncs=9; clk=1; sdio=2 ;;
        range) ncs=16; clk=1; sdio=2 ;;
    esac
    if "$cc" -std=c99 -Isrc/include -Itests/PAW3805EK/stubs \
        -DPAW3805EK_CONFIGURED=1 -DPAW3805EK_NCS_PIN="$ncs" \
        -DPAW3805EK_SCLK_PIN="$clk" -DPAW3805EK_SDIO_PIN="$sdio" \
        -c src/PAW3805EK.c -o "$build_dir/invalid.o" >"$build_dir/error" 2>&1; then
        echo "FAIL configuration guard accepted $invalid pins" >&2
        exit 1
    fi
    grep -q '#error' "$build_dir/error"
    echo "PASS compile-time GPIO guard: $invalid"
done
