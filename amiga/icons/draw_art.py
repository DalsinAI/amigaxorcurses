#!/usr/bin/env python3
"""XorCurses' picture for its GlowIcons (art/xorcurses.png), drawn here:
upstream has none. A maze's brick walls round one of Xor's masks.
64 x 64, RGBA, solid or see-through.
    python3 amiga/icons/draw_art.py amiga/icons/art
Pure Python. GPL-3.0-or-later, Copyright (c) 2026 AmiCradle Team."""
import struct, sys, zlib
from pathlib import Path

W = H = 64
INK = (16, 18, 22, 255)


def png(px, path):
    raw = b"".join(b"\0" + bytes(v for p in row for v in p) for row in px)
    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def draw():
    px = [[(0, 0, 0, 0)] * W for _ in range(H)]
    def rect(x0, y0, x1, y1, c):
        for y in range(max(0, y0), min(H, y1)):
            for x in range(max(0, x0), min(W, x1)):
                px[y][x] = c
    def ellipse(cx, cy, rx, ry, c):
        for y in range(H):
            for x in range(W):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1:
                    px[y][x] = c
    brick, mortar = (200, 160, 40, 255), (120, 90, 20, 255)
    rect(2, 2, 62, 62, INK)
    rect(3, 3, 61, 61, mortar)
    for row in range(8):
        y = 3 + row * 7
        off = 0 if row % 2 == 0 else 7
        for col in range(-1, 5):
            x = 3 + off + col * 14
            rect(x + 1, y + 1, x + 13, y + 6, brick)
    for x in range(W):          # the frame again, over the bricks' ends
        for y in range(H):
            if x < 3 or y < 3 or x > 60 or y > 60:
                px[y][x] = INK if 2 <= x <= 61 and 2 <= y <= 61 else (0, 0, 0, 0)
    rect(14, 14, 50, 50, (30, 30, 36, 255))
    ellipse(32, 32, 15, 16, INK)
    ellipse(32, 32, 14, 15, (236, 236, 228, 255))
    ellipse(26, 28, 4, 3, INK)
    ellipse(38, 28, 4, 3, INK)
    rect(31, 30, 33, 38, (180, 180, 172, 255))
    for x in range(25, 40):
        y = 41 - (1 if 28 < x < 36 else 0)
        rect(x, y, x + 1, y + 2, (200, 40, 40, 255))
    return px


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "art")
    out.mkdir(parents=True, exist_ok=True)
    png(draw(), out / "xorcurses.png")
