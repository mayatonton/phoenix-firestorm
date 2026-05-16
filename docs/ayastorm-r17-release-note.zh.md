# AYAstorm r17 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r17 是视觉真实感章 (r14–r20) 的第四弹**,引入太阳 elevation 驱动的色温 modulator,使太阳接近地平线时 sun / ambient / cloud color 向 orange 方向变暖,找回单凭 preset 取值出不来的「夕烧的暖意」cinematic 感。

> **发布形态**: r17 与 **r23 发布版本一同捆绑发布** (不单独发行 r17 tag)。r23 发布页面会回链至本说明及 r17 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r17-color-temperature.md`) 中。本说明仅作为该文档的入口及差异亮点。

> **r17 经过备忘**: r17 曾在「instant A/B 看不出效果」的判定下被 drop,但 drop 后的 sustained viewing 让 **「orange 夕烧的世界丢失了」** 变得清晰可见,同日内 revert (实现恢复至 `c3d6aee734` 的完整内容)。**instant A/B 与 sustained viewing 是不同的评估轴** (memory `feedback_instant_ab_vs_sustained.md`) — 即时切换「感觉不出变化」的功能,sustained 视听中可能是 cumulative 起作用的。本 release 反映这一教训。

---

## AYAstorm r17 — 时间带色温

### r17 主轴: 找回夕烧的暖意

WindLight preset 自身带有颜色信息,但 SL 的 sun elevation 曲线缺少物理性的色温调制,即使 Sunset preset 送来 warm 数值, **太阳本身在 day cycle 中不会动态向 orange 加深**、ambient / cloud light 也不会跟随太阳方向偏向 warm tone,导致照片感的缺失。

r17 加入单一 helper `LLSettingsVOSky::getR17SunModulator(lightnorm, psky)`:

- 取太阳方向 elevation (`lightnorm.z`)
- `t = smoothstep(0, 0.4, lightnorm.z)` 做归一化
- `K = mix(2200, 6500, t)` 得到 Kelvin (warm horizon → neutral midday)
- 通过 Tanner Helland 2012 公开式将 Kelvin → RGB modulator

modulator 在 **3 个注入点** 应用:

1. **sky path** (`llsettingsvo.cpp::applySpecial`): 乘到 `SUNLIGHT_COLOR` + `CLOUD_COLOR` + ambient
2. **scene path** (`pipeline.cpp::setupHWLights`): 乘到 `mSunDiffuse` + `gGL.setAmbientLightColor` 前的 ambient
3. **cloud path B 轴** (与 r18 共享): 上述 sky path 中 modulated 的 `CLOUD_COLOR` 流入 cloud shader,体积化的云面 (r18) 也带上 warm tone

### 哪些 preset 有效 / 哪些无效 (preset 依存)

modulator 是 **物理驱动 (elevation driven)** 的,因此在太阳实际移动的场景中真正发挥作用,而在把太阳固定在 zenith 的 preset 中故意保持 subtle:

| Preset | sun elevation | K | Modulator | 有效场面 |
|---|---|---|---|---|
| Day cycle (estate time) | 0.0 → 1.0 动态 | 2200 → 6500 | strong amber → identity | **效果最大** — 向日落进行的 cumulative warm-up |
| Sunset (固定) | 0 | 2200 | strong warm | **cinematic orange 夕烧复活** (revert 的动机) |
| Sunrise (固定) | 0.996 (zenith) | 6500 | identity | no-op (preset 设计者将太阳钉在正上方) |
| Midday (固定) | 0.37 | ≈6500 | near-identity | no-op |
| 昼间 (legacy preset, `KNOWN_SKY_LEGACY_MIDDAY`) | — | — | identity (pinpoint 除外) | 保护 PBR 前 noon 还原 preset 的意图 |

「有效 preset / 无效 preset 混在」的状态按 `feedback_release_with_user_feedback.md` 流派接受,本说明仅为校准用户预期。

### 设置

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | 章节 master switch (r14–r20)。本 release **U32 化** (Firestorm View=0 / AYAstorm View=1),与 Preferences → Graphics → Shaders 新增的 `AYAViewMode` combo_box 直接 binding |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | r17 sentinel。`0` 时 modulator 变 identity (preset 颜色原样素通) |

### Master cvar 升级为 U32

本 release 将 `AYAVisualRealismEnabled` 由 **Boolean → U32** (default 仍为 1)。原因:新增的 `AYAViewMode` combo_box (`Firestorm View=0` / `AYAstorm View=1`) 与 U32 cvar 干净 binding,与 Boolean cvar 之间的 LLSD coercion 不稳定 (memory `feedback_combo_box_u32_cvar.md`)。C++ 侧 3 个读取点 (`llsettingsvo.cpp::applySpecial` ×2、`pipeline.cpp::doGodrays`) 已更新为 `LLCachedControl<U32>` + `() != 0` 判定。

### 实现概要

- `llsettingsvo.{h,cpp}` — `getR17SunModulator()` + `kelvinToRGB()` helper、3 注入点集成
- `pipeline.cpp::setupHWLights` — scene-path modulator 应用
- `settings.xml` — `AYAR17ColorTemperatureEnabled` Boolean default 1、`AYAVisualRealismEnabled` 升级为 U32
- `panel_preferences_graphics1.xml` (en/ja) — Preferences 暴露 `AYAViewMode` combo_box

### 已知限制

- **instant A/B 中 subtle,sustained viewing 中 cumulative 生效**。从 drop/revert 中获得的教训:此类效果应通过 sustained 视听评估,而非即时切换 (memory `feedback_instant_ab_vs_sustained.md`)
- **「昼间 (legacy)」preset 被 pinpoint 除外** — 不调制 PBR 前 noon 还原 preset 的意图

### 文档

- r17 spec / revert 记录 / Day cycle 有效但 Sunrise 无效的原因: `docs/ayastorm-r17-color-temperature.md`
- r18 云体积化 spec (B 轴 CLOUD_COLOR mod 的同居处): `docs/ayastorm-r18-cloud-volumetric.md`
- 视觉真实感章节路线图: `docs/ayastorm-visual-realism-roadmap.md`
- combo_box ↔ U32 cvar binding 模式: memory `feedback_combo_box_u32_cvar.md`
- instant A/B 与 sustained viewing 评估轴: memory `feedback_instant_ab_vs_sustained.md`
