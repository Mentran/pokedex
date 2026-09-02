<p align="right">
  <a href="TODO.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Task board

Status: handheld dex with sprites and cries. Device flash is the remaining check.

## M0

- [x] Root README pair
- [x] Architecture, plan, TODO, decisions
- [x] `main/pokedex.c` navigation + `tests/test_pokedex.c`
- [x] `tools/pokedex/fetch_gen1.py` writes 151-entry `catalog.json`
- [x] Ignore `assets/pokedex/raw/`

## M1

- [x] PokeAPI bios plus trivia; do not import Who Am I text files
- [x] Generate `pokedex_catalog.inc`
- [x] Host tests for 151 parse + four spot checks

## M2

- [x] Convert 151 sprites
- [x] Record byte total vs 1.6 MB budget
- [x] Cover page image

## M3

- [x] Subset font
- [x] Five info pages + home (text)
- [x] Boot directly into the play

## M4

- [x] Legacy cries -> ADPCM
- [x] `pokedexfs` partition
- [x] Cry playback worker

## M5

- [x] Battery placement
- [x] Random without immediate repeat
- [x] Changelog
- [ ] Device pass

## Verification log

| Date | Check | Result |
| --- | --- | --- |
| 2026-09-02 | Isolated worktree `feature/pokemon-pokedex` from upstream `main` | Pass: idiom-chain tree untouched |
| 2026-09-02 | Host tests `test_pokedex` + catalog 151 | Pass |
| 2026-09-02 | `./tools/validate.sh --firmware` text Pokedex | Pass: app 1750160 bytes, merged image 1.7 MB |
| 2026-09-02 | Handheld dex + 151 sprites + cries | Pass: sprites 1.93 MB in two packs, cries 507 KB, app 1783344 bytes, BLE contract PASS. Device flash pending |
