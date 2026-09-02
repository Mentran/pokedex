<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Generation I Pokedex

A wearable Pokedex play for [AI Passport](https://ai-passport.folotoy.cn): browse the original 151 Pokemon in order, or jump to a random entry. Each entry shows a picture, a short official-style bio, cry, types, base stats, signature moves, and the evolution line.

This branch is `feature/pokemon-pokedex`. It does **not** share a working tree with the idiom-chain play. That work stays in its own directory and branch.

## Flash a device

Do **not** run `idf.py erase-flash` on a provisioned badge, and do **not** write `build/FoloToy-AI-Passport-full.bin` from `0x0` once `pokedexfs` is in the merge (that would 0xFF-wipe `cardid`). From this repo:

```bash
export PATH="$HOME/.espressif/python_env/idf5.5_py3.12_env/bin:/opt/homebrew/bin:$PATH"
. $HOME/esp/esp-idf-v5.5.3/export.sh
python3 tools/pokedex/build_media.py
idf.py flash
```

`idf.py flash` writes bootloader, partition table, the app, and `pokedexfs` at `0x35A000`. It skips `cardid` and Recovery. USB is the native Type-C serial port.

On the badge: Up/Down chooses Browse or Random, OK opens an entry (cover plays the cry), Up/Down changes Pokemon, OK cycles pages, OK long-press returns (and replays the cry when leaving the cover).

## What this device can actually hold

AI Passport is an ESP32-C3 badge: 240 x 320 color screen, three keys, speaker, 8 MB flash, no extra RAM, and a 3 MB application ceiling so the mini-program Recovery installer still works.

Uncompressed official art plus raw cries for 151 Pokemon will not fit. The play therefore:

- boots straight into the Pokedex (no hardware-demo menu);
- keeps Recovery, `cardid`, and the 3 MB app slot untouched;
- stores pictures and cries in a new data partition between `cardid` and Recovery (~3.65 MB);
- downscales art to an 80 x 80 screen sprite and encodes cries as short IMA-ADPCM clips.

See [docs/assets/pokedex/README.md](docs/assets/pokedex/README.md) for the flash budget, screen map, and data pipeline.

## Controls

| Key | Home | Entry |
| --- | --- | --- |
| Up / Down | Browse or Random | Previous / next Pokemon |
| OK click | Open the selected mode | Next info page (cover, bio, stats, moves, evolution) |
| OK long-press | — | Back to home. On the cover page, also plays the cry |

## Project files

| Path | Role |
| --- | --- |
| [docs/assets/README.md](docs/assets/README.md) | Index of fork-only documents |
| [docs/assets/pokedex/README.md](docs/assets/pokedex/README.md) | Feasible design and architecture |
| [docs/assets/pokedex/DEVELOPMENT_PLAN.md](docs/assets/pokedex/DEVELOPMENT_PLAN.md) | Milestones and acceptance |
| [docs/assets/pokedex/TODO.md](docs/assets/pokedex/TODO.md) | Task board |
| [docs/assets/pokedex/DECISIONS.md](docs/assets/pokedex/DECISIONS.md) | Decision log |

Firmware, host-tested logic, and assets for this play all live in this repository. Do not keep the source of truth in chat history or in the Who Am I web game.

## Fan-work notice

This is a personal, non-commercial fan play. Pokemon names, characters, artwork, cries, and Pokedex text belong to Nintendo, The Pokemon Company, and Game Freak. Data is generated from [PokeAPI](https://pokeapi.co); pictures reuse the local official-artwork cache from the Who Am I project when present, otherwise PokeAPI artwork; cries come from [PokeAPI/cries](https://github.com/PokeAPI/cries).
