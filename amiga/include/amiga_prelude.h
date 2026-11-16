/* Included before upstream's files in the Amiga build (-include): its
 * pauses (nanosleep, for the sliding animation) as dos.library Delay,
 * through amiga/console_amiga.c (acurses_sleep).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef AMIGA_PRELUDE_H
#define AMIGA_PRELUDE_H
#include <time.h>
void acurses_sleep(int ms);
#define nanosleep(t, rem) \
	(acurses_sleep((int)((t)->tv_sec * 1000 + (t)->tv_nsec / 1000000)), 0)
#endif
