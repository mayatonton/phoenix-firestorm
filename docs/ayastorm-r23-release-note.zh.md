# AYAstorm r23 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r23 把 AYAstorm 的 3D stream (positional / 5.1ch / distributed-stereo / binaural — r6 系以来) 接入 SL 的 `PARCEL_FLAG_SOUND_LOCAL` 边界规约** — 与 gesture / object sound 同规约。无新增 cvar、无反向 tag,parcel flag 本身就是 root truth。

> **发布形态**: r23 作为 r13〜r23 一并发布 tag 的主功能出货。同捆的其它 release note 由 GitHub Release 页面直接给出链接。

实现细节 / 已知限制 / 设置说明都保留在永久规格 (`docs/ayastorm-r23-parcel-bound-3d-stream.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r23 — Parcel-bound 3D stream

### r23 主轴: 把 3D stream 接入 SL 的 parcel 规约

到 r22 为止,AYAstorm 的 3D stream (positional / distributed-stereo / 5.1ch placement / venue reverb / binaural — r6 系以来) **无条件地漏到隔壁 parcel**。SL 的 vanilla 流程中,gesture / object sound / parcel music 已经在 source / listener 任一 parcel 立 `PARCEL_FLAG_SOUND_LOCAL` ("Restrict gestures and object sounds to this parcel") 时会被 parcel boundary 截断。3D stream 一直是这个规约的例外 — 对希望举办活动且不想外溢到邻接 parcel 的配信者来说,这是个问题。

r23 把 3D stream 直接接入同一规约。无新 cvar、无配信者侧 opt-in tag、无 escape hatch — parcel flag 本身即 root truth,与 listeners 对 gesture / object sound 的既有期待完全一致。

### 工作原理

在 3D stream channel 的 volume update hook 上加一行 `LLViewerParcelMgr::canHearSound(pos_global)`。行为:

1. listener 与 source 处于 **同一 parcel** → 可听 (现状维持)
2. 不同 parcel,**agent parcel** 立有 SOUND_LOCAL → 静音
3. 不同 parcel,**source parcel** 立有 SOUND_LOCAL → 静音
4. 其他 → 可听 (现状维持)

这是 SL 已用于 gesture / object sound / 部分 attached sound 的 **同一逻辑** — 没有发明新规约,只是把 3D stream 接入既有规约。

### per-channel position 与 source position

5.1ch placement / distributed-stereo (r8–r10) 中每个 FMOD channel 都有各自的空间位置,但 **parcel 判定使用 1 个点 — 3D stream root prim 的位置**,结果一律应用到全 channel (全鸣 or 全静)。

- 配信者 parcel 内的 listener → 全 ch 可听 (5.1 FR speaker 的音像即使物理上跨到邻 parcel 也 OK)
- 邻 parcel 的 listener → 全 ch 静音 (即使 FR 音像物理上就在 listener 附近也静)

per-channel parcel 判定会形成「偏侧」的音像,musically 不自然,故有意不采用 (spec §3.2)。

### 静音方式: volume=0 (不 paused)

静音通过 `setVolume(0)` 实现,不暂停 FMOD channel:
- volume=0 仍持续计算 channel 位置 / 空间衰减,parcel 回归时无缝恢复
- paused 在恢复时会有 click 噪音 / 需要 re-seek 的风险
- 由 `parcel_change` 信号驱动 + idempotent guard (状态未变化时不调 FMOD) 防止多余 API call

### 2 tier 评估模型

**Tier 1 (据置 stream,绝大多数)**: 仅在 `LLAgent::addParcelChangedCallback` 触发时重评。avatar 越过 parcel 边界的瞬间运行,per-frame 成本为零。

**Tier 2 (装着 stream,少见)**: binding 构建时在 attachment 上立 flag,仅装着 binding 在 per-frame 中重评 `canHearSound()`。没有装着 stream 时 Tier 2 循环不跑。有时也只是「装着 binding 数 × 1 query × 60 fps」,成本足够低。

### 设置

**有意不提供任何新 cvar。**

- escape-hatch cvar (如 "FSParcelBoundStream3D") 会让配信者绕过规约,不符合本 release 的初衷
- 配信者侧的反向 tag (如 "ignore parcel boundary") 同样会绕过规约且要求所有人感知,故不采用
- 想要 global broadcast 的配信者,只需保持 parcel 的 SOUND_LOCAL 为 off — 这恰是 SL 既有的 native pathway

### 配信者迁移备注

如果你正用 AYAstorm 3D stream 做配信,并依赖邻接 parcel 的 listener 也能听到:

- parcel flag 的 default 是 **off** — 若你没有开启 SOUND_LOCAL,**对你来说没有任何变化**
- 若你已在自己的 parcel 上开了 SOUND_LOCAL,邻接 parcel 的 listener 现在会正确停止收听 — 这与 SL 一直以来对 gesture / object sound 的行为一致,3D stream 此前只是一个无意的例外

### 已知限制

- **region 越境 TP**: 新 region 的 parcel-changed signal 触发后立即正确重适用。在极快的越境瞬间可能出现短暂的播放窗口,但有界
- **装着 stream 跨 parcel 行走**: 装着者本人始终与 source 同一 parcel (持续可听)。周围只有与装着者同 parcel 的 listener 才能听到
- **Venue reverb tail**: 本体 mute 后 reverb 输入停止,tail 自然衰减。无需特别处理

### 实现概要

- `llpositionalstreammgr.cpp` / `.h` — volume update hook 添加 `canHearSound()` 一行
- 无新增 cvar、无新增 uniform、无 UI 变更
- 3 OS (Linux / macOS / Windows) 构建 + 动作确认 PASS

### 文档

- r23 完整 spec / source position / 2 tier 评估 / 风险登记: `docs/ayastorm-r23-parcel-bound-3d-stream.md`
- 过去的 3D stream releases (r6–r12,含 listener binding): 参考 in-tree spec 文档及历次 release note (r10 5.1ch placement、r11 binaural + venue reverb 等)
