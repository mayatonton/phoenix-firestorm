# 3D Stream Tag Format Guide

> **Language / 言語 / 语言**: **English** · [日本語](./3dstream-tag-guide.ja.md) · [中文](./3dstream-tag-guide.zh.md)
>
> Tag format reference for AYAstorm's **3D Stream** feature, which plays HTTP audio streams or Media-on-a-Prim (MOAP) audio as 3D-positional audio from prims.
>
> This document reflects the final specification as of AYAstorm `r31`. In r31, the recommended prefix for both single-prim playback and linkset speaker layouts is unified as `[3dstream:...]`. Legacy `[3dstream-stereo:...]` / `[ayastream-stereo:...]` prefixes are still accepted for compatibility.

---

## Table of Contents

1. [What is 3D Stream?](#1-what-is-3d-stream)
2. [Quick Start](#2-quick-start)
3. [Terminology](#3-terminology)
4. [Tag Overview](#4-tag-overview)
5. [Single-prim URL Playback `[3dstream:...]`](#5-single-prim-url-playback-3dstream)
6. [Linkset Layout / Distributed Stereo `[3dstream:...]`](#6-linkset-layout--distributed-stereo-3dstream)
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
   [3dstream:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **Add to root Description** (root itself becomes the L speaker):
   ```
   [3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child Description**:
   ```
   [3dstream:{ch:R}]
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
| **Source declaration** | A root prim tag containing either `{url:...}` or `{source:media...}`. Declares what audio source the linkset plays. Source declarations are **root-only** (ignored on child prims) |
| **Media source** | A media/MOAP face in the same linkset used as the 3D Stream audio source. Enabled by `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` on the root, and selected with `{link:N}{face:N}` when needed |
| **Speaker prim** | A prim with a tag containing `{ch:...}`. Actually emits sound. **Can be either root or child** |
| **binding** | The internal "source → speaker group" mapping built per linkset. 1 linkset = 1 binding |
| **ch (channel)** | The audio channel a speaker prim is responsible for. `L` / `R` / `M` (mono), or 5.1ch values `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff** | Distance attenuation. Volume decreases as the listener moves away from the speaker |

---

## 4. Tag Overview

### 4.1 Three use cases

| Use case | Prefix | Conditions |
|---|---|---|
| **Single-prim URL playback** | `[3dstream:...]` | Has `{url:...}` and the same linkset has no 3D Stream tag with `{ch:...}` |
| **Linkset layout / distributed stereo** | `[3dstream:...]` | The root has `{url:...}` or `{source:media...}`, and at least one root or child prim has `{ch:...}` |
| **Static occlusion tag** (added in r13) | `[ayastorm:occlude]` | Mark wall / door / floor / ceiling prims as sound-blocking geometry (venue operator / builder, see §16) |

### 4.2 Aliases for legacy prefixes

For new content in r31 and later, use **`[3dstream:...]`**. Legacy prefixes (`[ayastream:...]` / `[3dstream-stereo:...]` / `[ayastream-stereo:...]`) are still accepted so existing prims from before the r5 rename and the r31 unified-tag change do not need to be edited.

```
[3dstream:{url:...}]              ← recommended (canonical)
[3dstream:{url:...}{ch:L}]        ← recommended for linkset layouts (canonical)

[ayastream:{url:...}]             ← legacy, accepted
[3dstream-stereo:{ch:L}]          ← legacy, accepted
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

For `[3dstream:...]`, **4 frequent keys** and **all 9 `venue` values** have **short-form aliases** introduced in r12. They make the Description fit comfortably under SL's 127-byte limit (§4.4). Long-form and short-form are **fully equivalent** (resolved to the same canonical form internally). New tags and existing tags can use either form, and behavior is identical.

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
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{binaural:on}{venue:hall_medium}{wetgain:1.5}{upmix:on}]
```

Short form (110 bytes — fits, 23 bytes saved):

```
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{bin:on}{v:hm}{wg:1.5}{upmix:on}]
```

#### LSL helper behavior

The bundled LSL `aya_3dstream_setup.lsl` (see §17) **accepts both forms on input** and **always writes short-form on output (Description writes)**. Descriptions configured via the LSL dialog are automatically converted to short-form.

#### Notes

- Key names are **case-insensitive** (per §4.3 common rule). `{BIN:on}`, `{bin:on}`, and `{binaural:on}` are all equivalent.
- Long and short forms may be mixed in the same tag (for example, `{binaural:on}{v:hm}{wg:1.5}`). For readability, using one style consistently is recommended.

### 4.6 Tag activation timing

- AYAstorm **polls in-range prim Descriptions every 30 seconds** (`Stream3DPollInterval` setting).
- When you modify Description via LSL `llSetObjectDesc`, the next poll re-evaluates and applies the change (typically within 5–30 seconds).
- When you manually right-click → Edit → Description, the edit triggers immediate re-evaluation (via Properties broadcast).
- Linking / unlinking also triggers re-evaluation.

---

## 5. Single-prim URL Playback `[3dstream:...]`

### 5.1 Syntax

```
[3dstream:{url:URL}{min:N}{max:N}]
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
- A `[3dstream:...]` tag with only `{url:...}` is single-prim URL playback.
- If the same linkset has at least one 3D Stream tag with `{ch:...}`, `[3dstream:{url:...}]` is treated as a linkset source declaration instead of single-prim mono playback.
- The presence of child prims alone does not switch the object into linkset layout mode. The decision is based on whether `{ch:...}` exists, not on whether child prims exist.
- `{source:media...}` is not single-prim playback. Media/MOAP source routing is a linkset layout feature (§6.7) and must be combined with at least one `{ch:...}` speaker.
- A stereo source is **internally mixed down to mono** (L+R average).

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

## 6. Linkset Layout / Distributed Stereo `[3dstream:...]`

### 6.1 Syntax

```
[3dstream:{url:URL}{range:N}{ch:CH}{volume:V}]
[3dstream:{source:media}{link:N}{face:N}{range:N}{ch:CH}{volume:V}]
```

Or with the legacy prefix:

```
[3dstream-stereo:...]
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

Linkset playback starts when the linkset has both **a source declaration** (root with `{url}` or `{source:media}`) and **at least one speaker** (a prim with `{ch}`). If there are child prims but no `{ch}` tags, `[3dstream:{url:...}]` remains single-prim URL playback. `{source:media}` cannot start playback without at least one `{ch}` speaker. `{url}` and `{source:media}` are mutually exclusive; writing both in the same root tag is a structural error.

### 6.3 Key reference

#### 6.3.1 Keys meaningful only on the root prim

| Key | Required | Type | Default | Meaning |
|---|---|---|---|---|
| `url` | required, mutually exclusive with `source` | string | — | Stream URL. Empty string is an error |
| `source` | required, mutually exclusive with `url` | enum | — | `media` / `media-stereo` = treat a media/MOAP face as a 2ch source. `media-5-1` = treat it as a 5.1ch / 6ch source |
| `link` | optional | S32 | auto-select | Link number containing the media face when `{source:media}` is used. Used to select among multiple media faces |
| `face` | optional | S32 | auto-select | Media face number when `{source:media}` is used. Used to select among multiple media faces |
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
| `range` | optional | F32 (m) | Falls back: root `range` → `Stream3DRolloffMax` | Per-speaker rolloff distance |
| `volume` | optional | F32 [0.0–1.0] | 1.0 | Per-speaker volume multiplier |

> **Important**: The `min` / `max` keys from single-prim URL playback are **ignored** in linkset layouts. In linkset layouts, the near distance is internally fixed at 1.0m, and the far distance is the `range` key (or default `Stream3DRolloffMax`).

> **Important**: `{url:...}` and `{source:media}` are alternatives. Do not put both in the same root tag. If you want to show media on a face while the speakers play a separate stream, keep `{url:...}` as the source and leave the media face unselected by 3D Stream (§6.7).

### 6.4 One root + one child (basic stereo pair)

Smallest stereo placement:

```
Root Description:
  [3dstream:{url:http://example.com/stream.mp3}{ch:L}]

Child Description:
  [3dstream:{ch:R}]
```

The root takes L, the child takes R. The **link order (link number)** of root vs child does NOT affect playback. Spatial positioning is determined by where in space you place each prim.

### 6.5 Multi-speaker (4+ speakers)

The same stereo stream played from four corners of a venue:

```
Root Description:
  [3dstream:{url:http://example.com/stream.mp3}{range:50}]

Child #1 Description:
  [3dstream:{ch:L}]

Child #2 Description:
  [3dstream:{ch:R}]

Child #3 Description:
  [3dstream:{ch:L}{volume:0.7}]

Child #4 Description:
  [3dstream:{ch:R}{volume:0.7}]
```

- Root only declares the source; it does not play (no `{ch}`)
- Root `{range:50}` becomes the default rolloff distance for all four child speakers
- Children #1, #2 carry L/R at full volume
- Children #3, #4 carry the same L/R at 70% volume (rear or support speakers)
- If the same `{ch}` appears on multiple prims, that channel plays from multiple positions. This is useful for placing the same L/R pair at the front and rear of a venue
- The speaker count is capped at `Stream3DStereoMaxSpeakers` (**default 16**, see §12)

### 6.6 5.1ch venue placement (6 prims)

Deploy a 5.1ch source (Opus surround / FLAC 6ch) across six speaker prims:

```
Root Description:
  [3dstream:{url:http://example.com/test_5_1.flac}{range:30}]

FL prim:  [3dstream:{ch:FL}]
FR prim:  [3dstream:{ch:FR}]
C prim:   [3dstream:{ch:C}]
LFE prim: [3dstream:{ch:LFE}]
SL prim:  [3dstream:{ch:SL}]
SR prim:  [3dstream:{ch:SR}]
```

- Place each prim physically in the venue's "speaker positions" (front L/R of the stage, center, subwoofer, surround L/R)
- LFE is treated equivalently to the other 5 channels — no special processing (no low-pass filter, no 2D-ization). If you need LFE band-limiting, do it on the broadcaster side.
- If you want to align the layout to a cinema-like listening position, you can choose a reference point in the venue and place speakers around it. Since listeners can also walk freely in SL, perceived localization changes as they move away from that reference point. In that case, the setup behaves as a multi-point PA layout using the 5.1ch channels as spatial speaker feeds.

### 6.7 Media / MOAP source (r26 / r31 unified tag)

In r26, a Media-on-a-Prim (MOAP) face inside the linkset can be used as the 3D Stream source instead of an HTTP URL. In r31 and later, write `[3dstream:{source:media}]` or `[3dstream:{source:media-5-1}]` on the root Description, and put `[3dstream:{ch:...}]` on speaker prims.

```
Root Description:
  [3dstream:{source:media}{ch:L}]

Child Description:
  [3dstream:{ch:R}]
```

The media face may be on the root prim or on a child prim. If there is only one media face, `{link}` / `{face}` can be omitted. If the linkset has multiple media faces, explicitly select which face is routed to 3D Stream on the root tag.

```
Root Description:
  [3dstream:{source:media}{link:3}{face:2}{range:30}]

Speaker #1 Description:
  [3dstream:{ch:L}]

Speaker #2 Description:
  [3dstream:{ch:R}]
```

`{link:N}` is only a link number for selecting the **media source**. It does not affect speaker order or L/R assignment. Speaker roles are always determined by each prim's `{ch:...}`. If multiple media faces exist and `{link}` / `{face}` cannot identify one unambiguously, the result is a structural error.

URL sources and media display can coexist. However, `{url}` and `{source:media}` are mutually exclusive. If you want to show a media screen while placing a separate URL stream in 3D, write only `{url:...}` on the root and do not select the media face as the 3D Stream source. In that case, 3D Stream speakers play the URL stream, and media audio behaves as normal MOAP audio.

Volume behavior depends on the number of media faces. If there is exactly one media face, its media volume / mute works as source gain. If there are multiple media faces, the selected media routed to 3D is treated as source gain `1.0` and is controlled by 3D Stream master volume / speaker volume. Unselected media faces keep the normal media volume behavior.

Media source channel selection:

- `{source:media}` / `{source:media-stereo}`: treats media as a 2ch source. Use this for stereo media, including stereo media with `{upmix:on}`.
- `{source:media-5-1}`: treats media as a 5.1ch / 6ch source. Place `FL / FR / C / LFE / SL / SR` speakers.

This guide covers media sources up to **2ch and 5.1ch (6ch)**. Even if the Dullahan/CEF callback bus appears as 8ch, that does not mean 7.1ch speaker routing is implemented in 3D Stream.

### 6.8 How to identify the root prim

While editing a linkset, in the Build floater's **Object** tab the "Selected" indicator shows which prim is selected. The parent of the linkset (= root) is normally the **first prim selected when the linkset was originally linked**.

Most reliable confirmation:
- Build → Edit → "Edit linked" OFF → click any prim → the root of that linkset is selected
- LSL: `llGetLinkNumber()` returns `1` for the root (when child prims exist). For a single un-linked prim, it returns `0`.

The link order (link number 1, 2, 3, ...) of root vs children **does NOT affect speaker channel assignment**. The spec from r5 that used link number to determine L/R was retired in r8; from r8 onwards speaker routing is `{ch:...}` declaration based.

`{link:N}` can be used with `{source:media}` to select which prim's media face becomes the audio source. This link number is **only a media-source selector**; it still does not decide L/R/FL/FR speaker order.

---

## 7. Binaural / Venue Reverb (r12)

r10 3D Stream could already place dry material as speakers in the world. r12 adds tag keys for optional headphone localization correction and venue reverb inside the viewer DSP.

| Key | Short form | Default | Function |
|---|---|---|---|
| `binaural` | `bin` | `off` | Headphone localization enhancement using lite-HRTF (ITD + air absorption) |
| `venue` | `v` | `dry` | 9 venue reverb presets (convolution reverb) |
| `wetgain` | `wg` | `0.2` | Reverb wet-component gain (0.0-2.0, recommended range 0.1-0.5) |
| `lfegain` | `lg` | `1.0` | LFE channel gain multiplier (0.0-4.0, added in r12.1) |

These keys describe the sound of the source as a whole, so write them on the **root prim with the source declaration**. Writing them on child prims does not apply them per speaker; they are ignored. There is no normal listener UI for these values. The venue-side tag setting is what listeners hear (§7.5).

### 7.1 `{binaural:on|off}` (short-form `bin`)

Enables the **lite-HRTF DSP** that improves left/right localization for listeners using headphones.

#### Behavior

When `on`, each speaker channel gets:

- **ITD (interaural time delay)** — computed from the speaker direction with the Woodworth-Schlosberg approximation and applied as a sample-fractional delay. This is closer to "sound arriving from the left" than simply making the left ear louder.
- **air absorption (distance HF rolloff)** — high frequencies are reduced with distance (`-0.5 dB/m`, capped at `-25 dB`). A speaker 50m away sounds darker.

ILD (interaural level difference) remains handled by FMOD's `FMOD_3D_LINEARSQUAREROLLOFF`, as in r10 and earlier.

#### When to choose `off`

- The streamed material is already produced as headphone-oriented spatial audio
- The venue is mainly intended for listeners using speakers instead of headphones
- You want the plain distance and direction behavior of existing 3D Stream without additional localization processing

#### Why the default is `off`

To avoid changing existing placements when their tags are not edited. If a venue wants headphone-oriented localization enhancement, explicitly write `{bin:on}` or `{binaural:on}` on the root tag.

### 7.2 `{venue:NAME}` (short-form `v`)

Select one of 9 venue reverb presets. If the source already contains strong reverb, it will stack with `venue` reverb. If you want to tune the room sound on the 3D Stream side, keep the streamed source relatively dry.

#### Preset list

| Long form | Short form | RT60 | Use case | CPU (incremental) |
|---|---|---|---|---|
| `dry` | `d` | — | No reverb (= r10 behavior) | 0 (no DSP) |
| `room_small` | `rs` | 0.3 s | Small studio, bedroom | +0.1 pp |
| `room_medium` | `rm` | 0.6 s | Medium studio, talk show | +0.1 pp |
| `hall_small` | `hs` | 1.0 s | Small live house / small theater | ~+3 pp |
| `hall_medium` | `hm` | 1.5 s | Concert hall, ballroom | +7.7 pp |
| `hall_large` | `hl` | 2.0 s | Large hall, opera house | +9.6 pp |
| `club` | `cl` | 0.8 s | Dance club, dense early reflections | ~+5 pp |
| `cathedral` | `ct` | 3.0 s | Cathedral, long ambient tail | +10.2 pp |
| `outdoor` | `od` | 0.2 s | Outdoor, very light early reflections only | +0.1 pp |

#### Why the default is `dry`

Unspecified tags should play as before. When `dry` is selected, the reverb DSP is not inserted, so there is no CPU cost for reverb processing.

#### CPU note

`hall_medium` / `hall_large` / `club` / `cathedral` use longer impulse responses, increasing the partitioned FFT convolution cost. `cathedral` is about **+10.2 pp** versus r10 and consumes roughly 53% of one core. On multi-core CPUs this is around 3-7% total CPU, but for venues targeting lower-spec machines, start with `room_small` / `room_medium` / `outdoor`.

### 7.3 `{wetgain:N}` (short-form `wg`)

Multiplier on the **wet (reverb) component**. Range: 0.0–2.0. Default: **0.2** (changed from 1.0 in r12.1).

| Value | Effect |
|---|---|
| `0.0` | Full dry (= same as venue=dry, though the DSP remains inserted) |
| `0.1` | Very subtle wet |
| **`0.2` (default)** | Subtle wet — musically usable baseline |
| `0.3–0.5` | Moderate to fairly thick wet (top of the musical range) |
| `1.0` and above | Wet at parity-or-higher with dry — typically too saturated for music broadcasts |
| `2.0` | Wet at 200% — heavily-drowned ambient feel (rarely useful) |

#### Design notes

Each venue IR is **unity-gain normalized**, so `{wg:0.2}` keeps the wet/dry ratio stable when switching venues. However, presets with longer RT60 can still sound wetter because the tail lasts longer.

> **Default change in r12.1 (1.0 → 0.2)**: The original `1.0` ("wet at parity with dry") saturated the source on hall / cathedral presets and fell outside the musically usable range. Listening tests confirmed **0.1–0.5 is the practical musical range**, so the default was lowered to `0.2`. The bundled LSL UI quick-pick buttons were also re-graded to `0.1`–`0.5` in fine increments.

#### When `{venue:dry}` is used

When `venue` is `dry`, the reverb DSP is **fully bypassed**, so `wetgain` is **ignored**.

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

These four keys (`binaural` / `venue` / `wetgain` / `lfegain`) are specified by the root prim tag. General Preferences do not expose listener-side controls for them.

#### Why no listener UI?

- If every listener could change venue reverb and binaural settings independently, the same venue would sound substantially different per listener.
- AYAstorm keeps venue sound-shaping controls on the tag side instead of exposing them in the normal listener UI.

#### Exception: listener-side rescue for speaker viewing

For verification or recovery, Debug Settings can override these values. `Stream3DBinauralRender = 0` forces binaural off, `Stream3DVenueOverride = "dry"` forces reverb off, and `Stream3DVenueWetGain` / `Stream3DLfeGain` can override the corresponding gain values. These are debug controls, not normal Preferences controls.

### 7.6 Combination examples

#### Nothing written (= default)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}]
```
→ Equivalent to `{bin:off}{v:d}{wg:0.2}`. No additional binaural or venue reverb is applied; playback stays as dry 3D Stream.

#### Live house (PA-oriented, beat-driven music)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:0.3}]
```
→ Uses the club preset. `wg:0.3` adds a little more reverb than the default.

#### Large hall (orchestra)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.25}]
```
→ Uses hall_large with wet at 0.25x. The long hall tail is kept restrained.

#### Cathedral (ambient / environmental)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.15}]
```
→ Uses cathedral with wet at 0.15x so the long tail does not dominate the source.

#### Outdoor (environment / strolling BGM)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:0.2}]
```
→ Uses outdoor and adds light early reflections. `wg:0.2` is the default.

#### Already-binaural material (avoid double processing)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF, no reverb (= r10 baseline behavior).

---

## 8. stereo→5.1 upmix (r12)

r10 per-channel placement (six speakers: FL/FR/C/LFE/SL/SR) was built for **5.1ch broadcasts**. In SL, however, common streaming software such as butt, Mixxx, OBS, and SAM is usually stereo-oriented.

> **Tips:** If you want to broadcast in 5.1ch, you can use [SurroundStreamer](https://github.com/t-noami/SurroundStreamer), created by t-noami, a contributor to this repository.

The `{upmix:on}` key added in r12 expands **stereo 2ch to 6ch** inside the viewer DSP. This lets a 6-speaker layout play from a stereo broadcast without changing the broadcaster side to native 5.1ch.

### 8.1 `{upmix:on|off}` (no short-form)

Whether to upmix a stereo source into 5.1.

| Value | Meaning |
|---|---|
| **`off` (default)** | Upmix disabled. A stereo source plays through `{ch:L}`, `{ch:R}`, and `{ch:M}` routes as before |
| `on` | Convert a stereo source to 6ch and route it to `{ch:FL}` `{ch:FR}` `{ch:C}` `{ch:LFE}` `{ch:SL}` `{ch:SR}` prims |

#### Why opt-in (default off)?

Upmix DSP uses CPU and changes the image, so omitted tags preserve the existing stereo behavior. To use 6-speaker expansion, explicitly write `{upmix:on}`.

#### Effect on existing layouts

For 6-speaker linksets created in r8/r10, adding `{upmix:on}` to the root tag expands a stereo source to 6 speakers. With `{upmix:off}` or no `upmix` key, behavior stays as before.

### 8.2 Algorithm (matrix upmix + band separation)

The upmix is a fixed algorithm that derives center / surround / LFE components from a stereo source. It combines `(L+R)` / `(L-R)` matrix processing with band separation and rear delay. Broadcasters only choose `on` or `off`; there is no tag for selecting an algorithm.

| Output channel | Derived from |
|---|---|
| `C` (center) | `(L+R)/sqrt(2)` in-phase component |
| `Ls` / `Rs` (rear) | `(L-R)/sqrt(2)` decorrelated with fixed 16ms delay +/- jitter |
| `LFE` | `(L+R)` through an 80Hz low-pass filter |
| `FL` / `FR` (front) | `L` / `R` with center component removed by `bleed_amount` |

Additional processing:

- **LFE LPF**: sends only the low band to the LFE speaker
- **Center bleed removal**: avoids a double phantom center from center speaker plus front L/R
- **Rear decorrelation**: separates surround L/R by a small time difference

Machine-learning upmix is not used. The implementation prioritizes predictable output for a given input.

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
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0–1.0 | Fraction of center component subtracted from front L/R (`0` = no removal, `1` = full removal) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0–32 | Rear decorrelation base delay (L / R differ by ±2 ms jitter) |

For a listener-side force OFF / ON, one sentinel:

| Debug setting | Default | Meaning |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = follow tag) | `0` ignores the tag and forces OFF / `1` forces ON (5.1-native auto-bypass still applies) |

### 8.5 Combination examples

#### Default behavior (no upmix)

```
Root Description:
  [3dstream:{url:http://example/stereo.ogg}{ch:L}]

Child Description:
  [3dstream:{ch:R}]
```
→ The stereo source L/R is routed to two speaker prims. Without `{upmix:on}`, C / LFE / SL / SR are not generated.

#### 6-spk placement + upmix

```
Root Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ Stereo source is expanded to 6 channels and routed to the 6 speaker prims.

#### upmix + binaural + venue

```
Root Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:0.25}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ Combines 6-speaker upmix with lite-HRTF and hall_medium reverb. `wg:0.25` is a restrained reverb amount for hall_medium.

#### `{upmix:on}` on a 2-speaker layout

```
Root Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{ch:L}]
Child: [3dstream:{ch:R}]
```
→ `ch:L` / `ch:R` are treated as FL / FR equivalents, but C / LFE / SL / SR have no destination prim and do not play. For a 2-speaker layout, `{upmix:on}` is normally unnecessary. Use the 6-speaker layout above if you want center or surround output.

#### Putting upmix on a 5.1-native broadcast (auto-bypass)

```
Root Description:
  [3dstream:{url:http://example/stream_5_1.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ When a 6ch source is detected, upmix is auto-bypassed and Local Chat shows one notification. Each speaker prim plays the corresponding native 5.1 source channel directly.

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
| **Opus (Ogg)** | ✓ | ✓ | ✓ | 6ch uses Opus channel mapping family 1. Verified with the r9-opus completion path |
| **FLAC** | ✓ | ✓ | △ | Codec layout is implemented; delivery paths may have seek constraints |
| AAC (ADTS / HLS) | — | — | — | Not supported |
| AC-3 / E-AC-3 | — | — | — | Not supported |

Source URLs may be `http://` or `https://`. A path that maintains HTTP/1.1 keep-alive (= a SHOUTcast-compatible streamer or ffmpeg's TCP output) tends to be more stable than plain static HTTP.

### 11.2 1ch / 2ch streaming

For 1ch / 2ch sources, normal SHOUTcast, Icecast, and static HTTP delivery are all usable. MP3, Vorbis, Opus, and FLAC are supported, so existing tools such as `oggenc`, ffmpeg, and butt can be used.

### 11.3 5.1ch (Vorbis / Opus 6ch) streaming

For 5.1ch streaming, use **Vorbis 6ch** or **Opus 6ch**. Both have been verified on the viewer side. Musicians and DJs who want to stream Opus 6ch can use [SurroundStreamer](https://github.com/t-noami/SurroundStreamer), created by t-noami. The following example uses Vorbis 6ch because it is easy to create test material with ffmpeg.

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

### 11.4 Opus 6ch and FLAC 6ch handling

Opus 6ch is decoded through the r9-opus codec plugin path, so normal Ogg/Opus 6ch delivery does not use the FMOD parser path that caused the seek issue. Opus channel mapping family 1 6ch sources have been verified over Icecast. For live 5.1ch streaming, Opus 6ch is a practical choice.

FLAC 6ch has the codec layout implemented, but the FLAC parser may require seeking. If the delivery route cannot seek, it may fail to open with `FMOD_ERR_FILE_COULDNOTSEEK`. Verify the actual delivery path before using FLAC 6ch.

Operational guidance:

- Prefer **Opus 6ch** for live 5.1ch streaming
- Musicians / DJs can use **SurroundStreamer** for Opus 6ch streaming
- **Vorbis 6ch** is convenient for static verification files
- Use FLAC 6ch only after confirming that the delivery path can seek when needed

### 11.5 Choosing a broadcast tool

| Tool | Use case | Notes |
|---|---|---|
| **SurroundStreamer** | Opus 6ch 5.1ch streaming | For musicians / DJs. Created by t-noami. See [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) |
| **ffmpeg** | Test material / Vorbis 6ch streaming / codec conversion | CLI required; useful for tests and automation |
| **butt** | 1ch / 2ch live streaming | Not used for 5.1ch streaming |
| **Liquidsoap** | Broadcast automation / server-side processing | Complex configuration. Confirm that 6ch is preserved before deployment |
| **Mixxx / DarkIce / ezstream** | DJ / automation | Stereo-oriented; not used for 5.1ch streaming |

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

As described in §7.5 / §8.4, `binaural` / `venue` / `wetgain` / `lfegain` / `upmix` are specified by tag. They do not have normal Preferences UI, but Debug Settings can override them for verification or individual adjustment.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = follow tag) | `0` to force OFF on the listener side / `1` to force ON. If this is `-1` and `{binaural}` is omitted, the result is off (details in §7.5 Exception) |
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
  Example: [3dstream:{ch:L}{range:30}]
```

```
3D Stream: Structural error (linkset root: "MainStage")
  Source declaration found on root, but no speakers (ch) found.
  Each speaker prim should have [3dstream:{ch:L|R|M}].
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
2. **Tag spelling**: Confirm the prefix contains `[3dstream:`. Legacy layouts also accept `[3dstream-stereo:` / `[ayastream-stereo:` / `[ayastream:` (watch for typos)
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

- §11.4 seek limitation: this can occur when FLAC 6ch is delivered over a non-seekable path. Switch to **Vorbis 6ch / Opus 6ch**, use SurroundStreamer for Opus 6ch, or deliver FLAC through a seek-capable path.
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

- Description changes normally apply on the next polling pass (`Stream3DPollInterval`, default 30 seconds; §4.6).
- For manual edits, saving the Description again sends a Properties notification and triggers re-evaluation.
- If it still does not stop, temporarily set `Stream3DEnabled` to `false` to force-release all bindings, then set it back to `true` if needed.

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
| Opus (Ogg) | ✓ verified end-to-end | ✓ verified end-to-end (r9-opus completion, Opus channel mapping family 1) |
| FLAC | ✓ verified end-to-end | △ codec layout implemented; delivery paths may have seek constraints |
| MP3 | ✓ verified end-to-end | — |

For 5.1ch streaming, choose **Vorbis 6ch** or **Opus 6ch**. Opus 6ch is practical for live streaming; Vorbis 6ch is convenient for static verification files.

### 15.5 No special LFE handling

5.1ch's LFE (subwoofer) is treated equivalently to the other 5 channels — no low-pass filter, no 2D-ization. It is 3D-positioned and distance-attenuated like the rest. If LFE band-limiting is needed, do it on the broadcaster mix.

The intended pattern is: place a physical subwoofer-shaped prim in SL at the appropriate position and have low-frequency audio emit from there.

### 15.6 5.1ch in a free-camera world

5.1ch sources are usually mixed with a reference listening position in mind. In AYAstorm, you can place speakers around a specific position in the venue, similar to seats in a cinema.

At the same time, listeners in SL can move freely. As they move away from that reference point, the perceived localization changes from what the mix assumed. 3D Stream 5.1ch layouts can be used for fixed-seat listening, but they also behave as multi-point PA layouts where each 5.1ch channel is placed as a speaker in the venue.

### 15.7 Behavior in other Viewers

`[3dstream:...]` tags, and compatible legacy prefixes `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]`, are **AYAstorm-specific**. Mainline Firestorm, the official Second Life Viewer, Catznip, and other viewers ignore them entirely.

- AYAstorm users: 3D-positional audio plays as designed
- Other Viewer users: the tag only appears as part of the description text, and `{url:...}` 3D Stream audio does not play. Parcel music remains audible if configured
- With `{source:media}`, the media face itself still displays and plays as normal MOAP in other viewers. 3D Stream speaker layout, `{ch:...}` routing, upmix, binaural, and venue reverb do not apply
- If a media screen is shown while `{url:...}` 3D Stream audio is used, other viewers only get the normal MOAP side; the `{url:...}` 3D Stream audio does not play

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

For media/MOAP sources, media volume can also act as source gain in the single-media-face case. See §6.7 for the exact media volume rule.

---

## 16. Static Occlusion `[ayastorm:occlude]` (r13)

A **venue operator / builder** tag. Marking wall / door / floor / ceiling prims with this tag makes AYAstorm treat them as **sound-blocking geometry** when the listener-to-source segment passes through them — the audio becomes muffled (less volume + lowpass-coloured).

Whereas `[3dstream:...]` (§5 / §6) is a **sound-emitting** tag, `[ayastorm:occlude]` is a **sound-blocking** tag. The two are entirely independent — a prim tagged only with `occlude` does not emit any audio.

### 16.1 Syntax

```
[ayastorm:occlude]                            ← defaults (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← explicit values
[ayastorm:occlude{direct:0.6}]                ← one key only (other uses default)
```

The legacy prefix (`[ayastream:occlude]`) is **not accepted** — occlusion was introduced in r13 and there are no legacy ayastream prims to preserve. Common syntax rules (§4.3, case-insensitive keys / whitespace-trimmed values / unknown keys silently ignored) apply.

### 16.2 Behaviour model

#### What gets occluded

- **`[3dstream:...]` audio** (positional streams, per speaker prim)
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
| `docs/specs/3dstream-user-guide.en.md` | 3D Stream user guide — placement steps, streaming formats, SurroundStreamer, and other-viewer fallback |
| `docs/specs/spec_stream3d_decode_thread.md` | The 3-thread model established in r7 |
| `docs/specs/spec_distributed_stereo.md` | r8 distributed stereo spec — legacy `[3dstream-stereo:...]` field syntax |
| `docs/specs/ayastorm-r31-3dstream-unified-tag.md` | r31 3D Stream unified tag spec — unification to `[3dstream:...]`, `{ch}`-based linkset routing promotion, legacy prefix compatibility |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch source ingestion spec — Opus/FLAC 6ch decode path + BS.775 downmix |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch venue placement spec — `ch=FL/FR/C/LFE/SL/SR` extension + compatibility matrix |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 binaural + venue reverb spec (bundled into r12 release) — lite-HRTF / 9 venue presets / wetgain details |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 upmix spec — matrix upmix + band-separation algorithm details |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase breakdown (P0-P11) and effort estimate |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB occlusion spec + implementation record — `[ayastorm:occlude]` design decisions / spike implementation / remaining work |
| `docs/ayastorm-stream3d-roadmap.md` | Overall 3D Stream roadmap |

---

## Revision History

- **2026-05-05 (initial)**: Compiled as the final spec at r10. Cumulative spec from r5 / r8 / r9 / r10 / r10.x. r11 and later not covered (unreleased).
- **2026-05-08 (r12)**: Added §4.5 (short-forms), §7 (Binaural / Venue Reverb), §8 (stereo→5.1 upmix). r11 is bundled into r12 (not released independently — to avoid two-step tag-format change confusion). Renumbered later sections (§7–§14 → §9–§16). §12.2 lists r11/r12 listener-side sentinel debug settings; broadcaster-driven model preserved (no general Preferences UI).
- **2026-05-09 (r12.1)**: Added new §7.4 `{lfegain:N}` (short-form `lg`); renumbered the prior §7.4 Broadcaster-driven model to §7.5 and the prior §7.5 Combination examples to §7.6. Lowered the `wetgain` default from `1.0` to `0.2` to reflect the practical musical range (0.1–0.5) confirmed by listening tests. Added the `Stream3DLfeGain` sentinel to §12.2 and a live-tuning fix note in §12.2 / §12.3 (covering the r12 regression where `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` only took effect after a prim touch).
- **2026-05-11 (r13)**: Added new §16 Static OBB Occlusion `[ayastorm:occlude]`; renumbered the prior §16 Related Documents to §17. Extended §4.1 from "Two tag types" to "Three tag types". Documented the r13 debug settings (`Stream3DOcclusion` master sentinel / `Stream3DOccluderRange` distance cull / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` overlay) inside §16.6-§16.8. Added `docs/ayastorm-r13-occlusion.md` row to the §17 spec table.
- **2026-05-11 (r13 P15)**: Upgraded occlusion from OBB approximation to **exact-shape triangle raycast** (OBB pre-cull + Möller-Trumbore, see §16.2). Path Cut / Hollow / Mesh now feed real geometry into the audio calculation. Corrected the multi-occluder section to describe **multiplicative pass-through** (the implementation was always multiplicative; older docs misdescribed it as `max`). Changed `Stream3DShowOccluders` from OBB wireframe to **cyan triangle mesh** (translucent fill + wireframe), with live tracking while a prim is selected in the build floater (§16.8). Added the 2000-tri per-occluder cap and OBB-only fallback rules in §16.9. Shortened §16 title from "Static OBB Occlusion" to "Static Occlusion".
- **2026-05-17 (r26)**: Added media/MOAP source routing. Added root source-selection keys `{source:media}`, `{link:N}`, and `{face:N}` in §3 / §6, plus media/MOAP examples, URL + media display coexistence, media volume / mute rules, and media callback channel notes up to 5.1ch.
- **2026-05-24 (r31)**: Unified the recommended new tag syntax as `[3dstream:...]`. Documented that `[3dstream:{url:...}]` remains single-prim URL playback when the linkset has no `{ch}`, but becomes a linkset-layout source declaration when the same linkset contains `{ch}`. Clarified that `[3dstream:{source:media...}]` is linkset routing only and must be combined with `{ch}` speakers, and that routing mode is decided by the presence of `{ch}`, not by the presence of child prims. Legacy `[3dstream-stereo:...]` / `[ayastream-stereo:...]` prefixes are compatibility prefixes.
- **2026-05-24 (r31 doc revision)**: Changed omitted `binaural` default to `off` and updated §7 plus the Debug Settings table. Removed trademarked naming from the upmix explanation and described it as matrix upmix + band separation. Updated §11 / §15.4 codec status after r9-opus completion: Opus 6ch is verified, while FLAC 6ch may have seek constraints depending on delivery path. Added SurroundStreamer as a practical Opus 6ch streaming route. Revised §15.6 free-viewpoint 5.1ch wording, §15.7 other-viewer / MOAP fallback behavior, and §14 troubleshooting text.
