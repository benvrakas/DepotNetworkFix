"""Draws the Depot Network Fix icon: a depot ring sending many small updates that a funnel merges into one arrow.

  python Tools/make_icon.py

Writes Docs/Images/icon-1080.png (full resolution), Docs/Images/icon-512.png (mod page) and Resources/Icon128.png
(shown in the game's Mods menu). The design is laid out on a 512-unit grid and rendered at full resolution.
"""
import math
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIZE = 512  # Design grid; all coordinates below are in these units.
FULL_SIZE = 1080
SUPERSAMPLE = 2  # Drawn larger than FULL_SIZE and scaled down for smooth edges.
SCALE = FULL_SIZE * SUPERSAMPLE / SIZE

BACKGROUND_TOP = (38, 40, 58)
BACKGROUND_BOTTOM = (20, 21, 32)
DEPOT = (150, 110, 230)
DEPOT_GLOW = (88, 62, 150)
DOT = (214, 200, 250)
FUNNEL = (120, 126, 140)
FUNNEL_EDGE = (78, 82, 94)
ORANGE = (250, 149, 73)
ORANGE_DARK = (196, 104, 40)


def s(value):
    return value * SCALE


def ellipse(draw, cx, cy, rx, ry, **kwargs):
    draw.ellipse([s(cx - rx), s(cy - ry), s(cx + rx), s(cy + ry)], **kwargs)


def main():
    big = int(FULL_SIZE * SUPERSAMPLE)
    image = Image.new("RGB", (big, big))
    draw = ImageDraw.Draw(image)
    for row in range(big):
        t = row / big
        color = tuple(int(BACKGROUND_TOP[i] + (BACKGROUND_BOTTOM[i] - BACKGROUND_TOP[i]) * t) for i in range(3))
        draw.line([(0, row), (big, row)], fill=color)

    # Depot ring on the left.
    ellipse(draw, 92, 256, 62, 150, fill=DEPOT_GLOW)
    ellipse(draw, 92, 256, 52, 138, outline=DEPOT, width=int(s(16)))

    # Many small updates leaving the depot.
    rows = [-110, -66, -22, 22, 66, 110]
    for k, dy in enumerate(rows):
        for step in range(3):
            x = 168 + step * 34 + (k % 2) * 14
            y = 256 + dy * (1 - step * 0.12)
            ellipse(draw, x, y, 9, 9, fill=DOT)

    # Funnel narrowing the updates into one.
    funnel = [(s(262), s(116)), (s(330), s(222)), (s(330), s(290)), (s(262), s(396))]
    draw.polygon(funnel, fill=FUNNEL, outline=FUNNEL_EDGE, width=int(s(6)))

    # One merged arrow out of the funnel.
    shaft_top, shaft_bottom = 228, 284
    arrow = [(s(330), s(shaft_top)), (s(410), s(shaft_top)), (s(410), s(184)), (s(486), s(256)),
             (s(410), s(328)), (s(410), s(shaft_bottom)), (s(330), s(shaft_bottom))]
    draw.polygon(arrow, fill=ORANGE, outline=ORANGE_DARK, width=int(s(6)))

    full = image.resize((FULL_SIZE, FULL_SIZE), Image.LANCZOS)
    docs = os.path.join(ROOT, "Docs", "Images")
    resources = os.path.join(ROOT, "Resources")
    os.makedirs(docs, exist_ok=True)
    os.makedirs(resources, exist_ok=True)
    full.save(os.path.join(docs, "icon-1080.png"))
    full.resize((512, 512), Image.LANCZOS).save(os.path.join(docs, "icon-512.png"))
    full.resize((128, 128), Image.LANCZOS).save(os.path.join(resources, "Icon128.png"))
    print("Wrote Docs/Images/icon-1080.png, Docs/Images/icon-512.png and Resources/Icon128.png")


if __name__ == "__main__":
    main()
