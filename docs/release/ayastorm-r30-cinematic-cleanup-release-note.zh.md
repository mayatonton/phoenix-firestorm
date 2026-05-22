# AYAstorm r30 Cinematic Cleanup — 发布公告

**r30 Cinematic Cleanup 是对 Phase 6 (live cvar 移植) 后 Cinematic Controls 浮窗的打磨** — 端到端审计全部 35 cvar / 9 标签页，恢复在代码中未被 dispatch 的项目，移除无法产生可见效果的项目。

完整审计与逐 cvar trace 保存在常驻文档 (`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`)。本 note 是入口与迁移摘要。

---

## AYAstorm r30 Cinematic Cleanup

### Headline: 浮窗的每个滑块都真正生效

Phase 6 在 Cinematic Controls 浮窗上线 13 个 live cvar 后，代码 trace 审计发现 **浮窗上能操作的 5 项实际上什么都没发生** — 或者 cvar 取值范围超出 `pipeline.cpp` 实际 dispatch，或者 cvar 被另一条代码路径完全绕过。Cleanup 解决这 5 项。

### 变更点

| ID | 控件 | 处置 | 效果 |
|---|---|---|---|
| A.1 | Shadow Detail 滑块最大值 | **0–3 → 0–2 缩小** | level 3 是空操作 (LL/BD 中 spot 光与 projector 光共享同一个 `mSpotShadow[]` 数组，level 2 已经启用两者)。tooltip 已更正 |
| A.2 | `RenderFSAAType = 3` | **现在会 dispatch SMAA + T2x** | 之前是空操作 (仅有 1=FXAA / 2=SMAA 分支)。FSAAType 现成为 AA 的单一选择器: 0=关闭, 1=FXAA, 2=SMAA, 3=SMAA+T2x |
| A.3 | Cinematic 模式下的 `RenderShadowResolutionScale` | **现会乘算到 per-cascade Vector4 上** | 在 Cinematic per-channel shadow 路径中被完全忽略。Scale 现乘算到 `RenderShadowResolution` 与 `RenderProjectorShadowResolution` 两者，并设 64px 下限以防止 0 大小分配 |
| A.4 | General 标签的「Deferred Rendering」复选框 | **移除** | Cinematic 模式按设计只支持 deferred (从未维护 forward 路径)。Header 文字明确写出 deferred 必需。cvar 自身不变，非 Cinematic 用途仍可通过 偏好设置→图形→Advanced Lighting Model 访问 |
| A.5 | `RenderSMAAT2x` 复选框 + cvar | **移除** | 功能被 A.2 (`FSAAType = 3`) 吸收。单一 enum 比并列两个开关 UX 更清晰 |

### 迁移提示

- **`RenderSMAAT2x` 已删除。** 若之前在 debug settings 设为 `1`，请改用 `RenderFSAAType = 3` 获得同等效果。旧 cvar 值会被静默忽略 — `pipeline.cpp` 中的 dispatch 在 Cinematic 模式下原本就因 BD parity 原因空转，所以用户视角的实际影响为零
- **`RenderDeferred` 复选框已搬走。** 请改用 偏好设置→图形→Advanced Lighting Model。Cinematic 模式必须开启
- **`RenderShadowDetail = 3` 不再能从浮窗选择。** debug settings 仍可设置，但 dispatch 上与 `= 2` 等价 (无行为变化)

### 文档

- 完整 cleanup spec + 逐 cvar 审计: [`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`](../specs/ayastorm-r30-cinematic-controls-cleanup.md)
- 母 spec (r30 章节): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Phase 6 (本 cleanup 所修正的 live cvar 移植): [`docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`](../specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
