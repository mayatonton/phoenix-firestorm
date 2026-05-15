# AYAstorm r22 — Chat tab split (人間 vs Object/LSL) 仕様ドラフト

**作成日**: 2026-05-15
**ステータス**: 仕様検討中 (実装未着手)
**対象ブランチ**: `docs/ayastorm-r22-chat-tab-spec`

このドキュメントは r22 で追加予定の「Chat 表示の人間/Object タブ分離」機能の仕様検討メモ。AYA さんとの対話で確定した部分と、次セッションで詰める論点をまとめている。

---

## 1. 狙い

Nearby Chat / IM の表示において、**人間アバター発言** と **Object (LSL) 由来発言** を別タブに分離する。

人間同士の会話に LSL からの警告 / 広告 / HUD 通知が混入して「会話の流れが途切れる」状況を解消するのが主目的。Object 発言を完全に消すのではなく、別タブに退避させて見落としは防ぐ。

---

## 2. スコープ

| 項目 | 対象 / 対象外 |
|---|---|
| Nearby Chat (Local Chat) | ✅ 対象 |
| IM (1 on 1) — 通常 | ✅ 対象 (受信側で判定) |
| IM (1 on 1) — `IM_FROM_TASK` (Object 発信) | ✅ Object タブ |
| Group chat | 🔶 基本 Human (Object 発言が来ない経路)、要再確認 |
| System message (sim restart / Second Life messages) | ⏸️ 未確定 (§7) |
| 対象 style: FS V1 / V7 / LL | ⏸️ 未確定 (§7) — 構造上は 3 style 同時対応が可能 |

---

## 3. 表示構造 (GUI)

### 3.1 確定: タブ切替式

各 chat ウィンドウの history 領域上部に **タブ** を配置する。

```
┌─────────────────────────────┐
│ [Human] [Object (3)]        │ ← 非アクティブ側に未読件数バッジ
├─────────────────────────────┤
│  (選択中タブの history)     │
├─────────────────────────────┤
│  > 入力欄                   │ ← 共有 1 つ
└─────────────────────────────┘
```

- タブは `[Human]` `[Object]` の 2 つ
- 非アクティブ側に **未読件数バッジ** を表示し見落としを防ぐ
- 入力欄は 1 つ共有 (どちらのタブを見ていても Local Chat に送信)
- 自分の Local Chat 発言は **Human タブに記録**
- V1 / V7 / LL の 3 style とも同じ形状で揃える (style 切替時の体験を一貫させるため)

### 3.2 見た目フォールバック案 (実機検証で A が詰まった時の二の矢)

実機で「タブが見た目的に変」と判明したら、以下の順に検討:

| Plan | 場所 | 形 | 採用しやすさ |
|---|---|---|---|
| **A タブ (本命)** | history 上部 | `[Human] [Object (3)]` 横並び | — |
| **B-1 ツールバートグル** | 既存 Muted ボタンの隣 | `[H/O]` トグル | ◎ (既存 Muted パターン踏襲) |
| B-2 右上ミニタブ | ヘッダ右端 | 小さく `[Human] [Object]` | ○ |
| B-3 combo_box | ツールバー | `[Show: Human ▼]` ドロップダウン | △ (ステップ数増) |

→ **B-1 が最有力フォールバック**。既存 `chat_history` / `chat_history_muted` の visibility_control 切替パターン (§5.3 参照) と完全一致するため、実装/見た目とも違和感が出にくい。

### 3.3 必須要件: 表示色維持 (タブ切替で文字色を変えない)

Firestorm の `fs_chat_history` widget は行ごとに色が固定される:

| 行の出所 | 色 |
|---|---|
| 起動時に history load した過去ログ | `ChatHistoryTextColorPersisted` (グレー) |
| session 中に受信した発言 | `ChatHistoryTextColor` (通常) |

色は **append された時点で決まる** ので、後から変わらない。問題が起きるのは「タブ切替時に widget を reload する」粗い実装をした場合のみ。

→ **必須実装方針**:

- Human / Object の `fs_chat_history` widget を **最初から両方常駐** させる
- 起動時の history load は両 widget に同時振り分け (どちらも persisted color = グレー)
- chat 受信 hook で source_type を見て対応 widget だけに append (通常色)
- **タブ切替は visible/invisible swap のみ** — widget 内容は一切触らない

これで「タブ往復で色がグレーになる」事故を構造的に回避できる。既存の `chat_history` / `chat_history_muted` パターンと同じ理屈。

---

## 4. データ層

### 4.1 履歴ファイルは 1 本のまま維持

| 種類 | ファイル | 方針 |
|---|---|---|
| Nearby Chat 履歴 | `chat-LOCAL-CHAT.txt` | **1 本のまま維持**、物理分離しない |
| IM 履歴 | `chat-{user_uuid}.txt` | **1 本のまま維持**、物理分離しない |

理由:
- 既存利用者の履歴アーカイブと外部 viewer / grep の連続性を壊さない
- Object タブを後から消したり Human と統合する自由度が残る (ファイル構造を一切変えない)
- 履歴ファイル数が倍増しない
- 上流 Firestorm が同じファイルを開いても破綻しない

### 4.2 行末メタマーカー

新規書き込み行の末尾に source_type を埋め込む。

**フォーマット例**:

```
[2026-05-15 10:23] Alice: hello\t<!--src:avatar-->
[2026-05-15 10:24] PetBox: greetings, traveler!\t<!--src:task-->
```

**source_type の値** (拡張余地あり):

| 値 | 意味 |
|---|---|
| `avatar` | 人間アバターの発言 (Human タブ) |
| `task` | LSL/Object 発言 (Object タブ) |
| `system` | System message (振り分けは §7 で確定) |

**HTML コメント風 (`<!-- -->`) にした理由**:
- 外部 viewer / エディタで直接開いても **意味のあるコメント記法に見え害が薄い** (`[TASK]` プレフィックスより目立たない)
- `grep "src:task"` のような検索 noise になりにくい
- **上流 Firestorm で同じ履歴ファイルを開いても破綻しない** (ただの末尾文字列として見える)

### 4.3 読み込み時の挙動

`LLLogChat::loadHistory` が行を読む際:

1. 行末の `<!--src:.*-->` を **正規表現で抽出してメタフィールドに分離**
2. 本文文字列からはマーカーを **剥がしてから** chat widget に渡す
3. メタに従って Human / Object widget に振り分け

→ **AYAstorm を介する限り、ユーザー画面にマーカー文字列は絶対出ない**。

### 4.4 旧履歴 (マーカーなし) のフォールバック

マーカーがない行は判別不能 → **Human タブにフォールバック**。

理由:
- 過去ログは大半が人間会話なので Human 側に流すのが安全
- Object 側に巻き込むと検索性が悪化する
- 「判別つかないものは Human」が default safe

---

## 5. 実装スケッチ (構造のみ)

### 5.1 影響ファイル想定

**XUI**:

- `indra/newview/skins/default/xui/en/panel_nearby_chat.xml` — Human/Object 2 widget + tab UI を配置
- `indra/newview/skins/default/xui/en/floater_fs_nearby_chat.xml` — 既存 muted/normal 2 widget パターンに準拠して visibility 切替で実装可能 (要検討)
- `indra/newview/skins/default/xui/en/floater_im_session.xml` — IM 側のタブ配置 (要構造調査)
- `indra/newview/skins/default/xui/en/floater_im_container.xml` — LL style 側で panel_nearby_chat をどう埋め込んでいるか要確認

**C++**:

- `LLLogChat` — 書き込み時にマーカー付与、読み込み時にマーカー抽出 + メタ分離 (core)
- `LLFloaterNearbyChat` / `LLFloaterIMSession` — chat 受信 hook で source_type 判定して widget 振り分け
- `LLChat` struct — source_type 拡張 or 新 enum 追加

### 5.2 既存の参考パターン (viewer 内)

- **`chat_history` / `chat_history_muted` 2 widget + `visibility_control` 切替** — `floater_fs_nearby_chat.xml` に既存。「複数の chat_history を切り替える」設計の前例
- **`tab_container` widget** — viewer 内で 20+ ファイル使用 (Preferences の Chat/Graphics/Privacy 等)。熟成済みなので技術的リスクは低い

### 5.3 構造調査メモ (2026-05-15 時点)

XUI を覗いた結果、タブ差し込みは構造的に十分可能と判断:

- `panel_nearby_chat.xml` は単純構造 (`layout_stack > layout_panel > fs_chat_history`) で差し込み余地あり
- `floater_fs_nearby_chat.xml` は既に 2 つの chat_history を visibility_control で切り替える設計を採用済み
- `panel_nearby_chat.xml` を LL style と FS V1/V7 の両方が共有しているなら、panel 側に手を入れるだけで 3 style 同時対応できる (要 `floater_im_container.xml` 確認)

---

## 6. 受入基準 (実装後)

- [ ] Human タブに人間アバター発言、Object タブに LSL/Object 発言が分離して表示される
- [ ] 旧履歴 (マーカーなし) は Human タブにフォールバック
- [ ] タブ切替で session 中の発言色が維持される (グレーにならない)
- [ ] 起動時の history load は両タブともグレー (persisted color) で表示
- [ ] 非アクティブタブに未読件数バッジが表示される
- [ ] 自分の Local Chat 発言は Human タブに記録される
- [ ] AYAstorm を介してユーザー画面にマーカー文字列が漏れない
- [ ] 上流 Firestorm で同じ履歴ファイルを開いても破綻しない (末尾文字列として見える)
- [ ] FS V1 / V7 / LL の 3 style すべてでタブが機能する (LL 優先で進めるなら別途段階分け)
- [ ] IM (1 on 1) の `IM_FROM_TASK` 由来発言が Object タブに分離される
- [ ] 3 OS (Linux / Win / Mac) でビルド通過

---

## 7. 未確定論点 (次セッションで詰める)

### 7.1 判定境界

「どこから Object 扱いにするか」の境界線:

| ケース | 仮判定 | 確定要 |
|---|---|---|
| 通常の avatar chat | Human | ✓ |
| LSL `llSay` / `llShout` / `llWhisper` / `llRegionSay` | Object | ✓ |
| HUD scripted attachment (自分の HUD からの発言) | ⏸️ | 未確定 |
| 自分の scripted attachment (他人にも見える) | ⏸️ | 未確定 |
| 他人の attachment 内 object 発言 | Object | ✓ |
| System message (sim restart, Second Life messages) | ⏸️ | 未確定 |
| Group notice | ⏸️ | 未確定 |
| `IM_FROM_TASK` (Object → 自分) | Object | ✓ |
| Bot avatar (人間が AI 制御) | Human (区別不可) | ✓ |

### 7.2 その他

- **対象 style の段階分け**: V1 / V7 / LL を r22 で一気に対応するか、LL 優先で V1/V7 は r23 に分けるか
- **opt-in / opt-out のデフォルト**: 初回利用時に Human/Object 分離を on にするか off にするか (memory `feedback_prefer_defaults_over_config.md` を踏まえて「無難なデフォルト」を選ぶ)
- **「Object タブを完全に hide できる隠しトグル」の要否** — タブ自体を消して旧挙動 (1 widget) に戻す escape hatch
- **重要 LSL を Human タブに混ぜる allow list の要否** — 例: 自分の HUD だけ Human タブに流す
- **`floater_im_container.xml` の panel_nearby_chat 埋め込み確認** — LL style 側の構造確定
- **バッジの設計詳細** — 件数表示 (`Object (3)`) vs 単なるドット表示の選択

---

## 8. リスク登録

| リスク | 影響 | 対策 |
|---|---|---|
| タブの見た目が実機で詰む | 中 | フォールバック B-1〜B-3 を仕様に明記済み |
| 履歴ファイルの行末マーカー regex が既存パース処理 (URL 抽出等) と衝突 | 低 | 実装時に URL/highlight regex と末尾マーカー regex の優先順位を確認 |
| LL style の panel_nearby_chat 埋め込み構造が想定と違う | 低-中 | 実装着手前に `floater_im_container.xml` を読んで確定 |
| マーカー付き履歴を上流 Firestorm で開いた時の挙動 | 低 | 末尾文字列なので表示には漏れる可能性あり (パースされないので無害)、§4.2 参照 |
| 「Human/Object 判定境界」のエッジケースで誤分類 | 中 | §7.1 を実装前に確定、ユーザーフィードバックで微調整 |

---

## 9. 履歴

- **2026-05-15**: ドラフト作成。AYA さんとの設計検討対話 (GUI 案 α 採用、データ層 1 本維持 + suffix marker、表示色維持の必須要件) を 1 ページにまとめ。判定境界とスコープ詳細は §7 に未確定として記録。
