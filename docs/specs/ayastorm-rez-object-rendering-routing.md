# AYAstorm Rez Object Rendering Routing リファレンス

**作成日**: 2026-05-22 (r30 P5 透過 DoF C 案準備、装着物資料 `ayastorm-attachment-rendering-routing.md` の双子。SIM-rezzed object を全 dispatcher で per-draw 識別して全 path を canary 検証して確定)
**対象**: SIM に Rez された world geometry (`vobj->getAvatar() == NULL` かつ terrain/sky/water/grass 系 pool でない) が deferred で踏む **全描画 path** の確定マップ。「地上に Rez された建物・木・mesh・prim の描画はどこか」を即座に引けるようにするためのリファレンス。
**作成経緯**: r30 P5 透過 DoF C 案 (alpha BLEND を別 RT に分離) 着手前に「地上にあるもの = SIM-rezzed object を全部 1 色 (黒) で塗りつぶす」canary を全 dispatcher に仕込み、装着物資料と対称な path map を実機で確定した。装着物資料が `mAttachedToAvatar.notNull()` 側の地図、本書は **その鏡像** (`mAttachedToAvatar.isNull()` 側) の地図。

> **再利用方針**: world geometry 単位で per-draw 処理を入れたい (terrain と建物を区別したい / SIM ↔ HUD 境界判定 / 透過 RT 分離) ときは **まず本書を参照**。装着物資料と対称関係なので両方を併読すると pool 全体の枠が一望できる。

---

## 0. クイック参照: 現行 canary 色定義 (2026-05-22 時点、Rez Object = 11 / 12)

実機で何色が出ているかから「これは何」を即座に逆引きするための表。canary uniform 名は `aya_attachment_canary` (int、装着物資料と共用 — 1 個の uniform で装着物 + Rez Object 両側をまとめて識別)。値は `LLDrawInfo` の `mAttachedToAvatar` 有無で「装着物側 (1〜4)」か「Rez Object 側 (11〜12)」か振り分ける。

| 実機で見える色 | 意味 | canary uniform 値 | C++ 条件 |
|---|---|---|---|
| 🟢 **green (HUD)** | **HUD 装着物** | 任意 (`!= 0`) | (装着物資料 §0 参照) |
| 🟣 **magenta** | **MeshBody / MeshHead** | `1` | (装着物資料 §0 参照) |
| 🟫 **brown** | **透過装飾物 (alpha BLEND attachment)** | `4` | (装着物資料 §0 参照) |
| 🔵 **blue** | **不透明 mesh 装飾物** | `2` | (装着物資料 §0 参照) |
| ⚪ **gray** | **レガシー prim 装飾物** | `3` | (装着物資料 §0 参照) |
| ⚫ **black** | **SIM-rezzed Object の opaque / alpha MASK 面** = 建物・mesh・prim・sculpt・Linden tree などの非透過部分 | `11` | `mAttachedToAvatar.isNull()` かつ **alpha pool 以外** の dispatcher 経由 (Simple / Bump / Material / PBR opaque / tree) |
| 🟢 **green (Rez)** | **SIM-rezzed Object の alpha BLEND 面** = 透ける窓ガラス・葉先・透過装飾・半透明 light fixture など | `12` | `mAttachedToAvatar.isNull()` かつ **alpha pool 経由** dispatcher (`lldrawpoolalpha.cpp:851` 周辺) |
| (元色) | (上記いずれにも該当しない) 通常描画 | `0` | 任意の reset (実際は dispatcher が常に 0/1/2/3/4/11/12 のいずれかを set するので 0 はほぼ出ない) |

**11 vs 12 の選定根拠 (2026-05-22)**: 装着物側で「同じ Mesh 装飾物でも opaque (2=blue) と alpha BLEND (4=brown) を分けた」のと同じ動機で、Rez Object 側も `mAttachedToAvatar.isNull()` 1 つで括らず opaque (11=黒) と alpha BLEND (12=緑) に分離。**透過 DoF C 案 (`#275`, alpha BLEND を別 RT に分離)** の前段識別として、画面で「opaque vs alpha BLEND」を直接視認できる canary 化が必要だったため。

**緑 (HUD) vs 緑 (Rez) の衝突回避**: HUD は本人にしか見えない別 pass、Rez Object alpha BLEND は world geometry。同じ画面で重ねて見える状況は稀。万一の区別が必要になったら HUD 緑を変更する (Rez 側を変えると alpha pool dispatcher の C++ 値を全 shader で同期する必要があるため固定が望ましい)。

**重要な約束 (AYA さんの呼称定義)**:
- 「**地上にある Object**」「**Rez Object**」 = SIM に Rez された world geometry の vobj 全般 (canary=11 or 12)
  - 建物 / 屋根 / 構造物 (mesh / prim / sculpt) → opaque 面は 11 (黒)、alpha BLEND 面は 12 (緑)
  - 木の幹 / 葉 (Linden tree pool = `LLDrawPoolTree`、drawRange 直叩きで LLDrawInfo を介さない特殊 path) → 常に 11 (黒、shader bind 時 forced)
  - 装飾家具・道具・装飾物
  - mesh upload された装飾建材
  - 透ける窓ガラス・葉先・半透明 light fixture などの **alpha BLEND 面は 12 (緑)**、それ以外 (opaque / alpha MASK) は 11 (黒)
- 「**Rez Object に含まれないもの**」 = 別 pool で本 canary uniform を経由しない:
  - **空 (sky)** — `LLDrawPoolSky` / `LLDrawPoolWLSky` (atmospheric / WindLight)
  - **土地 (terrain)** — `LLDrawPoolTerrain` (地面の texture splat / heightmap)
  - **海 (water)** — `LLDrawPoolWater` / `LLDrawPoolWaterExclusion`
  - **草 (grass)** — `LLDrawPoolGrass` (Linden grass pool、tree とは別)
  - **particles** — particle pool (alpha pool 内分岐、LLDrawInfo 経由するが mAttachedToAvatar.isNull で 12 が振られるはず — 検証要)

**値域整理 (2026-05-22 時点)**:
- `0` = reset / 未振り分け (現状 dispatcher が必ず set するので実質出ない)
- `1` / `3` = 装着物のうち BoM body・head / プリム (どの pool 経由でも値固定、意味優先)
- `2` = 装着物のうち alpha pool **以外** 経由の Mesh (blue)
- `4` = 装着物のうち alpha pool 経由の Mesh (brown)
- `5〜10` = 将来予約 (terrain canary / sky canary / water canary 等)
- `11` = Rez Object のうち alpha pool **以外** 経由 (黒) — opaque / alpha MASK の SIM 配置物
- `12` = Rez Object のうち alpha pool 経由 (緑) — alpha BLEND の SIM 配置物
- `13〜` = 将来予約 (Rez Object 細分化 — 例: tree only / GLTF only / static vs dynamic 等)

「opaque vs alpha BLEND」軸は装着物 (2 vs 4) と Rez Object (11 vs 12) で同じ意味論で揃えてある。

**canary 必須の前提**: 本 uniform は **dispatcher (= LLDrawInfo を pop して draw する側) が set する**。dispatcher を経由しないで drawRange を直叩きする pool (= tree pool) では shader bind 直後に shader 全体に 1 度 11 を流す方式に切り替えてある (§3.3 参照)。

---

## 1. 「Rez Object」の定義 (コード上)

### 1.1 識別子: `LLDrawInfo::mAttachedToAvatar` (の **否定** 側)

`indra/newview/llspatialpartition.h:130`:

```cpp
LLPointer<LLVOAvatar> mAttachedToAvatar = nullptr;
```

- `notNull()` = 装着物 (装着物資料 §1)
- **`isNull()` = Rez Object (本書のスコープ)** = world geometry / SIM-rezzed prim / mesh / sculpt / Linden tree など、装着していない vobj

すなわち装着物と Rez Object は **1 個の flag (`mAttachedToAvatar`) で 1:1 排他に分かれる**。pool 自体は同じものを共有することも多い (例: `LLDrawPoolSimple` は装着物の不透明 mesh と Rez された装飾物の不透明 mesh の **両方** を流す)。違いは LLDrawInfo の `mAttachedToAvatar` だけ。

### 1.2 set 場所 (装着物資料と同じ)

`indra/newview/llvovolume.cpp:5838`:

```cpp
if (LLViewerObject* vobj = facep->getViewerObject())
{
    draw_info->mIsSSSTarget       = vobj->isSSSTarget();
    draw_info->mFSPickerLocalID   = vobj->getLocalID();
    draw_info->mAttachedToAvatar  = vobj->getAvatar();  // ← isNull() なら Rez Object
}
```

`vobj->getAvatar()` の挙動は装着物資料 §1.3 参照。要約:
- 装着している → 装着先 avatar pointer
- animesh → control avatar pointer
- **それ以外 (= 親 chain に avatar がない vobj) → NULL = Rez Object**

### 1.3 Rez Object のうち pool で別れるもの

Rez Object 全体は `mAttachedToAvatar.isNull()` で 1 つだが、内部的に踏む pool が分かれる。canary 値は **alpha pool 経由 = 12 (緑)**、**その他 = 11 (黒)** で振り分けられる:

| Rez Object の種類 | 踏む Pool | canary 値 |
|---|---|---|
| 不透明 mesh / prim / sculpt (建物・mesh家具・mesh建材) | `LLDrawPoolSimple` / `LLDrawPoolBump` | **11 (黒)** |
| Legacy material (normal/spec map 付き mesh) | `LLDrawPoolMaterials` | **11 (黒)** |
| GLTF PBR opaque (新型 mesh 建材) | `LLDrawPoolPBROpaque` (= `LLDrawPoolGLTFPBR` の opaque 分岐) | **11 (黒)** |
| Alpha MASK 面 (discard cutout 葉・服) | 各 pool の alpha mask shader | **11 (黒)** ← MASK は alpha pool ではない |
| **Alpha BLEND 面** (透ける窓ガラス / 半透明装飾 / アルファ装飾) | `LLDrawPoolAlpha` (legacy + PBR 共通) | **12 (緑)** |
| Linden tree (`LLVOTree`) | `LLDrawPoolTree` | **11 (黒、shader bind 時 forced § 3.2)** |
| Linden grass | `LLDrawPoolGrass` | 対象外 (本書スコープ外) |
| Terrain | `LLDrawPoolTerrain` | 対象外 |
| Sky / WindLight | `LLDrawPoolSky` / `LLDrawPoolWLSky` | 対象外 |
| Water / Mirror | `LLDrawPoolWater` / `LLDrawPoolWaterExclusion` | 対象外 |

**alpha MASK は 11 (黒) で 12 (緑) ではない** 注意点: alpha MASK (discard cutout) は不透明 path (depth write する) を踏むので、dispatcher は alpha pool でなく simple/material 系経由。視覚的には透けて見えるが pipeline 的には opaque。12 (緑) は「forward alpha BLEND 合成」専用 = SL pipeline 上で `LLDrawPoolAlpha::renderAlpha` を踏む面のみ。

---

## 2. 描画 path 完全分類: Pool × Shader × Dispatcher

Rez Object (`mAttachedToAvatar.isNull()`) が踏みうる **可視 base color** path の完全表 (装着物資料 §2 と pool は同一、`isNull()` 側に振り分けられる)。

| Rez Object の種類 | Pool | drawRange 呼び出し元 | Bound shader (fragment) | 備考 |
|---|---|---|---|---|
| **GLTF PBR opaque** (新型 mesh 建材) | `LLDrawPoolPBROpaque` | `LLRenderPass::pushGLTFBatch` `lldrawpool.cpp:1138`, `pushUntexturedGLTFBatch` `lldrawpool.cpp:1165` | `class1/deferred/pbropaqueF.glsl` | log で `Deferred PBR Opaque Shader` / `PBR Glow Shader` 観測 |
| **GLTF PBR alpha BLEND** | `LLDrawPoolAlpha` (PBR 分岐) | `LLDrawPoolAlpha::renderAlpha` inline `lldrawpoolalpha.cpp:859` | `class2/deferred/pbralphaF.glsl` (deferred) | alpha-blend mode の GLTF material |
| **Legacy material** | `LLDrawPoolMaterials` | `LLDrawPoolMaterials::renderDeferred` inline `lldrawpoolmaterials.cpp:307` | `class3/deferred/materialF.glsl` | log で `Material Shader 0/2/4/8/12/13` 観測 |
| **Legacy alpha BLEND** (透ける窓 / 葉先 等) | `LLDrawPoolAlpha` (legacy 分岐) | `lldrawpoolalpha.cpp:859` | `class2/deferred/alphaF.glsl` | log で `Deferred Alpha Shader` 観測 |
| **Simple textured** | `LLDrawPoolSimple` | `pushBatch` `lldrawpool.cpp:665`, `pushUntexturedBatch` `lldrawpool.cpp:698` | `class1/deferred/diffuseF.glsl`, `diffuseIndexedF.glsl`, `diffuseAlphaMaskF.glsl`, `diffuseAlphaMaskIndexedF.glsl`, `diffuseAlphaMaskNoColorF.glsl` | log で `Deferred Diffuse Shader` / `Deferred Diffuse Non-Indexed Alpha Mask Shader` 観測 |
| **Bump-mapped** | `LLDrawPoolBump` | `pushBatch` `lldrawpool.cpp:665` (alpha mask), `pushBumpBatch` `lldrawpoolbump.cpp:1068` (本体) | `class1/deferred/bumpF.glsl` | log で `Deferred Bump Shader` / `Bump Shader` 観測 |
| **Fullbright** (発光 mesh / 表示パネル等) | `LLDrawPoolBump` fullbright path | `pushBatch` `lldrawpool.cpp:665` | `class1/deferred/fullbrightF.glsl` | log で `Deferred Fullbright Shader` / `Deferred FullbrightShiny Shader` / `Deferred Emissive Shader` 観測 |
| **Linden Tree** (`LLVOTree` 専用) | `LLDrawPoolTree` | `LLDrawPoolTree::renderDeferred` 内 `LLFace*->getVertexBuffer()->drawRange` 直叩き `lldrawpooltree.cpp:139` | `class1/deferred/treeF.glsl` | **LLDrawInfo を経由しない**。canary は §3.3 の shader-bind 時 forced 11 方式 |

### 2.1 shadow pass (補足)

Rez Object も shadow caster として shadow map に書かれる。shadow pass で踏む dispatcher:

| Shadow shader | dispatcher |
|---|---|
| `Deferred Shadow Shader` | `pushUntexturedBatch` `lldrawpool.cpp:698`, `pushUntexturedGLTFBatch` `lldrawpool.cpp:1206` |
| `Deferred Shadow Alpha Mask Shader` | `pushBatch` `lldrawpool.cpp:665` |
| `Deferred Tree Shadow Shader` | `pushBatch` `lldrawpool.cpp:665` (tree shadow) |

これらにも canary uniform は流れる (装着物資料 §3.1 と同じ dispatcher 経由)。Shadow pass で frag_color は使われない (depth のみ書込み) ので canary 色は実質見えないが、log で確認はできる。

---

## 3. drawRange 呼び出し点 全分類 — Rez Object 側

### 3.1 LLDrawInfo 経由 dispatcher (装着物資料 §3.1 と同じ 7 箇所)

Rez Object も装着物と **同じ 7 箇所** の dispatcher を経由する。違いは `mAttachedToAvatar.isNull()` 側で canary=11 が振られる点のみ。

| File:Line | 呼び出し関数 | uniform set 条件 |
|---|---|---|
| `lldrawpool.cpp:665` | `LLRenderPass::pushBatch` | `att = params.mAttachedToAvatar.notNull()`; `att ? (BoM ? 1 : prim ? 3 : 2) : 11` |
| `lldrawpool.cpp:698` | `LLRenderPass::pushUntexturedBatch` | 同上 |
| `lldrawpool.cpp:1138` | `LLRenderPass::pushGLTFBatch` | 同上 |
| `lldrawpool.cpp:1165` | `LLRenderPass::pushUntexturedGLTFBatch` | 同上 |
| `lldrawpoolbump.cpp:1068` | `LLRenderPass::pushBumpBatch` | 同上 |
| `lldrawpoolmaterials.cpp:307` | `LLDrawPoolMaterials::renderDeferred` inline | 同上 (`mShader` 経由) |
| `lldrawpoolalpha.cpp:859` | `LLDrawPoolAlpha::renderAlpha` inline | `att ? (BoM ? 1 : prim ? 3 : 4) : 12` (alpha pool は装着物側を 4 に、**Rez Object 側を 12 (緑) に振り替える**。BoM body/head (1) と prim (3) は意味優先で維持) |

### 3.2 LLDrawInfo 経由しない経路 — tree pool 特殊配線

`LLDrawPoolTree::renderDeferred` は LLDrawInfo を介さず、`mDrawFace` (LLFace*) を直接 iterate して `LLVertexBuffer::drawRange` を呼ぶ。`LLDrawInfo` が無いので装着物資料 §3.1 のパターン (LLDrawInfo の `mAttachedToAvatar` を見て canary 値を決める) が **使えない**。

代替策: Linden tree は **常に非装着** (Linden tree は Rez Object 専用、装着できない) なので、shader bind 時に shader 全体に **canary=11 を 1 度だけ forced** で流す。

`lldrawpooltree.cpp:78-93`:

```cpp
// Linden trees は常に非装着 (Rez Object 扱い)。tree pool は
// LLDrawInfo 経由でなく drawRange 直叩きのため、ここで shader 全体に 1 度
// canary=11 を流す。
{
    static LLStaticHashedString s_aya_attachment_canary("aya_attachment_canary");
    if (shader)
    {
        shader->uniform1i(s_aya_attachment_canary, 11);
    }
}
```

これで `treeF.glsl` も他の 11 shader と同じ canary 分岐コード (`if (aya_attachment_canary == 11) { frag_data[0] = vec4(0,0,0,0); ...}`) で黒く塗れる。

### 3.3 非可視 / bloom only → canary 不要 (装着物資料 §3.2 と同じ)

motion blur / emissive / depth pre-pass / shiny cube map など。装着物資料 §3.2 参照。

---

## 4. Rez Object 識別の per-draw 注入レシピ

新規に「Rez Object だけ別処理」を入れる場合の手順 (装着物資料 §4 の鏡像)。

### 4.1 C++ 側: uniform set

LLDrawInfo 経由 dispatcher (§3.1 の 7 箇所) は装着物資料 §4.1 と **同じ 1 個の `if` 文** で両方処理できる。新規に「Rez Object 限定の flag」が必要なら:

```cpp
static LLStaticHashedString s_aya_my_rez_flag("aya_my_rez_flag");
LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;
if (cur)
{
    cur->uniform1i(s_aya_my_rez_flag,
                   params.mAttachedToAvatar.isNull() ? 1 : 0);  // ← isNull() 側
}
```

tree pool (§3.2) では shader bind 直後に `shader->uniform1i(s_aya_my_rez_flag, 1)` を強制 (tree は常に Rez Object なので)。

### 4.2 GLSL 側: uniform 宣言と分岐

装着物資料 §4.2 と同じ。`#ifndef IS_HUD` 罠も同じ — pbropaqueF / pbralphaF は uniform を外側に置く。

### 4.3 既存 uniform を共用するか新設するか

判断指針:
- **同じ「per-draw 分類」軸を増やすだけ** (例: Rez Object のうち legacy / GLTF / tree を細分化) → 既存 `aya_attachment_canary` の **値域を拡張** (13〜 を予約に確保しているので 13, 14, 15... と使える)
- **別軸の分類** (例: Rez Object のうち動的 / 静的を分けたい) → 新 uniform を追加

`aya_attachment_canary` 値域は装着物 1〜4、Rez Object 11/12、予約 5〜10 + 13〜。新値追加時は本書 §0 表に追記すること。

---

## 5. Rez Object の細分化 (現状 + 将来分岐)

### 5.1 実装済 (2026-05-22)

| 分けたい軸 | 判定 | canary 値 |
|---|---|---|
| **Alpha BLEND だけ** | dispatcher が `LLDrawPoolAlpha::renderAlpha` (`lldrawpoolalpha.cpp:851` 周辺) | **12 (緑)** ← `att ? ... : 12` に振替済 |
| Linden tree 専用 | dispatcher が `LLDrawPoolTree` (§3.2 の forced uniform) | 現状 11 (黒) で他 opaque と同値 (区別したければ 13+ 化可能) |
| その他 opaque / alpha MASK | 残り全 dispatcher | 11 (黒) |

### 5.2 将来分岐の予約値 (5〜10 + 13〜)

| 分けたい軸 | 判定 | 予約値域 |
|---|---|---|
| Terrain canary | `LLDrawPoolTerrain` 内 shader forced | 5 (案) |
| Sky canary | `LLDrawPoolSky` / `LLDrawPoolWLSky` | 6 (案) |
| Water canary | `LLDrawPoolWater` | 7 (案) |
| Grass canary | `LLDrawPoolGrass` | 8 (案) |
| Linden tree 専用分離 (現状 11 と同色を分けたい) | dispatcher が `LLDrawPoolTree` で forced を 13 に変更 | 13 (案) |
| GLTF PBR opaque だけ | dispatcher が `pushGLTFBatch` / `pushUntexturedGLTFBatch` | 14 (案) |
| Legacy material だけ | dispatcher が `LLDrawPoolMaterials` (mShader 経由) | 15 (案) |
| Static / Dynamic (動かない / 動く) | `vobj->getVelocity().magVecSquared() < eps` 等 (motion blur 系と整合) | 16+ (案) |
| Sim ↔ HUD (HUD は Rez Object でないが念のため境界) | `LLPipeline::sRenderingHUDs` static (HUD pass 中) | 17+ (案) |

dispatcher 単位の分岐は装着物資料 §0 の「alpha pool だけ blue → brown」の仕掛けと同じ要領で、**dispatcher で値を分けてから shader に渡す** のが鍵。同じ shader が複数 pool 経由で bind される (例: `materialF.glsl` は legacy material と alpha pool 両方経由) ので、shader 側で「自分が今どの pool 経由か」を判別できない → dispatcher 側で値を確定する設計が必要。

---

## 6. 既存の per-draw flag (装着物資料 §6 と共用)

装着物資料 §6 参照。`mAttachedToAvatar` は装着物 / Rez Object 両側の 1:1 排他軸として共用される。

---

## 7. 検証: canary=11 / 12 レシピ (現行)

§0 で示した黒 (canary=11) + 緑 (canary=12) の実装レシピ。

### 7.1 C++ 側 uniform set (各 dispatcher §3.1 の 7 箇所)

装着物資料 §7.1 と **完全に同じコード** で両方処理。`mAttachedToAvatar.isNull()` 側は 11 が振られる:

```cpp
static LLStaticHashedString s_aya_attachment_canary("aya_attachment_canary");
LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;  // pool により mShader
if (cur)
{
    const bool att = params.mAttachedToAvatar.notNull();
    const int cval = att ? (params.mIsBoMBodyOrHead ? 1
                              : (params.mIsPrim ? 3 : 2))
                          : 11;  // ← Rez Object opaque
    cur->uniform1i(s_aya_attachment_canary, cval);
}
```

alpha pool 専用 (`lldrawpoolalpha.cpp:851` 周辺) は装着物側の `: 2` を `: 4` に、**Rez Object 側の `: 11` を `: 12` に振り替える**:

```cpp
const int cval = att ? (params.mIsBoMBodyOrHead ? 1
                          : (params.mIsPrim ? 3 : 4))
                      : 12;  // ← Rez Object alpha BLEND
```

tree pool (`lldrawpooltree.cpp:78-93`) は §3.2 の forced 方式 (tree は alpha BLEND を通らないので 11 固定):

```cpp
shader->uniform1i(s_aya_attachment_canary, 11);  // tree は常に Rez Object opaque
```

### 7.2 GLSL 側 canary=11 / 12 分岐パターン

5 値分岐 (装着物資料 §7.2) の末尾に Rez Object opaque (11 = 黒) と Rez Object alpha BLEND (12 = 緑) を順次ネスト:

```glsl
vec3 canary_rgb = (aya_attachment_canary == 1)
    ? vec3(1.0, 0.0, 1.0)                              // BoM body/head = magenta
    : ((aya_attachment_canary == 3)
        ? vec3(0.5, 0.5, 0.5)                          // プリム装着物 = gray
        : ((aya_attachment_canary == 4)
            ? vec3(0.214, 0.051, 0.0)                  // alpha BLEND 装着物 = brown
            : ((aya_attachment_canary == 11)
                ? vec3(0.0, 0.0, 0.0)                  // 11 = Rez Object opaque 黒
                : ((aya_attachment_canary == 12)
                    ? vec3(0.0, 1.0, 0.0)              // 12 = Rez Object alpha BLEND 緑
                    : vec3(0.0, 0.0, 1.0)))));         // mesh 装着物 = blue
```

frag_data 系 (deferred gbuffer shader、opaque path):

```glsl
frag_data[0] = vec4(canary_rgb, 0.0);
frag_data[1] = vec4(0.0);
frag_data[2] = encodeNormal(norm, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
frag_data[3] = vec4(canary_rgb, 0.0);
#endif
```

`frag_data[0].a = 0` 必須 (memory `project_aya_visual_realism_alpha_protect` 通り、後段 sky compositing で alpha=1 を引きずると sky が真っ白になる)。

frag_color 系 (forward / alpha pool / HUD あり shader) — **11 と 12 は alpha=1.0 で不透明化**:

```glsl
float canary_a = (aya_attachment_canary == 11 || aya_attachment_canary == 12) ? 1.0 : color.a;
frag_color = vec4(canary_rgb, canary_a);
```

**`canary_a` 強制 1.0 の理由**: alpha pool 経路で本来の `color.a` (透過率) をそのまま出すと、透過合成の結果「画面で見えない」状態になりかねない。canary 12 (緑) は **可視性を担保するため不透明化** する。これは canary 11 (黒) で既に採用済の流儀を 12 にも拡張した形。本番描画と同じ alpha 挙動を観察したい場合は別途 canary 用 cvar を用意する (現状不要)。

### 7.3 実装済 shader (2026-05-22 時点、12 file canary=11/12 化済)

| Shader | 装着物 1〜4 | Rez Object 11 (黒) | Rez Object 12 (緑) | alpha pool 経由? |
|---|---|---|---|---|
| `class1/deferred/pbropaqueF.glsl` | ✅ | ✅ | ✅ (defensive、実機到達はしない) | ❌ opaque only |
| `class2/deferred/pbralphaF.glsl` | ✅ | ✅ | ✅ (実機到達 = GLTF PBR alpha BLEND) | ✅ |
| `class2/deferred/alphaF.glsl` | ✅ | ✅ | ✅ (実機到達 = legacy alpha BLEND) | ✅ |
| `class1/deferred/fullbrightF.glsl` | ✅ | ✅ | ✅ (実機到達 = fullbright + IS_ALPHA) | ✅ (IS_ALPHA 分岐時) |
| `class3/deferred/materialF.glsl` | ✅ | ✅ | ✅ (実機到達 = forward BLEND 分岐) | ✅ (BLEND 分岐時) |
| `class1/deferred/bumpF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ opaque only |
| `class1/deferred/diffuseF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ opaque only |
| `class1/deferred/diffuseIndexedF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ opaque only |
| `class1/deferred/diffuseAlphaMaskF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ alpha MASK (opaque path) |
| `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ alpha MASK |
| `class1/deferred/diffuseAlphaMaskNoColorF.glsl` | ✅ | ✅ | ✅ (defensive) | ❌ alpha MASK |
| `class1/deferred/treeF.glsl` | ❌ (tree は装着不可) | ✅ (forced) | ✅ (defensive、tree は alpha pool 不通過) | ❌ |

**defensive な 12 分岐**: opaque pool 経由 shader は実機で canary 12 を受け取ることはない (alpha pool 経由でない限り 12 は流れない) が、誤って到達した場合に視覚的に発見できるよう全 shader で 12→緑分岐を入れてある。安全側コード。実機検証で canary=12 が opaque shader で観測されたら C++ 側の三項演算が壊れている (= bug)。

### 7.4 実機検証 (2026-05-22 確認)

AYAcanary log + 画面 screenshot で確認できた全 dispatcher (`att=F` = Rez Object 側):

| dispatcher | log タグ | 観測 shader |
|---|---|---|
| `pushBatch` | `[pushBatch]` | Deferred Diffuse / Deferred Diffuse Non-Indexed Alpha Mask / Deferred Fullbright / Deferred FullbrightShiny / Deferred Emissive / Deferred Shadow Alpha Mask / Deferred Tree Shadow |
| `pushUntexturedBatch` | `[pushUntexBatch]` | Deferred Shadow Shader |
| `pushGLTFBatch` | `[pushGLTFBatch]` | Deferred PBR Opaque / PBR Glow |
| `pushUntexturedGLTFBatch` | `[pushUntexGLTFBatch]` | Deferred Shadow Shader |
| `pushBumpBatch` | `[pushBumpBatch]` | Bump Shader / Deferred Bump Shader |
| `materialsPool` | `[materialsPool]` | Material Shader 0 / 2 / 4 / 8 / 12 / 13 |
| `alphaPool` | `[alphaPool]` | Deferred Alpha / Deferred Fullbright Alpha Masking / Deferred Fullbright Alpha Masking Alpha / Material Shader 13 |
| `treePool` (forced) | `[treePool]` | Deferred Tree Shader |

画面確認: 建物・屋根・木の幹・葉・構造物 → 全て真っ黒で出力。装着物 (本人 avatar) は別色 (canary 1〜4) で正しく分類。空・草地 → 通常色 (本 canary 対象外 pool)。

### 7.5 カバー漏れデバッグの順序

1. **地上の何かが黒くならない** → その vobj が踏んでいる dispatcher が §3.1 + §3.2 のいずれにも無い path → grass / particles 等の対象外 pool か、新規 dispatcher の漏れ。`grep drawRange indra/newview/lldrawpool*.cpp` で追加 dispatcher の有無を再確認
2. **空 / terrain / water が誤って黒くなる** → 別 pool (Sky / Terrain / Water) が canary uniform を **misordered に継承** している可能性。これらは別 shader を bind するはずなので、`aya_attachment_canary` uniform が前 pool の値を保持していると起きる。各 shader の canary=11 分岐に `#ifdef` ガードで pool 種別を絞るか、別 pool 入口で uniform を 0 reset する
3. **木 (Linden tree) が黒くならない** → §3.2 の forced uniform set が走っていない、または `treeF.glsl` に canary=11 分岐が無い (§7.3 表を確認)
4. **装着物まで黒くなる** → §3.1 の三項演算が壊れている (`att ? ... : 11` の順序が逆等)。装着物側で `mAttachedToAvatar.notNull()` が誤判定されている可能性 (装着物資料 §1.3 参照)
5. **alpha BLEND の Rez Object (透ける窓ガラス等) が canary=11 でなく装着物 brown (4) になる** → alpha pool の三項演算で Rez Object 側を `: 11` でなく `: 4` で終わらせている誤り

---

## 8. terrain / sky / water / grass — 本 canary 対象外 pool

Rez Object とは別 pool で、本 `aya_attachment_canary` を経由しない。canary=11 化したいかどうかは別判断 (現状は **対象外**)。

| Pool | 用途 | shader 例 | LLDrawInfo 経由? | 本 canary 経由? |
|---|---|---|---|---|
| `LLDrawPoolSky` | 大気球 | `class*/deferred/skyF.glsl` / `cloudsF.glsl` | ❌ | ❌ |
| `LLDrawPoolWLSky` | WindLight 雲・星 | `class*/deferred/cloudsF.glsl` 等 | ❌ | ❌ |
| `LLDrawPoolTerrain` | SIM 地面 (heightmap + splat texture) | `class*/deferred/terrainF.glsl` 等 | ❌ | ❌ |
| `LLDrawPoolWater` | 海面 | `class*/deferred/waterF.glsl` | ❌ | ❌ |
| `LLDrawPoolWaterExclusion` | 水抜け穴 (parcel water exclusion) | water exclusion shader | ❌ | ❌ |
| `LLDrawPoolGrass` | Linden grass (草) | `class*/deferred/treeF.glsl` 系? (要確認) | LLDrawInfo 経由する場合あり | ❌ (現状未配線) |

これらを canary 化したい場合は、各 pool の `renderDeferred` 内で shader 全体に **forced** uniform を流す (tree pool 方式) のが現実的。値は 5〜10 の予約値域から選定 (例: terrain=5, sky=6, water=7)。

---

## 9. Rez Object 透過 path = r30 透過 DoF C 案の出発点

「Rez Object のうち alpha BLEND で描かれる面」は、装着物の茶 path (装着物資料 §9) と **同じ構造的問題** を持つ:

- **DoF が透過部だけ効かない / 誤った距離でぼける**
- SSAO / SSR / 反射プローブが透過部で抜ける
- Z-order の崩れ
- depth write しないので後段 post-process pass が depth buffer 上「そこに何もない」扱い

Rez Object 例:
- 透ける窓ガラス (建物)
- 葉先 (mesh 製品の植物)
- 透過装飾 (lace / chiffon の家具・カーテン)
- 半透明 light fixture (照明シェード)
- 水槽の水 (alpha BLEND mesh で水を表現するもの — 本物の water pool とは別)

### 9.1 透過 DoF C 案 (Phase 2 で進行中) との関係

r30 P5 で議論した **C 案 (alpha BLEND を別 RT に分離)** は、装着物側 (茶 = canary=4) と Rez Object 側 (緑 = canary=12) **両方を同時にカバー** する必要がある:

- 別 RT (新 framebuffer attachment) に alpha BLEND の depth + color を分離書込み
- post-process DoF / SSAO pass で「opaque depth + alpha depth の合成」を読んで blur 半径を補正
- 装着物の茶 path と Rez Object の alpha BLEND path は **`LLDrawPoolAlpha::renderAlpha` で同じ dispatcher** を経由するので、別 RT 分離は両方を 1 つの仕組みで処理できる
- canary uniform は alpha pool dispatcher で `att ? 4 : 12` の分岐があるので、shader 側で `aya_attachment_canary == 4 || aya_attachment_canary == 12` で「透過 path 全部」を識別可能 (= 装着物の茶と Rez Object の緑をまとめて C 案の対象とする)

### 9.2 C 案実装前の必須トレース (装着物資料 §9.2 を Rez Object に拡張)

装着物資料 §9.2 と同じトレース手順を Rez Object alpha BLEND 面でも実施。追加で:

1. **対象が Rez Object の alpha BLEND (緑) か装着物の茶か確認** — canary=4 (茶) と canary=12 (Rez Object 緑) を画面で区別。両方が同じ場所に重なって描かれているケース (例: 装着物の透ける髪が地上の透ける窓ガラス越し) は **2 段透過** で更に複雑 → C 案でも特殊取扱いが必要
2. **同じ alpha pool でも shader が複数系統** — 実機 log で観測される 4 種 (§10.8 で詳細)、すべて canary=12 分岐済 (§7.3)
3. **alpha pool の sort 順** — Rez Object の透過面と装着物の透過面が距離ソートで混ざる。複数 SIM-rezzed alpha 面と複数 attachment alpha 面の前後関係はソート 1 本に統一されているので、別 RT 分離時に「装着物のみ別 RT」「Rez Object のみ別 RT」のような部分分離はソート順を壊す → **alpha BLEND は全部まとめて 1 つの別 RT に分離するのが整合性高い**

### 9.3 C 案修正に着手する前のチェックリスト

装着物資料 §9.4 と同じチェックリストを Rez Object 側でも実施。加えて:

- [ ] 対象画素が `att=F`/canary=12 (Rez Object 透過 = 緑) か `att=T`/canary=4 (装着物透過 = 茶) かを canary で識別済
- [ ] 同じ症状が両方で起きているか / Rez Object 側だけ / 装着物側だけ かを分離済
- [ ] alpha pool の sort 順を C 案の RT 分離設計が壊さない (= 装着物 + Rez Object を 1 つの alpha RT にまとめる) ことを設計レベルで確認済

---

## 10. SIM-rezzed alpha BLEND 描画経路 完全トレース (canary=12 視点)

「地上に Rez された透過マテリアル (= 緑で塗られた面)」が **どの C++ → GLSL path を辿って画面に出るか** の確定マップ。装着物の茶 path (装着物資料 §9) と alpha pool の dispatcher を **共有** するので、装着物資料との対比で読むと差分が浮く。

### 10.1 1 行サマリ

```
LLVOVolume::genDrawInfo (PASS_ALPHA 振分)
  → LLDrawPoolAlpha::renderPostDeferred (POST_WATER / PRE_WATER の 2 sub-pool)
    → forwardRender(rigged=false) → forwardRender(rigged=true)
      → renderAlpha (per-draw shader 選択 + canary=12 set + drawRange)
        → 4 種のいずれかの alpha shader (§10.8)
          → frag_data[0] = 緑、frag_data[1/2/3] = 0、depth write **なし**
            → softenLightF を bypass、postDeferred 系で depth 参照 → 透過部だけ "そこに何もない" 扱い (= C 案の動機)
```

### 10.2 エントリ判定 (どこで PASS_ALPHA に入るか)

`LLVOVolume::genDrawInfo` (llvovolume.cpp:6981 周辺) で **face 単位** に判定。SIM-rezzed object も装着物も同じ logic を通る (差は §1.3 と最終 dispatcher のみ):

| 判定経路 | 条件 | registerFace pass | 入る pool |
|---|---|---|---|
| GLTF alpha mode = BLEND | `gltf_mat->mAlphaMode == ALPHA_MODE_BLEND` | `PASS_ALPHA` | LLDrawPoolAlpha |
| Blinn-Phong fullbright + MASK + transparent | `te->getFullbright() && mat MASK + !blinn_phong_opaque` | `PASS_ALPHA` | LLDrawPoolAlpha |
| Blinn-Phong fullbright + is_alpha | `te->getFullbright() && is_alpha` | `PASS_ALPHA` | LLDrawPoolAlpha |
| Blinn-Phong material + alpha (env/shiny 無し + 透明) | mat 経由で 6981+ 分岐 | `PASS_ALPHA` (//PASS_MATERIAL_ALPHA 等は alias) | LLDrawPoolAlpha |
| Legacy texture + alpha | `te->getColor().a < 0.999` または `getPoolType() == POOL_ALPHA` | `PASS_ALPHA` | LLDrawPoolAlpha |
| Particle | particle 系 | `PASS_ALPHA` | LLDrawPoolAlpha (但し `disable_cull` ON) |

`is_alpha = (facep->getPoolType() == LLDrawPool::POOL_ALPHA) || (te->getColor().mV[3] < 0.999f)` (llvovolume.cpp:5713 / 6981) が起点。`is_alpha |= blinn_phong_transparent` で legacy 透明色 (color.a < 0.999) も拾う。

**Rez Object 側だけが入る訳ではない** — 装着物の茶 path (= canary=4) も全く同じ判定を通り、`mAttachedToAvatar` 有無は per-draw でしか分からないので、ここでは分離されない。分離は §10.6 の renderAlpha 内 canary set 時に行う。

### 10.3 Pool 振り分け: POOL_ALPHA_PRE_WATER vs POOL_ALPHA_POST_WATER

LLDrawPoolAlpha は **2 sub-pool に分裂**:

| sub-pool | 描く範囲 | water との関係 |
|---|---|---|
| `POOL_ALPHA_PRE_WATER` | 水面より下にある alpha BLEND 面 | water plane の **後ろ** に描く (= water に塗りつぶされる順序) |
| `POOL_ALPHA_POST_WATER` | 水面より上にある alpha BLEND 面 | water plane の **前** に描く (= water 越しに見える順序) |

`above_water` 判定 (lldrawpoolalpha.cpp:764)。`LLPipeline::sUnderWaterRender` 中は反転 (水中視点で「water の前」と「後ろ」が入れ替わる)。spatial group の bbox z extent と `water_height` で reject。

Rez Object の透ける窓ガラス (= 緑) は **どちらにも入りうる** (一つの建物に水面下と水面上の窓が混在しても OK)。

### 10.4 renderPostDeferred 全 flow (lldrawpoolalpha.cpp:80-298)

順序付きで列挙:

| # | C++ 行 | 処理 | 副作用 |
|---|---|---|---|
| 1 | ~110 | `prepare_alpha_shader(emissive_shader / pbr_emissive_shader / fullbright_shader / simple_shader / materialShader[SHADER_COUNT] / pbr_shader)` を全部準備 | gamma, waterSign, minimum_alpha, rigged variant の uniform をまとめて set |
| 2 | 200 周辺 | C-(a): `if (use_alpha_rt) gPipeline.mAYAAlphaColor.bindTarget(); mForwardToAlphaRT = true;` | 以降の forward write は alpha 専用 color RT に redirect (depth は main FBO 共有) |
| 3 | 210-270 | `forwardRender(rigged=false)` → `forwardRender(rigged=true)` (use_alpha_rt 有時は順序逆 — 建物窓越しに hair gap が見える regression 修正) | drawRange を 2 周 |
| 4 | 280 周辺 | `mForwardToAlphaRT = false`; `mAYAAlphaColor.flush()` | 通常 RT に戻す |
| 5 | ~310 | Cinematic 時 DoF depth prepass at cutoff (`RenderDepthOfFieldAlphas` false → cutoff=1.0 = 完全 skip) | 透過面を **DoF 用 depth に書き戻し**、blur 半径補正の素材 |
| 6 | ~340 | L2-β 第二 depth prepass at cutoff 0.5 → `gPipeline.mAYAAlphaDepth` | 「ガラス・レース・葉先」など中〜高 alpha は depth 残し、低 alpha は discard。hair-style の超低 alpha は捨てる |
| 7 | renderDebugAlpha (デバッグ time のみ) | 透過面を highlight 描画 | 通常 build では off |

### 10.5 forwardRender の state setup

`forwardRender(rigged)` (lldrawpoolalpha.cpp:~440-510) は以下を有効化:

| GL state | 値 | 役割 |
|---|---|---|
| `LLGLSPipelineAlpha` | 既製 state 一式 | depth test ON / depth write OFF / blend ON |
| `gGL.blendFunc(srcF, dstF, sAlpha, dAlpha)` | 通常時: color = `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`、alpha = `ZERO, ONE` (画面側 α 触らず) | 普通の透過合成 |
| `mForwardToAlphaRT == true` 時 | alpha factor が変わる (C-(a) RT 側に α 寄与を蓄積) | 別 RT 蓄積用 |
| `setSceneBlendType(BT_ALPHA)` | 最終戻し | 終了時に標準 BLEND に復帰 |

**depth write は OFF** — これが C 案の動機の根源。透過面は depth buffer に書かれないので、postDeferred 段 (SSAO / SSR / DoF / 反射プローブ等) の depth 参照では「**そこに何もない**」扱いになる。

### 10.6 renderAlpha 内の per-draw shader 4 系統選択 (lldrawpoolalpha.cpp:736-1107)

spatial group → drawmap → 各 `LLDrawInfo& params` ループ。**shader 選択優先順位** (lldrawpoolalpha.cpp:847-910):

```
gltf_mat && gltf_mat->mAlphaMode == ALPHA_MODE_BLEND
    → target_shader = pbr_shader (gDeferredPBRAlphaProgram)        ─── (A) PBR
else if (LLPipeline::sRenderingHUDs)
    → target_shader = fullbright_shader                            ─── (B) fullbright (HUD)
else if (mat)
    → target_shader = &gDeferredMaterialProgram[params.mShaderMask] ─ (C) legacy Material (16 variant)
else if (params.mFullbright)
    → target_shader = fullbright_shader                            ─── (B) fullbright
else
    → target_shader = simple_shader                                ─── (D) simple

最後に: if (params.mAvatar != nullptr) target_shader = target_shader->mRiggedVariant
```

bind は `gPipeline.bindDeferredShaderFast(*target_shader)` (lldrawpoolalpha.cpp:858 / 915)。rebind は shader が変わる時のみ (不要な state thrashing 回避)。

### 10.7 canary=12 set 位置 (lldrawpoolalpha.cpp:983-985)

shader bind → texture/material setup → blendFunc set の直後、`drawRange` の直前に挿入:

```cpp
static LLStaticHashedString s_aya_attachment_canary("aya_attachment_canary");
const bool att = params.mAttachedToAvatar.notNull();
const int  cval = att ? (params.mIsBoMBodyOrHead ? 1 : (params.mIsPrim ? 3 : 4))
                      : 12;  // ← Rez Object alpha BLEND は 12 = 緑
current_shader->uniform1i(s_aya_attachment_canary, cval);
```

| `params.mAttachedToAvatar` | `params.mIsBoMBodyOrHead` | `params.mIsPrim` | cval | 画面色 |
|---|---|---|---|---|
| notNull | true | — | **1** | magenta (BoM body/head) |
| notNull | false | true | **3** | gray (legacy prim 装着物) |
| notNull | false | false | **4** | brown (mesh 装着物 alpha BLEND) |
| **isNull** | — | — | **12** | **green (Rez Object alpha BLEND)** |

opaque pool (Simple / Bump / Material / PBROpaque) は `att=false` で `: 11` を流すので、**alpha pool だけが 12 を出す唯一の dispatcher**。検証で 12 が opaque 系 shader で観測されたら C++ 側 bug。

### 10.8 fragment 段で緑が出る shader (実機 log 観測 4 種)

実機検証 (2026-05-22) で `[alphaPool] canary=12 att=F` が出た shader 名と GLSL ファイル:

| 観測 shader 名 | 対応 GLSL | 主な対象 face |
|---|---|---|
| `Deferred Alpha Shader` | `class2/deferred/alphaF.glsl` | legacy Blinn-Phong 透過 mesh / prim (simple_shader 経由) |
| `Deferred Fullbright Alpha Masking Shader` | `class1/deferred/fullbrightF.glsl` (の派生) | fullbright + alpha BLEND |
| `Deferred Fullbright Alpha Masking Alpha Shader` | `class1/deferred/fullbrightF.glsl` (rigged 等 variant) | 同上の variant |
| `Deferred PBR Alpha` (内部名) | `class2/deferred/pbralphaF.glsl` | GLTF material `ALPHA_MODE_BLEND` |

別途 `class3/deferred/materialF.glsl` も legacy Material path で受ける可能性あり (mat → gDeferredMaterialProgram[mShaderMask]、SHADER_COUNT=16 variant)。**全 12 file に canary=12 → 緑分岐は注入済** (§7.3) なので、ルーティングがどう変わっても緑が出るはず。

各 shader の canary=12 分岐は共通形式:

```glsl
if (aya_attachment_canary != 0) {
    vec3 canary_rgb = ... ((aya_attachment_canary == 12) ? vec3(0.0, 1.0, 0.0) : ...) ...;
    frag_data[0] = vec4(canary_rgb, 0.0);   // ← alpha=0 で sky 真っ白防止 (memory: project_aya_visual_realism_alpha_protect)
    frag_data[1] = vec4(0.0);
    frag_data[2] = encodeNormal(normal, 0.0, GBUFFER_FLAG_SKIP_ATMOS);  // ← softenLightF bypass
#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(canary_rgb, 0.0);
#endif
    return;  // ← 通常 path 完全 skip
}
```

forward alpha BLEND path (alphaF / pbralphaF / materialF の forward 出力) では更に `frag_color.a` を canary_a で 1.0 fill する分岐も入れて、alpha 合成で緑が薄れないようにしている (§7.2 末尾参照)。

### 10.9 depth write なし → post-process 影響 = C 案の動機

`LLGLSPipelineAlpha` で **depth write OFF**。結果として:

| post-process pass | 透過面の depth 扱い | 症状 |
|---|---|---|
| DoF (CoC 計算) | "そこに何もない" → 後ろの opaque depth を参照 | 透過部だけぼけが効かない、または誤った距離でぼける |
| SSAO | 隣接 depth 参照不可 | 透過面の縁で AO が抜ける |
| SSR (screen-space reflection) | depth 参照不可 | 透過面に反射が乗らない / 抜ける |
| 反射プローブ depth match | 透過面が無視される | プローブ blend が変 |

**だから C 案 (= 透過専用の depth + color RT を別建てして post-process で合成参照する)** が必要。L2-β 第二 depth prepass (cutoff 0.5) は **暫定対策** で、ガラスや葉先のような「α>0.5」面は depth に書き戻すが、α<0.5 (hair / lace の薄い部分) は discard で救えない。C 案は depth/color の **完全分離** で根本解決を狙う。

### 10.10 検証時の最短再現手順

1. AYAstorm 起動、地上にあるガラス窓・葉先・透ける装飾を視界内に入れる
2. Debug Settings で **`RenderTreeChannels` 系の canary kill-switch が無いこと確認** (現状はそもそも switch 無し、永続化された canary uniform は viewer 終了で消える)
3. AYAstorm 起動した状態で「緑」が見える = canary=12 path 生存
4. AYAstorm log で `[alphaPool] shader=... canary=12 att=F` を grep — 1 種以上の shader 名が出ていれば実 dispatcher 生存
5. 緑が出ない透過 (例: 特殊 particle / hair の超低 alpha) があれば §10.8 の 4 種以外で別 shader 経由 → log で shader 名を確認して §7.3 の表に追加判定

---

## 11. 二重アルファブロック (Double Alpha Block) — 全 SL viewer 共通の描画 bug + 修正

**作成日**: 2026-05-22 (案 A 適用 + 実機 verify PASS。disclosure 用 special branch `fix/double-alpha-block` の reference 資料)

### 11.1 適用範囲 — 全 viewer family 共通

このバグは **AYAstorm 固有ではなく**、LL upstream viewer から派生した **全 fork** に存在する pipeline 構造由来の bug:

- **LL official viewer** (`indra/newview/lldrawpoolalpha.cpp`)
- **Firestorm** (本 fork の上流)
- **Alchemy**
- **BlackDragon**
- 他 LL viewer 派生 fork 全般

理由: 二重アルファブロックの原因コード (`write_depth = rigged ||` および「rigged → non-rigged」順の forward render) は **LL upstream に元から存在** し、各 fork は同 logic を継承しているため。再現は viewer 種別を問わず DoF OFF (もしくは use_alpha_rt 系統の代替分離 path が無い) 状態で attachment alpha BLEND (hair / clothing) と Rez Object alpha BLEND (窓ガラス / lace / 葉先) が同画素で重なれば 100% 発生する。

各 viewer maintainer は本資料 + `fix/double-alpha-block` branch を Pull して **自由に取り込んで構わない** (PR 化はしない — 来たければ来い方針)。

### 11.2 現象の定義

**二重アルファブロック (double alpha block)**:
alpha BLEND face A (手前 = 髪 / 衣服) が depth write した後、その背後にある alpha BLEND face B (奥 = 窓ガラス / lace / 葉先) が **depth test で reject されて fragment shader 自体が実行されず**、最終画素が opaque 段の sky にすり替わる現象。

| canary 状態 | A (髪) の領域 | A 以外の領域 |
|---|---|---|
| canary OFF (通常 path) | **sky / 木の枝が透ける** = B が描かれていない | B が薄く合成されて見える |
| canary ON (alpha BLEND を強制 α=1 化) | 緑で埋まる = B の fragment は走っている (= depth reject の証明) | 同上 |

### 11.3 根本原因 — `write_depth = rigged` + rigged-first order の組合せ

**コード A** — `indra/newview/lldrawpoolalpha.cpp::forwardRender(bool rigged)`:

```cpp
bool write_depth = rigged ||                      // ← rigged (= attachment) は無条件 depth 書き込み
    LLDrawPoolWater::sSkipScreenCopy ||
    LLPipeline::sImpostorRenderAlphaDepthPass ||
    getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);
```

**コード B** — `renderPostDeferred` の元順 (LL/Firestorm/Alchemy/BD 全部):

```cpp
forwardRender(true);   // ① rigged (attachment alpha BLEND) 先 → 共有 depth に z 書き込み
forwardRender();       // ② non-rigged (Rez Object alpha BLEND) 後 → ①の z で reject
```

**結果の連鎖**:
1. forwardRender(true) で hair (alpha BLEND) を描く + depth 書き込み (write_depth=true)
2. forwardRender(false) で window (alpha BLEND) を描こうとする
3. window の z > hair の z (奥にあるため) → GL_LEQUAL で reject → **fragment shader 呼ばれず**
4. 画素は opaque 段で書かれた sky のまま → 「**sky に抜ける**」

### 11.4 既存の AYAstorm 限定 mitigation (= 部分修正)

AYAstorm では `r30 P5 transparent-DoF C-(a)` で別 RT 分離 (`mAYAAlphaColor`) を導入した時に **同じ regression** を一度認識し、`use_alpha_rt = true` の限定 path で order swap を仕込んでいた (lldrawpoolalpha.cpp の AYA コメントに「`building-windows-through-hair-gap regression AYA observed`」と既述):

```cpp
const bool use_alpha_rt =
    ... && LLPipeline::RenderDepthOfField &&        // ← DoF OFF だと false → swap 無効
    ... && getType() == POOL_ALPHA_POST_WATER &&
    gPipeline.mAYAAlphaColor.isComplete();
if (use_alpha_rt)
{
    forwardRender();        // non-rigged 先 (swap order)
    forwardRender(true);    // rigged 後
}
else
{
    forwardRender(true);    // ← 元順、ここで二重アルファブロックが残存
    forwardRender();
}
```

= **DoF OFF / probe 描画 / impostor 描画 / cube snapshot** など `use_alpha_rt = false` の path では依然として再現していた。

### 11.5 修正 (案 A) — POST_WATER 全 path で swap order を default 化

**修正後** — `renderPostDeferred` (lldrawpoolalpha.cpp:240-272):

```cpp
if (!LLPipeline::sRenderingHUDs &&
    getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
{
    // back-to-front: non-rigged (background — windows / foliage) 先 →
    // rigged (foreground — hair) 後
    forwardRender();          // ① non-rigged (Rez Object alpha BLEND)
    forwardRender(true);      // ② rigged (attachment alpha BLEND) — opaque depth のみと test
}
else
{
    // PRE_WATER / HUD は元順維持 (water fog 計算整合性)
    if (!LLPipeline::sRenderingHUDs) forwardRender(true);
    forwardRender();
}
```

### 11.6 修正後の動作

| 段階 | 操作 | depth 状態 | 結果 |
|---|---|---|---|
| ① | non-rigged 描画 (write_depth=false on POST_WATER) | opaque depth のまま | window が opaque z で test → 普通に描ける |
| ② | rigged 描画 (write_depth=true) | rigged depth が書き加わる | hair が opaque depth + 自分の z で test → window の上に over-blend |

**副作用**: 無し (POST_WATER 限定、PRE_WATER の water fog 計算は元順維持、HUD は forwardRender 1 回のみで対象外)

### 11.7 実機 verify (2026-05-22)

- 環境: AYAstorm Cinematic mode + `RenderDepthOfField = false` (use_alpha_rt = false に強制)
- 場所: 髪 (attachment alpha BLEND) 越しに窓ガラス (Rez Object alpha BLEND) が見える地点
- canary ON 状態:
  - **修正前** (screenshot `2026-05-22 01-32-21.png`): 髪のシルエット領域で緑が消え、sky / 木の枝が透ける
  - **修正後** (screenshot `2026-05-22 01-44-13.png`): 髪のシルエット全体が緑で埋まる = window が髪より先に描けている

### 11.8 他 viewer に取り込む側へのガイダンス

1. 単純な 2 行修正で済む (forwardRender の呼び出し順序を POST_WATER 限定で入れ替えるだけ)
2. PRE_WATER は触らない (water fog 計算のため rigged-first depth 書きが必要)
3. HUD は forwardRender 1 回なので関係無し
4. 副作用検証: hair の z が必要な系統 (impostor / shadow pass / cube snapshot) は **forwardRender とは別経路** で depth 書きを持っているはずなので、forward 内 swap だけで完結する
5. 自 fork に既に C-(a) 相当の別 RT 分離があるなら、その path だけ swap → 残り path で抜けが残るため **全 path で default 化** を推奨

### 11.9 別解 — 採用しなかった案

| 案 | 説明 | 不採用理由 |
|---|---|---|
| B: `write_depth = rigged` から rigged を外す | rigged も depth 書かない | L2-β prepass 等で rigged depth を別途供給する必要、影響範囲広い |
| C: alpha 完全別 RT + 別 depth | depth を opaque と alpha で完全分離 | 数日工数。AYAstorm `#275` で進行中。案 A で可視症状は消えるが、透過の DoF/SSAO/SSR 正しさは C 案の動機として残る |

---

## 12. 関連リファレンス

- `ayastorm-attachment-rendering-routing.md` — **本書の対称版** (`mAttachedToAvatar.notNull()` 側の地図)。1 つの uniform で両側分類するので両書併読推奨
- `ayastorm-deferred-shader-routing.md` — gbuffer flag → softenLightF 分岐の対応 (lit 計算側)
- `ayastorm-gbuffer3-trace.md` — gbuffer3 storage (alpha channel 仕様)
- `ayastorm-attachment-magenta-canary-trace.md` — 装着物 canary 実装の trace
- `ayastorm-r30-p4-bd-dof-chain-trace.md` — r30 P4 BD DoF chain trace (本書 §11.9 C 案の文脈)
- **special branch `fix/double-alpha-block`** — §11 の disclosure 用 reference (色付き検証状態を意図的に永続化)
