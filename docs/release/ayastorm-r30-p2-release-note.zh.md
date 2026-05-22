# AYAstorm r30 P2 — 发布公告

**r30 P2 在 Cinematic 模式中引入按对象动态模糊（per-object motion blur）和 SMAA T2x** — 作为 r30（电影化渲染）章节的基础设施，本期实装速度缓冲生成路径与时序解析（temporal resolve）。

实装追踪、改造点、验收观测、上游参考行均保留在长期资料（`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`）中，本说明仅作为入口与差异要点。

---

## AYAstorm r30 P2 — 速度缓冲 + 按对象动态模糊 + SMAA T2x

### r30 P2 的主轴：搭建 Cinematic 渲染基础

r30 章节（电影化渲染）在 P1 阶段完成了三模式重启切换的 View Mode 统一，并在 UI 上先行加入 Cinematic 槽位。P2 在该 Cinematic 模式中实装 **按对象动态模糊** 与 **SMAA T2x 时序抗锯齿**。两者共享同一个 **速度缓冲**（按像素写入相对前一帧的 NDC delta，使用 `GL_RG16F` RT），仅在 Cinematic 模式启动时分配并绘制。

普通（Standard）/ 真实感（AYAstorm View）模式下完全不分配速度缓冲，附加成本为零。

### 工作原理

速度缓冲生成路径：

```
display() 流程
  → renderGeomMotionBlur()
       ├─ 将 mVelocityMap 清为 (0,0,0,1)
       └─ 对所有 pool 派发 pool.renderMotionBlur()
              ├─ Bump / Materials / PBR opaque（rigged / static）
              ├─ Tree / Terrain（face-iter）
              ├─ Alpha mask（含 alpha-discard）
              └─ Avatar（LL 原生 skin）
  → 各 pool 通过 pushVelocityBatches{,Textured} / pushRiggedVelocityBatches{,Textured}
     将当前帧与前一帧的矩阵送入 uniform，由 velocity shader 写入 NDC delta
```

后处理合成（composite）：

```
deferredScreen（光照后的最终颜色）
  → motionBlurF.glsl（沿速度方向 32-tap 三角加权模糊）
       ├─ NaN/inf 守卫（防御来自未初始化矩阵的垃圾速度）
       ├─ 噪声基线 2.0 px（抑制亚像素漂移让静物糊掉）
       ├─ 上限 2× max_blur（让 skinning 爆炸 / 跨 SIM 边界的失控速度直接 passthrough）
       └─ 每采样速度门（消除 opt-out 头像周围的光晕渗透）
  → frag_color
```

SMAA T2x 时序解析：

```
在 SMAA（空间 AA）最终 blend pass 之前
  → 向投影矩阵注入 Halton(2,3) 2-tap 亚像素抖动
  → 用速度对前一帧结果做重投影
  → 50/50 混合，对两个样本做平均（即 T2x 的 "2x"）
```

### 设置

**日常使用无需操作。** 启动 Cinematic 模式（`AYAVisualRealismEnabled = 2`）后，motion blur 与 SMAA T2x 的基础设施自动启用。调优 cvar 如下：

| Cvar | 默认值 | 用途 |
|---|---|---|
| `RenderMotionBlurStrength` | `32` | motion blur composite 的最大模糊长度（像素）。`0` 关闭 composite（速度缓冲仍会生成） |
| `RenderMotionBlurSelfAvatar` | `1` (ON) | 是否将自己的头像写入速度缓冲。OFF 时自己始终清晰（第一人称 / 自拍中只想让相机运动产生模糊时有用） |
| `RenderMotionBlurOtherAvatars` | `1` (ON) | 是否将其他头像写入速度缓冲。OFF 时他人始终清晰（团体拍摄中只想让环境产生模糊时有用） |
| `RenderSMAAT2x` | `0` (OFF) | SMAA T2x 时序解析。需要 `RenderFSAAType=2`（SMAA）+ Cinematic 模式同时成立。在边缘细节（树叶 / 头发 / 细枝）上有微小平滑改善 |
| `RenderBufferVisualization` | `-1` | 设为 `7` 时在屏幕上可视化速度缓冲（R=X、G=Y 速度）。仅限诊断 |

Boolean 类型 cvar 的值改动会在 **关闭 Debug Settings 窗口的瞬间** 提交（auto-widget 的提交时机，并非代码 bug）。切换后请先关闭一次 Debug Settings 窗口再观察行为。

### 迁移说明

- Cinematic 模式新增 per-object motion blur 与 SMAA T2x
- 普通 / 真实感模式毫无影响（速度缓冲不会分配）
- Cinematic 与其它模式之间切换 **需要重启 viewer**（r30 P1 已确定的设计，避免动态重组 velocity / SMAA RT 结构）
- 这些 cvar 使用的 key 与上游（Linden / Firestorm）的 `RenderMotionBlur` 系列完全独立，不会发生设置冲突

### 已知限制

- **classic / system avatar body 的 per-bone motion blur**：avatar pool 渲染路径不上传 `lastMatrixPalette` uniform，因此 classic body 的肢体动作不产生 per-bone velocity。composite 端的上限守卫（`speed > max_blur * 2.0` → passthrough）保证画面不会被污染。现代 SL 主流（mesh body 头像）由 rigged mesh 挂件路径独立上传矩阵，per-bone velocity 正常工作。少数情况——把 classic body 直接示人的头像在剧烈动作场景下不会出现 per-bone 模糊——将在 r30 后期阶段重新评估
- **macOS / Windows 实机验证**：AYAstorm 端 Linux 构建动作确认 PASS，Mac/Win 的 Release 二进制将在切 tag 时验证

### 实装概要

- Shader（`indra/newview/app_settings/shaders/class1/deferred/`）：
  - 9 个 velocity shader（自 Black Dragon Viewer 借用）：`avatarVelocity{F,V}.glsl`、`skinnedVelocity{V,AlphaV}.glsl`、`velocity{F,V,Alpha{F,V},FuncV}.glsl`
  - SMAA T2x resolve：`SMAAResolve{V,F}.glsl`
  - motion blur composite：`motionBlurF.glsl`（AYAstorm 侧追加 per-sample velocity gate，抑制 `RenderMotionBlur{Self,Other}Avatars` opt-out 时的光晕渗透）
- C++ pipeline（`indra/newview/`）：
  - `pipeline.{cpp,h}`：`mVelocityMap` / `mSMAAHistory` RT 分配（Cinematic 门），新增 `renderGeomMotionBlur()`、新增 `renderMotionBlurComposite()`、`renderBufferVisualization` case 7 追加
  - `lldrawpool.{cpp,h}`：`LLDrawPool::{getNumMotionBlurPasses, beginMotionBlurPass, renderMotionBlur, endMotionBlurPass}` 虚函数 + 4 个 push helper（`push{,Rigged}VelocityBatches{,Textured}`）
  - 各 drawpool subclass（Bump / Materials / PBR / Tree / Terrain / Alpha / Avatar）：velocity pass override
  - `lldrawpoolavatar.{cpp,h}`：`RenderMotionBlurSelfAvatar` / `RenderMotionBlurOtherAvatars` opt-out
  - `llspatialpartition.h`：新增 `LLDrawInfo::mAttachedToAvatar`（static prim 挂件的穿戴者绑定，与仅用于 rigged 的 `mAvatar` 独立）
  - `llvovolume.cpp`：`registerFace()` 中设置 `mAttachedToAvatar = vobj->getAvatar()`
- `indra/newview/app_settings/settings.xml`：新增 4 个 P2 cvar + 扩展 `RenderBufferVisualization=7` 说明
- 速度缓冲仅在进入 Cinematic 模式时分配 → 普通 / 真实感模式成本为零

### Credits

velocity buffer + per-object motion blur + SMAA T2x 的实装范式源自 [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer)（NiranV Dean）。AYAstorm 以 BD `995a1354d8`（2026-04-19）为上游参考点，按授权继承（LGPL-2.1-only）取入 9 个 velocity shader + composite shader + SMAA resolve shader。在 AYAstorm 侧追加了：

- Cinematic 模式门（仅 `AYAVisualRealismEnabled == 2` 时分配 RT）
- 2 个头像 opt-out cvar（Self / OtherAvatars）及写入侧 skip helper
- composite shader 的 per-sample velocity gate（消除 opt-out 周围的光晕渗透）
- 修复 BD `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` 的 T-pose 光栅化 bug（在 `current_clip` 侧同样应用 object skinning）
- composite 端垃圾速度防御（NaN/inf guard + 上限 2× max_blur）

### 文档

- r30 P2 完整追踪 / file:line 改造图 / step 1〜5e 实装 commit log / 验收观测：[`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`](../specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md)
- 父 spec（r30 章节）：[`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前一阶段（r30 P1, View Mode 重启切换）：[`docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`](../specs/ayastorm-r30-p1-view-mode-restart-switch.md)
