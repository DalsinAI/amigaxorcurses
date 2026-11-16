#!/usr/bin/env python3
"""Read what amiga/curses.c (or Backgammon, through amiga/termcap.c) sent
(ANSI: CSI row;col H, CSI [0] SP p (the cursor), CSI A/B/C/D, CSI J, CSI K, SGR 0/7, CSI 20 h/l, CR,
LF, BS, text) as the Amiga's console.device reads it: LF also returns the
carriage until CSI 20 l (and again after CSI 20 h), and LF on the last
line scrolls, and print the 24 x 80 screen it leaves, or check it:
    vt.py OUTPUT [--want TEXT ...] [--standout TEXT]
Exits 1 when a wanted text is not on the screen.
Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, like XorCurses."""
import re, sys

def screen(data, rows=24, cols=80):
    s = [[" "] * cols for _ in range(rows)]
    so = [[False] * cols for _ in range(rows)]
    y = x = 0
    inv = False
    lnm = True
    i = 0
    while i < len(data):
        c = data[i]
        if (c == "\033" and i + 1 < len(data) and data[i + 1] == "[") or c == "\x9b":
            m = re.match(r"(?:\033\[|\x9b)([0-9;]*) ?([A-Za-z])", data[i:])
            if not m:
                i += 1; continue
            args, cmd = m.group(1), m.group(2)
            nums = [int(a) if a else 0 for a in args.split(";")] if args else []
            if cmd == "H":
                y = (nums[0] if nums and nums[0] else 1) - 1
                x = (nums[1] if len(nums) > 1 and nums[1] else 1) - 1
            elif cmd == "J":
                for r in range(rows):
                    for k in range(cols):
                        if r > y or (r == y and k >= x) or (nums and nums[0] == 2):
                            s[r][k] = " "; so[r][k] = False
            elif cmd == "K":
                for k in range(x, cols):
                    s[y][k] = " "; so[y][k] = False
            elif cmd in "ABCD":
                n = nums[0] if nums and nums[0] else 1
                if cmd == "A": y = max(0, y - n)
                elif cmd == "B": y = min(rows - 1, y + n)
                elif cmd == "C": x = min(cols - 1, x + n)
                else: x = max(0, x - n)
            elif cmd in "hl" and 20 in nums:
                lnm = cmd == "h"
            elif cmd == "m":
                inv = 7 in nums
            i += m.end(); continue
        if c == "\r": x = 0
        elif c == "\n":
            if lnm: x = 0
            if y == rows - 1:
                s.pop(0); so.pop(0); s.append([" "] * cols); so.append([False] * cols)
            else:
                y += 1
        elif c == "\b": x = max(0, x - 1)
        elif c == "\a": pass
        elif c >= " ":
            if 0 <= y < rows and 0 <= x < cols:
                s[y][x] = c; so[y][x] = inv
            x += 1
            if x >= cols: x = 0; y = min(y + 1, rows - 1)
        i += 1
    return ["".join(r).rstrip() for r in s], so

if __name__ == "__main__":
    data = open(sys.argv[1], encoding="latin-1").read()
    lines, so = screen(data)
    args = sys.argv[2:]
    if not args:
        print("\n".join(lines)); sys.exit(0)
    text = "\n".join(lines)
    bad = 0
    while args:
        opt, val = args[0], args[1]; args = args[2:]
        if opt == "--want" and val not in text:
            print(f"missing on the screen: {val!r}"); bad = 1
    if bad:
        print("\n".join(lines))
    sys.exit(bad)
