# AYAstorm r30 P3 — 发布公告

**r30 P3 在 Cinematic 模式中引入体积光照（volumetric lighting / 神光 / 薄明光线）** — 作为 r30（电影化渲染）章节的表现力强化，本期实装太阳光从建筑边缘或树枝穿过时的放射状光束与空气中的雾感散射。

实装追踪、改造点、验收观测（含 4 阶段 shader canary bisect）、上游参考行均保留在长期资料（`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`）中，本说明仅作为入口与差异要点。

---

## AYAstorm r30 P3 — Volumetric Lighting (Godrays)

### r30 P3 的主轴：让阳光在空气里散射

r30 章节（电影化渲染）在 P2 完成了 per-object motion blur 与 SMAA T2x 的 Cinematic 模式渲染基础。P3 在其上实装 **体积光照（godrays）**。

Godrays（神光 / 薄明光线 / crepuscular rays）指阳光被建筑屋顶、树枝、百叶窗等部分遮挡时，未被遮挡的光线在空气中的尘埃和湿气上散射形成的放射状光束。AYAstorm r30 的核心命题（`docs/specs/ayastorm-r30-cinematic-chapter.md`）是「值得拍照的空气与空间」，本期将光本身纳入这种物质感的可视层。

普通（Standard）/ 真实感（AYAstorm View）模式下 hook 根本不会派发，附加成本为零。

### 工作原理

作为后处理 pass 插入 `renderFinalize()` 中，位于 `generateGlow()` 与 `combineGlow()` 之间：

```
deferredScreen（光照后的最终颜色）
  → renderVolumetric(src, dst)
       ├─ 由 Cinematic 模式 + RenderVolumetricLighting 双重门控
       ├─ volumetricLightF.glsl 沿太阳方向做 shadow march（默认 16 采样）
       ├─ 深度加权（pow(depth, 100)）只让天空像素参与
       ├─ 通过 haze_weight + sunlight_color 合成为大气散射
       └─ GODRAYS_FADE permutation（可关闭）只在太阳位于摄像机前方时保留 shaftify
  → ping-pong swap → 交给 combineGlow
```

阴影采样复用 AYAstorm/Firestorm 标准的 `sampleDirectionalShadow`（来自 `shadowUtil.glsl`），用 `sun_dir` 作为替代法线让 PCF bias 仍然有效（空中采样点没有真实表面法线，与 `class1/deferred/godraysF.glsl` 的做法一致）。

### 设置

**日常使用无需操作。** 启动 Cinematic 模式（`AYAVisualRealismEnabled = 2`）后体积光照自动启用。调优 cvar 如下：

| Cvar | 默认值 | 用途 |
|---|---|---|
| `RenderVolumetricLighting` | `1` (ON) | godrays composite 的总开关。仅在 Cinematic 模式有效 |
| `RenderVolumetricLightingResolution` | `16` | 沿太阳方向的 shadow march 采样数。越高越平滑，GPU 成本线性增加 |
| `RenderVolumetricLightingMultiplier` | `50.0` | 光束强度。`50` 为「接近现实」的自然薄明光线；`100` 为明显主张的光束；`200` 偏向 PV / 电影感。BD 默认 `1.0` 在 AYAstorm 的 ACES tone mapping + HDR scene buffer 下几乎不可见（参见 spec §8.2 验收观测） |
| `RenderVolumetricLightingFalloffMultiplier` | `1.0` | 距离衰减强度。值越大 godrays 在远景越快淡出 |
| `RenderVolumetricLightingDirectional` | `1` (ON) | 仅当太阳位于摄像机前方时保留 shaftify 的门（GODRAYS_FADE permutation）。关闭后即使太阳在画面外 godrays 也会铺满全屏（视觉表现：光束悬于半空，建筑剪影被光束吞没，不自然）。切换此 cvar **需要重启 viewer**（permutation 变更） |

### 视觉表现要点

- **时间段**：白天到傍晚前（太阳较高时）最容易看到。SL 的 sunlight clamp 使傍晚的 `sunlight_color` 变暗，godrays 也随之变弱（BD 物理近似的限制）
- **构图**：把太阳放在画面接近中央，让建筑边缘 / 树枝 / 细枝等剪影位于太阳前方。能部分遮挡太阳的边缘产生的光束最强
- **模式**：仅限 Cinematic。Standard / AYAstorm View 完全不执行此路径

### 迁移说明

- Cinematic 模式新增体积光照支持
- 普通 / 真实感模式毫无影响（hook 不派发）
- Cinematic 与其它模式之间切换 **需要重启 viewer**（r30 P1 已确定的设计）
- `RenderVolumetricLightingDirectional` 切换也 **需要重启 viewer**（shader permutation 变更）
- 使用与现有 Linden / Firestorm 标准实现（`gDeferredGodraysProgram` 等既有 godrays）独立的 shader 程序（`gVolumetricLightProgram`），不会与上游发生设置冲突

### 已知限制

- **默认 Multiplier 与 BD 偏离**：BD 默认 `1.0` 即可见，AYAstorm 需要 `50.0` 才能达到同等视觉范围。推测原因：AYAstorm 的 ACES tone mapping + HDR scene buffer 比 BD 的 sRGB 直写路径更强地压缩了加法贡献。BD 端 `1.0` 时的真实显示行为尚未在实机验证
- **GODRAYS_FADE 窗口窄**：默认（`Directional=1`）下太阳必须在屏幕中心约 30° 之内 shaftify 才会保留。如需太阳在画面外也出 godrays，需关闭 `Directional` 并重启
- **仅天空像素参与**：`depth *= pow(depth, 100.0)` 的加权将地面与近景的 depth 压到 0，godrays 主要出现在天空像素（含大气）上。地面接收并延展光束阴影的效果不会产生（可在后续阶段考虑）
- **macOS / Windows 实机验证**：AYAstorm 端 Linux 构建动作确认 PASS，Mac/Win 的 Release 二进制将在切 tag 时验证

### 实装概要

- Shader（`indra/newview/app_settings/shaders/class3/deferred/`）：
  - `volumetricLightF.glsl`（自 Black Dragon Viewer 借用；AYAstorm 端将 shadow helper 替换为 `sampleDirectionalShadow` 并加入 `HAS_SUN_SHADOW` permutation gate）
  - vertex shader 复用现有的 `deferred/postDeferredNoTCV.glsl`
- C++ pipeline（`indra/newview/`）：
  - `pipeline.{cpp,h}`：新增 `renderVolumetric()`（Cinematic 门 + ping-pong）、`renderFinalize()` 内 hook
  - `llviewershadermgr.{cpp,h}`：`gVolumetricLightProgram` extern + register + `mShaderList.push_back()` 让大气 uniform 自动绑定
  - `lldrawpoolalpha.cpp`：forward pass alpha 的 depth-write gate 扩展为 Cinematic + `RenderVolumetricLighting` 也启用（让 godrays 获得正确的深度）
  - `llshadermgr.{cpp,h}`：新增 `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` uniform 字符串
- `indra/newview/app_settings/settings.xml`：新增 5 个 P3 cvar
- 仅在进入 Cinematic 模式时派发 hook → 普通 / 真实感模式成本为零

### Credits

体积光照（godrays）的实装范式源自 [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer)（NiranV Dean）。AYAstorm 以 BD `995a1354d8`（2026-04-19）为上游参考点，按授权继承（LGPL-2.1-only）取入 `volumetricLightF.glsl`。在 AYAstorm 侧追加了：

- 将 BD-only 的 `nonpcfShadowAtPos` 阴影 helper 替换为 AYAstorm/Firestorm 标准 `sampleDirectionalShadow`（`shadowUtil.glsl`，以 `sun_dir` 作为替代法线）
- `HAS_SUN_SHADOW` permutation gate 使整个 godrays 计算仅在 `RenderShadowDetail > 0` 时执行（阳光阴影关闭时自动 passthrough）
- 将 `gVolumetricLightProgram` 注册到 `mShaderList`，让大气 uniform（`sunlight_color` / `sun_dir` / `blue_density` / `haze_density`）自动绑定
- Cinematic 模式门（仅 `AYAVisualRealismEnabled == 2` 时派发 hook）
- `mPostPingMap` / `mPostPongMap` ping-pong 以保证 GPU read-after-write 安全
- 将 `RenderVolumetricLightingMultiplier` 默认值从 BD 的 `1.0` 重调为 `50.0`（通过 4 阶段 shader canary bisect + AYA 主观评价确定，详见 §8.2）

### 文档

- r30 P3 完整追踪 / file:line 改造图 / step 1〜6 实装 commit log / 验收观测（含 4 阶段 canary bisect）：[`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`](../specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md)
- 父 spec（r30 章节）：[`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前一阶段（r30 P2, velocity buffer + motion blur + SMAA T2x）：[`docs/release/ayastorm-r30-p2-release-note.zh.md`](ayastorm-r30-p2-release-note.zh.md)
