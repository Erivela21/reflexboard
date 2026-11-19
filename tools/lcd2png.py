#!/usr/bin/env python3
"""Convert LCD frames saved by the simulator (128x64 PBM, P1) into PNG images
that look like the DE10-Standard LCD.  Usage: lcd2png.py in.pbm out.png [scale]"""
import struct
import sys
import zlib

BG = (198, 214, 160)    # backlit LCD background
FG = (30, 40, 30)       # pixel colour
BORDER = 6              # pixels of background around the screen (before scaling)


def read_pbm(path):
    tokens = open(path).read().split()
    assert tokens[0] == "P1", "expected a plain PBM (P1) file"
    w, h = int(tokens[1]), int(tokens[2])
    bits = "".join(tokens[3:])
    return w, h, [[bits[y * w + x] == "1" for x in range(w)] for y in range(h)]


def write_png(path, w, h, rows):
    raw = b"".join(b"\x00" + bytes(row) for row in rows)

    def chunk(kind, data):
        c = kind + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    src, dst = sys.argv[1], sys.argv[2]
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 4
    w, h, px = read_pbm(src)
    W, H = (w + 2 * BORDER) * scale, (h + 2 * BORDER) * scale
    rows = []
    for Y in range(H):
        y = Y // scale - BORDER
        row = []
        for X in range(W):
            x = X // scale - BORDER
            on = 0 <= x < w and 0 <= y < h and px[y][x]
            row.extend(FG if on else BG)
        rows.append(row)
    write_png(dst, W, H, rows)


if __name__ == "__main__":
    main()
