# AYAstorm r20 — Release Announcement

Short text intended to be pasted into the GitHub release page. **r20 is the seventh release of the visual-realism chapter (r14–r20) and completes the B-axis (material color)**. Adds **screen-space subsurface scattering (SSS) to avatar skin** — your avatar (and other avatars) get a real soft, lit-from-within skin quality with **per-pixel skin masking** via gbuffer3 `.a`, **world-space-scaled blur** that auto-fades with distance, and **right-click learning** of new mesh bodies/heads (no UUID typing).

> **Distribution**: r20 ships **bundled with the r23 release** (no standalone r20 tag). The r23 release page links back to this note and to the r20 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`doc/spec_avatar_skin_sss.md`). This note is link-only + diff highlights.

---

## AYAstorm r20 — Avatar skin SSS (subsurface scattering)

### Headline: skin starts to feel alive

A-axis (r14–r18) made the air believable. r19 made thin objects transmit light. r20 lands the **skin** itself — the photographic reference point for "people":

- **Skin softens at the right scale** — separable 5-tap SSS blur with wavelength-dependent weights (red diffuses further than green / blue)
- **Skin lights up subtly** — glow restore (`pow(lit, 3) × glow_gain × warm_salmon`) restores the highlight crispness that blur naturally dulls, with a warm salmon tint that pushes "blood under the skin" rather than "glowing porcelain"
- **Works on yourself and on others** — the identifier is **mesh asset UUID**, available locally from the existing `ObjectUpdate`, zero sim cost, no creator cooperation required

This is the first viewer-side avatar skin SSS in SL/OpenSim history. Mainstream Mesh body/head products (Maitreya, Legacy, Reborn, eBody, LeLutka Evolution heads, Genus, etc.) all use stable mesh UUIDs, so once a few are learned, the typical SL crowd is covered.

### How it works

**Identifier — mesh asset UUID** (single axis):

`getVolume()->getParams().getSculptID()` is read locally from every avatar's attachments. If the UUID is in the user's whitelist cvar, the attachment is marked `mIsSSSTarget = true`. No sim roundtrip, no Description editing, no inventory item name lookup (which doesn't work for other avatars).

**Right-click learning UX**:

Right-click an attachment → **"Add to SSS whitelist"** → the mesh UUID is appended to the whitelist cvar → cvar-changed signal re-evaluates every avatar in view → the same body/head on everyone else lights up too, instantly. Users never see a UUID.

Available from both flat context menus (`menu_attachment_self/other.xml`) and pie menus (under "More >") for self and others.

**Per-pixel skin mask** (Phase C):

`gbuffer3` was extended from `RGB16F` to `RGBA16F`. The `.a` channel carries a `skin` bit. The SSS pass reads this bit (via `emissiveRect`) per pixel, so only skin pixels are blurred — clothing, hair, glasses, eyes are untouched.

**World-space-scaled blur** (Jimenez "Separable SSS" pattern, Phase D):

```
r_eff = aya_blur_radius / max(eye_dist_m, 1m)
```

- Up to 1m the radius is capped at `aya_blur_radius` (close-up SSS stays meaningful)
- Past 1m, radius decreases inversely with distance
- At ~10m, radius < 1px, blur becomes a structural no-op

This **replaces the earlier smoothstep distance fade** (two cvars) which had a "blur-of-blur" failure mode at mid distance. The world-space formulation makes the distance-fade logic self-cancelling.

**Glow restore** (Phase D1):

```
glow_additive = pow(blurred_lit, 3) × glow_gain × glow_color
```

Single-pass additive on top of the blurred result. No extra render target. The warm salmon default (1.0, 0.65, 0.5) was chosen because pure white reads as "skin glowing" rather than "blood under skin".

### Settings (Preferences → Graphics → SSS tab)

| Key | Default | Purpose |
|---|---|---|
| `AYAR20AvatarSkinSSSEnabled` | `1` (ON) | Master switch for SSS |
| `AYAR20AvatarSkinSSSBlurRadius` | `1.0` | Pixel radius at eye_dist = 1m (world-space-scaled past that) |
| `AYAR20AvatarSkinSSSStrength` | `0.7` | Blur strength |
| `AYAR20AvatarSkinSSSGlowGain` | `3.0` (max 5.0) | Glow restore intensity |
| `AYAR20AvatarSkinSSSGlowColor` | warm salmon (1.0, 0.65, 0.5, 1.0) | Glow tint |
| `AYAR20AvatarSkinSSSWhitelist` | — (user-grown) | Newline-separated mesh UUIDs |

UI also includes per-cvar **Default** buttons, a **Reset all to defaults** button, and a **Lock** checkbox that gates the whitelist text editor to read-only (prevents accidental edits).

### Known limitations

- **No seed UUID list shipped** — first-launch is "nothing is whitelisted yet". Right-click learning is the path. Seeding mainstream body/head UUIDs is considered for r21+ once community-list maintenance is designed (§6.1 of the spec).
- **No transmittance** — true ear/finger light transmission (rim glow physics) is r21+ territory. r20 covers diffuse-side SSS only.
- **No per-skin-tone parameters** — single global blur/strength/glow values. Adjusting per ethnicity / skin tone is parked for r21+.
- **Non-mesh attachments grey-out the menu** — old sculpt prims and basic prims can't be added (no mesh UUID to extract).
- **Pre-PBR / old hair shaders** may write `.a` differently in non-PBR variants; the SSS pass falls back to "no skin pixel here" and behaves safely (skin just doesn't blur).

### Implementation summary

- `llayaskinsss.{h,cpp}` (new) — `SkinSSSMatcher` singleton, mesh UUID extraction, whitelist parsing, per-avatar re-evaluation
- `class1/deferred/skinSSSV.glsl` + `skinSSSF.glsl` (new) — 2-pass separable SSS blur, wavelength weights, world-space-scaled radius, `emissiveRect.a` skin mask
- `pipeline.cpp` — `doSkinSSS()` 2-pass driver, gbuffer3 extended to RGBA16F
- `llvoavatar.cpp` — `attachObject` / `detachObject` route into `SkinSSSMatcher`
- `llviewermenu.cpp` — `SSS.Add` / `SSS.Remove` / `SSS.EnableAdd` / `SSS.EnableRemove` handlers
- `panel_preferences_sss.xml` (new) — Preferences → Graphics → SSS tab
- `menu_attachment_self/other.xml` + `menu_pie_attachment_self/other.xml` — right-click entries
- `settings.xml` — six SSS cvars (Enabled / BlurRadius / Strength / GlowGain / GlowColor / Whitelist)
- Shader cache tag bumped (`AYASTORM_SHADER_CACHE_TAG = "AYAstorm r20"`) so old compiled shaders auto-invalidate

### Documentation

- r20 full spec (Phase A–E status, identifier strategy comparison, debug-settings restore guide): `doc/spec_avatar_skin_sss.md`
- gbuffer3 storage extension reference (RGBA16F skin bit): memory `reference_gbuffer3_storage.md`
- Deferred shader routing reference (writer → SSS mask path): `docs/ayastorm-deferred-shader-routing.md`
- Visual-realism chapter roadmap (B-axis complete = r20): `docs/ayastorm-visual-realism-roadmap.md`
- Restore-debug-settings note for sustained-test override values: memory `feedback_restore_debug_settings.md`
