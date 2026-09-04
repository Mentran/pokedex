"""Knock studio/white fringe from artwork, then flatten to RGB565.

Official PNGs store white RGB on transparent pixels. Lanczos then bakes a
white halo around the silhouette. Flood the canvas edge through real
background (transparent / black / LCD green), then eat only a few pixels of
near-white glow so interior whites (Gengar teeth, Oak's coat) stay put.
"""

from __future__ import annotations

import json
import subprocess
from collections import deque
from pathlib import Path

LCD = (0xC8, 0xE6, 0xC9)


def probe_size(src: Path) -> tuple[int, int]:
    probe = subprocess.check_output(
        [
            "ffprobe", "-v", "error", "-select_streams", "v:0",
            "-show_entries", "stream=width,height", "-of", "json", str(src),
        ]
    )
    st = json.loads(probe)["streams"][0]
    return int(st["width"]), int(st["height"])


def read_rgba(src: Path) -> tuple[bytearray, int, int]:
    w, h = probe_size(src)
    raw = subprocess.check_output(
        [
            "ffmpeg", "-y", "-loglevel", "error", "-i", str(src),
            "-f", "rawvideo", "-pix_fmt", "rgba", "pipe:1",
        ]
    )
    if len(raw) != w * h * 4:
        raise RuntimeError(f"{src} decoded to {len(raw)} bytes, expected {w * h * 4}")
    return bytearray(raw), w, h


def _studio_bg(r: int, g: int, b: int, a: int) -> bool:
    if a < 40:
        return True
    if r < 18 and g < 18 and b < 18:
        return True
    if 190 <= r <= 220 and 215 <= g <= 245 and 190 <= b <= 220:
        return True
    return False


def _white_fringe(r: int, g: int, b: int, a: int) -> bool:
    if a < 40:
        return True
    if a < 230 and r >= 190 and g >= 190 and b >= 190:
        return True
    lo = min(r, g, b)
    hi = max(r, g, b)
    return lo >= 200 and (hi - lo) <= 40


def knock_edge_fringe(rgba: bytearray, w: int, h: int, radius: int = 3) -> None:
    n = w * h
    bg = bytearray(n)
    q: deque[int] = deque()

    def consider(i: int) -> None:
        if bg[i]:
            return
        o = i * 4
        if _studio_bg(rgba[o], rgba[o + 1], rgba[o + 2], rgba[o + 3]):
            bg[i] = 1
            q.append(i)

    for x in range(w):
        consider(x)
        consider((h - 1) * w + x)
    for y in range(h):
        consider(y * w)
        consider(y * w + w - 1)
    while q:
        i = q.popleft()
        x, y = i % w, i // w
        if x > 0:
            consider(i - 1)
        if x + 1 < w:
            consider(i + 1)
        if y > 0:
            consider(i - w)
        if y + 1 < h:
            consider(i + w)

    for _ in range(max(0, radius)):
        extra: list[int] = []
        for i in range(n):
            if bg[i]:
                continue
            o = i * 4
            if not _white_fringe(rgba[o], rgba[o + 1], rgba[o + 2], rgba[o + 3]):
                continue
            x, y = i % w, i // w
            hit = (
                (x > 0 and bg[i - 1])
                or (x + 1 < w and bg[i + 1])
                or (y > 0 and bg[i - w])
                or (y + 1 < h and bg[i + w])
            )
            if hit:
                extra.append(i)
        for i in extra:
            bg[i] = 1

    for i in range(n):
        o = i * 4
        if bg[i] or rgba[o + 3] < 16:
            rgba[o] = 0
            rgba[o + 1] = 0
            rgba[o + 2] = 0
            rgba[o + 3] = 0


def choke_foreground(rgba: bytearray, w: int, h: int, radius: int = 2) -> None:
    """Eat the outer anti-aliased ring (light lilac / gray, not just white)."""
    if radius <= 0:
        return
    n = w * h
    bg = bytearray(n)
    for i in range(n):
        if rgba[i * 4 + 3] < 16:
            bg[i] = 1
    for _ in range(radius):
        extra: list[int] = []
        for i in range(n):
            if bg[i]:
                continue
            x, y = i % w, i // w
            if (
                (x > 0 and bg[i - 1])
                or (x + 1 < w and bg[i + 1])
                or (y > 0 and bg[i - w])
                or (y + 1 < h and bg[i + w])
            ):
                extra.append(i)
        for i in extra:
            bg[i] = 1
            o = i * 4
            rgba[o] = 0
            rgba[o + 1] = 0
            rgba[o + 2] = 0
            rgba[o + 3] = 0


def crop_rgba(rgba: bytes, w: int, h: int, x: int, y: int, cw: int, ch: int) -> bytearray:
    out = bytearray(cw * ch * 4)
    for row in range(ch):
        src = ((y + row) * w + x) * 4
        dst = row * cw * 4
        out[dst : dst + cw * 4] = rgba[src : src + cw * 4]
    return out


def scale_rgba(rgba: bytes, w: int, h: int, tw: int, th: int) -> bytearray:
    vf = (
        f"scale={tw}:{th}:force_original_aspect_ratio=decrease:flags=lanczos,"
        f"pad={tw}:{th}:(ow-iw)/2:(oh-ih)/2:color=0x00000000,format=rgba"
    )
    raw = subprocess.check_output(
        [
            "ffmpeg", "-y", "-loglevel", "error",
            "-f", "rawvideo", "-pix_fmt", "rgba", "-s", f"{w}x{h}",
            "-i", "pipe:0",
            "-vf", vf,
            "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgba", "pipe:1",
        ],
        input=bytes(rgba),
    )
    if len(raw) != tw * th * 4:
        raise RuntimeError(f"scale produced {len(raw)} bytes, expected {tw * th * 4}")
    return bytearray(raw)


def hex565(r: int, g: int, b: int) -> int:
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def flatten_rgb565(rgba: bytes, w: int, h: int, bg: tuple[int, int, int] = LCD) -> bytes:
    br, bg_, bb = bg
    fill = hex565(br, bg_, bb)
    out = bytearray(w * h * 2)
    n = w * h
    pale = bytearray(n)
    q: deque[int] = deque()

    pixels = [0] * n
    for i in range(n):
        o = i * 4
        r, g, b, a = rgba[o], rgba[o + 1], rgba[o + 2], rgba[o + 3]
        if a <= 8:
            p = fill
        elif a >= 250:
            p = hex565(r, g, b)
        else:
            t = a / 255.0
            pr = int(r * t + br * (1.0 - t) + 0.5)
            pg = int(g * t + bg_ * (1.0 - t) + 0.5)
            pb = int(b * t + bb * (1.0 - t) + 0.5)
            p = hex565(pr, pg, pb)
        pixels[i] = p

    def is_pale(p: int) -> bool:
        r = (p >> 11) & 31
        g = (p >> 5) & 63
        b = p & 31
        if r >= 22 and r <= 27 and g >= 52 and g <= 60 and b >= 22 and b <= 27:
            return True
        return r >= 28 and g >= 56 and b >= 28

    def push(i: int) -> None:
        if pale[i] or not is_pale(pixels[i]):
            return
        pale[i] = 1
        q.append(i)

    for x in range(w):
        push(x)
        push((h - 1) * w + x)
    for y in range(h):
        push(y * w)
        push(y * w + w - 1)
    while q:
        i = q.popleft()
        x, y = i % w, i // w
        if x > 0:
            push(i - 1)
        if x + 1 < w:
            push(i + 1)
        if y > 0:
            push(i - w)
        if y + 1 < h:
            push(i + w)

    for i in range(n):
        p = fill if pale[i] else pixels[i]
        out[i * 2] = p & 0xFF
        out[i * 2 + 1] = p >> 8
    return bytes(out)


def pack_sprite(src: Path, tw: int, th: int, radius: int = 4) -> bytes:
    rgba, w, h = read_rgba(src)
    knock_edge_fringe(rgba, w, h, radius=radius)
    # Keep the dark outline. Choking eats it and leaves a light body rim
    # that reads as a white halo on the LCD green.
    scaled = scale_rgba(rgba, w, h, tw, th)
    knock_edge_fringe(scaled, tw, th, radius=1)
    harden_alpha(scaled, tw, th, cut=128)
    return flatten_rgb565(scaled, tw, th)


def harden_alpha(rgba: bytearray, w: int, h: int, cut: int = 128) -> None:
    n = w * h
    for i in range(n):
        o = i * 4
        if rgba[o + 3] < cut:
            rgba[o] = 0
            rgba[o + 1] = 0
            rgba[o + 2] = 0
            rgba[o + 3] = 0
        else:
            rgba[o + 3] = 255
