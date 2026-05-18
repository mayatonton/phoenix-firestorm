# AYAstorm r30 P2 — リリース告知

**r30 P2 は Cinematic mode に per-object motion blur と SMAA T2x を導入するリリース** — 撮影描画章 (r30 chapter) の基盤として、速度バッファ生成 path と temporal resolve を本実装します。

実装トレース・改修ポイント・受入観測・上流参照行は永続資料 (`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r30 P2 — Velocity buffer + Per-object Motion Blur + SMAA T2x

### r30 P2 の柱: Cinematic mode の描画基盤を立てる

r30 章 (撮影描画) は P1 で View Mode を 3 モード再起動切替に統一し、Cinematic 枠を UI 上に先行追加しました。P2 はその Cinematic mode に **per-object motion blur** と **SMAA T2x temporal antialiasing** を本実装します。両者は共通の **velocity buffer** (前フレームからの NDC delta を per-pixel で書き出す `GL_RG16F` RT) を基盤とし、Cinematic 起動時のみ確保 / 描画されます。

通常 (Standard) / リアリズム (AYAstorm View) モードでは velocity buffer 自体を確保しないため、追加コストはゼロです。

### 仕組み

velocity buffer 生成 path:

```
display() flow
  → renderGeomMotionBlur()
       ├─ mVelocityMap を (0,0,0,1) で clear
       └─ pool.renderMotionBlur() を全 pool に発行
              ├─ Bump / Materials / PBR opaque  (rigged / static)
              ├─ Tree / Terrain (face-iter)
              ├─ Alpha mask (alpha-discard 付き)
              └─ Avatar (LL 専用 skin)
  → 各 pool が pushVelocityBatches{,Textured} / pushRiggedVelocityBatches{,Textured} 経由で
     現フレーム matrix と前フレーム matrix を uniform に乗せ、velocity shader で NDC delta を書く
```

post-process 合成 (composite):

```
deferredScreen (lighting 後の最終色)
  → motionBlurF.glsl (32-tap triangle-weighted blur along velocity direction)
       ├─ NaN/inf guard (uninitialized matrix 由来の garbage velocity 防御)
       ├─ noise floor 2.0 px (subpixel drift で static 物が滲むのを抑止)
       ├─ sanity ceiling 2× max_blur (skinning blowup / SIM 境界跨ぎの暴走 velocity を passthrough)
       └─ per-sample velocity gate (opt-out avatar 周辺の halo bleed 抑止)
  → frag_color
```

SMAA T2x temporal resolve:

```
SMAA (空間 AA) の最終 blend pass の前に
  → Halton(2,3) 2-tap subpixel jitter (projection matrix に注入)
  → 前フレーム結果を velocity でリプロジェクト
  → 50/50 blend で 2-sample 平均 (T2x の "2x")
```

### 設定

**通常運用での操作は不要。** Cinematic mode (`AYAVisualRealismEnabled = 2`) を起動すれば motion blur / SMAA T2x の基盤が自動で有効化されます。チューニング用の cvar は以下:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `RenderMotionBlurStrength` | `32` | motion blur composite の最大 blur 長 (pixel)。`0` で composite を off (velocity buffer 自体は生成される) |
| `RenderMotionBlurSelfAvatar` | `1` (ON) | 自分のアバターを velocity buffer に書き込むか。OFF にすると自身は常に crisp (1人称視点 / 自撮りでカメラ振りだけブラしたい時) |
| `RenderMotionBlurOtherAvatars` | `1` (ON) | 他アバターを velocity buffer に書き込むか。OFF にすると他人は常に crisp (グループ撮影で背景だけブラしたい時) |
| `RenderSMAAT2x` | `0` (OFF) | SMAA T2x temporal resolve。`RenderFSAAType=2` (SMAA) + Cinematic 起動が前提。エッジ細部 (木の葉 / 髪 / 細枝) でわずかに滑らかになる |
| `RenderBufferVisualization` | `-1` | `7` で velocity buffer を画面に可視化 (R=X, G=Y velocity)。診断専用 |

Boolean cvar の値変更は Debug Settings の Window を **閉じた時点** で commit されます (auto-widget の commit タイミング、code 側 bug ではない)。トグル後に Window を一度閉じてから挙動を確認してください。

### 移行ノート

- 撮影モード (Cinematic) で per-object motion blur / SMAA T2x が新たに使えます
- 通常 / リアリズムモードには一切影響なし (velocity buffer も確保されない)
- Cinematic ↔ 他モードの切替は **viewer 再起動が必要** (r30 P1 で確定した設計、velocity / SMAA RT 構成の動的差し替えを避けるため)
- 既存の `RenderMotionBlur` 系 cvar とは独立した key を使っているため、上流 (Linden / Firestorm) との設定衝突は発生しません

### 既知の制約

- **classic / system avatar body の per-bone motion blur**: avatar pool 経路では `lastMatrixPalette` uniform を upload しないため、classic body の四肢動作は per-bone velocity が出ません。実装上は composite 側の sanity ceiling guard (`speed > max_blur * 2.0` で passthrough) で安全側に倒れます。現代 SL の大多数 (mesh body アバター) では mesh attachment 側の rigged 経路が独立して per-bone velocity を正しく出すため可視影響なし。classic body をそのまま見せるアバターに動きが激しい場面で per-bone blur が乗らない、というレアケース限定の挙動です。修正は r30 後期 phase で再検討
- **chained Ogg / serial change** 等のように上流 streaming 起因の挙動ではなく純粋に描画 path の特性
- **macOS / Windows 実機検証**: AYAstorm 側 Linux ビルドで動作確認 PASS、Mac/Win の Release ビルドは tag 切り出し時に実施

### 実装概要

- shader (`indra/newview/app_settings/shaders/class1/deferred/`):
  - velocity 系 9 shader (Black Dragon Viewer から借用): `avatarVelocity{F,V}.glsl`, `skinnedVelocity{V,AlphaV}.glsl`, `velocity{F,V,Alpha{F,V},FuncV}.glsl`
  - SMAA T2x resolve: `SMAAResolve{V,F}.glsl`
  - motion blur composite: `motionBlurF.glsl` (per-sample velocity gate を AYAstorm 側で追加、`RenderMotionBlur{Self,Other}Avatars` opt-out 時の halo bleed 抑止)
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: `mVelocityMap` / `mSMAAHistory` RT 確保 (Cinematic gate)、`renderGeomMotionBlur()` 新規、`renderMotionBlurComposite()` 新規、`renderBufferVisualization` case 7 追加
  - `lldrawpool.{cpp,h}`: `LLDrawPool::{getNumMotionBlurPasses, beginMotionBlurPass, renderMotionBlur, endMotionBlurPass}` 仮想 API + 4 push helper (`push{,Rigged}VelocityBatches{,Textured}`)
  - 各 drawpool subclass (Bump / Materials / PBR / Tree / Terrain / Alpha / Avatar): velocity pass override 実装
  - `lldrawpoolavatar.{cpp,h}`: `RenderMotionBlurSelfAvatar` / `RenderMotionBlurOtherAvatars` opt-out
  - `llspatialpartition.h`: `LLDrawInfo::mAttachedToAvatar` 追加 (static prim attachment の wearer 紐付け、`mAvatar` (rigged) と独立)
  - `llvovolume.cpp`: `registerFace()` で `mAttachedToAvatar = vobj->getAvatar()` 設定
- `indra/newview/app_settings/settings.xml`: P2 cvar 4 件 + `RenderBufferVisualization=7` 説明拡張
- Cinematic 起動時のみ velocity buffer 確保 → 通常 / リアリズムモードはコストゼロ

### Credits

velocity buffer + per-object motion blur + SMAA T2x の本実装パターンは [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean) 由来です。AYAstorm では BD `995a1354d8` (2026-04-19) を上流参照点として、9 shader + composite shader + SMAA resolve shader をライセンス継承 (LGPL-2.1-only) で取り込み、AYAstorm 側で以下の改修を加えました:

- Cinematic mode gate (`AYAVisualRealismEnabled == 2` でのみ RT 確保)
- avatar opt-out cvar 2 件 (Self / OtherAvatars) と書込み側 skip helper
- composite shader の per-sample velocity gate (opt-out 周辺の halo bleed 抑止)
- BD `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` の T-pose rasterize bug 修正 (`current_clip` 側に object skinning を適用)
- composite 側 garbage velocity 防御 (NaN/inf guard + sanity ceiling 2× max_blur)

### ドキュメント

- r30 P2 full trace / file:line 改修マップ / step 1〜5e 実装 commit log / 受入観測: [`docs/specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md`](../specs/ayastorm-r30-p2-velocity-buffer-bd-trace.md)
- 親 spec (r30 章): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前段 (r30 P1, View Mode 再起動切替): [`docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`](../specs/ayastorm-r30-p1-view-mode-restart-switch.md)
