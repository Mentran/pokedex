<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 第一世代宝可梦图鉴架构

这是原 151 只宝可梦做成机上图鉴的可行方案，也是本玩法的技术真源。Flash 预算、页面或数据管线有变时，先改这份文档。

## 目标

把胸牌挂在身上，用三个键查宝可梦：先看图，再看介绍、叫声、属性、能力值、招式和进化。

V1 范围只有**全国图鉴 1–151**。真机跑通之前，不做后续世代。

## 决定方案的硬件事实

| 事实 | 数值 | 影响 |
| --- | --- | --- |
| MCU | ESP32-C3，无 PSRAM | 一次只解码一张图、一段叫声；图鉴不能整本进内存 |
| Flash | 8 MB | 有空间，但 Recovery 和 `cardid` 是保护区 |
| 应用分区 | `0x10000` 起 3 MB | 只放固件、中文子集字库和逻辑 |
| 保护分区 | `cardid@0x356000`、`recovery@0x700000` | 不得移动、不得重叠 |
| 屏幕 | 240 × 320 RGB565，无触摸 | 一页一张卡片，不做密密麻麻的表 |
| 输入 | 上、下、确定 | 上下翻精灵，确定翻资料页 |
| 音频 | ES8311，16-bit PCM | 工作任务里播放解出来的 PCM |

可用数据窗口是 **`0x35A000` 到 `0x700000`**：3,825,664 字节（约 3.65 MB）。分区名 `pokedexfs`（SPIFFS）。除非以后测出固件塞不进 3 MB，否则不要去砍应用分区。

带这个分区的合并镜像，不能整文件刷到已经写过设备身份的机器上。`factory` 和 `pokedexfs` 要分段烧录，或走小程序 Recovery。已经开通的胸牌禁止 `erase-flash`。

## 什么装得下，什么装不下

| 素材 | 原样体积 | V1 编码 | 预算 |
| --- | --- | --- | --- |
| 官方立绘 PNG，约 180 KB × 151 | 约 27 MB | 80 × 80 RGB565，拆成两个打包文件 | 约 1.93 MB |
| 叫声 OGG，约 1 秒 × 151 | 数 MB | IMA-ADPCM，8 kHz 单声道，一个索引包 | 约 0.8 MB |
| 名称、属性、能力值、介绍、全部升级招式、进化 | 很小 | 打包成 C / JSON 再编译 | 0.2 MB |
| SPIFFS 开销和余量 | — | 留余量 | 约 1.0 MB |

V1 明确不做：

- 把官方原图原分辨率放进仓库或固件；
- 技能机 / 教学招式（只保留升级自学招式）；
- 第一版就上 Opus（解码器吃 RAM；151 段短叫声用不上）；
- 运行时联网查图鉴（目录离线）；
- 第二套语言界面（界面中文优先；英文名只作为数据字段）。

## 立绘

设备精灵图是 gitignore 的 `assets/pokedex/fs/` 里 80 × 80 RGB565。设置了 `WHOAMI_ROOT` 时，`tools/pokedex/build_media.py` 优先读 `$WHOAMI_ROOT/public/pokemon-artwork/`；否则从 PokeAPI 下载官方立绘到 gitignore 的 `assets/pokedex/raw/`。原 PNG 和其他项目的源文件不进本仓库。

目录文本和叫声来自 PokeAPI。

## 数据来源

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| 全国编号、英文名 | PokeAPI `/pokemon/{id}` | 1–151 稳定 |
| 中文名、分类 | PokeAPI `/pokemon-species/{id}` 的 `zh-hans` | 编进目录 |
| 介绍 | PokeAPI `zh-hans` 图鉴文本，一句 | 若有第二句且不重复，放在介绍页当补充 |
| 属性 | PokeAPI types，映射成中文 | 最多两个 |
| 身高 / 体重 | PokeAPI `height`（分米）、`weight`（百克） | 封面显示成米和千克 |
| 捕获 / 性别 | PokeAPI `capture_rate`、`gender_rate` | 捕获率入库但不显示；封面只写性别（`-1` 无性别；`0..8` 为雌性八分比） |
| 特性 | PokeAPI abilities 的 `zh-hans` 名和介绍 | 初代没有特性，这里展示现代特性名加介绍 |
| 能力值 | HP / 攻击 / 防御 / 特攻 / 特防 / 速度 | 画成条，并显示六项总和 |
| 招式 | 红绿（缺则黄 / 火红叶绿）升级招式 | 列出全部升级自学招式；不加技能机、威力和招式说明 |
| 进化 | PokeAPI evolution chain | 记录从/到编号和一句条件 |
| 世界小知识 | `assets/pokedex/gen1/facts.json` | 原创短句，不复制图鉴原文 |
| 图片 | PokeAPI 官方立绘；可选 `WHOAMI_ROOT` 缓存 | 转成 80 × 80 RGB565 精灵图；原 PNG 保持 gitignore |
| 叫声 | `https://raw.githubusercontent.com/PokeAPI/cries/main/cries/pokemon/legacy/{id}.ogg` | legacy 更接近初代 151，不用 latest |

原始下载放进 `assets/pokedex/raw/`，并加入 gitignore。打包后的立绘和叫声在 `assets/pokedex/fs/`，同样 gitignore。入库文本是 `assets/pokedex/gen1/catalog.json` 和 `facts.json`。

## 交互

开机先全屏随机抽一条短动画（开场对战 / 图鉴苏醒 / 对手登场），任意键跳过，再进首页。首页四个入口。这套玩法用红色手持图鉴外壳（转轴、内屏、喇叭孔），不用模板的天空 / 草地 / 吉祥物。电量放在红色顶框右上角。进入图鉴后循环播放「我是谁」同款 8-bit 宝可梦中心 BGM，音量 40、节奏约 0.75 倍；叫声音量 60，播叫声时会暂时打断。

```text
开机动画（全屏，可跳过）
首页
  图鉴浏览  ----+--> 条目（总是先封面）
  随机遇见  ----+    上 / 下 : 上一只 / 下一只
                     （克制页：内容超出时先滚动，滚到头再换编号）
                     确定短按 : 封面 -> 介绍 -> 能力 -> 招式 -> 克制 -> 进化 -> 封面
                                 （放大时短按还原）
                     确定双击 : 封面放大立绘
                     确定长按 : 在封面播放叫声，然后回首页
  猜猜我是谁 --+--> 剪影题（只显示黑色背影）
                     确定短按 : 揭晓，进入该只的图鉴资料
                     上 / 下 : 下一只剪影（不是下一只图鉴）
                     确定长按 : 回首页
  大木讲堂 ----+--> 讲堂页（全文 + 左下或右下小人像）
                     进入和上下翻条时都再抽人像（大木 / 小智 / 小刚 / 小霞 / 小茂）
                     上 / 下 : 上一条 / 下一条
                     确定短按 : 下一条
                     确定长按 : 回首页
```

随机模式从首页进入时抽一个新编号。之后上下键仍按图鉴顺序走。猜猜我是谁每次上下都换新剪影。讲堂进入时随机一条，之后上下键在池子里翻；人像每次进入和翻条都换，正文池不改口吻。

三个键在 V1 做不了按名搜索。顺序和随机已经覆盖这次需求。

## 屏幕分区（240 × 320）

顶栏是红色图鉴边框。内容放在绿色内屏里。

**封面：** 编号、80 × 80 图、中文名、英文名、一或两个属性标签、分类、身高体重和性别比。打开封面会播叫声。双击确定把立绘放到全屏 3 倍（最近邻）；短按确定还原，封面短按仍进介绍。

**介绍：** 一段折行正文。若有第二句且不重复，接在后面。

**能力：** 六条更紧的能力条、六项总和，然后用「特性：」标出现代特性名和介绍。

**招式：** 全部初代升级自学招式，带学会等级。不加威力和招式说明。

**克制：** 被打和打出放同一页，属性用彩色底。内容超出内屏时，上下键滚动；滚到头仍可换宝可梦。

**进化：** 中文名竖着排，中间写进化条件。单阶宝可梦显示「不会进化」。

首页文案是「宝可梦图鉴 / 第一世代151只」，三个入口为图鉴浏览、随机遇见、大木讲堂。讲堂页不显示条数。

## 固件切分

```text
main/pokedex.c          可主机测试的图鉴目录和翻页（不含 LVGL）
main/pokedex_media.c    SPIFFS 立绘、叫声和大木讲堂人像
main/pokedex_ima.c      IMA-ADPCM 解码
main/pokedex_media.c    SPIFFS 挂载、读精灵图、叫声工作任务
main/demo_pokedex.c     手持图鉴 LVGL 外壳和按键
main/font_pokedex_16.*  按实际文案子集生成的 16 像素中文字库
assets/pokedex/gen1/    目录 JSON 和世界小知识池
assets/pokedex/fs/      打包后的 sprites1/2.bin + cries.bin（gitignore）
tools/pokedex/          抓取、转换、字库、素材打包
tests/test_pokedex.c    编号循环、随机、页循环、目录解析
```

板级模板规则仍然有效：访问 LVGL 必须持 `bsp_lvgl_lock()`，按键回调不得阻塞，音频和 SPIFFS 读取放工作任务，`exit` 在删屏前停掉全部定时器和任务。

## 构建管线

```text
tools/pokedex/fetch_gen1.py
    -> assets/pokedex/gen1/catalog.json

tools/pokedex/enrich_catalog.py
    -> 补身高、体重、捕获率、性别比、特性介绍

tools/pokedex/gen_catalog_inc.py
    -> main/pokedex_catalog.inc
    -> main/pokedex_facts.inc

tools/pokedex/build_media.py
    -> assets/pokedex/fs/sprites1.bin      (编号 1-80，gitignore)
    -> assets/pokedex/fs/sprites2.bin      (编号 81-151，gitignore)
    -> assets/pokedex/fs/cries.bin         (gitignore)

tools/pokedex/gen_font.py
    -> main/font_pokedex_16.c

根目录 CMakeLists 里的 spiffs_create_partition_image(pokedexfs)
```

`catalog.json` 是入库的文本真源。V1 生成 `pokedex_catalog.inc`，主机测试不依赖文件系统。
