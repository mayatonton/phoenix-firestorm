# AYAstorm r25 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r25 修复了 AYAstorm r10.x-bugfix-1 以后一直损坏的土地音乐 Ogg Vorbis live stream 播放** — 在保留 Ogg Opus 支持的前提下,使 Ogg Vorbis 的 Icecast live stream 在 AYAstorm 上也能正常播放。

实现细节、调查日志、验证 URL 都保留在永久文档 (`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r25 — 土地音乐 Ogg Vorbis live stream 播放修复

### r25 主轴: 找回 AYAstorm 自己损坏的 Ogg Vorbis 土地音乐

AYAstorm 在 r9-opus 系列中加入了独立的 FMOD codec plugin (用于 Ogg Opus 播放)。r10.x-bugfix-1 将该 codec 的注册 priority 改为 `0` (FMOD 最优先),并新增 4-byte "OggS" capture probe **提前 reject 非 Ogg stream (例如 MP3)**,从而保护 built-in HTTP codec 不被破坏。

但该设计对 Ogg Vorbis live stream 存在盲点。Vorbis stream 通过 `OggS` 4-byte gate 后,custom codec 读取第一个 Ogg packet,看到 `\x01vorbis` 而非 `OpusHead`,返回 `FMOD_ERR_FORMAT`。FMOD 随后尝试 fallback 到 built-in Vorbis codec — 但 HTTP live stream 不能回卷到头部,因此 built-in Vorbis path 以 `FMOD_ERR_FILE_COULDNOTSEEK` 失败。结果就是 **从 r10.x-bugfix-1 直到 r24,Ogg Vorbis 土地音乐在 AYAstorm 上完全无法播放** (vanilla Firestorm 因为没有自定义 codec 跑在 built-in Vorbis 之前所以不受影响)。

r25 将 custom FMOD codec 自身扩展为 **同时支持 Ogg Opus 与 Ogg Vorbis**,fallback path 永不会被踩。

### 工作原理

custom codec 一次读取 Ogg first packet,按内容分支:

```
土地音乐 URL
  → LLStreamingAudio_FMODSTUDIO
  → FMOD::System::createStream(url)
  → AYAstorm Ogg Opus/Vorbis codec (priority 0)
     ├─ first packet "OpusHead"  → opus_decoder / opus_multistream_decoder (既有 path)
     └─ first packet "\x01vorbis" → libvorbis decoder (新 path)
  → PCMFLOAT 喂给 FMOD mixer
```

- 非 Ogg stream (MP3 等) 仍按 4-byte `OggS` gate reject,built-in codec 照常处理
- Vorbis stream **在 custom codec 内端到端解码**,fallback 引起的 seek failure 不会发生
- Opus mapping family 0 (mono/stereo) / family 1 (multistream 5.1ch surround) 既有 path 不动

### 设置

**正常使用无需任何操作。** 仅追加 2 个诊断用 cvar:

| Cvar | 默认 | 用途 |
|---|---|---|
| `AYAOpusCodecEnable` | `1` | OFF 时 custom codec 完全不注册,仅 built-in codec 运行。该模式下 Opus stream 无法播放;诊断专用 |
| `AYAOpusCodecPriority` | `0` (最高) | FMOD codec dispatch 顺位数值。非 `0` 会导致 Opus/Vorbis 与 built-in 的顺序冲突而损坏,诊断以外请勿改动 |

两者都需要重启 viewer 才能生效。**正常运行请保持默认值。**

### 迁移备注

- 配信者无需任何操作。Icecast Ogg Vorbis stream 维持原样,AYAstorm listener 可正常收听
- listener 无需任何操作。启动 r25 即可恢复 Ogg Vorbis 土地音乐
- 预期效果是「在 vanilla Firestorm / 其他 viewer 能听见的 Vorbis 配信,在 AYAstorm 也同样能听见」 — 行为回归正确

### 已知限制

- **chained Ogg / serial change**: 单一 HTTP stream 中途切换到不同 Ogg logical stream (serial number 变更) 的场景不处理。Icecast 通常运用中罕见,但配信侧长时间运行时可能出现。检测到 serial change 时的自动 re-init 留到 r26 以后
- **其他非 Vorbis Ogg 系 codec** (Theora / Speex 等): 本 codec 有意不吸入。不采用「Ogg 一律吞下」的设计 (误判风险)
- **macOS / Windows 实机验证**: PR 作者在 macOS arm64 验证 PASS,AYAstorm 侧在 Linux 构建并验证 PASS;Mac/Win 的 Release 二进制验证将在 tag 切出时进行

### 实现概要

- `indra/llaudio/fmod_codec_ogg.{cpp,h}` — Ogg Opus/Vorbis 两栖 codec (PR #75 之后从 `fmod_codec_opus.{cpp,h}` rename)
- `indra/llaudio/llaudioengine_fmodstudio.cpp` — codec 注册单 1 个,登记名 "AYAstorm Ogg Opus/Vorbis codec"
- `indra/llaudio/llpositionalstreammulti.cpp` — 3D Stream 路径将 plugin codec name `"Ogg Vorbis"` 提升为 `FMOD_SOUND_TYPE_OGGVORBIS`
- `indra/newview/app_settings/settings.xml` — 追加 2 个诊断 cvar
- 既有 Opus path (mono/stereo/multistream 5.1ch) 不动,5.1ch surround Opus source 播放亦经验证

### Credits

r25 的本实装 (Ogg Vorbis decoder 取入 + Opus 保留 + fallback path 回避设计 + 调查文档) 由 [t-noami](https://github.com/t-noami) 完成。AYAstorm 侧导入了文件 / 函数 / header guard 的 rename cleanup (`fmod_codec_opus` → `fmod_codec_ogg`、`FMODGetCodecDescriptionOpus()` → `FMODGetCodecDescriptionOgg()`)。

### 文档

- r25 完整调查日志 / 验证 URL / 失败假设整理 / 设计笔记 / 6ch multichannel 处理: [`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`](./ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- 既往 Opus codec 相关 (r9-opus 系列): [`docs/specs/spec_5_1ch_opus_decode.md`](../specs/spec_5_1ch_opus_decode.md)
