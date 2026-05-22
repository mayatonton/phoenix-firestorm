# AYAstorm r30 — 发布公告

**r30 是摄影渲染章 (r30+) 的第 1 弹**,为 AYAstorm 新增 **Cinematic mode**,将新引擎描绘 995a1354d8 **1:1 完全移植** 并与 AYAstorm 自己的视觉扩展并行运行。通过单一 cvar (`AYAVisualRealismEnabled`),可在 **Firestorm View**、**AYAstorm View (r14-r20 扩展)** 与 **Cinematic** 之间通过重启切换。

> **发布形式**: r30 独立 tag 发布 (从 β release 开始)。
>
> **重启切换模型**: mode 切换需要 viewer 重启 (Phase 4 G4 中 round-trip 行为为推荐验证项,长期稳定性待 β feedback)。

实现、移植策略、shader mount 表、受入基准都保留在 `docs/specs/` 下。本说明仅作为入口与差异亮点。

| 文档 | 内容 |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (新引擎描绘与 AY 差分: shader 49 + cpp 92 + cvar 扩展) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE 分类 + r14-r20 gating) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch 结构) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 file) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar 26 件 dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49 file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 UI mount (新引擎描绘 env floater + Cinematic Controls 集成,sidebar 路线已撤回 — §0 记录) |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3-mode 受入检验 (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — 新引擎描绘完全移植 + 3 mode 集成

### r30 主轴: 1 viewer / 3 mode 实现 "Firestorm 兼容"、"AYAstorm 视觉扩展"、"新引擎描绘 写真摄影" 的切换

AYAstorm 在 r14-r20 中持续构筑自家的视觉真实感扩展 (atmospheric perspective / cloud volumetric / sun Kelvin modulator / SSS skin marker / translucency / chromatic aberration / SMAA T2x / volumetric godrays / depth-of-field 强化 等)。另一方面,一套面向写真摄影的描绘引擎在另一系统中独立进化,AYAstorm 用户中也有 "希望直接从 AYAstorm 中呼出该摄影体验" 的需求。

r30 将该 **新引擎描绘 1:1 完全移植**,在 AYAstorm 内集成 **3 mode**:

| `AYAVisualRealismEnabled` | mode 名 | 描绘 pipeline | 用途 |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY 扩展 OFF) | 直播观看 / 业务 / 节省资源 |
| `1` | AYAstorm View | AY r14-r20 视觉真实感扩展 | r29 以前的 AYAstorm 体验 |
| `2` | Cinematic (默认) | 新引擎描绘 995a1354d8 pipeline 1:1 移植 | 写真摄影 / machinima |

### 工作原理: 让 3 mode 并行运行的 4 层 dispatch

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr 等 53 file 读取 `AYAVisualRealismEnabled`,按 mode 切换 shader bind / state setting / render order
2. **shader permutation (Phase 3.8)**: 49 REDO shader 用 4 strategy 进行 mount
   - **A**: uniform 喂入吸收 (5 file,不动 shader,中性值 collapse AY 分支)
   - **B**: overwrite (9 file,新引擎描绘 baseline 覆盖 AY,无 AY 扩展)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 file,AY 与新引擎描绘在同一 file 共存)
   - **D**: dual-file mount (2 file,放入 `cinematic_bd/` 并由 shadermgr 探索切换)
3. **cvar 扩展 (Phase 3.3-3.6)**: 新增 新引擎描绘专用 cvar 7 件,同梱 专用 sky/water/day preset 7 件,AY-only Render* cvar 26 件在 Cinematic 中固定为与新引擎描绘一致的 noop 值
4. **UI mount (Phase 3.9)**: Cinematic mode 的操作集中在 **`Avatar → Cinematic Controls...`(`Alt+C`)→ `floater_aya_cinematic.xml`**。最初计划移植新引擎描绘侧的 `panel_machinima.xml` (1021 行) + 专用 sidebar,但为了与 AYAstorm 已有 floater 体系保持一致,改为集成到 Cinematic Controls floater 中,sidebar 路线已撤回 (详见 [Phase 3.9 spec §0](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md))

### 设置: 常规运营操作

**常规运营保持默认即可 (mode 2 = Cinematic)**。仅在需要切回 AYAstorm View / Firestorm View 时:

| Cvar | 默认值 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic,**需重启** |
| `RenderShadowAutomaticDistance` | `1` | 新引擎描绘专用 自动 shadow distance 计算 (mode 2 中活动) |

Cinematic mode 的各项参数调整请通过 `Avatar → Cinematic Controls...`(`Alt+C`)打开的 Cinematic Controls floater 操作。

### 迁移说明

- **r30 默认变更为 Cinematic (mode 2)**。如需保留 r29 以前的描绘 (= AYAstorm View),请在 Debug Settings 中将 `AYAVisualRealismEnabled` 改回 `1` 并重启。
- Cinematic mode (mode 2) 的动作验证请参考 Phase 4 受入 spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) 的 G1-G5。
- mode 切换建议 **重启** 进行。round-trip 切换 (仅改 cvar 不重启) 在 β feedback 确认稳定前请谨慎。
- Cinematic mode 内 Machinima Sidebar slider 操作可能与 AY 扩展 cvar 控件名冲突,精密调整 AY 扩展请回到 mode 1。
- 新引擎描绘侧的 `panel_preferences_render_settings` / `panel_preferences_ui_colors` 仅放置文件作为 **orphan**,不动 AY preferences 标签布局 ([Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) 判断,优先维持 AY 17 标签 UX)。

### 已知注意点

- `llfloatereditsky` / `llfloatereditwater` 在新引擎描绘上游自身就未 register,AY 侧也按 orphan 1:1 移植 (完全移植原则)。
- cinematic_bd/ 下的 shader 维持 GPU class 别 fallback (class3→class2→class1)。低 class GPU 自动 fallback。
- Cinematic mode 中 classic / system avatar body (素体・Ruth/Roth・传统 system 服装的素体部分) **不在 motion blur 范围内**。这与新引擎描绘 baseline (995a1354d8 中 `LLDrawPoolAvatar::renderMotionBlur` 整个函数被 `/* ... */` commented out) 保持一致,现代 rigged mesh 头像 (手、头发、服装等大部分 attachments) 仍通过其他 pool 进入 motion blur 范围。

### 致谢

整体参照了一套外部的写真摄影向描绘引擎实现 (NiranV Dean 氏作)。`bdfunctions` 的 `Copyright (C) 2018, NiranV Dean` header 完整保留。

---
