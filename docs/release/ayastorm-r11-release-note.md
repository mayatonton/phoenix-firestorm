# AYAstorm r11 — リリース告知文案

GitHub release ページに貼り付ける用の短文案。リリース判断確定後、AYA が公開時に貼り付ける。

実装記録 / 仕様詳細は `docs/ayastorm-r11-binaural-venue-reverb.md` / `docs/specs/spec_binaural_venue_reverb.md` を参照。

---

## AYAstorm r11 — バイノーラル + 会場残響 (lite-HRTF + venue convolution reverb)

### 新機能

3D Stream の per-channel placement (r10) に **listener 中心のヘッドホン体感** を一段引き上げる Layer 2 を追加。

- **lite-HRTF**: ITD (両耳時間差) + ILD shadow + air absorption (距離 HF 減衰) の DSP を per-channel に挿入。ヘッドホン視聴で「左の耳だけ大きい」感が「左から鳴っている」感へ。
- **venue convolution reverb**: 9 種の会場 IR (`dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`) を partitioned FFT 畳み込みで再現。素材は dry のまま、viewer 側で会場感を着替え。
- **wetgain は dry/wet 比**: 全 IR が unity-gain 正規化されているので、`{wetgain:1.0}` = 「wet が dry と同レベル」、`0.5` = half-mix、として venue 横断で同じ感覚で効きます。venue を切り替えても wetgain を retune する必要なし。
- **配信者主導タグ**: 親プリム Desc に `{binaural:on|off}` / `{venue:NAME}` / `{wetgain:N}` を追加。listener viewer は配信者の表現意図を忠実にレンダリング。listener 側 UI 改修ゼロ。
- **配信者向け LSL 拡張** (`docs/guides/lsl/aya_3dstream_setup.lsl`): r11 タグ (binaural / venue / wetgain) を menu / dialog から設定可能。Custom 値は textbox 入力。Default ボタンで viewer 側 default に戻せる (= debug setting が効く sentinel 状態)。
- **書いた URL がそのまま維持される (urlsave 廃止)**: LSL の自動退避/復元を撤去。再生失敗時に裏で前 URL に戻らないので、配信者は「自分の入力したURLが反映されている」「再生されない = URL もしくはサーバ側に問題がある」を即座に判別できます。
- **URL pre-resolve**: HTTPS→HTTP cross-protocol redirect を viewer 側で先解決、FMOD `createStream` 直渡しで落ちる問題を解消。

### 既存配置の扱い

r8 / r9 / r10 で既に置いた全プリムは **タグ無改修で binaural の恩恵を受けます** (`{binaural}` 未指定は on)。venue を当てたい場合のみ親プリムに `{venue:NAME}` を追加してください。

### 既知の制約

- **`hall_medium` / `hall_large` / `cathedral` は CPU 重め** (r10 比 +8〜10pp、cathedral で +10.2pp)。低スペック機では `room_small` / `room_medium` / `hall_small` を推奨します。配信者がタグで明示的に opt-in する選択として全 9 venue を出荷します。
- **listener 側 venue override 一般 UI は提供しません** (配信者主導モデル)。実装/検証用の debug settings (`Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain`) は残してありますが、各 default は「タグ通り」sentinel です (= 一般 listener はノータッチで配信者意図そのまま聴けます)。
- SOFA 個人 HRTF / Steam Audio integration / venue IR upload UI / dynamic venue は r12+ に持ち越し。

### IR ライセンス

bundled venue IR は OpenAIR 系 (CC-BY 4.0)。出典は `app_settings/venue_ir/CREDITS.md` を参照。

### ドキュメント

- 仕様: `docs/specs/spec_binaural_venue_reverb.md`
- 実装記録: `docs/ayastorm-r11-binaural-venue-reverb.md`
- 受入結果: 仕様 §5.5 + §13
