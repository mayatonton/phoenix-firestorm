# AYAstorm r16 — Release Announcement

**r16 is the third release of the visual-realism chapter (r14–r20)** — adds wavelength-dependent (Rayleigh λ⁻⁴) in-scatter weighting to the scene aerial perspective path so distant terrain shifts toward blue while preserving WindLight preset compatibility and **without touching the sky dome** (sun disc is structurally protected).

> **Distribution**: r16 ships **bundled with the r23 release** (no standalone r16 tag). The r23 release page links back to this note and to the r16 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r16-aerial-perspective.md`). This note is link-only + diff highlights.

---

## AYAstorm r16 — Aerial perspective (distance shifts color)

### Headline: distant terrain settles into the air physically

r14 made the air feel like a medium; r15 made light beams traverse the volume; r16 makes **distant terrain settle into that air** the way mountains do in a photograph:

- **Distant mountains haze toward blue** (Rayleigh scattering — short wavelengths scatter more, the longer the line-of-sight, the bluer the residual)
- **Foreground unchanged** (atmosFragLighting scalarizes atten, so wavelength dependence only shows through the additive in-scatter term — by design)
- **WindLight preset values are reinterpreted as input**, preset compatibility preserved

Done **without rewriting any pipeline**:

- New `rayleigh_w = (1.0, 2.33, 5.71)` weighting in `atmosphericsFuncs.glsl::calcAtmosphericVars` is applied to `combined_haze` and `blue_weight` (in-scatter), but **not** to `light_atten` (which would turn the near scene amber — `aerial perspective ≠ sunset`)
- **`skyV.glsl` is not touched**; the sun disc / horizon / haze_glow remain at the r14 P2.a refined state — structurally protects the sun disc that r14 P2.b/c had degraded

### What was tried and dropped (intentional)

- **Preetham 1999 spherical sec(θ) approximation (P1.b)**: implemented and verified on Linux. Numerically the difference is real (sec=57.3 → Preetham=26.5 at θ=89°), but the AYAstorm reviewer judged "no perceptible difference" because SL's sun timeline crosses ±5° at the horizon in a single time step. Dropped per `feedback_feature_value_in_main_usecase.md` (working ≠ effective). Revisited later in the chapter when sun-path physics can be retried with sun-disc protection as a constraint.
- **Distance Multiplier physical reinterpretation**: deferred, P1.a alone already produces the perceptual shift, no reason to take preset-compat risk.

### Master switch + per-feature sentinel

| Key | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | Chapter master switch (r14–r20). `0` reverts everything to pre-r14 behaviour |
| `AYAR16AerialPerspectiveEnabled` | `1` (ON) | r16 sentinel. `0` reverts to r15 behaviour (rayleigh_w = (1,1,1), arithmetically identity) |

The per-feature sentinel exists because the chapter master toggles r14 / r15 effects simultaneously — without a per-r16 toggle, instant A/B evaluation of r16 alone is impossible. We accept this small departure from "prefer one sensible default" as a chapter-evaluation tool, with the intention to fold it into the master once the A-axis (r14–r18) is complete.

### Known tradeoffs

- **`atmosFragLighting` scalarizes atten** (`light *= atten.r`): surface direct-transmission wavelength dependence is effectively a no-op. The visible Rayleigh shift you see in r16 comes entirely through the **additive (in-scatter) path** (`blue_weight` and `(1 - combined_haze)`). This is documented in spec §3 and memory `project_atmos_atten_scalarized.md` to keep future contributors from assuming per-channel atten works on direct transmission.
- **Foreground appears nearly unchanged** by design — `density_dist` is small at short ranges, so additive is thin and Rayleigh weight has little leverage there. This is the correct physical behaviour (aerial perspective is a long-path phenomenon).

### Implementation summary

- `atmosphericsFuncs.glsl` — `rayleigh_w` ternary, applied to `combined_haze` and `blue_weight`; `light_atten` deliberately untouched
- `LLSettingsVOSky::applyToShader` — `aya_r16_aerial_perspective_enabled` uniform plumbing
- `LLShaderMgr` enum + reserved uniform name
- `settings.xml` — `AYAR16AerialPerspectiveEnabled` Boolean default 1

### Documentation

- r16 spec / pipeline rationale / dropped P1.b notes / risk register: `docs/ayastorm-r16-aerial-perspective.md`
- P0 atmospheric pipeline survey: `docs/archive/r16/aerial_perspective_survey.md`
- Visual-realism chapter roadmap (r14–r20 overview): `docs/ayastorm-visual-realism-roadmap.md`
- atmosFragLighting atten scalarize note: memory `project_atmos_atten_scalarized.md`
