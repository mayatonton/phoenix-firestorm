# 3D Stream Tag Format Guide

> Tag format reference for AYAstorm's **3D Stream** feature, which plays HTTP audio streams or Media-on-a-Prim (MOAP) audio as 3D-positional audio from prims.
>
> This document reflects the final specification as of AYAstorm `r26`. It includes r12 features (binaural / venue reverb / stereo→5.1 upmix / tag short-forms) and the r26 media/MOAP source routing extension.

---

## Table of Contents

1. [What is 3D Stream?](#1-what-is-3d-stream)
2. [Quick Start](#2-quick-start)
3. [Terminology](#3-terminology)
4. [Tag Overview](#4-tag-overview)
5. [Mono Tag `[3dstream:...]`](#5-mono-tag-3dstream)
6. [Distributed Stereo / Venue Placement Tag `[3dstream-stereo:...]`](#6-distributed-stereo--venue-placement-tag-3dstream-stereo)
7. [Binaural / Venue Reverb (r12)](#7-binaural--venue-reverb-r12)
8. [stereo→5.1 upmix (r12)](#8-stereo51-upmix-r12)
9. [`ch` (Channel) Value Reference](#9-ch-channel-value-reference)
10. [Source Channel Count × Tag Value Compatibility Matrix](#10-source-channel-count--tag-value-compatibility-matrix)
11. [Streaming Side (Building Source URLs)](#11-streaming-side-building-source-urls)
12. [Viewer-side Settings](#12-viewer-side-settings)
13. [Error Notifications / Diagnostics](#13-error-notifications--diagnostics)
14. [Troubleshooting](#14-troubleshooting)
15. [Known Limitations / Specification Notes](#15-known-limitations--specification-notes)
16. [Static Occlusion `[ayastorm:occlude]` (r13)](#16-static-occlusion-ayastormocclude-r13)
17. [Related Documents / Internal Specifications](#17-related-documents--internal-specifications)

---

## 1. What is 3D Stream?

The standard Second Life Viewer plays HTTP audio streams (SHOUTcast / Icecast) as **parcel-level BGM** in 2D only — it has no information about where in 3D space the sound is coming from.

AYAstorm's **3D Stream** feature treats prims (objects) as "speakers" and plays the stream **as if the audio is emitted from that prim's location** with full 3D positional rendering. As the listener (camera or avatar) moves, sound direction and distance attenuation update in real time.

Primary use cases:

- **Live venue PA**: Place speaker prims in front of a stage so that the broadcast audio plays from those positions
- **Ambient sound**: Play matching audio from objects like rivers, jukeboxes, TVs
- **Stereo placement / multi-speaker venues**: Assign L / R / mono to multiple prims to spread stereo across space
- **5.1ch source venue deployment**: Place the six 5.1ch channels (FL / FR / C / LFE / SL / SR) on six prims
- **Media / MOAP speaker object**: Route the sound of a media face, such as a web player or YouTube page, into 3D Stream speakers

Everything is configured by **writing a tag into a prim's Description field** — no LSL script, no SL server-side change. Only AYAstorm users hear the 3D audio. Other Viewers (mainline Firestorm, official LL Viewer, etc.) ignore these tags, so there is no compatibility problem.

---

## 2. Quick Start

### 2.1 Single prim playing audio (simplest example)

Create any prim and write the following into its **Description field**:

```
[3dstream:{url:http://example.com/stream.mp3}]
```

That's all you need. The stream `http://example.com/stream.mp3` will be played as 3D-positional audio from that prim's location. Move away and the volume drops; move sideways and the panning naturally follows.

### 2.2 Stereo placement (split L / R into separate prims)

Split a stereo source between two prims to spread stereo across space.

1. Link a root prim and a child prim (Ctrl+L)
2. **Root Description**:
   ```
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **Add to root Description** (root itself becomes the L speaker):
   ```
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child Description**:
   ```
   [3dstream-stereo:{ch:R}]
   ```

Now the root plays L and the child plays R.

### 2.3 More details

Read sections 3 onward. Multi-speaker, 5.1ch, fine-tuning, and broadcaster-side recipes are explained there.

---

## 3. Terminology

| Term | Meaning |
|---|---|
| **Stream** | Audio data delivered over HTTP (SHOUTcast / Icecast / static file, etc.). Main supported codecs: MP3 / Vorbis / Opus / FLAC |
| **Linkset** | A group of prims linked together via SL's "link" operation (Ctrl+L). One root + N child prims |
| **Root prim** | The parent prim of a linkset. Selected first when "Edit linked" is OFF in Build → Edit |
| **Child prim** | Any prim in the linkset other than the root |
| **Source declaration** | A root prim tag containing either `{url:...}` or `{source:media...}`. Declares what audio source the linkset plays. Source declarations are **root-only** |
| **Media source** | A media/MOAP face in the same linkset used as the 3D Stream audio source via `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` |
| **Speaker prim** | A prim with a tag containing `{ch:...}`. Actually emits sound. **Can be either root or child** |
| **binding** | The internal "source → speaker group" mapping built per linkset. 1 linkset = 1 binding |
| **ch (channel)** | The audio channel a speaker prim is responsible for. `L` / `R` / `M` (mono), or 5.1ch values `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff** | Distance attenuation. Volume decreases as the listener moves away from the speaker |

---

## 4. Tag Overview

### 4.1 Three tag types

| Tag | Prefix | Purpose |
|---|---|---|
| **Mono tag** | `[3dstream:...]` | Play one stream from a single prim (minimal config) |
| **Distributed stereo / venue tag** | `[3dstream-stereo:...]` | Synchronize one stream across multiple prims of a linkset (stereo / multi-speaker / 5.1ch) |
| **Static occlusion tag** (added in r13) | `[ayastorm:occlude]` | Mark wall / door / floor / ceiling prims as sound-blocking geometry (venue operator / builder, see §16) |

### 4.2 Aliases for legacy prefixes

Both tags also accept legacy prefixes (`[ayastream:...]` / `[ayastream-stereo:...]`) as **permanent aliases**. When `ayastream` was renamed to `3dstream` in r5 (2026-05), these aliases were kept so that prims placed before the rename do not need to be re-edited. **`3dstream` is recommended for new content**, but mixing is fine.

```
[3dstream:{url:...}]              ← recommended (canonical)
[ayastream:{url:...}]             ← legacy, accepted

[3dstream-stereo:{url:...}{ch:L}] ← recommended (canonical)
[ayastream-stereo:{ch:L}]         ← legacy, accepted
```

### 4.3 Common syntax rules

- **The tag may appear anywhere in the Description.** Surrounding text is ignored (e.g., `Shop name [3dstream:{url:...}] open` mixed with descriptive text is fine).
- Fields are a set of `{key:value}` items. No separator is required between fields (whitespace, commas, or no separator all work).
- **Key names are case-insensitive** (normalized to lowercase internally). `{URL:...}` and `{url:...}` are equivalent.
- **Whitespace around values is trimmed.** `{ url : http://example/  }` is fine.
- **Unknown keys are silently ignored.** For example, `{foo:bar}` has no effect but does not produce an error.
- If multiple tags of the same kind appear in one prim's Description, **only the first one is used.**

### 4.4 SL Description limit (127 bytes)

The Description writable via LSL `llSetObjectDesc` has a **127-byte limit**. UTF-8 multibyte characters (e.g., Japanese) consume this quickly, so for long URLs either **shorten the URL** or use the distributed pattern (write `{url}` only on the root and `{ch:...}` only on child prims; see §6). **Short-forms are available for frequent keys and venue values** (see §4.5).

### 4.5 Short-forms for key names / venue values (r12 / r12.1)

For `[3dstream-stereo:...]`, **4 frequent keys** and **all 9 `venue` values** have **short-form aliases** introduced in r12. They make the Description fit comfortably under SL's 127-byte limit (§4.4). Long-form and short-form are **fully equivalent** (resolved to the same canonical form internally). New tags and existing tags can use either form, and behavior is identical.

#### Key short-forms

| Long form | Short form | Meaning |
|---|---|---|
| `binaural` | `bin` | Binaural ON/OFF (§7.1) |
| `venue` | `v` | Venue reverb preset (§7.2) |
| `wetgain` | `wg` | Reverb wet level (§7.3) |
| `lfegain` | `lg` | LFE channel gain multiplier (§7.4, added in r12.1) |

Other keys (`url`, `source`, `link`, `face`, `ch`, `range`, `volume`, `min`, `max`, `upmix`) are already short, so no aliases are added.

#### `venue` value short-forms

| Long form | Short form | RT60 (approx.) |
|---|---|---|
| `dry` | `d` | — (no reverb) |
| `room_small` | `rs` | 0.3 s |
| `room_medium` | `rm` | 0.6 s |
| `hall_small` | `hs` | 1.0 s |
| `hall_medium` | `hm` | 1.5 s |
| `hall_large` | `hl` | 2.0 s |
| `club` | `cl` | 0.8 s |
| `cathedral` | `ct` | 3.0 s |
| `outdoor` | `od` | 0.2 s |

#### Byte count example

Long form (133 bytes — over the 127-byte limit):

```
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{binaural:on}{venue:hall_medium}{wetgain:1.2}]
```

Short form (110 bytes — fits, 23 bytes saved):

```
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{bin:on}{v:hm}{wg:1.2}]
```

#### LSL helper behavior

The bundled LSL `aya_3dstream_setup.lsl` (see §17) **accepts both forms on input** and **always writes short-form on output (Description writes)**. Descriptions configured via the LSL dialog are automatically converted to short-form.

#### Notes

- Key names are **case-insensitive** (per §4.3 common rule). `{BIN:on}`, `{bin:on}`, and `{binaural:on}` are all equivalent.
- If both forms are written in the same tag (e.g., `{binaural:on}{bin:off}`), only the **first occurrence** is used (§4.3 common rule).
- The route diagnostic chat output and error messages always use **canonical (long) names** to keep the wording stable.

### 4.6 Tag activation timing

- AYAstorm **polls in-range prim Descriptions every 30 seconds** (`Stream3DPollInterval` setting).
- When you modify Description via LSL `llSetObjectDesc`, the next poll re-evaluates and applies the change (typically within 5–30 seconds).
- When you manually right-click → Edit → Description, the edit triggers immediate re-evaluation (via Properties broadcast).
- Linking / unlinking also triggers re-evaluation.

---

## 5. Mono Tag `[3dstream:...]`

### 5.1 Syntax

```
[3dstream:{url:URL}{min:N}{max:N}]
```

Or with the legacy prefix:

```
[ayastream:{url:URL}{min:N}{max:N}]
```

### 5.2 Key reference

| Key | Required | Type | Default | Meaning |
|---|---|---|---|---|
| `url` | **required** | string | — | Stream URL (`http://` / `https://`). Empty string is an error |
| `min` | optional | F32 (m) | `Stream3DRolloffMin` (1.0) | **Near distance** for rolloff (volume = 100% within this distance) |
| `max` | optional | F32 (m) | `Stream3DRolloffMax` (20.0) | **Far distance** for rolloff (volume = 0% beyond this distance) |

The rolloff model is FMOD's `FMOD_3D_LINEARSQUAREROLLOFF` (linear-square rolloff). It attenuates smoothly between `min` and `max`.

### 5.3 Behavior

- The tag may be written on **any prim** of the linkset (root or child). The prim with the tag itself acts as the speaker.
- A stereo source is **internally mixed down to mono** (L+R average).
- If the same linkset also has `[3dstream-stereo:...]`, the mono tag is NOT given priority for that prim's speaker assignment — the two binding paths are evaluated independently. Using the same prim for both is not recommended (behavior undefined).

### 5.4 Examples

#### 5.4.1 Minimum

```
[3dstream:{url:http://example.com/radio.mp3}]
```

`min` / `max` are omitted, so the setting defaults (1m / 20m) are used.

#### 5.4.2 Custom distance

```
[3dstream:{url:http://example.com/radio.mp3}{min:2}{max:50}]
```

Volume is at maximum within 2m and fades to silence at 50m. Use this for large outdoor fields where you want sound to carry.

#### 5.4.3 Mixed with descriptive text

```
Shop BGM [3dstream:{url:http://radio.example.jp/8000/jazz}] enjoy
```

Surrounding text is fine.

---

## 6. Distributed Stereo / Venue Placement Tag `[3dstream-stereo:...]`

### 6.1 Syntax

```
[3dstream-stereo:{url:URL}{range:N}{ch:CH}{volume:V}]
[3dstream-stereo:{source:media}{link:N}{face:N}{range:N}{ch:CH}{volume:V}]
```

Or with the legacy prefix:

```
[ayastream-stereo:...]
```

This tag handles **one audio source across the entire linkset**. The root prim declares which source to play (`{url:...}` for an HTTP stream, or `{source:media}` for a media/MOAP face), and each speaker prim in the linkset declares which channel it is responsible for.

### 6.2 Per-prim role

Each prim takes on a role based on its tag fields:

| Description fields | Role |
|---|---|
| Contains `{url:...}` | **URL source declaration** (root only — `{url}` on a child prim is ignored) |
| Contains `{source:media...}` | **Media/MOAP source declaration** (root only — selects a media face in this linkset) |
| Contains `{ch:...}` | **Speaker** (root or child, both fine) |
| Contains both (= root only) | Source declaration + also acts as speaker |
| Contains neither | Does nothing (not part of the binding) |

Playback only starts when the linkset has both **a source declaration** (root with `{url}` or `{source:media}`) and **at least one speaker** (a prim with `{ch}`). If there are zero speakers, a "structural error" is raised and a notification is shown (§13).

### 6.3 Key reference

#### 6.3.1 Keys meaningful only on the root prim

| Key | Required | Type | Default | Meaning |
|---|---|---|---|---|
| `url` | required for URL source | string | — | HTTP stream URL. Empty string is an error. Mutually exclusive with `{source:media}` |
| `source` | required for media source | enum | — | Source type. `media` / `media-stereo` route a media/MOAP face as a 2ch source. `media-5-1` routes it as a 5.1ch / 6ch source. Mutually exclusive with `{url:...}` |
| `link` | optional | S32 | any media prim | Media source selector. Link number of the prim that owns the media face; only meaningful with `{source:media}` |
| `face` | optional | S32 | any media face | Media source selector. Face number containing media; only meaningful with `{source:media}` |
| `range` | optional | F32 (m) | `Stream3DRolloffMax` (20.0) | Default rolloff distance for speakers in the linkset that don't have their own `range` |
| `binaural` | optional | bool | `off` | Binaural ON/OFF (details §7.1). Short-form `bin` |
| `venue` | optional | enum | `dry` | Venue reverb preset, 9 values (details §7.2). Short-form `v` |
| `wetgain` | optional | F32 [0.0–2.0] | `0.2` | Reverb wet-component gain (details §7.3). Short-form `wg` |
| `lfegain` | optional | F32 [0.0–4.0] | `1.0` | LFE channel gain multiplier (details §7.4, added in r12.1). Short-form `lg` |
| `upmix` | optional | bool | `off` | stereo→5.1 upmix (details §8). No short-form |

#### 6.3.2 Speaker declaration keys (any prim)

| Key | Required | Type | Default | Meaning |
|---|---|---|---|---|
| `ch` | **required** | enum | — | Channel this prim handles (see §9) |
| `range` | optional | F32 (m) | Falls back: speaker `range` → root `range` → `Stream3DRolloffMax` | Per-speaker rolloff distance |
| `volume` | optional | F32 [0.0–1.0] | 1.0 | Per-speaker volume multiplier |

> **Important**: The `min` / `max` keys from the mono tag are **ignored** in the distributed-stereo tag. For distributed stereo the near distance is internally fixed at 1.0m, and the far distance is the `range` key (or default `Stream3DRolloffMax`).

> **Important**: `{url:...}` and `{source:media}` are alternatives. Do not put both in the same root tag. If you want to show media on a face while the speakers play a separate stream, keep `{url:...}` as the source and leave the media face unselected by 3D Stream (§6.10.3).

### 6.4 One root + one child (basic stereo pair)

Smallest stereo placement:

```
Root Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:L}]

Child Description:
  [3dstream-stereo:{ch:R}]
```

The root takes L, the child takes R. The **link order (link number)** of root vs child does NOT affect playback. Spatial positioning is determined by where in space you place each prim.

### 6.5 Multi-speaker (4+ speakers)

The same stereo stream played from four corners of a venue:

```
Root Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]

Child #1 Description:
  [3dstream-stereo:{ch:L}{range:50}]

Child #2 Description:
  [3dstream-stereo:{ch:R}{range:50}]

Child #3 Description:
  [3dstream-stereo:{ch:L}{volume:0.7}]

Child #4 Description:
  [3dstream-stereo:{ch:R}{volume:0.7}]
```

- Root only declares the source; it does not play (no `{ch}`)
- Children #1, #2 carry L/R at 50m range, full volume
- Children #3, #4 carry the same L/R at 70% volume (front-row support speakers)
- The speaker count is capped at `Stream3DStereoMaxSpeakers` (**default 16**, see §12)

### 6.6 5.1ch venue placement (6 prims)

Deploy a 5.1ch source (Opus surround / FLAC 6ch) across six speaker prims:

```
Root Description:
  [3dstream-stereo:{url:http://example.com/test_5_1.flac}{range:30}]

FL prim:  [3dstream-stereo:{ch:FL}]
FR prim:  [3dstream-stereo:{ch:FR}]
C prim:   [3dstream-stereo:{ch:C}]
LFE prim: [3dstream-stereo:{ch:LFE}]
SL prim:  [3dstream-stereo:{ch:SL}]
SR prim:  [3dstream-stereo:{ch:SR}]
```

- Place each prim physically in the venue's "speaker positions" (front L/R of the stage, center, subwoofer, surround L/R)
- LFE is treated equivalently to the other 5 channels — no special processing (no low-pass filter, no 2D-ization). If you need LFE band-limiting, do it on the broadcaster side.
- The listener has no "movie sweet spot" (= SL is free-camera). If they walk around the venue, the intended 5.1 image will of course break. Treat this as **multi-point venue PA, not cinema-style surround reproduction**.

### 6.7 Assigning the same `ch` to multiple prims

If you write `{ch:L}` on two or more prims, both will play the L channel. Useful for putting "front-row L" and "back-row L" speakers in a venue.

Conversely, if no `ch` is written on any prim, you'll get a "0 speakers" structural error.

### 6.8 Root acts as both source and speaker

```
Root Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:M}{range:25}]
```

By writing a source declaration (`{url}` or `{source:media}`) and `{ch}` in the root tag, the root declares the source AND acts as an M (mono) speaker. This works for simple mono setups with no child prims (functionally close to `[3dstream:...]`; `[3dstream:...]` itself is URL-source only).

### 6.9 How to identify the root prim

While editing a linkset, in the Build floater's **Object** tab the "Selected" indicator shows which prim is selected. The parent of the linkset (= root) is normally the **first prim selected when the linkset was originally linked**.

Most reliable confirmation:
- Build → Edit → "Edit linked" OFF → click any prim → the root of that linkset is selected
- LSL: `llGetLinkNumber()` returns `1` for the root (when child prims exist). For a single un-linked prim, it returns `0`.

The link order (link number 1, 2, 3, ...) of root vs children **does NOT affect speaker channel assignment**. The spec from r5 that used link number to determine L/R was retired in r8; from r8 onwards speaker routing is `{ch:...}` declaration based.

In r26, `{link:N}` can be used with `{source:media}` to select which prim's media face becomes the audio source. This link number is **only a media-source selector**; it still does not decide L/R/FL/FR speaker order.

### 6.10 Media / MOAP source (r26)

Use `{source:media}` or `{source:media-5-1}` when the audio should come from a media face in the same linkset instead of from a direct HTTP stream URL. This is intended for MOAP / shared media surfaces such as web players, YouTube pages, or custom HTML players.

The tag still lives on the **root prim**. The media face itself may be on the root prim or on a child prim. Speaker prims continue to use `{ch:...}` exactly like URL-based 3D Stream.

#### 6.10.1 Basic media source

If the linkset has exactly one media face, the root can simply select media:

```
Root Description:
  [3dstream-stereo:{source:media}{range:30}]

FL prim: [3dstream-stereo:{ch:FL}]
FR prim: [3dstream-stereo:{ch:FR}]
C prim:  [3dstream-stereo:{ch:C}]
```

When the media page plays audio, that audio is routed through the 3D Stream speaker prims. During media load, reload, navigation, or an audio-less page, the 3D route stays open and waits silently instead of briefly returning to normal 2D media audio.

#### 6.10.2 Selecting a child prim / face

If the media face is on a child prim, or if the linkset has multiple media faces, specify the media source with `{link:N}` and optionally `{face:N}`:

```
Root Description:
  [3dstream-stereo:{source:media}{link:2}{face:0}{range:30}]

Child link 2, face 0:
  Set Media / MOAP here

Speaker prims:
  [3dstream-stereo:{ch:L}]
  [3dstream-stereo:{ch:R}]
```

Rules:

- `{link:N}` uses the SL link number of the prim containing the media face.
- `{face:N}` uses the face number containing media.
- If `{face:N}` is omitted and the selected prim has exactly one media face, that media face is used.
- If multiple media faces exist and the tag does not identify one unambiguously, 3D Stream reports a structural error.
- `{link:N}` / `{face:N}` only select the media source. Speaker placement still comes from each speaker prim's `{ch:...}`.

#### 6.10.3 Showing media while playing a URL stream

You can still display media on a prim while the 3D Stream speakers play a normal URL stream:

```
Root Description:
  [3dstream-stereo:{url:http://example.com/stream.ogg}{range:30}]

Any prim / face:
  Set Media / MOAP for visuals
```

In this configuration, the speakers play the `{url:...}` stream. The media face is not routed into 3D Stream and keeps the normal viewer media-audio behavior.

#### 6.10.4 Volume behavior for media source

For URL sources, volume works as before:

```
final volume = Stream3DVolumeMaster × {volume:N} × distance attenuation × master audio
```

For media/MOAP sources:

- If the linkset has **one media face**, the normal media volume / mute acts as the source gain for the 3D route. This matches the meaning of media volume in the normal 2D media path.
- If the linkset has **two or more media faces**, the selected media routed into 3D Stream is treated as source gain `1.0`. Adjust it with the 3D Stream master volume and speaker `{volume:N}`. Other, unselected media faces continue to use normal media volume.

This avoids changing the volume of unrelated media faces when one specific media source is used as the 3D Stream input.

#### 6.10.5 Channel count notes

Media source channel selection:

- `{source:media}` / `{source:media-stereo}` treats the media as a 2ch source. Use this for stereo media, including stereo sources that should use `{upmix:on}`.
- `{source:media-5-1}` treats the media as a 5.1ch / 6ch source. Place `FL / FR / C / LFE / SL / SR` speaker prims for this mode.

This guide covers media sources up to **2ch and 5.1ch (6ch)**. Even if the Dullahan/CEF callback bus appears as 8ch, that does not mean 7.1ch speaker routing is implemented in 3D Stream.

---

## 7. Binaural / Venue Reverb (r12)

> Three new keys (`binaural` / `venue` / `wetgain`) are added in r12, plus `lfegain` in r12.1. They apply uniformly to all speakers in the linkset, are meaningful only on the root prim, and produce a more natural sense of "live venue / hall" on top of the existing 3D positional rendering.

#### Why root-only?

These keys describe the **broadcaster's intent for that performance** — "this content is mixed for a hall" / "today's broadcast is binaural" — so they are decided once at the source, not per-speaker. **All four are root-only** (writing them on a child prim is silently ignored). They are NOT exposed in any listener-side UI ─ listeners hear what the broadcaster decided via the tag (§7.5).

### 7.1 `{binaural:on|off}` (short-form `bin`)

Applies a **lite binaural HRTF** to each speaker. With headphones, this enhances localization (front/back / above/below disambiguation).

| Value | Meaning |
|---|---|
| `on` | Apply lite binaural HRTF (recommended for headphone listeners) |
| `off` | Bypass HRTF (vanilla 3D positioning only) |

Default: `off`.

#### What "lite binaural" does

- ITD (interaural time difference) per Woodworth-Schlosberg formula — ear-to-ear arrival-time delay
- HF rolloff per air absorption (−0.5 dB/m, capped at −25 dB) — high-frequency dimming for far sources
- **No** ILD (interaural level difference), **no** spectral cone-of-confusion correction (those are r13+ SOFA territory)

In short, it's a low-cost HRTF using ITD-based spatialization plus distance-based HF rolloff. CPU overhead per speaker: ~+0.4 percentage points (measured on r10 reference hardware).

#### When to use

- **Headphone listeners** = `on`. Spatial cues are much clearer than vanilla 3D.
- **Speaker listeners** = either is fine. ITD on speakers can occasionally feel reversed (mixed for ears, not loudspeakers); see "Exception" in §7.5 for the listener-side rescue.
- **Already-binaural source material** (broadcasting a pre-mixed binaural track) = `off` to avoid double-processing.

### 7.2 `{venue:NAME}` (short-form `v`)

Selects one of 9 **venue reverb presets**. Each preset has fixed RT60, EQ, early-reflection pattern, and CPU cost — pick the one that fits the venue.

| Long form | Short form | RT60 | Use case | CPU (incremental) |
|---|---|---|---|---|
| `dry` | `d` | — | No reverb (= r10 behavior) | 0 (no DSP) |
| `room_small` | `rs` | 0.3 s | Small studio, bedroom | +0.1 pp |
| `room_medium` | `rm` | 0.6 s | Medium studio, talk show | +0.1 pp |
| `hall_small` | `hs` | 1.0 s | Live house, small theater | +0.5 pp |
| `hall_medium` | `hm` | 1.5 s | Concert hall, ballroom | +7.7 pp |
| `hall_large` | `hl` | 2.0 s | Large hall, opera house | +9.6 pp |
| `club` | `cl` | 0.8 s | Dance club, dense early reflections | +0.4 pp |
| `cathedral` | `ct` | 3.0 s | Cathedral, long ambient tail | +10.2 pp |
| `outdoor` | `od` | 0.2 s | Outdoor, very light early reflections only | +0.1 pp |

Default: `dry`.

#### CPU note

`hall_medium` / `hall_large` / `cathedral` (long-tail venues) consume noticeably more CPU than `dry` / `room_*`. Choose based on the venue feel you need — e.g., a live house can use `hall_small` or `club` rather than `hall_large`. The "incremental" column is per-binding overhead (not per-speaker × N); details in `docs/specs/spec_binaural_venue_reverb.md`.

### 7.3 `{wetgain:N}` (short-form `wg`)

Multiplier on the **wet (reverb) component**. Range: 0.0–2.0. Default: **0.2** (changed from 1.0 in r12.1).

| Value | Effect |
|---|---|
| `0.0` | Full dry (= same as `venue:dry` regardless of preset) |
| `0.1` | Very subtle wet |
| **`0.2` (default)** | Subtle wet — musically usable baseline |
| `0.3–0.5` | Moderate to fairly thick wet (top of the musical range) |
| `1.0` and above | Wet at parity-or-higher with dry — typically too saturated for music broadcasts |
| `2.0` | Wet at 200% — heavily-drowned ambient feel (rarely useful) |

The dry component is fixed at 1.0; only wet is scaled by `wetgain`. To go fully dry, use `{venue:dry}` (equivalent to `{wetgain:0.0}` but spec-clean).

> **Default change in r12.1 (1.0 → 0.2)**: The original `1.0` ("wet at parity with dry") saturated the source on hall / cathedral presets and fell outside the musically usable range. Listening tests confirmed **0.1–0.5 is the practical musical range**, so the default was lowered to `0.2`. The bundled LSL UI quick-pick buttons were also re-graded to `0.1`–`0.5` in fine increments.

### 7.4 `{lfegain:N}` (short-form `lg`, added in r12.1)

Gain multiplier applied to the **LFE channel** — both the `{ch:LFE}` route and the LFE band produced by `{upmix:on}`. Independent of dry/wet.

| Value | Effect |
|---|---|
| `0.0` | LFE muted (no output from LFE prim / no low-end emphasis from upmix) |
| `0.5` | LFE at half level |
| **`1.0` (default)** | Source-level (r12-compatible behavior) |
| `2.0` | LFE doubled (typical when emphasizing low-end is desired) |
| `4.0` | LFE 4× (upper bound, sub-PA scenarios) |

#### Use cases

- **5.1 native broadcast** (with a `{ch:LFE}` prim): boost a quietly-recorded LFE bus on the listener side
- **`{upmix:on}` stereo→5.1**: when the 80 Hz LPF band feels weak, push it up
- Conversely, when the LFE prim is mounted on a non-subwoofer speaker, set to `0` to stop low-end leakage

#### When LFE is not active

If there is no `{ch:LFE}` prim and `upmix` is off, the LFE route is inactive and `lfegain` is **a no-op** (writing it is harmless but has no effect).

#### Listener-side sentinel

The debug setting `Stream3DLfeGain` (sentinel `-1.0` = follow tag, otherwise `0.0–4.0` to force-override) was also added in r12.1 (details §12.2). It sits in the broadcaster-driven model rescue slot and is intentionally not exposed in general-listener UI.

### 7.5 Broadcaster-driven model

These four keys (`binaural` / `venue` / `wetgain` / `lfegain`) are **root-prim-Description-as-truth (root truth)** — there is **no Preferences / Debug Settings UI for general listeners**.

#### Why no listener UI?

- If broadcasters decide "this venue is hall, binaural ON" but listeners freely override the values, you get the situation where "the same broadcast sounds different depending on who's listening" — ambiguity in the artistic intent.
- The AYAstorm policy (per `r5 naming consistency` / `r11 broadcaster-driven model`) is **"do not increase expressive ambiguity"**. Adding more tuning axes erodes operational consistency.

#### Exception: listener-side rescue for speaker viewing

A listener using **speakers, not headphones**, may find ITD counterproductive when listening to a `{binaural:on}` broadcast. For this **rescue purpose only**, one sentinel debug setting is provided ─ `Stream3DBinauralRender = 0` to force OFF on the listener side (details §12.2). Similarly `Stream3DVenueOverride` (empty = follow tag, `"dry"` to force all reverb OFF) / `Stream3DVenueWetGain` (sentinel `-1.0` = follow tag) / `Stream3DLfeGain` (sentinel `-1.0` = follow tag, r12.1) are also provided. None of these are exposed in general-user UI.

### 7.6 Combination examples

#### Nothing written (= default)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}]
```
→ Equivalent to `{bin:on}{v:d}{wg:1.0}`. Lite-HRTF applies but no reverb (r10 + localization boost).

#### Live house (PA-oriented, beat-driven music)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:1.0}]
```
→ club preset, dense reflections, dry/wet at parity.

#### Large hall (orchestra)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.8}]
```
→ hall_large, wet pulled down to 0.8× (the long hall reverb leaves source clarity).

#### Cathedral (ambient / environmental)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.6}]
```
→ cathedral, wet at 0.6× (RT60 ~3 s is long, so don't make it too thick).

#### Outdoor (environment / strolling BGM)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:1.0}]
```
→ outdoor, light early reflections only — open-air feel.

#### Already-binaural material (avoid double processing)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF, no reverb (= r10 baseline behavior).

---

## 8. stereo→5.1 upmix (r12)

> r12 adds **stereo-to-5.1 upmix** as an in-viewer DSP. It expands a 2-channel source to 6 channels (FL / FR / C / LFE / SL / SR) so that 5.1 placement (§6.6) can be used even when the broadcast is plain stereo.

This is the largest single addition in r12. Because most SL streaming software (butt / Mixxx / OBS / SAM, etc.) is stereo-only and 5.1-native broadcasting is rare, **without upmix the 6-speaker placement built in r10 was effectively only experienceable with 5.1-native broadcasts**. r12 closes that gap from the viewer side.

### 8.1 `{upmix:on|off}` (no short-form)

Whether to upmix a stereo source into 5.1.

| Value | Meaning |
|---|---|
| `on` | Upmix stereo → 6 ch via DPL2-family matrix decode + band separation |
| `off` | No upmix (= r10 behavior — stereo plays as L/R only) |

Default: `off`. The broadcaster opts in.

#### Why opt-in (default off)?

- Stereo material was originally mixed for stereo. Upmix is interpretation, not reproduction.
- If listeners with 6-prim placement decide independently whether to upmix, "the same broadcast sounds different per listener" — same ambiguity issue as §7.5.
- So the broadcaster decides via tag. r10 behavior is preserved by default.

### 8.2 Algorithm (DPL2-family matrix decode + band separation)

The internal algorithm is fixed (NG1 — no algorithm choice, no "Logic 7 / SRS / ML upmix" option):

1. **DPL2 matrix decode** — derive C and S (surround) channels from L+R, generating L′ / R′ / C / Lₛ / Rₛ
2. **LFE band split** — low-pass the source mono mixdown (cutoff `Stream3DUpmixLfeCutoff`, default 80 Hz THX) and route to LFE
3. **Center bleed removal** — subtract `Stream3DUpmixCenterBleed` × C from L′ / R′ so center-imaged content does not also leak out to FL/FR (default 1.0 = full removal)
4. **Rear decorrelation** — base delay of `Stream3DUpmixRearDelayMs` (default 16 ms) on Lₛ / Rₛ, with ±2 ms jitter to avoid comb filtering between the rear pair

Result: FL = L′, FR = R′, C, LFE, SL = Lₛ′, SR = Rₛ′.

#### Why a single fixed algorithm?

- Per AYAstorm's "do not increase expressive ambiguity" policy, we don't expose `{upmix:dpl2|logic7|srs|...}` as a tag value. Broadcasters pick ON or OFF; the rest is deterministic DSP.
- DPL2 is well-understood, license-clean, and produces stable results across genres. Logic 7 / ML upmix etc. may be revisited in r13+ as objective FFT and listening tests permit.

### 8.3 Auto-bypass for 5.1-native broadcasts

If the source's actual channel count is **6 or higher**, `{upmix:on}` is **automatically bypassed** (no double-processing) and a chat notification is shown once:

```
3D Stream: source is already 5.1 (6ch) — upmix bypassed
```

This way, a broadcaster can leave `{upmix:on}` in the tag and use the same Description for both stereo (upmix runs) and 5.1-native (upmix bypassed) sources. No need to edit the tag when switching content.

### 8.4 Fine-tuning (3 debug settings)

DSP-internal parameters are **NOT exposed as broadcaster tags** — they are listener-side debug settings instead. To preserve the broadcaster-driven model, broadcasters only choose on/off; internal parameters are treated as "part of the algorithm" and held fixed. Listeners do not adjust these in normal use either — they're for implementation/verification or personal tuning.

| Debug setting | Default | Range | Meaning |
|---|---|---|---|
| `Stream3DUpmixLfeCutoff` | `80.0` Hz | 20–200 | LFE LPF cutoff frequency |
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0–1.0 | Fraction of center component subtracted from front L/R (`0` = DPL1-compatible, `1` = full removal) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0–32 | Rear decorrelation base delay (L / R differ by ±2 ms jitter) |

For a listener-side force OFF / ON, one sentinel:

| Debug setting | Default | Meaning |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = follow tag) | `0` ignores the tag and forces OFF / `1` forces ON (5.1-native auto-bypass still applies) |

### 8.5 Combination examples

#### Default behavior (no upmix)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{ch:L}]
[3dstream-stereo:{ch:R}]
```
→ Same as r10 (stereo source goes to L/R 2 spk).

#### 6-spk placement + upmix (r12 recommended format)

```
Root Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}]
FL:  [3dstream-stereo:{ch:FL}]
FR:  [3dstream-stereo:{ch:FR}]
C:   [3dstream-stereo:{ch:C}]
LFE: [3dstream-stereo:{ch:LFE}]
SL:  [3dstream-stereo:{ch:SL}]
SR:  [3dstream-stereo:{ch:SR}]
```
→ Stereo source is expanded to 6 channels and routed to the 6 speaker prims.

#### upmix + binaural + venue (r12 full feature set)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:1.2}]
```
→ Stereo source expanded to 6 ch, each speaker gets lite-HRTF + hall_medium reverb (wet 1.2×). Maximum venue feel + headphone localization.

#### r10 legacy placement (`ch:L`/`ch:R` only) + upmix

```
Root Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}]
L child: [3dstream-stereo:{ch:L}]
R child: [3dstream-stereo:{ch:R}]
```
→ With only L/R speakers in the linkset, upmix runs internally but only L/R is routed out. Effectively becomes "the L/R channels of the upmixed signal are emitted, while C / LFE / SL / SR are computed but unrouted" → in audible terms, **almost identical to no upmix** (DPL2's L′ / R′ ≈ L / R minus center bleed).

→ Not wrong, but if the placement has only L/R speakers, **there is no point in turning upmix on**. Use the 6-spk placement (`ch:FL/FR/C/LFE/SL/SR`) to actually benefit.

#### Putting upmix on a 5.1-native broadcast (auto-bypass)

```
Root Description:
  [3dstream-stereo:{url:http://example/test_5_1.flac}{upmix:on}]
FL:  [3dstream-stereo:{ch:FL}]   # 5 more speakers
```
→ Source has 6 channels, so upmix is auto-bypassed (chat notification once). Each speaker plays the source's corresponding channel directly.

---

## 9. `ch` (Channel) Value Reference

`{ch:value}` accepts the following values. **Case is not significant** (`{ch:l}` and `{ch:L}` are the same).

| Value | Meaning | Primary use |
|---|---|---|
| `L` | Left channel | Stereo L |
| `R` | Right channel | Stereo R |
| `M` | Mono (average of L+R) | "Center speaker" use, or single-point playback |
| `FL` | Front Left | 5.1ch front left |
| `FR` | Front Right | 5.1ch front right |
| `C` | Center | 5.1ch center |
| `LFE` | Low Frequency Effects (subwoofer) | 5.1ch low-frequency |
| `SL` | Surround Left | 5.1ch surround left |
| `SR` | Surround Right | 5.1ch surround right |

When the source's actual channel count and the `ch` value don't match, the system performs an **automatic fallback** rather than reporting a mismatch (§10). For example, writing `{ch:FL}` on a stereo (2ch) source plays L.

Invalid values (e.g., `{ch:foo}`) raise a **format error** notification (§13).

---

## 10. Source Channel Count × Tag Value Compatibility Matrix

What a speaker prim actually plays is determined by the combination of the **source channel count** and the **`ch` value you wrote**.

### 10.1 Compatibility matrix

| Source | `{ch:L}` | `{ch:R}` | `{ch:M}` | `{ch:FL}` | `{ch:FR}` | `{ch:C}` | `{ch:LFE}` | `{ch:SL}` | `{ch:SR}` |
|---|---|---|---|---|---|---|---|---|---|
| **1ch (mono)** | M | M | M | M | M | M | silent | silent | silent |
| **2ch (stereo)** | L | R | (L+R)/2 | L | R | (L+R)/2 | silent | silent | silent |
| **6ch (5.1)** | BS.775 L | BS.775 R | (BS.775 L + R)/2 | FL | FR | C | LFE | SL | SR |

Legend:
- `L` / `R` / `FL` / `FR` / `C` / `LFE` / `SL` / `SR` = direct playback of the corresponding source channel
- `BS.775 L` = the value computed by ITU-R BS.775 downmix coefficients, folding 6ch into stereo L/R (see §10.2)
- `silent` = that speaker emits no sound (the binding is preserved, but no audio comes from the prim)

### 10.2 BS.775 downmix coefficients (6ch source → L/R)

```
L_out = c × ( FL + 0.707·C + 0.707·SL + 0.5·LFE )
R_out = c × ( FR + 0.707·C + 0.707·SR + 0.5·LFE )
c = 1 / 2.914 ≈ 0.343 (normalization for clipping prevention)
```

Center is split equally L/R, surround is summed to its same side, LFE is mixed into both sides equally.

### 10.3 Mixing both placement styles for the same source

If you assign a 5.1ch source to both `{ch:L}` and `{ch:FL}`, the L prim plays the BS.775 downmix while the FL prim plays direct. This is confusing, so the recommendation is to **stick to one channel family per source — either `L/R/M` or `FL/FR/...` — across a venue**.

If a fallback occurs in mixed placements, the **routing diagnostic chat notification** (§12.3 / §13.3) lets you confirm what each ch is actually playing. Turning this ON during 5.1ch venue construction makes mistakes immediately visible.

### 10.4 5.1ch venue placement playing a 2ch / 1ch source

Suppose you have a 5.1ch venue with six speaker prims (`ch:FL` / `FR` / `C` / `LFE` / `SL` / `SR`) already deployed, and you switch the source URL from a 5.1ch broadcast to a **regular stereo (2ch) broadcast** or **mono (1ch) broadcast**. For example: "5.1ch during the live show, regular stereo BGM during breaks", or "MC mono voice between DJ sets".

In this case, **no rearrangement or settings change is needed**. Each speaker prim automatically behaves as follows.

#### When a 2ch (stereo) source plays

| Prim's `ch` value | What plays |
|---|---|
| `{ch:FL}` | **L** (instead of front-left, plays stereo L direct) |
| `{ch:FR}` | **R** (instead of front-right, plays stereo R direct) |
| `{ch:C}` | **(L+R)/2** (center plays mono downmix of L+R) |
| `{ch:LFE}` | **silent** (no LFE signal in source) |
| `{ch:SL}` | **silent** (no surround-left signal in source) |
| `{ch:SR}` | **silent** (no surround-right signal in source) |

Audibly: "**The three front speakers (FL / FR / C) play stereo, while the three surround speakers (LFE / SL / SR) go silent**".

#### When a 1ch (mono) source plays

| Prim's `ch` value | What plays |
|---|---|
| `{ch:FL}` / `{ch:FR}` / `{ch:C}` | **M** (all three front speakers play mono; same audio from three locations) |
| `{ch:LFE}` / `{ch:SL}` / `{ch:SR}` | **silent** |

#### Design intent / notes

- **The three front speakers (FL / FR / C) always play something regardless of source channel count** — so when switching 5.1ch ↔ 2ch ↔ 1ch, the front of the venue never goes silent.
- **LFE / SL / SR stay silent when the source has no corresponding signal** — no fake bass or fake surround is synthesized.
- **When the source returns to 5.1ch, each prim automatically reverts to direct channel playback** (re-evaluated on URL-switch reconnect). No rearrangement, no settings change.
- Running 2ch BGM through a 5.1ch venue placement is a **legitimate operational pattern**. "Surround speakers go silent" is by design, not a bug.

#### Show fallback details in chat (recommended during construction / verification)

A diagnostic switch is provided to confirm in **Local Chat** whether a silent prim is silent due to fallback specification or due to something being broken.

**Setting location** (both control the same value and are synchronized):

- **Preferences > Sound > Show channel routing diagnostics in chat** (checkbox)
- **Debug Settings: `Stream3DRoutingDiagnostic`** (`true` / `false`)

When ON, fallback events for 5.1 placement × 2ch / 1ch source produce lines in **Local Chat as messages from yourself** in the form below (`3D Stream:` prefix added by the §13.3 helper):

**For a 2ch source × 5.1 placement (six prims FL/FR/C/LFE/SL/SR)**:

```
[12:34] You: 3D Stream: ch:FL prim playing L (source is 2ch)
[12:34] You: 3D Stream: ch:FR prim playing R (source is 2ch)
[12:34] You: 3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)
[12:34] You: 3D Stream: ch:LFE prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SL prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SR prim silent (source is 2ch)
```

**For a 1ch source × 5.1 placement**:

```
[12:35] You: 3D Stream: ch:FL prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:FR prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:C prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:LFE prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SL prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SR prim silent (source is 1ch)
```

This makes "LFE / SL / SR are silent by spec, FL / FR / C are operating in fallback" obvious at a glance.

Notifications are throttled with a key of `(root_id, url, observed_channel_count, prim_set_signature)`, so the same notification is not repeated until the placement or source channel count changes. **Recommended: turn ON only during 5.1ch venue construction / verification; OFF (`false`, the default) for production**. See §13.3 for details.

#### Reverse direction: 2ch placement playing a 5.1ch source

For reference, the opposite direction. When a stereo venue (= 2ch placement using only `{ch:L}` / `{ch:R}` / `{ch:M}`) plays a 5.1ch source:

- L / R / M prims play the **BS.775 downmix** (§10.2), folding 6ch into 2ch. FL / C / SL / LFE all sum into L with their coefficients; FR / C / SR / LFE all sum into R.
- All channel signals are audible via L / R, so **no channels are dropped to silence**.
- Local Chat output when `Stream3DRoutingDiagnostic` is ON:

```
[12:36] You: 3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)
[12:36] You: 3D Stream: C content folded into BS.775 downmix (source is 6ch, no ch:C prim)
[12:36] You: 3D Stream: LFE content folded into BS.775 downmix (source is 6ch, no ch:LFE prim)
[12:36] You: 3D Stream: SL content folded into BS.775 downmix (source is 6ch, no ch:SL prim)
[12:36] You: 3D Stream: SR content folded into BS.775 downmix (source is 6ch, no ch:SR prim)
```

You're notified per-channel that "no dedicated prim, so folded into BS.775 downmix path".

---

## 11. Streaming Side (Building Source URLs)

### 11.1 Supported codecs / containers

| Codec / container | 1ch | 2ch | 6ch | Notes |
|---|---|---|---|---|
| **MP3** | ✓ | ✓ | — | Traditional SHOUTcast / Icecast path |
| **Vorbis (Ogg)** | ✓ | ✓ | ✓ | 6ch end-to-end verified (r9 P10) |
| **Opus (Ogg)** | ✓ | ✓ | △ | 6ch uses Opus channel mapping family 1. **Plain HTTP / Icecast push may fail to open due to seek failure** (§11.4) |
| **FLAC** | ✓ | ✓ | △ | 6ch supported in theory; same seek limitation as Opus |
| AAC (ADTS / HLS) | — | — | — | Not supported |
| AC-3 / E-AC-3 | — | — | — | Not supported (Dolby licensing) |

Source URLs may be `http://` or `https://`. A path that maintains HTTP/1.1 keep-alive (= a SHOUTcast-compatible streamer or ffmpeg's TCP output) tends to be more stable than plain static HTTP.

### 11.2 1ch / 2ch streaming

Standard SHOUTcast / Icecast / static HTTP works fine. MP3 / Vorbis / Opus / FLAC all play without issues. Tools like `oggenc`, ffmpeg, or butt work as-is.

### 11.3 5.1ch (Vorbis 6ch) streaming

The recommended path for reliable viewer-side 5.1ch playback is **Vorbis 6ch** (verified end-to-end in r9 P10).

#### 11.3.1 Test material (ffmpeg)

```bash
# 5.1 WAV with a unique frequency per ch (10 sec)
ffmpeg -f lavfi -i "sine=440:d=10" -f lavfi -i "sine=550:d=10" \
       -f lavfi -i "sine=660:d=10" -f lavfi -i "sine=110:d=10" \
       -f lavfi -i "sine=770:d=10" -f lavfi -i "sine=880:d=10" \
       -filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]amerge=inputs=6[a]" \
       -map "[a]" -ac 6 -channel_layout 5.1 test_5_1.wav

# Encode to Vorbis 6ch
ffmpeg -i test_5_1.wav -c:a libvorbis -q:a 5 test_5_1.ogg
```

#### 11.3.2 Static HTTP serving (for verification)

```bash
python3 -m http.server 8080
```

URL: `http://<host>:8080/test_5_1.ogg`

#### 11.3.3 Real-time streaming (ffmpeg → Icecast)

```bash
ffmpeg -re -i test_5_1.wav \
  -c:a libvorbis -q:a 5 \
  -ac 6 -ar 48000 \
  -content_type audio/ogg \
  -f ogg icecast://source:hackme@localhost:8000/aya_5_1.ogg
```

Key options:
- `-re` = real-time (stream at source duration; simulates live broadcast)
- `-content_type audio/ogg` = declare the MIME to Icecast (otherwise it may misidentify as MP3)
- `-ac 6 -ar 48000` = preserve 6ch 48kHz

### 11.4 Opus 6ch / FLAC 6ch limitations

When Opus 6ch (channel mapping family 1) or FLAC 6ch is delivered via **plain HTTP** (e.g., `python3 -m http.server`) or **Icecast push**, the FMOD parser may issue a **seek request** that fails with `FMOD_ERR_FILE_COULDNOTSEEK`, leaving the stream un-openable.

Workarounds:

- Use a **SHOUTcast-compatible streamer** (keep-alive + range support)
- Route through **ffmpeg primary** (TCP backpressure resolves it)
- Use the **5.1ch GUI broadcast tool `butt-aya`** (separate AYA project, unreleased as of writing) to push

To be certain it will work, **Vorbis 6ch is the shortest reliable path right now**.

### 11.5 Choosing a broadcast tool

| Tool | Use case | Notes |
|---|---|---|
| **ffmpeg** | Any codec / any ch / static / real-time | CLI; most flexible |
| **butt** (official) | DJ broadcasting | 1ch / 2ch only; no 5.1ch |
| **butt-aya** (5.1ch fork) | 5.1ch GUI broadcasting | Separate AYA project, unreleased as of writing |
| **Liquidsoap** | Advanced broadcast automation | High config complexity, advanced users |
| **Mixxx / DarkIce / ezstream** | DJ / automation | Stereo-oriented, no 5.1ch |

---

## 12. Viewer-side Settings

### 12.1 Via Preferences

The **Preferences → Sound** tab has these controls:

- **3D Stream** slider — master volume multiplier for all streams (`Stream3DVolumeMaster`)
- **Enabled** checkbox — overall feature ON/OFF (`Stream3DEnabled`)
- **Show channel routing diagnostics in chat** — routing diagnostic notifications (`Stream3DRoutingDiagnostic`, see §13.3)
- **Hear media and sounds from:** — listener position selector, Camera or Avatar (`MediaSoundsEarLocation`, see §15.1)

The same "3D Stream" slider also appears in the speaker icon's Volume dropdown for quick volume adjustment near the voice chat controls.

### 12.2 Debug Settings (advanced tuning)

`Ctrl + Alt + D` opens the Advanced menu → Show Debug Settings to edit any key directly.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `Stream3DEnabled` | bool | `true` | Master kill switch. Setting `false` immediately tears down all bindings; re-enabling does not auto re-bind (it gets re-discovered on the next poll) |
| `Stream3DDescriptionScan` | bool | `true` | When `false`, Description tag scanning is suspended and all prim bindings are released (debug streams unaffected) |
| `Stream3DMaxConcurrent` | S32 | `4` | Max concurrent bindings (mono + stereo combined). 0 = unlimited |
| `Stream3DStereoMaxSpeakers` | S32 | `16` | Max speakers per linkset. Excess is dropped from the tail of the traversal with a warning notification |
| `Stream3DRolloffMin` | F32 (m) | `1.0` | Mono tag default near distance (when `{min}` is omitted) |
| `Stream3DRolloffMax` | F32 (m) | `20.0` | Default far distance (shared fallback when mono `{max}` or stereo `{range}` is omitted) |
| `Stream3DMaxDistance` | F32 (m) | `64.0` | Prim discovery polling radius. Set ≥ `Stream3DRolloffMax` |
| `Stream3DPollInterval` | F32 (sec) | `30.0` | Description polling interval. Affects how quickly LSL-mediated tag changes are picked up. 0 disables active polling |
| `Stream3DVolumeMaster` | F32 [0–1] | `0.5` | Master volume multiplier. Same as the 3D Stream slider in Preferences |
| `Stream3DReconnectAttempts` | S32 | `3` | Auto-reconnect attempts on stream disconnect. Each retry waits 5 seconds. 0 disables reconnect |
| `Stream3DRoutingDiagnostic` | bool | `false` | Routing-diagnostic chat notifications ON/OFF (see §13.3). Synchronized with the Preferences checkbox |

#### r11/r12 broadcaster-driven model: listener-side sentinels (no general UI)

As described in §7.5 / §8.4, the `binaural` / `venue` / `wetgain` / `lfegain` / `upmix` keys are **broadcaster-tag-as-truth**, with no Preferences UI. For rescue purposes only, sentinel debug settings are provided. General listeners should not touch these.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = follow tag) | `0` to force OFF on the listener side / `1` to force ON. Rescue use when listening to a `{binaural:on}` broadcast on speakers (details in §7.5 Exception) |
| `Stream3DVenueOverride` | string | `""` (sentinel = follow tag) | A venue name like `"dry"` forces all broadcasts to play in that venue (`"dry"` = force all reverb OFF, the typical use) |
| `Stream3DVenueWetGain` | F32 | `-1.0` (sentinel = follow tag) | A value in `0.0–2.0` forces a wet-gain override |
| `Stream3DLfeGain` | F32 | `-1.0` (sentinel = follow tag) | A value in `0.0–4.0` forces an LFE-gain override (added in r12.1, details §7.4) |
| `Stream3DUpmix` | S32 | `-1` (sentinel = follow tag) | `0` ignores the tag and forces OFF / `1` forces ON. The 5.1-native auto-bypass still applies (details §8.1 / §8.3) |
| `Stream3DUpmixLfeCutoff` | F32 (Hz) | `80.0` | upmix DSP LFE LPF cutoff (20–200). Details §8.4 |
| `Stream3DUpmixCenterBleed` | F32 | `1.0` | upmix DSP center bleed removal fraction (0.0–1.0). Details §8.4 |
| `Stream3DUpmixRearDelayMs` | F32 (ms) | `16.0` | upmix DSP rear decorrelation delay (0–32). Details §8.4 |

> **r12.1 live-tuning fix**: At r12 release, the settings `Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` only took effect after the affected prim was **touched (Description re-parsed)**. r12.1 adds a per-poll push to the polling loop, so all of these are now applied from the next frame after the value changes (§12.3).

#### Debug-only (for development verification)

| Key | Type | Use |
|---|---|---|
| `Stream3DDebugUrl` | string | Debug-target URL |
| `Stream3DDebugPlay` | bool | When `true`, places & plays a mono stream 5m in front of the avatar (quick test, no tag editing required) |
| `Stream3DDebugStereoPlay` | bool | Same for the stereo version |

### 12.3 Persistence and immediate apply

Most settings are **"Live"** — applied from the next frame after the value changes. No viewer restart needed. Exceptions:

- Toggling `Stream3DEnabled` from `false` to `true` does NOT auto re-bind. Re-discovery happens on the next poll cycle (within ~30s by default).
- Toggling `Stream3DDescriptionScan` immediately tears down or re-discovers all bindings.

> **Fixed in r12.1**: At r12 release, `Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` regressed and only took effect after the affected prim was **touched (Description re-parsed)**. r12.1 added a per-poll push in `LLPositionalStreamMgr::update()` so these now follow the standard "next-frame apply" semantics.

---

## 13. Error Notifications / Diagnostics

### 13.1 How notifications appear

Tag format errors and structural errors are **shown in Local Chat** as system messages, prefixed with "3D Stream:".

```
3D Stream: Tag format error (object name: "MySpeaker")
  ch must be one of L/R/M/FL/FR/C/LFE/SL/SR.
  Example: [3dstream-stereo:{ch:L}{range:30}]
```

```
3D Stream: Structural error (linkset root: "MainStage")
  Source declaration found on root, but no speakers (ch) found.
  Each speaker prim should have [3dstream-stereo:{ch:L|R|M}].
```

### 13.2 30-second suppression

The same prim × the same error kind is **suppressed for 30 seconds**, to avoid chat flooding while you edit tags. After 30s, the notification fires once again.

Suppressed notifications still go to the LL_DEBUGS log (debug log channel), so you can confirm what's happening internally.

### 13.3 Routing diagnostics (5.1ch placement)

A diagnostic feature for **construction / verification** of 5.1ch / multi-speaker venues, letting you confirm in Local Chat "what each prim is playing / why a prim is silent". **Default OFF** — does not emit unless explicitly enabled.

#### 13.3.1 How to enable

Either of the following enables it (they are synchronized):

- **Preferences > Sound > Show channel routing diagnostics in chat** — check the box
- **Debug Settings: set `Stream3DRoutingDiagnostic` to `true`** (Advanced > Show Debug Settings)

Applied immediately. No viewer restart needed.

#### 13.3.2 Output destination and format

When ON, fallback events emit one line per occurrence in **Local Chat as messages from yourself**, in this form:

```
[HH:MM] You: 3D Stream: <content>
```

The `3D Stream:` prefix is added automatically by the `notifyStream3D` helper. They are stored in the chat log (`Show in Chat`) so you can review them after the test. **They are NOT visible to anyone else** (these are pseudo-messages displayed only in your own Local Chat).

#### 13.3.3 Notification text reference

| Situation | Local Chat line |
|---|---|
| 6ch source × `ch:L/R/M` prim only (no dedicated ch prim) | `3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)` (one line per FL/FR/C/LFE/SL/SR) |
| 6ch source × neither dedicated nor `ch:L/R/M` prim | `3D Stream: FL content has no destination — dropped (source is 6ch, no ch:L/R/M prim)` |
| 2ch source × `ch:FL` prim | `3D Stream: ch:FL prim playing L (source is 2ch)` |
| 2ch source × `ch:FR` prim | `3D Stream: ch:FR prim playing R (source is 2ch)` |
| 2ch source × `ch:C` prim | `3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)` |
| 1ch source × `ch:FL/FR/C` prim | `3D Stream: ch:FL prim playing M (source is 1ch)` (FR / C use the same form) |
| 1ch / 2ch source × `ch:LFE/SL/SR` prim | `3D Stream: ch:LFE prim silent (source is 2ch)` (or `1ch`; SL / SR use the same form) |

For concrete sample output for "5.1 placement × 2ch source", see "Show fallback details in chat" in §10.4.

#### 13.3.4 Throttle and re-display conditions

Notifications are throttled with a key of `(root_id, url, observed_channel_count, prim_set_signature)`. While the same venue and the same source configuration are in effect, **lines do not repeat** (to avoid flooding the chat). Re-evaluation and re-emission happen when any of the following changes:

- Source URL changes (= switched to a different stream)
- Source channel count changes (= same URL but it switched 5.1ch ↔ 2ch)
- Speaker prim configuration changes (prim added / removed / `ch` value changed)
- Right after toggling `Stream3DRoutingDiagnostic` from OFF to ON

#### 13.3.5 Operational recommendation

- **Turn ON during venue construction and placement verification** to confirm fallback behavior of each prim in Local Chat
- **Turn OFF for production (live shows etc.)** to keep the chat clean
- The default (OFF) is the production-intended state. Manually flip to ON only during placement verification.

### 13.4 Logs (`LL_INFOS("Stream3D")`)

Detailed runtime logs are recorded in AYAstorm's log file:

```
~/.ayastorm_x64/logs/AYAstorm.log
```

Grep for the `Stream3D` channel to see binding establishment / teardown / reconnect / source format detection / dropouts.

---

## 14. Troubleshooting

### 14.1 Tag was written but no sound plays

Check in order:

1. **Description was actually updated**: Right-click the prim → Edit → Description tab to confirm the current value
2. **Tag spelling**: Confirm the prefix is exactly `[3dstream:` or `[3dstream-stereo:` (typos)
3. **Source declaration is valid**: URL source requires `{url:http://...}` or `{url:https://...}`. Media source requires `{source:media}` on the root prim and a media face in the same linkset
4. **`Stream3DEnabled` / `Stream3DDescriptionScan` are both true**: Confirm in Preferences > Sound or Debug Settings
5. **Wait for poll**: Changes via LSL `llSetObjectDesc` take up to 30 seconds (`Stream3DPollInterval`)
6. **Look for an error notification in chat**: See §13
7. **Look in logs for `LL_INFOS("Stream3D")` reconnect attempts**: Stream URL may be down

### 14.2 Only one stereo channel plays

- If the root has only `{url}` and the child only `{ch:R}`, no prim handles L → "L missing" state. Add `{ch:L}` to the root, or assign `{ch:L}` to another prim.
- If you write `{ch:L}` to two prims on purpose to double-up the L speaker, that's intended and fine.
- Turn ON the routing diagnostic (`Stream3DRoutingDiagnostic`) to see what each ch is actually playing, in chat.

### 14.3 5.1ch source won't open / audio glitches

- §11.4 seek limitation: common with Opus 6ch / FLAC 6ch over plain HTTP / Icecast push. **Switch to Vorbis 6ch**, or route via a SHOUTcast-compatible streamer / ffmpeg primary.
- The first 5–10 seconds after an HTTP switch may produce dropout warnings while the prebuffer fills (LAN: ~408–2045 frames/spk/s ≈ 0.8–4%). They subside in steady state.
- Bitrate too high / network congested → dropouts: lower bitrate on broadcaster (≤ 256kbps recommended) / reduce concurrent bindings.

### 14.4 Tag on a child prim but it's not recognized as a speaker

- Child-prim Description is fetched via Properties messaging. The **first time you enter the linkset's region it can take a few seconds (up to ~10s)**.
- After modifying child Description via LSL `llSetObjectDesc`, it applies on the next poll cycle (within 30s).
- Confirm `{ch:...}` is not a typo (case-insensitive, but spelling errors are invalid).

### 14.5 Listener position seems wrong (sound direction is off)

- When you camera-flick around, listener position follows camera, so the localization changes. To lock to avatar, set Preferences > Sound **"Hear media and sounds from:" to Avatar** (`MediaSoundsEarLocation = 1`).
- This Camera/Avatar selector applies to 3D Stream as well (shared with parcel BGM, LSL `llPlaySound`, etc.).

### 14.6 Want to play multiple 3D Streams at once

- New bindings are rejected once `Stream3DMaxConcurrent` (default 4) is reached. Increase the value if you need more concurrent streams (8 / 16 are practical).
- Note that 1 binding = 1 decoder thread + N speaker channels of CPU cost. 20 concurrent will stress CPU; bump only as needed.

### 14.7 Removed the tag but sound continues

- Re-evaluation may not have triggered. Move the prim once, or look around the area to wait for the next poll.
- If still stuck, toggle `Stream3DEnabled` to false (force-release all bindings) then back to true (re-discover).

### 14.8 `{source:media}` reports no media face

- Confirm the media is set on a face in the same linkset as the root tag.
- If the media is on a child prim, add `{link:N}` using that child's SL link number.
- If the selected prim has multiple media faces, add `{face:N}`.
- Right after linking or loading an object, media-face information may arrive slightly later than the root Description. Wait a few seconds or touch/edit the object to trigger re-evaluation.
- Do not combine `{url:...}` and `{source:media}` in the same root tag.

---

## 15. Known Limitations / Specification Notes

### 15.1 Listener position is camera or avatar

The listener position used for 3D Stream's spatialization follows Preferences > Sound **"Hear media and sounds from:"**:

- `Camera` (default): camera position / orientation
- `Avatar`: avatar position / orientation

This is the same setting used by LSL `llPlaySound`, parcel BGM, and Media-on-a-Prim.

### 15.2 1 linkset = 1 source

What matters per linkset is whether a root source declaration exists. The source is either `{url:...}` or `{source:media}`. **Multiple source declarations in one linkset are not allowed**, and `{url}` / `{source:media}` on a child prim is ignored.

To run multiple distinct sources in one venue, split them into separate linksets and place them — they coexist as separate bindings within `Stream3DMaxConcurrent`.

### 15.3 Description byte limit (127)

The Description writable via LSL `llSetObjectDesc` is **limited to 127 bytes**. URLs and descriptions that contain Japanese (or any UTF-8 multibyte) easily exceed this.

When it gets long:

- **Use short-forms for keys / venue values** (§4.5). `binaural`/`venue`/`wetgain` become `bin`/`v`/`wg`, and the 9 venue values also have 1–2 character aliases. Long-form and short-form are fully equivalent.
- Use the distributed pattern: write `{url}` on root only, `{ch}` on children only (each prim's Description stays short)
- Shorten the URL (URL shortener, or a shorter path on the broadcaster side)

### 15.4 Verified behavior per codec

| Codec | 1ch / 2ch | 6ch |
|---|---|---|
| Vorbis (Ogg) | ✓ verified end-to-end | ✓ verified end-to-end (r9 P10, 12 minutes continuous, 0 dropout) |
| Opus (Ogg) | ✓ verified end-to-end | △ code review only (works on production paths; static HTTP / Icecast push fail at seek) |
| FLAC | ✓ verified end-to-end | △ code review only (same constraint as Opus) |
| MP3 | ✓ verified end-to-end | — |

For reliable 5.1ch playback, choose **Vorbis 6ch**.

### 15.5 No special LFE handling

5.1ch's LFE (subwoofer) is treated equivalently to the other 5 channels — no low-pass filter, no 2D-ization. It is 3D-positioned and distance-attenuated like the rest. If LFE band-limiting is needed, do it on the broadcaster mix.

The intended pattern is: place a physical subwoofer-shaped prim in SL at the appropriate position and have low-frequency audio emit from there.

### 15.6 5.1ch in a free-camera world

A real 5.1 system (cinema standard / ITU-R BS.775) assumes **the listener is in a fixed position** and bakes directional cues per channel. SL's listener is free-camera, so the "sweet spot" concept does not apply. The intent of this feature is **"multi-point reproduction of a 5.1 source in a venue"**, in the spirit of venue PA — not the reproduction of cinematic surround.

When the listener walks around the space, the intended 5.1 image will of course break, but the "venue feel / sense of sound covering an area" comes through clearly.

### 15.7 Behavior in other Viewers

`[3dstream:...]` / `[3dstream-stereo:...]` tags are **AYAstorm-specific**. Mainline Firestorm, official LL Viewer, Catznip, etc. ignore them entirely.

- AYAstorm users: 3D-positional audio plays as designed
- Other Viewer users: the tag just appears as text in the description, no audio plays (3D Stream is independent of parcel BGM, so any parcel BGM that's set up is still audible to them)

### 15.8 Concurrency caps

| Cap | Default |
|---|---|
| `Stream3DMaxConcurrent` (bindings per linkset) | 4 |
| `Stream3DStereoMaxSpeakers` (speakers per binding) | 16 |
| Resulting max concurrent speakers | 4 × 16 = 64 |

Up to ~64 channels stays within FMOD headroom. If you need more, raise via debug settings (verify CPU load on real hardware first).

### 15.9 Volume composition

For URL sources, final volume is:

```
Stream3DVolumeMaster × {volume:N} × FMOD distance attenuation × Master Audio Slider × any mute states
```

Typically use `Stream3DVolumeMaster` (the 3D Stream slider in Preferences) for global control, `{volume:N}` for per-prim correction, and `range` (per-speaker) or `Stream3DRolloffMax` (global default) for distance attenuation.

For media/MOAP sources, media volume can also act as source gain in the single-media-face case. See §6.10.4 for the exact media volume rule.

---

## 16. Static Occlusion `[ayastorm:occlude]` (r13)

A **venue operator / builder** tag. Marking wall / door / floor / ceiling prims with this tag makes AYAstorm treat them as **sound-blocking geometry** when the listener-to-source segment passes through them — the audio becomes muffled (less volume + lowpass-coloured).

Whereas `[3dstream:...]` / `[3dstream-stereo:...]` (§5 / §6) are **sound-emitting** tags, `[ayastorm:occlude]` is a **sound-blocking** tag. The two are entirely independent — a prim tagged only with `occlude` does not emit any audio.

### 16.1 Syntax

```
[ayastorm:occlude]                            ← defaults (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← explicit values
[ayastorm:occlude{direct:0.6}]                ← one key only (other uses default)
```

The legacy prefix (`[ayastream:occlude]`) is **not accepted** — occlusion was introduced in r13 and there are no legacy ayastream prims to preserve. Common syntax rules (§4.3, case-insensitive keys / whitespace-trimmed values / unknown keys silently ignored) apply.

### 16.2 Behaviour model

#### What gets occluded

- **`[3dstream:...]` / `[3dstream-stereo:...]` audio** (positional streams, per speaker prim)
- **`llPlaySound` / attached sounds / child-prim SFX** (world SFX)

If the segment from listener (camera or avatar) to source crosses the **actual prim shape (triangle mesh)** of an occlude prim, occlusion is applied (volume attenuation + lowpass colouration). Path Cut openings, Hollow interiors, and full mesh shapes all participate, so audio behaves intuitively — sound passes through a doughnut hole, gets muffled by the wall itself.

If the segment crosses multiple occluders simultaneously, attenuation is **multiplicative pass-through** — e.g. two walls of `direct=0.7` yield an effective `1 - (1-0.7)² ≈ 0.91`, three walls go further. More walls means more muffling, matching natural intuition.

Internally this is a two-stage test: a cheap bounding-OBB pre-cull (segment-vs-AABB) rejects ~95% of mismatched pairs, then surviving candidates run a Möller-Trumbore segment-triangle raycast. Keeps CPU low while preserving exact-shape accuracy.

#### What does not get occluded

- **2D streams** (parcel music — no positional info)
- **Voice (Vivox / WebRTC)**
- **UI / preview SFX** (filtered out via `isForcedPriority` internally)

### 16.3 Argument reference

| Key | Default | Range | Effect |
|---|---|---|---|
| `direct` | `0.7` | `0.0`-`1.0` | Direct-path (= volume) attenuation. `0.0` = pass-through, `1.0` = near-silent |
| `reverb` | `0.5` | `0.0`-`1.0` | Reverb-tail attenuation. `0.0` = reverb pass-through, `1.0` = reverb cut |

Higher `direct` deepens the "behind-the-wall" impression and additionally pushes the viewer's built-in LOWPASS_SIMPLE cutoff down (22 kHz → 300 Hz, full effect at `direct=1.0`) so the audio also sounds bass-heavy and muffled. `reverb` only has audible effect if the source prim has venue reverb enabled via `{venue:...}` (§7.2).

### 16.4 Recommended values by material

Empirical starting points. Adjust to taste on-site.

| Material impression | `direct` | `reverb` | Subjective effect |
|---|---|---|---|
| Stone / concrete | `0.9` | `0.7` | Near-silent, only bass bleeds through |
| Wood / interior panel | `0.7` | `0.5` | Default — typical "wall next door" |
| Thin wood / curtain | `0.6` | `0.4` | Muffled bleed, lightweight partition |
| Glass / paper screen | `0.3` | `0.2` | Slight muffling, content still discernible |
| Decorative (essentially transparent) | `0.1` | `0.05` | Mostly pass-through, silhouette only |

### 16.5 Automatic follow (dynamic doors work)

`refreshOccluders` runs **every tick** (= once per `LLPositionalStreamMgr::update()`) and re-reads every occluder prim's position / rotation / scale. So:

- A **moving door** (LSL-animated `llSetPos` / `llSetRot`) tagged with `[ayastorm:occlude]` automatically tracks the open/close motion — occlusion changes in real time
- **Vehicles / mobile prims** with the tag also track the same way
- A dedicated "door tag" is **not needed** (r13 spec originally planned `[ayastorm:door]` but `refreshOccluders` covers it cleanly, so it was permanently dropped)

### 16.6 Distance cull (`Stream3DOccluderRange` = 64m)

If the listener-to-source distance exceeds `Stream3DOccluderRange` (default 64m), OBB raycast is **skipped** for that source (audibility is already negligible due to distance attenuation). For very large venues that need 64m+ occlusion, raise the value or set it to `0` to always raycast (see §12.2).

### 16.7 Master toggle (`Stream3DOcclusion`)

A **global on/off switch** for troubleshooting (debug setting).

| Value | Behaviour |
|---|---|
| `-1` (default) | Enabled. All `[ayastorm:occlude]` tags are evaluated |
| `0` | Disabled. Tags fully ignored; previously-muffled audio ramps back to bypass via the normal smoothing path |
| `1` | Explicit enabled (reserved for future per-mode override) |

Live toggling **never produces an audible cliff** — even when disabled, the smoothing path keeps running, so the DSP ramps back to bypass over `Stream3DOcclusionRampMs` (default 250 ms).

### 16.8 Visualisation (`Stream3DShowOccluders`, Alt+Shift+O)

A debug overlay that renders every registered occluder prim as a **cyan triangle mesh** (translucent fill + wireframe). The exact triangles used by the raycast are drawn, so Path Cut / Hollow / mesh shapes appear **as they are**. Useful during venue construction to verify "is the tag recognised" and "is the right shape being used for occlusion".

- **Menu**: View → Highlighting and Visibility → "Show 3D Stream Occluders (AYAstorm)"
- **Hotkey**: `Alt+Shift+O` (live toggle)

**Live tracking**: An occluder prim **selected in the build floater** updates its cyan shape live while you drag Path Cut / Hollow / Sculpt sliders — you can verify the occlusion shape before closing the edit window. Non-selected occluders update once the edit window closes (sim round-trip).

**Fallback indicator**: An occluder whose triangle extraction failed (e.g. mesh exceeds the 2000-triangle cap, see §16.9) renders no cyan. A tagged prim that shows no cyan overlay is a visual cue that it has fallen back to OBB-only.

Setting `Stream3DOcclusion` (master toggle, §16.7) to `0` does **not** disable the overlay — the two toggles are intentionally independent so a venue operator can inspect occluder structure with audio occlusion off.

### 16.9 Limits

- **256 simultaneous occluders** (`kMaxOccluders` hardcoded). If more than 256 `[ayastorm:occlude]`-tagged prims exist in the sim, the 257th onward are not registered (`LL_WARNS` logged). Typical SL venues (~100 prims) have plenty of headroom.
- **Triangle count cap**: **2000 triangles per occluder** (`kMaxTrisPerOccluder` hardcoded). A mesh prim exceeding the cap skips triangle extraction and falls back to bounding-OBB-only occlusion (`LL_WARNS_ONCE` logged; the §16.8 cyan overlay shows nothing for the prim). Standard SL building prims (cube / cylinder / hollow / Path Cut) stay in the tens-to-hundreds range; even architectural mesh prims are usually well within budget.
- **CPU cost**: The 64m distance cull + OBB pre-cull reject the vast majority of (segment, occluder) pairs in a handful of operations. Triangle raycast only runs for the few prims a listener-source line actually intersects. Stays under 1 ms/sec on typical venues (~100 occluders).
- **Bundled FMOD constraint**: The implementation does a viewer-side OBB pre-cull + Möller-Trumbore triangle raycast (the bundled `libfmod 2.03.07`'s `FMOD::Geometry::createGeometry` is non-functional). Transparent to end users.

---

## 17. Related Documents / Internal Specifications

This guide is the **user-facing** format reference. Implementation details (decode thread / FMOD path / ring buffer / shutdown order, etc.) are in the following internal specs.

| Document | Content |
|---|---|
| `docs/specs/spec_positional_stream_audio.md` | 3D Stream core spec (revised at r5) — base architecture |
| `docs/specs/spec_stream3d_decode_thread.md` | The 3-thread model established in r7 |
| `docs/specs/spec_distributed_stereo.md` | r8 distributed stereo spec — `[3dstream-stereo:...]` field syntax |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch source ingestion spec — Opus/FLAC 6ch decode path + BS.775 downmix |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch venue placement spec — `ch=FL/FR/C/LFE/SL/SR` extension + compatibility matrix |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 binaural + venue reverb spec (bundled into r12 release) — covers §7 of this guide |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 upmix spec — DPL2 + 4-step band separation, covers §8 of this guide |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase breakdown (P0–P11) and verification design |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB occlusion spec + implementation record — `[ayastorm:occlude]` design decisions / spike implementation / remaining work, covers §16 of this guide |
| `docs/ayastorm-stream3d-roadmap.md` | Overall 3D Stream roadmap |

---

## Revision History

- **2026-05-05 (initial)**: Compiled as the final spec at r10. Cumulative spec from r5 / r8 / r9 / r10 / r10.x. r11 and later not covered (unreleased).
- **2026-05-08 (r12)**: Added §4.5 (short-forms), §7 (Binaural / Venue Reverb), §8 (stereo→5.1 upmix). r11 is bundled into r12 (not released independently — to avoid two-step tag-format change confusion). Renumbered later sections (§7–§14 → §9–§16). §12.2 lists r11/r12 listener-side sentinel debug settings; broadcaster-driven model preserved (no general Preferences UI).
- **2026-05-09 (r12.1)**: Added new §7.4 `{lfegain:N}` (short-form `lg`); renumbered the prior §7.4 Broadcaster-driven model to §7.5 and the prior §7.5 Combination examples to §7.6. Lowered the `wetgain` default from `1.0` to `0.2` to reflect the practical musical range (0.1–0.5) confirmed by listening tests. Added the `Stream3DLfeGain` sentinel to §12.2 and a live-tuning fix note in §12.2 / §12.3 (covering the r12 regression where `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` only took effect after a prim touch).
- **2026-05-11 (r13)**: Added new §16 Static OBB Occlusion `[ayastorm:occlude]`; renumbered the prior §16 Related Documents to §17. Extended §4.1 from "Two tag types" to "Three tag types". Documented the r13 debug settings (`Stream3DOcclusion` master sentinel / `Stream3DOccluderRange` distance cull / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` overlay) inside §16.6-§16.8. Added `docs/ayastorm-r13-occlusion.md` row to the §17 spec table.
- **2026-05-11 (r13 P15)**: Upgraded occlusion from OBB approximation to **exact-shape triangle raycast** (OBB pre-cull + Möller-Trumbore, see §16.2). Path Cut / Hollow / Mesh now feed real geometry into the audio calculation. Corrected the multi-occluder section to describe **multiplicative pass-through** (the implementation was always multiplicative; older docs misdescribed it as `max`). Changed `Stream3DShowOccluders` from OBB wireframe to **cyan triangle mesh** (translucent fill + wireframe), with live tracking while a prim is selected in the build floater (§16.8). Added the 2000-tri per-occluder cap and OBB-only fallback rules in §16.9. Shortened §16 title from "Static OBB Occlusion" to "Static Occlusion".
