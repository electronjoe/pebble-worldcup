#!/usr/bin/env python3
"""Render the hand-drawn 1-bit art (pixel grids below) to PNGs under
resources/images/. Run manually from the repo root; outputs are committed:
    python3 tools/gen_art.py
Requires Pillow: pip3 install Pillow
"""
import os

from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG_DIR = os.path.join(REPO, 'resources', 'images')

# 9x9 soccer ball (black on white) - flanks the WORLD CUP title
BALL_SMALL = [
    "..XXXXX..",
    ".X.....X.",
    "X...X...X",
    "X..XXX..X",
    "X.XXXXX.X",
    "X..XXX..X",
    "X...X...X",
    ".X.....X.",
    "..XXXXX..",
]

# 16x16 soccer ball (black on white) - centerpiece art
BALL_LARGE = [
    "....XXXXXXXX....",
    "..XX........XX..",
    ".X....XXXX....X.",
    ".X...X....X...X.",
    "X....X....X....X",
    "X..XX.XXXX.XX..X",
    "X.X...XXXX...X.X",
    "X.X..XXXXXX..X.X",
    "X.X..XXXXXX..X.X",
    "X.X...XXXX...X.X",
    "X..XX.XXXX.XX..X",
    "X....X....X....X",
    ".X...X....X...X.",
    ".X....XXXX....X.",
    "..XX........XX..",
    "....XXXXXXXX....",
]

# 24x35 World Cup trophy silhouette (black on white) - centerpiece art
TROPHY = [
    "........XXXXXXXX........",
    "......XX........XX......",
    ".....X....XXXX....X.....",
    "....X....X....X....X....",
    "....X...X..X...X...X....",
    "....X....X....X....X....",
    ".....X....XXXX....X.....",
    "......XX........XX......",
    ".XX.....XXXXXXXX.....XX.",
    "X..X....X......X....X..X",
    "X...X...X......X...X...X",
    "X...X...X......X...X...X",
    ".X...X...X....X...X...X.",
    ".X...X...X....X...X...X.",
    "..X...X...X..X...X...X..",
    "...XXX....X..X....XXX...",
    "..........X..X..........",
    "..........X..X..........",
    ".........X....X.........",
    ".........X....X.........",
    "........X......X........",
    "........X......X........",
    ".......X........X.......",
    "......X..........X......",
    "......X..........X......",
    ".....XXXXXXXXXXXXXX.....",
    "......X..........X......",
    ".....XXXXXXXXXXXXXX.....",
    "....X..............X....",
    "....X..............X....",
    "...XXXXXXXXXXXXXXXXXX...",
    "..X..................X..",
    "..XXXXXXXXXXXXXXXXXXXX..",
    ".XXXXXXXXXXXXXXXXXXXXXX.",
    ".XXXXXXXXXXXXXXXXXXXXXX.",
]

# 20x12 '?' placeholder flag (white on black - drawn onto the black match box)
FLAG_TBD = [
    "XXXXXXXXXXXXXXXXXXXX",
    "X..................X",
    "X.......XXXX.......X",
    "X......X....X......X",
    "X......X....X......X",
    "X..........X.......X",
    "X.........X........X",
    "X.........X........X",
    "X..................X",
    "X.........X........X",
    "X..................X",
    "XXXXXXXXXXXXXXXXXXXX",
]

# 7x11 Bluetooth rune (white on black - drawn onto the black status bar
# when the phone is disconnected)
BT_OFF = [
    "...X...",
    "...XX..",
    "...X.X.",
    "X..X..X",
    ".X.X.X.",
    "..XXX..",
    ".X.X.X.",
    "X..X..X",
    "...X.X.",
    "...XX..",
    "...X...",
]


def render(rows, filename, ink_white_on_black=False):
    w, h = len(rows[0]), len(rows)
    for i, row in enumerate(rows):
        assert len(row) == w, '%s row %d has length %d, expected %d' % (filename, i, len(row), w)
    # mode '1': 0 = black, 1 = white
    img = Image.new('1', (w, h), 0 if ink_white_on_black else 1)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == 'X':
                img.putpixel((x, y), 1 if ink_white_on_black else 0)
    path = os.path.join(IMG_DIR, filename)
    img.save(path)
    print('wrote %s (%dx%d)' % (filename, w, h))


os.makedirs(IMG_DIR, exist_ok=True)
render(BALL_SMALL, 'ball_small.png')
render(BALL_LARGE, 'ball_large.png')
render(TROPHY, 'trophy.png')
render(FLAG_TBD, 'flag_TBD.png', ink_white_on_black=True)
render(BT_OFF, 'bt_off.png', ink_white_on_black=True)
