<p align="right">
  <a href="DECISIONS.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Decision log

## 2026-09-04 — Who am I, BGM, three boot scenes

Home gains Who am I: a black silhouette, OK reveals the dex, Up/Down draws the next silhouette. Boot is one Gengar-vs-Nidorino scene for about 4 seconds, skippable; staging follows the cover art (Gengar left, Nidorino right) on the same LCD green as the dex, with no fade-to-white. BGM starts after home at codec volume 40; cries play at 60. Lecture portraits are upper-body crops at 80px in the bottom-left or bottom-right of the text panel.

## 2026-09-04 — Official art for boot and lecture

Boot Pokémon use the same 80×80 cover sprites: Gengar 94, Nidorino 33, Pikachu 25, Charmander 4, Squirtle 7. Lecture portraits: Oak `oak-gen3`, Ash `red-lgpe`, Brock `brock-lgpe`, Misty `misty-lgpe`, Gary `blue-lgpe`. Let's Go has no Oak battle sprite. The five portraits pack into `trainers.bin` on pokedexfs.

## 2026-09-03 — Boot animation and Oak's lecture

Boot plays fullscreen before the red shell. One of five original pixel scenes is chosen at random, about 1.5 seconds, skippable with any key. Do not ship the official Red/Green opening. The third home item is Oak's lecture. Speakers are Oak, Ash, Brock, Misty, and Gary; they reroll on enter and on every flip. The sentence pool stays as-is. 16x16 pixels live in firmware, not pokedexfs.

## 2026-09-02 — Isolated worktree

The idiom-chain play stays on `feature/chengyu-play`. This play uses a second git worktree and `feature/pokemon-pokedex`, branched from upstream `main`. Do not merge the two catalogs.

## 2026-09-02 — Generation I only

151 entries is already a flash and font problem. Later generations wait until this set is on hardware.

## 2026-09-02 — Resource partition, not a smaller app

Keep the 3 MB factory app. Put pictures and cries in `pokedexfs` at `0x35A000` / `0x3A6000`, which ends exactly at Recovery. This preserves mini-program BLE install.

## 2026-09-02 — Offline catalog

The badge has Wi-Fi, but V1 does not fetch at runtime. A complete local catalog is simpler, cheaper on RAM, and works at a meetup with no network.

## 2026-09-02 — Legacy cries, IMA-ADPCM

Use PokeAPI legacy OGG (original-style clips) and IMA-ADPCM rather than Opus. The Voice Keychain experience shows Opus wins on hours of speech; 151 one-second cries do not justify the decoder.

## 2026-09-02 — Reuse Who Am I as a cache, not as the product

Who Am I may supply a local official-art cache via `WHOAMI_ROOT`. This play does not vendor that project's sources, quiz, or PNGs. Catalog text and cries come from PokeAPI.

## 2026-09-02 — Abilities are modern names

Generation I battles had no abilities. The Pokedex still shows the modern ability names because a blank "ability" row is worse than a later-generation label.

## 2026-09-02 — Evolution stays inside 1-151

PokeAPI chains include later forms (Pichu, later Eeveelutions). V1 drops any node whose national id is outside 1-151 so the dex matches the Generation I roster.

## 2026-09-02 — Text-first device slice

Sprites and cries are not required to validate keys, paging, and Chinese text on hardware. A text firmware boots into the dex first; art and audio follow once the interaction feels right.

## 2026-09-02 — No name search in V1

Three keys cannot type. Sequential browse plus random covers the requested interaction.

## 2026-09-02 — Handheld dex chrome, 80x80 split sprites

After the text build confirmed the key flow, the UI became a red handheld dex instead of the template sky/grass. Sprites are 80x80 RGB565 split into `sprites1.bin` / `sprites2.bin` so no SPIFFS file exceeds about half the partition. Cries stay legacy OGG to IMA-ADPCM. Provisioned badges must use segmented `idf.py flash`; do not write a merged `full.bin` that contains `pokedexfs` from `0x0`.

## 2026-09-02 — Full level-up moves and modern matchups

The moves page lists every red/blue (then yellow / FR/LG) level-up move, not a late-game slice of eight. Stats show the six-stat total. The extra matchup page uses the modern type chart because the catalog already stores modern types such as Fairy and Steel.

## 2026-09-02 — Extra fields stay on existing pages

Height and weight sit on cover with gender ratio. Catch rate stays in the catalog but is not shown. Ability names plus flavor sit on stats, labeled as abilities. The second dex sentence sits on bio when it is not a duplicate of intro. World facts remain Oak's lecture, a shared pool not tied to the current id.

## 2026-09-02 — Cover double-click zooms the sprite

Cover OK click still opens Bio. Double-click OK toggles a 3x nearest-neighbor fullscreen sprite; OK click restores. Up / Down while zoomed keep the large view and change the id.
