#!/usr/bin/env python3
"""Pack Generation I sprites and IMA-ADPCM cries for the pokedexfs partition.

SPIFFS struggles with a single file larger than about half the partition, so
80x80 RGB565 sprites are split: sprites1.bin (ids 1-80) and sprites2.bin
(ids 81-151). Cries stay in one cries.bin index+payload pack.
"""

from __future__ import annotations

import argparse
import os
import struct
import subprocess
import sys
import tempfile
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FS = ROOT / "assets" / "pokedex" / "fs"
RAW_ART = ROOT / "assets" / "pokedex" / "raw" / "art"
RAW_CRY = ROOT / "assets" / "pokedex" / "raw" / "cries"
COUNT = 151
PACK1_COUNT = 80
SPRITE_W = 80
SPRITE_H = 80
SPRITE_BYTES = SPRITE_W * SPRITE_H * 2
RATE = 8000
WHOAMI_ROOT = os.environ.get("WHOAMI_ROOT", "")
ART = Path(WHOAMI_ROOT) / "public" / "pokemon-artwork" if WHOAMI_ROOT else None
ART_URL = (
    "https://raw.githubusercontent.com/PokeAPI/sprites/master/"
    "sprites/pokemon/other/official-artwork/{id}.png"
)
CRY_URL = "https://raw.githubusercontent.com/PokeAPI/cries/main/cries/pokemon/legacy/{id}.ogg"

STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
]
INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8]


def ima_nibble(pcm: int, pred: int, index: int) -> tuple[int, int, int]:
    step = STEP_TABLE[index]
    diff = pcm - pred
    nibble = 0
    if diff < 0:
        nibble = 8
        diff = -diff
    if diff >= step:
        nibble |= 4
        diff -= step
    if diff >= (step >> 1):
        nibble |= 2
        diff -= step >> 1
    if diff >= (step >> 2):
        nibble |= 1
    delta = step >> 3
    if nibble & 4:
        delta += step
    if nibble & 2:
        delta += step >> 1
    if nibble & 1:
        delta += step >> 2
    pred = pred - delta if nibble & 8 else pred + delta
    pred = max(-32768, min(32767, pred))
    index = max(0, min(88, index + INDEX_TABLE[nibble & 7]))
    return nibble, pred, index


def encode_ima(samples: bytes) -> bytes:
    pcm = memoryview(samples).cast("h")
    pred = 0
    index = 0
    nibbles: list[int] = []
    for sample in pcm:
        nibble, pred, index = ima_nibble(int(sample), pred, index)
        nibbles.append(nibble)
    packed = bytearray((len(nibbles) + 1) // 2)
    for i, nibble in enumerate(nibbles):
        if i % 2 == 0:
            packed[i // 2] = nibble & 0x0F
        else:
            packed[i // 2] |= (nibble & 0x0F) << 4
    header = struct.pack("<4sHHhH", b"IMAD", RATE, 0, 0, 0)
    header += struct.pack("<I", len(pcm))
    return bytes(header) + bytes(packed)


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url, headers={"User-Agent": "ai-passport-pokedex/0.1"})
    last = None
    for attempt in range(5):
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                dest.write_bytes(resp.read())
            return
        except Exception as exc:  # noqa: BLE001
            last = exc
            time.sleep(0.4 * (attempt + 1))
    raise RuntimeError(f"GET {url} failed: {last}") from last


def art_png(pid: int) -> Path:
    if ART is not None:
        local = ART / f"{pid}.png"
        if local.is_file() and local.stat().st_size > 1000:
            return local
    cached = RAW_ART / f"{pid}.png"
    if cached.is_file() and cached.stat().st_size > 1000:
        return cached
    download(ART_URL.format(id=pid), cached)
    return cached


def convert_sprite(src: Path, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    # Official art stores white RGB on transparent pixels; knock that fringe
    # before lanczos so the 80x80 does not get a white halo on the LCD green.
    from rgba_prep import pack_sprite

    dest.write_bytes(pack_sprite(src, SPRITE_W, SPRITE_H))


def write_sprite_pack(path: Path, first_id: int, count: int, tmp: Path) -> None:
    blob = bytearray(SPRITE_BYTES * count)
    for i in range(count):
        pid = first_id + i
        src = art_png(pid)
        raw = tmp / f"{pid}.rgb"
        convert_sprite(src, raw)
        data = raw.read_bytes()
        if len(data) != SPRITE_BYTES:
            raise RuntimeError(f"sprite #{pid} is {len(data)} bytes, expected {SPRITE_BYTES}")
        blob[i * SPRITE_BYTES : (i + 1) * SPRITE_BYTES] = data
        print(f"sprite #{pid:03d}", file=sys.stderr)
    path.write_bytes(blob)
    print(path, len(blob), file=sys.stderr)


def build_sprites() -> None:
    FS.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        write_sprite_pack(FS / "sprites1.bin", 1, PACK1_COUNT, tmp_path)
        write_sprite_pack(
            FS / "sprites2.bin", PACK1_COUNT + 1, COUNT - PACK1_COUNT, tmp_path
        )


def write_cries_blob(payloads: list[bytes]) -> None:
    offset = 8 + COUNT * 8
    index = bytearray()
    blob = bytearray()
    for payload in payloads:
        index += struct.pack("<II", offset + len(blob), len(payload))
        blob += payload
    header = struct.pack("<4sHH", b"CRY1", COUNT, RATE)
    out = FS / "cries.bin"
    FS.mkdir(parents=True, exist_ok=True)
    out.write_bytes(header + bytes(index) + bytes(blob))
    print(out, out.stat().st_size, file=sys.stderr)


def build_cries() -> None:
    RAW_CRY.mkdir(parents=True, exist_ok=True)
    payloads: list[bytes] = []
    for pid in range(1, COUNT + 1):
        ogg = RAW_CRY / f"{pid}.ogg"
        if not ogg.is_file() or ogg.stat().st_size < 200:
            download(CRY_URL.format(id=pid), ogg)
        pcm = subprocess.check_output(
            [
                "ffmpeg", "-y", "-loglevel", "error",
                "-i", str(ogg), "-ar", str(RATE), "-ac", "1",
                "-f", "s16le", "pipe:1",
            ]
        )
        payloads.append(encode_ima(pcm) if pcm else b"")
        print(f"cry #{pid:03d} {len(payloads[-1])}", file=sys.stderr)
    write_cries_blob(payloads)


def build_placeholder() -> None:
    """Zero-filled packs so firmware CI can still emit pokedexfs."""
    FS.mkdir(parents=True, exist_ok=True)
    (FS / "sprites1.bin").write_bytes(b"\0" * SPRITE_BYTES * PACK1_COUNT)
    (FS / "sprites2.bin").write_bytes(b"\0" * SPRITE_BYTES * (COUNT - PACK1_COUNT))
    write_cries_blob([b""] * COUNT)
    print("placeholder media written", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--placeholder",
        action="store_true",
        help="write empty packs without ffmpeg or network",
    )
    parser.add_argument(
        "--sprites-only",
        action="store_true",
        help="rebuild sprite packs without touching cries.bin",
    )
    args = parser.parse_args()
    if args.placeholder:
        build_placeholder()
        return 0
    build_sprites()
    if not args.sprites_only:
        build_cries()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
