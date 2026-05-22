# AYAstorm r28 — Release Announcement

**r28 extends the r21 self rigged picker to other avatars** — right-clicking a rigged attachment on someone else's avatar now resolves through the same GPU object-ID buffer that r21 introduced for self picking, scoped to one armed target at a time.

Implementation details / load controls / verification log live in the permanent spec (`docs/specs/ayastorm-r28-other-rigged-picker.md`). This note is link-only + diff highlights.

---

## AYAstorm r28 — Other rigged picker

### Headline: right-clicking someone else's attachments now picks what you see, too

r21 made the self picker honour what's actually on screen by routing right-click resolution through a GPU object-ID buffer skinned with the **same matrices used by the visible scene**. r28 carries that same property over to other avatars — when you right-click a rigged attachment on another avatar, the picker re-renders just that one avatar's rigged draw info into `mObjectIDBuffer` and reads back the LocalID at the mouse pixel.

The cost containment story is deliberate:

- Only **one** other avatar is ever armed at a time (the one currently under the cursor or just right-clicked).
- The GPU ID pass is gated by a short arm window (`FSOtherRiggedPickerArmSeconds`, default `1.0` s).
- Hover in default third-person camera does **not** arm — you need to be in alt-cam / orbit / zoom for the picker to engage.
- Mouselook and avatar customise modes skip the picker entirely.
- If the pass exceeds the per-frame draw-call / triangle budget (512 / 1.2 M, hardcoded), the buffer is invalidated and right-click falls back to the upstream worldray pick.

Self picker behaviour from r21 is preserved unchanged. The two pickers share `mObjectIDBuffer` and use a buffer-owner tag to prevent stale reads when the cursor moves between your own avatar and someone else's.

Details → spec `docs/specs/ayastorm-r28-other-rigged-picker.md`

### Settings

| Key | Default | Purpose |
|---|---|---|
| `FSOtherRiggedPickerEnable` | `1` | Master switch for the other-avatar picker. `0` reverts right-click on other avatars to upstream worldray behaviour entirely |
| `FSOtherRiggedPickerGPU` | `1` | Kill-switch for the GPU buffer pass. `0` = no-op, upstream worldray is used unchanged. Provided as escape hatch for environments where the GPU pass misbehaves |
| `FSOtherRiggedPickerArmSeconds` | `1.0` | Seconds the ID pass remains armed after the last hover on the target avatar |

> **Default-camera gate** (`RequireNonDefaultCamera` semantics) and the per-frame **draw-call / triangle budget** (512 / 1,200,000) are hardcoded — not exposed as cvars. Re-tuning, if needed, will move the hardcoded values in a future release rather than expanding the cvar surface.

### Additional fix folded into r28

- **Self avatar face selection**: a regression path where right-clicking your own face left `mPick.mObjectID` un-normalised (not set to `gAgent.getID()`) and broke self-menu detection. r28 normalises the upstream self-object hit to `gAgent.getID()` and falls back to avatar-body selection when the GPU self picker has no attachment hit. Self face right-click now resolves correctly.

### Known limitations

- **HUD attachments**: out of scope (HUD camera is screen-space, not present in `mObjectIDBuffer`) — same as r21.
- **Non-rigged attachments** (single-prim accessories, piercings, etc.): out of scope — falls through to the upstream worldray pick.
- **Alpha-blended hair** on other avatars: same depth limitation as r21 — alpha-discarded triangles miss the picker, but truly alpha-blended hair that does not write depth can still pierce.
- **Crowded venues**: hovering rapidly between many avatars increases arm-switch churn. The 1.0-second arm window and 1-avatar-at-a-time scope are deliberately tight to keep this cost bounded.
- **Heavy outfits**: an avatar with extremely heavy rigged outfit may saturate the 512 draw-call / 1.2 M triangle budget; in that case the GPU pass is invalidated for that frame and right-click falls back to the upstream worldray pick.

### Credits

The r28 implementation (other-rigged ID pass, buffer owner discriminator, default-camera gate, budget enforcement, self face selection fix) is by [@t-noami](https://github.com/t-noami).

### Documentation

- r28 spec / architecture / load controls / verification log: `docs/specs/ayastorm-r28-other-rigged-picker.md`
- r21 self picker spec (sibling feature): `docs/specs/ayastorm-r21-self-rigged-picker.md`
- Attachment rendering routing reference: `docs/specs/ayastorm-attachment-rendering-routing.md`
