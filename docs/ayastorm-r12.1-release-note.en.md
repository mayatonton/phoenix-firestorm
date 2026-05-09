# AYAstorm r12.1 — Release Announcement Draft

Short text intended to be pasted into the GitHub release page. **r12.1 is the first public release that bundles r11 + r12 + r12.1 together**, so this note presents the cumulative r10 → r12.1 delta as a single unit.

Feature details live in the user-facing guide (`doc/3dstream-tag-guide.{ja,en,zh}.md`) and the spec docs. This note is link-only + diff highlights.

---

## AYAstorm r12.1 — Binaural + Venue Reverb + stereo→5.1 upmix + LFE gain + parcel music quality

A single jump from r10 to r12.1 delivers all of the below at once. **r11 and r12 were not released independently — to keep the tag-format change consolidated into one stage**, both are bundled into this release.

### From r11 — Broadcaster-controlled headphone feel + venue feel

- **`{binaural:on|off}` (short `bin`)**: lite-HRTF (ITD + air-absorption HF rolloff) inserted per channel. Details → [tag-guide §7.1](../doc/3dstream-tag-guide.en.md#71-binauralonoff-short-form-bin) / spec `doc/spec_binaural_venue_reverb.md`
- **`{venue:NAME}` (short `v`)**: 9 venue reverb presets — `dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`. Details → [tag-guide §7.2](../doc/3dstream-tag-guide.en.md#72-venuename-short-form-v)
- **`{wetgain:N}` (short `wg`)**: wet multiplier in 0.0–2.0. All IRs are unity-gain normalized so no retune is needed when switching venue. **Default changed in r12.1 from `1.0` to `0.2`** (reflects the practical musical range — see r12.1 section below). Details → [tag-guide §7.3](../doc/3dstream-tag-guide.en.md#73-wetgainn-short-form-wg)
- **Broadcaster-driven model**: these keys are root-prim-Description-as-truth. **No new listener Preferences UI**. For exceptional rescue use only, sentinel debug settings `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` are provided. Details → [tag-guide §7.5](../doc/3dstream-tag-guide.en.md#75-broadcaster-driven-model) / [§12.2](../doc/3dstream-tag-guide.en.md#122-debug-settings-advanced-tuning)

### From r12 — Expand stereo broadcasts into 5.1 placement

- **`{upmix:on|off}` tag**: in-viewer DSP generates 6ch (FL/FR/C/Ls/Rs/LFE) from 2ch. Given that most SL streaming software is stereo-only, this lets the 6-speaker placement built in r10 **be experienced even on stereo broadcasts**. Details → [tag-guide §8](../doc/3dstream-tag-guide.en.md#8-stereo51-upmix-r12) / spec `doc/spec_stereo_upmix.md`
- **Algorithm is fixed (NG1)**: DPL2-family matrix decode + band separation (LFE LPF / center bleed removal / rear decorrelation). No `{upmix:dpl2|logic7|...}` choice (= "do not increase expressive ambiguity" policy).
- **Auto-bypass on 5.1-native source**: when source ch ≥ 6, `{upmix:on}` is auto-bypassed (chat notification once). The same Description works for both stereo and 5.1-native material.
- **Default is `off` (opt-in)**: r10 behavior preserved; broadcaster opts in via tag.
- **Tag short-forms**: `binaural`/`venue`/`wetgain` → `bin`/`v`/`wg`, and the 9 venue values get 1–2-char aliases (`d`/`rs`/`rm`/`hs`/`hm`/`hl`/`cl`/`ct`/`od`). Long form and short form are fully equivalent. Helps fit within SL's 127-byte Description limit. Details → [tag-guide §4.5](../doc/3dstream-tag-guide.en.md#45-short-forms-for-key-names--venue-values-r12--r121)
- **LSL helper update** (`doc/lsl/aya_3dstream_setup.lsl`): r11 tags (binaural/venue/wetgain), r12 tag (upmix), and r12.1 tag (lfegain) are settable from the menu/dialog. Output **always uses short form**.
- **macOS build returns**: macOS was paused at r10.x (Linux/Windows only); r12 brings it back. Mac users should migrate r10 → r12.1.

### From r12.1 — LFE gain + parcel music quality + live-tuning fix

- **`{lfegain:N}` (short `lg`)**: gain multiplier for the `{ch:LFE}` route and the LFE band produced by `{upmix:on}` (range 0.0–4.0, default 1.0). Used to boost a quietly-recorded LFE bus on the listener side, or to set `0` when the LFE prim is mounted on a non-subwoofer speaker to stop low-end leakage. The listener-side sentinel `Stream3DLfeGain` is added in lockstep. Details → [tag-guide §7.4](../doc/3dstream-tag-guide.en.md#74-lfegainn-short-form-lg-added-in-r121) / spec `doc/spec_stereo_upmix.md` §4.7
- **`wetgain` default `1.0` → `0.2`**: `1.0` saturated the source on hall / cathedral presets; the new default reflects the practical musical range (0.1–0.5) confirmed by listening tests. The LSL UI quick-pick was also re-graded to fine `0.1`–`0.5` increments. Details → [tag-guide §7.3](../doc/3dstream-tag-guide.en.md#73-wetgainn-short-form-wg)
- **Live-tuning fix for listener-side debug settings**: at r12 release, `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` regressed and only took effect after a prim touch (Description re-parse). r12.1 restores the standard "next-frame apply" semantics. Details → [tag-guide §12.3](../doc/3dstream-tag-guide.en.md#123-persistence-and-immediate-apply)
- **Per-speaker volume tuning is now live**: r10's per-speaker volume trims (`Stream3DSpkVol*`) used to require a rebuild (prim touch) to take effect; r12.1 restores **next-frame apply** for them. Debugging and tuning sessions become considerably less painful.
- **Routing diagnostic is upmix-aware**: the `Stream3DDescriptionScan` routing dump did not reflect the 6ch routing produced by r12 upmix. r12.1 fixes the dump so the channel→speaker mapping is correct even when `{upmix:on}` is in effect.
- **Parcel music playback quality, opt-in**: a new `FSParcelStreamQuality` debug setting addresses the **buffer starvation** observed on high-bitrate (256/320 kbps mp3, FLAC-over-HTTP) parcel music streams (default `0` = bit-identical to upstream FS / `1` = AYAstorm enhanced). With `1`, the stream buffer hint is raised to a 320 kbps assumption, the resampler switches to SPLINE, and a +4 dB high-shelf @ 6 kHz EQ is attached to the stream group to gently restore the presence band that typical 128 kbps mp3 streams thin out. Independent of the 3dstream paths; existing users are unaffected. Details → `doc/spec_parcel_stream_quality.md`

### Existing placements

All prims placed in r8 / r9 / r10 **continue to work without tag edits**. r11/r12/r12.1 features are broadcaster opt-in by design (defaults: `binaural=off` / `venue=dry` / `wetgain=0.2` / `lfegain=1.0` / `upmix=off` = r10 behavior with the wetgain default lowered in r12.1).

### Known limitations

- `hall_medium` / `hall_large` / `cathedral` are CPU-heavy (+7.7 to +10.2 pp vs r10). Lower-spec machines should prefer `room_small` / `room_medium` / `hall_small`. Details → [tag-guide §7.2](../doc/3dstream-tag-guide.en.md#72-venuename-short-form-v) CPU table
- No general listener UI for venue / binaural / lfegain override (broadcaster-driven model).
- The bus-tail VenueReverb is a **stereo IR convolver** — even on a 6-speaker placement, the wet is heard as a fixed master-stereo image rather than per-speaker reverb. Improvement options (per-channel reverb / pre-3D send / L/R wet decorrelation, three candidates) are deferred to r13+. Details → `docs/ayastorm-r12-stereo-upmix.md` §6.4
- SOFA personal HRTF / Steam Audio integration / VenueReverb CPU optimization / air-absorption objective FFT / public README are deferred to r13+.

### IR license

Bundled venue IRs are from OpenAIR (CC-BY 4.0). Sources in `app_settings/venue_ir/CREDITS.md`.

### Documentation

- User-facing guide: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md`
- r11 spec: `doc/spec_binaural_venue_reverb.md`
- r12 / r12.1 spec: `doc/spec_stereo_upmix.md` (r12.1 extension in §4.7)
- r12 / r12.1 implementation log: `docs/ayastorm-r12-stereo-upmix.md` (r12.1 follow-on in §6)
- Parcel music quality spec: `doc/spec_parcel_stream_quality.md` (shipped with r12.1, independent from 3dstream)
- Roadmap: `docs/ayastorm-stream3d-roadmap.md`
