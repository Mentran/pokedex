<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 第一世代宝可梦图鉴

给 [AI Passport](https://ai-passport.folotoy.cn) 用的可穿戴图鉴：按全国图鉴 1–151 顺序翻看，也可以随机跳一只。每条条目包含图片、官方风格介绍、叫声、属性、能力值、招式和进化路线。

当前分支是 `feature/pokemon-pokedex`。它和成语接龙**不共用同一份工作区**。成语接龙仍留在原来的目录和 `feature/chengyu-play` 分支，互不覆盖。

## 真机试玩

已经写过身份的胸牌**不要**执行 `idf.py erase-flash`，也**不要**把带 `pokedexfs` 的 `build/FoloToy-AI-Passport-full.bin` 从 `0x0` 整包刷进去（会把 `cardid` 写成 0xFF）。在本仓库里：

```bash
export PATH="$HOME/.espressif/python_env/idf5.5_py3.12_env/bin:/opt/homebrew/bin:$PATH"
. $HOME/esp/esp-idf-v5.5.3/export.sh
python3 tools/pokedex/build_media.py
idf.py flash
```

`idf.py flash` 会写 bootloader、分区表、应用，以及 `0x35A000` 的 `pokedexfs`，跳过 `cardid` 和 Recovery。USB 走 Type-C 原生串口。

胸牌上：上下键选「顺序查看 / 随机查看」，确定进入条目（封面会叫），上下换精灵，确定翻页，长按确定返回（从封面离开时会再叫一声）。

## 这块硬件实际装得下什么

AI Passport 是 ESP32-C3 胸牌：240 × 320 彩屏、三键、喇叭、8 MB Flash、没有 PSRAM，应用分区上限 3 MB，这样才能继续用小程序 Recovery 刷机。

151 只宝可梦的官方原图加原始叫声，原样塞不进去。所以这套玩法会：

- 开机直接进入图鉴（不走硬件 demo 菜单）；
- 不改 Recovery、`cardid` 和 3 MB 应用槽；
- 把图片和叫声放到 `cardid` 与 Recovery 之间的新数据分区（约 3.65 MB）；
- 把立绘缩成 80 × 80 屏上精灵图，叫声编成短 IMA-ADPCM。

容量账、屏幕分区和数据管线见 [docs/assets/pokedex/README.zh_CN.md](docs/assets/pokedex/README.zh_CN.md)。

## 按键

| 按键 | 首页 | 条目页 |
| --- | --- | --- |
| 上 / 下 | 顺序查看 / 随机查看 | 上一只 / 下一只 |
| 确定短按 | 进入所选模式 | 下一页资料（封面、介绍、能力、招式、克制、进化） |
| 确定长按 | — | 回首页。在封面页同时播放叫声 |

## 项目文件

| 路径 | 作用 |
| --- | --- |
| [docs/assets/README.zh_CN.md](docs/assets/README.zh_CN.md) | fork 专用文档索引 |
| [docs/assets/pokedex/README.zh_CN.md](docs/assets/pokedex/README.zh_CN.md) | 可行方案与架构 |
| [docs/assets/pokedex/DEVELOPMENT_PLAN.zh_CN.md](docs/assets/pokedex/DEVELOPMENT_PLAN.zh_CN.md) | 里程碑与验收 |
| [docs/assets/pokedex/TODO.zh_CN.md](docs/assets/pokedex/TODO.zh_CN.md) | 任务板 |
| [docs/assets/pokedex/DECISIONS.zh_CN.md](docs/assets/pokedex/DECISIONS.zh_CN.md) | 决策记录 |

本玩法的固件、可主机测试的逻辑和素材都以本仓库为准。不要把真源留在聊天记录或「我是谁」网页游戏里。

## 粉丝向说明

这是个人、非商业的粉丝向玩法。宝可梦名称、角色、立绘、叫声和图鉴文本的权利归 Nintendo、The Pokémon Company、Game Freak 所有。数据由 [PokeAPI](https://pokeapi.co) 生成；图片优先复用「我是谁」项目里已缓存的官方立绘，缺失时再下 PokeAPI 立绘；叫声来自 [PokeAPI/cries](https://github.com/PokeAPI/cries)。
