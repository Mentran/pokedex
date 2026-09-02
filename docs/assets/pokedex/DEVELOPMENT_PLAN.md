<p align="right">
  <a href="DEVELOPMENT_PLAN.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Development plan

North star: on a real AI Passport, a user can browse all 151 entries, open every info page, and hear a cry, without breaking mini-program Recovery.

Work in this order. Do not start a later milestone while the previous one's acceptance is still red, except for documentation.

## M0 — Project skeleton

- Root README pair and the documents in `docs/assets/pokedex/`.
- Host-testable navigation: wrap 1-151, random in range, tab cycle.
- Fetch script that writes `assets/pokedex/gen1/catalog.json`.
- `.gitignore` for raw art and cries.

Acceptance: `./tools/validate.sh --static` includes `tests/test_pokedex.c`; catalog has 151 ids with Chinese name, types, stats, moves, and evolution fields.

## M1 — Catalog quality

- Fill missing Chinese fields from the Who Am I cache if PokeAPI is short.
- Cap moves at 8. Keep evolution conditions readable on a 240 px panel.
- Generate `pokedex_catalog.inc` from JSON.

Acceptance: host tests parse every entry; spot-check Bulbasaur, Pikachu, Eevee, and a fossil Pokemon.

## M2 — Sprites

- Convert 151 images to 80 x 80 device sprites.
- Split RGB565 packs so no SPIFFS file is larger than about half of `pokedexfs`.
- Draw the cover page with handheld-dex chrome.

Acceptance: sprite partition usage logged; Pikachu and Charizard are recognizable on a screenshot or device.

## M3 — Chinese UI

- Subset 16 px font from the catalog plus UI strings.
- Cover, bio, stats, moves, evolution pages.
- Home: Browse / Random. Boot directly into the play.

Acceptance: every page shows CJK without tofu; OK cycles pages; Up/Down changes id.

## M4 — Cries and data partition

- Download legacy OGG, encode IMA-ADPCM.
- Add `pokedexfs` at `0x35A000`, size `0x3A6000`.
- Worker task plays a cry on Cover + OK long-press (then home).

Acceptance: `./tools/validate.sh --firmware` still honors Recovery contracts; one cry plays on device.

## M5 — Device polish

- Battery on the top-right of the red bezel.
- Random mode does not repeat the last id.
- Empty evolution and missing cry fail softly.
- Changelog entry for the play.

Acceptance: full 151 walk on hardware, or a recorded pass of first, middle, last, and a random jump.

## Out of scope until V1 ships

Name search, later generations, shiny forms, type-chart quiz, Wi-Fi updates, catching / owned flags.
