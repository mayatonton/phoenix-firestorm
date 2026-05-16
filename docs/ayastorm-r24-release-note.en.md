# AYAstorm r24 — Release Announcement

GitHub release page copy. **r24 routes CEF / Dullahan media audio (MOAP / Web / HTML5 / YouTube) through the viewer's FMOD 2D channel** — the viewer's Media volume / mute now applies cleanly to all CEF-driven media.

> **Distribution**: r24 ships as one of the features in the r13–r24 bundle tag. Other release notes for the bundled releases are linked directly from the GitHub Release page.

Implementation details, known limits, maintainer review responses, and Windows/Linux verification steps live in the permanent spec (`docs/ayastorm-r24-moap-audio-to-fmod-2d.md`). This note is the entry point and diff highlight.

---

## AYAstorm r24 — MOAP audio to FMOD 2D channel

### Headline: CEF/MOAP audio now flows through the viewer's FMOD 2D channel

Through r23, AYAstorm played MOAP / Web / HTML5 / YouTube and other CEF / Dullahan media audio via **CEF's native output (direct to the OS audio device)**. The viewer's Media volume / mute was an indirect overlay applied through `VolumeCatcher`. This caused well-known annoyances: the slider didn't fully silence the source, and Media mute didn't always behave consistently with the slider.

r24 uses the Dullahan fork's audio callback API to lift CEF-decoded PCM into the viewer process and plays it through the viewer's **FMOD 2D channel**. Media volume / mute is now applied directly on the FMOD channel — MOAP / YouTube volume control behaves the same way as Parcel Music / Streaming Music.

### How it works

New primary path:

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

Volume control:

```
viewer Media volume / mute
  → LLViewerMediaImpl
  → LLMediaAudioStream
  → FMOD channel volume
```

On macOS, the AudioUnit that CEF opens for its native output is held muted by `VolumeCatcher` so that the FMOD path is the only audible one (no double playback).

### Build-time fallback switch: `LL_DULLAHAN_AUDIO_CALLBACK`

A new CMake option `-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE/FALSE` selects between two build modes:

| flag | Dullahan | audio path |
|---|---|---|
| `FALSE` (**default**) | upstream `secondlife/dullahan` v1.26.0-CEF_139.0.40 | CEF native output → OS audio device (same as r23 and earlier) |
| `TRUE` | `t-noami/dullahan` fork v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3 | viewer's FMOD 2D channel |

`autobuild.xml` carries two installables side by side (`dullahan` and `dullahan_aya_audio`), and `indra/cmake/CEFPlugin.cmake` fetches only the one selected by the flag. Flipping the switch and re-running configure is enough — `autobuild uninstall <other>` and a sentinel reset run automatically, so file conflicts never occur.

**Why a fallback is needed**: the fork is a personal release with non-trivial distribution risk; if the fork is temporarily withdrawn, AYAstorm must still build against the upstream `secondlife/dullahan`. The audio callback path is gated by `#if LL_DULLAHAN_AUDIO_CALLBACK`, so in OFF mode none of the new code is compiled in.

### Format change and lock-free ring design

To detect CEF media format changes (e.g. 44.1 kHz → 48 kHz), the audio ring header carries an `mFormatSerial`. The writer (`media_plugin_cef`) advances sample rate / channel count / serial when a new stream starts. The reader (`LLMediaAudioStream::update()`) compares against the values it captured when creating the current FMOD channel and, on mismatch, `stop()`s and re-`start()`s so the FMOD sound is rebuilt with the new format. This avoids reading PCM through a stale channel after a format switch.

The ring is **single-writer / single-reader**: the writer (CEF audio callback thread) release-stores `mWriteFrame`, and the reader (FMOD callback thread) release-stores `mReadFrame`. Because the writer never moves the reader's pointer, there's no race where the read pointer shifts mid-calculation. When the ring is full, the writer drops the incoming frame and increments `mTotalFramesDropped`.

### macOS VolumeCatcher's role

The `mac_volume_catcher.cpp` (CoreServices / AudioUnit) implementation is kept **solely to mute the CEF native output**. The primary Media volume control path is the FMOD channel side. If we left the file as `mac_volume_catcher_null.cpp` (no-op), CEF's native output would keep playing and you'd hear MOAP audio twice (CEF + FMOD).

We did not revert to the legacy QuickTime-based version — QuickTime framework is impractical for current macOS SDKs and Apple Silicon arm64 builds. If Dullahan / CEF ever stops opening a native audio output (or gains an "audio-callback only" mode), the VolumeCatcher dependency can be removed.

### Settings

**Intentionally none.** No viewer-side cvar or UI is added.

- The switch is build-time only (`LL_DULLAHAN_AUDIO_CALLBACK`)
- No runtime switch (different installable, different Dullahan API surface)
- User-facing controls remain the existing Media volume / mute sliders

### Known limitations

- **Windows / Linux real-device verification**: macOS is the only OS where the ON mode has been verified on real hardware at the time of the r24 push. Windows / Linux ON-mode build and playback verification are tracked for post-release follow-up.
- **libVLC direct media / `.mp3` `.mp4` URLs / Linux GStreamer**: out of scope. Only the CEF/MOAP path moves to FMOD 2D.
- **3D stream / parcel music / Voice**: unchanged — their existing paths stand.
- **Long-session playback**: short-term `ring_dropped=0` / `frames_silenced=0` confirmed; multi-hour continuous tests are not yet done.

### Implementation summary

- New files: `indra/llaudio/llmediaaudiostream.cpp/.h` (FMOD 2D OPENUSER stream), `indra/llcommon/llpluginaudio.h` (ring header struct)
- Changed files: `llpluginclassmedia.cpp/.h` (`ensureAudioSharedMemory()` / `getAudioData()`), `media_plugin_cef.cpp` (callback registration, `audio_shm_set` message, `writeAudioPacketToRing`), `llviewermedia.cpp/.h` (`mMediaAudioStream` member), `mac_volume_catcher.cpp` (CEF native output mute)
- `autobuild.xml`: two installables side-by-side (`dullahan` / `dullahan_aya_audio`)
- `indra/CMakeLists.txt` / `indra/cmake/CEFPlugin.cmake`: flag-driven branching
- macOS ON-mode build + playback verification PASS; Linux / Windows ON mode to be verified post-release

### Credits

The r24 implementation (Dullahan fork audio callback, viewer-side FMOD 2D wiring, shared memory ring, macOS VolumeCatcher strategy) is by [t-noami](https://github.com/t-noami). On the AYAstorm side, the `LL_DULLAHAN_AUDIO_CALLBACK` build-time fallback switch was added on top so AYAstorm can still build against upstream `secondlife/dullahan` if the fork is unavailable.

### Documentation

- Full r24 spec / maintainer review responses / Dullahan package details / Windows / Linux verification steps: `docs/ayastorm-r24-moap-audio-to-fmod-2d.md`
- Dullahan fork release: `https://github.com/t-noami/dullahan/releases/tag/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3`
