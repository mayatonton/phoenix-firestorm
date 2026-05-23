> **Language / 言語 / 语言**: **English** · [日本語](./skin-sss-user-guide.ja.md) · [中文](./skin-sss-user-guide.zh.md)

# Skin SSS (Subsurface Scattering) — User guide

**Available in**: AYAstorm r20 and later. UI relocated to **AYAstorm Controls** in r30.

**Where**: Top menu → **AYAstorm → AYAstorm Controls...** (shortcut `Alt+C`) → **Skin SSS** tab.

![Skin SSS panel in AYAstorm Controls](./images/skin-sss/panel-overview.png)

### Applied example (same scene, same lighting, same camera)

| Skin SSS **OFF** | Skin SSS **ON** |
|:---:|:---:|
| ![SSS off](./images/skin-sss/example-off.png) | ![SSS on](./images/skin-sss/example-on.png) |
| Skin highlights are sharp; the cheek-to-shadow transition reads as a clear edge. | Highlights soften, the cheek-to-shadow transition becomes a gentle gradient, and a faint warm hue appears around the cheeks and nose. |

---

## 1. What Skin SSS does

Real skin is not a hard surface — light enters slightly, scatters under the surface, and comes back out softened and slightly tinted (the reason ears glow red against a window, and why portrait photos have that "warm" softness around the cheeks and nose). In SL, many Skin brands have spent years pushing this problem forward through sheer texture craft and artistic skill, and the skin work you see in SL today is genuinely beautiful. **Skin SSS** is built with the intent of **supporting that work from the renderer side** — another layer underneath the artistry that is already there.

It applies a screen-space approximation of subsurface scattering on the avatar skin only, leaving clothing, hair, and props untouched. The visible result:

- Cheeks, ears, and nose get a soft, slightly warm transition between lit and shadowed areas
- Hard specular peaks on the skin soften into a warmer bloom, so the highlight reads as light pooling on skin rather than reflecting off a hard surface
- Optional red-shifted "glow" restores the small specular peaks that the blur would otherwise mute, for a more healthy, life-like complexion

The effect is intentionally subtle in motion and unmistakable in stills — designed for portrait / cinematic photography.

## 2. Prerequisites

| Requirement | Notes |
|---|---|
| View Mode | **AYAstorm View** (Preferences → Graphics). Firestorm View does not run the SSS pass. |
| Deferred Rendering | Required (always on for AYAstorm View). |
| Avatar mesh body / head | Any Mesh asset whose UUID is registered in the whitelist (§4). **The whitelist ships empty** — you have to register your Mesh body / head once before SSS can find them. |

## 3. The controls

All controls live in the **Skin SSS** tab of AYAstorm Controls. Each row has a small **D** button on the right to restore the AYAstorm default for that value.

| Control | What it does | Default |
|---|---|---|
| **Enabled SSS** | Master switch for the SSS pass. Off = standard SL skin lighting. | ON |
| **Blur radius** | How far light "leaks" inside the skin, measured at a 1 m viewing distance. The shader automatically scales this down as the avatar moves away, so the look stays consistent regardless of camera distance. | 1.0 |
| **Strength** | Mix between the original lit pixel and the SSS-blurred pixel. 0.0 = no SSS visible / 1.0 = full SSS. Most portraits look natural around 0.4 – 0.6. | 0.5 |
| **Glow gain** | How strongly the small red-shifted highlight is added back after the blur. Compensates for the slight "sleepiness" that strong blur introduces on peak highlights. 0.0 = pure SSS / higher = more visible blood-flush highlight. | 0.2 |
| **Glow color** | Tint of that highlight restore. Pure red (1, 0, 0) by default reads as healthy blood flush. Push toward orange for warmer skin, toward magenta for cooler / fairer skin. | Pure red |
| **Reset all to defaults** | Resets the five values above **and** the whitelist (§4) to AYAstorm defaults in one click. | — |

The recommended way to dial in the look is: leave **Blur radius** at 1.0, adjust **Strength** until the cheeks and nose feel soft, then nudge **Glow gain** up just enough to bring the specular peaks back.

## 4. The whitelist — telling the viewer which mesh is "skin"

SL has no API to ask "is this mesh skin or clothing?" — every Mesh body, Mesh head, and Mesh hand looks the same to the renderer. To avoid blurring eyes, teeth, nails, or accessories by mistake, Skin SSS only runs on **Mesh asset UUIDs that you have explicitly registered**.

### Registering a mesh

1. **Wear** the Mesh body / Mesh head you want SSS on.
2. **Right-click** the attached object in-world (on your own avatar) → **Add to SSS whitelist**.
   - For multi-part bodies (head + body + hands), use **Add entire linkset to SSS whitelist** in the same menu to register every linked part at once.
3. The mesh's asset UUID is appended to the whitelist text area in the **Skin SSS** tab.

**The whitelist ships empty.** Once you enable SSS, you need to register the Mesh body / head you actually wear with the steps above before the effect becomes visible.

### Editing the whitelist by hand

The whitelist is a plain UUID list — one 36-character UUID per line. To prevent accidental edits, the text area starts **locked** (read-only):

- Uncheck **Lock editing (prevent accidental changes)** to enable typing.
- Paste UUIDs from notecards or other sources, one per line.
- Re-check the lock when done. The list is saved automatically.

### Removing a mesh

Right-click the attached object → **Remove from SSS whitelist** (or **Remove entire linkset…** for multi-part bodies).

## 5. Quick start

1. AYAstorm → AYAstorm Controls (`Alt+C`).
2. Open the **Skin SSS** tab.
3. Make sure your View Mode is **AYAstorm View**. (Preferences → Graphics if it isn't.)
4. Check **Enabled SSS**.
5. Look at your avatar's face. If the highlights still feel hard and unchanged, your Mesh head is not in the whitelist yet — right-click the head → **Add to SSS whitelist**.
6. If you want a softer look, raise **Strength** toward 0.7. If the blood-flush starts looking a bit muted, raise **Glow gain** to ~0.3.

## 6. Tips

- **For portraits**: increase **Strength** slightly (0.6 – 0.7) and let **Glow gain** sit around 0.2 – 0.3. The skin will read as soft but not flat.
- **For full-body or scene shots**: the default 0.5 / 0.2 is usually right. Heavier values become invisible at distance anyway because the blur radius auto-scales down.
- **If the eyes / teeth blur**: that mesh's UUID was registered as skin by mistake. Right-click the eyes / teeth attachment → **Remove from SSS whitelist**.
- **If nothing changes when you toggle Enabled**: check View Mode (must be AYAstorm View) and check that **at least one** attached mesh's UUID is in the whitelist.
- **Performance**: SSS is a single screen-space pass, comparable in cost to SSAO. It is not a major frame-time contributor.

## 7. Restoring defaults

- **One value**: press the **D** button next to that row.
- **Everything in this tab (including the whitelist)**: press **Reset all to defaults**.

The whitelist default is **an empty list**, so resetting wipes every Mesh body / head you registered. You'll need to re-register them via §4, so use this knowingly.

## 8. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Skin looks identical with SSS on/off | View Mode is Firestorm View | Switch to AYAstorm View in Preferences → Graphics |
| Some parts of the body (hands / feet) still have hard highlights while the face has softened | Hands / feet are a separate Mesh asset and not in the whitelist | Right-click them → Add to SSS whitelist |
| Eyes, teeth, or nails look blurry | Those attachments share a Mesh UUID with the body, or were added to the whitelist by mistake | Right-click → Remove from SSS whitelist (or remove the UUID from the text area) |
| Whitelist text area is read-only | The lock is on (the default safety state) | Uncheck **Lock editing** |
| Effect looks too strong / too soft | Strength is too high / too low | Adjust **Strength**; for natural look stay between 0.4 and 0.6 |

## 9. Where to dig deeper

- Engineering spec for the SSS shader and the whitelist mechanism: [`docs/specs/ayastorm-r20-avatar-skin-sss.md`](./ayastorm-r20-avatar-skin-sss.md)
- UI relocation (r30): [`docs/specs/ayastorm-r30-aya-controls-tab-overhaul.md`](./ayastorm-r30-aya-controls-tab-overhaul.md)
