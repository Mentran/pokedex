<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Fonts

Store reusable font files and generated font sources here.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

`tools/pokedex/gen_font.py` downloads `NotoSansSC-Regular.otf` to `~/.cache/ai-passport/` (not committed) and writes `main/font_pokedex_16.c`, a 16 px 4 bpp subset of catalog and UI glyphs. Noto Sans SC is SIL Open Font License.
