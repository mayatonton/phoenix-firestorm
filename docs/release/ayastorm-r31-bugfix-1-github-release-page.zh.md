🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.zh.md)

# AYAstorm r31-bugfix-1 — FullBright prim 透过 SSS pink shadow 渗漏修复

r31-bugfix-1 是一次**单 bug 修复发布**,消除启用 SSS 肌肤时透过 FullBright prim 浮现的、形状与后方虚拟形象一致的 pink shadow。该 bug 自 r20+ 视觉真实感章节首次接入 SSS 以来在结构上一直存在,先前两次(各 3-4 小时)调查均告失败。

修复思路是从 SSS pass 内打补丁式 `if` 分支退一步,**并行编写 4 份 effect 轴 rendering routing 地图(SSS / FullBright / Glow / Environment)**。4 份独立调查全部收敛到同一根因 — single-RT FB pass 后 `gbuffer3.a` 的 staleness — 在此基础上才动 pipeline。实际修复仅是 `pipeline.cpp` 中 dispatch 一个块的位置移动,FB shader 完全未改。

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-1-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-1-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-1-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.zh.md)

## 关键文档(tag 锁定)

- SSS rendering routing: [docs/specs/ayastorm-sss-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing(案 D 讨论 §9.1): [docs/specs/ayastorm-fullbright-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [docs/specs/ayastorm-glow-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [docs/specs/ayastorm-environment-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112(已合并): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

## 与现有环境的兼容性

不扰动 r31 环境即可发布:

- **SSS pink-shadow 渗漏修复**: 自动生效。无需任何用户端设置改动。已安装 r31 的用户直接覆盖安装 r31-bugfix-1,FB prim 上的 pink-shadow 渗漏即消失
- **r31 全部功能**(3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb 等): 全部原样保留
- **不使用 SSS 的用户**: 无可见变化。当场景中没有 SSS 肌肤像素时,dispatch 重排在功能上即为 no-op
- **FB shader**: 完全未改。本修复完全可逆

## IR licence

r11 同捆并持续出货的 venue IR 来自 OpenAIR(CC-BY 4.0)。来源: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

_2 OS(Linux / Windows)构建完成后由 @mayatonton 填写。_

- Linux Installer: _TBD_ — `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-XXXXXX.tar.xz`
- Windows Installer: _TBD_ — `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-XXXXXX_Setup.exe`
- macOS Installer: **本次发布不提供。** 将在即将到来的 r32 章一并同捆(按 r10.x 先例,bugfix-N 版本仅提供 Linux/Windows)

## Contributors

@mayatonton
