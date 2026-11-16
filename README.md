# XorCurses for GFOS

The AmiCradle Team's port of James W. Morris's XorCurses, the maze puzzle
after Astral Software's XOR (1987), to GFOS: AmigaOS 3.2 with GFOpenUp,
68040/68060. It runs in its own 80 x 24 console window, in colour: no SDL,
no ACGame and no GFOpenGPU. It ships as a release archive with an
Installer and as the GFOpenUp part Parts/XorCurses.

Upstream is fetched at build time (package/xorcurses.lock) and patched
(amiga/patches); this repository holds only the Amiga port. See
[AMIGA_PORT.md](AMIGA_PORT.md).

GPL-3.0-or-later, like XorCurses. Copyright (c) 2026 AmiCradle Team.
