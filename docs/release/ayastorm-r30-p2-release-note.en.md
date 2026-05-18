# AYAstorm r30 P2 — Release Announcement

**r30 P2 introduces per-object motion blur and SMAA T2x into Cinematic mode** — landing the velocity buffer generation path and temporal resolve as the foundation of the r30 (cinematic rendering) chapter.

Implementation traces, modification points, acceptance observations, and upstream reference lines live in the permanent doc (`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`). This note is the entry point and diff highlight.

---

## AYAstorm r30 P2 — Velocity Buffer + Per-object Motion Blur + SMAA T2x

### Headline: building the Cinematic rendering foundation

The r30 chapter (cinematic rendering) started in P1 by unifying View Mode into a restart-switch with three modes and adding the Cinematic slot to the UI. P2 lands **per-object motion blur** and **SMAA T2x temporal antialiasing** into that Cinematic mode. Both share a common **velocity buffer** (per-pixel NDC delta from the previous frame, in a `GL_RG16F` RT) that is allocated and drawn only when Cinematic mode is active.

In Standard / Realism (AYAstorm View) modes the velocity buffer is not allocated at all, so additional cost is zero.

### How it works

Velocity buffer generation path:

```
display() flow
  → renderGeomMotionBlur()
       ├─ clear mVelocityMap to (0,0,0,1)
       └─ dispatch pool.renderMotionBlur() across all pools
              ├─ Bump / Materials / PBR opaque  (rigged / static)
              ├─ Tree / Terrain (face-iter)
              ├─ Alpha mask (with alpha-discard)
              └─ Avatar (LL-native skin)
  → each pool feeds current-frame and previous-frame matrices into uniforms via
    pushVelocityBatches{,Textured} / pushRiggedVelocityBatches{,Textured},
    and the velocity shader writes the NDC delta
```

Post-process composite:

```
deferredScreen (final color after lighting)
  → motionBlurF.glsl (32-tap triangle-weighted blur along velocity direction)
       ├─ NaN/inf guard (defends against garbage velocity from uninitialized matrices)
       ├─ noise floor 2.0 px (suppresses subpixel drift smearing static geometry)
       ├─ sanity ceiling 2× max_blur (passes through runaway velocity from skinning blowup / SIM-boundary crossings)
       └─ per-sample velocity gate (kills halo bleed around opt-out avatars)
  → frag_color
```

SMAA T2x temporal resolve:

```
before the final SMAA (spatial AA) blend pass
  → Halton(2,3) 2-tap subpixel jitter injected into the projection matrix
  → reproject previous-frame result via velocity
  → 50/50 blend, averaging the two samples (the "2x" of T2x)
```

### Settings

**No user action required for normal use.** Launching Cinematic mode (`AYAVisualRealismEnabled = 2`) automatically enables the motion blur and SMAA T2x foundation. The tuning cvars are:

| Cvar | Default | Purpose |
|---|---|---|
| `RenderMotionBlurStrength` | `32` | Max blur length (pixels) in the motion blur composite. `0` disables the composite (velocity buffer is still generated) |
| `RenderMotionBlurSelfAvatar` | `1` (ON) | Whether your own avatar is written into the velocity buffer. OFF keeps self always crisp (useful in first-person / selfie shots where only camera motion should smear) |
| `RenderMotionBlurOtherAvatars` | `1` (ON) | Whether other avatars are written into the velocity buffer. OFF keeps others always crisp (useful in group shots where only the environment should smear) |
| `RenderSMAAT2x` | `0` (OFF) | SMAA T2x temporal resolve. Requires `RenderFSAAType=2` (SMAA) and Cinematic mode. Subtly smooths edge detail (leaves, hair, thin branches) |
| `RenderBufferVisualization` | `-1` | Setting `7` visualizes the velocity buffer on screen (R=X, G=Y velocity). Diagnostic only |

Boolean cvar value changes commit **on closing the Debug Settings window** (auto-widget commit timing — not a code bug). After toggling, close the Debug Settings window once before observing the behavior.

### Migration note

- Cinematic mode now supports per-object motion blur and SMAA T2x
- No effect on Standard / Realism modes (velocity buffer is not allocated)
- Switching between Cinematic and other modes **requires a viewer restart** (design fixed in r30 P1, to avoid dynamic reconfiguration of velocity / SMAA RTs)
- These cvars use keys independent from any upstream (Linden / Firestorm) `RenderMotionBlur` family, so no setting collision occurs

### Known limitations

- **Per-bone motion blur on classic / system avatar bodies**: the avatar pool render path does not upload the `lastMatrixPalette` uniform, so limb motion on classic bodies does not produce per-bone velocity. The composite's sanity ceiling guard (`speed > max_blur * 2.0` → passthrough) keeps this from corrupting the image. For the modern SL majority (mesh-body avatars) the rigged mesh attachment path uploads matrices independently and per-bone velocity works correctly. The edge case is "classic-bodied avatars in motion get no per-bone blur" — to be revisited in a later r30 phase
- **macOS / Windows runtime verification**: AYAstorm Linux build verified; macOS / Windows Release binary verification will be performed when the tag is cut

### Implementation summary

- Shaders (`indra/newview/app_settings/shaders/class1/deferred/`):
  - 9 velocity shaders (borrowed from Black Dragon Viewer): `avatarVelocity{F,V}.glsl`, `skinnedVelocity{V,AlphaV}.glsl`, `velocity{F,V,Alpha{F,V},FuncV}.glsl`
  - SMAA T2x resolve: `SMAAResolve{V,F}.glsl`
  - Motion blur composite: `motionBlurF.glsl` (per-sample velocity gate added on the AYAstorm side to kill halo bleed around `RenderMotionBlur{Self,Other}Avatars` opt-outs)
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: `mVelocityMap` / `mSMAAHistory` RT allocation (Cinematic gate), new `renderGeomMotionBlur()`, new `renderMotionBlurComposite()`, `renderBufferVisualization` case 7
  - `lldrawpool.{cpp,h}`: `LLDrawPool::{getNumMotionBlurPasses, beginMotionBlurPass, renderMotionBlur, endMotionBlurPass}` virtuals + 4 push helpers (`push{,Rigged}VelocityBatches{,Textured}`)
  - Each drawpool subclass (Bump / Materials / PBR / Tree / Terrain / Alpha / Avatar): velocity pass override
  - `lldrawpoolavatar.{cpp,h}`: `RenderMotionBlurSelfAvatar` / `RenderMotionBlurOtherAvatars` opt-out
  - `llspatialpartition.h`: `LLDrawInfo::mAttachedToAvatar` added (static-prim attachment wearer link, independent of `mAvatar` which is rigged-only)
  - `llvovolume.cpp`: `registerFace()` sets `mAttachedToAvatar = vobj->getAvatar()`
- `indra/newview/app_settings/settings.xml`: 4 new P2 cvars and extended `RenderBufferVisualization=7` description
- Velocity buffer is allocated only on Cinematic mode entry → zero cost in Standard / Realism modes

### Credits

The velocity buffer + per-object motion blur + SMAA T2x implementation pattern originates with [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean). AYAstorm uses BD `995a1354d8` (2026-04-19) as the upstream reference point and adopts the 9 velocity shaders + composite shader + SMAA resolve shaders under license inheritance (LGPL-2.1-only). On the AYAstorm side we added:

- Cinematic mode gate (RT only allocated when `AYAVisualRealismEnabled == 2`)
- Two avatar opt-out cvars (Self / OtherAvatars) with write-side skip helpers
- Per-sample velocity gate in the composite shader (kills halo bleed around opted-out avatars)
- Fix for the T-pose rasterize bug in BD's `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` (now applies object skinning on the `current_clip` side too)
- Composite-side garbage velocity defenses (NaN/inf guard + sanity ceiling at 2× max_blur)

### Documentation

- r30 P2 full trace / file:line modification map / step 1–5e implementation commit log / acceptance observations: [`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`](../specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md)
- Parent spec (r30 chapter): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Prior phase (r30 P1, View Mode restart-switch): [`docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`](../specs/ayastorm-r30-p1-view-mode-restart-switch.md)
