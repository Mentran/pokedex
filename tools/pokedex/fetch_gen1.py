#!/usr/bin/env python3
"""Fetch Generation I Pokedex fields from PokeAPI into catalog.json."""

from __future__ import annotations

import json
import sys
import time
import unicodedata
import urllib.error
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "assets" / "pokedex" / "gen1" / "catalog.json"
COUNT = 151
API = "https://pokeapi.co/api/v2"
WORKERS = 6

TYPE_ZH = {
    "bug": "虫",
    "dragon": "龙",
    "electric": "电",
    "fairy": "妖精",
    "fighting": "格斗",
    "fire": "火",
    "flying": "飞行",
    "ghost": "幽灵",
    "grass": "草",
    "ground": "地面",
    "ice": "冰",
    "normal": "一般",
    "poison": "毒",
    "psychic": "超能力",
    "rock": "岩石",
    "water": "水",
}

MOVE_GROUPS = ("red-blue", "yellow", "firered-leafgreen")


def get_json(url: str, retries: int = 5) -> dict:
    last: Exception | None = None
    req = urllib.request.Request(url, headers={"User-Agent": "ai-passport-pokedex/0.1"})
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
            last = exc
            time.sleep(0.4 * (attempt + 1))
    raise RuntimeError(f"GET {url} failed: {last}") from last


def lang_field(entries: list, lang: str, field: str):
    for entry in entries:
        if entry.get("language", {}).get("name") == lang:
            return entry.get(field)
    return None


def clean_text(text: str) -> str:
    return "".join(unicodedata.normalize("NFKC", text.replace("\u000c", " ")).split())


def species_id_from_url(url: str) -> int:
    return int(url.rstrip("/").split("/")[-1])


def ability_intro(data: dict) -> str:
    for lang in ("zh-hans", "en"):
        for entry in data.get("flavor_text_entries", []):
            if entry.get("language", {}).get("name") != lang:
                continue
            text = clean_text(entry.get("flavor_text") or "")
            if text:
                return text
    return ""


def flavor_texts(species: dict) -> list[str]:
    found: list[str] = []
    for entry in species.get("flavor_text_entries", []):
        if entry.get("language", {}).get("name") != "zh-hans":
            continue
        text = clean_text(entry.get("flavor_text", ""))
        if text and text not in found:
            found.append(text)
    return found


def format_condition(details: dict) -> str:
    if not details:
        return ""
    level = details.get("min_level")
    if level:
        return f"Lv.{level}"
    item = details.get("item")
    if item:
        return item.get("name", "item")
    trigger = details.get("trigger", {}).get("name", "")
    if trigger == "trade":
        return "trade"
    if trigger == "use-item":
        return "item"
    return trigger or ""


def gen1_chain(rows: list[dict]) -> list[dict]:
    keep = {row["id"] for row in rows if 1 <= row["id"] <= COUNT}
    out = []
    for row in rows:
        if row["id"] not in keep:
            continue
        frm = row["from"]
        if frm is not None and frm not in keep:
            frm = None
        out.append({**row, "from": frm})
    return out


def flatten_chain(node: dict, from_id=None, cond: str = "") -> list[dict]:
    sid = species_id_from_url(node["species"]["url"])
    rows = [{"id": sid, "from": from_id, "condition": cond}]
    for evo in node.get("evolves_to", []):
        details = evo.get("evolution_details") or [{}]
        rows.extend(flatten_chain(evo, sid, format_condition(details[0])))
    return rows


def level_up_moves(pokemon: dict) -> list[tuple[int, str, str]]:
    rows: list[tuple[int, str, str]] = []
    for move in pokemon.get("moves", []):
        chosen = None
        for detail in move.get("version_group_details", []):
            if detail.get("move_learn_method", {}).get("name") != "level-up":
                continue
            group = detail.get("version_group", {}).get("name")
            if group == "red-blue":
                chosen = detail
                break
            if chosen is None and group in MOVE_GROUPS:
                chosen = detail
        if chosen is None:
            continue
        rows.append(
            (
                int(chosen.get("level_learned_at") or 0),
                move["move"]["name"],
                move["move"]["url"],
            )
        )
    rows.sort(key=lambda item: (item[0], item[1]))
    return rows


def fetch_pair(pid: int) -> tuple[int, dict, dict]:
    pokemon = get_json(f"{API}/pokemon/{pid}")
    species = get_json(f"{API}/pokemon-species/{pid}")
    print(f"core #{pid:03d}", file=sys.stderr)
    return pid, pokemon, species


def map_urls(urls: set[str], label: str) -> dict[str, dict]:
    out: dict[str, dict] = {}
    if not urls:
        return out

    def one(url: str) -> tuple[str, dict]:
        return url, get_json(url)

    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = [pool.submit(one, url) for url in sorted(urls)]
        for i, fut in enumerate(as_completed(futures), 1):
            url, data = fut.result()
            out[url] = data
            if i % 10 == 0 or i == len(futures):
                print(f"{label} {i}/{len(futures)}", file=sys.stderr)
    return out


def main() -> int:
    pairs: dict[int, tuple[dict, dict]] = {}
    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futs = [pool.submit(fetch_pair, pid) for pid in range(1, COUNT + 1)]
        for fut in as_completed(futs):
            pid, pokemon, species = fut.result()
            pairs[pid] = (pokemon, species)

    ability_urls: set[str] = set()
    move_urls: set[str] = set()
    chain_urls: set[str] = set()
    for pokemon, species in pairs.values():
        for item in pokemon.get("abilities", []):
            if not item.get("is_hidden"):
                ability_urls.add(item["ability"]["url"])
        for _level, _name, url in level_up_moves(pokemon):
            move_urls.add(url)
        chain = species.get("evolution_chain", {}).get("url")
        if chain:
            chain_urls.add(chain)

    abilities = map_urls(ability_urls, "ability")
    moves = map_urls(move_urls, "move")
    chains = map_urls(chain_urls, "chain")

    species_zh = {}
    for pid, (_pokemon, species) in pairs.items():
        zh = lang_field(species.get("names", []), "zh-hans", "name")
        if not zh:
            raise RuntimeError(f"missing zh-hans name for #{pid}")
        species_zh[pid] = zh

    chain_rows = {
        url: gen1_chain(flatten_chain(data["chain"])) for url, data in chains.items()
    }
    extra_ids = {
        row["id"]
        for rows in chain_rows.values()
        for row in rows
        if row["id"] not in species_zh
    }
    for eid in extra_ids:
        extra = get_json(f"{API}/pokemon-species/{eid}")
        species_zh[eid] = lang_field(extra.get("names", []), "zh-hans", "name") or extra["name"]

    entries = []
    for pid in range(1, COUNT + 1):
        pokemon, species = pairs[pid]
        flavors = flavor_texts(species)
        stats = {item["stat"]["name"]: int(item["base_stat"]) for item in pokemon["stats"]}
        evo = []
        chain_url = species.get("evolution_chain", {}).get("url")
        if chain_url:
            for row in chain_rows[chain_url]:
                evo.append(
                    {
                        "id": row["id"],
                        "zh": species_zh.get(row["id"], str(row["id"])),
                        "from": row["from"],
                        "condition": row["condition"],
                    }
                )
        art = f"raw/art/{pid}.png"
        entries.append(
            {
                "id": pid,
                "zh": species_zh[pid],
                "en": pokemon["name"],
                "category": lang_field(species.get("genera", []), "zh-hans", "genus") or "",
                "intro": flavors[0] if flavors else "",
                "trivia": flavors[1] if len(flavors) > 1 else "",
                "height_dm": int(pokemon.get("height") or 0),
                "weight_hg": int(pokemon.get("weight") or 0),
                "catch_rate": int(species.get("capture_rate") or 0),
                "gender_rate": int(species.get("gender_rate") if species.get("gender_rate") is not None else -1),
                "types": [
                    TYPE_ZH.get(slot["type"]["name"], slot["type"]["name"])
                    for slot in pokemon["types"]
                ],
                "abilities": [
                    {
                        "zh": lang_field(
                            abilities[item["ability"]["url"]].get("names", []),
                            "zh-hans",
                            "name",
                        )
                        or abilities[item["ability"]["url"]]["name"],
                        "intro": ability_intro(abilities[item["ability"]["url"]]),
                    }
                    for item in pokemon.get("abilities", [])
                    if not item.get("is_hidden")
                ],
                "stats": {
                    "hp": stats.get("hp", 0),
                    "atk": stats.get("attack", 0),
                    "def": stats.get("defense", 0),
                    "spa": stats.get("special-attack", 0),
                    "spd": stats.get("special-defense", 0),
                    "spe": stats.get("speed", 0),
                },
                "moves": [
                    {
                        "level": level,
                        "en": name,
                        "zh": lang_field(moves[url].get("names", []), "zh-hans", "name") or name,
                    }
                    for level, name, url in level_up_moves(pokemon)
                ],
                "evolution": evo,
                "cry": (
                    "https://raw.githubusercontent.com/PokeAPI/cries/main/"
                    f"cries/pokemon/legacy/{pid}.ogg"
                ),
                "artwork": art,
            }
        )

    OUT.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "generation": 1,
        "count": COUNT,
        "source": "https://pokeapi.co/api/v2",
        "entries": entries,
    }
    OUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
