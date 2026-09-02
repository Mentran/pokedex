<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图鉴素材

第一世代玩法资源。二进制立绘和叫声放这里；Markdown 说明放在 `docs/assets/pokedex/`。

| 路径 | 是否入库 | 作用 |
| --- | --- | --- |
| `gen1/catalog.json` | 是 | 151 条文本目录 |
| `gen1/facts.json` | 是 | 原创短句世界小知识 |
| `gen1/sprites/` | 稍后 | 设备精灵图 |
| `gen1/cries/` | 稍后 | IMA-ADPCM 叫声 |
| `raw/` | 否 | 原始 PNG / OGG 下载 |

用 `python3 tools/pokedex/fetch_gen1.py` 重新生成目录。只补身高、捕获、性别和特性介绍时用 `python3 tools/pokedex/enrich_catalog.py`。可选环境变量 `WHOAMI_ROOT` 指向本地「我是谁」缓存，用来取官方立绘。
