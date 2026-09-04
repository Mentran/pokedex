<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Pokedex assets

Play resources for Generation I. Binary art and cries belong here; Markdown documentation stays in `docs/assets/pokedex/`.

| Path | Tracked | Role |
| --- | --- | --- |
| `gen1/catalog.json` | yes | Text catalog for 151 entries |
| `gen1/facts.json` | yes | Original short world-fact sentences |
| `fs/sprites1.bin`, `fs/sprites2.bin`, `fs/cries.bin`, `fs/trainers.bin` | no | Packed device media |
| `trainers/` | yes | Oak's lecture portrait PNG sources |
| `raw/` | no | Original PNG / OGG downloads |

Regenerate the catalog with `python3 tools/pokedex/fetch_gen1.py`. Patch size, catch, gender, and ability intros with `python3 tools/pokedex/enrich_catalog.py` when a full move refresh is not needed. Optional `WHOAMI_ROOT` is a local official-art directory for packing sprites; that cache is never committed.
