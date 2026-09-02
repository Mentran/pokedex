#!/usr/bin/env python3
"""Sanity-check the Generation I catalog committed under assets/pokedex."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "assets" / "pokedex" / "gen1" / "catalog.json"
REQUIRED = (
    "id",
    "zh",
    "en",
    "category",
    "intro",
    "types",
    "abilities",
    "stats",
    "moves",
    "evolution",
)
NUMERIC = (
    "height_dm",
    "weight_hg",
    "catch_rate",
    "gender_rate",
)


def main() -> int:
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    entries = data["entries"]
    if data.get("count") != 151 or len(entries) != 151:
        print(f"expected 151 entries, got count={data.get('count')} len={len(entries)}", file=sys.stderr)
        return 1
    by_id = {}
    for entry in entries:
        for key in REQUIRED:
            if not entry.get(key):
                print(f"#{entry.get('id')} missing {key}", file=sys.stderr)
                return 1
        for key in NUMERIC:
            if key not in entry:
                print(f"#{entry.get('id')} missing {key}", file=sys.stderr)
                return 1
        abilities = entry["abilities"]
        if not isinstance(abilities, list) or not abilities:
            print(f"#{entry['id']} abilities empty", file=sys.stderr)
            return 1
        for ability in abilities:
            if not isinstance(ability, dict) or not ability.get("zh"):
                print(f"#{entry['id']} ability missing zh", file=sys.stderr)
                return 1
            if "intro" not in ability:
                print(f"#{entry['id']} ability missing intro", file=sys.stderr)
                return 1
        if not (1 <= entry["id"] <= 151):
            print(f"id out of range: {entry['id']}", file=sys.stderr)
            return 1
        if len(entry["moves"]) > 24:
            print(f"#{entry['id']} has {len(entry['moves'])} moves", file=sys.stderr)
            return 1
        if entry.get("artwork", "").startswith("/"):
            print(f"#{entry['id']} artwork is an absolute path", file=sys.stderr)
            return 1
        for node in entry["evolution"]:
            if not (1 <= node["id"] <= 151):
                print(f"#{entry['id']} evolution includes #{node['id']}", file=sys.stderr)
                return 1
        by_id[entry["id"]] = entry
    if sorted(by_id) != list(range(1, 152)):
        print("catalog ids are not 1..151", file=sys.stderr)
        return 1
    if by_id[25]["en"] != "pikachu" or len(by_id[25]["moves"]) < 9:
        print("spot check failed for Pikachu moves", file=sys.stderr)
        return 1
    if by_id[133]["zh"] != "伊布":
        print("spot check failed for Eevee", file=sys.stderr)
        return 1
    one = by_id[1]
    if not one.get("trivia") or one["trivia"] == one["intro"]:
        print("spot check failed for Bulbasaur trivia", file=sys.stderr)
        return 1
    if one["height_dm"] != 7 or one["weight_hg"] != 69 or one["catch_rate"] != 45:
        print("spot check failed for Bulbasaur size/catch", file=sys.stderr)
        return 1
    if one["gender_rate"] != 1 or by_id[25]["gender_rate"] != 4:
        print("spot check failed for gender_rate", file=sys.stderr)
        return 1
    if by_id[29]["gender_rate"] != 8 or by_id[32]["gender_rate"] != 0:
        print("spot check failed for Nidoran gender", file=sys.stderr)
        return 1
    if by_id[151]["gender_rate"] != -1:
        print("spot check failed for Mew gender", file=sys.stderr)
        return 1
    if not one["abilities"][0].get("intro"):
        print("spot check failed for ability intro", file=sys.stderr)
        return 1
    print("Pokedex catalog: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
