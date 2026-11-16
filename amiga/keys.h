/* The Amiga console's keys as the curses games read them (amiga/keys.c).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#ifndef AMIGA_KEYS_H
#define AMIGA_KEYS_H
/* The next key from NEXT (a byte, or -1 at the end): -1 at the end of the
 * input, 0 for a sequence with no key in it, else the key as a Unix tty in
 * cbreak mode gives it. */
int amiga_key_decode(int (*next)(void));
/* With amiga_keys_keypad set, the cursor keys (CSI A-D), Shift-up and
 * Shift-down (CSI T, S: a page) and Help (CSI ?~) come as these, curses'
 * KEY_* numbers, instead of 0. */
extern int amiga_keys_keypad;
/* Return as NL (curses' nl(), the default) or as CR (nonl()) */
extern int amiga_keys_crnl;
#define AMIGA_KEY_DOWN  0402
#define AMIGA_KEY_UP    0403
#define AMIGA_KEY_LEFT  0404
#define AMIGA_KEY_RIGHT 0405
#define AMIGA_KEY_NPAGE 0522
#define AMIGA_KEY_PPAGE 0523
#define AMIGA_KEY_HELP  0553
#endif
