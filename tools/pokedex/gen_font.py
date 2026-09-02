#!/usr/bin/env python3
"""Collect CJK glyphs used by the Pokedex play and build an LVGL subset font."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCES = [
    ROOT / "main/pokedex.c",
    ROOT / "main/demo_pokedex.c",
    ROOT / "assets/pokedex/gen1/catalog.json",
]
OUTPUT = ROOT / "main/font_pokedex_16.c"
FONT_URL = (
    "https://github.com/notofonts/noto-cjk/raw/main/"
    "Sans/SubsetOTF/SC/NotoSansSC-Regular.otf"
)
FONT_CACHE = pathlib.Path.home() / ".cache/ai-passport/NotoSansSC-Regular.otf"

CJK_RE = re.compile(
    r"[\u3000-\u303F\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF\uFF00-\uFFEF]"
)


def collect_symbols() -> str:
    chars: set[str] = set("，。、！？：；“”‘’（）")
    for path in SOURCES:
        if path.is_file():
            chars.update(CJK_RE.findall(path.read_text(encoding="utf-8")))
    return "".join(sorted(chars))


def ensure_font() -> pathlib.Path:
    FONT_CACHE.parent.mkdir(parents=True, exist_ok=True)
    if FONT_CACHE.is_file() and FONT_CACHE.stat().st_size > 100_000:
        return FONT_CACHE
    print(f"Downloading {FONT_URL}", file=sys.stderr)
    urllib.request.urlretrieve(FONT_URL, FONT_CACHE)
    return FONT_CACHE


def main() -> int:
    symbols = collect_symbols()
    if not symbols:
        print("No CJK glyphs found", file=sys.stderr)
        return 1
    font = ensure_font()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        "npx", "--yes", "lv_font_conv@1.5.3",
        "--font", str(font),
        "--size", "16",
        "--bpp", "4",
        "--format", "lvgl",
        "--lv-include", "lvgl.h",
        "--lv-font-name", "font_pokedex_16",
        "--range", "0x20-0x7E",
        "--symbols", symbols,
        "--no-compress",
        "--no-kerning",
        "--lv-fallback", "lv_font_montserrat_14",
        "-o", str(OUTPUT),
    ]
    print(f"{len(symbols)} CJK glyphs + ASCII", file=sys.stderr)
    subprocess.check_call(cmd, cwd=ROOT)
    header = OUTPUT.with_suffix(".h")
    header.write_text(
        '#pragma once\n\n#include "lvgl.h"\n\nextern const lv_font_t font_pokedex_16;\n',
        encoding="utf-8",
    )
    print(OUTPUT)
    print(header)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
