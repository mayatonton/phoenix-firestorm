# AYAstorm r31 — Release Announcement

**r31 unifies the 3D Stream linkset routing tag under `[3dstream:...]`**. The tag previously split between `[3dstream:...]` for single-prim mono URL streams and `[3dstream-stereo:...]` for linkset distributed routing is now reachable through a single `[3dstream:...]` prefix. All existing tags continue to be accepted for compatibility.

> **Distribution**: r31 ships as one of the features in the r25–r30 + r31 bundle tag (`v7.2.4-ayastorm-r31+bundle-fs.80646`). Other bundled release notes and the Firestorm upstream FS-7.1.18.80646 carry-over are linked directly from the GitHub Release page.

The single source of truth for the spec is `docs/specs/ayastorm-r31-3dstream-unified-tag.md`. The practical streamer guide is `docs/guides/3dstream-tag-guide.{en,ja,zh}.md` and the user guide is `docs/specs/3dstream-user-guide.{en,ja,zh}.md`. This note is the entry point and diff highlight.

---

## AYAstorm r31 — `[3dstream:...]` unified linkset tag

### Headline: one tag for both mono and linkset routing

Since r6, 3D Stream has had two tag prefixes:

- `[3dstream:{url:...}]` — single-prim mono URL stream
- `[3dstream-stereo:...]` — linkset distributed stereo / 5.1 / MOAP routing

r31 collapses this into **one prefix** that new authors need to remember: `[3dstream:...]`. The same prefix now covers both mono and linkset routing.

Promotion rule: whether `[3dstream:...]` is treated as linkset routing depends on whether any child prim in the linkset carries a valid `{ch:...}`. With no `{ch:...}` children, the root `[3dstream:{url:...}]` continues to play as mono just like before.

### Compatibility

All existing forms **continue to be accepted**. No rewrite of existing content is required:

- `[3dstream:{url:...}]` — single-prim mono URL stream
- `[ayastream:{url:...}]` — legacy mono alias
- `[3dstream-stereo:...]` — legacy linkset/distributed tag
- `[ayastream-stereo:...]` — legacy linkset/distributed alias

For new content, prefer `[3dstream:...]`, but there is no need to rush existing content over.

### Common forms

**Mono URL (unchanged)**

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

**Stereo / distributed (linkset)**

```text
root:  [3dstream:{url:http://example.com/stream.mp3}]
left:  [3dstream:{ch:L}]
right: [3dstream:{ch:R}]
```

With a `{ch:...}` child present, the root's mono binding is automatically promoted to linkset routing.

**MOAP stereo / 5.1**

```text
root: [3dstream:{source:media}]
L:    [3dstream:{ch:L}]
R:    [3dstream:{ch:R}]
```

```text
root: [3dstream:{source:media-5-1}]
FL:   [3dstream:{ch:FL}]
FR:   [3dstream:{ch:FR}]
C:    [3dstream:{ch:C}]
LFE:  [3dstream:{ch:LFE}]
SL:   [3dstream:{ch:SL}]
SR:   [3dstream:{ch:SR}]
```

### Configuration — `{bin}` / `{binaural}` default change

For the new `[3dstream:...]` tag, the **default when `{bin}` / `{binaural}` is unspecified is now `off`**. Venues that want binaural DSP should opt in with `{bin:on}`.

Rationale: changes to how existing placements sound, without tag rewrites, are kept to a minimum. The r12-era binaural DSP becomes an explicit opt-in for venues that want it. The listener-side debug override `Stream3DBinauralRender = 0 / 1` is still available (`-1` = follow tag).

### 5.1ch streaming codec guidance

5.1ch streaming codec guidance is refreshed to match the post-r9-opus state:

- **Vorbis 6ch** — verified on the live build
- **Opus 6ch** — verified on the live build via the r9-opus codec plugin (no FMOD parser seek constraint)
- **FLAC 6ch** — codec layout is implemented, but some delivery paths require seek and can fail with `FMOD_ERR_FILE_COULDNOTSEEK`

For live streaming, **prefer Opus 6ch**. [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) is the recommended practical streaming path.

### Upmix description change

Stereo → 5.1 upmix is now described as **matrix upmix + band separation** without referring to any commercial decoder name. The implementation is a fixed `LLStereoUpmix` algorithm (center is `(L+R)/√2`, SL/SR are delayed `±(L-R)/√2`, LFE is the LPF), described in terms of its math rather than any product name.

### Other-viewer / MOAP fallback

Other viewers do not interpret 3D Stream tags:

- `{url:...}` 3D Stream audio does not play on other viewers
- The **media face itself** for `{source:media}` continues to display and play as a normal MOAP on other viewers
- `{ch:...}` routing, upmix, binaural, and venue reverb are AYAstorm-only

For setups that show a media surface while playing `{url:...}` 3D Stream audio, the media side continues to act as a normal MOAP for other viewers.

### Error notification tag examples

Local Chat tag-format error notifications (`BadCh` / `BadRange` / `BadVolume`, etc.) are now exemplified in the new recommended `[3dstream:...]` form. Legacy `[3dstream-stereo:...]` / `[ayastream-stereo:...]` are still accepted; the internal log label `[3dstream-stereo]` remains as a subsystem/debug label and does not imply user-facing tag recommendation.

### Known constraints

- **Linkset promotion only on `{ch:...}` discovery**: the mere existence of child prims does not promote `[3dstream:...]` from mono to linkset. At least one child with a valid `{ch:...}` is required
- **Child description fetch delay**: while child prim descriptions are still being fetched, the root URL plays as mono; once `{ch:...}` is detected, the binding moves to linkset routing
- **`{source:media}` / `{source:media-5-1}` have no mono fallback**: these are distributed source declarations and require at least one `{ch:...}` speaker

### Implementation summary

- `LLPositionalStreamMgr::parseDistributedStereoTag()` now reads `[3dstream:...]` as distributed grammar
- `DistStereoTagData::unified_3dstream_prefix` records whether the tag came from the legacy `[3dstream-stereo:...]` or the unified `[3dstream:...]` form
- `evaluateBinding()` treats `[3dstream:{url:...}]` as mono initially, then drops the mono binding and forwards to `evaluateLinkset()` once a `{ch:...}` is observed on the same tag or a known linkset child
- `evaluateLinkset()` falls back unified `[3dstream:{url:...}]` roots with no `{ch:...}` speakers back to mono
- `LLPositionalStreamMgr::effectiveBinaural()` resolves an unspecified tag as `false`

### Documentation

- r31 unified tag spec (single source of truth): `docs/specs/ayastorm-r31-3dstream-unified-tag.md`
- Cumulative streamer-facing guide: `docs/guides/3dstream-tag-guide.{en,ja,zh}.md`
- User-facing how-to: `docs/specs/3dstream-user-guide.{en,ja,zh}.md`
- Earlier 3D Stream chapter lineage (r6 onward): `docs/specs/ayastorm-r*-*.md`
