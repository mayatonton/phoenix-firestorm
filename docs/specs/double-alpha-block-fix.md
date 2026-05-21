> **Language / 言語 / 语言**: **English** · [日本語](./double-alpha-block-fix.ja.md) · [中文](./double-alpha-block-fix.zh.md)

# Double Alpha Block — A rendering bug shared by all SL viewers, and a two-line fix

**Status**: Fixed in AYAstorm. Open for free adoption by any LL viewer fork — no PR required, pull what you need.

**Reference branch**: [`fix/double-alpha-block`](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block) on `mayatonton/phoenix-firestorm`. HEAD tracks the latest doc revision; the §5 fix itself is unchanged from the original commit. The **verification snapshot** (canary state for §8) is permanently anchored at commit `2597b657ac` — `git checkout 2597b657ac` to reproduce the colored verification frames.

**Deep trace**: see [`docs/specs/ayastorm-rez-object-rendering-routing.md`](./ayastorm-rez-object-rendering-routing.md) §11 for full canary verification and dispatcher map.

---

## 1. TL;DR

When forward alpha BLEND is rendered in the post-water alpha pool, **rigged attachments (hair, clothing) are drawn first and write depth into the shared depth buffer**. Non-rigged alpha BLEND geometry behind them (windows, lace, foliage) is then **depth-rejected before its fragment shader runs**, so the final pixel reverts to the sky written during the opaque pass.

The fix is to swap the two forward passes in `LLDrawPoolAlpha::renderPostDeferred` so non-rigged is drawn first (back) and rigged after (front), strictly inside `POOL_ALPHA_POST_WATER`. PRE_WATER and HUD paths are left untouched.

Two lines of net change. No new uniforms, no new render targets, no shader edits required.

## 2. Affected viewers

This bug is **not AYAstorm-specific**. The offending logic — `write_depth = rigged || ...` combined with rigged-first forward order — lives in Linden Lab's upstream viewer source, so any viewer derived from LL upstream inherits the same code path.

Reproduction is viewer-agnostic. With Depth-of-Field off (or any path where the viewer's internal alpha-RT separation is not active), put a hair attachment in front of a transparent glass / lace / foliage Rez Object and the sky/clouds bleed through the hair silhouette.

## 3. Symptom

When the camera looks at an avatar wearing a hair attachment (or any rigged alpha BLEND mesh) and behind that hair there is a non-rigged alpha BLEND object — a window pane, lace cloth, foliage, particle wisp — the silhouette of the hair shows **the sky written during the opaque pass** instead of the object that should be visible through the hair.

The effect is most obvious indoors looking through a window with hair in the foreground: the window simply disappears behind the hair edges.

| Before fix (normal rendering) | After fix (normal rendering) |
|:---:|:---:|
| ![Before](./images/double-alpha-block/before-normal.png) | ![After](./images/double-alpha-block/after-normal.png) |
| The window grid behind the hair silhouette is missing — sky / tree branches show through where the hair occludes the window. | The window grid is correctly visible through the hair silhouette. |

## 4. Root cause

### 4.1 Depth-write rule

`indra/newview/lldrawpoolalpha.cpp::forwardRender(bool rigged)`:

```cpp
bool write_depth = rigged ||
    LLDrawPoolWater::sSkipScreenCopy ||
    LLPipeline::sImpostorRenderAlphaDepthPass ||
    getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);
```

`write_depth` is unconditionally true for rigged. Rigged alpha BLEND geometry writes to the shared depth buffer.

### 4.2 Forward render order (upstream)

`renderPostDeferred` (consistent across LL upstream-derived viewers):

```cpp
if (!LLPipeline::sRenderingHUDs)
{
    // first pass, render rigged objects only and render to depth buffer
    forwardRender(true);   // ① rigged first  — writes depth
}

// second pass, regular forward alpha rendering
forwardRender();           // ② non-rigged after — depth-rejected by ①
```

### 4.3 The chain

1. ① draws hair (rigged alpha BLEND). `write_depth = true`, so hair z lands in the shared depth buffer.
2. ② tries to draw the window (non-rigged alpha BLEND). Window z > hair z because the window is behind the hair.
3. `GL_LEQUAL` rejects the window fragment **before the fragment shader runs**. No blend, no color write.
4. The pixel retains whatever was written during the opaque pass — typically the skybox.

Result: the window vanishes inside the hair silhouette. **Double alpha block.**

## 5. The fix

`renderPostDeferred` — swap the order **only for `POOL_ALPHA_POST_WATER`**:

```cpp
if (!LLPipeline::sRenderingHUDs &&
    getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
{
    // back-to-front: non-rigged background first, rigged foreground after
    forwardRender();        // ① non-rigged (Rez Object alpha BLEND)
    forwardRender(true);    // ② rigged (attachment alpha BLEND) — over the bg
}
else
{
    // PRE_WATER / HUD: keep upstream order (water fog integrity).
    if (!LLPipeline::sRenderingHUDs)
    {
        forwardRender(true);
    }
    forwardRender();
}
```

## 6. Why this works

| step | what runs | depth state at start | outcome |
|---|---|---|---|
| ① | non-rigged (`write_depth = false` on POST_WATER) | opaque-only depth | window depth-tests against opaque z, fragment runs, color blends in |
| ② | rigged (`write_depth = true`) | opaque depth + window z **not** written | hair depth-tests against opaque z only, fragment runs, blends over the window |

Because step ① on POST_WATER does **not** write depth (only rigged does — and rigged is step ②), the window does not block subsequent hair fragments either. Both surfaces draw correctly in back-to-front order. Standard painter's-algorithm alpha compositing.

## 7. Side effects

None observed.

- **PRE_WATER** keeps the original rigged-first order. `write_depth` is unconditionally true under PRE_WATER (the `POOL_ALPHA_PRE_WATER` term in the OR), and the water fog pass downstream relies on rigged depth being present. Touching this would change water rendering.
- **HUD** stays on a single `forwardRender()` call. No re-ordering applies.
- **Impostors / shadow / cube snapshot** acquire rigged depth through separate code paths (`sImpostorRenderAlphaDepthPass`, shadow-pass dedicated forward calls), not through `renderPostDeferred`. The forward-internal swap does not affect those.
- **DoF / SSAO / SSR** correctness for transparent surfaces is a **separate, deeper problem** caused by `LLGLSPipelineAlpha` keeping depth-write off across the whole alpha pool. The fix above does not address that — it only stops alpha-on-alpha occlusion of fragment shading. AYAstorm has since solved the **DoF** case via the C-plan separate-RT route (see §9). SSAO / SSR / reflection-probe remain open.

## 8. How to verify

Quickest reproduction:

1. Put a hair attachment (rigged alpha BLEND) in front of a transparent / translucent Rez Object — a windowed wall, a lace curtain, a foliage prim.
2. Turn off Depth-of-Field, or use any setting where your viewer's alpha-RT separation (if any) is inactive.
3. Look at the hair silhouette.
   - **Before fix**: sky or far-away geometry shows through the hair — the window/lace is missing.
   - **After fix**: the window/lace shows through the hair, correctly blended.

For a stronger signal, force the alpha BLEND fragments to a known color (e.g., paint Rez-Object alpha BLEND output green) and confirm the green fills the hair silhouette after the fix. See `ayastorm-rez-object-rendering-routing.md` §10–§11 for the canary protocol AYAstorm used (`aya_attachment_canary == 12 → green`).

| Before fix (canary on) | After fix (canary on) |
|:---:|:---:|
| ![Before canary](./images/double-alpha-block/before-canary.png) | ![After canary](./images/double-alpha-block/after-canary.png) |
| Hair (magenta canary) occludes the green Rez-Object alpha BLEND. The black inside the hair silhouette is the opaque-pass background that survives — proving the green window fragments never ran. | Green fills the entire hair silhouette. The Rez-Object alpha BLEND fragments now run before hair, then the hair (magenta) blends on top. |

## 9. What this fix does NOT solve

This patch stops the *visible occlusion* of background alpha BLEND fragments by foreground rigged alpha BLEND. It does not fix the more general problem that alpha BLEND geometry is invisible to subsequent post-process passes:

| post-process | depth seen for alpha BLEND surface | effect | AYAstorm status |
|---|---|---|---|
| Depth-of-Field (CoC) | sees opaque z behind the surface | transparent surfaces ignore focus distance | **✅ Solved (2026-05-22, separate from this §5 fix)** — see §9.1 |
| SSAO | no neighbor depth | AO falls off at transparent edges | ⏳ Open (out of scope for this branch) |
| SSR | no depth | reflections drop on / through transparent surfaces | ⏳ Open (out of scope for this branch) |
| Reflection probe blend | surface ignored | probe blending shifts | ⏳ Open (out of scope for this branch) |

The structural fix for any of these is to render the alpha BLEND pool into a separate color (and ideally depth) attachment and composite it back in the post-process stage. AYAstorm tracks this work as the **C plan** (`mAYAAlphaColor` separate color RT + `mAYAAlphaDepth` cutoff-0.5 alpha-aware depth + over-blend in `dofCombineF`). The §5 fix in this document is **orthogonal** to the C plan — adopters interested only in the occlusion fix can take §5 alone without touching the alpha RT separation.

### 9.1 C plan — DoF first-class wiring (AYAstorm only, 2026-05-22)

The C plan is **wired end-to-end for DoF** on AYAstorm's `experiment/ayastorm-layered-dof` branch. Alpha BLEND surfaces (hair, clothing, window glass, foliage tips, lace, particles) now respond to `CameraFNumber`, `CameraFocalLength`, and `CameraMaxCoF` exactly like opaque geometry.

Pipeline overview (`indra/newview/app_settings/shaders/class1/deferred/`):

| pass | input | output | role |
|---|---|---|---|
| `cofF.glsl` | `mAYAAlphaDepth` (alpha-aware) | `mRT->deferredLight` (.rgb = src, .a = CoC) | CoC computed from alpha plate depth (alpha ≥ 0.5) or bg depth (alpha < 0.5) |
| `postDeferredHQDoFF.glsl` | `mRT->deferredLight` + scene depth | DoF-blurred scene | opaque scene blurred by CoC |
| `dofCombineF.glsl` | DoF result + sharp lightMap + **`mAYAAlphaColor`** | final | DoF-blurred opaque + alpha plate **CoC-driven 12-tap disc gather** composite |

Key edit (alpha plate over-blend in `dofCombineF.glsl`):

```glsl
if (aya_alpha_plate_enabled)
{
    float coc_px = abs(diff.a * 2.0 - 1.0) * max_cof * 4.0;  // same magnitude as HQDoFF
    vec4 plate;
    if (coc_px < 0.75) {
        plate = texture(aya_alpha_plate, vary_fragcoord.xy);  // in-focus → single tap
    } else {
        const int N = 12;
        vec4 acc = vec4(0.0);
        for (int i = 0; i < N; ++i) {
            float ang = float(i) * 6.2831853 / float(N);
            vec2 off = vec2(cos(ang), sin(ang)) * coc_px / screen_res;
            acc += texture(aya_alpha_plate, vary_fragcoord.xy + off);
        }
        plate = acc / float(N);
    }
    frag_color.rgb = plate.rgb + frag_color.rgb * (1.0 - plate.a);   // standard "over"
}
```

The premultiplied color/coverage in `mAYAAlphaColor` averages correctly under uniform-weight box gather, so the standard "over" composite remains valid after blur.

**Accuracy:**
- alpha ≥ 0.5 pixels (glass, leaves, opaque-leaning clothing): CoC is exact — depth carries the alpha plate's own z via `mAYAAlphaDepth`.
- alpha < 0.5 pixels (hair tips, lace edges): CoC falls back to background depth, so wispy edges blur with background blur amount. Visually indistinguishable in practice.

**Adoption note for C plan:**

The C plan is a multi-commit feature involving render-target allocation, alpha pool redirection, depth re-injection, cofF bind switching, and the dofCombineF over-blend gather above. It is **not** part of the `fix/double-alpha-block` reference branch. Adopters wanting DoF correctness for transparent surfaces should consult `ayastorm-rez-object-rendering-routing.md` §10.10 and pick up the alpha-RT + dofCombineF commits from `experiment/ayastorm-layered-dof` independently.

SSAO / SSR / reflection-probe correctness for transparent surfaces is a **separate engineering chapter** (different design tradeoffs — whether AO/SSR is applied to the plate, or to the background visible through the plate). Industry-wide it is still largely unresolved. AYAstorm has no scheduled work on these.

## 10. Adoption

The minimal change to import is the §5 swap. It is self-contained inside `LLDrawPoolAlpha::renderPostDeferred` in `indra/newview/lldrawpoolalpha.cpp` and does not depend on any other AYAstorm change.

Pull the reference branch to read the surrounding context. Its HEAD tracks the latest doc revision (so the in-tree copy of this file stays in sync with the published version), while the code change itself is stable. The **colored canary verification state** for §8 is permanently anchored at commit `2597b657ac` — checkout that specific commit if you want to reproduce the verification frames against your own scene. The subsequent commit `c454ce0b0f` (on the main experiment branch, not this reference branch) is what reverted the canary back to normal texturing in the running viewer.

```sh
git remote add ayastorm https://github.com/mayatonton/phoenix-firestorm.git
git fetch ayastorm fix/double-alpha-block
git log -1 ayastorm/fix/double-alpha-block
git show ayastorm/fix/double-alpha-block -- indra/newview/lldrawpoolalpha.cpp

# reproduce the canary verification state:
git checkout 2597b657ac
```

No PR is planned upstream. Take it on your own terms.

---

## License

The reference branch and this document are released under the same license as Phoenix-Firestorm / Linden Lab viewer (LGPL v2.1).
