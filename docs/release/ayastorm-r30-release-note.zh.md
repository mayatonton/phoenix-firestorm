# AYAstorm r30 — 发布公告

**r30 是摄影渲染章 (r30+) 的第 1 弹**,为 AYAstorm 新增 **Cinematic mode**,将 BlackDragon Viewer (BD) 995a1354d8 的描绘 pipeline **1:1 完全移植** 并与 AYAstorm 自己的视觉扩展并行运行。通过单一 cvar (`AYAVisualRealismEnabled`),可在 **Firestorm View**、**AYAstorm View (r14-r20 扩展)** 与 **BlackDragon Cinematic** 之间通过重启切换。

> **发布形式**: r30 独立 tag 发布 (从 β release 开始)。
>
> **重启切换模型**: mode 切换需要 viewer 重启 (Phase 4 G4 中 round-trip 行为为推荐验证项,长期稳定性待 β feedback)。

实现、移植策略、shader mount 表、受入基准都保留在 `docs/specs/` 下。本说明仅作为入口与差异亮点。

| 文档 | 内容 |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (BD 与 AY 差分: shader 49 + cpp 92 + cvar 扩展) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE 分类 + r14-r20 gating) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch 结构) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 file) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar 26 件 dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49 file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 BD UI floater + bdsidebar mount |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3-mode 受入检验 (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — BlackDragon 完全移植 + 3 mode 集成

### r30 主轴: 1 viewer / 3 mode 实现 "Firestorm 兼容"、"AYAstorm 视觉扩展"、"BlackDragon 写真摄影" 的切换

AYAstorm 在 r14-r20 中持续构筑自家的视觉真实感扩展 (atmospheric perspective / cloud volumetric / sun Kelvin modulator / SSS skin marker / translucency / chromatic aberration / SMAA T2x / volumetric godrays / depth-of-field 强化 等)。另一方面 BlackDragon Viewer 朝着写真摄影方向独立进化,AYAstorm 用户中也有 "希望直接从 AYAstorm 中呼出 BlackDragon 的摄影体验" 的需求。

r30 将 BD 的描绘 pipeline **1:1 完全移植**,在 AYAstorm 内集成 **3 mode**:

| `AYAVisualRealismEnabled` | mode 名 | 描绘 pipeline | 用途 |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY 扩展 OFF) | 直播观看 / 业务 / 节省资源 |
| `1` | AYAstorm View (默认) | AY r14-r20 视觉真实感扩展 | 常规 AYAstorm 体验 |
| `2` | Cinematic | BlackDragon 995a1354d8 pipeline 1:1 移植 | 写真摄影 / machinima |

### 工作原理: 让 3 mode 并行运行的 4 层 dispatch

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr 等 53 file 读取 `AYAVisualRealismEnabled`,按 mode 切换 shader bind / state setting / render order
2. **shader permutation (Phase 3.8)**: 49 REDO shader 用 4 strategy 进行 mount
   - **A**: uniform 喂入吸收 (5 file,不动 shader,中性值 collapse AY 分支)
   - **B**: overwrite (9 file,BD baseline 覆盖 AY,无 AY 扩展)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 file,AY 与 BD 在同一 file 共存)
   - **D**: dual-file mount (2 file,放入 `cinematic_bd/` 并由 shadermgr 探索切换)
3. **cvar 扩展 (Phase 3.3-3.6)**: 新增 BD-only cvar 7 件,同梱 BD-only sky/water/day preset 7 件,AY-only Render* cvar 26 件在 Cinematic 中固定为 BD-noop 值
4. **UI mount (Phase 3.9)**: Cinematic mode (`MachinimaSidebar=1`) 时画面右侧出现 BD Machinima Sidebar (panel_machinima 1021 行),通过 `gSideBar->refreshGraphicControls()` 双向 binding

### 设置: 常规运营操作

**常规运营保持默认即可 (mode 1 = AYAstorm View)**。仅需切到 Cinematic 时:

| Cvar | 默认值 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic,**需重启** |
| `MachinimaSidebar` | `1` | Cinematic mode 时是否显示 BD Machinima Sidebar。`0` 时 pipeline 仍是 BD 但隐藏 sidebar |
| `RenderShadowAutomaticDistance` | `1` | BD-only 自动 shadow distance 计算 (mode 2 中活动) |

### 迁移说明

- **AYAstorm View (mode 1) 与 r29 以前的描绘等同**,直接升级到 r30 不改设置外观不变。
- Cinematic mode (mode 2) 的动作验证请参考 Phase 4 受入 spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) 的 G1-G5。
- mode 切换建议 **重启** 进行。round-trip 切换 (仅改 cvar 不重启) 在 β feedback 确认稳定前请谨慎。
- Cinematic mode 内 Machinima Sidebar slider 操作可能与 AY 扩展 cvar 控件名冲突,精密调整 AY 扩展请回到 mode 1。
- BD `panel_preferences_render_settings` / `panel_preferences_ui_colors` 仅放置文件作为 **orphan**,不动 AY preferences 标签布局 ([Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) 判断,优先维持 AY 17 标签 UX)。

### 已知注意点

- mode 2 Cinematic + AY 扩展 floater (`floater_aya_cinematic.xml`) 可以并存,但画面右的 bdsidebar 可能与 UI 重叠 — 请择一使用。
- BD `llfloatereditsky` / `llfloatereditwater` 在 BD 上游自身就未 register,AY 侧也按 orphan 1:1 移植 (完全移植原则)。
- cinematic_bd/ 下的 shader 维持 GPU class 别 fallback (class3→class2→class1)。低 class GPU 自动 fallback。

### 致谢

整体参照了 BlackDragon Viewer (NiranV Dean) 的描绘 pipeline / panel_machinima UI / shader 群。bdfunctions / bdsidebar 的 `Copyright (C) 2018, NiranV Dean` header 完整保留。

---
