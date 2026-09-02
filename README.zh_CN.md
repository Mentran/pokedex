<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 第一世代宝可梦图鉴

给 [AI Passport](https://ai-passport.folotoy.cn) 用的可穿戴图鉴（ESP32-C3，ESP-IDF 5.5.3）。开机直接进第一世代 151 只：按顺序浏览、随机遇见，或看短小知识。每条有立绘、叫声、属性、身高体重、性别、介绍、能力、招式、克制和进化。

方案、容量和数据管线见 [docs/assets/pokedex/README.zh_CN.md](docs/assets/pokedex/README.zh_CN.md)。

## 烧录

已经写过身份的胸牌**不要**执行 `idf.py erase-flash`，也**不要**把 `build/FoloToy-AI-Passport-full.bin` 从 `0x0` 整包刷进去（会清掉 `cardid`）。

```bash
export PATH="$HOME/.espressif/python_env/idf5.5_py3.12_env/bin:/opt/homebrew/bin:$PATH"
. "$HOME/esp/esp-idf-v5.5.3/export.sh"
idf.py flash
```

会写 bootloader、分区表、应用，以及 `0x35A000` 的 `pokedexfs`，跳过 `cardid` 和 Recovery。若 `assets/pokedex/fs/` 里还没有打包好的立绘和叫声，先运行 `python3 tools/pokedex/build_media.py`。

## 按键

| 按键 | 首页 | 条目里 |
| --- | --- | --- |
| 上 / 下 | 移动菜单 | 上一只 / 下一只（克制页先滚动） |
| 确定短按 | 进入所选入口 | 下一页：封面、介绍、能力、招式、克制、进化。放大时还原封面 |
| 确定双击 | — | 封面放大立绘到全屏 3 倍 |
| 确定长按 | 回到 BSP 演示菜单 | 回图鉴首页（从封面离开时再播叫声） |

小知识：上下键或短按确定翻池子；长按确定回首页。

## 粉丝向说明

个人、非商业的粉丝向玩法。宝可梦名称、角色、立绘、叫声和图鉴文本的权利归 Nintendo、The Pokémon Company、Game Freak。目录数据来自 [PokeAPI](https://pokeapi.co)，叫声来自 [PokeAPI/cries](https://github.com/PokeAPI/cries)。
