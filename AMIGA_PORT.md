XorCurses 0.2.2 for GFOS
========================

James W. Morris's XorCurses, after Astral Software's XOR (1987): collect
every one of Xor's masks in each of fifteen mazes, then find the exit,
moving two shields in turn past fish, chickens, dolls and bombs.

Pick a level with its number and Return. The cursor keys (or z, x, '
and /) move the shield; Return or p swaps shields; m shows the map; q leaves
the level and, in the level menu, ends the game (as does the window's
close gadget). h in the menu shows the help pages: the game, its keys
and its objects. Best scores are kept in .xorcurses beside the program,
and an update leaves them. Replays save and load where you name them.

What it needs
-------------
GFOS (AmigaOS 3.2 with GFOpenUp) on a 68040 or 68060; the FPU is used
when there is one, and not needed. The game runs in a console window on
any screen with eight colours or more: no SDL, no ACGame, no GFOpenGPU.

How it was ported
-----------------
The game is built from upstream as it is, its levels and help pages in
the drawer. Its curses is GFOpenCurses (DalsinAI/gfopencurses, the
library our console ports share): windows kept
in memory, and a refresh that sends the console only the lines written
since, as ANSI that console.device reads, with colour pairs on the
screen's first eight pens (SGR 30-37, 40-47), lines and corners in
ASCII, halfdelay and nodelay through WaitForChar, and line input
(wgetnstr) for replay names; it opens the window, puts it in raw mode
and reads the keys (the cursor keys as curses' KEY_*, Return as CR after
nonl()). Its pauses are dos.library
Delay (amiga/include/amiga_prelude.h). The one patch, amiga/patches/0001,
keeps the scores in PROGDIR:, finds the levels by their first file
(AmigaDOS opens no drawer as a file), and joins drawer paths without a
second '/' (which AmigaDOS reads as the parent).

Tests: tests/host/test_xorcurses.sh builds the game on the build machine
over a plain terminal with the Amiga paths, plays it with scripted keys
and reads back the screen (the level menu read from maps/, level 1, moves
by cursor keys, the help page, q, the scores file, the window's close),
and checks the key decoder.

Credits
-------
XorCurses (c) James W. Morris, GPL-3.0 (LICENSE.txt; ReadMe-XorCurses is
upstream's README); XOR (c) 1987 Astral Software.
Amiga port: Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team, GPL-3.0-or-later like XorCurses.
