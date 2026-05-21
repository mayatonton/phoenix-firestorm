# AYAstorm Cinematic DoF — Layered Live + Path-Traced Shot (Architecture Spec, Draft)

**Status**: Draft / Pre-implementation
**Branch**: `experiment/ayastorm-layered-dof`
**Base**: feature/ayastorm-r30-bd-full-port-inventory @ 8e1294ed9c
**Chapter**: r31+ (r30 章スコープ外、experiment branch で試行)
**Author**: AYA + Claude
**Date**: 2026-05-20

## 0. 凍結条件 (rollback path)

このブランチは失敗しても r30 進行に影響しない。失敗判定したら:

```
git checkout feature/ayastorm-r30-bd-full-port-inventory
git branch -D experiment/ayastorm-layered-dof   # 任意
```

r30 branch には `cutoff 0.1` の polish commit (`8e1294ed9c`) が乗っており、実用的な halo 抑制は既に達成済。本 experiment が頓挫しても撮影品質は r30 現状のまま。

## 0.1 設計目標 (改訂)

**「精度最優先」を採用**。AYAstorm Cinematic mode の主用途が **静止撮影** であることを踏まえ、live preview と final shot を別 path で実装する 2 段構成を取る。

| モード | 用途 | アーキテクチャ |
|---|---|---|
| **Live Preview (Cinematic mode 通常)** | 構図合わせ・カメラ動かし | **Tier 2: Layered DoF (3 層)** |
| **High Quality Shot (新トリガ)** | 最終 1 枚撮影 | **Tier 0: Path-Traced DoF (stochastic aperture + TAA accumulation)** |

着手順は **段階的**: Tier 2 を Phase L1〜L4 で完成 → 受入合格なら Tier 0 を Phase L5〜L7 で追加。

## 1. 問題定義

### 1.1 症状
α-blend mesh (典型: 髪 mesh の重ね板) を Cinematic DoF + bokeh で撮影すると、髪の外周 wisp に **背景の bokeh kernel が乗った halo (ピンク・グレーの円弧状 fringe)** が発生する。

### 1.2 根本原因
deferred renderer の DoF post-process は **「1 pixel = 1 depth = 1 bokeh radius」** の単層モデル。α-blend pixel は depth buffer に書かない (ANY depth write はバンド幅とソート不能で破綻するため、これは LL/BD の設計判断)。

結果:
- α-blend pixel の color は forward alpha pass で lit RT に乗る
- DoF post-pass は **その pixel の depth = 背景 depth** で bokeh radius を決める
- 髪 (近距離) の色を、背景 (遠距離) の bokeh radius で smear → halo

### 1.3 現 r30 の暫定対策 (cutoff 0.1)
α-pre-pass で「α > 0.1 の pixel は depth を書く」マスクで近似。**根本ではないが** 90% の visible artifact は消える。残り 10% は wisp 先端 (α < 0.1) で原理的に救えない。

## 2. アプローチ階層

| Tier | 概要 | 精度 | コスト |
|---|---|---|---|
| **0: Path-Traced** | レンズ瞳から jitter ray、stochastic accumulation、多角形 aperture、TAA 蓄積 | 物理光学完全 | 撮影 1 枚 0.5〜2 秒 |
| **1: 全 OIT + 層別** | α 加重 OIT depth、4 層 bucketing、polygonal bokeh | リアルタイム業界 SOTA | +3〜5 ms / +60〜100 MB |
| **2: 3 層 Layered DoF** | near / mid / far の bucketing、各層独立 blur、α 加重 (binary cutoff 廃止) | 90〜95% halo 除去 | +1〜2 ms / +30〜40 MB |
| **3: cutoff 0.1 (r30)** | binary mask 近似 | wisp 先端 halo 残 | 0 (既出) |

**本 spec は Tier 2 (live) + Tier 0 (shot) を採用**。Tier 1 は中途半端 (Tier 2 を超える程ではない & Tier 0 の代替にはならない) のためスキップ。

## 3. Live Preview アーキテクチャ (Tier 2: Layered DoF)

### 3.1 RT 構成 (現状 → 新規)

```
[現状]
mRT->deferredScreen  (gbuffer, depth)
mRT->screen          (HDR scene)
mRT->deferredLight   (lighting + CoC)
mPostPingMap         (DoF source ↔ swap)
mPostPongMap         (DoF target ↔ swap)

[Tier 2 追加後]
+ mNearLayerColor    GL_RGBA16F   near 層 color + α 蓄積 (premultiplied)
+ mNearLayerCoC      GL_RG16F     near 層 CoC + geometry depth
+ mNearLayerBlur     GL_RGBA16F   near 層 blur 結果 (half-res 検討)
+ mMidLayerColor     GL_RGBA16F   mid 層 color + α (optional, 3 層化時)
+ mMidLayerCoC       GL_RG16F     mid 層 CoC + depth (optional)
```

メモリ増分予測 (1920×1080, FP16, 3 層構成):
- near 3 buffer:  ~40 MB
- mid 2 buffer:   ~24 MB
- **合計**: 60〜70 MB

2 層構成 (near + opaque) で着手し、効果不足なら 3 層化。

### 3.2 描画パス順序

```
1. opaque deferred           → mRT->deferredScreen (depth + gbuffer)
2. lighting                  → mRT->deferredLight (lit color)
3. forward alpha (改修)      → MRT 同時書き:
                                  既存 lit RT (色寄与) ← 互換維持
                                  mNearLayerColor (α premultiplied)
                                  mNearLayerCoC (geometry depth → CoC)
4. opaque DoF                → mPostPingMap (既存通り)
5. near DoF (NEW)            → mNearLayerBlur (mNearLayerCoC を使った blur)
6. composite (NEW)           → final
   out_color = blurred_opaque * (1 - near_alpha_blurred) + near_blurred
```

### 3.3 α 加重の徹底 (cutoff binary 廃止)

Tier 2 では「α > cutoff の pixel は depth に書く」という **binary 近似を完全排除**。代わりに:

- 各 α pixel は geometry depth + α 値を **そのまま** mNearLayerCoC に保存
- blur 時に α weight で加算 (α=0.05 の wisp も α=0.95 の塊も両方 in-focus 扱い)
- composite で α でアルファ合成 → wisp 先端は α 弱いので物理的に薄く出る (halo にならない)

これが「cutoff 0.1 → 0.05 → … → 0」の理論的最終形。

### 3.4 Shader 変更

| ファイル | 変更 |
|---|---|
| `cofF.glsl` | 変更なし (opaque 層用) |
| `cofNearF.glsl` (NEW) | near 層用 CoC、geometry depth から CoC を算出 |
| `postDeferredF.glsl` | 変更なし (opaque 層 blur) |
| `postDeferredNearF.glsl` (NEW) | near 層 α 加重 blur |
| `dofCombineF.glsl` | near-layer-aware composite に書き換え |
| `alphaF.glsl` | MRT output を追加 (近層 RT への並行書き込み) |

### 3.5 Cinematic mode gating

`AYAVisualRealismEnabled == 2` (Cinematic) + 新 cvar `RenderLayeredDoF` 両方 ON でのみ層別パス走行。それ以外は現 1-pass DoF (cutoff 0.1) のまま。

## 4. High Quality Shot アーキテクチャ (Tier 0: Path-Traced DoF)

### 4.1 概要
撮影トリガ (例: Shift+F8 or 専用ボタン) で発動。**カメラ瞳から複数本の ray を jitter sampling して shot を accumulate** する。物理レンズの光学を再現する。

```
[擬似コード]
for (sample = 0; sample < TARGET_SAMPLES; sample++)
{
    aperture_uv = stochastic_disk_sample(sample);  // 多角形 aperture も可
    camera.position += aperture_uv * aperture_radius;
    camera.lookAt = focus_point;  // 焦点位置で ray が収束
    render_scene_to(accumulation_buffer, weight=1.0/TARGET_SAMPLES);
}
final = accumulation_buffer;
```

- TARGET_SAMPLES: 64 / 128 / 256 (品質と時間のトレードオフ)
- 静止撮影なので TAA 不要、frame ごとに jitter pattern を変えて accumulation
- 各 sample は完全な scene render → α 透過も正しい depth で blur される
- アパチャ形状 = 絞り羽根数 (5〜9 枚) を polygonal sampling で再現

### 4.2 着手順 (Phase L5〜L7)

L1〜L4 (Tier 2) 完了後、その結果を受けて開始。

### 4.3 既存 BD parity との関係
Tier 0 は **BD には存在しない領域**。AYAstorm 独自の新章 (= "BD を超える" thesis) として明示。

## 5. Phase 分割

### Phase L1 — Tier 2 Prototype (single near layer, blur 無し)
- 新 RT 1 枚 + composite 1 pass で「近層を blur せず単純合成」までやって、流れが死なないことだけ確認
- 目標: 起動して画面が黒くならない、近層 α が見える位置に乗る
- 失敗判定: shader compile / RT allocate / pipeline 全段組み立て不能 → 即 rollback

### Phase L2 — Tier 2 Near-layer blur with own CoC
- `cofNearF.glsl` + `postDeferredNearF.glsl` 実装
- 近層を **その層の geometry depth** で blur
- α 加重 (binary cutoff 完全廃止)
- スクショで halo が r30 cutoff 0.1 より明確に改善しているか比較
- 失敗判定: 改善が観察できない / 別の artifact (smear, bleed) が発生

### Phase L3 — Tier 2 Multi-layer (near / mid / far)
- 2 層では足りない場合に 3 層化を検討
- focus plane を mid に置き、near と far で独立 blur
- L2 で十分なら skip

### Phase L4 — Tier 2 最適化 + UI
- half-res near layer (帯域節約)
- HQ DoF / chroma との合成
- Cinematic Control floater に新 cvar 露出 (slider or checkbox)
- 性能測定 (frame profile で post-process budget)
- **ここまでで一旦 release 可能** (live preview の Tier 2 単独で )

### Phase L5 — Tier 0 Prototype (jitter sampling 基盤)
- camera を jitter する仕組み構築 (modelview matrix offset)
- accumulation buffer (RGBA32F、HDR 蓄積)
- 単純な disk sampling で 32 sample 程度回して画が出ることだけ確認
- 失敗判定: accumulate しても収束しない / 異常な noise / FBO 構成失敗

### Phase L6 — Tier 0 Stochastic Quality
- 多角形 aperture (5〜9 枚絞り) sampling
- Halton / Sobol low-discrepancy sequence で sample 分布最適化
- TARGET_SAMPLES を可変化 (Quick: 64, Standard: 128, High: 256)
- 撮影中の進捗表示 UI

### Phase L7 — Tier 0 統合 + UI
- 撮影トリガ (新キーバインド or floater button)
- 撮影中 cancel 機能
- 出力 PNG への保存
- 「LIVE → SHOT 切替」のスムーズな UX

## 6. 既存実装との接続マップ

trace 結果より:

### 6.1 既存 DoF dispatch
- pipeline.cpp:9956-9962 — `renderDoF(sourceBuffer, targetBuffer)` 呼び出し
- pipeline.cpp:9630- — `LLPipeline::renderDoF` 本体

→ ここに `renderLayeredDoF()` (Tier 2) と `renderPathTracedDoF()` (Tier 0) を分岐させる。

### 6.2 既存 alpha-prepass for DoF depth
- lldrawpoolalpha.cpp:231-258

→ Layered DoF (Tier 2) が ON ならこの depth pre-pass は **無効化** (重複 + 不要)。OFF なら今のまま (cutoff 0.1) で fallback として機能。

### 6.3 既存 forward alpha pass
- lldrawpoolalpha.cpp:261- `forwardRender`

→ ここに MRT output 追加 (`mNearLayerColor + mNearLayerCoC` への並行書き込み)。

### 6.4 既存 shader rebuild gate
- llviewershadermgr.cpp:3022-3046 — `AYAVisualRealismEnabled == 2` の permutation 切替

→ Layered DoF permutation を追加。

### 6.5 既存 BD parity DoF chain
- pipeline.cpp:9764-9835 — CoF / Blur / Combine の既存 3 pass
- BD parity 関連は Phase 4/5 で全て移植済 (HQDoFF, FRONT_BLUR, HAS_DOF_CHROMA, chroma strength)

→ Tier 2 はこの BD chain と共存 (opaque 層は既存 chain、near 層は新 chain)。Tier 0 は完全に別 path。

## 7. 性能予測

### 7.1 Tier 2 (Live Preview)
| 項目 | 増分 |
|---|---|
| GPU メモリ | +60〜70 MB (3 層) / +40 MB (2 層) |
| GPU 時間 | +1〜2 ms / frame (full-res) / +0.5 ms (half-res near) |
| シェーダ compile | 新 3 shader |

### 7.2 Tier 0 (High Quality Shot)
| 項目 | 数値 |
|---|---|
| 撮影時間 (Quick 64 sample) | 約 0.3〜0.5 秒 |
| 撮影時間 (Standard 128) | 約 0.6〜1.0 秒 |
| 撮影時間 (High 256) | 約 1.2〜2.0 秒 |
| GPU メモリ (accumulation) | +30 MB (RGBA32F, 1920×1080) |
| live フレームレート影響 | **ゼロ** (撮影中のみ走行) |

## 8. リスクと未解決事項

### 8.1 OIT 不採用のリスク (Tier 2)
複数 α 層が深さ方向に重なる場合の back-to-front sort が forward alpha 既存 sort に依存。**髪単独なら問題なし**、複雑シーンで artifact 発生したら Tier 1 (OIT 系) に上げる選択肢を持つ。

### 8.2 Tier 0 の motion blur 干渉
撮影中に avatar が animation で動くとブレる (= modelview accumulation で blur)。これは「物理光学正解」なので意図通りだが、UX 上嫌う層もある可能性。撮影前 freeze オプション要検討。

### 8.3 HQ DoF / chroma との整合 (Tier 2)
postDeferredHQDoFF.glsl の per-sample depth gate を near layer に転用するか、別 kernel にするか。L2 で実装時判断。

### 8.4 撮影中 cancel (Tier 0)
進捗 30% で気が変わった時の中断・破棄。Phase L6 で UI 設計。

## 9. 受入条件

### 9.1 Phase L4 (Tier 2) 完了時
1. 同 viewpoint で r30 cutoff 0.1 の halo と比較して、外周 halo が **目視で明確に改善**
2. Cinematic mode 非有効時に従来描画と差分ゼロ (regression なし)
3. Frame Profile で post-process budget が +2 ms 以内
4. AYA さん主観で「live preview として halo 気にならない」レベル

### 9.2 Phase L7 (Tier 0) 完了時
1. 同 viewpoint で Tier 2 と比較して bokeh 形状・noise・ハイライト広がりが **明確に向上**
2. 撮影 1 枚 ≤ 2 秒で完了 (256 sample 設定)
3. 撮影中 live preview と切替できる UX
4. AYA さん主観で「offline レンダ並の photographic quality」に到達

両方達成で `experiment/` prefix を外して `feature/ayastorm-r31-cinematic-dof-2tier` 等にリネーム、r31 章として正式 release。

## 10. 次の具体的着手

Phase L1 (Tier 2 prototype) 開始:
1. `mNearLayerColor` RT allocate (pipeline.cpp resizeScreenTexture / allocateScreenBuffer)
2. forward alpha shader に MRT output 追加 (alphaF.glsl / alpha shader 一式)
3. composite shader (`dofCombineF.glsl` 改) で `mNearLayerColor.rgb * α + opaque_buffer * (1-α)` を最後に乗せる
4. 「画面が出る」「halo の場所に α が乗っている」だけ確認 → スクショ取得

ここまでで 1 セッション分。L2 (実 blur) は次セッション以降。
