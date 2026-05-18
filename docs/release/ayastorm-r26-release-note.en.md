# AYAstorm r26 — Release Announcement

GitHub release page copy. **r26 routes MOAP (Media-on-a-Prim) audio through 3D Stream's speaker routing** — the existing `{ch:L}/{ch:R}/{ch:FL}/...` distributed-stereo / 5.1 placement / HRTF / venue reverb / occlusion pipeline now works when the source is a MOAP face instead of an HTTP URL.

Implementation strategy, current-spec breakdown, wiring design, and validation plan all live in the permanent doc (`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`). This note is the entry point and diff highlight.

---

## AYAstorm r26 — MOAP audio connected to 3D Stream

### Headline: let MOAP audio play out of 3D Stream speaker prims

Until now AYAstorm's 3D Stream only accepted HTTP audio streams (Icecast / SHOUTcast) as URL sources. MOAP audio went through a separate path (`LLMediaAudioStream` driving 2D FMOD playback) and never touched 3D Stream's speaker-prim routing, 5.1 routing, HRTF, venue reverb, occlusion, or volume control.

r26 adds a **PCM ring source that sits alongside the URL source** in `LLPositionalStreamMulti`. The float PCM that CEF / Dullahan's audio callback writes into shared memory is now consumable as a 3D Stream input. The MOAP video still displays on its face, but the audio plays from the linkset's speaker prims — and the streamer composes that just by writing tags.

### How it works

Put `{source:media}` (2ch) or `{source:media-5-1}` (6ch) in the root prim's Description. The speaker prims keep their existing `{ch:L}/{ch:R}/{ch:FL}/{ch:FR}/{ch:C}/{ch:LFE}/{ch:SL}/{ch:SR}` tags.

```
Root Description:
  [3dstream-stereo:{source:media}{ch:L}]

Child Description:
  [3dstream-stereo:{ch:R}]
```

```
MOAP video → CEF/Dullahan audio callback
  → LLPluginAudioRingHeader (shared-memory PCM ring, magic="AYAA" v2)
  → LLViewerMediaImpl::getAudioRingForStream3D() ── LL_DULLAHAN_AUDIO_CALLBACK gate
  → LLPositionalStreamMulti (SourceKind::MediaRing)
     ├─ ring read → existing LLMultiTailRing
     └─ per-speaker FMOD_OPENUSER | FMOD_3D mono sound + routing matrix
  → Existing HRTF / venue reverb / occlusion / volume / 5.1 routing apply unchanged
```

- The callback bus's actual channel count (CEF always delivers 8ch) and the **logical source channel count** (`{source:media}`=2 / `{source:media-5-1}`=6) are decoupled. `{upmix:on}` against a stereo MOAP still does the right thing
- If the linkset has multiple MOAP faces, the root tag uses `{link:N}{face:M}` to pick which face to bind. Single-face linksets need no `{link}` / `{face}`
- `{url}` and `{source:media}` are mutually exclusive in the same linkset. You can still display media while 3D-routing a URL — in that case the MOAP audio plays as ordinary 2D media
- Crash safety on plugin teardown: `destroyMediaSource()` → `onMediaSourceDestroying()` → `setMediaRingFor3DStream(nullptr)` → `stopDecodeThread()` (join) → ring pointer swap, in that order. The decode worker is guaranteed to be gone before the shared memory is unmapped
- The ring header's magic/version is re-validated on every pump iteration. Format changes → reopen, ring disappears → setFailed
- The `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch added in r24 (upstream vs fork dullahan toggle) stays intact. Builds with it OFF retain the URL path; the media path is simply disabled

### Settings

**No viewer-side configuration required for normal use.** The streamer composes tags; viewers just need r26. The `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` / `{link:N}` / `{face:M}` syntax is documented in the [3D Stream tag guide §6.10](../guides/3dstream-tag-guide.en.md#610-media--moap-source-r26).

Volume policy depends on how many MOAP faces the linkset has:

- **Single MOAP face**: that media's own volume / mute acts as the source gain (single-media equivalent)
- **Multiple MOAP faces**: the face that's routed to 3D is treated as source gain 1.0 and controlled by 3D Stream's master / per-speaker volume. The non-routed media faces keep playing at their normal 2D media volume

### Migration note

- **Streamers**: rewrite the root Description as `[3dstream-stereo:{source:media}{ch:L}]`. Add `{link:N}{face:M}` only if the linkset has more than one MOAP face
- **Listeners**: nothing to do. Launch r26, walk up to a tagged linkset, and MOAP audio comes out of the 3D Stream speaker prims. No UI changes
- **Streamer-led model is preserved**: the streamer's tags are the source of truth and the listener side has no UI to configure, exactly as it has been since r11

### Known limitations

- **7.1 (8ch) speaker routing is not exposed**: the CEF / Dullahan callback bus is wired as 8ch internally, but the only tag values exposed are `{source:media}` (2ch) and `{source:media-5-1}` (6ch). `BL`/`BR` enums and the routing code paths are in place for forward-compat; 7.1 routing is a r27+ task
- **A/V sync**: prebuffer ~43 ms (2048 frames @ 48 kHz), target buffered ~85 ms (4096 frames). Drift against MOAP video is expected to offset against video-decoder latency on the MOAP side; no tuning knob is exposed
- **Mid-stream MOAP page transitions**: if the MOAP page changes and the audio format (sample_rate / channels) flips, the ring is reopened. The pump loop detects format changes and reopens, but a brief audio glitch can occur
- **Linux / macOS / Windows runtime verification**: AYAstorm side verifies the minimal stereo MOAP setup (root + child) on Linux right after merge. macOS / Windows verification happens when the next Release binaries are cut

### Implementation summary

- `indra/llcommon/llpluginaudio.h` — new ring header (`LLPluginAudioRingHeader`, magic `0x41594141` "AYAA" / version 2 / max 8ch, `ll_plugin_audio_ring_supported_3d_channel_count()` accepts 1/2/6/8 ch)
- `indra/llcommon/tests/llpluginaudio_test.cpp` — new unit test (ring sizing math / accepted channel counts / 6ch & 8ch enum ordering)
- `indra/llaudio/llpositionalstreammulti.{h,cpp}` — `SourceKind {Url, MediaRing}` added, plus `startMedia()` / `setMediaRingFor3DStream()` / `pumpMediaRingSource()` and CEF 7.1 channel remap (`kCef71ToStream8 = {0,1,2,3,6,7,4,5}`). `releaseSpeakerRuntime()` extracted (no behaviour change)
- `indra/newview/llpositionalstreammgr.{h,cpp}` — `DistSourceKind::{Url, Media}` / `SourceBindingKey` / tag parser learns `{source}` / `{link}` / `{face}`. New `onMediaSourceDestroying()` / `evaluateLinkset()` / `findMediaFor3DSource()` / `effectiveDistributedStreamVolume()`. `BL`/`BR` added to `ChannelKind`
- `indra/newview/llviewermedia.{h,cpp}` — new `getAudioRingForStream3D()` / `getStream3DAudioGain()` / `setStream3DAudioRedirected()` (all gated by `LL_DULLAHAN_AUDIO_CALLBACK`). `destroyMediaSource()` now calls `LLPositionalStreamMgr::onMediaSourceDestroying()` for lifecycle safety
- `indra/llplugin/llpluginclassmedia.cpp` — `getAudioData()` gains an `mPlugin->isRunning()` guard, new `audio_stream_format` plugin message handler
- `indra/media_plugins/cef/media_plugin_cef.cpp` — emits `audio_stream_format` (sample_rate / channels / max_channels) on start / stop / error
- `autobuild.xml` — Dullahan bumped to `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.4` (picks up the 48kHz / 8ch `GetAudioParameters` override)
- `indra/cmake/FMODSTUDIO.cmake` — exposes libopus on the `ll::fmodstudio` interface so manual-FMOD builds don't lose the AYAstorm FMOD codec plugin's libopus symbols
- `indra/newview/CMakeLists.txt` — re-links `ll::fmodstudio` after llaudio's static archive so the AYAstorm FMOD codec symbols resolve in universal macOS builds
- `docs/guides/3dstream-tag-guide.{ja,en,zh}.md` — new "Media / MOAP source (r26)" section (§6.7 in ja, §6.9 in zh, §6.10 in en), §6.3 key table extended with `{source}` / `{link}` / `{face}`, mutual exclusion with `{url}` and the single-media vs multi-media volume policy spelled out
- `docs/ayastorm-r26-moap-3d-stream-implementation-plan.md` — new permanent doc (strategy / current spec / wiring design / implementation phases / validation plan / known issues)

### Credits

The r26 implementation (plan doc / `LLPluginAudioRingHeader` design / `MediaRing` source addition to `LLPositionalStreamMulti` / CEF plugin `audio_stream_format` emission / `LLViewerMediaImpl` 3D Stream wiring / 3-language tag guide updates / unit tests / Dullahan callback.4 bump / FMOD codec link fix) is by [t-noami](https://github.com/t-noami).

On the AYAstorm side we landed the PR as-is and added only this release note (3 languages).

### Documentation

- r26 plan doc / current-spec breakdown / wiring design / implementation phases / validation plan / open issues: [`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`](../ayastorm-r26-moap-3d-stream-implementation-plan.md)
- 3D Stream tag guide (`{source:media}` usage is in §6.10): [`docs/guides/3dstream-tag-guide.en.md`](../guides/3dstream-tag-guide.en.md)
- r24 `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch (upstream vs fork dullahan toggle): [`docs/release/ayastorm-r24-release-note.en.md`](./ayastorm-r24-release-note.en.md)
