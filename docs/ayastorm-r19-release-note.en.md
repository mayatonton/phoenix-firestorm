# AYAstorm r19 — Release Announcement

Short text intended to be pasted into the GitHub release page. **r19 is the sixth release of the visual-realism chapter (r14–r20) and the first release of the B-axis (material color)**. Adds wrap-around diffuse + back-light transmission to the deferred lit path so thin objects — leaves, white curtains, the rim of an ear in strong backlight — start to **transmit light** instead of going flat black.

> **Distribution**: r19 ships **bundled with the r23 release** (no standalone r19 tag). The r23 release page links back to this note and to the r19 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r19-translucency.md`). This note is link-only + diff highlights.

---

## AYAstorm r19 — Translucency (thin-object back-light transmission)

### Headline: thin objects start to transmit light

A-axis (r14–r18) made the air believable. B-axis turns toward **what the air is wrapped around** — the materials themselves. r19 lands the entry point: thin objects letting sunlight transmit through:

- **Leaves glow at the edge against the sun** — looking up through a tree, leaf edges catch and transmit sunlight, the silhouette stops being a flat dark cutout
- **White curtains glow from within** — standing behind a thin curtain by a window, light wraps through it instead of being blocked
- **Ear / nose rim / fingertips transmit red** — in strong rim-lighting on portraits, skin starts to feel *alive*
- **Paper, candles, thin ceramics** — thin-material translucency in general

Effect is visible in **instant A/B** by design — the failed predecessor (r19 albedo fidelity, frozen archive `feature/aya-r19-albedo-fidelity-spec-draft`) taught us that "mathematically correct but invisible" should be dropped early, so r19 was deliberately re-scoped to a visually-decisive effect.

### How it works

A single algorithm — **wrap-around diffuse (Burley wrap) + back-light transmission** — is added as a sum to `sun_contrib` in `class3/deferred/softenLightF.glsl`:

```
nl_wrap = max((N·L + w) / (1 + w), 0)           // wrap term, replaces da in Legacy
back    = pow(max(-N·L, 0), k_back)              // back-light: strong only when the sun is behind
view    = pow(max(V·L, 0), k_view)               // and the viewer is looking toward the sun
transmit = back * view * tint * strength * sunlit_linear
```

Two injection sites in `softenLightF.glsl` cover the full deferred routing (per `docs/ayastorm-deferred-shader-routing.md`):

- **Legacy branch** (walls, avatars, ears, older clothing → `materialF` writer): `da` is replaced by `nl_wrap`, then `transmit * baseColor.rgb` is added after `sun_contrib`
- **PBR branch** (Mesh clothing → `pbropaqueF` writer): `transmit * baseColor.rgb` is added after the `pbrBaseLight()` call

The C++ side pushes a 4-tuple per intensity tier from the slow + fast `bindDeferredShader` paths so live toggling works without rebuild.

### Local lights (point / spot) and the sun

r19 is **sun-driven only**. Local lights producing wrap / back-transmission is treated as a secondary effect and deferred to r20+ if needed. The sun-only design covers the photography use cases (leaves against sun, curtain behind window, portrait against window) and keeps the scope tight.

### Settings

| Key | Default | Purpose |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | Chapter master switch (Firestorm View / AYAstorm View) |
| `AYAR19TranslucencyEnabled` | `1` (ON) | r19 sentinel. `0` reverts to r18 lit calculation |
| `AYAR19TranslucencyIntensity` | `1` (U32, 0–3) | 0=OFF / 1=subtle (default) / 2=standard / 3=strong. Tier table for `(wrap, k_back, k_view, strength)` lives in `pipeline.cpp` |

The intensity slider is included intentionally because translucency is one of those effects where "subtle by default, available louder for portraits" is the right shape. Tier 1 (default) is calibrated to avoid the CG-look failure mode (everything glowing all the time).

### Known limitations

- **Surface transparency (`alpha > 0` prims) bypasses softenLightF** — those prims take the forward `alphaF` path which isn't touched in r19. Intensity changes don't apply there. Forward-path injection is a candidate for r20+.
- **Subsurface depth dependence is approximate** — r19 uses `scol` (sun shadow) for self-shadow attenuation, which gives a useful secondary thickness dependence on flat prims but isn't a real diffusion profile. Physical SSS (thickness map / Burley diffusion / multi-scatter) is permanently dropped per chapter roadmap §6 (heavy, requires preset rework).
- **`Legacy Midday` preset is unaffected** — we don't pinpoint-exclude it in r19, but its high sun angle naturally minimizes back-transmission anyway. No reports of regression on it.

### Implementation summary

- `class3/deferred/softenLightF.glsl` — wrap + back-transmission uniforms + helpers (`ayaTranslucencyWrap`, `ayaTranslucencyTransmit`), Legacy branch + PBR branch injection
- `pipeline.cpp::renderDeferredLighting` — softenLightF bind site pushes the tier table after `bindDeferredShader`; both slow and fast paths covered
- `settings.xml` — `AYAR19TranslucencyEnabled` Boolean default 1, `AYAR19TranslucencyIntensity` U32 default 1

### Documentation

- r19 spec / Burley wrap parameters / risk register: `docs/ayastorm-r19-translucency.md`
- Deferred routing reference (writer → softenLightF branch mapping): `docs/ayastorm-deferred-shader-routing.md`
- Frozen r19 albedo-fidelity archive (predecessor scope, dropped): `docs/ayastorm-r19-albedo-fidelity.md`
- Visual-realism chapter roadmap (B-axis entry = r19): `docs/ayastorm-visual-realism-roadmap.md`
