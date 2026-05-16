# AYAstorm r19 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r19 是视觉真实感章 (r14–r20) 的第六弹、B 轴 (物质色) 的第一弹**。在 deferred lit 路径上加入 wrap-around diffuse + back-light transmission 加算,使叶片、白色窗帘、强逆光下耳缘等 **薄物开始透过太阳光** (此前一律 flat 黑掉)。

> **发布形态**: r19 与 **r23 发布版本一同捆绑发布** (不单独发行 r19 tag)。r23 发布页面会回链至本说明及 r19 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r19-translucency.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r19 — 薄物的透过 (translucency / 太阳光的背面透过)

### r19 主轴: 薄物开始透过太阳光

A 轴 (r14–r18) 让空气变得可信。B 轴转向 **空气包裹之物本身 — 物质** 。r19 落位于入口:让薄物透过太阳光:

- **叶子在太阳的方向透光** — 从树荫向上看天空,叶缘捕捉太阳光透过,叶脉 / 轮廓的质感浮现 (之前是死黑的剪影)
- **白窗帘自内发光** — 站在窗边薄窗帘背后,光绕过窗帘,看起来像从内部发光
- **人物的耳/鼻翼/指尖透红** — 强逆光人像中耳缘透出红色,皮肤开始有「活着」的感觉
- **纸 / 蜡烛 / 薄陶瓷的半透明感** — 薄物质感整体

效果按 **instant A/B 任何人都能看出来** 来设计。前任 r19 (albedo fidelity, 冻结归档 `feature/aya-r19-albedo-fidelity-spec-draft`) 因「数学正确但视认困难」整体 drop,从那次教训中我们把 r19 主动 re-scope 到视觉决定性强的效果。

### 工作原理

**Wrap-around diffuse (Burley wrap) + Back-light transmission 的加算** 在 `class3/deferred/softenLightF.glsl` 的 `sun_contrib` 加算项中以单一路径实现:

```
nl_wrap = max((N·L + w) / (1 + w), 0)            // wrap 项,Legacy 中替换 da
back    = pow(max(-N·L, 0), k_back)              // back-light: 太阳在背面时强
view    = pow(max(V·L, 0), k_view)               // 且视线朝向太阳时
transmit = back * view * tint * strength * sunlit_linear
```

softenLightF.glsl 内 2 处注入覆盖 deferred routing 全部分支 (见 `docs/ayastorm-deferred-shader-routing.md`):

- **Legacy 分支** (墙 / avatar / 耳 / 旧服 → `materialF` writer): 用 `nl_wrap` 替换 `da`,在 `sun_contrib` 加算后再加 `transmit * baseColor.rgb`
- **PBR 分支** (Mesh 服 → `pbropaqueF` writer): 在 `pbrBaseLight()` 调用之后加 `transmit * baseColor.rgb`

C++ 侧按 intensity tier push 4-tuple,slow / fast 两条 `bindDeferredShader` 路径都覆盖,live toggle 可用。

### 局部光 (point / spot) 与太阳

r19 **只对应太阳光**。局部光的 wrap / back-transmission 视为次要效果,如有需要可推迟到 r20+。太阳主导的设计完全覆盖摄影 use case (逆光叶 / 窗后窗帘 / 窗边人像),scope 也保持紧凑。

### 设置

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | 章节 master switch (Firestorm View / AYAstorm View) |
| `AYAR19TranslucencyEnabled` | `1` (ON) | r19 sentinel。`0` 时回到 r18 时代的 lit 计算 |
| `AYAR19TranslucencyIntensity` | `1` (U32, 0-3) | 0=OFF / 1=低调 (default) / 2=标准 / 3=强烈。`(wrap, k_back, k_view, strength)` 的 tier table 保存于 `pipeline.cpp` |

intensity slider 是有意同捆。透过效果属于「默认低调,人像时也想强些」的性质,tier 1 (default) 校准为避免 CG 感失败模式 (所有东西常时透光)。

### 已知限制

- **prim transparency (`alpha > 0`) 不经过 softenLightF** — 这些 prim 走 forward `alphaF` 路径,r19 未触碰。intensity 也不生效。forward 路径注入是 r20+ 的候选
- **Subsurface 厚度依存仅为近似** — r19 借用 `scol` (sun shadow) 作为 self-shadow 衰减,在平面 prim 上也能产生副次性的厚度依存,但并非真正的 diffusion profile。物理 SSS (thickness map / Burley diffusion / multi-scatter) 重 + 需 preset 改造,按章 roadmap §6 永久 drop
- **昼间 (legacy) preset 不受影响** — r19 中没有 pinpoint 除外,但其太阳角度高,back-transmission 自然 minimal,无回归报告

### 实现概要

- `class3/deferred/softenLightF.glsl` — wrap + back-transmission uniform + helper (`ayaTranslucencyWrap`, `ayaTranslucencyTransmit`),Legacy 分支 + PBR 分支 2 个注入点
- `pipeline.cpp::renderDeferredLighting` — softenLightF bind 之后立即 push tier table uniform,slow / fast 两条路径都覆盖
- `settings.xml` — `AYAR19TranslucencyEnabled` Boolean default 1、`AYAR19TranslucencyIntensity` U32 default 1

### 文档

- r19 spec / Burley wrap 参数 / 风险登记: `docs/ayastorm-r19-translucency.md`
- deferred shader routing 参考 (writer → softenLightF 分支表): `docs/ayastorm-deferred-shader-routing.md`
- 冻结的 r19 albedo-fidelity 归档 (前任 scope、drop): `docs/ayastorm-r19-albedo-fidelity.md`
- 视觉真实感章节路线图 (B 轴入口 = r19): `docs/ayastorm-visual-realism-roadmap.md`
