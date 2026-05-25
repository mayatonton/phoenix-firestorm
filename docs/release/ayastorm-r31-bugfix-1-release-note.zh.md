# AYAstorm r31-bugfix-1 — 发布公告

**r31-bugfix-1 修复了 SSS pink shadow 以后方虚拟形象的轮廓透过 FullBright prim 渗出的结构性 bug** — 这个 bug 自 r20+ 视觉真实感章节首次引入 SSS 以来在结构上一直存在,并非 r31 引入的新问题。

实现细节、routing 调查与设计说明常驻于 `docs/specs/` 下的 4 份 effect 轴 routing 地图。本说明为入口与差异要点。

---

## AYAstorm r31-bugfix-1 — FullBright prim 透过 SSS pink shadow 渗漏修复

### 概要: 消除浮现在 FullBright prim 上、形状与后方虚拟形象一致的 pink shadow

当一个启用了 SSS(次表面散射)肌肤的虚拟形象站在 FullBright prim 后方时,该 prim 表面会浮现出**与虚拟形象同形状**的 pink shadow。此 bug 的天空(sky)变体早前已经修复,但 SSS 变体先后两次(每次 3-4 小时)调查均告失败而被搁置。r31-bugfix-1 在结构层面将其解决。

此 bug 并非 r31 引入。自 r20+ 视觉真实感章节首次接入 SSS 时起就已存在,任何启用了 SSS 且场景中存在 FullBright prim 的 AYAstorm 版本都会受到影响。

### 背景 — 为什么会发生

4 条 single-RT 输出路径都**不会**写入 `gbuffer3.a`(SSS skin flag 通道):

- `POOL_FULLBRIGHT`
- `POOL_FULLBRIGHT_ALPHA_MASK`
- `POOL_BUMP` FB Shiny
- `POOL_ALPHA` 内 FB 路径

因此,当这些 pass 覆盖在已经写入 framebuffer 的虚拟形象之上时,虚拟形象先前 opaque pass 写入的 **stale `aya_sss_skin_flag = 1.0`** 仍然残留在 `gbuffer3.a` 中。SSS pass(`skinSSSF.glsl`)读取 `gbuffer3.a`,判定为「此像素是肌肤」,然后对 `mRT->screen` 进行 blur — 而此时 `mRT->screen` 中已是 FullBright prim 的表面色,经由虚拟形象形状的 flag mask 模糊后,就形成了与被遮挡虚拟形象同形状、透过 FullBright prim 浮现的 pink shadow。

### 修复原理

将 SSS dispatch 位置移动:

- **修复前**: 到达 `POOL_ALPHA_POST_WATER` 时执行 SSS(此时 FB 已经覆盖了 scene color)
- **修复后**: 到达 `POOL_FULLBRIGHT` 之前执行 SSS(在 FB 写入 scene color 之前)

这样 SSS 采样的是 softenLight 之后的原始 skin 色,而非 FB 覆盖后的输出。FB shader 完全未改,仅在 `indra/newview/pipeline.cpp::renderGeomPostDeferred` 中将 dispatch 移动一个块、新增 `done_sss` / `sss_pass` 两个独立 flag/变量,变更完全可逆。

在考虑过的 4 种方案中(案 A: 将 FB shader MRT 化 + 在 C++ pool 侧绑定 MRT,三 OS 全覆盖约半天工作量;案 B/C: 不同的 compositing 思路;案 D: dispatch 重排),**最终采用案 D 作为风险最低、能彻底解决 SSS 路径的方案**,完全不动 FB shader 和 pool 的 MRT 绑定。

### 方法论的转变(技术亮点)

此 bug 此前已两次因在 SSS pass 内打补丁式 `if` 分支而失败。r31-bugfix-1 选择退一步,在动任何代码之前先**并行编写 4 份 effect 轴 routing 地图**:

- SSS rendering routing
- FullBright rendering routing
- Glow rendering routing
- Environment rendering routing

它们与既有的 object 轴 routing 资料(deferred shader / attachment / rez-object / gbuffer3-trace)互补,以「fragment 最终落入哪种视觉 effect」而非「来自哪类对象」的视角追踪同一条 pipeline。4 份独立 agent 调查全部收敛到同一根因 — single-RT FB pass 后 `gbuffer3.a` 的 staleness — 这让根因诊断在动 pipeline 之前就具备了高置信度。

4 份 routing 地图(合计 1894 行)随本次发布一同合入,作为今后描绘系统 bug 调查的可复用资产长期保留。

### 迁移说明

- **无需任何用户端设置改动。** 已安装 r31 的用户直接覆盖安装 r31-bugfix-1 即可
- r31 所有功能(3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker)保持原样工作
- 实际使用 SSS 的用户会看到 FullBright prim 上的 pink shadow 渗漏消失

### 已知限制 / 未来工作

- `gbuffer3.a` 在结构上的双重含义(SSS skin mask 与 emissive MRT blend factor)本次未解决。今后若有 effect 在 single-RT pass 之后读取 `gbuffer3.a`,同类 bug 仍可能复发
- 更根本的结构修复(案 A: 将 FB shader MRT 化,使其写入干净的 `gbuffer3.a`)仍是开放课题。就 SSS 而言本次已完整修复

### 实现概要

- `indra/newview/pipeline.cpp` (+43 / −8 行) — SSS dispatch 分离
- `docs/specs/ayastorm-sss-rendering-routing.md` (335 行,新增)
- `docs/specs/ayastorm-fullbright-rendering-routing.md` (542 行,新增)
- `docs/specs/ayastorm-glow-rendering-routing.md` (534 行,新增)
- `docs/specs/ayastorm-environment-rendering-routing.md` (483 行,新增)
- PR [#112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

### 致谢

- [@t-noami](https://github.com/t-noami) — r31-bugfix-1 的 macOS 构建,以及 AYAstorm 全局的持续实装贡献 (r24 Dullahan audio callback / r25 Ogg Vorbis codec / r26 3D Stream media ring / r27 macOS branding 等)。
- [@mayatonton](https://github.com/mayatonton) — r31-bugfix-1 SSS pink-shadow 修复实装、routing 地图编写、bug 分析。

### 文档

- SSS rendering routing: [`docs/specs/ayastorm-sss-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing(案 D 讨论: §9.1): [`docs/specs/ayastorm-fullbright-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [`docs/specs/ayastorm-glow-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [`docs/specs/ayastorm-environment-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112(已合并): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)
