[![Download](https://img.shields.io/github/v/release/mayatonton/phoenix-firestorm?label=⬇%20Download&style=for-the-badge&color=blue)](https://github.com/mayatonton/phoenix-firestorm/releases/latest)

**English** | [日本語](README.ja.md) | [中文](README.zh.md)

<img align="left" width="100" height="100" src="indra/newview/icons/ayastorm/ayastorm_512.png" alt="AYAstorm logo"/>

**AYAstorm is a custom Second Life viewer based on [Firestorm](https://www.firestormviewer.org).**
It adds rendering enhancements, UI improvements, and first-class Japanese language support.

---

## Features

### AYAstorm Controls

Open from the **AYAstorm** menu (between Build and Help) → `AYAstorm Controls...`, or press `Alt+C`.

![AYAstorm Controls](docs/images/ayastorm_controls.png)

A single floater that surfaces AYAstorm's preset master switches and 11 rendering categories on the left tab strip. Every control has a `D` button to snap back to the AYAstorm preset default, so you can live A/B-tune without hunting through debug settings.

- **General** — Master switches for SSAO / Glow & Bloom / Render Attached Particles, plus the Reflection probe quality slider. Use this tab as the top-level on/off panel before diving into per-effect tuning

- **Shadows** — Shadow Detail level (off / sun only / sun + spot + projector), automatic vs. manual cascade distance, shadow blur size, resolution scale, and shadow far clip. Lets you balance softness against shadow draw distance

- **SSAO** — SSAO master toggle (mirrors General tab), `Blur deferred lights` toggle, and AO tuning sliders. Adjust contact shadow strength and falloff

- **DoF & Camera** — Depth of Field master, **High-quality DoF (4× CoF, depth-gated)**, front-of-focus blur, alpha-blended-surface depth inclusion, plus camera-side AYAstorm preset values (focal length / aperture / etc.) and a DoF-coupled chromatic aberration strength slider

- **SSR** — Screen Space Reflections master toggle and six quality-tuning sliders (step count, max distance, etc.). Adds wet-floor / glass reflections on top of the deferred opaque buffer

- **Motion Blur** — Motion Blur master toggle, separate `Blur self avatar` and `Blur other avatars` switches, plus an Antialiasing sub-section

- **Glow & Volumetric** — Glow / Bloom intensity tuning, **Volumetric Lighting (AYA godrays)** with directional fade (sun-below-horizon), and a separate **Godrays (sun-facing beam)** pass for the brighter on-axis variant

- **Lighting** — Enable fullbright textures (global), render in-world point/spot lights, render own attached lights, render others' attached lights — the four switches that decide how much the local-light pipeline contributes to the scene

- **Post-process** — Contrast Adaptive Sharpening (CAS) and Post FX sliders, applied as the final post-process pass over the framebuffer

- **Atmosphere & sky** — Sky depth & sun glare, distant blue haze, and a morning blue / evening warm tint — AYAstorm View's signature atmosphere shaping on top of EEP

- **Skin SSS** — Subsurface Scattering for skin translucency: master switch, blur radius / strength / etc. parameters, and a target-mesh-UUID whitelist with `Lock editing (prevent accidental changes)` and `Reset all to defaults`

> Tip: change the **View Mode** (Firestorm / AYAstorm View) in Preferences → Graphics. Restart the viewer after switching to apply.

### Rendering

Configurable from Preferences → Graphics → Rendering tab.

![Preferences - Rendering](docs/images/preferences_graphics_rendering.png)

- **Shadow Softness** — New slider to soften shadow edges

- **Selectable Tone Mappers** — Upstream Firestorm hard-codes Khronos Neutral internally; AYAstorm exposes a UI selector with five options:
  - Khronos Neutral / ACES / Filmic (Uncharted 2) / Uchimura (GT) / Filmic (BD Style)

- **Color Grading Controls** — Adjustable Saturation, Contrast, Color Temperature, and Brightness sliders (with a `Reset Color Grading` button) added to the UI

- **Color LUT (.cube) Loading** — Apply 3D LUT files (`.cube`) for post-process color grading. Seven presets are bundled (`teal_orange`, `warm`, `cold_war`, `sepia`, `cool`, `cinematic`, `film_noir`), but the primary goal is to let **users load their own `.cube` files to fully customize the look of the viewer**. Pick a LUT via `Browse...` and adjust `LUT Intensity` to taste

### Parcel

Configurable from Preferences → Firestorm → Build 2 tab.

![Preferences - Firestorm Build 2](docs/images/preferences_firestorm_build2.png)

#### Visitor side — Hide objects outside the parcel

- **`Hide objects outside your parcel`** — Don't render objects outside the parcel you're currently standing on. Works on **any parcel** — handy when taking screenshots and you want to clear away neighbouring prims or signs from the background. Avatars / attachments / HUDs / your own objects can be kept visible via `Keep avatars visible` / `Keep my own objects visible`

#### Parcel-owner side — Forced hiding via description tag

Just write the tag below into the **parcel description** and any **AYAstorm visitor** to that parcel will have the hide-outside behaviour forced on. **Other viewers (upstream Firestorm, official LL viewer, etc.) ignore the tag entirely**, so it functions as a "privacy tag that only affects AYAstorm users". As parcel owner you can set it without any cooperation from visitors.

**Tag format:**

```
[parcelhide:{key:value}{key:value}...]
```

- Can appear anywhere in the description (other text before/after is fine)
- The legacy `[AYAstorm:...]` form is still accepted (renamed to `[parcelhide:...]` in r5)

| Key | Default | Behaviour |
|---|---|---|
| `hideoutside` | `true` | `false` temporarily disables the tag |
| `keepavatars` | `false` | `true` keeps avatars & HUDs visible |
| `keepownobject` | `false` | `true` keeps the visitor's own objects visible |
| `altitude` | (none) | `min-max[,min-max...]` — fires only when your Z (altitude) falls inside any of the listed ranges (both endpoints inclusive, hyphen-separated, comma-separated for multiple ranges). E.g. hide only at a specific skybox floor for photo work |

**Examples:**

| String in description | Effect |
|---|---|
| `[parcelhide:]` | Hide everything outside the parcel (avatars and own objects also hidden) |
| `[parcelhide:{keepavatars:true}]` | Avatars stay visible, other objects hidden |
| `[parcelhide:{keepavatars:true}{keepownobject:true}]` | Common-sense default |
| `[parcelhide:{hideoutside:false}]` | Temporarily disable tag (during events etc.) |
| `[parcelhide:{altitude:1000-2000,3000-4000}]` | Hide only when at altitude 1000-2000m or 3000-4000m (e.g. specific skybox floors) |

**Effect comparison:**

<table>
<tr>
<td width="50%" align="center"><b>Without tag</b><br/>(normal rendering)</td>
<td width="50%" align="center"><b>With tag</b><br/>(<code>[parcelhide:...]</code> in description)</td>
</tr>
<tr>
<td><img src="docs/images/parcel_magic_off.png" alt="Without tag"/></td>
<td><img src="docs/images/parcel_magic_on.png" alt="With tag"/></td>
</tr>
</table>

### Audio

#### 3D Stream — 3D-positional audio streaming from prims

Plays HTTP audio streams (SHOUTcast / Icecast / static MP3 / Vorbis / Opus / FLAC) as **3D-positional audio from prim locations**. Unlike SL's standard parcel-level 2D BGM, sound direction and distance update in real time as the listener moves. Use cases: **live venue PA / ambient audio / multi-speaker venues / 5.1ch venue deployment**. Configure entirely by **writing tags into a prim's Description field** — no LSL script required.

**Minimum example:**

```
[3dstream:{url:http://example.com/stream.mp3}]
```

The prim with this tag plays the stream as 3D-positional audio.

**Stereo / 5.1ch / multi-speaker support:**

Assign `[3dstream-stereo:{ch:L|R|M|FL|FR|C|LFE|SL|SR}]` to prims in a linkset to split L/R across separate prims or deploy a 5.1ch source onto 6 prims. The full reference (all keys, compatibility matrix, broadcaster recipes, troubleshooting) is in the per-language guides below.

**Full reference:**

- 🇯🇵 [3D Stream タグ書式ガイド (日本語)](docs/guides/3dstream-tag-guide.ja.md)
- 🇬🇧 [3D Stream Tag Format Guide (English)](docs/guides/3dstream-tag-guide.en.md)
- 🇨🇳 [3D Stream 标签格式指南 (简体中文)](docs/guides/3dstream-tag-guide.zh.md)

### Chat UI

Configurable from Preferences → Chat → Chat Windows tab.

![Preferences - Chat Windows](docs/images/preferences_chat_chatwindows.png)

- **Ported LL-style Chat Window** — Upstream Firestorm's Nearby Chat already offers the capable `FS V1 (plain text)` and `FS V7 (modern headers)` styles, but **seeing who is within chat range requires opening a separate window**. AYAstorm adds a new **`LL style`** that ports the look of Linden Lab's official viewer's CONVERSATIONS window, **letting you see avatars within chat range directly inside the chat window** — no extra panel needed

- **Profile Icons Next to Speakers (`Show mini icons in chat`)** — Show each speaker's profile icon next to their name on every chat line, so you can **intuitively tell who said what** instead of squinting at name strings alone

  ![Mini icons in nearby chat](docs/images/neaby_chat_miniicon_exapmpleshot.png)

- **Chat Range Participant Filter** — Show only avatars within chat range (20m) in the nearby chat list

### Japanese Support

- **Fixed IME Candidate Window Position on Linux + Mozc + Fcitx5** — Upstream Firestorm has a critical bug on Linux where the **conversion candidate window jumps to the bottom-left of the screen, making Mozc + Fcitx5 essentially unusable**. AYAstorm fixes this so candidates appear directly under the caret as expected. Especially impactful for Japanese-speaking Linux users

- **Four Bundled Japanese Font Families** — Japanese text renders correctly out of the box, no extra install required. Switch between them in Preferences:
  - **Noto Sans JP** — Google Noto's Japanese sans-serif. Neutral and versatile, the default recommendation
  - **IBM Plex Sans JP** — IBM's open-source corporate font. Slightly geometric, modern feel
  - **Alibaba Sans JP** — Alibaba's open-sourced sans-serif. Soft and friendly
  - **LINE Seed JP** — LINE Corp's open-source font. Contemporary with a touch of character

- **OTF Font Support Fix** — OTF format fonts (Alibaba / IBM Plex / LINE Seed JP above are OTF) now load correctly

---

## Documentation

### User guides

- **Skin SSS — avatar photography**: [🇺🇸 en](docs/specs/skin-sss-user-guide.md) · [🇯🇵 ja](docs/specs/skin-sss-user-guide.ja.md) · [🇨🇳 zh](docs/specs/skin-sss-user-guide.zh.md)
- **3D Stream tag format**: [🇺🇸 en](docs/guides/3dstream-tag-guide.en.md) · [🇯🇵 ja](docs/guides/3dstream-tag-guide.ja.md) · [🇨🇳 zh](docs/guides/3dstream-tag-guide.zh.md)

### Open technical specifications (for other SL viewer developers)

SL-viewer-wide bugs and AYAstorm's fixes — published as open specs for free adoption by any LL-derived viewer fork. No PR required, pull what you need.

- **Rigged Mesh Picker — GPU object-ID buffer**: [🇺🇸 en](docs/specs/rigged-mesh-picker-gpu-buffer.md) · [🇯🇵 ja](docs/specs/rigged-mesh-picker-gpu-buffer.ja.md) · [🇨🇳 zh](docs/specs/rigged-mesh-picker-gpu-buffer.zh.md)
- **Double Alpha Block — forward alpha BLEND fix** (separate disclosure branch): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.md) · [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.ja.md) · [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.zh.md)

---

## Download

Pre-built binaries are available from **[GitHub Releases](https://github.com/mayatonton/phoenix-firestorm/releases/latest)**.

| OS | File | How to use |
|----|------|------|
| Windows (x64) | `Phoenix-FirestormOS-AYAstorm-release_AVX2-*_Setup.exe` | NSIS installer. Download and run |
| Linux (x64) | `Phoenix-FirestormOS-AYAstorm-release_LEGACY-*.tar.xz` | Extract anywhere and run the bundled `install.sh` |
| macOS | (Coming soon) | — |

> **For older CPUs without AVX2 (Windows only)**: If you run the AVX2 build above, the installer will show a message before installation begins. Download `Phoenix-FirestormOS-AYAstorm-release_LEGACY-*_Setup.exe` instead. AVX2 is supported on most Intel / AMD CPUs from 2013 onward, so the AVX2 build works for most users.

**Linux install example:**

```bash
tar xf Phoenix-FirestormOS-AYAstorm-release_LEGACY-*.tar.xz
cd Phoenix-FirestormOS-AYAstorm-release_LEGACY-*/
./install.sh
~/ayastorm/ayastorm
```

---

## Build Instructions

AYAstorm-specific build guide (Linux / Windows):

- [AYAstorm Build Guide](docs/build/building_ayastorm.md)

Upstream Firestorm build guides (refer to these for Mac):

- [Windows](doc/building_windows.md)
- [Mac](doc/building_macos.md)
- [Linux](doc/building_linux.md)

---

## Contributors

### AYAstorm Team

<table>
  <tr>
    <td align="center">
      <a href="https://github.com/mayatonton">
        <img src="https://github.com/mayatonton.png?size=100" width="80" height="80" alt="mayatonton"/>
        <br/>
        <sub><b>mayatonton</b></sub>
      </a>
      <br/>
      <sub>Creator / Maintainer</sub>
    </td>
    <td align="center">
      <a href="https://github.com/t-noami">
        <img src="https://github.com/t-noami.png?size=100" width="80" height="80" alt="t-noami"/>
        <br/>
        <sub><b>t-noami</b></sub>
      </a>
      <br/>
      <sub>Co-Maintainer</sub>
    </td>
    <!--
    To add another contributor, copy a <td> block above and update:
      - the GitHub username in the URL and image src
      - the display name in <sub><b>...</b></sub>
      - the role text in the trailing <sub>...</sub>
    -->
  </tr>
</table>

Want to help build AYAstorm? Bug reports, PRs, and translations are all welcome — see [CONTRIBUTING.md](CONTRIBUTING.md).

### Built on Firestorm

AYAstorm is a fork of [Phoenix Firestorm](https://www.firestormviewer.org), which is itself an open-source viewer derived from the official [Second Life](https://github.com/secondlife/viewer) client, licensed under LGPL.

Huge thanks to the Firestorm team and all upstream contributors whose work AYAstorm builds upon:

<a href="https://github.com/FirestormViewer/phoenix-firestorm/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=FirestormViewer/phoenix-firestorm" alt="Firestorm contributors" />
</a>
