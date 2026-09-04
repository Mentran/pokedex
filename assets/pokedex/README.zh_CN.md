<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图鉴素材

第一世代玩法资源。二进制立绘和叫声放这里；Markdown 说明放在 `docs/assets/pokedex/`。

| 路径 | 是否入库 | 作用 |
| --- | --- | --- |
| `gen1/catalog.json` | 是 | 151 条文本目录 |
| `gen1/facts.json` | 是 | 原创短句世界小知识 |
| `fs/sprites1.bin`、`fs/sprites2.bin`、`fs/cries.bin`、`fs/trainers.bin` | 否 | 打包后的机上素材 |
| `trainers/` | 是 | 大木讲堂五人立绘 PNG 源文件 |
| `raw/` | 否 | 原始 PNG / OGG 下载 |

用 `python3 tools/pokedex/fetch_gen1.py` 重新生成目录。只补身高、捕获、性别和特性介绍时用 `python3 tools/pokedex/enrich_catalog.py`。可选环境变量 `WHOAMI_ROOT` 指向本机官方立绘目录，只用于打包精灵图，该缓存不入库。
