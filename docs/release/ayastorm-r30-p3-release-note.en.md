# AYAstorm r30 P3 — Release Announcement

**r30 P3 introduces volumetric lighting (godrays / crepuscular rays) into Cinematic mode** — adding the radial light shafts and atmospheric haze that appear when sunlight grazes building edges and tree branches, as the expressive layer of the r30 (cinematic rendering) chapter.

Implementation traces, modification points, acceptance observations (including the 4-stage shader canary bisect), and upstream reference lines live in the permanent doc (`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`). This note is the entry point and diff highlight.

---

## AYAstorm r30 P3 — Volumetric Lighting (Godrays)

### Headline: making sunlight scatter through air

The r30 chapter (cinematic rendering) landed per-object motion blur and SMAA T2x in P2 as the Cinematic mode foundation. P3 lands **volumetric lighting (godrays)** on top.

Godrays (crepuscular rays / "god rays" / 薄明光線) are the radial shafts of light you see when sunlight is partially occluded by something — a roof edge, a tree branch, a window blind — and the unblocked light scatters off dust and humidity in the air. AYAstorm r30's core thesis (`docs/specs/ayastorm-r30-cinematic-chapter.md`) is "an atmosphere and space worth photographing," and this step makes light itself part of that material substance.

In Standard / Realism (AYAstorm View) modes the hook is not dispatched at all, so additional cost is zero.

### How it works

Inserted into `renderFinalize()` as a post-process pass between `generateGlow()` and `combineGlow()`:

```
deferredScreen (final color after lighting)
  → renderVolumetric(src, dst)
       ├─ gated by Cinematic mode + RenderVolumetricLighting
       ├─ volumetricLightF.glsl marches along the sun direction (16 samples default)
       ├─ depth weighting (pow(depth, 100)) so only sky pixels contribute
       ├─ composited as atmospheric scattering via haze_weight + sunlight_color
       └─ GODRAYS_FADE permutation (optional, can be disabled) keeps shaftify only when the sun is in front of the camera
  → ping-pong swap → handed to combineGlow
```

Shadow sampling reuses the AYAstorm/Firestorm standard `sampleDirectionalShadow` from `shadowUtil.glsl`, with `sun_dir` as a surrogate normal so PCF bias still works (mid-air sample points have no real surface normal — same trick as `class1/deferred/godraysF.glsl`).

### Settings

**No user action required for normal use.** Launching Cinematic mode (`AYAVisualRealismEnabled = 2`) automatically enables volumetric lighting. The tuning cvars:

| Cvar | Default | Purpose |
|---|---|---|
| `RenderVolumetricLighting` | `1` (ON) | Master switch for the godrays composite. Only active in Cinematic mode |
| `RenderVolumetricLightingResolution` | `16` | Number of shadow march samples along the sun direction. Higher = smoother shafts, linear GPU cost increase |
| `RenderVolumetricLightingMultiplier` | `50.0` | Shaft intensity. `50` for "close to real-world" natural crepuscular rays. `100` for clearly assertive shafts, `200` for a strong PV / cinematic look. BD's default of `1.0` is near-imperceptible under AYAstorm's ACES tone mapping + HDR scene buffer (see spec §8.2 acceptance observation) |
| `RenderVolumetricLightingFalloffMultiplier` | `1.0` | Distance falloff strength. Higher values fade godrays out faster in the distance |
| `RenderVolumetricLightingDirectional` | `1` (ON) | Gate that keeps shaftify only when the sun is in front of the camera (GODRAYS_FADE permutation). Turning OFF lets godrays bleed everywhere even when the sun is offscreen (visually: light beams hang in mid-air and building silhouettes are eaten by the wash — unnatural). Toggling this **requires a viewer restart** (permutation change) |

### What it looks like

- **Time of day**: midday to early afternoon (sun high) is where it shows strongest. SL's sunlight clamp dims `sunlight_color` toward evening, so godrays also get dim (limitation of BD's physical approximation)
- **Composition**: put the sun near screen center with building edges / branches / fine twigs silhouetted in front of it. Edges that partially occlude the sun produce the strongest shafts
- **Mode**: Cinematic only. Standard / AYAstorm View do not run this path at all

### Migration note

- Cinematic mode now supports volumetric lighting
- No effect on Standard / Realism modes (the hook is not dispatched)
- Switching between Cinematic and other modes **requires a viewer restart** (design fixed in r30 P1)
- Toggling `RenderVolumetricLightingDirectional` also **requires a viewer restart** (shader permutation change)
- This uses an independent shader program (`gVolumetricLightProgram`) separate from the existing Linden / Firestorm `gDeferredGodraysProgram`, so no setting collision occurs

### Known limitations

- **Default Multiplier diverges from BD**: BD shipped at `1.0` and saw the effect; AYAstorm needs `50.0` to reach the same visual range. Presumed cause: AYAstorm's ACES tone mapping + HDR scene buffer compress the additive contribution more than BD's sRGB direct-write path. BD's original on-screen behavior at `1.0` has not been verified on real hardware
- **GODRAYS_FADE window is narrow**: with the default (`Directional=1`) the sun must be within roughly 30° of screen center for shaftify to survive. Disable `Directional` (restart required) if you want godrays to appear with the sun offscreen
- **Sky pixels only**: the `depth *= pow(depth, 100.0)` weighting drops ground and near-field depth to zero, so godrays appear primarily on sky pixels (atmosphere). Ground shadows extending in shafts of light are not produced (could be revisited in a later phase)
- **macOS / Windows runtime verification**: AYAstorm Linux build verified; macOS / Windows Release binary verification will be performed when the tag is cut

### Implementation summary

- Shaders (`indra/newview/app_settings/shaders/class3/deferred/`):
  - `volumetricLightF.glsl` (borrowed from Black Dragon Viewer; AYAstorm swaps the shadow helper to `sampleDirectionalShadow` and adds a `HAS_SUN_SHADOW` permutation gate)
  - Vertex shader reuses the existing `deferred/postDeferredNoTCV.glsl`
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: new `renderVolumetric()` (Cinematic gate + ping-pong), `renderFinalize()` hook
  - `llviewershadermgr.{cpp,h}`: `gVolumetricLightProgram` extern + register + `mShaderList.push_back()` so atmosphere uniforms auto-bind
  - `lldrawpoolalpha.cpp`: forward-pass alpha depth-write gate extended for Cinematic + `RenderVolumetricLighting` (so godrays get proper depth)
  - `llshadermgr.{cpp,h}`: added `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` uniform strings
- `indra/newview/app_settings/settings.xml`: 5 new P3 cvars
- Hook dispatched only on Cinematic mode entry → zero cost in Standard / Realism modes

### Credits

The volumetric lighting (godrays) implementation pattern originates with [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean). AYAstorm uses BD `995a1354d8` (2026-04-19) as the upstream reference point and adopts `volumetricLightF.glsl` under license inheritance (LGPL-2.1-only). On the AYAstorm side we added:

- Replaced the BD-only `nonpcfShadowAtPos` shadow helper with the AYAstorm/Firestorm standard `sampleDirectionalShadow` from `shadowUtil.glsl` (using `sun_dir` as the surrogate normal)
- `HAS_SUN_SHADOW` permutation gate so the entire godrays computation only runs when `RenderShadowDetail > 0` (automatic passthrough when sun shadows are off)
- Registered `gVolumetricLightProgram` in `mShaderList` so atmosphere uniforms (`sunlight_color` / `sun_dir` / `blue_density` / `haze_density`) auto-bind
- Cinematic mode gate (hook dispatched only when `AYAVisualRealismEnabled == 2`)
- `mPostPingMap` / `mPostPongMap` ping-pong for GPU read-after-write safety
- Retuned default `RenderVolumetricLightingMultiplier` from BD's `1.0` to `50.0` (determined by a 4-stage shader canary bisect + AYA's subjective evaluation, see §8.2)

### Documentation

- r30 P3 full trace / file:line modification map / step 1–6 implementation commit log / acceptance observations (including the 4-stage canary bisect): [`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`](../specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md)
- Parent spec (r30 chapter): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Prior phase (r30 P2, velocity buffer + motion blur + SMAA T2x): [`docs/release/ayastorm-r30-p2-release-note.en.md`](ayastorm-r30-p2-release-note.en.md)
