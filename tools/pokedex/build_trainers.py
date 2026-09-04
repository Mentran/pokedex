#!/usr/bin/env python3
"""Pack lecture portraits into assets/pokedex/fs/trainers.bin.

Order matches pokedex_speaker_name: Oak, Ash, Brock, Misty, Gary.
Sources are high-res official bust/character art. Crop to the upper body,
lanczos-down to 80x80, and flatten onto the dex LCD green.
"""

from __future__ import annotations

import sys
from pathlib import Path

from rgba_prep import (
    crop_rgba,
    flatten_rgb565,
    harden_alpha,
    knock_edge_fringe,
    read_rgba,
    scale_rgba,
)

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "assets" / "pokedex" / "trainers"
FS = ROOT / "assets" / "pokedex" / "fs"
SPRITE_W = 80
SPRITE_H = 80
SPRITE_BYTES = SPRITE_W * SPRITE_H * 2

FILES = ("oak.png", "ash.png", "brock.png", "misty.png", "gary.png")


def is_fill(r: int, g: int, b: int, a: int) -> bool:
    if a < 16:
        return True
    return r < 20 and g < 20 and b < 20


def bust_crop(rgba: bytes, w: int, h: int) -> tuple[int, int, int, int]:
    xs: list[int] = []
    ys: list[int] = []
    for y in range(h):
        for x in range(w):
            o = (y * w + x) * 4
            r, g, b, a = rgba[o : o + 4]
            if is_fill(r, g, b, a):
                continue
            xs.append(x)
            ys.append(y)
    if not xs:
        return 0, 0, w, h
    x0, x1 = min(xs), max(xs)
    y0, y1 = min(ys), max(ys)
    body_h = y1 - y0 + 1
    bust_h = max(36, int(body_h * 0.52))
    y1b = min(h - 1, y0 + bust_h - 1)
    pad = max(4, body_h // 40)
    y0 = max(0, y0 - pad)
    x0 = max(0, x0 - pad)
    x1 = min(w - 1, x1 + pad)
    bw = x1 - x0 + 1
    bh = y1b - y0 + 1
    side = min(bw, bh)
    cx = (x0 + x1) // 2
    x0c = max(x0, cx - side // 2)
    if x0c + side - 1 > x1:
        x0c = max(x0, x1 - side + 1)
    return x0c, y0, side, side


def convert(src: Path) -> bytes:
    rgba, w, h = read_rgba(src)
    knock_edge_fringe(rgba, w, h, radius=10)
    cx, cy, cw, ch = bust_crop(rgba, w, h)
    cropped = crop_rgba(rgba, w, h, cx, cy, cw, ch)
    scaled = scale_rgba(cropped, cw, ch, SPRITE_W, SPRITE_H)
    knock_edge_fringe(scaled, SPRITE_W, SPRITE_H, radius=1)
    harden_alpha(scaled, SPRITE_W, SPRITE_H, cut=128)
    print(f"{src.name} crop={cw}x{ch}+{cx}+{cy} from {w}x{h}", file=sys.stderr)
    return flatten_rgb565(scaled, SPRITE_W, SPRITE_H)


def main() -> int:
    FS.mkdir(parents=True, exist_ok=True)
    blob = bytearray()
    for name in FILES:
        src = SRC / name
        if not src.is_file():
            raise SystemExit(f"missing {src}")
        data = convert(src)
        if len(data) != SPRITE_BYTES:
            raise SystemExit(f"{name} is {len(data)} bytes, expected {SPRITE_BYTES}")
        blob += data
        print(name, SPRITE_BYTES, file=sys.stderr)
    out = FS / "trainers.bin"
    out.write_bytes(blob)
    print(out, len(blob), file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
