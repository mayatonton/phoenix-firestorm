# AYAstorm r20 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r20 是视觉真实感章 (r14–r20) 的第七弹、B 轴 (物质色) 完走 release**。把 screen-space SSS (subsurface scattering) 应用到 **avatar 皮肤**,自己和他人都能呈现柔和、自内发光的皮肤质感。**gbuffer3 `.a` 的 per-pixel skin mask**、**世界坐标尺度 blur** 自动随距离淡出、**右键学习** 瞬时新增 mesh body/head (无需输入 UUID)。

> **发布形态**: r20 与 **r23 发布版本一同捆绑发布** (不单独发行 r20 tag)。r23 发布页面会回链至本说明及 r20 spec 文档。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/specs/spec_avatar_skin_sss.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r20 — Avatar 皮肤 SSS (subsurface scattering)

### r20 主轴: 皮肤变得「活着」

A 轴 (r14–r18) 让空气可信,r19 让薄物透过太阳光。r20 接下 **皮肤本身** — 拍「人」时的核心:

- **皮肤在合适的尺度上柔化** — separable 5-tap SSS blur + 波长依存权重 (红色比绿/蓝扩散更远)
- **皮肤微微发亮** — glow restore (`pow(lit, 3) × glow_gain × warm_salmon`) 把 blur 弄钝的 hi-light 找回,warm salmon tint 提供「皮下血色」感而非「发光瓷器」
- **对自己和他人都生效** — 识别用 **mesh asset UUID**,通过既有 `ObjectUpdate` 在 viewer 本地获取,sim 往返为零,无需着装者配合

SL/OpenSim viewer 史上首个 viewer 侧 avatar 皮肤 SSS。主流 Mesh body/head 产品 (Maitreya、Legacy、Reborn、eBody、LeLutka Evolution heads、Genus 等) 使用稳定的 mesh UUID,只要记住几个,普通 SL 场景 8–9 成已覆盖。

### 工作原理

**识别 — mesh asset UUID** (单一轴):

在每个 avatar 的 attachment 上本地读 `getVolume()->getParams().getSculptID()`。如果 UUID 在用户的 whitelist cvar 中,则把该 attachment 标记 `mIsSSSTarget = true`。无 sim 往返、无需编辑 Description、不依赖 inventory item 名 (对他人无效)。

**右键学习 UX**:

右键 attachment → **「Add to SSS whitelist」** → mesh UUID 追加到 whitelist cvar → cvar 变更信号触发全 avatar 重新评估 → 使用同 body/head 的 **其他人也瞬时跟着发光**。用户永远看不到 UUID。

flat 上下文菜单 (`menu_attachment_self/other.xml`) 与饼菜单 (`More >` 下) × 自己 / 他人 4 处全注册。

**Per-pixel skin mask** (Phase C):

`gbuffer3` 由 `RGB16F` 扩展为 `RGBA16F`。`.a` 通道携带 skin bit。SSS pass 逐 pixel (经 `emissiveRect`) 读 bit,仅在皮肤 pixel 上 blur — 衣服 / 头发 / 眼镜 / 眼睛不受影响。

**世界坐标尺度 blur** (Jimenez "Separable SSS"、Phase D):

```
r_eff = aya_blur_radius / max(eye_dist_m, 1m)
```

- 1m 以内半径以 `aya_blur_radius` 为上限 (维持近景 SSS)
- 超过 1m 后半径与距离成反比衰减
- 约 10m 处半径 < 1px,blur 结构性退化为 no-op

**这替代了此前的 smoothstep 距离 fade (2 个 cvar)**,smoothstep 在中距离持续出现「模糊之模糊」问题。世界坐标尺度公式让距离 fade 逻辑自动消除。

**Glow restore** (Phase D1):

```
glow_additive = pow(blurred_lit, 3) × glow_gain × glow_color
```

在 blurred 结果上做 additive 单 pass,无附加 RT。default 的 warm salmon (1.0, 0.65, 0.5) 是为「皮下血色」而非「皮肤发光」选取。

### 设置 (Preferences → Graphics → SSS 标签)

| 键 | 默认 | 用途 |
|---|---|---|
| `AYAR20AvatarSkinSSSEnabled` | `1` (ON) | SSS master switch |
| `AYAR20AvatarSkinSSSBlurRadius` | `1.0` | eye_dist=1m 基准 pixel 半径 (之后为世界坐标尺度) |
| `AYAR20AvatarSkinSSSStrength` | `0.7` | Blur strength |
| `AYAR20AvatarSkinSSSGlowGain` | `3.0` (max 5.0) | Glow restore 强度 |
| `AYAR20AvatarSkinSSSGlowColor` | warm salmon (1.0, 0.65, 0.5, 1.0) | Glow 染色 |
| `AYAR20AvatarSkinSSSWhitelist` | — (用户自维护) | 换行分隔的 mesh UUID 列表 |

UI 还包含每个 cvar 的 **Default** 按钮、**Reset all to defaults** 按钮,以及把 whitelist text editor 设为只读的 **Lock** 复选框 (防止误编辑)。

### 已知限制

- **不附带种子 UUID list** — 首次启动 whitelist 为空,通过右键学习成长。主流 body/head UUID 的种子同捆与 community list 维护设计 (§6.1) 一起在 r21+ 讨论
- **无 Transmittance** — 耳 / 指尖的真透过光 (rim glow 物理) 属 r21+,r20 只覆盖 diffuse 侧 SSS
- **无 skin tone 区分参数** — 单一全局 blur / strength / glow 值,人种 / 肤色调节延后到 r21+
- **非 mesh 的 attachment 菜单 grey out** — 旧 sculpt prim 与基本 prim 无 mesh UUID 可提取
- **Pre-PBR / 旧发型 shader** 在非 PBR 变体中对 `.a` 的写入方式可能不同,SSS pass 会以「此处无 skin pixel」安全 fallback (仅不 blur)

### 实现概要

- `llayaskinsss.{h,cpp}` (新增) — `SkinSSSMatcher` singleton、mesh UUID 抽取、whitelist 解析、全 avatar 重新评估
- `class1/deferred/skinSSSV.glsl` + `skinSSSF.glsl` (新增) — 2-pass separable SSS blur、波长依存权重、世界坐标尺度半径、`emissiveRect.a` 经由的 skin mask
- `pipeline.cpp` — `doSkinSSS()` 2-pass driver、gbuffer3 扩展为 RGBA16F
- `llvoavatar.cpp` — `attachObject` / `detachObject` 接入 `SkinSSSMatcher`
- `llviewermenu.cpp` — `SSS.Add` / `SSS.Remove` / `SSS.EnableAdd` / `SSS.EnableRemove` 处理器
- `panel_preferences_sss.xml` (新增) — Preferences → Graphics → SSS 标签
- `menu_attachment_self/other.xml` + `menu_pie_attachment_self/other.xml` — 右键菜单项
- `settings.xml` — 6 个 SSS cvar (Enabled / BlurRadius / Strength / GlowGain / GlowColor / Whitelist)
- shader cache tag 上升 (`AYASTORM_SHADER_CACHE_TAG = "AYAstorm r20"`),让旧 compiled shader 自动失效

### 文档

- r20 完整 spec (Phase A–E 状态 / 识别子策略比较 / debug settings 回退指南): `docs/specs/spec_avatar_skin_sss.md`
- gbuffer3 storage 扩展参考 (RGBA16F skin bit): memory `reference_gbuffer3_storage.md`
- deferred shader routing 参考 (writer → SSS mask path): `docs/ayastorm-deferred-shader-routing.md`
- 视觉真实感章节路线图 (B 轴完走 = r20): `docs/ayastorm-visual-realism-roadmap.md`
- sustained 验证后 debug settings 回退备忘: memory `feedback_restore_debug_settings.md`
