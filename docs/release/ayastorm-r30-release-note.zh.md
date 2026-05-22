# AYAstorm r30 — 发布公告

**r30 将 r30 章中构建的新渲染引擎作为新的 "AYAstorm View" 发布**。视图模式选择器整理为 2 项 —— `Firestorm View` 和 `AYAstorm View` —— 新引擎 (velocity buffer + SMAA T2x + Volumetric Light + BD 级 DoF chain + Motion Blur + Chromatic Aberration + 35 cvar 的 AYAstorm Controls floater) 就是新的 "AYAstorm View"。

> **发布形式**: r30 作为 r25–r30 一括发布 tag 中的一个功能出货。同捆发布的其他 release note 从 GitHub Release 页面直接链接。

实现脉络 (P1 重启切换基础设施 → P2 velocity buffer → P3 Volumetric Light → P4 BD DoF chain → P5 BD parity gate → P6 live BD cvar port → Phase 6 Controls Cleanup → r30 release picker reshuffle) 以及 migration / 残留代码设计作为历史规范保留在 `docs/specs/` 下。r30 release 决定的**单一真相来源**是 `docs/specs/ayastorm-r30-view-mode-reshuffle.md`。本说明仅作为入口与差异要点。

---

## AYAstorm r30 — View Mode picker reshuffle: Cinematic 提升为 AYAstorm View

### r30 的核心: AYAstorm View 现在成为新的渲染引擎

r14–r20 期间,AYAstorm 在 Firestorm 基线之上构建了视觉真实感扩展层 —— 在 r24 picker 中显示为 "AYAstorm View"。r30 章 (P1–P6) 中,与之并行通过另一路径移植/构建了渲染引擎本体,内部作为第 3 模式 "Cinematic" 以瞄准 BD 级摄影品质。

r30 release 中做出编辑判断:

- 新引擎达成了章目标
- 让用户记住 "Firestorm View / AYAstorm View / Cinematic" 3 个模式,对于本质上是 "引擎变好了" 这个事实而言说明过多

因此 **将新引擎作为新的 AYAstorm View 出货**。picker 整理为 2 项:

| Picker | 渲染 pipeline | 用途 |
|---|---|---|
| Firestorm View | Linden / Firestorm 基线 | 直播观看、作业、低资源运行 |
| **AYAstorm View** (默认) | 新渲染引擎 (velocity buffer / SMAA T2x / Volumetric Light / BD 级 DoF / Motion Blur / Chromatic Aberration / r14–r20 扩展为 opt-in) | 摄影、machinima、日常沉浸用途 |

### upgrade 后首次启动会发生什么

r24 之前的旧 AYAstorm View (持久化的 `AYAVisualRealismEnabled = 1`) 会在启动时由幂等的 one-shot migration 重写为 `2` (新的 AYAstorm View),并打上 `AYAViewModeMigrationVersion = 1` 标记。日志会有如下行:

```
View mode migration v0->v1: AYAVisualRealismEnabled 1 (legacy AYAstorm View) -> 2 (new AYAstorm View)
```

不会向用户弹出提示。从你的视角看,只是 "引擎变新了"。

### 模式切换需要重启

Firestorm View 和 AYAstorm View 之间的切换在 AYAstorm 重启后生效。pipeline 在启动时构建 1 次,运行中的 session 始终反映唯一的模式 (按 r30 章的设计 —— 为了避免帧中途 pipeline 重新构成导致的 bug,有意不采用运行时 gate)。

### AYAstorm Controls (Alt+C)

章中引入的 Cinematic Controls floater 现在通过 **`AYAstorm → AYAstorm Controls...`** (`Alt+C`) 打开。提供新引擎的调整 cvar —— BD live cvar (shadow / DoF / fullbright / lights / global light / post FX)、per-channel shadow tuning、将 r14–r20 的 AYA 视觉真实感扩展在新引擎之上单独 opt-in 的 6 个 cvar 等。

> **内部命名保持不变**。内部 mode index `AYAVisualRealismEnabled == 2`、`AYACinematicModeActive` helper cvar、`AYASTORM_CINEMATIC` shader `#define`、`LLCinematicOverlay` namespace、`floater_aya_cinematic.xml` 文件、`settings_cinematic_bd.xml` overlay 等保留原样。重命名对用户视角无收益,只会带来 churn-only refactor,且 commit / comment 中的 BD upstream 脉络可读性也会受损,因此 promote 仅限于 UI 上的 rename + one-shot migration。详情: `docs/specs/ayastorm-r30-view-mode-reshuffle.md` §2.2。

### 旧 AYAstorm View 怎么办?

r14–r20 的旧 AYAstorm View (mode `1`) **从 picker 中移除**,但代码路径本身保留。若出于某种原因想再次访问,在 Debug Settings 中同时回写 `AYAViewModeMigrationVersion = 0` **和** `AYAVisualRealismEnabled = 1` 并重启,即可临时一窥旧模式 —— 但下次启动 migration 会再次运行并写回 mode `2`。这是设计意图 (章的编辑立场是 "引擎被替换了",而非 "你有两个引擎可选")。

旧 AYAstorm View 的各个 r14–r20 效果 (大气透视、godray、aerial perspective、色温、cloud volumetric、translucency、avatar 皮肤 SSS) 作为 **可在新引擎之上单独 opt-in 的追加功能**,通过 AYAstorm Controls floater 到达。新的 AYAstorm View 默认 OFF 出货,需要时打开就能在新 pipeline 之上叠加 r14–r20 层。

### 设置

通常运行无需用户操作 (AYAstorm View 为默认)。

| Cvar | 默认 | 作用 |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0` = Firestorm View / `2` = AYAstorm View (新引擎)。**需要重启**。`1` 仅可通过 Debug Settings + `AYAViewModeMigrationVersion=0` 回写,下次启动 migration 会重写 |
| `AYAViewModeMigrationVersion` | `0` → migration 后 `1` | one-shot migration 的幂等 gate。除非有意让 migration 再次运行,否则不要修改 |

新引擎的参数调整集中在 **AYAstorm Controls** floater (`Alt+C`)。

### 已知限制

- **模式切换需要重启**: 有意不采用 session 内 live 切换。下次启动起反映
- **旧 AYAstorm View 不再支持**: 可通过上述 Debug Settings 路径访问,r30 release 中表面不支持 —— migration 在正常手段下有意不可逆
- **系统 body (Ruth/Roth) 不参与 motion blur**: 与新引擎基线一致 (`LLDrawPoolAvatar::renderMotionBlur` 完全注释)。现代 rigged mesh avatar 通过其他 pool 仍有 motion blur
- **macOS OpenGL deprecation 监视**: 与章整体一样,新引擎的 shader chain 在未来的 macOS toolchain 中可能需要 fallback。r30 出货时无已知 regression

### 实现概要

- picker reshuffle commit (`6a6b657441`) 变更: 10 文件 —— `settings.xml`、`llcinematicoverlay.{h,cpp}` (migration helper)、`llappviewer.cpp` (启动顺序)、`panel_preferences_graphics1.xml` (en/ja)、`menu_viewer.xml` (en)、`floater_aya_cinematic.xml` (en/ja)、`floater_about.xml` (en)
- Migration 启动顺序: `applyAYAViewModeMigrationIfNeeded()` 在 `applyCinematicOverlayIfNeeded()` **之前** 运行,确保 upgrade 用户在同一启动中也应用新引擎 overlay
- 完整脉络 (P1 → P6 + Controls Cleanup + view-mode reshuffle) 分散在 `docs/specs/` 的 r30 spec 群中

### Credits

新引擎 pipeline 大量借鉴自外部摄影向渲染引擎 (NiranV Dean 的 Black Dragon viewer,LGPL-2.1,与 viewerlgpl 兼容)。credit 在 `floater_about.xml` 中明示,port spec 的 header 中保留 BD repository commit ref。

### 相关文档

- r30 release 决定的单一真相来源: `docs/specs/ayastorm-r30-view-mode-reshuffle.md`
- 章 status block (历史记录,前指 reshuffle): `docs/specs/ayastorm-r30-cinematic-chapter.md`
- P1 重启切换基础设施 (历史记录): `docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`
- Phase 个别 spec (P2 velocity buffer / P3 Volumetric Light / P4 DoF chain / P5 BD parity / P6 live cvar port / Cinematic Controls Cleanup): `docs/specs/ayastorm-r30-*.md`
- BD live cvar port reference: `docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`
- Cinematic Controls floater audit: `docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`
