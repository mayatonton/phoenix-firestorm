# AYAstorm r24 — 发布公告

**r24 把 CEF / Dullahan 路径的 media audio (MOAP / Web / HTML5 / YouTube) 切换到 viewer 内部的 FMOD 2D channel** — viewer 的 Media volume / mute 现在能对这些 CEF media 直接生效。

> **发布形态**: r24 作为 r13〜r24 一并发布 tag 的其中一个 feature 出货。同捆的其它 release note 由 GitHub Release 页面直接给出链接。

实现细节 / 已知 limits / maintainer review 回应 / Win / Linux 验证步骤都保留在永久规格 (`docs/ayastorm-r24-moap-audio-to-fmod-2d.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r24 — MOAP audio to FMOD 2D channel

### r24 主轴: 把 CEF/MOAP audio 接到 viewer 的 FMOD 2D channel

到 r23 为止,AYAstorm 中 MOAP / Web / HTML5 / YouTube 等 CEF / Dullahan 路径的 media audio 一律走 **CEF native output (直达 OS audio device)**,viewer 的 Media volume / mute 只能通过 `VolumeCatcher` 间接覆盖。带来的现象是: 把滑块拉到底也不会完全静音、Media mute 与 slider 行为有时不一致。

r24 借助 Dullahan fork 的 audio callback API,把 CEF decode 出来的 PCM 提到 viewer 进程内,再用 **viewer 的 FMOD 2D channel** 播放。Media volume / mute 直接落在 FMOD channel 上 — 对 MOAP / YouTube 的音量管理,跟 Parcel Music / Streaming Music 同一感觉。

### 工作原理

新主路径:

```
MOAP / Web / HTML5 / YouTube
  → CEF / Dullahan
  → Dullahan audio callback (fork API)
  → media_plugin_cef
  → shared memory audio ring
  → viewer
  → LLMediaAudioStream
  → FMOD 2D channel (OPENUSER stream)
  → audio device
```

音量控制:

```
viewer Media volume / mute
  → LLViewerMediaImpl
  → LLMediaAudioStream
  → FMOD channel volume
```

macOS 上,CEF 在进程内开的 native output AudioUnit 由 `VolumeCatcher` 常时 mute,确保 FMOD 路径是唯一可听路径 (避免双重播放)。

### Build-time fallback switch: `LL_DULLAHAN_AUDIO_CALLBACK`

新增 CMake 选项 `-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE/FALSE`,在两种构建模式间切换:

| flag | Dullahan | audio path |
|---|---|---|
| `FALSE` (**default**) | upstream `secondlife/dullahan` v1.26.0-CEF_139.0.40 | CEF native output → OS audio device (与 r23 及以前相同) |
| `TRUE` | `t-noami/dullahan` fork v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3 | viewer 端 FMOD 2D channel |

`autobuild.xml` 中并列两个 installable (`dullahan` 与 `dullahan_aya_audio`),`indra/cmake/CEFPlugin.cmake` 依 flag 仅 fetch 被选中的那一个。切换 flag 后重新 configure 即可,`autobuild uninstall <other>` 与 sentinel reset 会自动跑,文件冲突不会发生。

**为什么要 fallback**: fork 属个人 release,有取下风险。即便 fork 暂时不可用,AYAstorm 必须仍可凭 upstream `secondlife/dullahan` 构建。Audio callback 路径被 `#if LL_DULLAHAN_AUDIO_CALLBACK` 包裹,OFF 模式下新增代码 0 行编入。

### Format 切换跟随与 lock-free ring 设计

为检测 CEF media format 切换 (例: 44.1 kHz → 48 kHz),audio ring header 加入 `mFormatSerial`。writer (`media_plugin_cef`) 在新 stream 开始时推进 sample rate / channel count / serial。reader (`LLMediaAudioStream::update()`) 比对当前 FMOD channel 创建时的 captured 值,一旦不一致就 `stop()` 后再 `start()` 重建 FMOD sound,避免在 format 切换后用旧 channel 继续读 PCM。

ring 是 **single-writer / single-reader** 设计: writer (CEF audio callback 单 thread) release-store `mWriteFrame`,reader (FMOD callback 单 thread) release-store `mReadFrame`。writer 不会去动 reader 的 pointer,因此 frame 计算中 read pointer 被改的竞态不会发生。ring 满时 writer 不写入而是 drop,并使 `mTotalFramesDropped` 递增。

### macOS VolumeCatcher 的定位

本 release 保留的 `mac_volume_catcher.cpp` (CoreServices / AudioUnit 版),**仅用于常时 mute CEF native output**。Media volume 的控制主路径是 FMOD channel 那一侧。若维持 `mac_volume_catcher_null.cpp` (no-op),CEF native output 会继续发声,与 FMOD 路径形成双重播放 — 故 ON 模式下不回退到 null 版。

QuickTime 依赖的旧版 **不** 回退采用 — 在现代 macOS SDK 与 Apple Silicon arm64 构建上都不现实。将来若 Dullahan / CEF 可不再开 native audio output (或加入 audio-callback only 模式),VolumeCatcher 依赖即可削减。

### 设置

**有意为零。** Viewer 端不引入新 cvar / UI。

- 切换只在编译期 (`LL_DULLAHAN_AUDIO_CALLBACK`)
- 运行期不可切换 (因为对应不同 installable / 不同 Dullahan API)
- 用户操作仍是既有的 Media volume / mute 滑块

### 已知限制

- **Windows / Linux 实机验证**: r24 push 时点,仅 macOS 完成了 ON 模式的实机 PASS。Windows / Linux ON 模式的构建与播放验证作为 release 后续逐步进行
- **libVLC direct media / `.mp3` `.mp4` 直 URL / Linux GStreamer**: 不在本次范围内,仅 CEF/MOAP 路径切到 FMOD 2D
- **3D stream / parcel music / Voice**: 路径保持不变
- **长时间播放**: 短时段 `ring_dropped=0` / `frames_silenced=0` 已确认,多小时连续测试未做

### 实现概要

- 新增 file: `indra/llaudio/llmediaaudiostream.cpp/.h` (FMOD 2D OPENUSER stream)、`indra/llcommon/llpluginaudio.h` (ring header struct)
- 修改 file: `llpluginclassmedia.cpp/.h` (`ensureAudioSharedMemory()` / `getAudioData()`)、`media_plugin_cef.cpp` (callback 注册、`audio_shm_set` message、`writeAudioPacketToRing`)、`llviewermedia.cpp/.h` (`mMediaAudioStream` 成员)、`mac_volume_catcher.cpp` (CEF native output mute)
- `autobuild.xml`: 两个 installable 并列 (`dullahan` / `dullahan_aya_audio`)
- `indra/CMakeLists.txt` / `indra/cmake/CEFPlugin.cmake`: 依 flag 分支
- macOS ON 模式构建 + 播放验证 PASS,Linux / Windows ON 模式 release 后验证

### Credits

r24 的本体实现 (Dullahan fork audio callback + viewer 侧 FMOD 2D 接线 + shared memory ring + macOS VolumeCatcher 策略) 由 [t-noami](https://github.com/t-noami) 完成。AYAstorm 侧补加 `LL_DULLAHAN_AUDIO_CALLBACK` 构建期 fallback switch,以便 fork 不可用时仍可凭 upstream `secondlife/dullahan` 构建。

### 文档

- r24 完整 spec / maintainer review 回应 / Dullahan package / Windows / Linux 验证步骤: `docs/ayastorm-r24-moap-audio-to-fmod-2d.md`
- Dullahan fork release: `https://github.com/t-noami/dullahan/releases/tag/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3`
