# AYAstorm r26 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r26 将 MOAP (Media-on-a-Prim) 的音声接入 3D Stream 的 speaker routing** — 现有的 `{ch:L}/{ch:R}/{ch:FL}/...` 分散立体声 / 5.1 配置 / HRTF / venue reverb / occlusion 流水线,现在源不是 HTTP URL 而是 MOAP 面时也能完整生效。

实施方针、现行规格梳理、接线设计、验证计划等细节全部保留在永久文档 (`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`) 中。本说明仅作为该文档的入口与差异亮点。

---

## AYAstorm r26 — MOAP 音声接入 3D Stream

### r26 主轴: 让 MOAP 的音从 3D Stream speaker prim 中传出

到 r25 为止,AYAstorm 的 3D Stream 仅接受以 URL 形式传入的 HTTP 音频流 (Icecast / SHOUTcast) 作为源。MOAP 的音通过另一条路径 (经 `LLMediaAudioStream` 的 2D FMOD 播放) 发声,完全不经过 3D Stream 的 speaker prim 分配 / 5.1 routing / HRTF / venue reverb / occlusion / 音量控制。

r26 在 `LLPositionalStreamMulti` 中新增了 **与 URL source 并列的 PCM ring source**,将 CEF / Dullahan 的 audio callback 写入共享内存的 float PCM 直接当作 3D Stream 的输入源。MOAP video 仍然在 MOAP 面上显示,但音频从链接集的 speaker prim 发声 — 这套构成 **配信者只用写 tag 就能完成**。

### 工作原理

在根 prim Description 写 `{source:media}` (2ch) 或 `{source:media-5-1}` (6ch)。speaker prim 仍按原来方式持有 `{ch:L}/{ch:R}/{ch:FL}/{ch:FR}/{ch:C}/{ch:LFE}/{ch:SL}/{ch:SR}`。

```
根 Description:
  [3dstream-stereo:{source:media}{ch:L}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

```
MOAP video → CEF/Dullahan audio callback
  → LLPluginAudioRingHeader (共享内存 PCM ring,magic="AYAA" v2)
  → LLViewerMediaImpl::getAudioRingForStream3D() ── LL_DULLAHAN_AUDIO_CALLBACK gate
  → LLPositionalStreamMulti (SourceKind::MediaRing)
     ├─ ring 读取 → 既有 LLMultiTailRing
     └─ 每个 speaker prim 创建 FMOD_OPENUSER | FMOD_3D mono sound + routing matrix
  → 既有 HRTF / venue reverb / occlusion / volume / 5.1 routing 原样应用
```

- callback bus 的实际 channel 数 (CEF 固定回传 8ch) 与 **逻辑 source channel 数** (`{source:media}`=2 / `{source:media-5-1}`=6) 解耦。对 2ch MOAP 使用 `{upmix:on}` 同样能正常工作
- 链接集内若有多个 MOAP 面,根 tag 用 `{link:N}{face:M}` 指定要 bind 的面。仅 1 个面时可省略 `{link}` / `{face}`
- 同一链接集内 `{url}` 与 `{source:media}` 互斥。仍可让 media 面照常显示,同时以 URL 做 3D 化 (此时 MOAP 音频按普通 2D media 播放)
- plugin 销毁时的崩溃安全性: `destroyMediaSource()` → `onMediaSourceDestroying()` → `setMediaRingFor3DStream(nullptr)` → `stopDecodeThread()` (join) → ring 指针交换,按此顺序确保共享内存 unmap 之前 decode worker 必定已退出
- ring header 每次 pump 迭代都会 re-validate magic/version。format 变化即 reopen,ring 消失则 setFailed
- r24 加入的 `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch (upstream / fork dullahan 切换) 维持。OFF 构建中仅 URL 路径存活,media 路径自动停用

### 设置

**正常运行无需 viewer 端配置。** 配信者写 tag,viewer 端启动 r26 即可。`{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` / `{link:N}` / `{face:M}` 的写法请参照 [3D Stream tag 指南 §6.9](../guides/3dstream-tag-guide.zh.md#69-mediamoap-音源的使用-r26)。

音量策略按 MOAP 面数量分:

- **单一 MOAP 面**: 该 media 自身的 volume / mute 作为 source gain 生效 (与 single-media 等效)
- **多 MOAP 面**: 被 routed 到 3D 的那个面按 source gain 1.0 处理,由 3D Stream 的 master / 各 speaker 音量控制。其余未 routed 的 media 面照常按 2D media volume 播放

### 迁移备注

- **配信者**: 根 Description 改写成 `[3dstream-stereo:{source:media}{ch:L}]`。链接集有 2 个以上 MOAP 面时再加 `{link:N}{face:M}`
- **listener**: 无操作。启动 r26,走近带 tag 的链接集,MOAP 音声就从 3D Stream speaker prim 中传出。UI 无改动
- **streamer-led 模式延续**: 由配信者的 tag 决定一切,listener 端无 UI 可配置,与 r11 以来一致

### 已知限制

- **不暴露 7.1 (8ch) speaker routing**: CEF / Dullahan 的 callback bus 内部按 8ch 接入,但 tag 端只暴露 `{source:media}` (2ch) 与 `{source:media-5-1}` (6ch)。`BL`/`BR` enum 与 routing 代码路径已 forward-compat 写入,7.1 routing 留到 r27+
- **A/V sync**: prebuffer ~43 ms (2048 frames @ 48 kHz),target buffered ~85 ms (4096 frames)。与 MOAP video 的偏差预期与 video 解码侧延迟相抵消,不提供 tuning knob
- **MOAP page 中途切换**: MOAP 切到另一页面且音频 format (sample_rate / channels) 变化时,ring 会 reopen。pump loop 会检测 format 变化并重开,但可能出现短暂瞬断
- **Linux / macOS / Windows 实机验证**: AYAstorm 侧在 merge 后即以 Linux 最小 stereo MOAP 构成 (root + child) 实机确认,macOS / Windows 在下次切 Release 二进制时验证

### 实现概要

- `indra/llcommon/llpluginaudio.h` — 新增 ring header (`LLPluginAudioRingHeader`,magic `0x41594141` "AYAA" / version 2 / max 8ch,`ll_plugin_audio_ring_supported_3d_channel_count()` 允许 1/2/6/8 ch)
- `indra/llcommon/tests/llpluginaudio_test.cpp` — 新增 unit test (ring 尺寸计算 / 可接受 channel 数 / 6ch & 8ch enum 顺序)
- `indra/llaudio/llpositionalstreammulti.{h,cpp}` — 新增 `SourceKind {Url, MediaRing}`,以及 `startMedia()` / `setMediaRingFor3DStream()` / `pumpMediaRingSource()` 与 CEF 7.1 channel remap (`kCef71ToStream8 = {0,1,2,3,6,7,4,5}`)。抽取 `releaseSpeakerRuntime()` (无行为变化)
- `indra/newview/llpositionalstreammgr.{h,cpp}` — 新增 `DistSourceKind::{Url, Media}` / `SourceBindingKey` / tag parser 支持 `{source}` / `{link}` / `{face}`。新增 `onMediaSourceDestroying()` / `evaluateLinkset()` / `findMediaFor3DSource()` / `effectiveDistributedStreamVolume()`。`ChannelKind` 加入 `BL`/`BR`
- `indra/newview/llviewermedia.{h,cpp}` — 新增 `getAudioRingForStream3D()` / `getStream3DAudioGain()` / `setStream3DAudioRedirected()` (全部受 `LL_DULLAHAN_AUDIO_CALLBACK` 控制)。`destroyMediaSource()` 改为调用 `LLPositionalStreamMgr::onMediaSourceDestroying()` 担保 lifecycle 安全
- `indra/llplugin/llpluginclassmedia.cpp` — `getAudioData()` 加 `mPlugin->isRunning()` 守卫,新增 `audio_stream_format` plugin message handler
- `indra/media_plugins/cef/media_plugin_cef.cpp` — 在 start / stop / error 时发出 `audio_stream_format` (sample_rate / channels / max_channels)
- `autobuild.xml` — Dullahan bump 至 `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.4` (吸收 48kHz / 8ch `GetAudioParameters` override)
- `indra/cmake/FMODSTUDIO.cmake` — 把 libopus 挂到 `ll::fmodstudio` interface,确保手动指定 FMOD 库的构成下 AYAstorm FMOD codec plugin 的 libopus 符号不丢
- `indra/newview/CMakeLists.txt` — universal macOS 链接下,在 llaudio static archive 之后再链一次 `ll::fmodstudio`,使 AYAstorm FMOD codec 的符号可解析
- `docs/guides/3dstream-tag-guide.{ja,en,zh}.md` — 新增「Media / MOAP source (r26)」一节 (ja §6.7 / zh §6.9 / en §6.10),§6.3 key 表追加 `{source}` / `{link}` / `{face}`,明示与 `{url}` 的互斥及 single-media vs multi-media 的音量策略
- `docs/ayastorm-r26-moap-3d-stream-implementation-plan.md` — 新增永久文档 (方针 / 现行规格 / 接线设计 / 实施阶段 / 验证计划 / 已知课题)

### Credits

r26 的本实装 (实装计划文档 / `LLPluginAudioRingHeader` 设计 / `LLPositionalStreamMulti` 的 `MediaRing` source / CEF plugin `audio_stream_format` 发送 / `LLViewerMediaImpl` 的 3D Stream 接入 / 3 语言 tag 指南更新 / 单体测试 / Dullahan callback.4 bump / FMOD codec 链接修复) 由 [t-noami](https://github.com/t-noami) 完成。

AYAstorm 侧将 PR 原样取入,仅追加本发布说明 (3 语言)。

### 文档

- r26 实装计划文档 / 现行规格梳理 / 接线设计 / 实施阶段 / 验证计划 / 未决课题: [`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`](../ayastorm-r26-moap-3d-stream-implementation-plan.md)
- 3D Stream tag 指南 (`{source:media}` 用法见 §6.9): [`docs/guides/3dstream-tag-guide.zh.md`](../guides/3dstream-tag-guide.zh.md)
- r24 的 `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch (upstream / fork dullahan 切换): [`docs/release/ayastorm-r24-release-note.zh.md`](./ayastorm-r24-release-note.zh.md)
