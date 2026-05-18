# AYAstorm r14 — Release Announcement

**r14 is the opening release of the visual-realism chapter** (r14–r20) — adding altitude-dependent atmospheric density and scene-referred (linear-space) integration on top of SL's existing Beer-Lambert + in-scatter atmospherics, so that air gains a real sense of volume without breaking WindLight preset compatibility.

> **Distribution**: r14 ships **bundled with the r23 release** (no standalone r14 tag). The r23 release page links back to this note and to the r14 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r14-volumetric-atmosphere.md`). This note is link-only + diff highlights.

---

## AYAstorm r14 — Volumetric atmosphere (air with volume)

### Headline: air starts to feel like a medium, not a flat tint

Through r13, SL atmospherics already used Beer-Lambert extinction and an in-scatter haze_glow term — but two physical ingredients were missing, making the sky read as a flat gradient rather than as an actual body of air:

- **No altitude density gradient**: ground and sky use the same density, so an overcast day never feels "thick at ground level, thinning above"
- **No scene-referred integration**: `additive` was composited in sRGB and converted to linear afterwards, breaking HDR scene-buffer physicality

r14 adds both, **without rewriting the existing pipeline**:

- New `calcAtmosphericVars` branch with an exponential altitude-density profile (controlled by `scale_height`, derived from preset `max_y * 0.1`)
- Linear-space mixing of `additive` and `blue_horizon` directly inside `atmosphericsFuncs.glsl` and `skyV.glsl` so depth-aware air composition lands physically correctly on the HDR scene buffer
- WindLight preset values are reinterpreted as inputs; preset compatibility is preserved

The implementation is **analytic and inexpensive** — no raymarching. Heavy volumetric work (godrays, cloud volume, aerial perspective) is deferred to r15–r18.

Details → spec `docs/ayastorm-r14-volumetric-atmosphere.md`

### Master switch (introduces the chapter-wide toggle)

r14 introduces the **chapter master switch** used through r14–r20:

| Key | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | Master switch for the entire visual-realism chapter. `0` reverts atmospherics, sky compositing and (later) godrays / aerial perspective / cloud volume / translucency / avatar SSS to the pre-r14 behaviour |

> **Per-feature debug cvars are intentionally not provided.** One sensible default beats a constellation of tuning knobs (per memory `feedback_prefer_defaults_over_config.md`).

### Known tradeoffs

- **Linear-space mixing flattens midtones slightly** — the sky reads as marginally more "overcast" overall. The horizon (sunrise / sunset / haze quality) gains physical accuracy; this is the price. Tunable preset values still let you push back, and later chapter releases (r15+) restore richness through dispersion / godrays / aerial perspective.
- **Sun disc protection split (P2.a refined)**: the in-shader split keeps `haze_horizon` (the sun-direction glow) in legacy sRGB to avoid linearised haze peaks washing out the sun disc. Only `blue_horizon` (the all-direction blue) is moved to linear.
- **Preetham off-axis path length (P2.b) and per-wavelength Rayleigh/Mie split (P2.c) were deferred** — both caused sun-disc artifacts in early experiments; reattempted in later chapter releases with sun-disc protection as a hard constraint.

### Implementation summary

- `atmosphericsFuncs.glsl` — altitude density profile + linear `additive` mixing under the master switch
- `skyV.glsl` — linear `blue_horizon` mixing under the master switch (with inline `aya_srgb_to_linear` / `aya_linear_to_srgb` since `srgbF.glsl` isn't attached to vertex shaders)
- `LLSettingsVOSky::applyToShader` — uniform plumbing for `aya_visual_realism_enabled` (template followed: `classic_mode`)
- `LLShaderMgr` enum + `mReservedUniforms` extended
- `settings.xml` — `AYAVisualRealismEnabled` Boolean default 1

### Documentation

- r14 spec / pipeline rationale / known tradeoffs / risk register: `docs/ayastorm-r14-volumetric-atmosphere.md`
- P0 atmospheric-pipeline survey: `docs/archive/r14/volumetric_atmosphere_survey.md`
- Visual-realism chapter roadmap (covers r14–r20): `docs/ayastorm-visual-realism-roadmap.md`
- Deprecated r14 sun-dazzle plan (pivot history): `docs/ayastorm-r14-sun-dazzle.md`
