<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Pokedex assets

Play resources for Generation I. Binary art and cries belong here; Markdown documentation stays in `docs/assets/pokedex/`.

| Path | Tracked | Role |
| --- | --- | --- |
| `gen1/catalog.json` | yes | Text catalog for 151 entries |
| `gen1/sprites/` | later | Device sprites |
| `gen1/cries/` | later | IMA-ADPCM cries |
| `raw/` | no | Original PNG / OGG downloads |

Regenerate the catalog with `python3 tools/pokedex/fetch_gen1.py`. Optional `WHOAMI_ROOT` points at the local Who Am I cache for official artwork.
