/* The Amiga under XorCurses (and the curses games before it): its console window, its mode,
 * the keys, and what the games ask of Unix that AmigaOS answers otherwise.
 *
 * The window: each game opens a console window (CON:) of 80 columns by 24
 * lines in the screen's default font (graphics.library's DefaultFont), from
 * a Shell as from Workbench, makes it the program's console and opens
 * stdin, stdout and stderr on it ("*"). Its close gadget ends the input,
 * which the games take as the end of the game.
 *
 * The mode: curses puts the console in raw mode (dos.library SetMode 1: a
 * key at a time, no echo).
 *
 * The keys (acurses_read): Return comes as CR, which a Unix tty gives as
 * NL; the cursor keys, function keys, Help and window events come as CSI
 * sequences, which are read to their end and dropped. amiga/keys.c decodes
 * them, so the host tests check it too.
 *
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "keys.h"

#ifndef AMIGA_GAME_TITLE /* the window's title: amiga/Makefile.amigaos gives each game its own */
#define AMIGA_GAME_TITLE "XorCurses"
#endif

extern struct GfxBase *GfxBase;

static BPTR console;
static int raw_now;

static void console_end(void)
{
	if (console) {
		fputs("\033[0m\033[20h\233 p", stdout); /* plain text, LF mode, the cursor shows again */
		fflush(stdout);
		if (raw_now) SetMode(console, 0);
		Close(console); /* stdin, stdout and stderr ("*") close after this */
		console = 0;
	}
}

/* The game's window, titled TITLE: 0 when it opened */
int amiga_console_window(const char *title)
{
	struct Process *me = (struct Process *)FindTask(NULL);
	struct TextFont *tf = GfxBase ? GfxBase->DefaultFont : NULL;
	int fw = tf ? tf->tf_XSize : 8, fh = tf ? tf->tf_YSize : 8;
	char spec[128];

	if (console) return 0;
	snprintf(spec, sizeof spec, "CON:0/16/%d/%d/%s/CLOSE",
	         80 * fw + 24, 24 * fh + 2 * fh + 20, title);
	console = Open((CONST_STRPTR)spec, MODE_NEWFILE);
	if (!console) return -1;
	atexit(console_end);
	me->pr_ConsoleTask = (APTR)((struct FileHandle *)BADDR(console))->fh_Type;
	if (!freopen("*", "r", stdin) || !freopen("*", "w", stdout) || !freopen("*", "w", stderr))
		return -1;
	/* until curses opens: nothing waits */
	setvbuf(stdout, NULL, _IONBF, 0);
	return 0;
}

/* curses.c's */
int acurses_console_open(int lines, int cols)
{
	(void)lines; (void)cols;
	if (amiga_console_window(AMIGA_GAME_TITLE)) return -1;
	setvbuf(stdout, NULL, _IOFBF, 4096); /* curses sends a screen at a time and flushes */
	return 0;
}

void acurses_console_mode(int raw)
{
	if (console && raw != raw_now) {
		fflush(stdout);
		SetMode(console, raw);
		raw_now = raw;
		if (raw) setvbuf(stdin, NULL, _IONBF, 0); /* a key is read as it comes */
	}
}

void acurses_console_close(void) { console_end(); }

static int key_next(void)
{
	unsigned char c;
	return read(0, &c, 1) == 1 ? c : -1;
}

/* read() of the games, with the keys as a Unix tty gives them */
int acurses_read(int fd, void *buf, unsigned int n)
{
	if (fd != 0 || n == 0) return (int)read(fd, buf, n);
	for (;;) {
		int c = amiga_key_decode(key_next);
		if (c < 0) return 0;  /* the end of the input: the window closed */
		if (c == 0) continue; /* a sequence with no key in it */
		*(unsigned char *)buf = (unsigned char)c;
		return 1;
	}
}

/* curses.c's wgetch: the next key (keys.c: KEY_* for the cursor keys with
 * KEYPAD), waiting MS milliseconds for it (dos.library WaitForChar on the
 * window), or until one comes when MS is negative. -1 (ERR) when none
 * came, -2 when the window closed. */
int acurses_key(int keypad, int ms)
{
	int c;

	fflush(stdout);
	amiga_keys_keypad = keypad;
	for (;;) {
		if (ms >= 0 && console && !WaitForChar(console, (LONG)ms * 1000)) return -1;
		c = amiga_key_decode(key_next);
		if (c < 0) return -2;
		if (c) return c;
	}
}

/* napms(): dos.library Delay, in 50ths of a second (at least one) */
void acurses_sleep(int ms)
{
	fflush(stdout);
	Delay(ms <= 20 ? 1 : (ULONG)(ms / 20));
}
