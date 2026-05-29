# 3D Stream — User Guide

> **Language / 言語 / 语言**: **English** · [日本語](./3dstream-user-guide.ja.md) · [中文](./3dstream-user-guide.zh.md)

**Available in**: AYAstorm r31 and later. In r32 and later, 3D Stream is disabled by default.

**Audience**: Streamers, DJs, live venue owners, and builders of exhibition, cinema, or event spaces.

**Reference**: For the full key list and detailed tag behavior, see the [3D Stream Tag Format Guide](../guides/3dstream-tag-guide.en.md).

---

## 1. What Is 3D Stream?

3D Stream uses prims as speakers and plays an HTTP audio stream or Media-on-a-Prim (MOAP) audio source as positional audio in the 3D world.

Normal parcel music is heard at the same level regardless of where the listener stands. With 3D Stream, the sound comes from the speaker prim position: it gets louder when the listener approaches and quieter when the listener moves away.

Typical uses:

- Left/right speakers for a live venue
- Multi-point speaker layouts for cinemas and exhibition spaces
- Spatial placement of a 5.1ch source
- Routing MOAP screen audio to the screen position or to venue speakers

## 2. Before Playback: Enable and Allow

3D Stream is disabled by default. To use it, enable it from the status bar 3D Stream button, the 3D Stream checkbox in the volume popup, or the 3D Stream control in **Preferences > Sound**.

When a 3D Stream tag uses `{url:...}`, the viewer shows a confirmation dialog before the first playback from that URL. Playback starts only if the user allows it. If the user denies it, that URL does not play during the same viewer session, and the viewer does not repeatedly ask for the same URL.

URL sources can use `http://` and `https://`. Other schemes are not played.

`{source:media}` routes audio from a MOAP / media face in the same linkset into 3D Stream. It does not make 3D Stream open a new URL directly, so it follows the normal media display and playback permission flow.

## 3. Minimal Setup: One Prim

Write the following in the prim **Description**:

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

In this setup, that prim itself becomes the speaker. If the source is stereo, a single prim plays it downmixed to mono.

To adjust distance attenuation:

```text
[3dstream:{url:http://example.com/stream.mp3}{min:2}{max:40}]
```

`min` is the near distance where volume remains 100%. `max` is the far distance where volume reaches 0%.

## 4. Place Left/Right Speakers

Link two or more prims and assign a role to the root and child prims.

Root Description:

```text
[3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
```

Child prim Description:

```text
[3dstream:{ch:R}]
```

The root becomes the left speaker, and the child prim becomes the right speaker. Stereo localization is based on the actual prim positions, not link numbers.

You can assign the same channel to more than one speaker. For example, if multiple prims have `{ch:L}`, all of them play the L channel.

## 5. 5.1ch / Multi-speaker Layouts

For a 5.1ch source, place prims for each channel.

Root Description:

```text
[3dstream:{url:http://example.com/live_5_1.opus}{range:30}]
```

Speaker prims:

```text
FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```

Vorbis 6ch and Opus 6ch can be used for 5.1ch streaming. For live streaming, Opus 6ch is a practical route. Musicians and DJs who want to stream Opus 6ch can use [SurroundStreamer](https://github.com/t-noami/SurroundStreamer), created by t-noami.

To expand a stereo stream to a 6-speaker layout, add `{upmix:on}` to the root tag.

```text
[3dstream:{url:http://example.com/stereo.ogg}{upmix:on}{range:30}]
```

If `{upmix:on}` is attached to a 5.1ch source, the viewer detects the 6ch source and automatically bypasses upmixing.

## 6. Use MOAP / Media Audio as a 3D Stream Source

Instead of an HTTP URL, you can use a media face inside the same linkset as the audio source.

Root Description:

```text
[3dstream:{source:media}{ch:L}]
```

Child prim Description:

```text
[3dstream:{ch:R}]
```

If the linkset has multiple media faces, choose the target face on the root tag with `{link:N}` / `{face:N}`.

```text
[3dstream:{source:media}{link:3}{face:2}{range:30}]
```

`{url:...}` and `{source:media}` cannot be used together in the same 3D Stream source declaration. If you want to show a media screen while positioning a separate URL stream in 3D, keep the 3D Stream tag as `{url:...}` and let the media face behave as normal MOAP.

## 7. Sound Adjustments

Common controls:

| Setting | Use |
|---|---|
| `{range:N}` | Speaker reach distance |
| `{volume:N}` | Per-speaker volume, from `0.0` to `1.0` |
| `{bin:on}` | Enable headphone-oriented localization correction |
| `{v:NAME}` | Select a venue reverb preset |
| `{wg:N}` | Wet level for the reverb component |
| `{upmix:on}` | Expand a stereo source to a 5.1ch speaker layout |

If omitted, `binaural` is `off` and `venue` is `dry`. Start by checking the speaker layout without extra processing, then add `{bin:on}` or `{v:...}` only when needed.

Venue reverb is easier to tune when the streamed source itself does not already contain heavy reverb.

## 8. Behavior in Other Viewers

3D Stream tags are AYAstorm-specific. Mainline Firestorm, the official Second Life Viewer, Catznip, and other viewers do not interpret them as 3D Stream.

- 3D Stream audio from `{url:...}` does not play in other viewers
- With `{source:media}`, the media face itself still displays and plays as normal MOAP
- `{ch:...}` routing, upmix, binaural, and venue reverb apply only for AYAstorm users
- Parcel music, if configured, remains audible in other viewers as usual

## 9. Troubleshooting

| Symptom | Check |
|---|---|
| No sound | Make sure 3D Stream is enabled in Preferences and the volume is not zero |
| A URL stream confirmation appears | A new `{url:...}` source must be allowed before playback |
| Only one side plays | Make sure both `{ch:L}` and `{ch:R}` exist in the linkset |
| `{source:media}` does not play | Make sure the media face is in the same linkset, and specify `{link}` / `{face}` if needed |
| Some 5.1ch channels are silent | Make sure corresponding speaker prims such as `{ch:FL}` exist |
| Sound remains after removing the tag | Description re-evaluation normally happens within 30 seconds. If needed, turn `Stream3DEnabled` off once |
| Users in other viewers cannot hear it | 3D Stream is AYAstorm-specific and separate from MOAP or parcel music |

While checking a layout, enable **Preferences > Sound > Show channel routing diagnostics in chat** to see in Local Chat which prim is playing which channel.

## 10. Detailed References

- Full key list and error messages: [3D Stream Tag Format Guide](../guides/3dstream-tag-guide.en.md)
- Opus 6ch streaming tool: [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)
