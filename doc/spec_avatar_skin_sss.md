# アバター肌 SSS (subsurface scattering) 仕様書 (r20)

> **対象**: AYAstorm `v7.2.5-ayastorm-r20` (想定)
> **前提**: r14+ 視覚的リアリティ章 (光/視覚表現の新章) の一環
> **roadmap 整合**: r13 で音響章完結 → r14+ 軸を audio から視覚に移行、本書はその第一弾

## 1. 目的とスコープ

SL のアバター肌は従来 fully opaque のフラット拡散反射で描画される。実物の肌は表皮を透過した光が真皮内で散乱して戻る (subsurface scattering, SSS) ため、輪郭・耳・鼻翼・指などの薄い部位では**透けたような柔らかい赤みのにじみ**が出る。これが無いと「写真を撮るに値する空気」が成立しない (記憶: `project_ayastorm_visual_realism_chapter.md`)。

r20 はこの肌透過感を**画面空間 SSS (screen-space SSS)** で表現する。フルレンダ屋向けの高品位 SSS (path-traced / pre-integrated) ではなく、**deferred の照明結果に対する screen-space blur + diffuse 寄与**で十分な質感が得られることが先行事例 (Naughty Dog "The Last of Us"、Crytek "skin shader" 等) で確立されているため、deferred routing を持つ Firestorm/AYAstorm のレンダパスに自然に乗る。

主目的:

- アバターの**肌部位だけ**を screen-space SSS で blur (服・髪・装飾は影響を受けない)
- 自分アバターでも他人アバターでも**同じ見え方**が成立する
- ユーザーが UUID を調べたり Description を編集したりせず**右クリック 1 発で対象を学習**できる UX
- r14+ 視覚章の他施策 (光/雰囲気) と直交して動く

非対象 (r20 では触らない):

- pre-integrated SSS (texture-space) — 重い、deferred では割に合わない
- transmittance (耳/指の真の透過光) — r21+ 検討
- 髪/服への SSS 適用 — 本書のスコープ外、識別子戦略上「肌だけ」を取り出す前提
- 動的肌色 (race/skin tone 別パラメータ) — r21+

## 2. Phase 分割

| Phase | 内容 | 状態 |
|------|------|------|
| A | screen-space SSS blur pass (フルスクリーン、blur 半径 + strength のみ) | shipped (r20 ベース) |
| B | per-attachment CPU フラグ `mIsSSSTarget` (whitelist マッチ結果を `LLViewerObject` に持つ) | **完成** |
| E | **mesh UUID 軸 + 右クリック学習 UX** (本書 §4) | **完成** (B と統合して着手、識別子は UUID 単一) |
| C | GBuffer 「skin」フラグ (B/E の bit を deferred GBuffer に書き込み、SSS pass は per-pixel mask で判定) | 未着手 |
| D | glow 復元 (skin 部位だけ blur 前の lit を additive で戻し、SSS の眠さを補正) | 未着手 |

### 2.1 各 Phase の境界

- **A は完成しているが Phase C が来るまで全画面 blur** — 肌でない部位 (服/髪/背景) も blur される副作用が出るが、見栄えのインパクトを早期に確認するためあえて shipping
- **B + E は同時に完成** — CPU 側 wiring (`mIsSSSTarget`) と識別子 (mesh UUID + 右クリック学習 UX) を一体で実装。Phase A の shader は今は読んでいない
- **B/E 単体では視覚的効果はゼロ** — bit が立つだけ。Phase A は全画面 blur のまま、ただし whitelist UI と右クリック登録 UX は動作確認可能
- **C で初めて見え方が変わる** — GBuffer に skin bit を書き、SSS pass がそれをマスクとして使う
- **D は補正** — SSS で blur されると肌のハイライトが眠くなるので、blur 前の lit を skin 部位だけ additive で足し戻す

### 2.2 Phase 順序を変えた理由

当初は C/D を先に完成させてから E で識別子を拡張する予定だったが、**B と E を統合先行**に切替。理由:

- 識別子 (whitelist 内容と UX) は r20 の真の deliverable。先に固めると Phase C/D の検証時に「対象が正しく flag されているか」を疑わなくて済む (= shader バグと識別子バグの切り分けが楽)
- 右クリック登録 UX は shader を 1 行も触らず完結、ビルドサイクルが軽い (autobuild 10 分の重い shader 修正と分離)
- inventory item 名軸 (Phase B 初期構想) は他人で機能せず、shipping 後すぐに E で置換する運命だったため、最初から UUID 軸単一に倒した方が後戻り工数ゼロ

## 3. 識別子戦略の比較

「装着物のどれが肌か」を判定する識別子の候補を比較する。

| 軸 | 取得経路 | 自分 | 他人 | sim cost | 装着者協力 | rot 速度 | 採否 |
|-----|---------|------|------|---------|------------|----------|------|
| inventory item 名 | `getAttachmentItemName()` (gInventory) | ✓ | ✗ (gInventory ローカル限定) | 0 | 不要 | 中 | **却下** (他人で機能せず) |
| prim Description タグ `[SSS]` 等 | ObjectProperties (要 ObjectSelect) | ✓ | ✓ | 高 (prim 数発) | **必須** (店が全商品書き直し) | 低 | **却下** (店側コスト過大) |
| prim 名 (`mName`) | ObjectProperties (要 ObjectSelect) | ✓ | ✓ | 高 | 不要 | 中〜高 | **却下** (sim throttle + ヒット率) |
| **mesh asset UUID** | `getVolume()->getParams().getSculptID()` | ✓ | ✓ | **0** (ObjectUpdate に含まれる) | 不要 | **低** | **採用** |

### 3.1 各軸の評価

**inventory item 名**: `gInventory` は自分の所持品 DB なので他人の装着物は名前 `""` (空) が返る。当初 Phase B の初期実装案だったが、shipping 前に Phase E と統合する形で UUID 軸に置換。識別子は**単一軸 (UUID のみ)** で出荷する。

**prim Description タグ**: r11/r13 で確立した「配信者主導モデル」の系譜だが、肌/body 商品は SL の歴史を通じて何百万点出回っており、**今更すべての店に Desc を書き直させるのは現実的に不可能**。新作だけに opt-in を期待しても、ユーザー視点では「効く商品と効かない商品が混ざる」体験になり破綻する。

**prim 名 (ObjectProperties)**: sim 往復で取得可能、装着者協力不要だが、(a)視界内 N アバター × 数十〜数百 prim = packet 数千発で sim throttle に衝突、(b)装着者が prim 名を編集している可能性があり whitelist ヒット率が読めない、で却下。

**mesh asset UUID** ← **本書採用**: `ObjectUpdate` で sim から自動で流れてくる (mesh を描画するために必須) ため viewer ローカルで完結、sim 往復ゼロ、装着者協力不要。Maitreya Lara/Reborn、Legacy、Inithium Kupra、eBody、LeLutka heads、Genus 等のメジャー body/head は UUID 固定で出回っているため、ユーザーが右クリック登録で覚えさせれば**普通の SL 風景の 8〜9 割をカバー**できる。UUID は版違いで初めて変わるため Description より rot が圧倒的に遅い。

## 4. Phase E: mesh UUID 軸 + 右クリック学習 UX

### 4.1 UX

他人 (または自分) のアバターの装着物を右クリック → コンテキストメニュー「**Add to SSS whitelist**」(既に登録済みなら「**Remove from SSS whitelist**」) を選択 → 当該装着物の mesh UUID を whitelist cvar に追記/削除 → 既存の cvar 変更シグナルが全アバター再評価をトリガー → **その瞬間に同じ mesh を使っている全員に伝播**。

ユーザーは UUID を一度も見る必要がない。「綺麗な肌の人を見つけた → 右クリックで覚えさせる → 以後同じ body の全員に効く」だけ。

### 4.2 メニュー項目の出し分け

| 装着物の種類 | メニュー状態 |
|-------------|--------------|
| mesh prim、未登録 | 「Add to SSS whitelist」のみ enabled |
| mesh prim、登録済み | 「Remove from SSS whitelist」のみ enabled |
| 古い sculpt prim (mesh なし) | 両方 grey out (mesh UUID が取れない) |
| 通常 prim (box/sphere) | 両方 grey out (同上) |
| 他人 prim で select 不可 | メニュー自体非表示 (sim/owner 設定次第) |

判定は `commit.add("SSS.Add", ...)` / `enable.add("SSS.EnableAdd", ...)` / 同 Remove で C++ 側に登録 (`llviewermenu.cpp`)、判定処理は `SkinSSSMatcher::getMeshId()` + `isInWhitelist()` で完結。

### 4.3 メニュー設置場所

ペア (フラット文脈メニュー / パイメニュー) × ペア (自分/他人) で 4 ファイルすべてに登録:

| ファイル | 場所 |
|----------|------|
| `menu_attachment_self.xml` (フラット、自分) | メニュー末尾、separator の下に 2 項目並列 |
| `menu_attachment_other.xml` (フラット、他人) | メニュー末尾、separator の下に 2 項目並列 |
| `menu_pie_attachment_self.xml` (パイ、自分) | "More >" sub-pie 内に "SSS >" sub-pie を作り Add/Remove |
| `menu_pie_attachment_other.xml` (パイ、他人) | "More >" sub-pie 内に "SSS >" sub-pie を作り Add/Remove |

パイは ring slot 8 個制限があるため top-level ではなく "More" 内に階層化、フラットは末尾並列で配置。

### 4.4 mesh UUID 取得経路

```cpp
// static LLUUID SkinSSSMatcher::getMeshId(LLViewerObject* obj)
if (!obj || !obj->isSculpted() || !obj->getVolume())
    return LLUUID::null;
const LLVolumeParams& vp = obj->getVolume()->getParams();
if ((vp.getSculptType() & LL_SCULPT_TYPE_MASK) != LL_SCULPT_TYPE_MESH)
    return LLUUID::null;
return vp.getSculptID();
```

`isSculpted()` で sculpt/mesh prim か絞り、sculpt type が mesh のものだけ UUID を返す。テクスチャ sculpt と通常 prim は null 返却で右クリックメニューが grey out される。

### 4.5 Whitelist parser

`AYAR20AvatarSkinSSSWhitelist` text editor (1 行 1 件) は **UUID のみ**を受け付ける:

| 行のフォーマット | 動作 |
|------------------|------|
| 36 文字、dash 区切り、16 進文字のみ (`LLUUID::parseUUID` 成功) | `std::set<LLUUID>` に登録 |
| 空行 / 空白のみ / その他のフォーマット | 黙って drop (警告/エラーは出さない) |

初心者は右クリック学習だけ使い、上級者は text editor で UUID 一覧を編集 (削除/メモ追加) できる**2 層 UI** が成立する。コメント形式は未定義だが、無効な行は drop されるので `# my comment` のような prefix も無害に共存する。

### 4.6 マッチング pipeline

```
LLVOAvatar::attachObject()                  ←─┐
LLVOAvatar::detachObject()                  ←─┤  装着/取外し
cvar AYAR20AvatarSkinSSSWhitelist changed   ←─┘  (Add/Remove も含む)
  ↓
ayastorm::setSSSTargetForAttachment(viewer_object)
  ↓ (per-prim 評価)
SkinSSSMatcher::matches(LLViewerObject*)
  └─ getMeshId(obj) ∈ mUUIDs ?
  ↓
LLViewerObject::setSSSTarget(true/false)
  ↓ (Phase C 以降)
GBuffer skin bit に伝播
  ↓ (Phase A + Phase C mask)
screen-space SSS blur が skin pixel だけに適用
  ↓ (Phase D)
glow restore で hi-light の眠さを補正
```

cvar 変更時は `SkinSSSMatcher::reloadAndReevaluate()` が `LLCharacter::sInstances` を全走査して全アバター × 全 attachment を再評価するため、右クリック登録 → 即座に全員に伝播する。

## 5. 実装状況 (Phase B + E 完成時点)

### 5.1 追加ファイル

- `indra/newview/llayaskinsss.h/.cpp` — `SkinSSSMatcher` singleton (cvar 監視 + UUID パース + 全アバター再評価)、static `getMeshId(LLViewerObject*)`、`addUUID()` / `removeUUID()` / `isInWhitelist()` API、`setSSSTargetForAttachment` / `clearSSSTargetForAttachment` 自由関数

### 5.2 既存ファイル変更

- `indra/newview/llviewerobject.h` — `mIsSSSTarget` 追加、`isSSSTarget()` / `setSSSTarget()` accessor 追加
- `indra/newview/llvoavatar.cpp` — `attachObject()` 末尾で `setSSSTargetForAttachment()` 呼出 (isSelf() ガードなし — UUID 軸は他人にも効く)、`detachObject()` で `clearSSSTargetForAttachment()` 呼出
- `indra/newview/llviewermenu.cpp` — `SSS.Add` / `SSS.Remove` (commit) と `SSS.EnableAdd` / `SSS.EnableRemove` (enable) の 4 ハンドラ登録、`LLSelectMgr::getSelection()->getPrimaryObject()` から mesh UUID を抽出して `SkinSSSMatcher` にディスパッチ
- `indra/newview/CMakeLists.txt` — 新規 2 ファイルを source/header list に追加
- `indra/newview/llviewershadermgr.cpp` — shader cache 鍵に `AYASTORM_SHADER_CACHE_TAG = "AYAstorm r20"` を mix-in (r-bump 時に compiled shader cache を自動無効化)
- `indra/newview/skins/default/xui/en/panel_preferences_sss.xml` — Preferences > Graphics > SSS タブ、ラベルを UUID 軸表現に更新 ("Target mesh UUIDs", hint "One UUID per line")
- `indra/newview/skins/default/xui/en/menu_attachment_self.xml` / `menu_attachment_other.xml` — フラット文脈メニュー末尾に "Add to SSS whitelist" / "Remove from SSS whitelist" を追加
- `indra/newview/skins/default/xui/en/menu_pie_attachment_self.xml` / `menu_pie_attachment_other.xml` — "More >" sub-pie 内に "SSS >" sub-pie (Add / Remove) を追加
- `indra/newview/app_settings/settings.xml` — `AYAR20AvatarSkinSSSWhitelist` Comment を UUID 軸表現に更新

### 5.3 Phase B + E の到達点と残課題

到達:

- 自分 / 他人どちらのアバターでも mesh UUID 経由で `mIsSSSTarget` が flip
- 右クリック 1 発で whitelist 登録 (cvar 変更シグナル → 全アバター再評価が即座に走る)
- Preferences の text_editor で手動 UUID 編集も可能 (2 層 UI)

残課題 (本書のスコープ内、Phase C 着手前に拾うか判断):

- **Lock checkbox** (whitelist 誤編集防止) は配置済みだが text_editor の read-only ゲートが未配線
- spinner 横の **「Default」ボタン × 2** (Blur radius / Strength) は配置済みだが handler 未配線
- **「Reset all to defaults」ボタン**は配置済みだが handler 未配線
- Phase A の SSS shader は `mIsSSSTarget` を読んでおらず**現状は全画面 blur** (Phase C で per-pixel mask に置換)
- 肌でない服や髪も blur される (副作用、Phase C のマスクで解消)

## 6. オープン課題

### 6.1 mesh UUID シード list (任意)

右クリック登録 UX が動くため**シード list 同梱は必須ではなくなった** (各ユーザーが必要に応じて育てればよい)。ただしメジャー body/head の UUID を `settings.xml` の Value 初期値に詰めて出荷すれば、ユーザーの初回体験が「いきなり見えのいい部位だけ blur」になり訴求力が増す。候補:

- Maitreya Lara / Reborn (body)
- Legacy / Legacy Perky (body)
- Inithium Kupra / Khara (body)
- eBody Reborn (body)
- LeLutka Evolution heads (Lilly/Avalon/etc)
- Genus Project heads
- Catwa heads (legacy だが利用者まだ多い)

シード収集方法は別途検討 (自分のインベントリから計測 / community 投稿)。Phase C/D で見え方が確定してから判断。

### 6.2 mesh UUID list の community 維持

UUID 一覧の保守は anti-griefer block list の保守と同程度の運用負荷を想定。新作 body リリースのたびに list を更新する仕組み (Discord 通知 / GitHub PR 等) は未設計。シード list を出荷するか否かで運用設計の必要度が変わるため、§6.1 と合わせて判断する。

### 6.3 Preferences パネルの未配線

§5.3 残課題のうち UI 配線の 3 件 (Lock checkbox の read-only ゲート、Default ボタン × 2、Reset all ボタン):

- 機能的に欠けても r20 出荷可能 (デフォルト値で動く、whitelist 直編集も可能、Reset は cvar を 1 つずつ Ctrl+R で対応)
- ただしユーザーの「あれ?ボタン押したのに反応しない」体験は減点要因
- shader 工程 (Phase C/D) と独立に進められるので、待ち時間に詰めると総合工数が減る

### 6.4 Phase C/D へ移る前のチェックリスト

C 着手前に確認したいこと:

- 右クリック Add → text editor に UUID が 1 行追記される
- 右クリック Remove → text editor から該当 UUID 行が消える
- text editor に手動で UUID を貼ると登録される (commit_on_focus_lost で発火)
- 同じ mesh を使う他アバターでも `mIsSSSTarget` が flip しているか (現状は外から見えない、簡易 stat 表示か LL_INFOS で確認可能)
- 装着/取外し時に `setSSSTargetForAttachment` / `clearSSSTargetForAttachment` が呼ばれる (LL_INFOS hook で確認)

## 7. 関連記憶 (LLM persistent memory)

- `project_ayastorm_visual_realism_chapter.md` — 視覚的リアリティ章の core thesis (光は入口、LUT/Tone では届かない根本)
- `project_ayastorm_r14_pivot_to_light.md` — r14+ で音→光軸への移行
- `project_aya_visual_realism_alpha_protect.md` — 視覚章 post-pass は scene buffer alpha を破壊しない
- `reference_deferred_shader_routing.md` — オブジェクト → Pool → bound shader → gbuffer flag → softenLightF 分岐の確定マップ (Phase C 着手前に必読)

## 8. r20 リリース時点での宣伝ポイント

- SL viewer 史上初の**アバター肌 SSS** (画面空間 SSS、deferred routing にネイティブ統合)
- 自分にも他人にも効く (mesh UUID 軸統一、Phase B/E 統合済み)
- ユーザーは UUID を 1 度も見ない (右クリックで学習、cvar 変更が即座に全アバターへ伝播)
- 設定 3 つ (Enable / Blur radius / Strength) + Whitelist で完結
