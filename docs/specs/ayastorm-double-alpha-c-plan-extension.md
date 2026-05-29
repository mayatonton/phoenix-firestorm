# AYAstorm 二重アルファブロック regression: 3-pass dispatch 採用案 spec

**作成日**: 2026-05-29 (起案) / 2026-05-30 (採用案確定)
**branch**: `fix/double-alpha-block-painter-merge`
**baseline HEAD**: `4d49216f46` (= `ayastorm-release` と完全同一、clean state)

**関連資料**:
- `docs/specs/ayastorm-six-category-render-order-trace.md` (本 fix 起点 trace、8 カテゴリ × 24 ケース描画順 + multi-avatar)
- `docs/specs/double-alpha-block-fix.md` (§5 swap = 現 baseline に入っている 2 行 fix の元 spec、`fix/double-alpha-block` branch 上の公開資料)
- `docs/specs/ayastorm-attachment-rendering-routing.md` / `ayastorm-rez-object-rendering-routing.md` (装着物 / Rez object 起点 trace)

---

## §1 目的と結論

### §1.1 目的

`ayastorm-release` に既に入っている **§5 swap** (`lldrawpoolalpha.cpp:277-294` の POST_WATER で `forwardRender()` (non-rigged) → `forwardRender(true)` (rigged) の non-rigged-first 化) が新規に発生させた構造的副作用 (= AYA さん観測 S1/S2/S3 = 装着物 N-BL prim が rigged hair に上書きされる) を解消する。

### §1.2 非目的 (絶対やらないこと)

- **§5 swap の revert は scope 外**。AYA さんが背景描画の美しさを理由に明示意思決定した経緯を尊重し、どの段階でも revert しない。
- **案 D (rigged-first revert + R-BL shader alpha cutoff) の再提案は禁止**。AYA さん既試済 = approach として確定 reject 済。
- **Phase 1 / Phase 2 / Phase 2.1 系の case-by-case comparator sort 設計を再演しない**。3 連続 REJECT で「sort 設計層では解けない」結論済。

### §1.3 結論

**採用案 = 3-pass dispatch** (`mAttachedToAvatar` discriminator)。詳細 §2。

**REJECT 案 = §6 PoC 改訂版 = 案 A (alpha plate independent depth)**。bisect R1/R2 で 2 段階 REJECT、構造的に alpha BLEND と write_depth は非両立。詳細 §3。

---

## §2 採用案: 3-pass dispatch (`mAttachedToAvatar` discriminator)

### §2.1 解決した問題

§5 swap (POST_WATER 全 non-rigged → 全 rigged) は SIM の「髪越し背景透過」二重アルファブロックを解消したが、副作用として **装着物の non-rigged N-BL prim (まつ毛 prim 等)** が rigged 段の前に描かれ、その後 rigged hair に over-blend で上書きされる S1 (前後反転) / S2 (手前透過上書き) regression を生んだ。装着物 N-BL prim の z は plate に書かれない (write_depth=false) ため LEQUAL gating も働かず、純粋に描画順による上書きが起きる。

### §2.2 採用したアプローチ

POST_WATER 内の forward render を **3 pass** に分割。`LLDrawInfo::mAttachedToAvatar` を per-draw discriminator として使い、装着物 (notNull) と SIM (isNull) を non-rigged batch loop 内で振り分ける:

```
pass 1: forwardRender(rigged=false, ATTACHMENT_NONE)  -- SIM rezz N-BL のみ
pass 2: forwardRender(rigged=true)                     -- 全 R-BL
pass 3: forwardRender(rigged=false, ATTACHMENT_ONLY)   -- 装着物 N-BL のみ
```

これで描画順は:

1. SIM 側の背景 N-BL (窓ガラス / 葉先) — §5 swap で fix 済の「髪越し背景透過」維持
2. 全 R-BL (rigged hair / clothing) — write_depth=true で z 書込
3. 装着物 N-BL prim (まつ毛 prim 等) — rigged hair の **後** に描かれて pix-level で上に来る

S1/S2 = 装着物 prim が hair に隠れる regression は構造的に解消。pass 3 は write_depth=false で plate.depth に書かないので、後続 pipeline (DoF など) は rigged の z を見る (= 装着物 prim は subject 扱いされない、これは r30 P5 DoF C plan と整合)。

PRE_WATER と HUD は water fog 整合性 / forwardRender 1 回のみのため §5 swap 元の形を維持 (3-pass 化対象外)。

### §2.3 実装ファイル

- `indra/newview/lldrawpoolalpha.h`
  - `enum AttachmentFilter { ATTACHMENT_ALL, ATTACHMENT_NONE, ATTACHMENT_ONLY }` 追加
  - `forwardRender` / `renderAlpha` に filter 引数を追加 (default = `ATTACHMENT_ALL` で legacy 互換維持)

- `indra/newview/lldrawpoolalpha.cpp`
  - `renderPostDeferred` POST_WATER 分岐を 2 call → 3 call に変更
  - `forwardRender(bool rigged, AttachmentFilter)` で filter を `renderAlpha` に thread
  - `renderAlpha` draw loop で `!rigged` 時のみ filter を per-batch 適用 (`mAttachedToAvatar` の null 判定で skip)
  - `renderDebugAlpha` 発火条件を pass 3 末尾 (filter==`ATTACHMENT_ONLY` or `ALL`) に限定 (SIM+装着物 batch 全部の highlight を 1 回だけ点灯)

### §2.4 検証 PASS (2026-05-30、AYA 実機 Linux)

- S1/S2 描画順 = 装着物 N-BL prim が rigged hair の前面 (正)
- 背景空抜け非再発 = SIM 窓ガラス / 葉先が hair に空抜けしない (§5 swap 当初 fix 維持)
- SIM rezz alpha mat の opaque 上書き非再発 (R2 で出た regression は構造的に発生しない、pass 1 の N-BL は opaque 直後の従来位置で描画)
- 報告 1 件 (「非 rigged rigid mesh attached の hair 商品で左右両側が両方表示 + HUD 左右切替効かず」) → 別 hair 商品では正常動作確認、商品側 LSL state 破損と切り分け済 (3-pass 起因ではない)

---

## §3 falsification 履歴: 案 A (alpha plate independent depth)

### §3.1 §6 PoC 改訂版の狙い

handoff doc で確定した PoC 改訂版 = alpha plate (`mAYAAlphaColor`) の depth を screen.depth から独立化し、plate 内で alpha BLEND の sort/depth を完結させる設計:

1. cvar `AYAAlphaPlateIndependentDepth` 追加
2. `mAYAAlphaColor.allocate(..., depth_buffer=true)` で plate 専用 depth attachment 確保
3. `shareDepthBuffer(mAYAAlphaColor)` を skip (screen.depth との共有を切る)
4. plate bind 直後に `glClear(DEPTH_BUFFER_BIT)` で plate depth を far-plane 初期化
5. forward alpha plate 経由時 (`mForwardToAlphaRT == true`) は rigged/non-rigged 問わず `write_depth = true` に拡張 (plate 内 R-BL ↔ R-BL 比較 + N-BL → R-BL sort 成立を期待)

実装ファイル 4 件 (settings.xml / pipeline.h / pipeline.cpp / lldrawpoolalpha.cpp)、約 40-60 行規模。

### §3.2 bisect R1 / R2 の経緯 (2026-05-30 同日)

- **Scene 1 (cvar=1 with 全 4 件 ON)**: AYA 実機で「alpha materials がすべて消える + 描画面が反転している?」報告。
  - 仮説 = (5) write_depth 拡張で alpha=0 fragment が depth 書込 → 後続 alpha を LEQUAL で reject の悪循環

- **Bisect R1 ((5) write_depth 拡張のみ disable、(1)-(4) 維持)**: 「white grid + alpha mesh 白塗り」報告。
  - 真因 = `llgl.cpp:2828-2834` の LL 仕様「`LLGLDepthTest(GL_FALSE, GL_TRUE)` は depth_test=false なら write_enabled を強制 false に書換」で plate depth clear が no-op (mask OFF) → plate.depth が garbage 残留 → 描画破綻

- **Bisect R2 (depth clear を `LLGLDepthTest(GL_TRUE, GL_TRUE, GL_ALWAYS)` に変更)**: 「SIM rezz alpha mat が不透明背景を上書き」報告。
  - 真因 = depth clear は効くようになったが、(5) write_depth が OFF のままなので plate.depth が 1.0 (far) 維持、全 alpha が depth gating 無しで pix を上書き

### §3.3 構造的 reject 理由

**alpha BLEND と write_depth は構造的に非両立**:

- BLEND は order-dependent (back-to-front 必要) で fragment ごとの contribution が累積
- depth write は order-independent (occlusion gating) で fragment ごとに z を覆い書き
- alpha=0 fragment の z は意味的に「存在しない」が、shader が discard しない限り depth pipeline は書く
- 結果として「透明 fragment の z が後続透明 fragment を reject」or 「不透明扱いの depth で背景が消える」のどちらかが必発

§6 PoC 改訂版は「plate 上で sort を独立化する」設計だったが、plate に depth を持たせた瞬間に上記非両立が顕在化し、bisect R1/R2 で 2 段階の REJECT。AYA 「どれも間違っている気がする、そもそも理解が足りていない」で再認識:

> 「単純に　装着物のアルファプリムより前面に　装着物のアルファの髪の毛やまつげが描画されている (上書きされている) だけを複雑に考えている」

これで「depth 機構を変える」アプローチ自体が overengineering だと判明、§2 採用案 (純粋に dispatch order を 1 段増やす) に pivot。

### §3.4 採否比較表

| 項目 | §6 PoC 改訂版 (案 A) | §2 採用案 (3-pass) |
|------|----------------------|----------------------|
| 追加 cvar | あり (1 件) | なし |
| RT 構成変更 | あり (plate に depth attach) | なし (baseline 維持) |
| depth 機構 | plate 独立 depth + write_depth 拡張 | 不変 (baseline 維持) |
| 構造的非両立 | alpha BLEND ⊥ write_depth で必発 | なし (純粋な順序変更) |
| 装着物 / SIM 識別 | 不要 (sort 任せ) | `mAttachedToAvatar` per-draw |
| AYA 実機検証 | 2 段階 REJECT (R1/R2) | PASS |
| 採否 | **REJECT (falsified)** | **採用** |

---

## §4 参照リファレンス

### §4.1 spec 関連リファレンス

- `docs/specs/ayastorm-six-category-render-order-trace.md` (8 カテゴリ × 24 ケース + multi-avatar 描画順 確定済、本 fix 起点 trace)
- `docs/specs/double-alpha-block-fix.md` (§5 swap = 現 baseline 2 行 fix の元 spec、`fix/double-alpha-block` branch 上の公開資料)
- `docs/specs/ayastorm-attachment-rendering-routing.md` (装着物 trace 起点、`mAttachedToAvatar.notNull()` 側)
- `docs/specs/ayastorm-rez-object-rendering-routing.md` (Rez object trace 起点、`mAttachedToAvatar.isNull()` 側)
- `docs/specs/ayastorm-deferred-shader-routing.md` (deferred shader routing 起点)
- `docs/specs/ayastorm-gbuffer3-trace.md` (gbuffer3 storage 仕様起点)

### §4.2 参照 source file:line

- `indra/newview/lldrawpoolalpha.cpp:262-285` — 3-pass dispatch 本体 (POST_WATER で `forwardRender(false, ATTACHMENT_NONE)` → `forwardRender(true)` → `forwardRender(false, ATTACHMENT_ONLY)`)
- `indra/newview/lldrawpoolalpha.cpp:413+` — `forwardRender(bool rigged, AttachmentFilter)` 実装
- `indra/newview/lldrawpoolalpha.cpp:790+` — `renderAlpha(..., AttachmentFilter)` draw loop 内 per-batch filter (`mAttachedToAvatar` null 判定)
- `indra/newview/lldrawpoolalpha.h:68-87` — `AttachmentFilter` enum + `forwardRender`/`renderAlpha` signature
- `indra/llrender/llgl.cpp:2828-2834` — `LLGLDepthTest(GL_FALSE, GL_TRUE)` で write_enabled が強制 false になる LL 仕様 (§3.2 R1 の真因)
- `indra/newview/llspatialpartition.h:128-131` — `LLDrawInfo::mAttachedToAvatar` 定義
- `indra/newview/llvovolume.cpp:5845-5851` — `mAttachedToAvatar` 設定箇所 (avatar attachment 時に avatar ptr を set)
