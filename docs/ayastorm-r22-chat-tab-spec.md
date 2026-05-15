# AYAstorm r22 — Chat tab split (人間 vs Object/LSL) 仕様

**作成日**: 2026-05-15
**最終更新**: 2026-05-16 (M7 完了)
**ステータス**: M1〜M7 完了、M8 (Release) 着手前
**対象ブランチ**: `feat/ayastorm-r22-chat-tab-split`

このドキュメントは r22 で追加予定の「Chat 表示の人間/Object タブ分離」機能の仕様。AYA さんとの対話で確定した内容を集約。M1 セッション (2026-05-15) で主要論点をすべて確定済み。

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
| Group chat | ✅ 基本 Human (Object 発言が来ない経路) |
| 対象 style: FS V1 / V7 / LL | ✅ **3 style 同時対応** (M1 確定) |

---

## 3. 表示構造 (GUI)

### 3.1 確定: タブ切替式 + Preferences スイッチ

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

**仕様**:
- タブは `[Human]` `[Object]` の 2 つ
- 非アクティブ側に **未読件数バッジ** を表示し見落としを防ぐ
  - 形式は `[Object (3)]` の件数表示 (M5 で確定)
  - タブ切替で 0 にリセットしラベルを `Object` に戻す
  - **セッション内のみ。再起動でリセット** (履歴ファイルには残るので件数自体は意味薄、と判断)
- 入力欄は 1 つ共有 (どちらのタブを見ていても Local Chat に送信)
- 自分の Local Chat 発言は **Human タブに記録**
- V1 / V7 / LL の 3 style とも同じ形状で揃える (UX 一貫性)

**設定スイッチ** (M1 確定):

| Name | Type | Default | Persist |
|---|---|---|---|
| `FSChatHumanObjectTabs` | Boolean | `true` | 1 |

- **opt-out (default ON)** — AYAstorm の目玉機能として default で有効
- 配置: `Preferences → Chat → Chat Windows` タブ (LL chat style 切替と同じタブ)
- ラベル: `Split chat into Human / Object tabs`
- tool_tip: `Separate human avatar speech from LSL/object messages into two tabs. Keeps human conversations free of script notifications.`
- `false` にすると旧 1-widget 挙動に戻る (escape hatch 兼用、別 cvar 不要)

### 3.2 見た目フォールバック案 (実機検証で α が詰まった時の二の矢)

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

## 4. 判定境界 (M1 確定)

`EChatSourceType` (llchat.h:35) の 6 値を以下に振り分ける:

| Source Type | 例 | 振り分け |
|---|---|---|
| `CHAT_SOURCE_AGENT` | アバター発言 | **Human** |
| `CHAT_SOURCE_OBJECT` | LSL `llSay` / `llOwnerSay` / HUD / 自分の attachment | **Object** |
| `CHAT_SOURCE_SYSTEM` | "You are now logged in", item received 等 | **Human** |
| `CHAT_SOURCE_TELEPORT` | TP offer / arrival | **Human** |
| `CHAT_SOURCE_REGION` | sim restart 通知等 | **Human** |
| `CHAT_SOURCE_UNKNOWN` | 不明 (fallback) | **Human** |

**判定方針**:
- Object タブには **「LSL 由来のうるさい話し声 (広告・警告・繰り返し通知)」だけ** を集める
- System / Teleport / Region / Unknown は性質が違う (頻度低い・重要) ので Human 側 = メインの流れに残す
- 「判別不能 → Human フォールバック」を default safe としても機能

---

## 5. データ層

### 5.1 履歴ファイルは 1 本のまま維持

| 種類 | ファイル | 方針 |
|---|---|---|
| Nearby Chat 履歴 | `chat-LOCAL-CHAT.txt` | **1 本のまま維持**、物理分離しない |
| IM 履歴 | `chat-{user_uuid}.txt` | **1 本のまま維持**、物理分離しない |

理由:
- 既存利用者の履歴アーカイブと外部 viewer / grep の連続性を壊さない
- Object タブを後から消したり Human と統合する自由度が残る (ファイル構造を一切変えない)
- 履歴ファイル数が倍増しない
- 上流 Firestorm が同じファイルを開いても破綻しない

### 5.2 行末メタマーカー

新規書き込み行の末尾に source_type を埋め込む。

**フォーマット例**:

```
[2026-05-15 10:23] Alice: hello\t<!--src:avatar-->
[2026-05-15 10:24] PetBox: greetings, traveler!\t<!--src:task-->
[2026-05-15 10:25] Second Life: You are now logged in.\t<!--src:system-->
```

**source_type の値** (`EChatSourceType` 既存定義からマッピング):

| Marker 値 | 対応する EChatSourceType |
|---|---|
| `avatar` | `CHAT_SOURCE_AGENT` |
| `task` | `CHAT_SOURCE_OBJECT` |
| `system` | `CHAT_SOURCE_SYSTEM` |
| `teleport` | `CHAT_SOURCE_TELEPORT` |
| `region` | `CHAT_SOURCE_REGION` |
| `unknown` | `CHAT_SOURCE_UNKNOWN` |

→ ファイル上にすべての種類を残しておくことで、将来 Human/Object 以外の細分化 (例: System タブを別に分ける) に対応できる柔軟性を確保。

**HTML コメント風 (`<!-- -->`) にした理由**:
- 外部 viewer / エディタで直接開いても **意味のあるコメント記法に見え害が薄い** (`[TASK]` プレフィックスより目立たない)
- `grep "src:task"` のような検索 noise になりにくい
- **上流 Firestorm で同じ履歴ファイルを開いても破綻しない** (ただの末尾文字列として見える)

### 5.3 読み込み時の挙動

`LLChatLogParser::parse()` が行を読む際:

1. 行末の `<!--src:(\w+)-->` を **正規表現で抽出して LLSD に `source` キーとして詰める**
2. 本文文字列からはマーカーを **剥がしてから** 後続パイプラインに渡す
3. メタに従って Human / Object widget に振り分け

→ **AYAstorm を介する限り、ユーザー画面にマーカー文字列は絶対出ない**。

### 5.4 旧履歴 (マーカーなし) のフォールバック

マーカーがない行は判別不能 → **Human タブにフォールバック**。

理由:
- 過去ログは大半が人間会話なので Human 側に流すのが安全
- Object 側に巻き込むと検索性が悪化する
- 「判別つかないものは Human」が default safe

---

## 6. 実装スケッチ

### 6.1 影響ファイル

**XUI** (M3 着手時に構造再調査した実態を反映):

- `indra/newview/skins/default/xui/en/floater_fs_nearby_chat.xml` — FS V1/V7 用、`tab_container` で `tab_human` / `tab_object` を抱える
- `indra/newview/skins/default/xui/en/floater_im_session.xml` — LL style 用 (Nearby Chat と 1:1 IM の両方が同じ XUI を共有)、同じく `tab_container` を持つ
- `indra/newview/skins/default/xui/en/panel_preferences_chat.xml` — `FSChatHumanObjectTabs` スイッチ + AYAChatWindowStyle の (requires restart) ラベル
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `ChangeChatLayoutSetting` モーダル (M4-extra)
- ~~`panel_nearby_chat.xml`~~ — **dead/未参照と判明** (M3 着手時に確認)、touch しない

**C++** (実装後の実態):

- `indra/llui/llchat.h` — `LLChat::mSourceType` (既存 `EChatSourceType`) をそのまま利用、新規 enum 追加なし
- `indra/newview/lllogchat.cpp`:
  - `LLChatLogFormatter::format()` — suffix marker 出力
  - `LLChatLogParser::parse()` — suffix marker 抽出 + 本文剥がし。マーカーがある行だけ `LL_IM_SOURCE_TYPE` を立てる (legacy 行は未設定のまま、load 側の heuristic に委ねる)
- `indra/newview/fsfloaternearbychat.cpp` / `.h` — FS 側の addMessage で source_type 振り分け + 未読バッジ更新
- `indra/newview/llfloaterimsessiontab.cpp` / `.h` — LL Nearby Chat と 1:1 IM の基底クラス、appendMessage で同じく振り分け + バッジ更新
- `indra/newview/llfloaterimnearbychat.cpp` — load path で `LL_IM_SOURCE_TYPE` を尊重
- `indra/llui/lltabcontainer.h` — `setTabsHidden()` を public へ昇格 (`FSChatHumanObjectTabs=false` 時のタブ strip 抑止に使用)
- `indra/newview/llviewercontrol.cpp` — `AYAChatWindowStyle` / `FSChatHumanObjectTabs` 切替時の `ChangeChatLayoutSetting` モーダル発火 + 旧スタイルの IM コンテナ自動クローズ + `floater_vis_*` クリア (M4-extra)

**Settings**:

- `indra/newview/app_settings/settings.xml` — `FSChatHumanObjectTabs` Boolean default true persist 1

### 6.2 既存の参考パターン (viewer 内)

- **`chat_history` / `chat_history_muted` 2 widget + `visibility_control` 切替** — `floater_fs_nearby_chat.xml` に既存。「複数の chat_history を切り替える」設計の前例
- **`tab_container` widget** — viewer 内で 20+ ファイル使用 (Preferences の Chat/Graphics/Privacy 等)。熟成済みなので技術的リスクは低い
- **`EChatSourceType`** — `indra/llui/llchat.h:35` で既に 6 値定義済み、15+ファイルで使用 (`fschathistory.cpp` / `llimprocessing.cpp` / `llviewermessage.cpp` 等)

### 6.3 構造調査メモ (M3 で確定)

XUI / C++ を覗いた結果:

- `panel_nearby_chat.xml` は **dead/未参照** だった (上流ですでに切り離されていた)。本 r22 では touch しない
- `floater_fs_nearby_chat.xml` は既に 2 つの chat_history を切り替える設計を採用済み、ここに `tab_container` を被せて Human/Object パネルに分割
- LL style の Nearby Chat と 1:1 IM はどちらも `floater_im_session.xml` を共有してロードされる (Nearby Chat も IM session の一種扱い) ため、ここに `tab_container` を入れることで **M4 単発で M6 (IM 1:1 対応) も実質カバーされる**
- `EChatSourceType` が既に LLChat に存在しており、source_type の判定ロジックは新規実装不要
- `LLLogChat::saveHistory()` のシグネチャに source_type が無いので拡張要

---

## 7. 受入基準 (実装後)

- [x] Human タブに人間アバター発言、Object タブに LSL/Object 発言が分離して表示される
- [x] 旧履歴 (マーカーなし) は Human タブにフォールバック
- [x] タブ切替で session 中の発言色が維持される (グレーにならない)
- [x] 起動時の history load は両タブともグレー (persisted color) で表示
- [x] 非アクティブタブに未読件数バッジ `(N)` が表示される (セッション内のみ、再起動でリセット)
- [x] 自分の Local Chat 発言は Human タブに記録される
- [x] AYAstorm を介してユーザー画面にマーカー文字列が漏れない
- [x] 上流 Firestorm で同じ履歴ファイルを開いても破綻しない (末尾文字列として見える)
- [x] FS V1 / V7 / LL の 3 style すべてでタブが機能する (Linux / Win / Mac 全 OS で確認)
- [x] IM (1 on 1) の `IM_FROM_TASK` 由来発言が Object タブに分離される (M4 で共有 XUI 経由実装、M6 で確認)
- [x] System / Teleport / Region / Unknown 発言は Human タブに流れる
- [x] `Preferences → Chat → Chat Windows` に `FSChatHumanObjectTabs` スイッチが表示される
- [x] `FSChatHumanObjectTabs=false` で旧 1-widget 挙動に戻る (escape hatch)
- [x] AYAChatWindowStyle / FSChatHumanObjectTabs 切替時に再起動誘導モーダルが出る (M4-extra)
- [x] AYAChatWindowStyle 切替時に旧スタイルの IM コンテナが自動で閉じる (M4-extra)
- [x] 3 OS (Linux / Win / Mac) でビルド通過 + 動作確認 (M7)

---

## 8. 残る未確定論点

| 論点 | 優先 | 対処タイミング | 状況 |
|---|---|---|---|
| バッジ詳細 (件数表示 vs ドット) | 低 | M5 実装中に決める | ✅ 件数表示 `(N)` で確定、セッション内のみ |
| `floater_im_container.xml` の panel_nearby_chat 埋め込み確認 | 中 | M2 着手前に最終確認 | ✅ `panel_nearby_chat.xml` は dead と判明 (M3) |
| HUD allow list (自分の HUD だけ Human に流す) | 低 | **r22 スコープ外**、r23+ で検討 | 据え置き |

`FSChatHumanObjectTabs=false` で 1-widget 旧挙動に戻れるため、別の escape hatch cvar は **不要**。

---

## 9. リスク登録

| リスク | 影響 | 対策 |
|---|---|---|
| タブの見た目が実機で詰む | 中 | フォールバック B-1〜B-3 を §3.2 に明記済み |
| 履歴ファイルの行末マーカー regex が既存パース処理 (URL 抽出等) と衝突 | 低 | 実装時に URL/highlight regex と末尾マーカー regex の優先順位を確認 |
| LL style の `floater_im_container.xml` / `floater_im_session.xml` の Nearby Chat 統合構造が想定と違う | 中 | M2 着手前に再確認、想定外なら影響範囲を再評価 |
| マーカー付き履歴を上流 Firestorm で開いた時の挙動 | 低 | 末尾文字列なので表示には漏れる可能性あり (パースされないので無害)、§5.2 参照 |
| `LLChatLogFormatter` / `LLChatLogParser` の上流互換性 | 中 | suffix marker は optional、なくても既存 parse が壊れない設計を保つ |

---

## 10. マイルストーン

| M | 内容 | ステータス |
|---|---|---|
| M1 | 仕様確定 + 構造調査 | ✅ 完了 (2026-05-15) |
| M2 | データ層実装 (LLLogChat + LLChat suffix marker) | ✅ 完了 |
| M3 | XUI — `floater_fs_nearby_chat.xml` / `floater_im_session.xml` に `tab_container` | ✅ 完了 (panel_nearby_chat.xml は dead/未参照と判明、touch しない方針に変更) |
| M4 | C++ 受信 hook で widget 振り分け + M4-extra (restart-only モーダル + 旧スタイル floater 自動クローズ) | ✅ 完了 |
| M5 | 未読バッジ (`(N)` 件数表示、セッション内のみ、再起動でリセット) | ✅ 完了 |
| M6 | IM (1 on 1) — `floater_im_session.xml` 対応、`IM_FROM_TASK` 判定 | ✅ 完了 (LSL `llInstantMessage` 経由で IM_FROM_TASK が Object タブに分離されることを確認) |
| M7 | 3 OS ビルド (Linux → Win → Mac) | ✅ 完了 (2026-05-16、3 OS ビルド + 動作確認 PASS) |
| M8 | Release — spec doc 更新、release note 3 言語、tag | ⏳ 着手前 |

---

## 11. 履歴

- **2026-05-15 (初版)**: ドラフト作成。AYA さんとの設計検討対話 (GUI 案 α 採用、データ層 1 本維持 + suffix marker、表示色維持の必須要件) を 1 ページにまとめ。判定境界とスコープ詳細は §7 に未確定として記録。
- **2026-05-15 (M1 完了)**: M1 で以下を確定:
  - 対象 style: FS V1 / V7 / LL の **3 style 同時対応**
  - 判定境界: `CHAT_SOURCE_AGENT` → Human、`CHAT_SOURCE_OBJECT` → Object、`SYSTEM` / `TELEPORT` / `REGION` / `UNKNOWN` → Human
  - opt-in/out: **opt-out (default ON)**
  - 設定 cvar: `FSChatHumanObjectTabs` (Boolean, default true, persist 1)、配置 `Preferences → Chat → Chat Windows`
  - UI ラベル: `Split chat into Human / Object tabs`
  - escape hatch: 上記 cvar の false 化で代用、別 cvar 不要
  - HUD allow list: r22 スコープ外、r23+ で検討
  - 構造調査の結果、`EChatSourceType` が既存定義済みで判定ロジックは新規不要、`LLLogChat::saveHistory()` シグネチャ拡張要、LL style は `panel_container` で session 切替する構造
- **2026-05-16 (M7 完了)**: Windows + macOS でもビルド通過 + 動作確認 PASS。3 OS で受入基準を全項目クリア、M8 (Release) に移行。
