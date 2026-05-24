# AYAstorm r20: avatar skin SSS (subsurface scattering)

**作成日**: 2026-05-13 (初版起票)
**位置づけ**: AYAstorm 視覚的リアリティ章 B 軸第 2 弾 (r19 translucency に続く)
**ベース branch**: `ayastorm-release` (r19 とは独立、依存なし)

> **2026-05-24 update (UI 配置のみ変更)**: SSS 設定 UI は r30 で
> Preferences → グラフィック → SSS から **AYAstorm Controls (Alt+C) →
> "Skin SSS" タブ** に移行。shader 経路 / cvar 名 / whitelist 形式は本 spec の
> まま無変更。詳細経緯は
> [`ayastorm-r30-aya-controls-tab-overhaul.md`](ayastorm-r30-aya-controls-tab-overhaul.md)
> 参照。

---

## 1. 章 thesis との接続

`docs/ayastorm-visual-realism-roadmap.md` §3 の B 軸 (物質色) は「アルベド忠実度 / 薄物の subsurface / **material response**」を含む。r20 はこのうち **「肌という物質の本来の見え方」** を対象にする。

写真撮影で人物が被写体になる場面で、肌の「温度感」「やわらかさ」は写真感を決定する大きな要素。現状の SL では肌が他の表面 (壁・服) と同じ Lambert + Specular で計算されており、「肌のリアルさ」が出ない。

- LUT / Filter / 色温度 で出せる範囲ではない (= 物理的な光のふるまい)
- 既存 SL 機能で代替不可
- インパクト: 人物撮影で「写真感」が一段上がる、r18 (cinematic 夕焼け) 級「90% 気づく」候補

---

## 2. 設計概要

### 2.1 課題

SL の Mesh body / head は deferred で **`materialF` (Legacy) または `pbropaqueF` (PBR)** を踏み、prim や Mesh 服と同じ shader を共有する。viewer 側で「この fragment は肌だ」と判別する API は無い。

判別方法の候補:
- **(α) avatar designer 主導タグ**: skin Mesh の Description に `[AYAstorm_skin]` を入れてもらう → エコシステム整備重い
- **(β) viewer 側 whitelist**: 主流 Mesh body / head 製品名で自動判別 → AYA さんの観察「3-4 製品で 90% カバー」と整合、viewer 側完結
- **(γ) texture hue ヒューリスティクス**: hue が肌っぽいかで判別 → 信頼性低い、誤検出多発見込み

**採用: (β) viewer 側 whitelist**。理由は AYAstorm 流儀「無難なデフォルト優先」と整合、avatar designer の協力不要、主流 90% カバーで instant impact が出る。

### 2.2 想定 whitelist (P0 で精査)

- **Mesh body**: Maitreya Lara, eBody Reborn, Legacy (RP/Athletic), Belleza Jake/Freya, Inithium Kupra
- **Mesh head**: Lelutka EvoX, Genus Project, LAQ, Catwa (legacy 残存分)

実際の Name / Description pattern は P0 で実機サンプリングして確定 (例: 「LELUTKA EVOX 〇〇」「Maitreya Lara V〇」等の prefix が typical)。

> **実装版での結論**: default whitelist は **空** で出荷しています。本セクションに列挙した主流 Mesh body / head 製品は **設計段階の想定リスト** であり、実機での asset UUID キュレーションを継続的にメンテする運用負荷を回避するため、出荷物には含めず、ユーザー側で右クリック UX (`Add to SSS whitelist` / `Add entire linkset to SSS whitelist`) または手動編集で登録する形を採っています。ユーザー向け案内は [`docs/specs/skin-sss-user-guide.md`](./skin-sss-user-guide.md) 参照。

### 2.3 アプローチ全体図

```
[LLVOAvatar attachment scan]
    ↓ (name/description match)
[Mark faces with skin tag]
    ↓ (writer 側: materialF / pbropaqueF)
[GBuffer: write IS_SKIN flag bit]
    ↓ (gbuffer flag 値拡張 or 別 channel)
[softenLightF or 専用 SSS pass]
    ↓ (skin flag のみ screen-space SSS 適用)
[Diffusion-blurred lit を skin 領域に合成]
```

### 2.4 SSS アルゴリズム候補

- **Separable SSS (Jimenez 2015)** — horizontal/vertical 2 pass blur with depth/normal preserve、ゲームで広く採用、高品質
- **Screen-space diffusion (3-4 sample tap)** — 軽量、blur kernel を skin-tone weighted で展開
- **PBR diffusion profile (Burley)** — 物理ベース、calculation 重め、tile-based 化要

**初期実装**: Screen-space diffusion (3-4 tap) で着地、P1.b PASS 後に Separable へ昇格検討。

---

## 3. P0 Survey 必須項目

P1.a 着手前に必ず実機サンプリングして確定:

1. **whitelist 実機検証** — AYA さん含む複数 avatar で主流 body / head の Name / Description を Bash で打って実サンプル収集、pattern match 文字列を確定
2. **gbuffer flag 拡張余地確認** — 現在の GBuffer flag (HAS_ATMOS / HAS_PBR / HAS_HDRI / SKIP_ATMOS) に IS_SKIN を追加する空間があるか、もしくは別 bit / 別 channel が必要か
3. **writer 影響範囲** — `materialF` / `pbropaqueF` / `avatarF` のうち、Mesh body / head は実機でどれを踏むかは r19 P0R3 trace (docs/ayastorm-deferred-shader-routing.md) で既知 = `materialF` (Legacy) と `pbropaqueF` (PBR)。両方の writer に skin flag 書き分岐を追加
4. **SSS pass 配置点** — softenLightF 内 (skin だけ別計算) vs 専用 fullscreen pass (softenLightF 結果に skin blur を合成) を比較
5. **コスト試算** — 3-4 tap blur × screen res × skin coverage の負荷見積もり、3 OS GPU 想定

---

## 4. P1.a 設計案 (P0 後に詳細化)

### 4.1 settings.xml

```xml
<key>AYAR20AvatarSkinSSSEnabled</key>
<map>
  <key>Comment</key><string>r20 avatar skin SSS: 主流 Mesh body/head の肌に screen-space diffusion を適用</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>Boolean</string>
  <key>Value</key><integer>1</integer>
</map>

<key>AYAR20AvatarSkinSSSIntensity</key>
<map>
  <key>Comment</key><string>r20 SSS 強度段階: 0=OFF / 1=控えめ / 2=標準 / 3=強め</string>
  <key>Persist</key><integer>1</integer>
  <key>Type</key><string>U32</string>
  <key>Value</key><integer>1</integer>
</map>
```

### 4.2 C++ (LLVOAvatar 走査 + flag 配布)

- `LLVOAvatar::idleUpdate` or attach event hook で attached objects を scan
- Name / Description pattern match (whitelist 4-6 件)
- 該当 LLViewerObject に `mIsAYAR20Skin` フラグセット
- draw 時に shader に push (or gbuffer flag bit 書き込み)

### 4.3 GBuffer flag 拡張

現在の flag 値: HAS_ATMOS=0.34, HAS_PBR=0.67, HAS_HDRI=1.0, SKIP_ATMOS=0.0、`abs(data-flag) < 0.1` で判定 = 0.1 単位の余地

候補:
- **(a) 別 bit channel**: gbuffer normal の alpha channel (現在 envIntensity 等で消費中) を 1 bit 借りる
- **(b) flag 値追加**: HAS_PBR_SKIN=0.50, HAS_ATMOS_SKIN=0.17 のように skin variant 追加
- **(c) 別 RT 追加**: 重め、避けたい

**P0 で (a)/(b) 比較確定**

### 4.4 SSS pass

- screen-space 3-4 tap blur (depth + normal で edge preserve)
- skin-tone weighted kernel (赤 > 緑 > 青 で blur 半径変える、皮膚下 diffusion の波長依存近似)
- skin flag が立った fragment のみ適用、それ以外は no-op

### 4.5 tier table (P1.a で確定、初期案)

| Intensity | blur radius (px) | red bias | green bias | blue bias | strength |
|---|---|---|---|---|---|
| 0=OFF  | 0   | -    | -    | -    | 0   |
| 1=控えめ | 2   | 1.0  | 0.6  | 0.3  | 0.4 |
| 2=標準 | 3   | 1.0  | 0.7  | 0.4  | 0.7 |
| 3=強め | 4   | 1.0  | 0.8  | 0.5  | 1.0 |

---

## 5. 受け入れ条件

- [ ] `AYAVisualRealismEnabled = 1` かつ `AYAR20AvatarSkinSSSEnabled = 1` で:
  - **instant A/B で** 主流 Mesh body / head 装着 avatar の肌に diffusion 感が出る
  - 顔 / 手 / 露出した肌部分の境界がやわらかくなる、写真の人物撮影で「肌のリアル感」が出る
  - 服 / 髪 / 装飾品など肌以外の領域に SSS が漏れない (= whitelist 判別が正確に効く)
  - sustained viewing で違和感なし (CG 感が出ない)
  - r14-r19 効果が壊れない
- [ ] whitelist にない avatar (= 自作 mesh skin / minor brand) では SSS 無効、普通の lit、破綻なし
- [ ] `AYAVisualRealismEnabled = 0` で r19 までの絵に戻る (SSS 完全 no-op)
- [ ] `AYAR20AvatarSkinSSSEnabled = 0` で master ON でも r20 完全 no-op
- [ ] `AYAR20AvatarSkinSSSIntensity = 0` で intensity ゼロ = 完全 no-op
- [ ] `AYAR20AvatarSkinSSSIntensity = 1/2/3` で段階的に diffusion 強度が増える
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2、**B 軸完走 = r19+r20 セット**で一括 tag/release、positioning は「AYAstorm らしさを完全に打ち出す / Firestorm との差を見せる」)
- [ ] FPS 影響が ±5% 以内 (skin coverage は画面の数 % 程度、 blur 3-4 tap で軽量見積もり)

---

## 6. リスク

| ID | リスク | 対策 / 現ステータス |
|---|---|---|
| R1 | whitelist 漏れ (= avatar の Name / Description rename / 改造版で検出失敗) | 検出失敗時は普通の lit にフォールバック (= 破綻なし)、主流 4 製品で 90% カバー前提、release note で「対応 body / head リスト」を明示 |
| R2 | whitelist 誤検出 (= 製品名と被る他オブジェクト) | Name + attach point 複合判定で誤検出抑制、P0 で実機サンプリング検証 |
| R3 | SSS が「美肌フィルタ」風に見えてしまう (LUT/Filter 系の「写真風 look」と AYA 拒否方針の衝突) | 物理ベースの diffusion profile を採用、tier default を控えめ、AYA レビューで「美肌補正に寄せすぎ」フィードバックで tier 値再調整 |
| R4 | sustained viewing で違和感 (人物 avatar が常に「やわらかすぎ」「ぼけすぎ」) | tier default を控えめ、P1.b で sustained 必須、r17 で得た「instant A/B と sustained で別評価」 lesson 適用 |
| R5 | 3 OS で blur 出方の差 (OpenGL / Metal のサンプリング精度差) | 計算式を OS 中立、P0 で `.metal` shader path 確認 |
| R6 | GBuffer flag 拡張が既存 SL viewer / Linden 本家 merge を破壊 | (a) 別 channel 案で破壊範囲を最小化、もしくは (b) flag 値追加で既存判定式の `abs() < 0.1` を維持 |
| R7 | avatar が複数 (周囲に他人 avatar 多数) ある場面で performance 悪化 | skin 領域は画面の数 % 程度、blur tap も 3-4 で頭打ち、 worst case でも軽い見積もり、P1.b で実測 |
| R8 | LLVOAvatar 走査タイミングが重い (毎フレーム scan) | attach event hook ベース (= 装脱着時のみ更新)、毎フレーム scan 回避 |

---

## 7. 更新履歴

- 2026-05-13 (初版起票): r19 P1.b PASS 後の章方向議論で AYA さんから「アバターの肌の温度というか SSS みたいなことができたらすごい」と発言。AYAstorm 視覚表現章 (r14+) が r14-r18 で空 / 大気 / 光 / 雲を触り切り、r19 (薄物透過) は technical PASS だがインパクト弱という評価から、章方向の見直しを議論中だった文脈で B 軸第 2 弾候補として浮上。avatar designer タグ駆動と viewer 側 whitelist の 2 案を比較し、AYA さんの「主流 body / head は 3-4 製品で 90% カバー」観察に基づき **viewer 側 whitelist 案 (= avatar designer の協力不要、viewer 完結)** を採用。spec を `feature/aya-r20-avatar-skin-sss-spec-draft` で起票 (base: ayastorm-release、r19 とは独立)。次は P0 Survey (whitelist 実機サンプリング + GBuffer flag 拡張案検討 + SSS pass 配置点比較)
- 2026-05-13 (release positioning 確定): AYA さんと release 戦略合意 — **B 軸完走 = r19+r20 セット**として一括 tag/release、positioning は「**見た目を完全に AYAstorm らしさを打ち出す / Firestorm との差を見せる**」。当初想定の r19-r21 セットから r21+ (C 軸: カメラ表現) を切り離し、B 軸 2 リリースで「人物撮影で気づくレベルの違い」を打ち出す方針。Release Notes は技術詳細より「FS にはない AYAstorm の写真感」narrative を主軸にする想定
