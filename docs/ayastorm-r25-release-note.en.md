# AYAstorm r25 — Release Announcement

GitHub release page copy. **r25 fixes parcel music Ogg Vorbis live stream playback that has been broken since AYAstorm r10.x-bugfix-1** — Ogg Vorbis Icecast live streams now play correctly on AYAstorm while preserving full Ogg Opus support.

Implementation details, investigation logs, and verification URLs live in the permanent doc (`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`). This note is the entry point and diff highlight.

---

## AYAstorm r25 — Parcel music Ogg Vorbis live stream playback fix

### Headline: restoring Ogg Vorbis parcel music that AYAstorm itself had broken

AYAstorm shipped a custom FMOD codec plugin (for Ogg Opus playback) in the r9-opus series. r10.x-bugfix-1 changed that codec's registration priority to `0` (FMOD highest) and added a 4-byte "OggS" capture probe that **rejects non-Ogg streams (MP3 etc.) early**, keeping built-in HTTP codecs intact.

However, this design had a blind spot for Ogg Vorbis live streams. A Vorbis stream passes the `OggS` 4-byte gate, the custom codec reads the first Ogg packet, sees `\x01vorbis` instead of `OpusHead`, and returns `FMOD_ERR_FORMAT`. FMOD then tries to fall back to the built-in Vorbis codec — but an HTTP live stream cannot be rewound, so the built-in Vorbis path fails with `FMOD_ERR_FILE_COULDNOTSEEK`. As a result, **Ogg Vorbis parcel music was unplayable on AYAstorm from r10.x-bugfix-1 through r24** (vanilla Firestorm was unaffected because it had no custom codec ahead of the built-in Vorbis decoder).

r25 extends the custom FMOD codec itself to handle **both Ogg Opus and Ogg Vorbis**, so the fallback path is never taken.

### How it works

The custom codec reads the first Ogg packet once and branches on its content:

```
parcel music URL
  → LLStreamingAudio_FMODSTUDIO
  → FMOD::System::createStream(url)
  → AYAstorm Ogg Opus/Vorbis codec (priority 0)
     ├─ first packet "OpusHead"  → opus_decoder / opus_multistream_decoder (existing path)
     └─ first packet "\x01vorbis" → libvorbis decoder (new path)
  → PCMFLOAT fed into the FMOD mixer
```

- Non-Ogg streams (MP3 etc.) are still rejected by the 4-byte `OggS` gate; built-in codecs handle them unchanged
- Vorbis streams are **decoded inside the custom codec end-to-end**, so the fallback-induced seek failure never occurs
- The existing Opus paths for mapping family 0 (mono/stereo) and family 1 (multistream 5.1ch surround) are untouched

### Settings

**No user action required for normal use.** Two diagnostic cvars are added:

| Cvar | Default | Purpose |
|---|---|---|
| `AYAOpusCodecEnable` | `1` | When OFF, the custom codec is not registered at all — only built-in codecs run. Opus streams become unplayable in this mode; diagnostic only |
| `AYAOpusCodecPriority` | `0` (highest) | FMOD codec dispatch priority. Any value other than `0` can break Opus/Vorbis playback via codec-order conflicts; do not touch outside diagnostics |

Both require a viewer restart to take effect. **Leave defaults as-is for normal operation.**

### Migration note

- No streamer-side action required. Icecast Ogg Vorbis streams now play on AYAstorm listeners as-is
- No listener-side action required. Just launch r25 and Ogg Vorbis parcel music works again
- The expected outcome is "Vorbis streams that worked on vanilla Firestorm / other viewers now also work on AYAstorm" — restoring correct behaviour

### Known limitations

- **Chained Ogg / serial change**: a single HTTP stream switching to a different Ogg logical stream (serial number change) mid-playback is not handled. Rare in normal Icecast deployments but can occur in long-running broadcasts. Automatic re-initialization on serial change is deferred to r26+
- **Other non-Vorbis Ogg family codecs** (Theora / Speex etc.): intentionally not absorbed by this codec. We do not adopt a "swallow any Ogg" design (misidentification risk)
- **macOS / Windows runtime verification**: the PR author verified on macOS arm64, AYAstorm side verified on Linux; macOS/Windows Release binary verification will be performed when the tag is cut

### Implementation summary

- `indra/llaudio/fmod_codec_ogg.{cpp,h}` — Ogg Opus/Vorbis codec (renamed from `fmod_codec_opus.{cpp,h}` after PR #75)
- `indra/llaudio/llaudioengine_fmodstudio.cpp` — single codec registration as "AYAstorm Ogg Opus/Vorbis codec"
- `indra/llaudio/llpositionalstreammulti.cpp` — 3D Stream path promotes plugin codec name `"Ogg Vorbis"` to `FMOD_SOUND_TYPE_OGGVORBIS`
- `indra/newview/app_settings/settings.xml` — two diagnostic cvars added
- Existing Opus paths (mono/stereo/multistream 5.1ch) are untouched; 5.1ch surround Opus source playback verified

### Credits

The r25 implementation (Ogg Vorbis decoder integration, Opus preservation, fallback-path-avoiding design, investigation write-up) is by [t-noami](https://github.com/t-noami). On the AYAstorm side we landed the file / function / header-guard rename cleanup (`fmod_codec_opus` → `fmod_codec_ogg`, `FMODGetCodecDescriptionOpus()` → `FMODGetCodecDescriptionOgg()`).

### Documentation

- r25 full investigation log / verification URLs / failure-hypothesis breakdown / design notes / 6ch multichannel handling: [`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`](./ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- Prior Opus codec work (r9-opus series): [`doc/spec_5_1ch_opus_decode.md`](../doc/spec_5_1ch_opus_decode.md)
