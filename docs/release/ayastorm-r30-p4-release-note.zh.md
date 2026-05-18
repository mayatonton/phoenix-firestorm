# AYAstorm r30 P4 — 发布公告

**r30 P4 在 Cinematic 模式中引入 BD DoF chain（高品质 DoF + 色差 + 前景虚化）** — 作为 r30（电影化渲染）章节的表现力强化，本期将 Black Dragon Viewer 的 DoF 管线整体移植，并新增专用的 Cinematic Controls floater（新设的 AYAstorm 顶部菜单 → `Cinematic Controls...` / `Alt+C`），用于拍摄中的实时调整。

实装追踪、改造点、验收观测（含 chroma 数式 hotfix）、上游参考行均保留在长期资料（`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`）中，本说明仅作为入口与差异要点。

---

## AYAstorm r30 P4 — BD DoF Chain (HQ DoF + Chromatic Aberration + Front Blur)

### r30 P4 的主轴：把摄影镜头的「虚化与色差」装进来

r30 章节（电影化渲染）在 P2 完成了 per-object motion blur + SMAA T2x，在 P3 完成了体积光照，搭起了 Cinematic 模式的渲染基础与光的物质感。P4 在其上实装 **摄影镜头的光学特性（HQ DoF + 色差 + 前景虚化）**。

色差（chromatic aberration）是玻璃镜片对 R/G/B 各波长以不同折射率折射所产生的光学现象，画面周缘处的轮廓会分裂出红色与蓝色的边缘，呈现「用老镜头拍的」质感。AYAstorm r30 的核心命题（`docs/specs/ayastorm-r30-cinematic-chapter.md`）是「值得拍照的空气与空间」，本期将镜头本身纳入这种物质感的可视层。

普通（Standard）/ 真实感（AYAstorm View）模式下 shader register 阶段不会附加 permutation，附加成本为零。

### 工作原理

通过 permutation 标志插入既有 DoF 路径作为后处理 pass：

```
deferredScreen（光照后的最终颜色）
  → gDeferredPostProgram（HQ DoF 路径）
       ├─ 由 Cinematic 模式 + RenderDepthOfFieldHighQuality 门控
       ├─ HAS_DOF_CHROMA permutation → 基于 CoF 的色差
       ├─ FRONT_BLUR permutation → 启用前景虚化路径
       └─ DEFERRED_CHROMA_STRENGTH uniform 控制运行时强度
  → gDeferredPostNoDoFProgram（NoDoF 路径）
       ├─ HAS_DOF_CHROMA permutation → radial offset 色差
       └─ 基于 vary_fragcoord 的 radial 数式（画面中心 0 → 周缘最大）
  → gDeferredPostNoDoFNoiseProgram（最终输出路径）
       └─ 同 NoDoF 路径 + film grain noise
```

色差以 per-channel texcoord offset 实装 —— R 通道沿径向「向内」采样，B 通道「向外」采样（G 不偏移）。

### 设置

**日常使用无需操作。** 启动 Cinematic 模式（`AYAVisualRealismEnabled = 2`）后 BD DoF chain 自动启用。拍摄中的实时调整请使用新设的 **AYAstorm 菜单 → Cinematic Controls...**（`Alt+C`）。调优 cvar 如下：

| Cvar | 默认值 | 用途 |
|---|---|---|
| `RenderDepthOfFieldHighQuality` | `0` (OFF) | 4× CoF 采样 + depth-gated 色差的高品质 DoF 后处理 pass。GPU 成本较高，user opt-in。仅在 Cinematic 模式有效 |
| `RenderDepthOfFieldChroma` | `1` (ON) | 色差功能的 compile 取入开关。OFF 时仅保留 vignette 路径（画面边缘轻微色差），ON 时启用每像素的、随 DoF 模糊量缩放的色差。仅在 Cinematic 模式有效 |
| `RenderChromaStrength` | `5.0` | 色差强度（0–100 区间）。`5` 为 subtle（可察觉范围），`10` 为肉眼明显的分离，`30` 为电影感的强色差，`>50` 为风格化表现。BD 默认 `0.0` 是在其 UI slider 暴露的前提下设置的；AYAstorm 通过 floater 提供同等动线，故提高默认值以避免初见时看起来「功能缺失」 |
| `RenderDepthOfFieldFront` | `1` (ON) | 允许前景虚化（焦平面更前方的物体也虚化）。完全复现 BD default。仅在 HQ DoF user opt-in 时实际生效。仅在 Cinematic 模式有效 |

### 视觉表现要点

- **色差的呈现**：画面中心保持原样，越靠近周缘轮廓越分裂为 R/B 边缘 —— 模拟老镜头 fringe 的径向数式（中心 r=0 → 周缘 r=1）
- **HQ DoF 的效用**：标准 DoF 在 forward pass 内用有限的采样数构造虚化，HQ DoF 在 post-pass 用 4× 采样重建 —— 虚化圆轮廓变得平滑，散景的 dot 状残影消失。在浅景深构图（脸部对焦、背景虚化）下差异最显著
- **前景虚化**：焦平面更前方的物体（例如自己 avatar 的手）会朝远景方向虚化。与不含前景虚化的标准 DoF 的差异，在镜头前放置道具的镜头中一目了然
- **模式**：仅限 Cinematic。Standard / AYAstorm View 完全不会附加 permutation

### 迁移说明

- Cinematic 模式新增 BD DoF chain（HQ DoF + 色差 + 前景虚化）支持
- 普通 / 真实感模式毫无影响（permutation gate）
- Cinematic 与其他模式之间切换 **需要重启 viewer**（r30 P1 已确定的设计）
- 既有的 FIRE-16728 free-aim DoF 机制（决定焦点位置）予以保留。P4 只借用「DoF 内部采样精度 + 色差 + 前景虚化」，焦点机制沿用 Firestorm 原有 —— hybrid 配置
- 新设的 AYAstorm 顶部菜单（位于 Build 与 Help 之间）将在 P5+ Cinematic 功能扩展时增加 sibling 项目
- Cinematic Controls floater 仅用于拍摄中的实时调整，刻意不出现在 Preferences 中（这些不是持久配置型设置，应归属拍摄 workflow 而非 Preferences）

### 已知限制

- **NoDoF 路径较微弱**：标准 DoF（非 HQ）的色差没有 CoF 可缩放，数式表现为温和的 vignette，画面中央保持原样。如需强色差，请同时开启 `RenderDepthOfFieldHighQuality=1`（HQ DoF）—— 这会启用 CoF 基础的色差，覆盖整个画面
- **`RenderChromaStrength` 默认值与 BD 偏离**：BD 出货为 `0.0`（效果关闭），AYAstorm 出货为 `5.0`。BD 假设用户会通过可见的 UI slider 调高值；AYAstorm 在 floater 中提供同等访问后，提高默认值以避免「功能看似不存在」的初次印象（参见 §5.7 / `feedback_match_bd_defaults_on_borrow.md`）
- **`lldrawpoolwater` water chroma 不取入**：BD `lldrawpoolwater.cpp:257` 将 chroma uniform 也 push 给 water shader，但 Firestorm 的 water 管线与 BD 偏离较大，移植存在未知 regression 风险 —— 不在 P4 范围
- **多语言**：P4 floater UI 仅英语版。日语 / 中文 lproj 翻译将在 P5+ Cinematic 系列功能扩展时合并处理
- **macOS / Windows 实机验证**：AYAstorm 端 Linux 构建动作确认 PASS，Mac/Win 的 Release 二进制将在切 tag 时验证

### 实装概要

- Shader（`indra/newview/app_settings/shaders/class1/deferred/`）：
  - `postDeferredHQDoFF.glsl`（新增，自 BD 借用并在 AYAstorm 侧改修）：HQ DoF + 基于 CoF 的色差 + 前景虚化 block
  - 既有 `postDeferredF.glsl`：在 `dofSample()` 末尾加入 `#if HAS_DOF_CHROMA` chroma block（标准 DoF 路径也搭载色差）
  - 既有 `postDeferredNoDoFF.glsl`：NoDoF 路径加入 radial offset 数式的 chroma block（验收过程将初始的 edge-gated vignette 案改写为 radial offset，参见 §9.2）
- C++ pipeline（`indra/newview/`）：
  - `pipeline.{cpp,h}`：`LLPipeline::RenderChromaStrength` static + 在 `renderDoF` / `renderFinalize` / `bindDeferredShader` 3 处推 uniform
  - `llviewershadermgr.cpp`：3 个 program（`gDeferredPostProgram` / `gDeferredPostNoDoFProgram` / `gDeferredPostNoDoFNoiseProgram`）以 Cinematic + cvar 双重 gate 附加 `HAS_DOF_CHROMA` / `FRONT_BLUR` permutation；cvar signal listener 将重启需求最小化
  - `llshadermgr.{cpp,h}`：新增 `DEFERRED_CHROMA_STRENGTH` reserved uniform enum + 字符串
- XUI / UI 配线：
  - `menu_viewer.xml`：新设 AYAstorm 顶部菜单（Build 与 Help 之间），含 `Cinematic Controls...` 项目（`Alt+C`）切换 floater
  - `floater_aya_cinematic.xml`（新增）：width 320 / height 320 / single_instance —— 克制的双 section 布局（DoF + Chromatic Aberration）
  - `llviewerfloaterreg.cpp`：floater 注册（复用 `FloaterQuickPrefs` 通用类）
- `indra/newview/app_settings/settings.xml`：新增 4 个 P4 cvar
- 仅在进入 Cinematic 模式时附加 permutation → 普通 / 真实感模式成本为零

### Credits

BD DoF chain（HQ DoF + 色差 + 前景虚化）的实装范式源自 [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer)（NiranV Dean）。AYAstorm 以 BD `995a1354d8`（2026-04-19）为上游参考点，按授权继承（LGPL-2.1-only）取入 `postDeferredHQDoFF.glsl`。在 AYAstorm 侧追加了：

- 修正 shader 文件 header 的 `@file` 标注 + 加入 provenance 注释（BD `995a1354d8`、LGPL-2.1-only）
- Cinematic 模式门（仅 `AYAVisualRealismEnabled == 2` 时附加 permutation）
- 未取入 BD 的独立 settings 文件（`settings_blackdragon.xml`），4 个 cvar 全部集成到 Firestorm 标准 `settings.xml`
- 将 NoDoF 路径的 chroma 数式从 BD 的固定 offset 方式改写为 radial per-channel offset 方式（基于 vary_fragcoord，画面中心 0 → 周缘最大）
- 将 `RenderChromaStrength` 默认值从 BD 的 `0.0` 重调为 `5.0`（验收发现 chroma_str=0 会让人觉得「功能缺失」，基于实证再次应用 `feedback_match_bd_defaults_on_borrow.md`）
- 未取入 BD UI（`panel_preferences_graphics1.xml` / `panel_machinima.xml`），改为通过 AYAstorm 独有的顶部菜单 + 专用 Cinematic Controls floater 提供动线（依照 chapter §1.2）
- 未取入 BD 与 FIRE-16728 独立的 focus 机制（`CameraFreeDoFFocus` static），保留 Firestorm 既有的 FIRE-16728 free-aim DoF
- 未取入 BD `lldrawpoolwater.cpp:257` 的 water chroma uniform push（Firestorm 的 water 管线偏离较大，深层 regression 风险）

### 文档

- r30 P4 完整追踪 / file:line 改造图 / step 1〜7 实装 commit log / 验收观测（含 chroma 数式 hotfix + UI revise 经过）：[`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`](../specs/ayastorm-r30-p4-bd-dof-chain-trace.md)
- 父 spec（r30 章节）：[`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前一阶段（r30 P3, volumetric lighting）：[`docs/release/ayastorm-r30-p3-release-note.zh.md`](ayastorm-r30-p3-release-note.zh.md)
