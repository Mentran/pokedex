#!/usr/bin/env python3
"""Patch catalog.json with size, catch rate, gender, and ability intros.

Leaves names, moves, and evolution untouched so a field refresh does not
re-download every move URL.
"""

from __future__ import annotations

import importlib.util
import json
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "assets" / "pokedex" / "gen1" / "catalog.json"
FETCH_PATH = Path(__file__).with_name("fetch_gen1.py")
WORKERS = 6


def load_fetch():
    spec = importlib.util.spec_from_file_location("fetch_gen1", FETCH_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {FETCH_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    fetch = load_fetch()
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    entries = data["entries"]
    if len(entries) != fetch.COUNT:
        print(f"expected {fetch.COUNT} entries", file=sys.stderr)
        return 1

    pairs: dict[int, tuple[dict, dict]] = {}
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futs = [pool.submit(fetch.fetch_pair, pid) for pid in range(1, fetch.COUNT + 1)]
        for fut in as_completed(futs):
            pid, pokemon, species = fut.result()
            pairs[pid] = (pokemon, species)

    ability_urls: set[str] = set()
    for pokemon, _species in pairs.values():
        for item in pokemon.get("abilities", []):
            if not item.get("is_hidden"):
                ability_urls.add(item["ability"]["url"])
    abilities = fetch.map_urls(ability_urls, "ability")

    by_id = {int(entry["id"]): entry for entry in entries}
    for pid in range(1, fetch.COUNT + 1):
        pokemon, species = pairs[pid]
        entry = by_id[pid]
        entry["height_dm"] = int(pokemon.get("height") or 0)
        entry["weight_hg"] = int(pokemon.get("weight") or 0)
        entry["catch_rate"] = int(species.get("capture_rate") or 0)
        gender = species.get("gender_rate")
        entry["gender_rate"] = int(gender if gender is not None else -1)
        entry["abilities"] = [
            {
                "zh": fetch.lang_field(
                    abilities[item["ability"]["url"]].get("names", []),
                    "zh-hans",
                    "name",
                )
                or abilities[item["ability"]["url"]]["name"],
                "intro": fetch.ability_intro(abilities[item["ability"]["url"]]),
            }
            for item in pokemon.get("abilities", [])
            if not item.get("is_hidden")
        ]

    CATALOG.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(CATALOG)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
