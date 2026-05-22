# AYAstorm r28 — 发布公告

**r28 将 r21 引入的 self rigged picker 扩展到其他用户的 avatar** — 右键点击其他 avatar 的 rigged attachment 时，也通过 r21 相同的 GPU object-ID buffer 解析屏幕上实际可见的 attachment。同时 armed 上限为 1 人，控制负载。

实现细节、负载控制和验证日志保存在永久文档 (`docs/specs/ayastorm-r28-other-rigged-picker.md`)。本说明仅作为入口与差异要点。

---

## AYAstorm r28 — Other rigged picker

### r28 的核心：右键点击他人的 attachment 也能选中"屏幕上看到的"

r21 让 self picker 通过 GPU object-ID buffer 使用 **与可见场景相同的 skinning matrix**，从而保证"选中的就是看到的"。r28 把这一性质引入其他 avatar — 右键点击他人 avatar 的 rigged attachment 时，仅将该 avatar 一人的 rigged draw info 重新绘制到 `mObjectIDBuffer`，并从鼠标像素读出 LocalID。

负载控制有意设计得严格：

- 同时 armed 始终为 **1 人** (光标下方，或最近右键点击的 avatar)
- GPU ID pass 仅在短 armed window 内有效 (`FSOtherRiggedPickerArmSeconds`，默认 `1.0` 秒)
- 标准第三人称视角下 hover **不会** arm — 需要 alt-cam / orbit / zoom 等非默认视角
- mouselook / avatar customize 模式中跳过整个 picker
- 单帧 draw call / triangle budget (512 / 1,200,000，hardcoded) 超限时，buffer 失效，回退到既有 worldray pick

r21 的 self picker 行为保持不变。两个 picker 共享 `mObjectIDBuffer`，但通过 buffer owner tag 区分"上一次绘制者是 self 还是 other"，避免光标在自己和他人之间切换时的 stale read。

详情 → spec `docs/specs/ayastorm-r28-other-rigged-picker.md`

### 设置

| Key | 默认 | 用途 |
|---|---|---|
| `FSOtherRiggedPickerEnable` | `1` | 他人 picker 的主开关。`0` 时他人 avatar 右键完全恢复到 upstream worldray 行为 |
| `FSOtherRiggedPickerGPU` | `1` | GPU buffer pass 的 kill-switch。`0` = no-op，原样使用 upstream worldray。为 GPU pass 异常环境提供的退路 |
| `FSOtherRiggedPickerArmSeconds` | `1.0` | 对目标 avatar 最后一次 hover 后 ID pass 保持 armed 的秒数 |

> **默认相机 gate** (`RequireNonDefaultCamera` 等价语义) 和 **draw call / triangle budget** (512 / 1,200,000) 为内部 hardcoded，不作为 cvar 暴露。如需再调整，将在后续 release 改动 hardcoded 值，而不扩大 cvar 面。

### r28 同步纳入的修复

- **自己 avatar 脸部选中修复**: 右键自己的脸时存在 `mPick.mObjectID` 未被规范化为 `gAgent.getID()`、self menu 判定失败的路径。r28 将 upstream 的 self-object 命中规范化为 `gAgent.getID()`，并在 GPU self picker 未返回 attachment 命中时回退到 avatar body 选择。自己脸部右键现在能正确解析。

### 已知限制

- **HUD attachment**: 不在范围内 (HUD 相机为 screen-space，不存在于 `mObjectIDBuffer`) — 与 r21 相同
- **非 rigged attachment** (饰品、耳钉等): 不在范围内 — 回退到 upstream worldray pick
- **他人 avatar 的 alpha-blend 头发**: 与 r21 相同的 depth 限制 — alpha-discard 三角形从 picker 消失，但不写 depth 的真 alpha-blend 头发仍可能贯穿
- **拥挤场地**: 在多个 avatar 间频繁切换 hover 会增加 arm 切换。1.0 秒 armed window + 单人 scope 是有意保持紧凑以约束此成本
- **重外装**: 极重 rigged outfit 的 avatar 可能触及 512 draw call / 1.2 M triangle budget，该帧 GPU pass 失效，右键回退到 upstream worldray pick

### Credits

r28 实现 (他人 rigged ID pass、buffer owner discriminator、默认相机 gate、budget 控制、自己脸部选中修复) 由 [@t-noami](https://github.com/t-noami) 完成。

### 相关文档

- r28 spec / 架构 / 负载控制 / 验证日志: `docs/specs/ayastorm-r28-other-rigged-picker.md`
- r21 self picker spec (姊妹功能): `docs/specs/ayastorm-r21-self-rigged-picker.md`
- attachment 渲染 routing 参考: `docs/specs/ayastorm-attachment-rendering-routing.md`
