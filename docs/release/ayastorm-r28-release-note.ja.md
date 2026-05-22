# AYAstorm r28 — リリース告知

**r28 は r21 で導入した self rigged picker を他人 avatar へ拡張するリリース** — 他人 avatar の rigged attachment を右クリックしたときも、r21 と同じ GPU object-ID buffer 経由で画面上に見えている attachment を選択できるようになります。同時 armed は 1 人だけ、という負荷制約付き。

実装詳細・負荷制御・検証ログは永続資料 (`docs/specs/ayastorm-r28-other-rigged-picker.md`) に常駐します。本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r28 — Other rigged picker

### r28 の柱: 他人の attachment を右クリックしても「見えているもの」が選ばれる

r21 で self picker は **画面に映っているのと同じ skinning matrix** を使う GPU object-ID buffer 経由で「見えているものを返す」性質を獲得しました。r28 はこの性質を他人 avatar にも持ち込みます — 他人 avatar の rigged attachment を右クリックすると、その avatar 1 人分の rigged draw info だけを `mObjectIDBuffer` に描き直し、マウス位置の pixel から LocalID を読み出します。

負荷制御は意図的に厳しく設計されています:

- 同時に armed されるのは **常に 1 人** (cursor 直下、または直近で右クリックされた avatar)
- GPU ID pass は短い armed window (`FSOtherRiggedPickerArmSeconds`、default `1.0` 秒) でのみ有効
- 標準三人称視点での hover は arm を **起動しない** — alt-cam / orbit / zoom など非デフォルト視点が要件
- mouselook / アバター customize モード中は picker 全体を skip
- 1 フレーム あたりの draw call / triangle budget (512 / 1,200,000、hardcoded) 超過時は buffer を無効化、既存 worldray pick に fallback

r21 の self picker 挙動は維持されます。2 つの picker は `mObjectIDBuffer` を共有しますが、buffer owner tag で「直前に描いたのは self / other どちらか」を区別し、cursor を自分と他人で行き来させたときの stale read を防ぎます。

詳細 → spec `docs/specs/ayastorm-r28-other-rigged-picker.md`

### 設定

| Key | Default | 役割 |
|---|---|---|
| `FSOtherRiggedPickerEnable` | `1` | 他人 picker のマスタースイッチ。`0` で他人 avatar 右クリックは upstream worldray 挙動に完全復帰 |
| `FSOtherRiggedPickerGPU` | `1` | GPU buffer pass の kill-switch。`0` = no-op、upstream worldray そのまま。GPU pass が誤動作する環境向けの逃げ道 |
| `FSOtherRiggedPickerArmSeconds` | `1.0` | 対象 avatar への最終 hover から ID pass が armed されている秒数 |

> **デフォルトカメラ gate** (`RequireNonDefaultCamera` 相当) と **draw call / triangle budget** (512 / 1,200,000) は内部 hardcoded で、cvar として外には出していません。再調整が必要になった場合は次の release で hardcoded 値を動かす方針 (cvar surface は広げない)。

### r28 に同梱される追加 fix

- **自分 avatar の顔選択修正**: 自分の顔を右クリックしたときに `mPick.mObjectID` が `gAgent.getID()` に正規化されず、self menu 判定が成立しない経路がありました。r28 では upstream の self-object hit を `gAgent.getID()` に正規化し、GPU self picker が attachment hit を返さなかった場合も avatar body 選択に戻すようにしました。自分の顔の右クリックが正しく解決します。

### 既知の制約

- **HUD attachment**: 対象外 (HUD カメラは screen-space で `mObjectIDBuffer` に存在しない) — r21 と同様
- **非 rigged attachment** (アクセサリ、ピアス等): 対象外 — upstream worldray pick へフォールスルー
- **他人 avatar の alpha-blend 髪**: r21 と同じ depth 制約 — alpha-discard 三角形は picker から消えるが、depth を書かない真の alpha-blend 髪は依然として貫通可能
- **混雑地**: 多数の avatar 間で hover を頻繁に切り替えると arm 切替が増える。1.0 秒の armed window + 1 人限定スコープでこのコストを意図的に抑制
- **重い outfit**: 極端に重い rigged outfit を持つ avatar では 512 draw call / 1.2 M triangle budget に当たることがあり、その frame は GPU pass を無効化、upstream worldray pick に fallback

### Credits

r28 の実装 (他人 rigged ID pass、buffer owner discriminator、デフォルトカメラ gate、budget 制御、自分の顔選択 fix) は [@t-noami](https://github.com/t-noami) によるものです。

### 関連資料

- r28 spec / アーキテクチャ / 負荷制御 / 検証ログ: `docs/specs/ayastorm-r28-other-rigged-picker.md`
- r21 self picker spec (姉妹機能): `docs/specs/ayastorm-r21-self-rigged-picker.md`
- attachment 描画 routing reference: `docs/specs/ayastorm-attachment-rendering-routing.md`
