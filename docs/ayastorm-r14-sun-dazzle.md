# AYAstorm r14: 太陽眩しさ表現 (sun disc overbright + bloom)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r14-sun-dazzle` (予定 / 着手前)
**位置づけ**: r13 で完結した音響表現章の後継、光表現章の初弾。`docs/ayastorm-light-expression-roadmap.md` の §3 r14 entry を canonical な詳細化

> **本書の役割**: r14 個別の **計画スナップショット**。実装着手後に commit hash / 実測値を埋めていく。
> 光表現章全体の位置づけは `docs/ayastorm-light-expression-roadmap.md` を参照。

---

## 1. ゴール

SL viewer は現状 **「太陽を直視しても眩しくない」**。AAA タイトルでは Layer A (光源輝度) × Layer C (bloom / 露出 / フレア) の組合せで「視線を太陽に向けた瞬間に白く滲んで眩しい」体感が成立しているのに対し、SL ではこれが体感ほぼゼロというギャップを埋める初弾。

r14 では **後段表現 (lens flare / auto-exposure)** には踏み込まず、最も投資対効果が高い **「sun disc 自体の輝度上げ + 既存 glow / bloom kernel パラメータ調整」** に絞る。

設計の核は **listener UI 改修ゼロ** (r11 / r13 と同流儀): 既存 glow / bloom 設定の延長として default 値の見直しと、太陽 billboard 側の emissive HDR 域への boost のみ。新規 debug settings は **必要最小限** (sentinel + 強度 1 つ程度)、`feedback_prefer_defaults_over_config.md` に従い tuning キー量産は避ける。

---

## 2. スコープ

### 含む
- **Layer A**: 太陽 billboard の輝度を HDR 域 (10x〜) に boost。日中天頂時に既存比 +10〜20 倍を目安に、glow buffer 経由で明確に滲む状態を狙う
- **Layer C**: 既存 glow / bloom kernel のサイズ / 強度 default を、sun disc が「白くハロー状に滲む」表現として成立する値に調整
- viewer 起動時の default ON、配信者 / 会場運営側のタグ要素なし (Layer A は viewer 任意のグローバル表現)
- 3 OS (Linux / macOS / Windows) でビルド + 体感確認

### 含まない (→ r15+ 候補)
- **lens flare** (sprite チェーン / ghost / halo): r15 候補、独立して別 release
- **auto-exposure (eye adaptation)**: r16 候補、工数 4〜6 日、独立 release
- **volumetric godrays / raymarch**: r17+、Layer B 本格化に伴うもの
- **emissive prim 全般の HDR 化**: 配信者 / 建物側コンテンツの見え方が広く変わるため別 release
- **月 / 夜景 / 星表現**: r17+ で Layer A 暗側として別途

### 永久 drop (現時点)
- 太陽輝度のユーザ UI 設定 (`feedback_prefer_defaults_over_config.md`、default のみで固める)
- 配信者タグ / 会場運営タグ (Layer A は viewer 任意のグローバル表現で、配信者主導 / 会場運営主導モデルとは独立)

---

## 3. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定

**目的**: 太陽 rendering と glow / bloom pipeline の実装位置を特定し、boost 注入点と kernel 調整点を確定する。

**調査対象**:
- 太陽 billboard / sky shader: `indra/newview/llvosky.{h,cpp}` / `app_settings/shaders/class*/deferred/skyV.glsl` 等
- glow / bloom pipeline: `indra/newview/pipeline.cpp` の glow pass、`app_settings/shaders/class*/effects/glow*.glsl`
- HDR / tonemap pipeline: 既存の PBR / ACES tonemapping の挿入順序 (boost が tonemap 後だと意味なし、tonemap 前 HDR 段で投入する必要)
- 既存 debug settings: `RenderGlow*` / `RenderHDREmissive*` / `Sun*` 系の名前空間と default 値

**完了条件**: 太陽輝度 boost を入れる関数 / shader uniform の同定、glow kernel 関連パラメータの同定、tonemap 前段で boost が効くことの確認。

**ファイル**:
- `doc/r14/sun_rendering_survey.md` (新規、P0 調査記録)
- 本書 §3.1 (P1) の touch points 欄を survey 結果で埋める

### P1: 太陽輝度 boost 実装

**目的**: 太陽 billboard / sky shader 出力の輝度を HDR 域に boost、tonemap 前の glow / bloom kernel に明るく拾わせる。

**設計**:
- 既存 sun disc 出力に **倍率 sentinel** (`RenderSunDiscBoost` debug setting、default 12.0 程度) を乗算
- boost は tonemap 前、glow buffer 入力前の HDR 値段階で適用
- sun disc 以外 (sky 全体や太陽以外の emissive) には影響させない
- 夜 / 日没時は太陽自体が暗いため自動的に弱まる (倍率は線形)

**ファイル**:
- `indra/newview/llvosky.{h,cpp}` あるいは sun shader (P0 で確定)
- `indra/newview/app_settings/settings.xml` (`RenderSunDiscBoost` default 12.0、`RenderSunDiscBoostEnabled` master sentinel default true)

### P2: glow / bloom kernel 調整

**目的**: 太陽輝度 boost に対して既存 glow kernel が「白くハロー状に滲む」絵になるよう default を調整。既存 RenderGlow* default 値の妥当性を確認、必要なら r14 同梱で default 微調整。

**設計**:
- 既存 `RenderGlow*` 設定は触らないことを優先 (一般コンテンツの emissive 表現に影響するため)
- 太陽専用の boost 倍率を上げる方向で表現を出す、kernel default 改変は最後の手段
- どうしても kernel 改変が必要な場合は、デフォルト変更ではなく r14 専用の補助パラメータを追加するか、`RenderSunDiscBoost` の倍率調整で吸収

**完了条件**: 通常の midday windlight で太陽を直視した時に、視野中心が白くハロー状に滲み、視線を逸らすと収まる絵が安定して出る。

### P3: 3 OS ビルド + 体感確認

**目的**: Linux / macOS / Windows それぞれでビルド成功 + 太陽眩しさが体感できることを確認。

**確認シナリオ**:
1. 既定 windlight (midday) で SL ワールド読み込み、太陽を直視 → 視野中心が滲んで眩しい
2. 視線を太陽から外す → 滲みが収まり通常の絵に戻る
3. 日没時 → 太陽自体が暗くなり、boost も自動で弱まる
4. 夜 → 太陽不可視、boost 影響ゼロ
5. emissive prim (発光オブジェクト) の見え方が r13 比で変わっていないこと
6. 撮影 / スナップショット用途で太陽周辺の表現が破綻していないこと

**完了条件**: AYA 実機で 1〜6 すべて PASS。

---

## 4. 受入条件

- 日中 windlight で太陽を直視すると、画面中央が白くハロー状に滲み「眩しい」と体感できる
- 視線を逸らすと数フレーム以内に通常表現に戻る (lens flare / 露出残光は r15+、ここでは即収束で可)
- 日没 / 夜は自動的に boost が弱まり / 消える
- 太陽以外の emissive (prim glow、街灯 projector など) が r13 比で明確には変わらない
- `RenderSunDiscBoostEnabled` = false で完全に r13 と同じ絵に戻る (sentinel として機能)
- 3 OS でビルド成功

---

## 5. リスク

| ID | リスク | 縮退策 | 担当フェーズ |
|---|---|---|---|
| L1 | tonemap 後で boost が clip され、見た目が変わらない | P0 で挿入位置を tonemap 前 HDR 段に決め打ち、shader 段階で boost | P0/P1 |
| L2 | sun disc 以外の sky / emissive まで boost が漏れる | sun-specific uniform / shader path 経由でのみ boost、sky 全体パスは触らない | P1 |
| L3 | glow kernel が太陽に対して飽和して「真っ白の塊」になり眩しさではなくバグに見える | P2 で kernel 改変を避け、boost 倍率側で表現を出す。default は控えめ (10〜12x) から始めて体感で詰める | P2 |
| L4 | コンテンツクリエイターから「自分の作品の見え方が変わった」と苦情 | sun 専用 path に閉じる + sentinel で完全 off 可、release note で明記 | 全体 |
| L5 | 撮影用途 (Photographer モード等) で太陽周辺の表現が破綻 | sentinel で off にして撮影できる経路を保証、release note で案内 | P3 |

---

## 6. r15+ への持ち越し

光表現章の後続として `docs/ayastorm-light-expression-roadmap.md` §3 で扱う。r14 で土台 (sun disc overbright + 既存 glow 経路の流用) が動く前提で、r15 (lens flare) / r16 (auto-exposure) はその上に乗る形になる。

- **r15 lens flare**: sun screen position + visibility (occlusion query) から sprite chain を加算。r14 sun boost が flare 強度ソースとして使える
- **r16 auto-exposure (eye adaptation)**: HDR 輝度 reduction + 時間平滑 + tonemap exposure 接続。r14 sun boost が暗所→屋外移行時の白飛びを誇張する効果を持つ
- **r17+ volumetric godrays**: Layer B 本格化、太陽方向への体積散乱

r14 単独でも体感の 6 割を取りに行くスコープなので、r15/r16 への移行は「r14 リリース後ユーザフィードバック収集 → 次の弾を決める」(`feedback_release_with_user_feedback.md`) 流儀で。

---

## 7. 更新履歴

- 2026-05-12: 初版作成。`docs/ayastorm-light-expression-roadmap.md` §3 r14 entry の詳細化。スコープを Layer A (sun disc HDR boost) + Layer C (既存 glow / bloom 流用) に限定、lens flare / auto-exposure / volumetric は r15+ へ持ち越し。debug settings は `RenderSunDiscBoost` (倍率、default 12.0) + `RenderSunDiscBoostEnabled` (sentinel、default true) の 2 件のみ提示。3 OS 込みで 0.5〜1 日工数の見積を継承
