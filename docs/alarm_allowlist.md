# alarm allow-list(fail-closed の唯一の逃がし穴・AYA 権限専用)

CLAUDE.md 冒頭「🔒 憲法」の default-deny gate が参照する唯一の allow-list。
**ここに明示されたシグナルだけが PASS を妨げない。それ以外の全 log alarm は自動でブロック(fail-closed)。**

## 運用ルール(不変)
- **追記できるのは AYA(人間)のみ。** agent はこのファイルに追記・編集してはならない(憲法 §4 = 凍結対象)。
- 1 エントリに **理由**必須。理由なき登録は無効。
- 登録は「バグを消した」ではなく「このシグナルは当面 PASS を妨げないと AYA が判断した」の記録。可能なら TICKET/期限を併記。
- alarm が根治したら該当行を削除(allow-list は最小を保つ)。

## 書式
```
| signal (grep パターン/一意キー) | 分類: ACCEPTED / KNOWN-BUG(#ref) | 理由 | 登録者 | 日付 |
```

## 登録済み
（空。= 現時点では全 log alarm が PASS をブロックする。AYA が明示登録するまで何も許可されない。）

| signal | 分類 | 理由 | 登録者 | 日付 |
|---|---|---|---|---|
