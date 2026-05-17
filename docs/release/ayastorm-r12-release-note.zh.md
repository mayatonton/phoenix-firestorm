# AYAstorm r12 — 发布告知文案

供粘贴到 GitHub release 页面的短文案。AYA 在确定发布时复制使用。

功能详情常驻于面向用户的指南 (`docs/guides/3dstream-tag-guide.{ja,en,zh}.md`) 与各规格书。本告知只做导引 + 差异要点。

---

## AYAstorm r12 — 双耳化 + 会场残响 + stereo→5.1 上混

r10 → r12 一次跳跃同时提供以下内容。**r11 不单独发布 — 为避免标签格式变更分两阶段引起混乱**，整体并入 r12。

### r11 起源 — 推流者掌控的耳机体感 + 会场感

- **`{binaural:on|off}` (短形式 `bin`)**: 在每条声道上插入 lite-HRTF (ITD + 空气吸收 HF 衰减)。详见 → [tag-guide §7.1](../guides/3dstream-tag-guide.zh.md#71-binauralonoff-短形式-bin) / 规格 `docs/specs/spec_binaural_venue_reverb.md`
- **`{venue:NAME}` (短形式 `v`)**: 9 种会场残响预设 — `dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`。详见 → [tag-guide §7.2](../guides/3dstream-tag-guide.zh.md#72-venuename-短形式-v)
- **`{wetgain:N}` (短形式 `wg`)**: 0.0–2.0 的 wet 倍率。所有 IR 已 unity-gain 归一化，切换 venue 时无需 retune。详见 → [tag-guide §7.3](../guides/3dstream-tag-guide.zh.md#73-wetgainn-短形式-wg)
- **推流者主导模型**: 这 3 个键以根 prim Description 为真理。**不新增听者侧 Preferences UI**。仅为例外救援用途提供 sentinel debug 设置 `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain`。详见 → [tag-guide §7.4](../guides/3dstream-tag-guide.zh.md#74-推流者主导模型) / [§12.2](../guides/3dstream-tag-guide.zh.md#122-debug-settings-高级调优)

### r12 单独 — 把 stereo 推流展开到 5.1 布置

- **`{upmix:on|off}` 标签**: viewer 内 DSP 由 2ch 生成 6ch (FL/FR/C/Ls/Rs/LFE)。考虑到 SL 推流软件多数仅支持 stereo，此功能让 r10 建立的 6 扬声器布置 **在 stereo 推流上也能体验**。详见 → [tag-guide §8](../guides/3dstream-tag-guide.zh.md#8-stereo51-上混-r12) / 规格 `docs/specs/spec_stereo_upmix.md`
- **算法固定 (NG1)**: DPL2 系矩阵解码 + 频段分离 (LFE LPF / 中央漏出消除 / 后置 decorrelation)。不提供 `{upmix:dpl2|logic7|...}` 等选择 (= "不增加表现不确定性" 方针)。
- **5.1 native 自动 bypass**: 源 ch ≥ 6 时 `{upmix:on}` 自动 bypass (chat 通知一次)。同一份 Description 可同时用于 stereo 与 5.1 native 素材。
- **默认 `off` (opt-in)**: 保留 r10 行为；推流者通过标签主动开启。

### r12 共通改进

- **标签短形式**: `binaural`/`venue`/`wetgain` → `bin`/`v`/`wg`，9 种 venue 取值也都有 1–2 字符别名 (`d`/`rs`/`rm`/`hs`/`hm`/`hl`/`cl`/`ct`/`od`)。长形式与短形式完全等价。便于装入 SL 的 127 字节 Description 限制。详见 → [tag-guide §4.5](../guides/3dstream-tag-guide.zh.md#45-键名--venue-取值的短形式-r12)
- **推流者向 LSL 扩展** (`docs/guides/lsl/aya_3dstream_setup.lsl`): 可从 menu/dialog 设置 r11 标签 (binaural/venue/wetgain) 与 r12 标签 (upmix)。输出 **始终使用短形式**。
- **macOS 构建复活**: r10.x 中暂停的 macOS 构建在 r12 复活。Mac 用户请按 r10 → r12 迁移。

### 既有布置的处理

r8 / r9 / r10 既已布置的所有 prim **无需改动标签即可继续运行**。r11/r12 功能依设计为推流者 opt-in (默认值: `binaural=off` / `venue=dry` / `wetgain=1.0` / `upmix=off` = r10 行为)。

### 已知限制

- `hall_medium` / `hall_large` / `cathedral` CPU 较重 (相对 r10 +7.7〜+10.2pp)。低规格机器请优先使用 `room_small` / `room_medium` / `hall_small`。详见 → [tag-guide §7.2](../guides/3dstream-tag-guide.zh.md#72-venuename-短形式-v) CPU 表
- 不提供听者侧 venue / binaural 一般 UI (推流者主导模型)。
- SOFA 个人 HRTF / Steam Audio 集成 / VenueReverb CPU 优化 / air absorption 客观 FFT 测量 / 公开 README 推迟到 r13+。

### IR 许可

附带的 venue IR 来自 OpenAIR 系 (CC-BY 4.0)。出处见 `app_settings/venue_ir/CREDITS.md`。

### 文档

- 用户向指南: `docs/guides/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md`
- r11 规格: `docs/specs/spec_binaural_venue_reverb.md`
- r12 规格: `docs/specs/spec_stereo_upmix.md`
- r12 实现记录: `docs/ayastorm-r12-stereo-upmix.md`
- 路线图: `docs/ayastorm-stream3d-roadmap.md`

---

## r12.1 — LFE gain + 实时调参修正 (2026-05-09)

实际试听反馈带来的小规模后续更新。

- **`{lfegain:N}` (短形式 `lg`)**：`{ch:LFE}` 路径与 `{upmix:on}` 时的 LFE band 增益倍率 (0.0〜4.0，默认 1.0)。听者侧 sentinel `Stream3DLfeGain` 同步追加。详见 → [tag-guide §7.4](../guides/3dstream-tag-guide.zh.md#74-lfegainn-短形式-lgr121-新增) / 规格 `docs/specs/spec_stereo_upmix.md` §4.7
- **`wetgain` 默认值 `1.0` → `0.2`**：在 hall / cathedral 等长尾预设下 `1.0` 会让源声饱和；新默认反映了试听确认的音乐用途实用区间 0.1〜0.5。LSL UI 的 quick-pick 也重新刻度为 `0.1`〜`0.5` 的细刻度。详见 → [tag-guide §7.3](../guides/3dstream-tag-guide.zh.md#73-wetgainn-短形式-wg)
- **听者侧 debug settings 实时调参修正**：r12 发布时 `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 出现回归 — 修改后必须触摸图元 (重新解析 Description) 才生效。r12.1 恢复了标准的"下一帧生效"语义。详见 → [tag-guide §12.3](../guides/3dstream-tag-guide.zh.md#123-设置的持久化与即时生效)
- **已知限制**：bus-tail 配置的 VenueReverb 是 stereo IR convolver — 即使 6 扬声器布置，wet 也以固定的 master 立体声像形式被听到，而非 per-spk 独立残响。改进方案延至 r13+。详见 → `docs/ayastorm-r12-stereo-upmix.md` §6.4
