# AYAstorm r21 — 发布公告

**r21 是位于视觉真实感章 (r14-r20) 之外的 UX 修复型单功能 release**，把自身 rigged attachment 的右键 picker 由「上游 CPU bind-pose mesh ray」改写为「GPU 专用 object-ID buffer」并以一次发布交付。

实现 / 已知限制 / 设置细节常驻于永久规格 (`docs/ayastorm-r21-self-rigged-picker.md`)。本公告仅做链接 + 差分要点。

---

## AYAstorm r21 — GPU self-rigged picker

### r21 核心: 右键自身 rigged attachment 时「选中的就是画面上看见的那一个」

r20 之前，右键自身的 rigged attachment (Mesh body / 服装 / 头发) 时走的是 Firestorm / Linden 上游 picker，求 world ray 与 **CPU bind-pose mesh** 的交点。这在结构上产生三类症状:

- **近距特写** 想点衣服 → 解析为自身 avatar 主体而不是衣服
- **alpha 抠图头发** 挡在脸前面 → ray 穿过被丢弃的三角形，把头发选中
- **idle 动画** 让 rig 微动的 frame → CPU bind-pose 与 GPU skinning 偏差约 4-5 cm，ray 完全偏离

r21 把自身 attachment 的 picker 切出到 **专用 GPU object-ID buffer** (`mObjectIDBuffer`) 中，使用 **与可见画面相同的 skinning 矩阵** 重新渲染 `gAgentAvatarp` 的 rigged attachment，将各 prim 的 LocalID 打包到四个 8-bit 通道。右键时读回鼠标像素处的 byte quad、还原 LocalID 并在 attachment 树中解析。CPU/GPU drift、alpha-discard 不一致、idle skin lag 在结构上 **不可能发生** — picker 看到的就是屏幕显示的内容。

详情 → spec `docs/ayastorm-r21-self-rigged-picker.md`

### 设置

| 键 | 默认 | 用途 |
|---|---|---|
| `FSSelfRiggedPickerEnable` | `1` | picker 的主开关。`0` 时完全回到 r20 之前的上游行为 |
| `FSSelfRiggedPickerGPU` | `1` | GPU buffer pass 的 kill-switch。`0` = picker 无效 (上游 worldray 不加修改直接使用)。为 Mac software OpenGL 等 GPU pass 不稳定环境提供逃生通道 |
| `FSSelfRiggedPickerArmedMode` (实验性) | `1` | 仅在自身 avatar / 自身 attachment 被 hover 时运行 GPU ID pass 的实验开关。`0` 时回到常时绘制 |
| `FSSelfRiggedPickerArmSeconds` (实验性) | `3.0` | 最后一次 hover 之后允许 ID pass 继续运行的秒数 (调短可在解除 hover 后更快停止；调长可减少右键 ready 等待，但浪费的绘制时间相应增加) |

> **不提供 CPU fallback。** 要么 GPU pass 启用 (`GPU=1`)、要么 picker 整体禁用 (`GPU=0`，即 r20 之前的行为)。这是有意的设计选择 — 见 memory `feedback_root_cause_not_dump.md` (不留半工作的 workaround) 与 `feedback_feature_value_in_main_usecase.md` (按主流用例判定功能价值)。

### r21 中追加并入的修复

- **解决 `Couldn't find object ... selected.` 警告** (selection handoff fix): 旧结构在每次右键时会先后向 sim 发送「上游 worldray 的过期临时 selection」和「GPU picker 修正后的 selection」两次。先发送的那次的 `ObjectProperties` 响应返回时，selection 已经被替换，因而 `LLSelectMgr` 大量产生该警告 (验证 session 中观测到 887 条)。现已修改为只在 AYA GPU picker block 修正 `mPick` 之后调用一次 `LLToolSelect::handleObjectSelection()`。详情: `docs/ayastorm-r21-selection-handoff-investigation.md`。
- **armed mode (实验性)**: 一个可选 gate，仅在自身 avatar / 自身 attachment 被 hover 时运行原本常时进行的 GPU ID pass。非 hover 时几乎为零开销；hover 中开销与常时绘制相同。评估日志 (光标移出 20 秒追踪 / mouselook 中行为 / 连续 hover 观测) 与调整候选: `docs/ayastorm-r21-picker-armed-mode.md`。

### 已知限制

- **alpha-blend 头发挡在脸前**: GPU buffer 与场景共享深度，alpha-discard 三角形不会触达 picker，但真正以 alpha-blend 绘制而不写深度的头发仍会被「穿过」。AYA 已评估并接受 (2026-05-14)。
- **HUD attachment**: 跳过 picker (HUD 属于另一个 screen-space camera，不出现在 `mObjectIDBuffer` 内)。
- **非 rigged 自身 attachment** (耳钉 / 单独的 jewelry prim 等): GPU buffer 仅覆盖 `PASS_*_RIGGED`，非 rigged 部分会回退到上游 worldray 结果。`lltoolpie` 通过 `isRiggedMesh()` 守卫透明处理。
- **他人 avatar 的 rigged attachment**: r21 范围外 (仅 self)。可能在 r22+ 中考虑。

### 文档

- r21 spec / 架构 / 已知 limits / 风险登记: `docs/ayastorm-r21-self-rigged-picker.md`
- selection handoff fix 的调查记录 / 对比验证日志: `docs/ayastorm-r21-selection-handoff-investigation.md`
- armed mode (实验性) 的负载估算 / 实测日志 / 改善候选: `docs/ayastorm-r21-picker-armed-mode.md`
- BoM body 的 rig hash 冲突缘由 (M4.17): memory `project_skin_hash_collision_bom_body.md`
- deferred shader routing 参考: `docs/ayastorm-deferred-shader-routing.md`
