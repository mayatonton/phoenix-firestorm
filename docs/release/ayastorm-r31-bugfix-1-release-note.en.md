# AYAstorm r31-bugfix-1 — Release Announcement

**r31-bugfix-1 fixes a structural bug where SSS pink shadow leaked through FullBright prims in the shape of avatars behind them** — a bug that has existed structurally since SSS was first added in the r20+ visual realism chapter, not something introduced by r31.

Implementation details, routing investigations, and design notes live in four permanent effect-axis routing maps under `docs/specs/`. This note is the entry point and diff highlight.

---

## AYAstorm r31-bugfix-1 — FullBright-prim SSS pink-shadow leak fix

### Headline: removing the avatar-shaped pink shadow that bled through FullBright prims

When an avatar with SSS (subsurface scattering) skin stood behind a FullBright prim, a pink shadow in the **shape of the avatar** leaked through the prim's surface. The equivalent sky-leak variant of this bug was fixed earlier, but the SSS variant had defeated two prior investigation attempts (3-4 hours each) and was deferred. r31-bugfix-1 resolves it at the structural level.

The bug was not introduced in r31. It has existed since SSS was first wired up in the r20+ visual realism chapter — any AYAstorm version with SSS enabled and FullBright prims in scene is affected.

### Background — why it was happening

Four single-RT output paths never write to `gbuffer3.a` (the SSS skin flag channel):

- `POOL_FULLBRIGHT`
- `POOL_FULLBRIGHT_ALPHA_MASK`
- `POOL_BUMP` FB Shiny
- `POOL_ALPHA` FB-internal path

As a result, after these passes run on top of an avatar already in the framebuffer, the **stale `aya_sss_skin_flag = 1.0`** written by the avatar's earlier opaque pass remains in `gbuffer3.a`. The SSS pass (`skinSSSF.glsl`) then reads `gbuffer3.a`, decides "this pixel is skin", and blurs `mRT->screen` — which by now contains the FullBright prim's surface colour — using the avatar-shaped flag mask. The result is a pink shadow shaped exactly like the occluded avatar bleeding through the FullBright prim.

### How it works

The fix moves the SSS dispatch site:

- **Before**: SSS ran on reaching `POOL_ALPHA_POST_WATER` (after FB had already overwritten scene colour)
- **After**: SSS runs on reaching `POOL_FULLBRIGHT` (just before FB writes scene colour)

This way SSS samples the raw post-softenLight skin colour, not FB-overwritten output. FB shaders are unchanged; only `indra/newview/pipeline.cpp::renderGeomPostDeferred` moved one dispatch block and introduced independent `done_sss` / `sss_pass` flags. The change is fully reversible.

Among the four candidate approaches considered (case A: MRT-ize the FB shaders + bind MRT on the C++ pool side — roughly a half-day with three-OS coverage; case B/C: alternative compositing; case D: dispatch reorder), **case D was adopted** as the lowest-risk fix that achieves complete resolution of the SSS path without touching any FB shader or pool MRT bind.

### Approach shift (technical highlight)

This bug had defeated two prior attempts that used ad-hoc `if` branches inside the SSS pass. r31-bugfix-1 was solved by stepping back and building **four effect-axis routing maps in parallel** before writing any code:

- SSS rendering routing
- FullBright rendering routing
- Glow rendering routing
- Environment rendering routing

These complement the existing object-axis routing references (deferred shader / attachment / rez-object / gbuffer3-trace) by tracing the same pipeline along *which visual effect a fragment ends up in*, not *which object class it came from*. Four independent agent investigations all converged on the same root cause — `gbuffer3.a` staleness after single-RT FB passes — which gave the diagnosis high confidence before any pipeline edit was made.

The four routing maps (1894 lines total) ship in this release as a permanent reusable asset for future rendering bug investigation.

### Migration note

- **No user setting change required.** Users on r31 can simply install r31-bugfix-1 on top
- All r31 features (3D Stream unified tag, AYAstorm View, parcel music Vorbis fix, MOAP routing, macOS branding, other-rigged picker) work unchanged
- Users who actually use SSS will see the pink-shadow leak through FullBright prims disappear

### Known limitations / future work

- The structural double meaning of `gbuffer3.a` (SSS skin mask **and** emissive MRT blend factor) is not resolved by this fix. The same class of bug may surface again for a future effect that reads `gbuffer3.a` after a single-RT pass
- A more structural fix (case A: MRT-ize the FB shaders so they write a clean `gbuffer3.a`) remains open as future work. For SSS, the present fix is complete

### Implementation summary

- `indra/newview/pipeline.cpp` (+43 / −8 lines) — SSS dispatch separation
- `docs/specs/ayastorm-sss-rendering-routing.md` (335 lines, new)
- `docs/specs/ayastorm-fullbright-rendering-routing.md` (542 lines, new)
- `docs/specs/ayastorm-glow-rendering-routing.md` (534 lines, new)
- `docs/specs/ayastorm-environment-rendering-routing.md` (483 lines, new)
- PR [#112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

### Credits

[@mayatonton](https://github.com/mayatonton) — implementation, routing-map authoring, bug analysis.

### Documentation

- SSS rendering routing: [`docs/specs/ayastorm-sss-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing (case D discussion: §9.1): [`docs/specs/ayastorm-fullbright-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [`docs/specs/ayastorm-glow-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [`docs/specs/ayastorm-environment-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112 (merged): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)
