# AYAstorm r17 — Release Announcement

Short text intended to be pasted into the GitHub release page. **r17 is the fourth release of the visual-realism chapter (r14–r20)** — adds a sun-elevation-driven color temperature modulator that warms the sun / ambient / cloud color toward orange as the sun approaches the horizon, restoring the cinematic warmth of sunset that flat preset values alone don't deliver.

> **Distribution**: r17 ships **bundled with the r23 release** (no standalone r17 tag). The r23 release page links back to this note and to the r17 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r17-color-temperature.md`). This note is link-only + diff highlights.

> **Note on r17 history**: r17 was once dropped after an instant A/B test reported "no perceptible difference" — but sustained viewing afterward made it clear that **the cinematic orange sunset was gone**. The drop was reverted on the same day. The original implementation (`c3d6aee734`) was restored in full. This is the lesson recorded in memory `feedback_instant_ab_vs_sustained.md`: instant A/B and sustained viewing are different evaluation axes, and "didn't show in A/B" ≠ "doesn't matter cumulatively".

---

## AYAstorm r17 — Time-of-day color temperature

### Headline: bring back the warmth of sunset

WindLight presets carry color information, but the sun-elevation curve in SL has no physical color-temperature modulation — Sunset preset gives you warm-tinted values, but the **sun itself never grows visibly orange** during a day cycle, and ambient / cloud light don't follow the sun toward warm tones the way they do in a photograph.

r17 adds a single helper, `LLSettingsVOSky::getR17SunModulator(lightnorm, psky)`, that:

- Takes the sun direction's elevation (`lightnorm.z`)
- Computes `t = smoothstep(0, 0.4, lightnorm.z)`
- Maps `t` to Kelvin: `K = mix(2200, 6500, t)` (warm horizon → neutral midday)
- Converts Kelvin → RGB via Tanner Helland 2012

The resulting modulator is applied at **three injection points**:

1. **Sky path** (`llsettingsvo.cpp::applySpecial`): multiplies `SUNLIGHT_COLOR`, `CLOUD_COLOR`, and ambient
2. **Scene path** (`pipeline.cpp::setupHWLights`): multiplies `mSunDiffuse` and ambient before `gGL.setAmbientLightColor`
3. **Cloud path B-axis** (shared with r18 — the modulated `CLOUD_COLOR` rides through the cloud shader so volumetric clouds (r18) catch the warm tone too)

### Where r17 is effective vs not (preset-dependent)

The modulator is **physical / elevation-driven**, which means it shines when the sun actually moves — and is intentionally subtle on fixed presets that pin the sun at zenith:

| Preset | sun elevation | K | Modulator | Where it shows |
|---|---|---|---|---|
| Day cycle (estate time) | varies 0.0 → 1.0 | 2200 → 6500 | strong amber → identity | **Strongest effect** — cumulative warm-up as sun sets |
| Sunset (fixed) | 0 | 2200 | strong warm | **Cinematic orange sunset restored** (the revert motivation) |
| Sunrise (fixed) | 0.996 (zenith) | 6500 | identity | No-op (preset designer pinned the sun high) |
| Midday (fixed) | 0.37 | ≈6500 | near-identity | No-op |
| Midday (legacy preset, `KNOWN_SKY_LEGACY_MIDDAY`) | — | — | identity (pinpoint exclusion) | Deliberately untouched to preserve pre-PBR noon |

This "effective on some presets, identity on others" behaviour is an acceptable tradeoff per `feedback_release_with_user_feedback.md`. We call it out here so users don't expect a uniform effect across every preset.

### Settings

| Key | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON) | Chapter master switch (r14–r20). Now **U32** (Firestorm View=0 / AYAstorm View=1), tied to the new `AYAViewMode` combo_box in Preferences → Graphics → Shaders |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | r17 sentinel. `0` returns the modulator to identity (preset colors pass through unmodified) |

### Master cvar promoted to U32

`AYAVisualRealismEnabled` changes from **Boolean → U32** in this release (default still 1). Reason: the new `AYAViewMode` combo_box (`Firestorm View=0` / `AYAstorm View=1`) couples cleanly with U32 cvars but suffers from LLSD-coercion flakiness against Boolean cvars (memory `feedback_combo_box_u32_cvar.md`). The three C++ read sites (`llsettingsvo.cpp::applySpecial` ×2, `pipeline.cpp::doGodrays`) were updated to `LLCachedControl<U32>` with `() != 0` predicate.

### Implementation summary

- `llsettingsvo.{h,cpp}` — `getR17SunModulator()` helper + `kelvinToRGB()`, three injection-point integrations
- `pipeline.cpp::setupHWLights` — scene-path modulator application
- `settings.xml` — `AYAR17ColorTemperatureEnabled` Boolean default 1; `AYAVisualRealismEnabled` promoted to U32
- `panel_preferences_graphics1.xml` (en/ja) — `AYAViewMode` combo_box exposed in Preferences

### Known limitations

- **Subtle in instant A/B**, cumulative in sustained viewing. The lesson from the drop / revert episode: evaluate this kind of effect by sustained viewing, not by quick toggle (memory `feedback_instant_ab_vs_sustained.md`).
- **`KNOWN_SKY_LEGACY_MIDDAY` is pinpoint-excluded** — that preset is intentionally a flat pre-PBR noon reproduction; we don't modulate it.

### Documentation

- r17 spec / revert record / why the modulator works on Day cycle but not Sunrise preset: `docs/ayastorm-r17-color-temperature.md`
- r18 cloud volumetric spec (where the B-axis CLOUD_COLOR mod lives): `docs/ayastorm-r18-cloud-volumetric.md`
- Visual-realism chapter roadmap: `docs/ayastorm-visual-realism-roadmap.md`
- combo_box ↔ U32 cvar pattern: memory `feedback_combo_box_u32_cvar.md`
- instant A/B vs sustained viewing lesson: memory `feedback_instant_ab_vs_sustained.md`
