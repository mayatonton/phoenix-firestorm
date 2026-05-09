# AYAstorm r12.1 — 发布告知文案

供粘贴到 GitHub release 页面的短文案。**r12.1 是首个把 r11 + r12 + r12.1 一并打包的公开发布**，因此本告知以 r10 → r12.1 的累积差异作为整体呈现。

功能详情常驻于面向用户的指南 (`doc/3dstream-tag-guide.{ja,en,zh}.md`) 与各规格书。本告知只做导引 + 差异要点。

---

## AYAstorm r12.1 — 双耳化 + 会场残响 + stereo→5.1 上混 + LFE gain

r10 → r12.1 一次跳跃同时提供以下内容。**r11 与 r12 均未单独发布 — 为把标签格式变更集中在一个阶段** 一并并入本次发布。

### r11 起源 — 推流者掌控的耳机体感 + 会场感

- **`{binaural:on|off}` (短形式 `bin`)**: 在每条声道上插入 lite-HRTF (ITD + 空气吸收 HF 衰减)。详见 → [tag-guide §7.1](../doc/3dstream-tag-guide.zh.md#71-binauralonoff-短形式-bin) / 规格 `doc/spec_binaural_venue_reverb.md`
- **`{venue:NAME}` (短形式 `v`)**: 9 种会场残响预设 — `dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`。详见 → [tag-guide §7.2](../doc/3dstream-tag-guide.zh.md#72-venuename-短形式-v)
- **`{wetgain:N}` (短形式 `wg`)**: 0.0–2.0 的 wet 倍率。所有 IR 已 unity-gain 归一化，切换 venue 时无需 retune。**默认值在 r12.1 由 `1.0` 改为 `0.2`** (反映音乐用途的实用区间 — 详见下文 r12.1 段落)。详见 → [tag-guide §7.3](../doc/3dstream-tag-guide.zh.md#73-wetgainn-短形式-wg)
- **推流者主导模型**: 这些键以根 prim Description 为真理。**不新增听者侧 Preferences UI**。仅为例外救援用途提供 sentinel debug 设置 `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain`。详见 → [tag-guide §7.5](../doc/3dstream-tag-guide.zh.md#75-推流者主导模型) / [§12.2](../doc/3dstream-tag-guide.zh.md#122-debug-settings-高级调优)

### r12 起源 — 把 stereo 推流展开到 5.1 布置

- **`{upmix:on|off}` 标签**: viewer 内 DSP 由 2ch 生成 6ch (FL/FR/C/Ls/Rs/LFE)。考虑到 SL 推流软件多数仅支持 stereo，此功能让 r10 建立的 6 扬声器布置 **在 stereo 推流上也能体验**。详见 → [tag-guide §8](../doc/3dstream-tag-guide.zh.md#8-stereo51-上混-r12) / 规格 `doc/spec_stereo_upmix.md`
- **算法固定 (NG1)**: DPL2 系矩阵解码 + 频段分离 (LFE LPF / 中央漏出消除 / 后置 decorrelation)。不提供 `{upmix:dpl2|logic7|...}` 等选择 (= "不增加表现不确定性" 方针)。
- **5.1 native 自动 bypass**: 源 ch ≥ 6 时 `{upmix:on}` 自动 bypass (chat 通知一次)。同一份 Description 可同时用于 stereo 与 5.1 native 素材。
- **默认 `off` (opt-in)**: 保留 r10 行为；推流者通过标签主动开启。
- **标签短形式**: `binaural`/`venue`/`wetgain` → `bin`/`v`/`wg`，9 种 venue 取值也都有 1–2 字符别名 (`d`/`rs`/`rm`/`hs`/`hm`/`hl`/`cl`/`ct`/`od`)。长形式与短形式完全等价。便于装入 SL 的 127 字节 Description 限制。详见 → [tag-guide §4.5](../doc/3dstream-tag-guide.zh.md#45-键名--venue-取值的短形式-r12--r121)
- **推流者向 LSL 扩展** (`doc/lsl/aya_3dstream_setup.lsl`): 可从 menu/dialog 设置 r11 标签 (binaural/venue/wetgain)、r12 标签 (upmix) 与 r12.1 标签 (lfegain)。输出 **始终使用短形式**。
- **macOS 构建复活**: r10.x 中暂停的 macOS 构建在 r12 复活。Mac 用户请按 r10 → r12.1 迁移。

### r12.1 起源 — LFE gain + 实时调参修正

- **`{lfegain:N}` (短形式 `lg`)**：`{ch:LFE}` 路径与 `{upmix:on}` 时的 LFE band 增益倍率 (0.0〜4.0，默认 1.0)。用途包括：在听者侧抬升源端 LFE 总线录制偏弱的素材，或当 LFE 图元挂在非低音炮的普通扬声器上时设为 `0` 以阻止低频泄漏。听者侧 sentinel `Stream3DLfeGain` 同步追加。详见 → [tag-guide §7.4](../doc/3dstream-tag-guide.zh.md#74-lfegainn-短形式-lgr121-新增) / 规格 `doc/spec_stereo_upmix.md` §4.7
- **`wetgain` 默认值 `1.0` → `0.2`**：在 hall / cathedral 等长尾预设下 `1.0` 会让源声饱和；新默认反映了试听确认的音乐用途实用区间 0.1〜0.5。LSL UI 的 quick-pick 也重新刻度为 `0.1`〜`0.5` 的细刻度。详见 → [tag-guide §7.3](../doc/3dstream-tag-guide.zh.md#73-wetgainn-短形式-wg)
- **听者侧 debug settings 实时调参修正**：r12 发布时 `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 出现回归 — 修改后必须触摸图元 (重新解析 Description) 才生效。r12.1 恢复了标准的"下一帧生效"语义。详见 → [tag-guide §12.3](../doc/3dstream-tag-guide.zh.md#123-设置的持久化与即时生效)
- **per-spk 音量调节实时化**：r10 6 扬声器布置的 per-spk 音量补正 (`Stream3DSpkVol*`) 此前必须 rebuild (触摸图元) 才生效，r12.1 改为 **下一帧生效**。调试 / 调音工作大幅减负。
- **routing diagnostic 适配 upmix**：`Stream3DDescriptionScan` 的 routing 输出未正确反映 r12 引入的 upmix 6ch routing。r12.1 修复后，`{upmix:on}` 时也会显示真实的 channel→spk 映射。
- **土地音 (parcel music) 播放品质 opt-in 改善**：新增 `FSParcelStreamQuality` debug setting，针对高码率 (256/320 kbps mp3、FLAC-over-HTTP) 土地音 stream 出现的 **buffer starvation** 问题 (默认 `0` = 与上游 FS bit-identical / `1` = AYAstorm 增强)。设为 `1` 时，stream buffer hint 提升至 320 kbps 假设，resampler 切换为 SPLINE，并在 stream group 上挂接 +4 dB high-shelf @ 6 kHz 的轻微 EQ，对典型 128 kbps mp3 配信常见的「闷感」做轻度补偿。与 3dstream 路径互不依赖，对既有用户无影响。详见 → `doc/spec_parcel_stream_quality.md`

### 既有布置的处理

r8 / r9 / r10 既已布置的所有 prim **无需改动标签即可继续运行**。r11/r12/r12.1 功能依设计为推流者 opt-in (默认值: `binaural=off` / `venue=dry` / `wetgain=0.2` / `lfegain=1.0` / `upmix=off` = r10 行为，wetgain 默认值在 r12.1 下调)。

### 已知限制

- `hall_medium` / `hall_large` / `cathedral` CPU 较重 (相对 r10 +7.7〜+10.2pp)。低规格机器请优先使用 `room_small` / `room_medium` / `hall_small`。详见 → [tag-guide §7.2](../doc/3dstream-tag-guide.zh.md#72-venuename-短形式-v) CPU 表
- 不提供听者侧 venue / binaural / lfegain 一般 UI (推流者主导模型)。
- bus-tail 配置的 VenueReverb 是 **stereo IR convolver** — 即使 6 扬声器布置，wet 也以固定的 master 立体声像形式被听到，而非 per-spk 独立残响。改进方案 (per-channel reverb / pre-3D send / L/R wet decorrelation 三案) 延至 r13+。详见 → `docs/ayastorm-r12-stereo-upmix.md` §6.4
- SOFA 个人 HRTF / Steam Audio 集成 / VenueReverb CPU 优化 / air absorption 客观 FFT 测量 / 公开 README 推迟到 r13+。

### IR 许可

附带的 venue IR 来自 OpenAIR 系 (CC-BY 4.0)。出处见 `app_settings/venue_ir/CREDITS.md`。

### 文档

- 用户向指南: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md`
- r11 规格: `doc/spec_binaural_venue_reverb.md`
- r12 / r12.1 规格: `doc/spec_stereo_upmix.md` (r12.1 扩展位于 §4.7)
- r12 / r12.1 实现记录: `docs/ayastorm-r12-stereo-upmix.md` (r12.1 follow-on 位于 §6)
- 土地音 (parcel music) 改善规格: `doc/spec_parcel_stream_quality.md` (r12.1 同时附带，与 3dstream 互不依赖)
- 路线图: `docs/ayastorm-stream3d-roadmap.md`
