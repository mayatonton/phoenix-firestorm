# AYAstorm r16 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r16 是视觉真实感章 (r14–r20) 的第三弹**,在 scene aerial perspective 路径上加入波长依存 (Rayleigh λ⁻⁴) 的 in-scatter weighting,使远景随距离向蓝色方向偏移。保持 WindLight preset 兼容,**完全不动 sky dome** (sun disc 在结构上得到保护)。

> **发布形态**: r16 与 **r23 发布版本一同捆绑发布** (不单独发行 r16 tag)。r23 发布页面会回链至本说明及 r16 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r16-aerial-perspective.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r16 — Aerial perspective (距离引发的色变)

### r16 主轴: 远景物理地坐落于空气之中

r14 让空气具备体积感、r15 让光束贯穿空间,r16 则让 **远景物理地坐落于那片空气之中** — 就像照片中远山被距离染蓝的那种感觉:

- **远山染向蓝色** (Rayleigh 散射 — 短波长散射更强,视线越长蓝味越突出)
- **近景几乎不变** (atmosFragLighting 把 atten 标量化,波长依存只能通过 additive (in-scatter) 路径生效 — 设计如此)
- **WindLight preset 取值作为 input 重新解释**,preset 兼容性保持

实现 **不重写既有 pipeline**:

- 在 `atmosphericsFuncs.glsl::calcAtmosphericVars` 引入新的 `rayleigh_w = (1.0, 2.33, 5.71)`,乘到 `combined_haze` 和 `blue_weight` (in-scatter color)
- **不** 应用到 `light_atten` (太阳光路) — 应用后近景也会变黄,演变为「常时夕烧」副作用 (aerial perspective ≠ 夕烧的物理分离)
- **`skyV.glsl` 在本 release 中完全不动**。sun disc / 地平线 / haze_glow 维持 r14 P2.a refined 的状态 — 结构上回避 r14 P2.b/c 出现的 sun disc 劣化

### 尝试过但 drop 的部分 (有意)

- **Preetham 1999 球面 sec(θ) 近似 (P1.b)**: 实现 + Linux 实机验证。数值上差异确实存在 (θ=89° 处 sec=57.3 → Preetham=26.5),但 AYA 体感「没有变得更柔和的感觉」 — SL 的 sun timeline 在地平线 ±5° 一个时间步就跨过,低于知觉阈值。依据 `feedback_feature_value_in_main_usecase.md` (动了 ≠ 效了) drop。后续章节会以「保护 sun disc」为约束条件重新挑战太阳方向光路
- **Distance Multiplier 物理系数化**: deferred。P1.a 已经能出远景青味偏移,不冒 preset 兼容破坏的风险

### Master switch + 单独 sentinel

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 章节 master switch (r14–r20)。`0` 完全回到 r14 之前的行为 |
| `AYAR16AerialPerspectiveEnabled` | `1` (ON) | r16 单独 sentinel。`0` 回到 r15 时的行为 (rayleigh_w = (1,1,1) 与旧式数学等价) |

需要单独 sentinel 是因为 master 一切则 r14/r15 效果同时消失,无法对 r16 单独做 instant A/B 体感评估。与「个别 cvar 不滥增原则」(memory `feedback_prefer_defaults_over_config.md`) 的折中:章节评估期间 sentinel 限定为 r14/r15/r16 各 1 件,A 轴完走时 (r18) 检讨并入 master。

### 已知 trade-off

- **`atmosFragLighting` 的 atten 标量化** (`light *= atten.r`): surface 直接透过的波长依存 effectively no-op。r16 看到的远景青味偏移 **完全来自 additive (in-scatter) 路径** (`blue_weight` 与 `(1 - combined_haze)`)。为避免未来开发者假定 atten 是 per-channel,已在 spec §3 与 memory `project_atmos_atten_scalarized.md` 中明记
- **近景几乎不变化** 是设计如此 — 在近距离 `density_dist` 很小、additive 本身很薄,Rayleigh weight 没有杠杆点 (aerial perspective 本质是长距离现象)

### 实现概要

- `atmosphericsFuncs.glsl` — 加入 `rayleigh_w` ternary、乘到 `combined_haze` 与 `blue_weight` (`light_atten` 故意不动)
- `LLSettingsVOSky::applyToShader` — `aya_r16_aerial_perspective_enabled` uniform plumbing
- `LLShaderMgr` enum + reserved uniform 名添加
- `settings.xml` — `AYAR16AerialPerspectiveEnabled` Boolean default 1

### 文档

- r16 spec / pipeline 依据 / drop 的 P1.b 经过 / 风险登记: `docs/ayastorm-r16-aerial-perspective.md`
- P0 atmospheric pipeline 调查: `docs/archive/r16/aerial_perspective_survey.md`
- 视觉真实感章节路线图 (r14–r20 整体): `docs/ayastorm-visual-realism-roadmap.md`
- atmosFragLighting atten scalarize 备忘: memory `project_atmos_atten_scalarized.md`
