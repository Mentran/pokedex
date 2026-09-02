<p align="right">
  <strong>简体中文</strong> · <a href="TODO.md">English</a>
</p>

# 任务板

当前：手持图鉴外壳 + 立绘 + 叫声。还差真机刷入确认。

## M0

- [x] 根目录 README 配对
- [x] 架构、计划、TODO、决策
- [x] `main/pokedex.c` 翻页 + `tests/test_pokedex.c`
- [x] `tools/pokedex/fetch_gen1.py` 写出 151 条 `catalog.json`
- [x] 忽略 `assets/pokedex/raw/`

## M1

- [x] 用 PokeAPI 介绍加 trivia；不导入「我是谁」文本文件
- [x] 生成 `pokedex_catalog.inc`
- [x] 151 条解析 + 四条抽查的主机测试

## M2

- [x] 转换 151 张精灵图
- [x] 记录总体积对照 1.6 MB 预算
- [x] 封面图

## M3

- [x] 子集字库
- [x] 五页资料 + 首页（文字版）
- [x] 开机直接进玩法

## M4

- [x] legacy 叫声 -> ADPCM
- [x] `pokedexfs` 分区
- [x] 叫声播放任务

## M5

- [x] 电量位置
- [x] 随机不立刻重复
- [x] changelog
- [ ] 真机验收

## 验证记录

| 日期 | 检查 | 结果 |
| --- | --- | --- |
| 2026-09-02 | 从上游 `main` 拉出独立工作树 `feature/pokemon-pokedex` | 通过：成语接龙目录未动 |
| 2026-09-02 | 主机测试 `test_pokedex` + 151 条目录 | 通过 |
| 2026-09-02 | `./tools/validate.sh --firmware` 文字图鉴 | 通过：应用 1750160 字节，合并镜像 1.7 MB |
| 2026-09-02 | 手持图鉴外壳 + 151 立绘 + 叫声 | 通过：立绘 1.93 MB 拆两包，叫声 507 KB，应用 1783344 字节，BLE 契约通过。待真机刷入 |
