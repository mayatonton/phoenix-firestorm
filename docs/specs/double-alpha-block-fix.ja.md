> **Language / 言語 / 语言**: [English](./double-alpha-block-fix.md) · **日本語** · [中文](./double-alpha-block-fix.zh.md)

# 二重アルファブロック — 全 SL viewer 共通の描画 bug と 2 行修正

**ステータス**: AYAstorm では修正済み。LL viewer 派生 fork からの自由取込を歓迎します — PR は不要、必要なところだけ持っていってください。

**Reference branch**: `mayatonton/phoenix-firestorm` の [`fix/double-alpha-block`](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block)。HEAD は本資料の最新リビジョンを追随しますが、§5 の修正本体は初出 commit から変わっていません。**検証スナップショット** (§8 用 canary 状態) は commit `2597b657ac` に永続固定 — `git checkout 2597b657ac` で色付き検証フレームを再現できます。

**詳細トレース**: [`docs/specs/ayastorm-rez-object-rendering-routing.md`](./ayastorm-rez-object-rendering-routing.md) §11 に canary 検証と dispatcher 地図の全貌があります

---

## 1. TL;DR

post-water alpha pool で forward alpha BLEND を描く際、**rigged 装着物 (髪・衣服) が先に描かれ共有 depth buffer に z を書き込みます**。その背後にある non-rigged alpha BLEND 物体 (窓ガラス・lace・葉) は **fragment shader が走る前に depth reject** され、最終画素は opaque pass で書かれた sky に戻ります。

修正は `LLDrawPoolAlpha::renderPostDeferred` の 2 つの forward pass の順序を入れ替え、non-rigged を先 (奥) → rigged を後 (手前) にするだけ。**`POOL_ALPHA_POST_WATER` 限定**。PRE_WATER と HUD は触りません。

実質 2 行の差分。新規 uniform も新規 render target も、shader 編集も不要。

## 2. 影響を受ける viewer

このバグは **AYAstorm 固有ではありません**。原因コード — `write_depth = rigged || ...` と rigged-first forward 順の組合せ — は Linden Lab の upstream viewer に元から存在するため、LL upstream 派生 viewer 全般が同じ code path を継承しています。

再現は viewer 種別に依存しません。Depth-of-Field OFF (または viewer 内部の alpha-RT 分離 path が走っていない条件) で、髪装着物の手前に透明な窓ガラス・lace・葉などの Rez Object を置けば、髪のシルエットを sky/雲が透けます。

## 3. 現象

カメラが髪装着物 (rigged alpha BLEND) を装備したアバターを写し、その髪の **背後に** non-rigged alpha BLEND オブジェクト — 窓ガラス、lace の布、葉、particle — がある時、髪のシルエット部分に **opaque pass で書かれた sky** が見え、本来見えるべきはずの背後オブジェクトが消えます。

最も分かりやすいのは室内で窓越しの風景を髪越しに見る構図: 髪のエッジで窓が単純に消失します。

| 修正前 (通常表示) | 修正後 (通常表示) |
|:---:|:---:|
| ![修正前](./images/double-alpha-block/before-normal.png) | ![修正後](./images/double-alpha-block/after-normal.png) |
| 髪のシルエットに本来あるべき窓の格子が無い — 髪の occlusion 領域で sky / 木の枝が透けてしまっている | 髪のシルエット越しに窓の格子が正しく見える |

## 4. 根本原因

### 4.1 depth-write のルール

`indra/newview/lldrawpoolalpha.cpp::forwardRender(bool rigged)`:

```cpp
bool write_depth = rigged ||
    LLDrawPoolWater::sSkipScreenCopy ||
    LLPipeline::sImpostorRenderAlphaDepthPass ||
    getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);
```

`write_depth` は rigged の場合無条件で true。rigged alpha BLEND は共有 depth buffer に z を書きます。

### 4.2 Forward render の順序 (upstream)

`renderPostDeferred` (LL upstream 派生 viewer で共通):

```cpp
if (!LLPipeline::sRenderingHUDs)
{
    // first pass, render rigged objects only and render to depth buffer
    forwardRender(true);   // ① rigged 先 — depth を書く
}

// second pass, regular forward alpha rendering
forwardRender();           // ② non-rigged 後 — ①の depth で reject される
```

### 4.3 連鎖

1. ① で髪 (rigged alpha BLEND) を描く。`write_depth = true` なので髪の z が共有 depth buffer に乗る。
2. ② で窓 (non-rigged alpha BLEND) を描こうとする。窓は髪より奥にあるので 窓 z > 髪 z。
3. `GL_LEQUAL` が窓 fragment を **fragment shader 起動前に** reject。blend も color write も無し。
4. 画素は opaque pass で書かれたまま — 通常は skybox。

結果: 髪のシルエット内で窓が消える。**二重アルファブロック (double alpha block)** です。

## 5. 修正

`renderPostDeferred` — **`POOL_ALPHA_POST_WATER` 限定で** 順序を入れ替え:

```cpp
if (!LLPipeline::sRenderingHUDs &&
    getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
{
    // back-to-front: non-rigged (背景) 先 → rigged (前景) 後
    forwardRender();        // ① non-rigged (Rez Object alpha BLEND)
    forwardRender(true);    // ② rigged (装着物 alpha BLEND) — 背景の上に over-blend
}
else
{
    // PRE_WATER / HUD: 元順維持 (water fog 整合性のため)
    if (!LLPipeline::sRenderingHUDs)
    {
        forwardRender(true);
    }
    forwardRender();
}
```

## 6. なぜ動くのか

| 段階 | 動作 | 開始時の depth | 結果 |
|---|---|---|---|
| ① | non-rigged 描画 (POST_WATER では `write_depth = false`) | opaque のみの depth | 窓は opaque z と test、fragment 走る、color が blend される |
| ② | rigged 描画 (`write_depth = true`) | opaque depth + 窓の z は **書かれていない** | 髪は opaque z だけと test、fragment 走る、窓の上に blend |

POST_WATER 上の ① は depth を **書かない** (depth 書くのは rigged だけ、そして rigged は ②) ため、窓が後続の髪 fragment を block することもありません。両面が back-to-front の正しい順序で描かれます。古典的な painter's algorithm の alpha 合成です。

## 7. 副作用

観測なし。

- **PRE_WATER** は元の rigged-first 順を維持。`write_depth` は PRE_WATER 下では無条件 true (上記 OR の `POOL_ALPHA_PRE_WATER` 項) で、下流の water fog pass が rigged depth の存在に依存しています。ここに触れると water 描画が変わります。
- **HUD** は `forwardRender()` 1 回のみ。順序入れ替えは無関係。
- **Impostor / shadow / cube snapshot** は rigged depth を別 code path (`sImpostorRenderAlphaDepthPass`、shadow pass 専用の forward 呼出) で取得しており、`renderPostDeferred` 経由ではありません。forward 内の swap はこれらに影響しません。
- **DoF / SSAO / SSR** の透過面に対する正しさは **別の、より深い問題** です。alpha pool 全体で depth-write を OFF にしている `LLGLSPipelineAlpha` 由来。上記修正はそこには手を付けません — fragment shading が alpha-on-alpha で隠される現象を止めるだけです。**DoF** については AYAstorm 側で C 案別 RT 経路として後日解決済み (§9 参照)。SSAO / SSR / 反射プローブは未解決のまま。

## 8. 検証方法

最速の再現手順:

1. 髪装着物 (rigged alpha BLEND) を、透明・半透明の Rez Object — 窓のある壁、lace のカーテン、葉プリム — の手前に配置
2. Depth-of-Field を OFF にする (あるいは viewer 側の alpha-RT 分離 path が走らない設定で)
3. 髪のシルエットを見る
   - **修正前**: 髪越しに sky や遠景が透ける — 窓 / lace が消えている
   - **修正後**: 髪越しに窓 / lace が見える、正しく blend されている

より強い証拠が欲しい場合は、alpha BLEND fragment を既知の色 (例: Rez Object alpha BLEND 出力を緑に塗る) に強制して、修正後に髪のシルエットが緑で埋まることを確認。AYAstorm が使った canary protocol (`aya_attachment_canary == 12 → 緑`) は `ayastorm-rez-object-rendering-routing.md` §10–§11 にあります。

| 修正前 (canary ON) | 修正後 (canary ON) |
|:---:|:---:|
| ![修正前 canary](./images/double-alpha-block/before-canary.png) | ![修正後 canary](./images/double-alpha-block/after-canary.png) |
| 髪 (magenta canary) が背後の緑 (Rez Object alpha BLEND) を occlude。髪のシルエット内に残る黒は opaque pass の背景 = 緑 fragment が走らなかった証拠 | 髪のシルエット全体が緑で埋まる。Rez Object alpha BLEND が髪より先に描かれ、その上に髪 (magenta) が blend されている |

## 9. この修正で **解決しない** もの

このパッチは前景の rigged alpha BLEND による背景の alpha BLEND fragment の *可視の occlusion* を止めます。alpha BLEND geometry が後続 post-process pass から見えないという、より一般的な問題は解決しません:

| post-process | alpha BLEND 面に対して見える depth | 結果 | AYAstorm 対応状況 |
|---|---|---|---|
| Depth-of-Field (CoC) | 面の奥にある opaque z を見る | 透過面が focus 距離を無視する | **✅ 解決済 (2026-05-22、§5 とは別 commit)** — §9.1 参照 |
| SSAO | 隣接 depth 無し | 透過面の縁で AO が抜ける | ⏳ 未着手 (本 branch 対象外) |
| SSR | depth 無し | 透過面に / を通って反射が落ちる | ⏳ 未着手 (本 branch 対象外) |
| Reflection probe blend | 面が無視される | probe blending がずれる | ⏳ 未着手 (本 branch 対象外) |

いずれの構造的解決も、alpha BLEND pool を別の color (理想的には depth も別) attachment にレンダーして、post-process 段で合成し直すこと。AYAstorm ではこれを **C 案** (`mAYAAlphaColor` 別 color RT + `mAYAAlphaDepth` cutoff 0.5 alpha-aware depth + `dofCombineF` の over-blend) として進めています。§5 の修正は C 案に **orthogonal** — occlusion 修正だけ欲しい方は §5 単独で alpha RT 分離に触れず取れます。

### 9.1 C 案 — DoF first-class 配線 (AYAstorm 限定、2026-05-22)

AYAstorm の `experiment/ayastorm-layered-dof` branch 上で C 案が **DoF について端から端まで配線** されました。装着物 alpha BLEND (髪・服) と Rez Object alpha BLEND (窓ガラス・葉先・lace・particles) の両方が、`CameraFNumber` / `CameraFocalLength` / `CameraMaxCoF` に opaque と同じように応答します。

パイプライン (`indra/newview/app_settings/shaders/class1/deferred/`):

| pass | 入力 | 出力 | 役割 |
|---|---|---|---|
| `cofF.glsl` | `mAYAAlphaDepth` (alpha-aware) | `mRT->deferredLight` (.rgb = src, .a = CoC) | CoC を alpha plate 自身の depth (alpha ≥ 0.5) または bg depth (alpha < 0.5) で計算 |
| `postDeferredHQDoFF.glsl` | `mRT->deferredLight` + scene depth | DoF-blurred scene | opaque scene を CoC に従って blur |
| `dofCombineF.glsl` | DoF result + sharp lightMap + **`mAYAAlphaColor`** | final | DoF-blurred opaque + alpha plate **CoC ベース 12-tap disc gather** で合成 |

キー編集 (`dofCombineF.glsl` の alpha plate over-blend):

```glsl
if (aya_alpha_plate_enabled)
{
    float coc_px = abs(diff.a * 2.0 - 1.0) * max_cof * 4.0;  // HQDoFF と同強度係数
    vec4 plate;
    if (coc_px < 0.75) {
        plate = texture(aya_alpha_plate, vary_fragcoord.xy);  // in-focus は単点 sample
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
    frag_color.rgb = plate.rgb + frag_color.rgb * (1.0 - plate.a);   // 標準 "over"
}
```

`mAYAAlphaColor` の premultiplied color/coverage は uniform-weight box 平均で正しく合成できるので、blur 後でも標準の "over" 公式がそのまま valid です。

**精度の限界**:
- alpha ≥ 0.5 pixel (ガラス、葉、不透明寄りの服): CoC は正確 — `mAYAAlphaDepth` に alpha 自身の z が injection されているため
- alpha < 0.5 pixel (hair tip、lace の edge): CoC は背景 depth fallback、wispy 部分なので blur 質感差はほぼ視認不可

**C 案の取込ノート**:

C 案は render target allocation、alpha pool redirection、depth re-injection、cofF bind 切替、dofCombineF の over-blend gather という複数 commit にまたがる feature です。`fix/double-alpha-block` reference branch には **含まれていません**。透過面の DoF correctness が欲しい方は `ayastorm-rez-object-rendering-routing.md` §10.10 を参照、`experiment/ayastorm-layered-dof` から alpha-RT + dofCombineF の commits を独立に取得してください。

SSAO / SSR / 反射プローブの透過面 correctness は **別 chapter** (設計判断が分岐 — AO/SSR を plate に載せるか、plate 越しの背景に載せるか)。業界全体で未解決寄り。AYAstorm では着手予定なし。

## 10. 取込

最小取込は §5 の swap だけ。`indra/newview/lldrawpoolalpha.cpp` の `LLDrawPoolAlpha::renderPostDeferred` 内で完結しており、他の AYAstorm 変更には依存しません。

reference branch を pull すれば周辺コンテキストが見られます。HEAD は本資料の最新リビジョンを追随 (= 公開資料と同期) しており、コード変更自体は安定。§8 用の **色付き canary 検証状態** は commit `2597b657ac` に永続固定 — 自分の scene で検証フレームを再現したい場合はその commit を checkout してください。実験 branch 側の後続 commit `c454ce0b0f` (この reference branch ではない) で動作中 viewer の canary を通常 texturing に戻しています。

```sh
git remote add ayastorm https://github.com/mayatonton/phoenix-firestorm.git
git fetch ayastorm fix/double-alpha-block
git log -1 ayastorm/fix/double-alpha-block
git show ayastorm/fix/double-alpha-block -- indra/newview/lldrawpoolalpha.cpp

# canary 検証状態を再現:
git checkout 2597b657ac
```

upstream への PR 化の予定はありません。各自の判断でお取りください。

---

## ライセンス

reference branch および本書は Phoenix-Firestorm / Linden Lab viewer と同じ LGPL v2.1 で公開します。
