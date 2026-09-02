<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Generation I Pokedex architecture

This is the feasible design for an on-device Pokedex of the original 151 Pokemon. It is the technical source of truth for this play. Update it when the flash budget, page map, or data pipeline changes.

## Goal

A trainer can hang the badge on their chest, press three keys, and look up a Pokemon the way a Pokedex would: picture first, then bio, cry, types, stats, moves, and evolution.

V1 scope is **national dex 1-151 only**. Later generations are out of scope until V1 is on hardware.

## Hardware facts that decide the design

| Fact | Value | Effect |
| --- | --- | --- |
| MCU | ESP32-C3, no PSRAM | Decode one sprite and one cry at a time; no full catalog in RAM |
| Flash | 8 MB | Room exists, but Recovery and `cardid` are reserved |
| Application | 3 MB at `0x10000` | Firmware, Chinese subset font, and logic only |
| Protected | `cardid@0x356000`, `recovery@0x700000` | Must not move or overlap |
| Display | 240 x 320 RGB565, no touch | One focused card per page; no tiny tables |
| Input | Up, Down, OK | Browse with Up/Down; cycle pages with OK |
| Audio | ES8311, 16-bit PCM | Play a decoded cry from a worker task |

The usable data window is **`0x35A000` to `0x700000`**: 3,825,664 bytes (~3.65 MB). Name it `pokedexfs` (SPIFFS). Do not shrink the 3 MB app slot unless a later measurement proves the firmware cannot fit.

Merged images that contain this partition must not be written as a single file onto a device that already has an identity. Flash `factory` and `pokedexfs` as separate segments, or install through the mini-program Recovery path. Never `erase-flash` a provisioned badge.

## What fits, and what does not

| Asset | Naive size | V1 encoding | Budget |
| --- | --- | --- | --- |
| Official artwork PNG, ~180 KB x 151 | ~27 MB | 80 x 80 RGB565, two packed files | ~1.93 MB |
| Cry OGG, ~1 s x 151 | several MB | IMA-ADPCM, 8 kHz mono, one index pack | ~0.8 MB |
| Names, types, stats, 1 bio, 8 moves, evolution | small | packed C / JSON then compiled or SPIFFS | 0.2 MB |
| SPIFFS overhead and spare | — | leave headroom | ~1.0 MB |

Rejected for V1:

- Full-resolution official art inside the repo or the firmware.
- All TM / tutor moves (only a short level-up set).
- Opus in the first drop (decoder costs RAM; 151 short cries do not need it).
- Network lookup at runtime (the catalog is offline).
- A second language UI (Chinese first; English names stay as data).

## Reuse from Who Am I

The local Who Am I web game already has:

- Chinese / English names and aliases for 386 Pokemon (`src/data/pokemon.ts`);
- category, two bio sentences, and types (`src/data/pokedex.ts`);
- 386 official-artwork PNGs under `public/pokemon-artwork/`.

V1 copies **ids 1-151 only**. It does not import the quiz, TTS, or silhouette logic. Artwork is converted here; the original ~180 KB PNGs are not committed. If that cache is missing, the fetch script downloads the same official-artwork files from PokeAPI.

Stats, abilities, moves, evolution chains, and cries are not in Who Am I. Those come from PokeAPI.

## Data sources

| Field | Source | Notes |
| --- | --- | --- |
| National id, English name | PokeAPI `/pokemon/{id}` | Stable ids 1-151 |
| Chinese name, category | PokeAPI `/pokemon-species/{id}` `zh-hans` | Fallback to Who Am I if an entry is missing |
| Bio | PokeAPI `zh-hans` flavor text, one sentence | Second sentence is trivia on the bio page if it differs |
| Types | PokeAPI types, mapped to Chinese labels | At most two |
| Abilities | PokeAPI abilities, `zh-hans` names | Generation I had no abilities; still show the modern names |
| Base stats | HP / Atk / Def / SpA / SpD / Spe | Drawn as bars |
| Moves | Red/Blue (or generation-red) level-up, max 8 | Prefer iconic late-level moves if there are more than 8 |
| Evolution | PokeAPI evolution chain | Store from/to ids and a short condition string |
| Picture | Who Am I cache, else PokeAPI official artwork | Converted to an 80 x 80 RGB565 sprite |
| Cry | `https://raw.githubusercontent.com/PokeAPI/cries/main/cries/pokemon/legacy/{id}.ogg` | Legacy clip matches the original 151 better than "latest" |

Raw downloads land in `assets/pokedex/raw/` and stay gitignored. Generated firmware blobs live under `assets/pokedex/gen1/` and are tracked when they are small enough.

## Interaction

Boot goes straight to a home screen with two modes. This play uses a red handheld-dex shell (hinge, LCD well, speaker grille), not the board-template sky / grass / mascot. Battery sits on the top-right of the red bezel.

```text
Home
  Browse  ----+
  Random  ----+--> Entry (always starts on Cover)
                  Up / Down : previous / next id (Browse wraps 1..151;
                              Random also wraps, and OK long-press on
                              Cover is reserved for cry + home)
                  OK click  : Cover -> Bio -> Stats -> Moves -> Matchup -> Evo -> Cover
                  OK long   : play cry when on Cover, then return home
```

Random mode picks a new id when entering from home. After that, Up / Down still walk the dex so the user is not stuck on one entry.

Three keys cannot search by name in V1. Sequential and random cover the request.

## Screen map (240 x 320)

Title bar is the red dex bezel. Content sits in the green inner LCD.

**Cover:** number, 80 x 80 sprite, Chinese name, English name, one or two type chips, category. Opening cover plays the cry.

**Bio:** category plus one wrapped paragraph. If a second distinct sentence exists, it follows.

**Stats:** six bars, values 1-255 scaled to a 100 px track, plus the six-stat total.

**Moves:** every Generation I level-up move, with the learn level.

**Matchup:** defending 4x / 2x / 1/2 / 1/4 / 0, and attacking 2x / 1/2 / 0, using the modern type chart.

**Evolution:** a vertical chain of Chinese names and the condition between them. Single-stage Pokemon show "does not evolve".

## Firmware split

```text
main/pokedex.c          Host-testable catalog + navigation (no LVGL)
main/pokedex_ima.c      IMA-ADPCM decoder
main/pokedex_media.c    SPIFFS mount, sprite load, cry worker
main/demo_pokedex.c     Handheld-dex LVGL chrome, keys
main/font_pokedex_16.*  Subset CJK font from the actual text corpus
assets/pokedex/gen1/    Catalog JSON
assets/pokedex/fs/      Packed sprites1/2.bin + cries.bin (gitignored)
tools/pokedex/          Fetch, convert, font, media pack
tests/test_pokedex.c    Id wrap, random, tab cycle, catalog parse
```

Rules from the board template still apply: LVGL only under `bsp_lvgl_lock()`, keys must not block, audio and SPIFFS reads run on a worker, and `exit` stops every timer or task before deleting the screen.

## Build pipeline

```text
tools/pokedex/fetch_gen1.py
    -> assets/pokedex/gen1/catalog.json
    -> assets/pokedex/raw/art/{id}.png     (gitignore)
    -> assets/pokedex/raw/cries/{id}.ogg   (gitignore)

tools/pokedex/build_media.py
    -> assets/pokedex/fs/sprites1.bin      (ids 1-80, gitignore)
    -> assets/pokedex/fs/sprites2.bin      (ids 81-151, gitignore)
    -> assets/pokedex/fs/cries.bin         (gitignore)

tools/pokedex/gen_font.py
    -> main/font_pokedex_16.c

spiffs_create_partition_image(pokedexfs) in the root CMakeLists
```

Catalog JSON is the checked-in text source. C code can embed a compact form or read the JSON from SPIFFS; V1 prefers a generated `pokedex_catalog.inc` so host tests do not need a filesystem.

## File map

```text
README.md                             Play overview (this fork's root README)
docs/assets/pokedex/README.md         This architecture
docs/assets/pokedex/DEVELOPMENT_PLAN.md
docs/assets/pokedex/TODO.md
docs/assets/pokedex/DECISIONS.md
assets/pokedex/                       Play assets (not mixed into docs)
tools/pokedex/                        Generators
main/pokedex*.c                       Firmware
tests/test_pokedex.c                  Host tests
```
