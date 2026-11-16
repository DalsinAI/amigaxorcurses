/* The part of curses that XorCurses uses (and 2048 and the BSD card games
 * before it), for the Amiga port: windows kept in memory, and a refresh that sends the
 * console only what changed, as ANSI (ECMA-48) sequences, which the Amiga's
 * console.device and the host tests' terminal both read. amiga/curses.c.
 *
 * The screen is LINES x COLS (24 x 80): the console window
 * amiga/console_amiga.c opens that size. stdscr is a variable (Cribbage and
 * Mille point it at their own windows for a while); curscr is what the
 * console shows.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses. */
#ifndef ACURSES_H
#define ACURSES_H

#include <stdio.h>
#include <stdarg.h>

#define TRUE  1
#define FALSE 0
#define ERR   (-1)
#define OK    0

/* curses' own bool (Mille Bornes uses it), unless C99's is there */
#if !defined(__bool_true_false_are_defined) && !defined(__cplusplus)
typedef char bool;
#endif
/* a cell: its character, its attributes, its colour pair (bits 16-23) */
typedef unsigned int chtype;
typedef chtype attr_t;
#define NCURSES_ATTR_T chtype
#define NCURSES_COLOR_T short
#define A_NORMAL     0
#define A_CHARTEXT   0xffu
#define A_STANDOUT   0x100u
#define A_REVERSE    A_STANDOUT
#define A_BOLD       0x200u
#define A_UNDERLINE  0x400u
#define A_DIM        0
#define A_BLINK      0
#define A_COLOR      0xff0000u
#define A_ATTRIBUTES 0xffffff00u
#define COLOR_PAIR(n)   ((chtype)(n) << 16)
#define PAIR_NUMBER(a)  ((int)(((a) & A_COLOR) >> 16))

/* the console's eight pens (SGR 30-37, 40-47) */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7
#define COLORS      8
#define COLOR_PAIRS 64

/* lines and corners: the console's font has none, so ASCII */
#define ACS_ULCORNER '+'
#define ACS_URCORNER '+'
#define ACS_LLCORNER '+'
#define ACS_LRCORNER '+'
#define ACS_LTEE     '+'
#define ACS_RTEE     '+'
#define ACS_TTEE     '+'
#define ACS_BTEE     '+'
#define ACS_PLUS     '+'
#define ACS_HLINE    '-'
#define ACS_VLINE    '|'
#define ACS_BLOCK    '#'
#define ACS_CKBOARD  ':'
#define ACS_BULLET   'o'
#define ACS_DIAMOND  '+'

/* keys with keypad() on (amiga/keys.c decodes them) */
#include "keys.h"
#define KEY_DOWN      AMIGA_KEY_DOWN
#define KEY_UP        AMIGA_KEY_UP
#define KEY_LEFT      AMIGA_KEY_LEFT
#define KEY_RIGHT     AMIGA_KEY_RIGHT
#define KEY_HOME      0406
#define KEY_BACKSPACE 0407
#define KEY_F0        0410
#define KEY_F(n)      (KEY_F0 + (n))
#define KEY_NPAGE     AMIGA_KEY_NPAGE
#define KEY_PPAGE     AMIGA_KEY_PPAGE
#define KEY_ENTER     0527
#define KEY_RESIZE    0632
#define KEY_HELP      AMIGA_KEY_HELP

typedef struct acurses_win {
	int begy, begx;          /* where it is on the screen */
	int maxy, maxx;          /* its lines and columns */
	int cury, curx;
	chtype **line;           /* its lines; a subwindow's point into its parent's */
	chtype attr;             /* wattrset(): attributes and colour pair */
	int keypad;              /* keypad(): the cursor keys as KEY_* */
	int delay;               /* wgetch's wait: -1 for a key, 0 none, else ms */
	int clear_next;          /* clearok: the next refresh redraws everything */
	int scroll_ok;
	int own;                 /* 1: line[] and its text are this window's */
	char *dirty;             /* a line written since the last refresh */
} WINDOW;

extern WINDOW *stdscr, *curscr;
extern int LINES, COLS;

WINDOW *initscr(void);
int endwin(void);
int isendwin(void);
WINDOW *newwin(int lines, int cols, int y, int x);
WINDOW *subwin(WINDOW *parent, int lines, int cols, int y, int x);
int delwin(WINDOW *w);

int wmove(WINDOW *w, int y, int x);
int waddch(WINDOW *w, chtype c);
int waddstr(WINDOW *w, const char *s);
int waddnstr(WINDOW *w, const char *s, int n);
int wprintw(WINDOW *w, const char *fmt, ...);
int vwprintw(WINDOW *w, const char *fmt, va_list ap);
int mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int printw(const char *fmt, ...);
int werase(WINDOW *w);
int wclear(WINDOW *w);
int wclrtoeol(WINDOW *w);
int wclrtobot(WINDOW *w);
int wrefresh(WINDOW *w);
int wnoutrefresh(WINDOW *w);
int doupdate(void);
chtype winch(WINDOW *w);
int wstandout(WINDOW *w);
int wstandend(WINDOW *w);
int touchwin(WINDOW *w);
int clearok(WINDOW *w, int on);
int leaveok(WINDOW *w, int on);
int scrollok(WINDOW *w, int on);
int idlok(WINDOW *w, int on);
int keypad(WINDOW *w, int on);
int mvcur(int oldy, int oldx, int y, int x);
int wgetch(WINDOW *w);
int wattrset(WINDOW *w, chtype a);
int wattron(WINDOW *w, chtype a);
int wattroff(WINDOW *w, chtype a);
int start_color(void);
int has_colors(void);
int init_pair(short pair, short fg, short bg);
int use_default_colors(void);
int curs_set(int visibility);
int set_escdelay(int ms);
int nodelay(WINDOW *w, int on);
int halfdelay(int tenths);
void wtimeout(WINDOW *w, int ms);
int wborder(WINDOW *w, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br);
int whline(WINDOW *w, chtype c, int n);
int wvline(WINDOW *w, chtype c, int n);
int napms(int ms);
int mvwin(WINDOW *w, int y, int x);
int wresize(WINDOW *w, int lines, int cols);
int wgetnstr(WINDOW *w, char *buf, int n);

int cbreak(void);
int nocbreak(void);
int noecho(void);
int echo(void);
int raw(void);
int noraw(void);
int nl(void);
int nonl(void);
int beep(void);
int flushinp(void);
char erasechar(void);
char killchar(void);
const char *unctrl(chtype c);
int baudrate(void);

#define attrset(a)         wattrset(stdscr, (a))
#define attron(a)          wattron(stdscr, (a))
#define attroff(a)         wattroff(stdscr, (a))
#define timeout(ms)        wtimeout(stdscr, (ms))
#define box(w, v, h)       wborder((w), (v), (v), (h), (h), 0, 0, 0, 0)
#define mvwhline(w, y, x, c, n) (wmove((w), (y), (x)) == ERR ? ERR : whline((w), (c), (n)))
#define mvwvline(w, y, x, c, n) (wmove((w), (y), (x)) == ERR ? ERR : wvline((w), (c), (n)))
#define crmode()           cbreak()
#define nocrmode()         nocbreak()
#define getyx(w, y, x)     ((y) = (w)->cury, (x) = (w)->curx)
#define getmaxyx(w, y, x)  ((y) = (w)->maxy, (x) = (w)->maxx)
#define getbegyx(w, y, x)  ((y) = (w)->begy, (x) = (w)->begx)
#define getmaxy(w)         ((w) ? (w)->maxy : ERR)
#define getmaxx(w)         ((w) ? (w)->maxx : ERR)
#define getbegy(w)         ((w) ? (w)->begy : ERR)
#define getbegx(w)         ((w) ? (w)->begx : ERR)
#define getcury(w)         ((w) ? (w)->cury : ERR)
#define getcurx(w)         ((w) ? (w)->curx : ERR)
#define wgetstr(w, s)      wgetnstr((w), (s), 1024)
#define getstr(s)          wgetnstr(stdscr, (s), 1024)

#define addch(c)           waddch(stdscr, (c))
#define addstr(s)          waddstr(stdscr, (s))
#define move(y, x)         wmove(stdscr, (y), (x))
#define clear()            wclear(stdscr)
#define erase()            werase(stdscr)
#define clrtoeol()         wclrtoeol(stdscr)
#define clrtobot()         wclrtobot(stdscr)
#define refresh()          wrefresh(stdscr)
#define inch()             winch(stdscr)
#define standout()         wstandout(stdscr)
#define standend()         wstandend(stdscr)
#define getch()            wgetch(stdscr)
#define mvaddch(y, x, c)   (wmove(stdscr, (y), (x)) == ERR ? ERR : waddch(stdscr, (c)))
#define mvaddstr(y, x, s)  (wmove(stdscr, (y), (x)) == ERR ? ERR : waddstr(stdscr, (s)))
#define mvwaddch(w, y, x, c)  (wmove((w), (y), (x)) == ERR ? ERR : waddch((w), (c)))
#define mvwaddstr(w, y, x, s) (wmove((w), (y), (x)) == ERR ? ERR : waddstr((w), (s)))
#define mvinch(y, x)       (wmove(stdscr, (y), (x)) == ERR ? (chtype)ERR : winch(stdscr))
#define mvwinch(w, y, x)   (wmove((w), (y), (x)) == ERR ? (chtype)ERR : winch(w))

/* What the console port adds (amiga/console_amiga.c on the Amiga,
 * tests/host/console_host.c on the host): the window's next key, Return
 * as NL, the cursor keys as KEY_* when KEYPAD (else dropped), waiting
 * MS milliseconds (-1: until a key comes); ERR when none came, -2 at the
 * end of the input (the window closed). */
int acurses_key(int keypad, int ms);
/* a pause of MS milliseconds, the keys left for later */
void acurses_sleep(int ms);
int acurses_read(int fd, void *buf, unsigned int n);

#endif
