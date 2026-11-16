/* The console under amiga/curses.c for the host tests (XorCurses): stdout is the
 * terminal (tests/host/vt.py reads it back), stdin the keys, decoded by
 * amiga/keys.c as on the Amiga (so CR comes as NL and Esc [ sequences are
 * dropped).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#include <poll.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "keys.h"

int acurses_console_open(int lines, int cols) { (void)lines; (void)cols; return 0; }
void acurses_console_mode(int raw) { (void)raw; }
void acurses_console_close(void) {}
int amiga_console_window(const char *title) { (void)title; return 0; }

static int key_next(void)
{
	unsigned char c;
	return read(0, &c, 1) == 1 ? c : -1;
}

int acurses_read(int fd, void *buf, unsigned int n)
{
	if (fd != 0 || n == 0) return (int)read(fd, buf, n);
	for (;;) {
		int c = amiga_key_decode(key_next);
		if (c < 0) return 0;
		if (c == 0) continue;
		*(unsigned char *)buf = (unsigned char)c;
		return 1;
	}
}

/* curses.c's wgetch on the host: poll() for MS milliseconds (the host
 * tests' keys are all there at once, so only the end of them waits) */
int acurses_key(int keypad, int ms)
{
	int c;

	fflush(stdout);
	amiga_keys_keypad = keypad;
	for (;;) {
		struct pollfd p = { 0, POLLIN, 0 };
		if (ms >= 0 && poll(&p, 1, ms) == 0) return -1;
		c = amiga_key_decode(key_next);
		if (c < 0) return -2;
		if (c) return c;
	}
}

void acurses_sleep(int ms)
{
	struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
	fflush(stdout);
	nanosleep(&t, NULL);
}
