🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO recovery + Bridge defense + 3D Stream filter + alpha render-order + Cinematic glow + Underwater alpha

> [!IMPORTANT]
> **r31-bugfix-2 is a release that, on the AYAstorm side, stops two structural behaviors that exist across the entire Firestorm viewer family.**
>
> These are not AYAstorm-specific issues; they originate from the shared inventory root used by every Firestorm-derived viewer (whether they are bugs or by-design is upstream's call to make).

1. **AO delete behavior**: in Firestorm-family viewers (upstream Firestorm / older AYAstorm builds / other FS-derived viewers), pressing "Delete" on an AO set permanently erases AO data under the shared inventory root `#Firestorm`, so logging in from a different viewer still shows it gone — a cross-viewer cascading deletion. Observed at ~1000-user scale. In r31-bugfix-2, normal AO-set delete becomes a non-destructive per-account Hide. The only destructive path is the explicit `Delete selected` action in the Hidden manager, behind a confirmation dialog
2. **LSL Bridge version collision**: the version-mismatch auto-recreate logic in `fslslbridge.cpp` is structured so that a future minor-version bump on upstream Firestorm could take AYAstorm-side Bridges down with it. Both sides are currently at `v2.29` so it has not fired yet; we ship a one-way defense — `if received version > ours, adopt` — and also accept newer bridges during startup attach before the `BridgeVer` message arrives

These fixes address structural behaviors that exist across the Firestorm viewer family. We remove destructive `#Firestorm` operations from the normal AO delete path on the AYAstorm side; recurrence in upstream Firestorm or other derived viewers needs each viewer to be patched on its own (the recommended workflow and workarounds are spelled out in the recovery guide).

## For users whose AO sets are already gone — AO re-setup procedure

> [!IMPORTANT]
> If your AO sets have already been erased by the deletion event described above, **the AO data itself cannot be brought back, either by the viewer or by the SL server**. The AO functionality itself, however, can be back in normal working order after a simple re-setup. **Step-by-step recovery guides are published in 3 languages — please use the one matching your environment:**
>
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
>
> Installing r31-bugfix-2 itself **stops the same event from recurring on the AYAstorm side**. Recurrence on upstream Firestorm / other FS-derived viewers needs each viewer to be patched on its own — workarounds for that case are spelled out inside the recovery guide.

## Attachment alpha render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

This is the **second major fix** bundled in r31-bugfix-2 alongside the AO recovery work. The previously shipped "rigged hair / SIM N-BL render-order swap" (r30 §5) resolved sky-bleed through hair, but caused attachment N-BL prims (eyelash prims, etc.) to be drawn before rigged hair and then over-blended away — a clearly visible regression on certain avatars (eyes / brows / eyelashes appearing washed out or missing on heads that use attachment alpha prims).

The POST_WATER forward pass is now split into three sub-passes (SIM N-BL → all R-BL → attachment N-BL) using `LLDrawInfo::mAttachedToAvatar` as the per-draw discriminator. This restores correct front-most ordering for attachment prims while keeping the §5 swap fix for sky-bleed through hair. Applies automatically — no setting change needed.

See [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md) for the structural compare against the falsified A/B/C alternatives.

### Special thanks (PR #122 — attachment alpha render-order)

**neria (neriamm)** helped enormously on this fix. Reproduction setups across multiple SL avatars and hands-on verification of the candidate approaches meaningfully shortened the structural compare and the final implementation choice. neria is a Second Life resident contributor (not a GitHub account).

## Other bundled fixes

r31-bugfix-2 also rolls up three additional fixes that landed after r31-bugfix-1:

- **3D Stream URL filter + UI update** (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121)): the 3D Stream feature now ships disabled by default; the prompts that ask viewers whether to play a stream URL now display the source identity (object name + owner) so each viewer can decide based on who is sending the URL, and unintended auto-play from rezzed objects is blocked at the source-of-truth level. See [`docs/specs/3dstream-user-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.en.md) for the user-facing guide
- **Cinematic glow min-luminance bugfix** (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123)): the BD-parity port carried `RenderGlowMinLuminance = 0.0` into Cinematic mode, which let blank-texture color-tinted prims (both attachments and SIM rez objects) fire post-process bloom even with Glow set to zero on the prim. The threshold is raised to `0.5`, and a one-shot migration force-corrects users who have the bad value persisted from r31.0 / r31.1 the next time they boot into Cinematic mode. Firestorm mode is unaffected (LL default `1.0` stays in place; the migration is skipped and re-checked on the next Cinematic boot)
- **Underwater alpha plate fix** (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124)): the r30 P5 transparent-DoF C-(a) `mAYAAlphaColor` redirect did not gate on `LLPipeline::sUnderWaterRender`. Underwater, the main RT carried the underwater fog-tinted opaque scene while the separated alpha plate was cleared to `(0,0,0,0)`; the pre-tonemap composite (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`) then let the plate paint over the underwater-tinted main RT — eyelash / brow attachment alpha prims and SIM particle transparent regions rendered as solid black underwater, and the same break transiently reappeared for a few frames after surfacing. The redirect is now skipped while `sUnderWaterRender` is true so forward alpha goes straight to the main RT (upstream FS-compatible behavior underwater). The above-water path is unchanged

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- AO + Bridge technical spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- AO user recovery guide (3 languages): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)
- 3D Stream URL filter report (日本語): [docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- 3D Stream user guide (3 languages): [docs/specs/3dstream-user-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/specs)
- Alpha render-order extension report: [docs/specs/ayastorm-double-alpha-c-plan-extension.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- Six-category render order trace: [docs/specs/ayastorm-six-category-render-order-trace.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

## Compatibility with existing setups

Ships without disturbing r31 / r31-bugfix-1 environments:

- **AO delete-behavior fix**: applies automatically. No setting change required. Users on r31 / r31-bugfix-1 install r31-bugfix-2 on top and normal AO-set delete becomes non-destructive Hide
- **LSL Bridge collision defense**: applies automatically. Prevents AYAstorm-side Bridges from being destroyed when upstream Firestorm bumps the Bridge minor version (not yet firing today, defensive for the future)
- **3D Stream default-off + URL filter**: existing streamers who were already running with 3D Stream enabled keep their `Stream3DEnabled = true` setting. Fresh installs land on default OFF
- **Alpha render-order 3-pass dispatch**: applies automatically to all avatars. Attachment N-BL prims (eyelash prims, etc.) now sit in front of rigged hair as expected, while the original §5 swap fix for sky-bleed through hair is preserved
- **Cinematic glow min-luminance**: one-shot migration runs only on the next Cinematic-mode boot for users whose persisted value was `0.0`. Firestorm-mode-only users are not touched (LL default `1.0` stays in place)
- **Underwater alpha plate**: applies automatically. Underwater alpha BLEND now writes straight to the main RT instead of the separated plate, so the underwater fog tint is preserved through compositing. Side effect: the transparent-DoF C-(a) plate composite is disabled while underwater (visually inert — underwater is already a fog-saturated wash where DoF bokeh is structurally invisible). Above water is unchanged
- **All r31 / r31-bugfix-1 features** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow fix, etc.): preserved unchanged
- **Users who don't edit or delete AO sets**: almost no visible change. The AO-set trash icon becomes a visibility-off icon, and the dialog wording changes to Hide
- **Users who want to clean up AO inventory**: the Hidden manager has `Delete selected`, which permanently deletes the selected hidden inventory folder after confirmation. This cannot be undone
- **Users whose AO sets are already gone**: installing r31-bugfix-2 **stops the same event from recurring**. Lost AO data itself cannot be restored, but the re-setup procedure above gets AO functionality back into working order right away

## IR licence

Venue IRs bundled since r11 (and still shipping) are from OpenAIR (CC-BY 4.0). Sources: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261492019_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81339.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261500427.tar.xz)

## Contributors

@t-noami @mayatonton

(See the Special thanks callout in the "Attachment alpha render-order" section for neria (neriamm)'s contribution to PR #122.)
