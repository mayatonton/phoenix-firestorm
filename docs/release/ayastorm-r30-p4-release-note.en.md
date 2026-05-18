# AYAstorm r30 P4 — Release Announcement

**r30 P4 introduces the BD DoF chain (high-quality DoF + chromatic aberration + front blur) into Cinematic mode** — porting the entire DoF pipeline from Black Dragon Viewer and adding a dedicated Cinematic Controls floater (new AYAstorm top menu → `Cinematic Controls...` / `Alt+C`) for live in-shot adjustment, as the expressive layer of the r30 (cinematic rendering) chapter.

Implementation traces, modification points, acceptance observations (including the chroma math hotfix), and upstream reference lines live in the permanent doc (`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`). This note is the entry point and diff highlight.

---

## AYAstorm r30 P4 — BD DoF Chain (HQ DoF + Chromatic Aberration + Front Blur)

### Headline: bringing photographic lens optics — bokeh and chromatic aberration

The r30 chapter (cinematic rendering) landed per-object motion blur and SMAA T2x in P2, and volumetric lighting in P3, building the Cinematic mode rendering base and the material substance of light. P4 lands the **optical characteristics of a photographic lens (HQ DoF + chromatic aberration + front blur)** on top.

Chromatic aberration is the optical phenomenon caused by glass lenses bending R/G/B wavelengths at different refractive indices, producing the "shot on an old lens" look where edges near the screen periphery split into red and blue fringes. AYAstorm r30's core thesis (`docs/specs/ayastorm-r30-cinematic-chapter.md`) is "an atmosphere and space worth photographing," and this step makes the lens itself part of that material substance.

In Standard / Realism (AYAstorm View) modes the permutation is not attached at the shader register stage, so additional cost is zero.

### How it works

Inserted into existing DoF paths via permutation flags as a post-process pass:

```
deferredScreen (final color after lighting)
  → gDeferredPostProgram (HQ DoF path)
       ├─ gated by Cinematic mode + RenderDepthOfFieldHighQuality
       ├─ HAS_DOF_CHROMA permutation → CoF-based chromatic aberration
       ├─ FRONT_BLUR permutation → front-of-focus blur path enabled
       └─ DEFERRED_CHROMA_STRENGTH uniform for runtime strength
  → gDeferredPostNoDoFProgram (NoDoF path)
       ├─ HAS_DOF_CHROMA permutation → radial offset chromatic aberration
       └─ vary_fragcoord-based radial formula (0 at screen center → max at periphery)
  → gDeferredPostNoDoFNoiseProgram (final present path)
       └─ Same NoDoF treatment + film grain noise
```

Chromatic aberration is implemented as per-channel texcoord offsets — the R channel samples slightly inward along the radial direction, the B channel slightly outward (G is not offset).

### Settings

**No user action required for normal use.** Launching Cinematic mode (`AYAVisualRealismEnabled = 2`) automatically enables the BD DoF chain. Live in-shot adjustment is via the new **AYAstorm menu → Cinematic Controls...** (`Alt+C`). The tuning cvars:

| Cvar | Default | Purpose |
|---|---|---|
| `RenderDepthOfFieldHighQuality` | `0` (OFF) | High-quality DoF post-pass with 4× CoF samples + depth-gated chroma. Higher GPU cost, user opt-in. Only active in Cinematic mode |
| `RenderDepthOfFieldChroma` | `1` (ON) | Compile-in switch for the chromatic aberration block. OFF leaves only the vignette path (subtle edge fringing). ON enables per-pixel chroma that scales with DoF blur. Only active in Cinematic mode |
| `RenderChromaStrength` | `5.0` | Chromatic aberration intensity (0–100 range). `5` is subtle (the "noticeable" range), `10` is clearly visible separation, `30` is cinematic, `>50` is stylized. BD's default of `0.0` assumes the user moves a UI slider; AYAstorm provides equivalent access via the floater and raises the default so the feature isn't invisible on first launch |
| `RenderDepthOfFieldFront` | `1` (ON) | Allow front-of-focus blur (objects closer than the focus plane also blur). BD default reproduced exactly. Only effective when HQ DoF is user-opted-in. Only active in Cinematic mode |

### What it looks like

- **Chromatic aberration pattern**: the screen center stays untouched, edges progressively split into R/B fringes toward the periphery — a radial formula (center r=0 → edge r=1) modeling the "fringe" of an old lens
- **Where HQ DoF helps**: standard DoF builds bokeh in the forward pass with a limited sample count, while HQ DoF rebuilds it in the post pass with 4× samples — bokeh circles get smoother and the "dot" artifacts in defocus disappear. Most visible in shallow-DoF compositions (close-up portrait with a blurred background)
- **Front blur**: objects in front of the focus plane (e.g. your own avatar's hand) blur outward in the same direction as the background. The difference vs. standard DoF (no front blur) is obvious when you place a prop between the camera and the subject
- **Mode**: Cinematic only. Standard / AYAstorm View do not attach the permutation at all

### Migration note

- Cinematic mode now supports the BD DoF chain (HQ DoF + chromatic aberration + front blur)
- No effect on Standard / Realism modes (permutation gate)
- Switching between Cinematic and other modes **requires a viewer restart** (design fixed in r30 P1)
- The existing FIRE-16728 free-aim DoF mechanism (where to put focus) is preserved. P4 only borrows "DoF internal sample quality + chromatic aberration + front blur"; the focus mechanism remains Firestorm's existing one — a hybrid configuration
- The new AYAstorm top menu (between Build and Help) will gain sibling items as P5+ Cinematic features ship
- The Cinematic Controls floater is for live in-shot adjustment only and intentionally does not appear in Preferences (these are not persistent-config-style settings — they belong in the shoot workflow)

### Known limitations

- **NoDoF path is subtle**: standard DoF (non-HQ) chromatic aberration has no CoF to scale against, so the formula produces a mild vignette where the screen center stays untouched. For strong chromatic aberration combine with `RenderDepthOfFieldHighQuality=1` (HQ DoF) — that activates CoF-based chroma across the full screen
- **`RenderChromaStrength` default diverges from BD**: BD ships at `0.0` (effect off); AYAstorm ships at `5.0`. BD assumed the user would raise the value through the visible UI slider; since AYAstorm provides equivalent access through the floater, we raise the default to avoid the "feature appears missing" first impression (see §5.7 / `feedback_match_bd_defaults_on_borrow.md`)
- **`lldrawpoolwater` water chroma is not borrowed**: BD `lldrawpoolwater.cpp:257` pushes chroma uniform onto the water shader too, but Firestorm's water pipeline diverges significantly from BD's and porting it carries an unknown regression risk — out of scope for P4
- **Localization**: P4 ships the floater in English only. Japanese / Chinese lproj translations will be batched into a future Cinematic-features release (P5+)
- **macOS / Windows runtime verification**: AYAstorm Linux build verified; macOS / Windows Release binary verification will be performed when the tag is cut

### Implementation summary

- Shaders (`indra/newview/app_settings/shaders/class1/deferred/`):
  - `postDeferredHQDoFF.glsl` (new, borrowed from BD with AYAstorm modifications): HQ DoF + CoF-based chromatic aberration + front blur blocks
  - Existing `postDeferredF.glsl`: added `#if HAS_DOF_CHROMA` chroma block at the end of `dofSample()` (chromatic aberration in the standard DoF path)
  - Existing `postDeferredNoDoFF.glsl`: added radial-offset chroma block for the NoDoF path (during acceptance the initial edge-gated vignette approach was rewritten to radial offset — see §9.2)
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: `LLPipeline::RenderChromaStrength` static + uniform push at three sites — `renderDoF` / `renderFinalize` / `bindDeferredShader`
  - `llviewershadermgr.cpp`: 3 programs (`gDeferredPostProgram` / `gDeferredPostNoDoFProgram` / `gDeferredPostNoDoFNoiseProgram`) gated by Cinematic + cvar (two-stage) and decorated with `HAS_DOF_CHROMA` / `FRONT_BLUR` permutations; cvar signal listeners minimize required restarts
  - `llshadermgr.{cpp,h}`: added `DEFERRED_CHROMA_STRENGTH` reserved uniform enum + string
- XUI / UI wiring:
  - `menu_viewer.xml`: new AYAstorm top menu (between Build and Help), with a `Cinematic Controls...` item (`Alt+C`) that toggles the floater
  - `floater_aya_cinematic.xml` (new): width 320 / height 320 / single_instance — restrained two-section layout (DoF + Chromatic Aberration)
  - `llviewerfloaterreg.cpp`: floater registration (reusing the `FloaterQuickPrefs` generic class)
- `indra/newview/app_settings/settings.xml`: 4 new P4 cvars
- Permutations attached only on Cinematic entry → zero cost in Standard / Realism modes

### Credits

The BD DoF chain (HQ DoF + chromatic aberration + front blur) implementation pattern originates with [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean). AYAstorm uses BD `995a1354d8` (2026-04-19) as the upstream reference point and adopts `postDeferredHQDoFF.glsl` under license inheritance (LGPL-2.1-only). On the AYAstorm side we added:

- Fixed the shader file header `@file` directive + added a provenance comment (BD `995a1354d8`, LGPL-2.1-only)
- Cinematic mode gate (permutation attached only when `AYAVisualRealismEnabled == 2`)
- Did not import BD's separate settings file (`settings_blackdragon.xml`); instead the 4 cvars are integrated into Firestorm's standard `settings.xml`
- Rewrote the NoDoF chroma formula from BD's fixed offset approach to a radial per-channel offset approach (vary_fragcoord based, 0 at screen center → max at periphery)
- Retuned default `RenderChromaStrength` from BD's `0.0` to `5.0` (acceptance found chroma_str=0 made the feature "look missing" — reapplied `feedback_match_bd_defaults_on_borrow.md` with empirical confirmation)
- Did not import BD UI (`panel_preferences_graphics1.xml` / `panel_machinima.xml`); instead provided access via an AYAstorm-original top menu + dedicated Cinematic Controls floater (per chapter §1.2)
- Did not import BD's FIRE-16728-independent focus mechanism (`CameraFreeDoFFocus` static); preserved Firestorm's existing FIRE-16728 free-aim DoF
- Did not import the BD `lldrawpoolwater.cpp:257` water chroma uniform push (Firestorm's water pipeline diverges significantly — deep regression risk)

### Documentation

- r30 P4 full trace / file:line modification map / step 1–7 implementation commit log / acceptance observations (including the chroma math hotfix + UI revise history): [`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`](../specs/ayastorm-r30-p4-bd-dof-chain-trace.md)
- Parent spec (r30 chapter): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Prior phase (r30 P3, volumetric lighting): [`docs/release/ayastorm-r30-p3-release-note.en.md`](ayastorm-r30-p3-release-note.en.md)
