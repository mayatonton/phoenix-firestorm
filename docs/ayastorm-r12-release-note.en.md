# AYAstorm r12 — Release Announcement Draft

Short text intended to be pasted into the GitHub release page. AYA pastes this when the release decision is finalized.

Feature details live in the user-facing guide (`doc/3dstream-tag-guide.{ja,en,zh}.md`) and the spec docs. This note is link-only + diff highlights.

---

## AYAstorm r12 — Binaural + Venue Reverb + stereo→5.1 upmix

A single jump from r10 to r12 delivers all of the below at once. **r11 is not released independently — to avoid splitting the tag-format change into two stages**, it is bundled wholesale into r12.

### From r11 — Broadcaster-controlled headphone feel + venue feel

- **`{binaural:on|off}` (short `bin`)**: lite-HRTF (ITD + air-absorption HF rolloff) inserted per channel. Details → [tag-guide §7.1](../doc/3dstream-tag-guide.en.md#71-binauralonoff-short-form-bin) / spec `doc/spec_binaural_venue_reverb.md`
- **`{venue:NAME}` (short `v`)**: 9 venue reverb presets — `dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`. Details → [tag-guide §7.2](../doc/3dstream-tag-guide.en.md#72-venuename-short-form-v)
- **`{wetgain:N}` (short `wg`)**: wet multiplier in 0.0–2.0. All IRs are unity-gain normalized so no retune is needed when switching venue. Details → [tag-guide §7.3](../doc/3dstream-tag-guide.en.md#73-wetgainn-short-form-wg)
- **Broadcaster-driven model**: these 3 keys are root-prim-Description-as-truth. **No new listener Preferences UI**. For exceptional rescue use only, sentinel debug settings `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` are provided. Details → [tag-guide §7.4](../doc/3dstream-tag-guide.en.md#74-broadcaster-driven-model) / [§12.2](../doc/3dstream-tag-guide.en.md#122-debug-settings-advanced-tuning)

### r12-only — Expand stereo broadcasts into 5.1 placement

- **`{upmix:on|off}` tag**: in-viewer DSP generates 6ch (FL/FR/C/Ls/Rs/LFE) from 2ch. Given that most SL streaming software is stereo-only, this lets the 6-speaker placement built in r10 **be experienced even on stereo broadcasts**. Details → [tag-guide §8](../doc/3dstream-tag-guide.en.md#8-stereo51-upmix-r12) / spec `doc/spec_stereo_upmix.md`
- **Algorithm is fixed (NG1)**: DPL2-family matrix decode + band separation (LFE LPF / center bleed removal / rear decorrelation). No `{upmix:dpl2|logic7|...}` choice (= "do not increase expressive ambiguity" policy).
- **Auto-bypass on 5.1-native source**: when source ch ≥ 6, `{upmix:on}` is auto-bypassed (chat notification once). The same Description works for both stereo and 5.1-native material.
- **Default is `off` (opt-in)**: r10 behavior preserved; broadcaster opts in via tag.

### Common to r12

- **Tag short-forms**: `binaural`/`venue`/`wetgain` → `bin`/`v`/`wg`, and the 9 venue values get 1–2-char aliases (`d`/`rs`/`rm`/`hs`/`hm`/`hl`/`cl`/`ct`/`od`). Long form and short form are fully equivalent. Helps fit within SL's 127-byte Description limit. Details → [tag-guide §4.5](../doc/3dstream-tag-guide.en.md#45-short-forms-for-key-names--venue-values-r12)
- **LSL helper update** (`doc/lsl/aya_3dstream_setup.lsl`): r11 tags (binaural/venue/wetgain) and r12 tag (upmix) are settable from the menu/dialog. Output **always uses short form**.
- **macOS build returns**: macOS was paused at r10.x (Linux/Windows only); r12 brings it back. Mac users should migrate r10 → r12.

### Existing placements

All prims placed in r8 / r9 / r10 **continue to work without tag edits**. r11/r12 features are broadcaster opt-in by design (defaults: `binaural=off` / `venue=dry` / `wetgain=1.0` / `upmix=off` = r10 behavior).

### Known limitations

- `hall_medium` / `hall_large` / `cathedral` are CPU-heavy (+7.7 to +10.2 pp vs r10). Lower-spec machines should prefer `room_small` / `room_medium` / `hall_small`. Details → [tag-guide §7.2](../doc/3dstream-tag-guide.en.md#72-venuename-short-form-v) CPU table
- No general listener UI for venue / binaural override (broadcaster-driven model).
- SOFA personal HRTF / Steam Audio integration / VenueReverb CPU optimization / air-absorption objective FFT / public README are deferred to r13+.

### IR license

Bundled venue IRs are from OpenAIR (CC-BY 4.0). Sources in `app_settings/venue_ir/CREDITS.md`.

### Documentation

- User-facing guide: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md`
- r11 spec: `doc/spec_binaural_venue_reverb.md`
- r12 spec: `doc/spec_stereo_upmix.md`
- r12 implementation log: `docs/ayastorm-r12-stereo-upmix.md`
- Roadmap: `docs/ayastorm-stream3d-roadmap.md`
