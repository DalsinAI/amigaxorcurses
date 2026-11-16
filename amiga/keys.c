/* The Amiga console's keys as the curses games read them (amiga/keys.h).
 * In raw mode (SetMode 1) the console gives Return as CR, which a Unix tty
 * gives as NL; the cursor keys, function keys, Help and window events as
 * CSI ($9B, or Esc [) sequences ending in a letter, '~' or '|': these are
 * read to their end and dropped, or with amiga_keys_keypad set given as
 * curses' KEY_* numbers where they are cursor keys, a page or Help. Esc alone is
 * given as Esc; the key after it is kept for the next call. No AmigaOS
 * calls here, so tests/host/test_keys.c runs it on the build machine.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#include "keys.h"

#define CSI 0x9b
#define ESC 0x1b

static int held = -2; /* a byte read after Esc, for the next call; -2: none */
int amiga_keys_keypad;
int amiga_keys_crnl = 1;

int amiga_key_decode(int (*next)(void))
{
	int c, n;

	if (held != -2) {
		c = held;
		held = -2;
	} else {
		c = next();
	}
	if (c == '\r') return amiga_keys_crnl ? '\n' : '\r';
	if (c != CSI && c != ESC) return c;
	if (c == ESC) {
		n = next();
		if (n != '[') {
			held = n;
			return n == -1 ? (held = -2, ESC) : ESC;
		}
	}
	{
		char par[8];
		int np = 0;
		for (;;) {
			n = next();
			if (n == -1) return -1;
			if ((n >= '@' && n <= '~') || n == '|') break;
			if (np < (int)sizeof par - 1) par[np++] = (char)n;
		}
		par[np] = 0;
		if (!amiga_keys_keypad) return 0;
		if (np == 0) {
			switch (n) {
			case 'A': return AMIGA_KEY_UP;
			case 'B': return AMIGA_KEY_DOWN;
			case 'C': return AMIGA_KEY_RIGHT;
			case 'D': return AMIGA_KEY_LEFT;
			case 'T': return AMIGA_KEY_PPAGE;
			case 'S': return AMIGA_KEY_NPAGE;
			}
		} else if (n == '~' && np == 1 && par[0] == '?') {
			return AMIGA_KEY_HELP;
		}
		return 0;
	}
}
