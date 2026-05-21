# AYAstorm r30 P5 — 透過 DoF L1 (opaque-only bg-depth snapshot)

## 動機 (実機観測)

Cinematic + DoF ON で髪 (rigged alpha BLEND attachment) を被写体に撮ったとき、
**髪の隙間/縁から見える背景**が周囲の背景と同じようにボケず、しばしばシャープ
になる症状。背景単独 (髪の外) は正常にボケる。

原因: 髪 mesh は flat board に透過テクスチャを貼った構成で、その物理 z は焦点
距離付近 (= avatar の頭の位置)。BD parity の forward alpha pass は rigged BLEND
が `write_depth = true` で深度書込を行う (`lldrawpoolalpha.cpp:270-278`) ため、
髪 board の pixel では `deferredScreen.depth` ≈ focal distance となる。後続の
`cofF.glsl` がこの depth で CoF を計算 → CoF ≈ 0 → ブラーなし。本来「板を抜けた
背景」が描画されているはずの pixel が、髪 board と一緒にシャープ扱いされてしまう。

## L1 方針 (Approach A: opaque-only depth snapshot)

「透明だったら背景の焦点でぼかす だけでいい」(AYA, 2026-05-21) を素直に実装。
全 pixel に対し **opaque pass 終了時点の depth (alpha BLEND 寄与なし) を cofF
入力に使う**。

- opaque-only な pixel: `bg_depth == deferredScreen.depth` → 挙動不変
- alpha BLEND 寄与のある pixel: `bg_depth` は alpha BLEND mesh を抜けた更に奥の
  opaque z → CoF が「板の z」でなく「板を抜けた背景の z」で決まる → 髪越し背景
  も背景と同じくボケる

L1 は per-pixel な「alpha の濃さ」を判別せず、**全 pixel 一律で bg_depth を採用**
する最も単純な構成。

## 実装 (3 ファイル, ~50 行)

- `pipeline.h`: `LLRenderTarget mAYABgDepth` 宣言
- `pipeline.cpp`:
  - `allocateScreenBufferInternal`: mMainRT のみで `mAYABgDepth.allocate(resX, resY, GL_RGBA, true)` (color は未使用、depth attachment が要るため allocate true)
  - `releaseScreenBuffers`: `mAYABgDepth.release()`
  - `renderDeferredLighting` の forward alpha 直前: `deferredScreen.depth → mAYABgDepth` を `gCopyDepthProgram` (interface/copyF.glsl の COPY_DEPTH permutation) でブリット。snapshot は forward alpha 前なので alpha BLEND の depth 寄与を含まない
  - `renderDoF` の `gDeferredCoFProgram` の `DEFERRED_DEPTH` binding を `&mAYABgDepth` に差替 (isComplete fallback 付き)
- shader 側: 無改変 (cofF.glsl の `depthMap` uniform 自体は変えず、CPU 側の binding 差替だけで意味を変える)

## 実機検証結果 (2026-05-21, AYA)

| シーン要素 | bg_depth の中身 | L1 cofF 挙動 | 観測 |
|---|---|---|---|
| opaque 単独 (建物壁・地面・metal frame) | 該当 mesh の z | 元と同じ CoF | 回帰なし ✓ |
| 髪 (rigged alpha BLEND board) | 髪を抜けた opaque z | 背景に揃った CoF | **目的達成 ✓** |
| 窓格子 (透過 texture 1 枚 flat mesh, 被写体性のある alpha BLEND) | 窓を抜けた更に奥 (= 空など) の z | 遠景準拠の巨大 CoF | 窓格子自身が過剰ブラーで画面から消える ✗ |

副作用の影響範囲は窓ガラスに限らず、**「透過 texture を貼った flat mesh で被写体
として見せたい alpha BLEND」全般** (鉄柵, 葉茂み, 布のドレープ, レース等)。SL の
撮影シーンで頻出するため副作用許容不能。

## 判断: 採用見送り、revert

主目的 (髪越し背景のボケ) は達成したが、L1 の構造的前提 (全 pixel 一律 bg_depth)
が「自身が被写体である alpha BLEND mesh」と衝突する。AYA 判断 (2026-05-21) に
より、本 commit は実装記録として履歴に残した上で次 commit で revert する。

## 次にやる予定 (L2)

forward alpha pass 中に「alpha 寄与の濃さ」を新 1ch buffer に accumulate、cofF
側で per-pixel に `self_depth / bg_depth` を切替える方向。詳細設計は L2 着手時に
実装と同期して書く (memory `feedback_build_only_verified.md` 順守、先 phase まで
の詳細設計はしない)。
