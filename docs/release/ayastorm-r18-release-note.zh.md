# AYAstorm r18 — 发布公告

**r18 是视觉真实感章 (r14–r20) 的第五弹,也是 A 轴 (大气) 完走 release**。通过对既有 2D `cloud_noise_texture` 在视线方向做轻量 slab raymarch 多重采样,云从「平面板」变成具有厚度与深度的立体,与 r17 的色温联动结合后形成 **cinematic 的 orange 夕烧云**。

> **发布形态**: r18 与 **r23 发布版本一同捆绑发布** (不单独发行 r18 tag)。r23 发布页面会回链至本说明及 r18 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r18-cloud-volumetric.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r18 — 云体积化 + 色温联动

### r18 主轴: 让云有深度,让夕烧云 cinematic

r14 → r17 把空气、光束、远景 haze、夕烧暖色一层层叠上去。r18 收下「**值得拍照的天空核心**」:

- **云不再是「板」** — 在既有 `cloud_noise_texture` (sampler2D) 上沿视线方向做 4 step slab raymarch,获得厚度、雕刻感的边缘、内部深度梯度
- **夕烧云变 cinematic** — 把 r17 色温 modulator (`getR17SunModulator`) 也应用到 `CLOUD_COLOR` (sustained viewing 评估后复活的 B 轴),让体积化的云面带上 warm tone
- **不附带新 asset** — 直接复用既有 2D noise texture,不需要 3D noise atlas,不需要 preset 改造

实现 **不倒向 heavy raymarch** (按章 roadmap §6,每帧全屏 ray-march 永久 drop): 每个 cloud pixel N=4 fixed slab step、Beer-Lambert 风 transmittance (45%/slab)。计算成本有界,AYA 实机评价「非常出色」且 FPS 没有显著下降。

### 内容

**A 轴 (云体积化)** — `cloudsF.glsl` slab raymarch
- 在 UV 空间 slab offset `(0.013, 0.008)`/step (视线方向 proxy) 上对既有 2D `cloud_noise_texture` 做 N=4 sample
- 逐 slab 累积 Beer-Lambert 风 transmittance
- 由 `AYAR18CloudVolumetricEnabled` + master + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外组合 gate
- **OFF 路径与 legacy flat sample 数式上完全一致** (preset 兼容性结构上得到保证)

**B 轴 (CLOUD_COLOR × r17 modulator)** — r17 色温的再应用
- `llsettingsvo.cpp::applySpecial` 中将 `psky->getCloudColor() * getR17SunModulator()` 通过 `CLOUD_COLOR` uniform push
- 原本属于 r17 → 随 r17 drop 同时 drop → sustained viewing 评估「orange 夕烧的世界丢失」后 **复活**
- 由 `AYAR17ColorTemperatureEnabled` (r17 sentinel) + master + Legacy Midday pinpoint 除外 gate

### 设置

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | 章节 master switch — 在 Preferences → Graphics → Shaders 的 `Firestorm View / AYAstorm View` combo_box 中切换 |
| `AYAR18CloudVolumetricEnabled` | `1` (ON) | r18 A 轴 sentinel。`0` 时回到 flat 2D sample (与 r17 数式一致) |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | 来自 r17 的 sentinel,本 release 中也 gate B 轴 CLOUD_COLOR mod |

### View Mode UI

Preferences → Graphics → Shaders 通过 combo_box (`AYAViewMode`) 暴露章节 master:

- **Firestorm View** — `AYAVisualRealismEnabled = 0`,完全回到 r14 之前的画面
- **AYAstorm View** — `AYAVisualRealismEnabled = 1`,视觉真实感章节 ON (默认)

(combo_box 在 r17 与 master cvar 的 U32 升级一同加入,r18 release 是它公开亮相之处。)

### 已知限制

- **A 轴效果在云量多的 preset (Cloudy / Sunset) 中最明显**。Clear-sky preset 云面积小,可供表现深度的空间有限 (定义如此)
- **昼间 (legacy) preset (`KNOWN_SKY_LEGACY_MIDDAY`) 被 pinpoint 除外** — A 轴 fallback 到 flat sample、B 轴 CLOUD_COLOR mod 为 identity。这个 preset 是 PBR 前 noon 还原意图,我们不调制它
- **地表云影超出 scope** — r18 阶段曾考虑同捆,为防止 scope 膨胀挪至 r19+
- **A 轴效果与 WindLight `cloud_pos_density` / `cloud_scale` 共存** — 这些 preset uniform 继续作为有意义的 input,raymarch 是在 preset 已定义的云量「立体面」上雕刻

### 实现概要

- `cloudsF.glsl` — N=4 slab raymarch、gated、为数学等价保留 OFF 路径
- `llsettingsvo.cpp::applySpecial` — A 轴 uniform push + B 轴 `CLOUD_COLOR × r17_sun_mod`
- `LLShaderMgr` — `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` enum + reserved uniform
- `settings.xml` — `AYAR18CloudVolumetricEnabled` Boolean default 1

### 文档

- r18 spec / slab raymarch 参数 / B 轴 revival 经过: `docs/ayastorm-r18-cloud-volumetric.md`
- P0 cloud shader 调查: `docs/archive/r18/cloud_volumetric_survey.md`
- r17 spec (drop / revert 教训、B 轴 revival 的根据): `docs/ayastorm-r17-color-temperature.md`
- 视觉真实感章节路线图 (A 轴完走 = r18): `docs/ayastorm-visual-realism-roadmap.md`
