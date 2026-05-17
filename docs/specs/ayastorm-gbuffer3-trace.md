# AYAstorm GBuffer3 (frag_data[3] / emissiveRect / DEFERRED_EMISSIVE) リファレンス

**作成日**: 2026-05-13
**作成経緯**: r20 Phase C Skin SSS の mask 不具合解析。「writers が `.a=0/aya_sss_skin_flag` を書いてるのに sample すると `.a=1`」という症状に対し、推論ベースの shader 書き換え→cp→cache clear→再起動 を 4 回ループして全部外し、AYA から「完全トレースに切り替えろ」指示。trace の結果 **GPU storage 仕様** が原因と確定したため、`reference_deferred_shader_routing.md` と並ぶ永続資料化。

---

## TL;DR — gbuffer3 で `.a` を使いたいなら format を変えろ

**gbuffer3 (DEFERRED_EMISSIVE / "emissiveRect" / frag_data[3]) は LL 標準では `GL_RGB16F` (HDR) または `GL_RGB` (LDR) で allocate されている — alpha channel が物理的に存在しない。**

その状態で `.a` を使うと:
- `frag_data[3].a = X` の write は **GPU が silently drop** する (書き込み先 storage 無し)
- `texture(emissiveRect, tc).a` の read は **GLSL spec の RGB→RGBA 既定で常に 1.0** を返す

`.a` を skin flag / SSS mask / その他 per-pixel marker に使いたい場合は `pipeline.cpp:addDeferredAttachments()` で format を **`GL_RGBA16F` / `GL_RGBA`** に変える必要がある。AYAstorm r20 Phase C で実施済。

---

## A. gbuffer3 (frag_data[3]) write 一覧

全 fragment shader を `grep "frag_data\[3\]"` で列挙。各 writer の `.a` write 値と HAS_EMISSIVE 依存性。

| Shader | .a の write 値 | HAS_EMISSIVE gate? | Forward / Deferred | 用途 |
|---|---|---|---|---|
| `class3/deferred/materialF.glsl` | `aya_sss_skin_flag` (r20 Phase C) | YES | Deferred | PASS_MATERIAL*/SPECMAP*/NORMMAP*/NORMSPEC* (legacy material) |
| `class1/deferred/pbropaqueF.glsl` | `aya_sss_skin_flag` (r20 Phase C) | YES | Deferred | POOL_GLTF_PBR / POOL_GLTF_PBR_ALPHA_MASK (rigged + non-rigged) |
| `class1/deferred/avatarF.glsl` | `aya_sss_skin_flag` (r20 Phase C) | YES | Deferred | Linden 標準 avatar body (gDeferredAvatarProgram) |
| `class1/deferred/pbrterrainF.glsl` | `max(emissive,0).a` (emissive.a 通常 0) | YES | Deferred | PBR terrain |
| `class1/gltf/pbrmetallicroughnessF.glsl` | `max(emissive,0).a` | (要確認) | (Forward) | GLTF scene manager 経路 |
| `class1/deferred/bumpF.glsl` | `0.0` | YES | Deferred | PASS_BUMP |
| `class1/deferred/diffuseF.glsl` | `0.0` | YES | Deferred | PASS_SIMPLE |
| `class1/deferred/diffuseIndexedF.glsl` | `0.0` | YES | Deferred | indexed simple |
| `class1/deferred/diffuseAlphaMaskF.glsl` | `0.0` | YES | Deferred | alpha mask |
| `class1/deferred/diffuseAlphaMaskNoColorF.glsl` | `0.0` | YES | Deferred | rigid avatar |
| `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | `0.0` | YES | Deferred | indexed alpha mask |
| `class1/deferred/treeF.glsl` | `0.0` | YES | Deferred | Linden tree |
| `class1/deferred/terrainF.glsl` | `0.0` | YES | Deferred | legacy terrain |
| `class1/deferred/impostorF.glsl` | `0.0` | YES | Deferred | avatar impostor |
| `class1/deferred/highlightF.glsl` | `0.0` | YES | Highlight overlay | object highlight |
| `class1/deferred/skyF.glsl` | `0.0` (r20 Phase C) | YES | Deferred | WL sky preset / HDRI sky |
| `class1/deferred/sunDiscF.glsl` | `0.0` (r20 Phase C) | YES | Deferred | sun disc |
| `class1/deferred/moonF.glsl` | `0.0` (r20 Phase C) | YES | Deferred | moon |
| `class1/deferred/cloudsF.glsl` | `0.0` (r20 Phase C) | YES | Deferred | clouds |
| `class1/deferred/starsF.glsl` | `0.0` (r20 Phase C) | YES | Deferred | stars |
| `class1/interface/occlusionF.glsl` | `0.0` | YES | Occlusion query | 実画面に出ない |

**観測**: AYAstorm r20 で sky/celestial 系を `.a=0` に統一済。skin writer (materialF/pbropaqueF/avatarF) のみ `aya_sss_skin_flag` を書く。

---

## B. HAS_EMISSIVE permutation

`llviewershadermgr.cpp:272`:
```cpp
static void add_common_permutations(LLGLSLShader* shader)
{
    static LLCachedControl<bool> emissive(gSavedSettings, "RenderEnableEmissiveBuffer", false);
    if (emissive) shader->addPermutation("HAS_EMISSIVE", "1");
}
```

- `RenderEnableEmissiveBuffer` の default は `false` だが `settings.xml:11524` で実値は **true**
- 全 deferred shader program で `add_common_permutations` が呼ばれており permutation は確実に立つ
- → HAS_EMISSIVE が原因で `frag_data[3]` write が消える、というケースは実プレイ条件では発生しない

C++ 側で同じ key を attribs に立てる 2 経路 (line 274 / line 824) があるので、`RenderEnableEmissiveBuffer` を **false に切り替えた場合は shader cache clear+再起動が必要** (cached permutation が古いまま残ると mismatch する)。

---

## C. gbuffer3 の allocate (`pipeline.cpp:addDeferredAttachments`) ← ROOT CAUSE

`pipeline.cpp:394`:

```cpp
bool addDeferredAttachments(LLRenderTarget& target, bool for_impostor = false)
{
    U32 orm      = GL_RGBA;     // frag_data[1] specular / PBR ORM
    U32 norm     = GL_RGBA16;   // frag_data[2] normal + flag
    U32 emissive = GL_RGB16F;   // frag_data[3] emissive  ← AYAstorm 前は alpha 無し
    ...
    if (!hdr) {
        norm     = GL_RGB10_A2;
        emissive = GL_RGB;      // ← LDR でも alpha 無し
    }
    ...
    if (has_emissive) {
        valid = valid && target.addColorAttachment(emissive); // gbuffer3
    }
}
```

### LL 標準 (alpha 無し) の挙動

- `frag_data[3].a = X` の write → **storage 無しで silently drop**
- `texture(emissiveRect, tc).a` の read → **GLSL spec の RGB→RGBA 既定で 1.0**

これで観測事実が全部説明される:
| 観測 | 説明 |
|---|---|
| writer 側を `.a=0` ハードコードしても全画面 `.a=1` | GPU が alpha 書き込みを drop |
| canary RGB で sky だけ色付き、それ以外黒 | `.rgb` は GL_RGB16F に正しく格納される |
| 透過物 (forward) 領域に canary が乗らない | SSS pass は forward 描画**前**に走る (これは別事実) |

### AYAstorm r20 Phase C の fix

```cpp
U32 emissive = GL_RGBA16F;  // alpha 付き HDR
...
if (!hdr) {
    emissive = GL_RGBA;     // alpha 付き LDR
}
```

副作用チェック (`grep "gb\.emissive"` / `"emissive\.rgb"`):
- 既存 reader (`softenLightF` / `pointLightF` / `multiPointLightF` / `pbrmetallicroughnessF`) は全て `.rgb` のみ参照
- `.a` reader は r20 で追加した `skinSSSF` のみ
- メモリ・帯域コスト: HDR で RGB16F (48bit) → RGBA16F (64bit)、deferred target あたり数 MB 増

---

## D. emissiveRect texture binding

- skinSSSProgram は `add_common_permutations` 経由で `emissiveRect` uniform を持つ
- `LLPipeline::doSkinSSS` で `mRT->deferredScreen.bindTexture(DEFERRED_EMISSIVE, channel)` 済
- canary RGB が正しく gbuffer3 を読めているので **binding 系の不具合は無い**

---

## E. write/sample 間の corruption

C で root cause が確定したため、copy/blit/clear 系の調査は不要。書き込み自体が GPU 段階で drop されているのが原因なので、後段は無関係。

---

## 落とし穴 / 再発防止メモ

1. **`.a` を flag に使う前に format を確認する**
   - gbuffer3 だけでなく gbuffer0 (diffuse) / gbuffer1 (spec) / gbuffer2 (norm) も同じ罠の可能性 — 用途に応じて要確認
   - gbuffer0=GL_RGBA / gbuffer1=GL_RGBA / gbuffer2=GL_RGBA16 (or RGB10_A2) は alpha 有り
   - gbuffer3 のみ historical reason で RGB

2. **書き込みが効かない時、shader だけ見ない**
   - C++ 側の format / clear color / FBO attachment 設定を必ず確認
   - 「shader が書いてるのに sample が違う」場合は writer ではなく **storage** を疑う

3. **canary は RGB と `.a` を別個に検証する**
   - RGB 可視化と `.a` 可視化を同時に出すと「RGB OK / `.a` 異常」の切り分けが即座にできる

4. **HAS_EMISSIVE permutation を切り替えた時は shader cache clear**
   - cached binary が古い permutation で残ると C++ 側設定との mismatch で謎挙動

---

## 関連ファイル

- `indra/newview/pipeline.cpp:394` — `addDeferredAttachments` (gbuffer3 format 決定点)
- `indra/newview/llviewershadermgr.cpp:272` — `add_common_permutations` (HAS_EMISSIVE 注入)
- `indra/newview/app_settings/shaders/class1/deferred/gbufferUtil.glsl` — sampling helper
- `indra/newview/app_settings/shaders/class3/deferred/softenLightF.glsl` — 主要 reader (`.rgb` のみ)
- `docs/ayastorm-deferred-shader-routing.md` — gbuffer 全体の routing 表 (姉妹資料)
