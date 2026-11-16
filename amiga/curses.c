/* The curses of amiga/include/curses.h: windows in memory, and a refresh
 * that sends the console what changed since it last drew.
 *
 * Each window has its lines of chtype (a character, its attributes and
 * its colour pair); a
 * subwindow's lines point into its parent's, so writing either shows in
 * both. wrefresh() copies the window onto the screen it is part of (vscr,
 * what the program wants seen) and then sends the console the cells of vscr
 * that differ from curscr (what the console shows), each run as a cursor
 * move and its text, with SGR for bold, reverse, underline and the pair's
 * pens (30-37, 40-47). wrefresh(curscr), or a
 * window with clearok, clears the console and draws everything.
 *
 * Output goes to stdout through acurses_out(), so the host tests can read
 * it back. Nothing here is Amiga-only; amiga/console_amiga.c opens the
 * console window and sets its mode (acurses_console_open/_mode).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "curses.h"

WINDOW *stdscr, *curscr;
int LINES = 24, COLS = 80;

static WINDOW *vscr;          /* what the program wants on the screen */
static int phys_y = -1, phys_x = -1;     /* the console's cursor */
static chtype phys_attr;                  /* and its attributes */
static short pair_fg[COLOR_PAIRS], pair_bg[COLOR_PAIRS], colour_on;
static int half_delay = -1;               /* halfdelay(): wgetch's wait in ms */
static int ended = 1;
static void resume(void);

/* The console port's (amiga/console_amiga.c, or the host test's) */
int acurses_console_open(int lines, int cols);
void acurses_console_mode(int raw);
void acurses_console_close(void);

static void out(const char *s) { fputs(s, stdout); }

static void out_move(int y, int x)
{
	if (y == phys_y && x == phys_x) return;
	printf("\033[%d;%dH", y + 1, x + 1);
	phys_y = y;
	phys_x = x;
}

static void out_attr(chtype attr)
{
	char sgr[32];
	int n, pair;

	attr &= A_ATTRIBUTES;
	if (attr == phys_attr) return;
	n = snprintf(sgr, sizeof sgr, "\033[0");
	if (attr & A_BOLD) n += snprintf(sgr + n, sizeof sgr - n, ";1");
	if (attr & A_UNDERLINE) n += snprintf(sgr + n, sizeof sgr - n, ";4");
	if (attr & A_STANDOUT) n += snprintf(sgr + n, sizeof sgr - n, ";7");
	pair = PAIR_NUMBER(attr);
	if (colour_on && pair > 0 && pair < COLOR_PAIRS)
		n += snprintf(sgr + n, sizeof sgr - n, ";3%d;4%d", pair_fg[pair], pair_bg[pair]);
	snprintf(sgr + n, sizeof sgr - n, "m");
	out(sgr);
	phys_attr = attr;
}

static WINDOW *make(int lines, int cols, int y, int x, WINDOW *parent)
{
	WINDOW *w;
	int i;

	if (lines <= 0) lines = LINES - y;
	if (cols <= 0) cols = COLS - x;
	if (lines <= 0 || cols <= 0) return NULL;
	w = (WINDOW *)calloc(1, sizeof *w);
	if (!w) return NULL;
	w->begy = y; w->begx = x; w->maxy = lines; w->maxx = cols;
	w->delay = -1;
	w->line = (chtype **)calloc((size_t)lines, sizeof *w->line);
	w->dirty = (char *)malloc((size_t)lines);
	if (!w->line || !w->dirty) { free(w->line); free(w->dirty); free(w); return NULL; }
	memset(w->dirty, 1, (size_t)lines);
	if (parent) {
		/* the parent's cells under it (y, x are screen positions) */
		if (y < parent->begy || x < parent->begx ||
		    y + lines > parent->begy + parent->maxy || x + cols > parent->begx + parent->maxx) {
			free(w->line); free(w->dirty); free(w); return NULL;
		}
		for (i = 0; i < lines; i++)
			w->line[i] = parent->line[y - parent->begy + i] + (x - parent->begx);
	} else {
		chtype *cells = (chtype *)malloc((size_t)lines * cols * sizeof *cells);
		if (!cells) { free(w->line); free(w->dirty); free(w); return NULL; }
		for (i = 0; i < lines * cols; i++) cells[i] = ' ';
		for (i = 0; i < lines; i++) w->line[i] = cells + (size_t)i * cols;
		w->own = 1;
	}
	return w;
}

WINDOW *newwin(int lines, int cols, int y, int x) { return make(lines, cols, y, x, NULL); }

WINDOW *subwin(WINDOW *parent, int lines, int cols, int y, int x)
{
	if (!parent) return NULL;
	return make(lines, cols, y, x, parent);
}

int delwin(WINDOW *w)
{
	if (!w) return ERR;
	if (w->own) free(w->line[0]);
	free(w->line);
	free(w->dirty);
	if (w == stdscr) stdscr = NULL;
	free(w);
	return OK;
}

WINDOW *initscr(void)
{
	if (!ended) return stdscr;
	if (acurses_console_open(LINES, COLS)) return NULL;
	vscr = newwin(LINES, COLS, 0, 0);
	curscr = newwin(LINES, COLS, 0, 0);
	stdscr = newwin(LINES, COLS, 0, 0);
	if (!vscr || !curscr || !stdscr) return NULL;
	acurses_console_mode(1);
	ended = 0;
	phys_attr = 0;
	out("\033[0m\033[H\033[J");
	phys_y = phys_x = 0;
	fflush(stdout);
	return stdscr;
}

int endwin(void)
{
	if (ended) return OK;
	out_attr(0);
	out("\033[ p"); /* the cursor shows again */
	fflush(stdout);
	acurses_console_mode(0);
	ended = 1;
	return OK;
}

int isendwin(void) { return ended; }

int wmove(WINDOW *w, int y, int x)
{
	if (!w || y < 0 || x < 0 || y >= w->maxy || x >= w->maxx) return ERR;
	w->cury = y;
	w->curx = x;
	return OK;
}

static void newline(WINDOW *w)
{
	w->curx = 0;
	if (w->cury + 1 < w->maxy) {
		w->cury++;
	} else if (w->scroll_ok) {
		chtype *first = w->line[0];
		int y;
		/* move the text up a line (in place: subwindows share it) */
		for (y = 0; y + 1 < w->maxy; y++)
			memcpy(w->line[y], w->line[y + 1], (size_t)w->maxx * sizeof *first);
		for (y = 0; y < w->maxx; y++) w->line[w->maxy - 1][y] = ' ';
		memset(w->dirty, 1, (size_t)w->maxy);
	}
}

int waddch(WINDOW *w, chtype c)
{
	int ch = c & A_CHARTEXT;

	if (!w) return ERR;
	if (ch == '\n') {
		wclrtoeol(w);
		newline(w);
		return OK;
	}
	if (ch == '\r') { w->curx = 0; return OK; }
	if (ch == '\b') { if (w->curx > 0) w->curx--; return OK; }
	if (ch == '\t') {
		do { if (waddch(w, ' ') == ERR) return ERR; } while (w->curx % 8 && w->curx);
		return OK;
	}
	if (ch < ' ' || ch == 0x7f) {
		const char *s = unctrl((chtype)ch);
		while (*s) waddch(w, (chtype)(unsigned char)*s++ | (c & A_ATTRIBUTES));
		return OK;
	}
	{
		/* the character's attributes and the window's; its pair, or else the window's */
		chtype a = (c | w->attr) & (A_ATTRIBUTES & ~A_COLOR);
		a |= (c & A_COLOR) ? (c & A_COLOR) : (w->attr & A_COLOR);
		w->line[w->cury][w->curx] = (chtype)ch | a;
		w->dirty[w->cury] = 1;
	}
	if (++w->curx >= w->maxx) {
		/* curses wraps: the next character goes on the next line */
		if (w->cury + 1 < w->maxy || w->scroll_ok) newline(w);
		else w->curx = w->maxx - 1;
	}
	return OK;
}

int waddnstr(WINDOW *w, const char *s, int n)
{
	if (!w || !s) return ERR;
	while (*s && n-- != 0) waddch(w, (chtype)(unsigned char)*s++);
	return OK;
}

int waddstr(WINDOW *w, const char *s) { return waddnstr(w, s, -1); }

int vwprintw(WINDOW *w, const char *fmt, va_list ap)
{
	char buf[512];

	vsnprintf(buf, sizeof buf, fmt, ap);
	return waddstr(w, buf);
}

int wprintw(WINDOW *w, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vwprintw(w, fmt, ap);
	va_end(ap);
	return r;
}

int printw(const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vwprintw(stdscr, fmt, ap);
	va_end(ap);
	return r;
}

int mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...)
{
	va_list ap;
	int r;

	if (wmove(w, y, x) == ERR) return ERR;
	va_start(ap, fmt);
	r = vwprintw(w, fmt, ap);
	va_end(ap);
	return r;
}

int mvprintw(int y, int x, const char *fmt, ...)
{
	va_list ap;
	int r;

	if (wmove(stdscr, y, x) == ERR) return ERR;
	va_start(ap, fmt);
	r = vwprintw(stdscr, fmt, ap);
	va_end(ap);
	return r;
}

int wclrtoeol(WINDOW *w)
{
	int x;

	if (!w) return ERR;
	for (x = w->curx; x < w->maxx; x++) w->line[w->cury][x] = ' ';
	w->dirty[w->cury] = 1;
	return OK;
}

int wclrtobot(WINDOW *w)
{
	int y, x;

	if (!w) return ERR;
	wclrtoeol(w);
	for (y = w->cury + 1; y < w->maxy; y++) {
		for (x = 0; x < w->maxx; x++) w->line[y][x] = ' ';
		w->dirty[y] = 1;
	}
	return OK;
}

int werase(WINDOW *w)
{
	int y, x;

	if (!w) return ERR;
	for (y = 0; y < w->maxy; y++)
		for (x = 0; x < w->maxx; x++) w->line[y][x] = ' ';
	memset(w->dirty, 1, (size_t)w->maxy);
	w->cury = w->curx = 0;
	return OK;
}

int wclear(WINDOW *w)
{
	if (werase(w) == ERR) return ERR;
	w->clear_next = 1;
	return OK;
}

chtype winch(WINDOW *w)
{
	if (!w) return (chtype)ERR;
	return w->line[w->cury][w->curx];
}

int wstandout(WINDOW *w) { if (w) w->attr = A_STANDOUT; return OK; }
int wstandend(WINDOW *w) { if (w) w->attr = 0; return OK; }
int touchwin(WINDOW *w) { if (!w) return ERR; memset(w->dirty, 1, (size_t)w->maxy); return OK; }
int clearok(WINDOW *w, int on) { if (w) w->clear_next = on; return OK; }
int leaveok(WINDOW *w, int on) { (void)w; (void)on; return OK; }
int scrollok(WINDOW *w, int on) { if (w) w->scroll_ok = on; return OK; }
int idlok(WINDOW *w, int on) { (void)w; (void)on; return OK; }

/* Send the console every cell of vscr that differs from curscr (all of
 * them after a clear), and put the cursor at (cy, cx). */
static void update(int all, int cy, int cx)
{
	int y, x;

	if (all) {
		out_attr(0);
		out("\033[H\033[J");
		phys_y = phys_x = 0;
		for (y = 0; y < LINES; y++)
			for (x = 0; x < COLS; x++) curscr->line[y][x] = ' ';
	}
	for (y = 0; y < LINES; y++) {
		for (x = 0; x < COLS; x++) {
			chtype c = vscr->line[y][x];
			if (c == curscr->line[y][x]) continue;
			if (y == LINES - 1 && x == COLS - 1) continue; /* the console would scroll */
			out_move(y, x);
			out_attr(c);
			putchar(c & A_CHARTEXT);
			curscr->line[y][x] = c;
			if (++phys_x >= COLS) phys_y = phys_x = -1; /* the console's own idea now */
		}
	}
	out_attr(0);
	if (cy >= 0 && cy < LINES && cx >= 0 && cx < COLS) out_move(cy, cx);
	fflush(stdout);
}

static void copy_to_vscr(WINDOW *w)
{
	int y;

	for (y = 0; y < w->maxy; y++) {
		int sy = w->begy + y;
		int n = w->maxx;
		/* as curses: only the lines written since the window was last refreshed */
		if (!w->dirty[y]) continue;
		w->dirty[y] = 0;
		if (sy < 0 || sy >= LINES) continue;
		if (w->begx + n > COLS) n = COLS - w->begx;
		if (n > 0) memcpy(vscr->line[sy] + w->begx, w->line[y], (size_t)n * sizeof(chtype));
	}
}

int wnoutrefresh(WINDOW *w)
{
	resume();
	if (!w || ended) return ERR;
	if (w != curscr) copy_to_vscr(w);
	return OK;
}

int doupdate(void)
{
	if (ended) return ERR;
	update(0, stdscr ? stdscr->begy + stdscr->cury : -1, stdscr ? stdscr->begx + stdscr->curx : -1);
	return OK;
}

/* A refresh after endwin() takes the console back, as curses does
 * (Cribbage ends curses for its instructions, then refreshes). */
static void resume(void)
{
	if (ended && vscr) {
		acurses_console_mode(1);
		ended = 0;
		curscr->clear_next = 1;
	}
}

int wrefresh(WINDOW *w)
{
	int all;

	resume();
	if (!w || ended) return ERR;
	if (w == curscr) {
		/* ^L: draw everything again */
		update(1, phys_y, phys_x);
		return OK;
	}
	all = w->clear_next || curscr->clear_next;
	w->clear_next = curscr->clear_next = 0;
	copy_to_vscr(w);
	update(all, w->begy + w->cury, w->begx + w->curx);
	return OK;
}

int mvcur(int oldy, int oldx, int y, int x)
{
	(void)oldy; (void)oldx;
	out_move(y, x);
	fflush(stdout);
	return OK;
}

int wgetch(WINDOW *w)
{
	int k, ms;

	if (w) wrefresh(w);
	ms = w ? w->delay : -1;
	if (ms < 0) ms = half_delay;
	k = acurses_key(w ? w->keypad : 0, ms);
	if (k == -2) {
		/* the input ended: the window was closed */
		endwin();
		exit(0);
	}
	return k;
}

int keypad(WINDOW *w, int on) { if (w) w->keypad = on; return OK; }
int nodelay(WINDOW *w, int on) { if (w) w->delay = on ? 0 : -1; return OK; }
void wtimeout(WINDOW *w, int ms) { if (w) w->delay = ms < 0 ? -1 : ms; }
int halfdelay(int tenths) { half_delay = tenths * 100; acurses_console_mode(1); return OK; }
int set_escdelay(int ms) { (void)ms; return OK; }

int wattrset(WINDOW *w, chtype a) { if (!w) return ERR; w->attr = a & A_ATTRIBUTES; return OK; }

int wattron(WINDOW *w, chtype a)
{
	if (!w) return ERR;
	if (a & A_COLOR) w->attr &= ~A_COLOR; /* one pair at a time */
	w->attr |= a & A_ATTRIBUTES;
	return OK;
}

int wattroff(WINDOW *w, chtype a)
{
	if (!w) return ERR;
	w->attr &= ~(a & A_ATTRIBUTES);
	if (a & A_COLOR) w->attr &= ~A_COLOR;
	return OK;
}

int start_color(void) { colour_on = 1; return OK; }
int has_colors(void) { return TRUE; }
int use_default_colors(void) { return OK; }

int init_pair(short pair, short fg, short bg)
{
	if (pair < 1 || pair >= COLOR_PAIRS || fg < 0 || fg >= COLORS || bg < 0 || bg >= COLORS) return ERR;
	pair_fg[pair] = fg;
	pair_bg[pair] = bg;
	return OK;
}

/* the console's cursor: CSI 0 SP p hides it, CSI SP p shows it */
int curs_set(int visibility)
{
	out(visibility ? "\033[ p" : "\033[0 p");
	fflush(stdout);
	return 1;
}

int whline(WINDOW *w, chtype c, int n)
{
	int sy, sx, x;

	if (!w) return ERR;
	if (!(c & A_CHARTEXT)) c |= ACS_HLINE;
	sy = w->cury; sx = w->curx;
	for (x = sx; x < w->maxx && n-- > 0; x++) {
		w->cury = sy; w->curx = x;
		waddch(w, c);
	}
	w->cury = sy; w->curx = sx;
	return OK;
}

int wvline(WINDOW *w, chtype c, int n)
{
	int sy, sx, y;

	if (!w) return ERR;
	if (!(c & A_CHARTEXT)) c |= ACS_VLINE;
	sy = w->cury; sx = w->curx;
	for (y = sy; y < w->maxy && n-- > 0; y++) {
		w->cury = y; w->curx = sx;
		waddch(w, c);
	}
	w->cury = sy; w->curx = sx;
	return OK;
}

static void put(WINDOW *w, int y, int x, chtype c) { w->cury = y; w->curx = x; waddch(w, c); }

int wborder(WINDOW *w, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br)
{
	int y, x;

	if (!w) return ERR;
	if (!ls) ls = ACS_VLINE;
	if (!rs) rs = ACS_VLINE;
	if (!ts) ts = ACS_HLINE;
	if (!bs) bs = ACS_HLINE;
	if (!tl) tl = ACS_ULCORNER;
	if (!tr) tr = ACS_URCORNER;
	if (!bl) bl = ACS_LLCORNER;
	if (!br) br = ACS_LRCORNER;
	for (x = 1; x < w->maxx - 1; x++) { put(w, 0, x, ts); put(w, w->maxy - 1, x, bs); }
	for (y = 1; y < w->maxy - 1; y++) { put(w, y, 0, ls); put(w, y, w->maxx - 1, rs); }
	put(w, 0, 0, tl);
	put(w, 0, w->maxx - 1, tr);
	put(w, w->maxy - 1, 0, bl);
	put(w, w->maxy - 1, w->maxx - 1, br);
	w->cury = w->curx = 0;
	return OK;
}

int mvwin(WINDOW *w, int y, int x)
{
	if (!w || w->own == 0 || y < 0 || x < 0 || y + w->maxy > LINES || x + w->maxx > COLS) return ERR;
	w->begy = y;
	w->begx = x;
	memset(w->dirty, 1, (size_t)w->maxy);
	return OK;
}

/* a window's new size: its text kept where it fits (the screen is fixed,
 * so XorCurses only asks this after a resize the console never sends) */
int wresize(WINDOW *w, int lines, int cols)
{
	WINDOW *n;
	int y, x;

	if (!w || !w->own || lines <= 0 || cols <= 0) return ERR;
	if (lines == w->maxy && cols == w->maxx) return OK;
	n = make(lines, cols, w->begy, w->begx, NULL);
	if (!n) return ERR;
	for (y = 0; y < lines && y < w->maxy; y++)
		for (x = 0; x < cols && x < w->maxx; x++) n->line[y][x] = w->line[y][x];
	free(w->line[0]); free(w->line); free(w->dirty);
	w->line = n->line; w->dirty = n->dirty; w->maxy = lines; w->maxx = cols;
	if (w->cury >= lines) w->cury = lines - 1;
	if (w->curx >= cols) w->curx = cols - 1;
	free(n);
	return OK;
}

static int echo_on = 1;

/* a line typed into W at its cursor, at most N - 1 characters: Backspace
 * takes one back, Return ends it; echoed while echo() is on */
int wgetnstr(WINDOW *w, char *buf, int n)
{
	int len = 0, k, keep = w ? w->keypad : 0;

	if (!w || !buf || n <= 0) return ERR;
	w->keypad = 0;
	for (;;) {
		k = wgetch(w);
		if (k == '\n' || k == '\r' || k == ERR) break;
		if (k == '\b' || k == 0x7f) {
			if (len > 0) {
				len--;
				if (echo_on) { w->curx = w->curx > 0 ? w->curx - 1 : 0; waddch(w, ' '); w->curx = w->curx > 0 ? w->curx - 1 : 0; }
			}
			continue;
		}
		if (k < ' ' || k > 0xff || len >= n - 1) continue;
		buf[len++] = (char)k;
		if (echo_on) waddch(w, (chtype)k);
	}
	buf[len] = 0;
	w->keypad = keep;
	return OK;
}

/* a pause, the keys left for the next wgetch */
int napms(int ms)
{
	fflush(stdout);
	acurses_sleep(ms);
	return OK;
}

int cbreak(void) { half_delay = -1; acurses_console_mode(1); return OK; }
int nocbreak(void) { half_delay = -1; acurses_console_mode(0); return OK; }
int raw(void) { acurses_console_mode(1); return OK; }
int noraw(void) { acurses_console_mode(0); return OK; }
/* the console in raw mode does not echo; the games echo what they need */
int noecho(void) { echo_on = 0; return OK; }
int echo(void) { echo_on = 1; return OK; }
int nl(void) { amiga_keys_crnl = 1; return OK; }
int nonl(void) { amiga_keys_crnl = 0; return OK; }
int beep(void) { putchar(7); fflush(stdout); return OK; }
int flushinp(void) { return OK; }
char erasechar(void) { return '\b'; }
char killchar(void) { return 0x15; /* ^U */ }
int baudrate(void) { return 38400; }

const char *unctrl(chtype c)
{
	static char buf[3];
	int ch = c & A_CHARTEXT;

	if (ch == 0x7f) return "^?";
	if (ch < ' ') { buf[0] = '^'; buf[1] = (char)(ch + '@'); buf[2] = 0; return buf; }
	buf[0] = (char)ch; buf[1] = 0;
	return buf;
}
