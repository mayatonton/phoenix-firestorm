# AYAstorm r14 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r14 是视觉真实感章 (r14–r20) 的开篇 release**,在 SL 既有的 Beer-Lambert + in-scatter atmospherics 之上加入高度依存的密度梯度和 scene-referred (linear 空间) 积分,在保持 WindLight preset 兼容的前提下让空气开始具备"体积感"。

> **发布形态**: r14 与 **r23 发布版本一同捆绑发布** (不单独发行 r14 tag)。r23 发布页面会回链至本说明及 r14 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r14-volumetric-atmosphere.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r14 — Volumetric atmosphere (空气的体积感)

### r14 主轴: 空气从「平面色调」变为「介质」

r13 之前的 SL atmospherics 已经具备 Beer-Lambert 衰减和 in-scatter (haze_glow),但物理上缺少两个要素,导致天空看起来仅是「平面渐变」:

- **缺少高度依存的密度梯度**: 地表和上空 density 相同,无法呈现「地表朦胧、上空通透」这种阴天物理质感
- **缺少 scene-referred 积分**: `additive` 在 sRGB 中合成后再线性化的路径,在 HDR scene buffer 上物理一致性不足

r14 在 **不重写既有 pipeline 的前提下** 补上这两点:

- 新的 `calcAtmosphericVars` 分支引入高度向 exponential 密度 profile (由 preset `max_y * 0.1` 导出的 `scale_height` 控制)
- 将 `additive` 和 `blue_horizon` 的合成在 `atmosphericsFuncs.glsl` / `skyV.glsl` 内移至 linear 空间,使 depth-aware 的空气合成在 HDR scene buffer 上物理正确落地
- WindLight preset 的取值仅作为输入重新解释,保持 preset 兼容

实现 **解析式、轻量** (无 raymarch)。重型 volumetric 处理 (godrays / cloud volume / aerial perspective) 拆分至 r15–r18。

详情 → spec `docs/ayastorm-r14-volumetric-atmosphere.md`

### Master switch (本章共通开关在 r14 引入)

r14 引入了 **章节级 master switch** (r14–r20 共享):

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 视觉真实感章节整体的 master switch。设为 `0` 后 atmospherics / 空合成 / (后续) godrays / aerial perspective / cloud volume / translucency / avatar SSS 全部回到 r14 之前的行为 |

> **不另外提供每个功能的 debug cvar。** 比起众多 tuning 键,更优先 1 个妥当默认值 (memory `feedback_prefer_defaults_over_config.md`)。

### 已知 trade-off

- **Linear 空间合成让 midtone 略显平坦** — 整体上「阴天感」增强。地平线 (朝/夕 / haze 质感) 因物理一致性而改善,这里是 trade-off。可用 preset 值 push back,后续章节 (r15+) 通过 dispersion / godrays / aerial perspective 再补回丰富度。
- **保护太阳 disc 的 split (P2.a refined)**: shader 内分流,`haze_horizon` (太阳方向的 glow) 保留旧 sRGB 路径,避免 linear 化后的 haze peak 把 sun disc 烧白。仅 `blue_horizon` (全方向的蓝) 切到 linear。
- **Preetham 太阳方向光路 (P2.b) 与 Rayleigh/Mie 波长分离 (P2.c) 暂缓** — 早期实验出现 sun disc 劣化故 drop,后续章节中以「保护 sun disc」为约束条件重新挑战。

### 实现概要

- `atmosphericsFuncs.glsl` — master switch 下加入高度密度 profile + linear `additive` 合成
- `skyV.glsl` — master switch 下加入 linear `blue_horizon` 合成 (vertex shader 不 attach `srgbF.glsl`,因此就地内联 `aya_srgb_to_linear` / `aya_linear_to_srgb`)
- `LLSettingsVOSky::applyToShader` — `aya_visual_realism_enabled` uniform plumbing (template = `classic_mode`)
- `LLShaderMgr` enum + `mReservedUniforms` 扩充
- `settings.xml` — `AYAVisualRealismEnabled` Boolean default 1

### 文档

- r14 spec / pipeline 依据 / 已知 trade-off / 风险登记: `docs/ayastorm-r14-volumetric-atmosphere.md`
- P0 atmospheric pipeline 调查: `doc/r14/volumetric_atmosphere_survey.md`
- 视觉真实感章节路线图 (r14–r20 整体): `docs/ayastorm-visual-realism-roadmap.md`
- 废案的 r14 sun-dazzle 草案 (pivot 经过): `docs/ayastorm-r14-sun-dazzle.md`
