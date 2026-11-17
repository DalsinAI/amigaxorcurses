#!/bin/sh
# Host tests of the port, on this machine: amiga/curses.c and amiga/keys.c
# built with upstream's XorCurses (amiga/patches applied, __amigaos__ set,
# so the Amiga's paths are the ones built, its drawer here a directory)
# over a plain terminal (tests/host/console_host.c), played with scripted
# keys; GFOpenCurses' tests/host/vt.py reads back the screen as the Amiga's console
# would show it:
#  - keys: Return is CR after nonl(), NL after nl(); the cursor keys come
#    as KEY_* with keypad on;
#  - the title and the level menu with the fifteen levels' names, read from
#    maps/ through the patched paths (a drawer path takes no second '/');
#  - 1 and Return start level 1: the maze, the info panel, moves 0;
#  - the cursor keys move the player (moves counted);
#  - h g shows the game help page (help_game.txt from the drawer);
#  - q leaves the level, the best scores file (.xorcurses) is written
#    beside the program, q again ends the game; the window's close ends it.
# Needs a C compiler and python3; skipped without a compiler.
#   UP=/path/to/XorCurses (patched) sh tests/host/test_xorcurses.sh
# Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
UP=${UP:?UP=upstream source}
GC=${GFOPENCURSES:?GFOPENCURSES=gfopencurses checkout}
VT=$GC/tests/host/vt.py
CC=${HOSTCC:-cc}
command -v "$CC" >/dev/null 2>&1 || { echo "host tests: skipped (no C compiler)"; exit 0; }
T=$(mktemp -d "${TMPDIR:-/tmp}/xorcurses-host.XXXXXX")
[ -n "${TEST_KEEP:-}" ] || trap 'rm -rf "$T"' EXIT INT TERM
cd "$ROOT"

# the drawer: the program's data where PROGDIR: would be
mkdir -p "$T/drawer/maps"
cp "$UP"/maps/*.xcm "$T/drawer/maps/"
cp "$UP"/help_*.txt "$T/drawer/"

MINE="-std=gnu99 -Wall -Werror -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -I$GC/include -Iamiga/include -Iamiga"
for f in curses keys; do $CC $MINE -c -o "$T/$f.o" "$GC/src/$f.c"; done
$CC $MINE -c -o "$T/con.o" "$GC/src/console_host.c"
for c in "$UP"/*.c; do
    f=$(basename "$c" .c)
    $CC -std=gnu99 -w -D__amigaos__ -DVERSION='"0.2.2"' -DDATADIR="\"$T/drawer/\"" \
        -DAMIGA_USER_DIR="\"$T/drawer/\"" -include amiga/include/amiga_prelude.h \
        -I$GC/include -Iamiga/include -Iamiga -c -o "$T/u_$f.o" "$c"
done
$CC -o "$T/xorcurses" "$T"/u_*.o "$T/curses.o" "$T/keys.o" "$T/con.o"

cat > "$T/keys_test.c" <<'C'
#include <stdio.h>
#include "gfopencurses_keys.h"
static const unsigned char *p; static int n;
static int next(void) { return n-- > 0 ? *p++ : -1; }
int main(void)
{
	static const unsigned char in[] = { '\r', '\r', 0x9b, 'A', 0x1b, '[', 'D' };
	int bad = 0, c;
	p = in; n = sizeof in;
	amiga_keys_keypad = 1;
	amiga_keys_crnl = 0;
	if ((c = amiga_key_decode(next)) != '\r') { printf("nonl: %d\n", c); bad = 1; }
	amiga_keys_crnl = 1;
	if ((c = amiga_key_decode(next)) != '\n') { printf("nl: %d\n", c); bad = 1; }
	if ((c = amiga_key_decode(next)) != AMIGA_KEY_UP) { printf("up: %d\n", c); bad = 1; }
	if ((c = amiga_key_decode(next)) != AMIGA_KEY_LEFT) { printf("left: %d\n", c); bad = 1; }
	return bad;
}
C
fails=0
$CC $MINE -o "$T/keys_test" "$T/keys_test.c" "$T/keys.o"
if "$T/keys_test"; then echo "ok   keys: CR after nonl, NL after nl, cursor keys"; else echo "FAIL keys"; fails=$((fails + 1)); fi

play() {	# keys (printf format), output
    (cd "$T" && printf "$1" | timeout 30 ./xorcurses > "$2" 2>&1)
}
want() {	# name, output, texts...
    name=$1; out=$2; shift 2
    n=$#
    for t in "$@"; do set -- "$@" --want "$t"; done
    shift "$n"
    if python3 "$VT" "$out" "$@" > "$T/vt.txt"; then
        echo "ok   $name"
    else
        echo "FAIL $name"; cat "$T/vt.txt"; fails=$((fails + 1))
    fi
}
moves() { python3 "$VT" "$1" | grep -o "moves: [0-9]*" | tr -dc 0-9; }

if play '' "$T/menu.out"; then echo "ok   the window's close ends the game"; else echo "FAIL the end of the input did not end it"; fails=$((fails + 1)); fi
want "title and level menu, the levels read from maps/" "$T/menu.out" \
    "XorCurses-0.2.2" "Select level:" " 1. Dots and Waves" "15. The Decoder" "q - Quit" "h - Help"
play '1\r' "$T/level.out"
want "1 and Return start level 1" "$T/level.out" "1. Dots and Waves" "masks:  0 of 47" "moves: 0"
play '1\r\033[D\033[D' "$T/moved.out"
if [ "$(moves "$T/moved.out")" = 2 ]; then echo "ok   the cursor keys move the player"; else echo "FAIL moves: $(moves "$T/moved.out")"; fails=$((fails + 1)); fi
play 'hg' "$T/help.out"
if python3 "$VT" "$T/help.out" | grep -q "Xor must be revealed"; then echo "ok   h g shows the game help page (help_game.txt)"; else echo "FAIL help"; python3 "$VT" "$T/help.out"; fails=$((fails + 1)); fi
rm -f "$T/drawer/.xorcurses"
if play '1\r\033[Dqq' "$T/quit.out" && [ -s "$T/drawer/.xorcurses" ]; then
    echo "ok   q leaves the level and the game, the scores are kept beside the program"
else
    echo "FAIL q or the scores file"; fails=$((fails + 1))
fi

if [ "$fails" -ne 0 ]; then echo "host tests: $fails failed"; exit 1; fi
echo "host tests: all passed"
