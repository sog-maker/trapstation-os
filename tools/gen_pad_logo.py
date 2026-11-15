#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 TRAPSTATION OS contributors
"""The installer's logo (web pages, emulator) from the approved START-B-BOOT pad logo (the boot SVG's 16 logo-pad groups, rect + highlight),
drawn 8x supersampled and scaled down: assets/logo/trapstation-web.png (144 x 144, shown at 72)."""
import re, sys
from PIL import Image, ImageDraw
SRC = sys.argv[1] if len(sys.argv) > 1 else "docs/design/svg/W04.svg"
svg = open(SRC, encoding="utf-8").read()
out = sys.argv[2] if len(sys.argv) > 2 else "assets/logo/trapstation-web.png"
pads = re.findall(r'<g class="logo-pad p\d+"[^>]*>(.*?)</g>', svg)
assert len(pads) == 16, len(pads)
X0, Y0, SIDE, PX, SS = 66.0, 30.0, 108.0, 144, 8
k = PX * SS / SIDE
img = Image.new('RGBA', (PX * SS, PX * SS), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
d.rounded_rectangle([0, 0, PX * SS - 1, PX * SS - 1], radius=int(14 * k), fill='#171717', outline='#303030', width=int(1.2 * k))
T = lambda x, y: ((x - X0) * k, (y - Y0) * k)
for g in pads:
    r = re.search(r'<rect x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="([\d.]+)" rx="([\d.]+)" fill="(#\w+)" stroke="(#\w+)" stroke-width="([\d.]+)"', g)
    x, y, w, h, rx = map(float, r.groups()[:5])
    a, b = T(x, y); c, e = T(x + w, y + h)
    d.rounded_rectangle([a, b, c, e], radius=rx * k, fill=r.group(6), outline=r.group(7), width=max(1, int(float(r.group(8)) * k)))
    for m in re.finditer(r'<path d="M([\d.]+) ([\d.]+)L([\d.]+) ([\d.]+)" stroke="(#\w+)" stroke-width="([\d.]+)"', g):
        x1, y1, x2, y2 = map(float, m.groups()[:4])
        d.line([T(x1, y1), T(x2, y2)], fill=m.group(5), width=max(1, int(float(m.group(6)) * k)))
img.resize((PX, PX), Image.LANCZOS).save(out, optimize=True)
print(out, PX)
