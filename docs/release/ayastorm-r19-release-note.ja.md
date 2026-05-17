# AYAstorm r19 — リリース告知

GitHub release ページ貼り付け用の文案。**r19 は視覚的リアリティ章 (r14〜r20) の第 6 弾、B 軸 (物質色) の第 1 弾**です。deferred lit 経路に wrap-around diffuse + back-light transmission を加算し、葉・白い布カーテン・逆光の耳の縁といった **薄物が太陽光を透過** する状態を作ります (これまで flat に黒く落ちていた)。

> **配信形態**: r19 は **r23 リリースに同梱配信** されます (r19 単独タグは発行しません)。r23 リリースページから本ノートと r19 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r19-translucency.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r19 — 薄物の透過 (translucency / 太陽光の背面透過)

### r19 の柱: 薄物が太陽光を透過する

A 軸 (r14〜r18) で空気の信頼性を作りました。B 軸ではその空気の中に置かれる **物質本来の色と質感** に向かいます。r19 はその入口、薄物が太陽光を透過する効果を解きます:

- **葉が太陽に向かって透ける** — 木陰から空を見上げると、葉の縁が太陽光を捉えて透過する、葉脈や輪郭の質感が出る (これまで真っ黒なシルエットだった)
- **白い布カーテンに光が回る** — 窓辺の薄布カーテンの裏側に立つと、光が透過してカーテンが内側から発光しているように見える
- **人物の耳・鼻翼・指先が透ける** — 強い順光で人物の耳の縁が赤く透ける、肌が生きている感じになる
- **紙・蝋燭・薄い陶器の半透明感** — 薄物全般の質感

効果は **instant A/B で誰でも分かる** ように設計。前任の r19 (アルベド忠実度、frozen archive `feature/aya-r19-albedo-fidelity-spec-draft`) は「数学的に正しいが視認困難」で全 drop した教訓から、r19 はあえて視覚的に決定的な効果へ scope を切り替えています。

### 仕組み

**Wrap-around diffuse (Burley wrap) + Back-light transmission の加算** を `class3/deferred/softenLightF.glsl` の `sun_contrib` 加算項として 1 経路で実装:

```
nl_wrap = max((N·L + w) / (1 + w), 0)            // wrap 項、Legacy で da を置換
back    = pow(max(-N·L, 0), k_back)              // back-light: 太陽が背面側で強い
view    = pow(max(V·L, 0), k_view)               // 視線が太陽方向のとき
transmit = back * view * tint * strength * sunlit_linear
```

softenLightF.glsl 内 2 ヶ所注入で deferred routing 全分岐をカバー (`docs/ayastorm-deferred-shader-routing.md` 参照):

- **Legacy 分岐** (壁・avatar・耳・旧服 → `materialF` writer): `da` を `nl_wrap` に置換 → `sun_contrib` 加算後に `transmit * baseColor.rgb` を加算
- **PBR 分岐** (Mesh 服 → `pbropaqueF` writer): `pbrBaseLight()` 呼び出し後に `transmit * baseColor.rgb` を加算

C++ 側は intensity tier ごとの 4-tuple を slow / fast の `bindDeferredShader` 両経路で push、live toggle 対応。

### 局所灯 (point / spot) と太陽

r19 は **太陽光のみ** 対応。局所灯の wrap / back-transmission は二次効果として r20+ に分離。太陽光主導の design で写真撮影ユースケース (太陽逆光の葉、窓越しのカーテン、窓辺ポートレート) は完全にカバーされ、scope を絞れる。

### 設定

| Key | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` (ON, U32) | 章 master switch (Firestorm View / AYAstorm View) |
| `AYAR19TranslucencyEnabled` | `1` (ON) | r19 sentinel。`0` で r18 までの lit 計算に戻る |
| `AYAR19TranslucencyIntensity` | `1` (U32, 0-3) | 0=OFF / 1=控えめ (default) / 2=標準 / 3=強め。`(wrap, k_back, k_view, strength)` の tier table は `pipeline.cpp` に保持 |

intensity slider は意図的に同梱。透過効果は「デフォルト控えめ、ポートレート用に強めも欲しい」性質で、tier 1 (default) は CG 感 (常に何でも透ける) の失敗モードを避けるように調整しています。

### 既知の制約

- **prim transparency (`alpha > 0`) は softenLightF を経由しない** — これらの prim は forward `alphaF` 経路を踏み、r19 は触れていない。intensity も効かない。forward 経路への注入は r20+ 候補
- **Subsurface 厚み依存は近似** — r19 は `scol` (sun shadow) を self-shadow 減衰に流用しており、平面 prim でも副次的な厚み依存性は機能するが、本来の diffusion profile ではない。物理正確 SSS (thickness map / Burley diffusion / multi-scatter) は heavy + preset 改修要のため章 roadmap §6 で永久 drop
- **昼間 (レガシー) preset は影響なし** — r19 では pinpoint 除外していないが、太陽が高い性質上 back-transmission は自然に minimal、回帰報告は無し

### 実装概要

- `class3/deferred/softenLightF.glsl` — wrap + back-transmission uniforms + helpers (`ayaTranslucencyWrap`, `ayaTranslucencyTransmit`)、Legacy 分岐 + PBR 分岐の 2 注入点
- `pipeline.cpp::renderDeferredLighting` — softenLightF bind 直後に tier table の uniform を push、slow / fast 両経路カバー
- `settings.xml` — `AYAR19TranslucencyEnabled` Boolean default 1、`AYAR19TranslucencyIntensity` U32 default 1

### ドキュメント

- r19 spec / Burley wrap パラメータ / リスク登記: `docs/ayastorm-r19-translucency.md`
- deferred shader routing リファレンス (writer → softenLightF 分岐表): `docs/ayastorm-deferred-shader-routing.md`
- frozen r19 albedo-fidelity archive (前任 scope、drop): `docs/ayastorm-r19-albedo-fidelity.md`
- 視覚的リアリティ章ロードマップ (B 軸入口 = r19): `docs/ayastorm-visual-realism-roadmap.md`
