# AYAstorm Attachment Rendering Routing リファレンス

**作成日**: 2026-05-21 (r30 P5 BD parity 検証中、装着物だけを per-draw で識別する必要が出て全 path を canary 検証して確定)
**対象**: avatar 装着物 (`vobj->getAvatar() != NULL`) が deferred / HUD で踏む **全描画 path** の確定マップ。「装着物の描画はどこか」を即座に引けるようにするためのリファレンス。
**作成経緯**: r30 P5 で `LLDrawInfo::mAttachedToAvatar` を per-draw 識別子として導入後、各 pool / shader で「magenta canary」を仕込み、装着物全種 (default prim / mesh / GLTF / rigged / non-rigged / 髪 / 服 / 靴 / アクセサリ) が magenta になることを実機で確認したうえで全 path を表化した。

> **再利用方針**: 装着物単位で per-draw 処理を入れたい (per-attachment uniform / per-attachment buffer write / per-attachment culling 等) ときは **まず本書を参照**。pool 追加や drawRange 経路の組み換えが入ったら本書の更新要否を確認すること。

---

## 0. クイック参照: 現行 canary 色定義 (2026-05-21 時点、5 値)

実機で何色が出ているかから「これは何」を即座に逆引きするための表。canary uniform 名は `aya_attachment_canary` (int)、値は `LLDrawInfo` の `mAttachedToAvatar` + `mIsBoMBodyOrHead` + `mIsPrim` と **どの pool dispatcher 経由か** (alpha pool だけ blue → brown) から導出。

| 実機で見える色 | 意味 | canary uniform 値 | C++ 条件 |
|---|---|---|---|
| 🟢 **green** | **HUD 装着物** (`isHUDAttachment()` 相当、装着している本人のみ可視) | 任意 (`!= 0`) | `mAttachedToAvatar.notNull()` かつ IS_HUD shader (`#ifdef IS_HUD` で常に green を強制) |
| 🟣 **magenta** | **MeshBody / MeshHead (= ボディ)** (BoM baked texture 参照 attachment) | `1` | `mAttachedToAvatar.notNull() && mIsBoMBodyOrHead == true` |
| 🟫 **brown (茶)** | **透過合成 (alpha BLEND) が必要な現代装飾物** = 半透明な服 / シフォン / レース / ストッキング / 透ける髪先端 / 透ける耳・羽 等。**← 撮影描画系バグ (DoF / SSAO / 反射 / Z-order) の温床。§11 参照** | `4` | alpha pool 経由 (`lldrawpoolalpha.cpp` dispatcher) かつ `mAttachedToAvatar.notNull() && mIsBoMBodyOrHead == false && mIsPrim == false` |
| 🔵 **blue** | **不透明な現代装飾物 (Mesh)** = 不透明 or alpha MASK (discard cutout) の現代装飾物。現代の装着物の主流 | `2` | alpha pool **以外** の dispatcher (Simple / Bump / Material / PBR opaque) 経由かつ `mAttachedToAvatar.notNull() && mIsBoMBodyOrHead == false && mIsPrim == false` |
| ⚪ **gray** | **レガシー装飾物 (プリム / sculpt)** = 現代では稀 (~1%、ノベルティ / 古い販売物 / 自作 prim 装飾) | `3` | `mAttachedToAvatar.notNull() && mIsPrim == true` (= `!vobj->isMesh()`)。alpha pool 経由でも灰のまま |
| (元色) | 非装着物 (world geometry / 自分以外の avatar 本体 / 土地 / 空 / 植生 / particles 等) | `0` | `mAttachedToAvatar.isNull()` |

**重要な約束 (AYA さんの呼称定義)**:
- 「**ボディ**」 = magenta のもの = MeshBody / MeshHead (BoM 装着) のこと
- 「**装飾物 (HUD 除く)**」 = **青 + 茶 + 灰** 3 色を合わせたもの。アバターが装着する装飾物全部
  - うち **青 (不透明 Mesh)** が現代の主流 path — 服 / 髪 / 靴 / アクセサリの mesh 製品の不透明 or MASK 面
  - うち **茶 (透過 Mesh)** = 同じ現代 Mesh 装飾物のうち、半透明 (alpha BLEND) で背景と合成しないと描けない面。**透過 path = 描画 cost が重く、Z-order の影響を受け、backface の見え方が変わる** 領域
  - うち **灰 (プリム / sculpt)** はレガシー (1%) — 現代では稀
- 「**HUD**」 = green で別カテゴリ (本人にしか見えない特殊 pass)

**青 / 茶 分離の根拠 (2026-05-21)**: 「いま青色になっている装着物のうち、透過して背景を描画しなければならない装着物を青紫色にしてください」 (AYA さん)。色選定の経緯:
1. 青紫 `vec3(0.5, 0.0, 1.0)` → 画面上で magenta (= BoM body/head) と紛らわしくて NG
2. 茶 `vec3(0.5, 0.25, 0.0)` (linear で直書き) → 画面上では **黄〜橙** に出てしまった。原因は shader 出力が linear 空間で、その後の tonemap (≈ linear→sRGB) で intermediate value が明側にシフトするのを見逃していたため
3. 茶 (sRGB 0.5, 0.25, 0.0) を出すために **linear で `vec3(0.214, 0.051, 0.0)`** (= `srgb_to_linear` 換算) を書いて成功。これが現行値

**教訓**: shader で「特定の sRGB 色を画面に出す」には linear→sRGB tonemap を逆引きする必要がある。canary に限らず、UI overlay / debug 描画 / 透過 effect すべてで同じ色空間補正を意識する。

透過 (alpha BLEND) は SL の描画で:
- **別 pool (`LLDrawPoolAlpha`)** に流れる
- 不透明 pass の後に **forward (= 非 deferred)** で背景と合成する
- **depth write しない** ことが多く、Z-order 問題で他装飾物を貫いて見える
- DoF / 反射 / 影で扱いが特殊

ため、撮影描画 (r30) の見え方が壊れたとき「透過装飾物だけで起きているのか / 不透明含めて起きているのか」を実機で即判別できることが debug 価値を持つ。青 / 茶 の分離はこれを実現する。

uniform set 箇所:
- `lldrawpool.cpp` (4 hook) / `lldrawpoolbump.cpp` / `lldrawpoolmaterials.cpp` → 値 0/1/2/3 のいずれか (alpha pool 経由しない)
- `lldrawpoolalpha.cpp` (alpha pool 専用) → blue (2) を **brown (4)** に振り替えて渡す。magenta (1) / gray (3) は維持
- shader 側は §7.2 の 5 値分岐パターンを参照

---

## 1. 「装着物」の定義 (コード上)

### 1.1 識別子: `LLDrawInfo::mAttachedToAvatar`

`indra/newview/llspatialpartition.h:130`:

```cpp
LLPointer<LLVOAvatar> mAttachedToAvatar = nullptr;
```

- `notNull()` = この draw batch は **装着物 (attachment)**
- `isNull()` = 装着物でない (world geometry / avatar 本体 / HUD 以外の独立 prim / 土地 / 植生 / 空 等)

### 1.2 set 場所

`indra/newview/llvovolume.cpp:5838` で 1 か所のみ:

```cpp
if (LLViewerObject* vobj = facep->getViewerObject())
{
    draw_info->mIsSSSTarget       = vobj->isSSSTarget();
    draw_info->mFSPickerLocalID   = vobj->getLocalID();
    draw_info->mAttachedToAvatar  = vobj->getAvatar();  // ← ここ
}
```

呼び出し元は `LLVOVolume::genDrawInfo()` → 全 LLDrawInfo の生成路を通る (= grass / particles / GLTF preview を除く、ほぼ全 visible draw が経由)。

### 1.3 `getAvatar()` の挙動

`indra/newview/llviewerobject.cpp:7694`:

```cpp
LLVOAvatar* LLViewerObject::getAvatar() const
{
    LLControlAvatar* ca = getControlAvatar();
    if (ca) return ca;                              // animesh → control avatar
    if (isAttachment())
    {
        LLViewerObject* vobj = (LLViewerObject*) getParent();
        while (vobj && !vobj->asAvatar())           // 親 chain を上って avatar に到達
            vobj = (LLViewerObject*) vobj->getParent();
        return (LLVOAvatar*) vobj;
    }
    return NULL;                                    // それ以外 (world geometry / avatar 本体 / HUD 等)
}
```

**結論**: `mAttachedToAvatar.notNull()` ⇔
- (a) 装着物の rigged mesh / non-rigged mesh / sculpt / default prim、または
- (b) animesh (control avatar 配下)

`isSelf()` で自分の装着物 / 他人の装着物が分岐可能 (`mAttachedToAvatar->isSelf()`)。

### 1.4 MeshBody / MeshHead の識別 (装着物の中での「ボディ」判定)

「装着物のうち BoM 対応の MeshBody / MeshHead だけ」を per-draw で識別したいとき (例: §0 の magenta vs blue 出し分け)、SL protocol / LLDrawInfo に直接の「これは body/head」flag は **無い**。実用的判別は **TE (TextureEntry) が BoM baked texture UUID を参照しているか** を見る。

`indra/newview/llspatialpartition.h:130` 近辺:

```cpp
LLPointer<LLVOAvatar> mAttachedToAvatar = nullptr;

// <AYAstorm r30 P5 canary> True when this draw batch is a BoM MeshBody /
// MeshHead face (TE id == IMG_USE_BAKED_HEAD / UPPER / LOWER / LEFTARM /
// LEFTLEG). Per-face flag; LLDrawInfo は元々 face 単位なので per-draw 判定で OK。
bool mIsBoMBodyOrHead = false;
```

set 場所: `indra/newview/llvovolume.cpp:5829` 周辺の `mAttachedToAvatar` 設置直後。判定は **object 全 face TE を走査して 1 face でも BoM baked tex を参照していれば true**:

```cpp
if (draw_info->mAttachedToAvatar.notNull())
{
    const S32 num_tes = vobj->getNumTEs();
    for (S32 i = 0; i < num_tes; ++i)
    {
        const LLTextureEntry* te = vobj->getTE(i);
        if (!te) continue;
        const LLUUID& tid = te->getID();
        if (tid == IMG_USE_BAKED_HEAD    ||
            tid == IMG_USE_BAKED_UPPER   ||
            tid == IMG_USE_BAKED_LOWER   ||
            tid == IMG_USE_BAKED_LEFTARM ||
            tid == IMG_USE_BAKED_LEFTLEG)
        {
            draw_info->mIsBoMBodyOrHead = true;
            break;
        }
    }
}
```

BoM UUID は `indra/llcommon/indra_constants.h:224-234` で `IMG_USE_BAKED_*` として extern 宣言され、PCH (`llviewerprecompiledheaders.h:54`) 経由でどの TU からも見える。

#### 部位 ↔ BoM UUID 対応

| BoM texture UUID | 装着先 (典型) | `mIsBoMBodyOrHead` 判定 |
|---|---|---|
| `IMG_USE_BAKED_HEAD` | MeshHead | **true** |
| `IMG_USE_BAKED_UPPER` | MeshBody (chest / torso) | **true** |
| `IMG_USE_BAKED_LOWER` | MeshBody (pelvis / legs) | **true** |
| `IMG_USE_BAKED_LEFTARM` | MeshBody (BoM 2.0 arm 分割) | **true** |
| `IMG_USE_BAKED_LEFTLEG` | MeshBody (BoM 2.0 leg 分割) | **true** |
| `IMG_USE_BAKED_EYES` | MeshEyes | false (eyes は body/head とは別系として除外) |
| `IMG_USE_BAKED_HAIR` | Hair (服扱い) | false |
| `IMG_USE_BAKED_SKIRT` | Skirt mesh (服扱い) | false |
| `IMG_USE_BAKED_AUX1/2/3` | Universal layer | false |

#### 判別の限界 (resolveable 残課題)

1. **BoM を使っていない MeshBody / MeshHead** (system layer の代わりに固有 texture を貼っている古い使い方) は判別できない → blue 側に流れる。BoM 使用が主流なので実害は小さい。
2. **BoM 服** (Tattoo / Underwear layer を受ける Mesh 服) も `UPPER`/`LOWER` を参照する場合がある → object 内 1 face でも一致すれば true なので「BoM 服を着ている服」は誤って magenta 側に流れる可能性あり。ただし通常の BoM 服は body 側に baked tex を流すだけで自分の TE は別 UUID を持つので、実機では稀。
3. `HEAD` を参照する服は基本ありえないので、MeshHead は精度高く識別できる。

#### 参考: LL 本体の同等処理

`indra/newview/llvoavatar.cpp:9941 updateMeshVisibility()` が同じパターンで全装着物 TE を走査し、BoM baked tex を参照していれば対応する classic avatar joint mesh を hide する。この処理を per-draw 識別に転用したのが §0 の canary 仕組み。

### 1.5 装飾物のうち「プリム / sculpt」の識別 (= レガシー判定)

「装飾物のうち現代的な Mesh ではなく、レガシーな prim / sculpt で構成されているもの」を per-draw で識別したいときの判定。**現代の SL 装着物の 99% は Mesh** なので、この flag が true になるのは **「ノベルティ」「古い販売物」「自作 prim 装飾」等のごく少数**。それでも分離する意味があるのは、撮影描画 (r30) の hit/miss が「主流 path で発生しているか / レガシー path だけで発生しているか」を実機で即判別するため (青 = 主流 / 灰 = レガシー)。

`indra/newview/llspatialpartition.h:130` 近辺:

```cpp
LLPointer<LLVOAvatar> mAttachedToAvatar = nullptr;
bool mIsBoMBodyOrHead = false;

// <AYAstorm r30 P5 canary> True when this draw batch is a prim attachment
// (= 装着物のうち mesh でないもの: box / sphere / cylinder / torus / ring /
// tube / prism + sculpt 全部含む)。判定式は
// `mAttachedToAvatar.notNull() && !vobj->isMesh()`。
//
// SL の慣習: sculpt は「特殊な prim」として扱う (mesh ではない)。
// 現代の装着物 99% は Mesh なのでこの flag が true になるのは ~1%。
bool mIsPrim = false;
```

set 場所: `indra/newview/llvovolume.cpp:5829` 周辺の `mAttachedToAvatar` 設置直後 (`mIsBoMBodyOrHead` 判定の後ろ):

```cpp
if (draw_info->mAttachedToAvatar.notNull())
{
    // ... (mIsBoMBodyOrHead 判定 §1.4) ...

    // mesh でない装着物 = legacy prim / sculpt
    draw_info->mIsPrim = !vobj->isMesh();
}
```

#### `LLViewerObject::isMesh()` の挙動

`indra/newview/llviewerobject.cpp` 内、`getVolume() && getVolume()->getParams().getSculptType() == LL_SCULPT_TYPE_MESH` 相当を返す。すなわち:

| object 種別 | `isMesh()` | `mIsPrim` (= `!isMesh()`) |
|---|---|---|
| Default prim (box / sphere / cylinder / torus / ring / tube / prism) | false | **true (灰)** |
| Sculpt prim (古い sculpt 装着) | false | **true (灰)** |
| Mesh (rigged / non-rigged の現代 Mesh assets) | true | false (青) |

**SL 慣習との一致**: sculpt は「特殊な prim」として扱われる慣習に合わせ、`!isMesh()` は box系 prim と sculpt をまとめて灰色にする (= AYA さんの定義「Sculptプリムは私たちはプリムのひとつだと認識している」と一致)。

---

## 2. 描画 path 完全分類: Pool × Shader × Dispatcher

装着物 (`mAttachedToAvatar.notNull()`) が踏みうる **可視 base color** path の完全表。

| 装着物の種類 | Pool | drawRange 呼び出し元 | Bound shader (fragment) | 備考 |
|---|---|---|---|---|
| **GLTF PBR opaque** (新型 mesh / 服 / アクセサリ) | `LLDrawPoolPBROpaque` | `LLRenderPass::pushGLTFBatch` `lldrawpool.cpp:1138` (textured), `pushUntexturedGLTFBatch` `lldrawpool.cpp:1165` (untextured) | `class1/deferred/pbropaqueF.glsl` | rigged は `pushRiggedGLTFBatch` / `pushUntexturedRiggedGLTFBatch` 経由で結局同 dispatcher |
| **GLTF PBR alpha BLEND** | `LLDrawPoolAlpha` (PBR 分岐) | `LLDrawPoolAlpha::renderAlpha` inline `lldrawpoolalpha.cpp:859` | `class2/deferred/pbralphaF.glsl` (deferred), `class1/deferred/pbralphaF.glsl` (HUD) | GLTF material が alpha-blend mode のとき alpha pool に流れる |
| **Legacy material** (normal / spec map 持ち、PBR 以前の SL material) | `LLDrawPoolMaterials` | `LLDrawPoolMaterials::renderDeferred` inline `lldrawpoolmaterials.cpp:307` | `class3/deferred/materialF.glsl` (AYAstorm Cinematic は class3 上書き) | normalmap or specmap いずれか有り |
| **Legacy alpha BLEND mesh** (透明部分のある髪 / 服) | `LLDrawPoolAlpha` (legacy 分岐) | `lldrawpoolalpha.cpp:859` | `class2/deferred/alphaF.glsl` | 同上 drawRange、shader が分岐 |
| **Simple textured** (default prim / 単色 texture / 普通の mesh) | `LLDrawPoolSimple` | `LLRenderPass::pushBatch` `lldrawpool.cpp:665`, `pushUntexturedBatch` `lldrawpool.cpp:698` | `class1/deferred/diffuseF.glsl`, `diffuseIndexedF.glsl`, `diffuseAlphaMaskF.glsl`, `diffuseAlphaMaskIndexedF.glsl`, `diffuseAlphaMaskNoColorF.glsl` | 一番素朴な path、alpha mode が NONE/MASK で material も PBR も無し |
| **Bump-mapped** (legacy shiny / 凹凸付き leg / 靴 等) | `LLDrawPoolBump` | `LLRenderPass::pushBatch` `lldrawpool.cpp:665` (alpha mask 系), `LLRenderPass::pushBumpBatch` `lldrawpoolbump.cpp:1068` (本体 bump batch) | `class1/deferred/bumpF.glsl` | pushBumpBatch は **pushBatch を経由しない独立 dispatcher**、両方カバー必要 |
| **Fullbright** (HUD / 光沢 mesh) | `LLDrawPoolBump` fullbright path | `pushBatch` `lldrawpool.cpp:665` | `class1/deferred/fullbrightF.glsl` | IS_HUD 分岐は同 file 内 |

---

## 3. drawRange 呼び出し点 全 17 箇所の分類

`grep mVertexBuffer->drawRange indra/newview/lldrawpool*.cpp` の結果を分類:

### 3.1 可視 base color → canary 必須

| File:Line | 呼び出し関数 | 用途 |
|---|---|---|
| `lldrawpool.cpp:665` | `LLRenderPass::pushBatch` | Simple / Bump (alpha-mask) / Fullbright 共通 dispatcher |
| `lldrawpool.cpp:698` | `LLRenderPass::pushUntexturedBatch` | 上記の untextured 版 |
| `lldrawpool.cpp:1138` | `LLRenderPass::pushGLTFBatch` | GLTF PBR (rigged/non-rigged 共通、textured) |
| `lldrawpool.cpp:1165` | `LLRenderPass::pushUntexturedGLTFBatch` | GLTF PBR (untextured) |
| `lldrawpoolbump.cpp:1068` | `LLRenderPass::pushBumpBatch` | Bump pool 本体 (pushBatch を経由しない) |
| `lldrawpoolmaterials.cpp:307` | `LLDrawPoolMaterials::renderDeferred` inline | Legacy material |
| `lldrawpoolalpha.cpp:859` | `LLDrawPoolAlpha::renderAlpha` inline | Alpha pool (legacy + GLTF blend 共通) |

### 3.2 非可視 / bloom only → canary 不要

| File:Line | 関数 | 理由 |
|---|---|---|
| `lldrawpool.cpp:873/920/962/1014` | motion blur 各 push | Velocity buffer 書込み、base color 描画でない |
| `lldrawpoolalpha.cpp:534/561/608` | `drawEmissive` / `renderPbrEmissives` / `renderRiggedPbrEmissives` | Glow / bloom 専用 framebuffer、base color は別 pass |
| `lldrawpoolalpha.cpp:411` | depth-only pre-pass | depth のみ書込み |
| `lldrawpoolalpha.cpp:440` | `inline void Draw` | **使用箇所なし** (dead code) |
| `lldrawpoolbump.cpp:419` | `LLDrawPoolBump::renderGroup` | shiny cube map 専用、現状 base color 別 path で出る |

---

## 4. 装着物識別の per-draw 注入レシピ

新規に「装着物だけ別処理」を入れる場合の手順:

### 4.1 C++ 側: uniform set

各 dispatcher (§3.1 の 7 箇所) の drawRange 直前で:

```cpp
static LLStaticHashedString s_aya_my_flag("aya_my_flag");
LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;  // または mShader (pool member)
if (cur)
{
    cur->uniform1i(s_aya_my_flag,
                   params.mAttachedToAvatar.notNull() ? 1 : 0);
}
```

**注意**: pool ごとに current shader の取得方法が違う:
- `LLRenderPass::push*Batch` 系 → `LLGLSLShader::sCurBoundShaderPtr`
- `LLDrawPoolMaterials` → member の `mShader`
- `LLDrawPoolAlpha::renderAlpha` → file 先頭の `current_shader` static

### 4.2 GLSL 側: uniform 宣言と分岐

各 shader (§2 表の右列) で:

```glsl
uniform int aya_my_flag;

void main()
{
    // ...
    if (aya_my_flag != 0)
    {
        // 装着物時の処理
    }
    // ...
}
```

**重要**: `pbropaqueF.glsl` / `pbralphaF.glsl` は `#ifndef IS_HUD` で deferred / HUD の 2 main を持つ。uniform 宣言は `#ifndef IS_HUD` の **外側** に置くこと (両 main から見える位置)。内側に置くと HUD shader リンク失敗 (`undefined variable` で ASSERT crash する)。

---

## 5. self / other / animesh の細分化

`mAttachedToAvatar.notNull()` の中をさらに分けたい場合:

```cpp
if (params.mAttachedToAvatar.notNull())
{
    LLVOAvatar* av = params.mAttachedToAvatar;
    bool is_self    = av->isSelf();
    bool is_animesh = av->isControlAvatar();
    bool is_other   = !is_self && !is_animesh;
    // ...
}
```

既存利用例: `lldrawpool.cpp:859-860, 943-944` で `RenderMotionBlurOtherAvatars` / `RenderMotionBlurSelfAvatar` 別に motion blur 抑制を出し分けている (r30 P2 BD lineage)。

---

## 6. 既存の per-draw flag (参考)

`mAttachedToAvatar` と同じ「LLDrawInfo に詰めて全 dispatcher で uniform 化」パターンは AYAstorm 内で複数あり、同じ場所に追加すれば良い:

- `mIsSSSTarget` (r20 Phase C, SSS skin flag, `pushGLTFBatch` 等で `aya_sss_skin_flag` uniform)
- `mFSPickerLocalID` (r21.1 M4.17, picker per-prim ID)
- `mAttachedToAvatar` (r30 P2, motion blur 抑制 + 装着物 canary)

---

## 7. 検証: 5 値 canary レシピ (現行)

§0 で示した 5 値色分け (green = HUD / magenta = MeshBody・MeshHead / blue = 不透明な現代 Mesh 装飾物 / **brown = 透過 (alpha BLEND) 現代 Mesh 装飾物** / gray = レガシー prim・sculpt 装飾物) の実装レシピ。

### 7.1 C++ 側 uniform set (各 dispatcher §3.1 の 7 箇所)

**alpha pool 以外の dispatcher** (Simple / Bump / Material / PBR opaque / lldrawpool.cpp の hook 4 箇所) — 透過 path に乗らない側、blue (2) を渡す:

```cpp
static LLStaticHashedString s_aya_attachment_canary("aya_attachment_canary");
LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;  // pool により mShader
if (cur)
{
    cur->uniform1i(s_aya_attachment_canary,
                   params.mAttachedToAvatar.notNull()
                       ? (params.mIsBoMBodyOrHead ? 1
                            : (params.mIsPrim ? 3 : 2))
                       : 0);
}
```

**alpha pool 専用 dispatcher** (`lldrawpoolalpha.cpp:851` 周辺) — 透過合成 (BLEND) 専用 path、blue (2) を **brown (4)** に振り替える。magenta (1) / gray (3) は意味を優先して維持:

```cpp
current_shader->uniform1i(
    s_aya_attachment_canary,
    params.mAttachedToAvatar.notNull()
        ? (params.mIsBoMBodyOrHead ? 1
             : (params.mIsPrim ? 3 : 4))
        : 0);
```

値の意味 (優先順位の通り):
- `0` = 非装着物 → そのまま元描画
- `1` = MeshBody / MeshHead (BoM 装着、= **ボディ**) → magenta
- `3` = プリム / sculpt 装着物 (= レガシー装飾物、~1%) → gray
- `4` = 透過合成が必要な現代 Mesh 装飾物 (alpha pool 経由) → brown
- `2` = 不透明 or alpha MASK の現代 Mesh 装飾物 (alpha pool 以外) → blue

判定順は **`mIsBoMBodyOrHead` 最優先 → `mIsPrim` 次点 → 残りは pool で 2 or 4 に分岐**。dispatcher で値を分けるのが鍵 — shader 単独では「自分が alpha pool 経由で bind されたか」を判別できない。

### 7.2 GLSL 側 5 値分岐パターン

frag_color 系 (forward / HUD あり shader):

```glsl
if (aya_attachment_canary != 0)
{
#ifdef IS_HUD
    frag_color = vec4(0.0, 1.0, 0.0, a);                   // HUD = green (全装着物)
#else
    vec3 canary_rgb = (aya_attachment_canary == 1)
        ? vec3(1.0, 0.0, 1.0)                              // BoM body/head = magenta
        : ((aya_attachment_canary == 3)
            ? vec3(0.5, 0.5, 0.5)                          // プリム装着物 = gray
            : ((aya_attachment_canary == 4)
                ? vec3(0.214, 0.051, 0.0)                  // alpha BLEND 装着物 = brown (sRGB 0.5,0.25,0)
                : vec3(0.0, 0.0, 1.0)));                   // mesh 装着物 = blue
    frag_color = vec4(canary_rgb, a);
#endif
    return;
}
```

frag_data 系 (deferred gbuffer shader、IS_HUD 無し):

```glsl
if (aya_attachment_canary != 0)
{
    vec3 canary_rgb = (aya_attachment_canary == 1)
        ? vec3(1.0, 0.0, 1.0)                              // BoM body/head = magenta
        : ((aya_attachment_canary == 3)
            ? vec3(0.5, 0.5, 0.5)                          // プリム装着物 = gray
            : ((aya_attachment_canary == 4)
                ? vec3(0.214, 0.051, 0.0)                  // alpha BLEND 装着物 = brown (sRGB 0.5,0.25,0)
                : vec3(0.0, 0.0, 1.0)));                   // mesh 装着物 = blue
    frag_data[0] = vec4(canary_rgb, 0.0);
    frag_data[1] = vec4(0.0);
    frag_data[2] = encodeNormal(norm, 0.0, GBUFFER_FLAG_HAS_PBR);  // or HAS_ATMOS
#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(canary_rgb, 0.0);
#endif
    return;
}
```

**色値の根拠 (色空間補正)**:
- shader 出力は **linear 空間** で書く (それが後段で tonemap → sRGB に変換される)
- sRGB で `(0.5, 0.25, 0.0)` ≈ 茶を出したい → linear では `srgb_to_linear(0.5, 0.25, 0.0)` = **`vec3(0.214, 0.051, 0.0)`**
- 鋭い primaries (`magenta(1,0,1)` / `blue(0,0,1)` / `green(0,1,0)`) は tonemap で色相シフトが小さいので linear 直書きで OK だが、**茶のような中間値は linear 直書きすると tonemap で黄〜橙に飛ぶ** (実証: r30 P5 で `(0.5, 0.25, 0.0)` 直書き → 髪が黄になった)
- 色変更時は **「画面で見たい sRGB 色」を `srgb_to_linear` 換算してから shader に書く** ことを徹底

**青紫廃案メモ**: 当初 `vec3(0.5, 0.0, 1.0)` を試したが magenta (BoM body/head) と画面で紛らわしくて NG。明確に違う系統の茶を採用 (2026-05-21、AYA さん指示)。

### 7.3 実装済 shader (2026-05-21 時点、11 file 全 5 値化済)

- `class1/deferred/pbropaqueF.glsl` (deferred main + HUD main 別 main、2 main 構造)
- `class2/deferred/pbralphaF.glsl` (deferred main + HUD main 別 main、2 main 構造)
- `class2/deferred/alphaF.glsl` (単一 main、IS_HUD 内部分岐)
- `class1/deferred/fullbrightF.glsl` (単一 main、IS_HUD 内部分岐)
- `class3/deferred/materialF.glsl` (forward BLEND + deferred gbuffer の 2 箇所)
- `class1/deferred/bumpF.glsl`
- `class1/deferred/diffuseF.glsl`
- `class1/deferred/diffuseIndexedF.glsl`
- `class1/deferred/diffuseAlphaMaskF.glsl`
- `class1/deferred/diffuseAlphaMaskIndexedF.glsl`
- `class1/deferred/diffuseAlphaMaskNoColorF.glsl`

### 7.4 カバー漏れデバッグの順序

1. 自分の avatar 本体 (MeshBody/MeshHead) が magenta にならない → `mIsBoMBodyOrHead` 判定 (§1.4) が false になっている → BoM 装着していない or BoM tex を参照していない (legacy texture を貼っている body)
2. 装着物が 1 つも色付かない (青・magenta・灰 のいずれも出ない) → `mAttachedToAvatar` が set されていない → `llvovolume.cpp:5829` 周辺の分岐確認
3. 一部の装着物だけ色付かない → その draw が踏んでいる shader が §7.3 リストに無い = §2 の表に未登録の path → Apitrace / RenderDoc / shader cache の `shaderUsage.json` で逆引き
4. HUD だけ色付かない (= green にならない) → shader の `#ifdef IS_HUD` 分岐に uniform 宣言が無い、または `aya_attachment_canary` uniform が `#ifndef IS_HUD` の内側に置かれて HUD shader からアクセス不能 (link 失敗 → 起動時 crash する)
5. 服が誤って magenta になる → §1.4 の判別限界 (2) 該当 (BoM tattoo/clothing layer を持つ服が `UPPER`/`LOWER` を参照)、object 内 1 face でも一致で true なので誤判定。許容できない場合は判定を per-face に変える
6. 現代 Mesh 装飾物が誤って **灰** になる → `vobj->isMesh()` が false を返している → object が `LL_SCULPT_TYPE_MESH` ではない (= 古い prim / sculpt として upload された); 製品側の問題か、`isMesh()` の判定変更が必要かを切り分け
7. レガシー prim 装飾物が誤って **青** になる → `mIsPrim` set 漏れ → `llvovolume.cpp:5829` 周辺で `draw_info->mIsPrim = !vobj->isMesh()` 行が走っているか確認 (`mAttachedToAvatar.notNull()` の中の判定なので分岐スコープに注意)
8. 透過装飾物が **青のまま** で茶にならない (= 透過 / 不透明 の分離が効いていない) → alpha pool dispatcher (`lldrawpoolalpha.cpp:851` 周辺) で blue (2) → brown (4) 振り替えが入っているか確認。dispatcher 単位の uniform set なので、別 pool (Simple / Material / PBR opaque) からは 2 のまま流れるのが正しい挙動
9. 不透明装飾物が誤って **茶** になる → alpha pool 以外の dispatcher が誤って 4 を渡している → §7.1 の「alpha pool 以外」のコードが `: 4` ではなく `: 2` で終わっているか確認。alpha pool だけが 4 を出す配線を堅持する

---

## 8. HUD 描画 path (装着物の特殊形態)

HUD attachment (`LLViewerObject::isHUDAttachment()`) は **装着物の特殊形態**。 attachment point が `HUD_ATTACHMENT_POINT_START` 以上に attach されたもので、world geometry とは **別 pass / 別 camera / 別 shader permutation** で描画される。

### 8.1 HUD pass 起動点

`indra/newview/llviewerdisplay.cpp:1404-1492` の `display()` 内 1 ブロックのみ:

```cpp
if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices())
{
    LLPipeline::sRenderingHUDs = true;                // ← HUD pass 印 (1406)
    LLCamera hud_cam = *LLViewerCamera::getInstance();
    hud_cam.setOrigin(-1.f, 0.f, 0.f);                // HUD 専用 camera
    hud_cam.setAxes(LLVector3(1,0,0), LLVector3(0,1,0), LLVector3(0,0,1));
    // ...
    // RENDER_TYPE_HUD のみ on、HUD 用 type / pass を全部 toggle on (1449-1472)
    // ...
    gPipeline.renderGeomPostDeferred(hud_cam);        // ← deferred 後の forward 経路で描画 (1476)
    // ...
    LLPipeline::sRenderingHUDs = false;               // ← HUD pass 終了 (1491)
}
```

### 8.2 HUD pass の特徴

| 項目 | World geometry pass | HUD pass |
|---|---|---|
| 描画関数 | `renderGeom` + deferred lighting | `renderGeomPostDeferred` のみ (forward) |
| Gbuffer | あり (frag_data[4] に書く) | **無し** (frag_color に直接書く) |
| Camera | `LLViewerCamera::getInstance()` | hud_cam (origin (-1,0,0), identity axes) |
| Shader permutation | 通常版 | **IS_HUD=1** permutation 版 |
| 観測者 | 全 player から見える | **装着している本人のみ** から見える |
| 大気 / 影 / 反射プローブ | 全部かかる | 全部 skip (forward でも IS_HUD で disable) |

### 8.3 HUD pass で toggle on される render type (`llviewerdisplay.cpp:1449-1472`)

通常 pass で off になっている下記を HUD pass 中だけ on にする (= HUD 専用に独立描画する):

- RENDER_TYPE: BUMP / SIMPLE / VOLUME / ALPHA / ALPHA_PRE_WATER / ALPHA_MASK / FULLBRIGHT_ALPHA_MASK / FULLBRIGHT / GLTF_PBR / GLTF_PBR_ALPHA_MASK
- RENDER_TYPE_PASS: ALPHA / ALPHA_MASK / BUMP / MATERIAL / FULLBRIGHT / FULLBRIGHT_ALPHA_MASK / FULLBRIGHT_SHINY / SHINY / INVISIBLE / INVISI_SHINY / GLTF_PBR / GLTF_PBR_ALPHA_MASK

これらが §2 表の各 pool / dispatcher を経由して HUD 用 shader を bind し描画する。

### 8.4 HUD 用 shader (`IS_HUD=1` permutation バインド)

`addPermutation("IS_HUD", "1")` を打つ場所は `indra/newview/llviewershadermgr.cpp` の以下 7 箇所:

| Line | Program | Shader file | 用途 |
|---|---|---|---|
| 1545 | `gHUDPBROpaqueProgram` | `deferred/pbropaqueF.glsl` | HUD GLTF PBR opaque |
| 1620 | `gHUDPBRAlphaProgram` | `deferred/pbralphaF.glsl` | HUD GLTF PBR alpha BLEND |
| 1923 | `gHUDAlphaProgram` | `deferred/alphaF.glsl` (i=2 / hud=true 分岐) | HUD legacy alpha (3 つの alpha shader を 1 loop で作る、`hud = (i==2)`) |
| 2051 | `gHUDFullbrightProgram` | `deferred/fullbrightF.glsl` | HUD fullbright (texture only) |
| 2094 | `gHUDFullbrightAlphaMaskProgram` | `deferred/fullbrightF.glsl` | HUD fullbright + alpha MASK |
| 2142 | `gHUDFullbrightAlphaMaskAlphaProgram` | `deferred/fullbrightF.glsl` | HUD fullbright + alpha MASK + IS_ALPHA |
| 2186 | `gHUDFullbrightShinyProgram` | `fullbrightShinyF.glsl` | HUD shiny |

### 8.5 IS_HUD 分岐を持つ shader (4 file)

| Shader | 構造 |
|---|---|
| `class1/deferred/fullbrightF.glsl` | 単一 main、内部で `#ifdef IS_HUD` 分岐 (out 1 つ) |
| `class2/deferred/alphaF.glsl` | 単一 main、`#ifdef IS_HUD` で linear_to_srgb + final_scale=1 だけ分岐 |
| `class1/deferred/pbropaqueF.glsl` | **`#ifndef IS_HUD` で 2 main 完全分離** (deferred main vs HUD forward main) |
| `class2/deferred/pbralphaF.glsl` | 同上 (2 main 分離) |

**罠**: pbropaqueF / pbralphaF は 2 main 構造のため、`#ifndef IS_HUD` の外側にも内側にも uniform を置く必要がある identifier が出てくる。canary uniform は **外側に置く** ことで両 main から見える。内側 (deferred main 側) に置くと HUD shader の link が `undefined variable` で失敗し、`llglslshader.cpp:1053` の `ASSERT (mProgramObject != 0)` で起動時 crash する (r30 P5 実体験)。

### 8.6 C++ 側で「これは HUD attachment」と判定する場所

- `LLViewerObject::isHUDAttachment()` — attachment point が HUD 範囲かを返す
- `LLPipeline::sRenderingHUDs` (static) — 現在 HUD pass を描いている最中か (描画系の helper で参照)
- `LLPipeline::sShowHUDAttachments` (static) — HUD 描画自体が有効か (debug settings `RenderHUDInSnapshot` 等で off にできる)

### 8.7 HUD 装着物だけ別表現にする (r30 P5 実装例)

shader 側で `#ifdef IS_HUD` を使えば C++ を一切触らずに HUD 専用分岐が書ける (HUD shader は IS_HUD permutation 別 program なので compile-time 確定)。世界側 (非 HUD) は §7.2 の 5 値分岐 (magenta / blue / brown / gray) を、HUD 側はそれを上書きして常に green を出す:

```glsl
if (aya_attachment_canary != 0)
{
#ifdef IS_HUD
    frag_color = vec4(0.0, 1.0, 0.0, color.a);             // HUD = green (全装着物まとめて)
#else
    vec3 canary_rgb = (aya_attachment_canary == 1)
        ? vec3(1.0, 0.0, 1.0)                              // BoM body/head = magenta
        : ((aya_attachment_canary == 3)
            ? vec3(0.5, 0.5, 0.5)                          // プリム装着物 = gray
            : ((aya_attachment_canary == 4)
                ? vec3(0.214, 0.051, 0.0)                  // alpha BLEND 装着物 = brown (sRGB 0.5,0.25,0)
                : vec3(0.0, 0.0, 1.0)));                   // mesh 装着物 = blue
    frag_color = vec4(canary_rgb, color.a);
#endif
    return;
}
```

実装済 file: `fullbrightF.glsl`, `alphaF.glsl`, `pbropaqueF.glsl` (HUD main), `pbralphaF.glsl` (HUD main)。HUD pass は世界側 5 値の差を捨てて green に丸めるので、HUD は「装着物全部まとめて 1 色」として観測される (= HUD は本人にしか見えない 別カテゴリという §0 の整理と一致)。

---

## 9. 茶 (alpha BLEND) path = 撮影描画系バグの鬼門

「装着物の中で茶になる部分」は **alpha BLEND** で描かれる面 = SL 描画 pipeline で最も例外処理が多く、撮影描画 (r30 章) のバグが集中する path。本節では「茶 path で典型的に発生する問題」と、修正に着手するときの **最低限の確認順序** をまとめる。

### 9.1 茶 path だけで起きる典型バグ

| 症状 | なぜ茶だけ起きるか |
|---|---|
| **DoF (焦点距離) が透過部だけ効かない / 誤った距離でぼける** | alpha pool は forward path で gbuffer (depth含む) に書かない。後段の DoF pass は depth buffer の **opaque 部分の深さ** しか参照できないので、透過面の手前/奥の判定が opaque 背景に置き換わる |
| **SSAO / SSR / 反射プローブが透過部で抜ける** | 同上。SSAO/SSR は gbuffer depth + normal を読むが、透過は gbuffer に書かないので「そこには何もない」扱い |
| **Z-order で背面が貫いて見える** | alpha BLEND は depth write しない (or 限定的にしか書かない)。複数透過 face の前後関係はソート順序に依存し、視点移動で順序が変わると見え方が変わる |
| **影が出ない / 受けない** | shadow map pass で alpha BLEND は skip されることが多い (shadow caster には alpha MASK だけ含めるのが一般的) |
| **tonemap / exposure の効き方が opaque と微妙に違う** | alpha BLEND は scene HDR buffer に **forward で直接 src*src_alpha + dst*(1-src_alpha) で混色** する。post-process exposure / bloom が alpha channel をどう扱うかで結果が変わる |
| **GLTF PBR と legacy material で透過の見え方が違う** | 同じ「透過した服」でも `class2/pbralphaF.glsl` (新 GLTF) と `class2/alphaF.glsl` (legacy) で別 shader。tonemap / atmospheric 適用の式が微妙に違う |
| **water 越し / mirror plane 越しの透過装飾物が消える / 二重に出る** | water pass / mirror pass は別 render target で `waterClip` / `mirrorClip` discard を打つ。透過 face は alpha pool 内で別ループに乗ることがあり、両方の clip が正しく揃わないと壊れる |

### 9.2 茶 path を触る前の必須トレース

修正に入る前に以下を **必ず確認** (推論で進めると 24 時間溶ける):

1. **茶 canary を ON にして実機で確認** (`Debug Settings: aya_attachment_canary` 経由ではなく shader 直書きでも可) — 自分が直したい画素が本当に **茶 (alpha BLEND)** path に乗っているか目で確認する。**「透過に見える」と「alpha BLEND を踏んでいる」は別** (alpha MASK / discard でも視覚的には透けて見える)
2. **bound shader を特定** — 茶になっている面が:
   - `class2/deferred/alphaF.glsl` (legacy alpha BLEND)
   - `class2/deferred/pbralphaF.glsl` (GLTF PBR alpha BLEND)
   - `class3/deferred/materialF.glsl` の **forward branch** (`#ifdef IS_ALPHA`)
   - `class1/deferred/fullbrightF.glsl` の `IS_ALPHA` branch
   
   どれを踏んでいるか shader cache の log + Apitrace / RenderDoc で確定する。同じ「透過服」でも素材 (legacy / PBR) で別 shader
3. **alpha pool dispatcher の render state** — `lldrawpoolalpha.cpp` の `renderAlpha` 周辺で:
   - `glBlendFunc` / `glDepthMask` / `glDepthFunc` がどう設定されているか
   - depth pre-pass (`lldrawpoolalpha.cpp:411`) を踏むか踏まないか
   - emissive 用 secondary pass (`lldrawpoolalpha.cpp:534/561/608`) が同じ face を 2 度描いていないか
4. **後段 post-process pass がどう読むか** — DoF / SSAO / SSR / mirror / water 各 pass が:
   - `frag_data[0/1/2/3]` の何を読むか (透過は gbuffer に書かないので、何が「読めない」状態になっているか確認)
   - depth buffer のみで判定しているか、alpha mask buffer も読むか

### 9.3 修正パターン (経験則)

- **「透過部だけ DoF が変」**: post-process DoF pass で depth buffer を読んだあと、alpha BLEND face の手前/奥を別 buffer (alpha mask / depth peeling) で補正する必要がある可能性大。upstream LL でも未解決テーマ。
- **「透過の Z-order が崩れる」**: alpha pool 内のソートキー (`F_GreaterThan` 距離ソート) と、本当の前後関係 (per-face) のズレ。複雑な mesh では避けがたく、A2C (Alpha to Coverage) や OIT (Order-Independent Transparency) 検討対象。
- **「片面しか見えない」**: `glDisable(GL_CULL_FACE)` してない or 2-sided alpha が rigged mesh で破綻。alpha pool では back-face culling 設定が pass ごとに違う。
- **「色だけ違う」**: tonemap / srgb_to_linear / linear_to_srgb の適用順序が opaque path と alpha path で違う。canary で色空間を確認する習慣を持つ (§7.2 の経験 — linear で `(0.5, 0.25, 0.0)` を書くと画面で黄になる)。

### 9.4 茶 path の修正に着手する前のチェックリスト

- [ ] 茶 canary で対象画素が alpha BLEND 経由と確認した
- [ ] 対象画素が踏んでいる shader を 4 候補のいずれか 1 つに特定した
- [ ] alpha pool dispatcher の render state (blend / depth / cull) を Apitrace で確認した
- [ ] 修正対象の post-process pass が gbuffer / alpha mask / depth 各 buffer をどう読むか確認した
- [ ] 同じ症状が opaque (青) でも起きるか (= alpha 固有か否か) を切り分けた

上記が揃っていない状態で shader 内 if 分岐や定数調整に手を出すと、原因の違う問題を覆い隠してさらに本丸を見失う。

---

## 10. 関連リファレンス

- `ayastorm-deferred-shader-routing.md` — gbuffer flag → softenLightF 分岐の対応 (lit 計算側)
- `ayastorm-gbuffer3-trace.md` — gbuffer3 storage (alpha channel 仕様)
- `ayastorm-attachment-magenta-canary-trace.md` — 本書のもとになった canary 実装の trace
