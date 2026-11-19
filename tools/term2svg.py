#!/usr/bin/env python3
"""Render a text file (terminal output) as an SVG 'terminal window' image.
Usage: term2svg.py input.txt output.svg "window title" """
import html
import sys

CHAR_W = 8.4      # width of one monospace character at 14px
LINE_H = 19
PAD = 16
WRAP = 96         # longer lines are wrapped
BAR = 30


def colour(line):
    if line.startswith("$ "):
        return "#7ee787"            # command typed by the user
    if "FALSE START" in line or "TOO SLOW" in line or "error" in line.lower():
        return "#ff7b72"
    if "LED ON" in line:
        return "#f2cc60"
    if line.startswith(("Best", "Average", "Rating", "-----", "=====")):
        return "#79c0ff"
    return "#e6edf3"


def main():
    src, dst, title = sys.argv[1], sys.argv[2], sys.argv[3]
    lines = []
    for line in open(src).read().rstrip("\n").split("\n"):
        # wrap long lines like a terminal would
        while len(line) > WRAP:
            cut = line.rfind(" ", 0, WRAP)
            cut = cut if cut > 0 else WRAP
            lines.append(line[:cut])
            line = "    " + line[cut:].lstrip()
        lines.append(line)
    width = int(max(60, max(len(l) for l in lines)) * CHAR_W + 2 * PAD)
    height = BAR + PAD + len(lines) * LINE_H + PAD // 2

    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" font-family="DejaVu Sans Mono, Consolas, monospace" font-size="14">',
        f'<rect width="{width}" height="{height}" rx="8" fill="#0d1117"/>',
        f'<rect width="{width}" height="{BAR}" rx="8" fill="#30363d"/>',
        f'<rect y="{BAR - 8}" width="{width}" height="8" fill="#30363d"/>',
        '<circle cx="18" cy="15" r="6" fill="#ff5f56"/><circle cx="38" cy="15" r="6" fill="#ffbd2e"/>'
        '<circle cx="58" cy="15" r="6" fill="#27c93f"/>',
        f'<text x="{width / 2}" y="20" fill="#c9d1d9" text-anchor="middle" '
        f'font-family="Segoe UI, Helvetica, Arial, sans-serif" font-size="13">{html.escape(title)}</text>',
    ]
    y = BAR + PAD + 6
    for line in lines:
        out.append(f'<text x="{PAD}" y="{y}" fill="{colour(line)}" xml:space="preserve">'
                   f'{html.escape(line)}</text>')
        y += LINE_H
    out.append("</svg>")
    open(dst, "w").write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
