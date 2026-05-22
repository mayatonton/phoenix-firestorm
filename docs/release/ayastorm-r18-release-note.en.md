# AYAstorm r18 — Release Announcement

**r18 is the fifth release of the visual-realism chapter (r14–r20) — and completes the A-axis (atmosphere)**. Clouds are no longer flat textured planes: a lightweight slab raymarch over the existing 2D `cloud_noise_texture` gives them depth, sculpted edges, and (when combined with r17's color temperature) **cinematic orange sunset clouds**.

> **Distribution**: r18 ships **bundled with the r23 release** (no standalone r18 tag). The r23 release page links back to this note and to the r18 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r18-cloud-volumetric.md`). This note is link-only + diff highlights.

---

## AYAstorm r18 — Cloud volumetric + color-temperature coupling

### Headline: clouds get depth, sunset clouds get cinematic

r14 → r17 built up the air, the light beams, the long-range haze, and the warm sun. r18 lands the core of "a sky worth photographing":

- **Clouds stop being flat planes** — a 4-step slab raymarch along the view direction over the existing `cloud_noise_texture` (sampler2D) gives thickness, edge sculpting, and an internal depth gradient
- **Sunset clouds become cinematic** — r17's color temperature modulator (`getR17SunModulator`) is applied to `CLOUD_COLOR` (the B-axis we revived after a sustained-viewing review), so the volumetric clouds **catch warm tone**
- **No new asset shipped** — the existing 2D noise texture is reused; no 3D noise atlas, no preset rework

Done **without heavy raymarch** (heavy per-frame full-screen ray-march is permanently dropped per chapter roadmap §6): N=4 fixed slab steps per cloud pixel, Beer-Lambert-style transmittance (45% per slab). Computational cost is bounded; live A/B reports "very impressive" without a noticeable FPS drop.

### What's inside

**A-axis (cloud volumetric)** — `cloudsF.glsl` slab raymarch
- Existing 2D `cloud_noise_texture` is sampled at N=4 positions along a UV-space slab offset (`(0.013, 0.008)` per step, a view-direction proxy)
- Per-slab transmittance accumulates Beer-Lambert style
- Gated by `AYAR18CloudVolumetricEnabled` + master + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint exclusion
- **OFF path is arithmetically identical to the legacy flat sample** (preset compatibility is structurally preserved)

**B-axis (CLOUD_COLOR × r17 modulator)** — re-applies r17's color temperature
- `llsettingsvo.cpp::applySpecial` multiplies `psky->getCloudColor()` by `getR17SunModulator()` before pushing `CLOUD_COLOR` uniform
- Originally part of r17 → dropped together with r17 → **revived** after sustained viewing showed "the orange sunset is gone"
- Gated by `AYAR17ColorTemperatureEnabled` (sentinel in r17) + master + Legacy Midday pinpoint exclusion

### Settings

| Key | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | Chapter master switch — toggled via `Firestorm View / AYAstorm View` combo_box in Preferences → Graphics → Shaders |
| `AYAR18CloudVolumetricEnabled` | `1` (ON) | r18 A-axis sentinel. `0` reverts to flat 2D cloud sample (arithmetically identical to r17) |
| `AYAR17ColorTemperatureEnabled` | `1` (ON) | Sentinel from r17, also gates the B-axis CLOUD_COLOR mod here |

### View Mode UI

Preferences → Graphics → Shaders now exposes the chapter master via a combo_box (`AYAViewMode`):

- **Firestorm View** — `AYAVisualRealismEnabled = 0`, full reversion to pre-r14 visuals
- **AYAstorm View** — `AYAVisualRealismEnabled = 1`, the visual-realism chapter is on (default)

(The combo_box was added with r17 alongside the U32 promotion of the master cvar; r18 release is where it lands publicly.)

### Known limitations

- **A-axis effect is most visible on cloud-rich preset** (Cloudy / Sunset). Clear-sky preset has little cloud area to express depth on — by definition.
- **Legacy Midday preset (`KNOWN_SKY_LEGACY_MIDDAY`) is pinpoint-excluded** — A-axis falls back to flat sample, B-axis CLOUD_COLOR mod is identity. This preset is a deliberate pre-PBR noon reproduction and we don't modulate it.
- **Cloud shadow on terrain is out of scope** — it was considered for r18 but parked for r19+ to keep the scope tight.
- **A-axis effect coexists with WindLight `cloud_pos_density` / `cloud_scale`** — those preset uniforms remain meaningful inputs; raymarch sculpts the volume *of* what the preset already specifies.

### Implementation summary

- `cloudsF.glsl` — N=4 slab raymarch, gated; OFF path retained for arithmetic compatibility
- `llsettingsvo.cpp::applySpecial` — A-axis uniform push + B-axis `CLOUD_COLOR × r17_sun_mod`
- `LLShaderMgr` — `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` enum + reserved uniform
- `settings.xml` — `AYAR18CloudVolumetricEnabled` Boolean default 1

### Documentation

- r18 spec / slab raymarch parameters / B-axis revival history: `docs/ayastorm-r18-cloud-volumetric.md`
- P0 cloud-shader survey: `docs/archive/r18/cloud_volumetric_survey.md`
- r17 spec (drop / revert lesson that drove the B-axis revival): `docs/ayastorm-r17-color-temperature.md`
- Visual-realism chapter roadmap (A-axis completion = r18): `docs/ayastorm-visual-realism-roadmap.md`
