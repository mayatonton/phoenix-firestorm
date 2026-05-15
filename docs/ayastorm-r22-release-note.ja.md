# AYAstorm r22 — リリース告知文案

GitHub release ページ貼り付け用の文案。**r22 は Chat 表示の UX 改良リリース**で、Nearby Chat / IM の history を「人間アバター発言」と「System & Object 系通知 (LSL 発言 + システム通知 + TP / Region)」の 2 タブに分離します。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r22-chat-tab-spec.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r22 — Chat tab split (Human vs System & Object)

### r22 の柱: 会話と通知を別タブに分けて流れを切らない

r21 までは Nearby Chat / IM の history が 1 本の widget で、人間同士の会話に LSL 由来の警告 / 広告 / HUD 通知に加えて TP 着地・region restart などのシステム通知も混ざり、会話の流れを切ってしまう状況がありました。これらを完全に消すと見落としが出るため、r22 では **2 タブに分離して退避させる** アプローチを採用しています。

- `[Human]` タブ: アバター発言 (会話) のみ
- `[System & Object]` タブ: LSL `llSay` / `llRegionSay` / `IM_FROM_TASK` / システム通知 / TP / Region notice 等、**会話以外すべて**
- 非アクティブタブには **未読件数バッジ `(N)`** を表示 (タブ切替で 0 にリセット、セッション内のみ)
- 自分の Local Chat 発言は **Human タブに記録**
- 入力欄は 1 つ共有 (どちらのタブを見ていても Local Chat に送信)

タブは Nearby Chat (FS V1 / V7 / LL の 3 style すべて) と IM (1:1) の両方に適用されます。Group IM は元から Object 発言が来ない経路なので Human のみで運用。

タブラベルは `System & Object` の順 (Object & System ではない)。

詳細 → spec `docs/ayastorm-r22-chat-tab-spec.md`

### フレンドオンライン通知だけは Human にも届く (新規 cvar)

「会話したい相手の登場」性質が強いフレンド online / offline 通知だけは **Human タブにも複製出力** されます (System & Object には常に出る)。Human タブだけ見ていても会話相手の登場を見落とさないための例外パスです。

- `FSFriendOnlineToHumanTab` (Boolean, default **ON**) で制御
- OFF にすると Human への複製が止まり、System & Object のみに出力されます
- なお SL の既存 cvar `OnlineOfflinetoNearbyChat` (デフォルト OFF、Nearby Chat に出すか自体) とは独立判定 — r22 のタブ分離が ON のときはこのフラグ経由で常に Nearby Chat に届きます

### 履歴ファイル形式: 1 本のまま、末尾 suffix marker で振り分け

`chat_*.txt` は r21 までと同じ 1 ファイル構造を維持しつつ、各行末に **suffix marker** を付与して読み込み時に Human / System & Object のどちらに振り分けるかを判定します。

- マーカーは末尾に付くため、上流 Firestorm で同じファイルを開いても **末尾文字列としてしか見えない** (パースは壊れない)
- AYAstorm 側ではマーカー文字列を **ユーザー画面に漏らさない** (history widget に append 前に剥がす)
- 旧履歴 (マーカーなし) は Human タブにフォールバック
- 過去のフレンドオンライン通知は起動時 reload で System & Object タブにだけ載ります (Human 複製は live ルーティング時の判定であり history reload では復元しない仕様)

### 起動時の表示色

Firestorm 既存挙動と一貫:

- 起動時に history load した過去ログは **両タブとも `ChatHistoryTextColorPersisted` (グレー)**
- session 中の受信は `ChatHistoryTextColor` (通常色) で append
- タブ切替は **widget の visible/invisible swap のみ** — 内容を一切触らないため、グレー化や色の入れ替わりは起きない

### 設定

`Preferences → Chat → Chat Windows` に配置:

| キー | 既定 | 用途 |
|---|---|---|
| `FSChatHumanObjectTabs` | `1` (ON) | タブ分離の master switch。`0` で旧 1-widget 挙動に完全に戻る (escape hatch 兼用) |
| `FSFriendOnlineToHumanTab` | `1` (ON) | フレンド online/offline 通知を Human タブにも複製出力。System & Object には cvar 値に関わらず常に出力 |

UI 上、`FSFriendOnlineToHumanTab` は master のインデント子要素として配置されており、master OFF 時は自動的にグレーアウトします。

> **escape hatch 用の別 cvar は意図的に提供しません。** master cvar の false 化で旧挙動に戻れるため、別キーを置く理由がありません (memory `feedback_prefer_defaults_over_config.md` — 多数の tuning キーより 1 つの妥当値)。

`FSChatHumanObjectTabs` および `AYAChatWindowStyle` (V1 / V7 / LL 切替) の変更時は **再起動誘導モーダル** が出て、旧スタイルの IM コンテナが自動で閉じる挙動を実装しています (M4-extra)。`FSFriendOnlineToHumanTab` は再起動不要で live 反映。

### LL style の通知系を 3 style 一貫化 (M8 同梱)

実装過程で見つかった上流 Firestorm の latent bug を同時修正:

- **TP 着地セパレーター** (`secondlife://... からのテレポートが完了しました` の前後の区切り) が **FS V1 / V7 / LL の 3 style 全てで** Nearby Chat (System & Object タブ) に表示されるようになりました
- **TP 完了通知 / region simulator version 差異通知 / RLV 系 system tip** など、`ChatSystemMessageTip` を経由するメッセージも LL style に届くようになりました
- 2021 年に上流で commented out されていた `LLFloaterIMNearbyChat` 経路を `findTypedInstance` でガードしつつ復活させた形で、`FSFloaterNearbyChat` と並列 dispatch されます

### 未読バッジは TP 着地でも正しく bump される

M5 で実装した未読バッジは履歴抑止フラグ (`do_not_log`) を流用していたため、TP セパレーターのような「履歴には残さないが session 内の新規イベント」で未読が増えない問題がありました。M8 で history reload 専用フラグ `is_replay` を導入し、本来の用途に分離しました。

### 既知の制約

- **未読バッジは セッション内のみ**: 再起動でリセットされます。履歴ファイル側に件数を残しても意味が薄い (履歴を開き直せば見える) と判断、永続化は採用していません。
- **HUD allow list (自分の HUD だけ Human に流す) は r22 スコープ外**: 「自分が貼った HUD の通知は人間扱いしたい」という需要は r23+ で検討予定。現状は HUD からの `llSay` 等もすべて System & Object タブに入ります。
- **過去のフレンドオンライン通知の Human 複製は起動時に復元されない**: live ルーティング時のフラグ判定で行うため、history reload 後は System & Object タブにだけ載ります。
- **`panel_nearby_chat.xml` には触っていません**: M3 着手時の構造調査で dead / 未参照と判明したため、`floater_fs_nearby_chat.xml` および `floater_im_session.xml` の `tab_container` のみで対応しています。

### ドキュメント

- r22 spec / GUI 案 / データ層 / 受入基準 / リスク登録: `docs/ayastorm-r22-chat-tab-spec.md`
- 履歴ファイルの suffix marker フォーマット: spec §5 (`docs/ayastorm-r22-chat-tab-spec.md#5-データ層`)
- 判定境界 (どの source type がどのタブに行くか): spec §4
- フレンド online/offline 例外パスの実装: spec §4「フレンド online/offline 例外」
- AYAChatWindowStyle (V1 / V7 / LL 切替) 周りの再起動誘導モーダル / floater 自動クローズ: spec §6 (M4-extra)
