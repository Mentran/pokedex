#!/usr/bin/env python3
"""Pack lecture portraits into assets/pokedex/fs/trainers.bin.

Order matches pokedex_speaker_name: Oak, Ash, Brock, Misty, Gary.
Showdown trainer PNGs are 80x80 on black; flatten onto the dex LCD green.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "assets" / "pokedex" / "trainers"
FS = ROOT / "assets" / "pokedex" / "fs"
SPRITE_W = 80
SPRITE_H = 80
SPRITE_BYTES = SPRITE_W * SPRITE_H * 2

# Speaker index 0..4
FILES = ("oak.png", "ash.png", "brock.png", "misty.png", "gary.png")


def convert(src: Path, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    # Black key + nearest scale keeps the Let's Go / FRLG pixel look.
    filt = (
        "[0:v]colorkey=0x000000:0.12:0.08,format=rgba,"
        f"scale={SPRITE_W}:{SPRITE_H}:force_original_aspect_ratio=decrease"
        ":flags=neighbor[fg];"
        f"color=c=0xC8E6C9:s={SPRITE_W}x{SPRITE_H}:d=1[bg];"
        "[bg][fg]overlay=(W-w)/2:(H-h)/2:format=auto"
    )
    subprocess.check_call(
        [
            "ffmpeg", "-y", "-loglevel", "error",
            "-i", str(src), "-filter_complex", filt,
            "-frames:v", "1", "-pix_fmt", "rgb565le",
            "-f", "rawvideo", str(dest),
        ]
    )


def main() -> int:
    FS.mkdir(parents=True, exist_ok=True)
    blob = bytearray()
    for name in FILES:
        src = SRC / name
        if not src.is_file():
            raise SystemExit(f"missing {src}")
        raw = FS / f".{src.stem}.rgb"
        convert(src, raw)
        data = raw.read_bytes()
        raw.unlink(missing_ok=True)
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
