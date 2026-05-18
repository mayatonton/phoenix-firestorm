# AYAstorm r30 P5 — BD 同等到達ゲート (Cinematic 初回正式 ship) 事前 spec

**作成日**: 2026-05-19
**ステータス**: 起草中 (P4 ship 直後)
**スコープ**: r30 章 §3 P5 の事前 trace。P2/P3 cvar 群の Cinematic Controls floater 集約 + View Mode UI の正式モード昇格 + 第三者ブラインド A/B 判定の段取り。**実装は含まない** (P5 着手時の作業者向け案内)。
**上流参照**: BD `995a1354d8` (Version to 5.6.2, 2026-04-19) — P2/P3/P4 と同じ参照点。本 phase は BD 取り込みは無し (UI/UX 整備 + 判定 phase)。

---

## 1. 概要

### 1.1 P5 スコープと位置づけ

r30 章 §3 P5 は **章の最重要ゲート**: Cinematic mode が BD と並走できる絵に到達したかを第三者ブラインド A/B で判定し、通過した時点で初めて Cinematic を「正式モード」として release notes / README で告知する。

技術実装は薄く、判定 protocol と UI 仕上げが本体:

1. **P2/P3 cvar 群の Cinematic Controls floater 集約** (P4 で新設した floater に section 追加で 9 cvar 統合)
2. **View Mode UI の正式モード昇格** (combo `Cinematic (preview)` → `Cinematic`、tool_tip 改修)
3. **第三者ブラインド A/B 判定** (BD と AYAstorm Cinematic の写真を並べて SL コミュニティ複数名に判定依頼)
4. **判定結果に基づく ship/no-ship 決定** (no-ship なら P6 進まず P1〜P5 差分 release で再走)

### 1.2 章 spec との整合 (2026-05-19 章 spec realign 済)

章 spec §3 P5 は元々「Preferences AYAstorm セクション統合 / Cinematic 専用 floater は作らない」を想定していたが、P4 step 4a で AYA 判断により **Cinematic Controls floater + AYAstorm top menu** を新設済 (`c534684df5` で章 spec を realign)。P5 はこの floater に既存 cvar を集約する方向で進める。

### 1.3 章スコープ外 (P6+ へ)

- **AYA 独自色作り**: Cinematic 用 Kelvin/LUT/aerial 値の新規探索。P6+ で完成した Cinematic pipeline の絵を見てから着手 (章 §3 P6+ 参照)
- **BD UI 翻訳**: 章 §1.2 通り、BD の Machinima Sidebar / Photo Tools panel は取り込まない / 翻訳もしない
- **floater 多言語対応**: P5 では英語のみ維持。日本語/中国語 lproj 翻訳は P6+ で AYA 色作りと並行して検討

### 1.4 意図的に P5 では触らない不整合 — Phototools floater との DoF 系重複

**現状**: 既存 Firestorm Phototools floater (汎用撮影 UI、6 tab) にも DoF / 関連項目があり、P4 で新設した Cinematic Controls floater との間で同 cvar に 2 つの動線がある状態。ユーザーが「同じ DoF 設定が 2 箇所にある」と感じる懸念は実在する。

**P5 判断 (2026-05-19 AYA confirm)**: **意図して P5 では触らない**。理由:
- Phototools は Firestorm 由来の汎用撮影 UI で、Cinematic mode 外でも使用される。撮影 workflow の **互換性** を残すことが Firestorm fork として価値がある (Firestorm ユーザーが乗り換え時に既存操作を失わない)
- 重複解消には Phototools 側の cvar 再配置 + テスト範囲拡大が必要で、P5 のコスト感に対して比して大きい
- 初期段階は release notes / README で「Cinematic 系設定は Cinematic Controls floater (Alt+C) を推奨、Phototools の同名項目もそのまま動く」と明示するだけで案内する

**release notes 反映義務**: 本判断を P5 ship 時の ja/en/zh release notes 「移行ノート」節に必ず明記すること。ユーザーが「どっちを使えばいい？」と迷う前に先回りで案内する。

**P6+ 以降の再検討余地**: AYA 色作りが進んで Cinematic が「独自の絵」を出すフェーズに入った段階で、Phototools との UI 動線を再設計する余地あり (例: Phototools 側に「このモードでは Cinematic Controls を開く」リンクを追加する等)。P6+ spec で判断する。

---

## 2. 現在状態 (P4 ship 直後)

### 2.1 Cinematic Controls floater (P4 既存)

`indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` (width 320 / height 320 / single_instance):

| section | cvar | 種別 |
|---|---|---|
| Header | (説明テキストのみ) | — |
| Depth of Field | `RenderDepthOfFieldHighQuality` | checkbox |
| Depth of Field | `RenderDepthOfFieldFront` | checkbox |
| Chromatic Aberration | `RenderDepthOfFieldChroma` | checkbox |
| Chromatic Aberration | `RenderChromaStrength` | slider_bar + spinner + Reset |
| Footer | BD credit | text |

floater 登録: `llviewerfloaterreg.cpp` で `FloaterQuickPrefs` 派生 generic class を流用 (cvar binding は XML の `control_name` 自動)。

### 2.2 P5 で集約する未統合 cvar (P2 + P3 由来、計 9 件)

#### 2.2.1 P2 cvar (4 件)

| cvar | デフォルト | 用途 |
|---|---|---|
| `RenderMotionBlurStrength` | (要確認) | per-object motion blur 強度 |
| `RenderMotionBlurSelfAvatar` | (要確認) | 自アバターへの motion blur ON/OFF |
| `RenderMotionBlurOtherAvatars` | (要確認) | 他アバターへの motion blur ON/OFF |
| `RenderSMAAT2x` | OFF (要確認) | SMAA temporal resolve、Cinematic + SMAA 同時 ON 時のみ effective |

#### 2.2.2 P3 cvar (5 件)

| cvar | デフォルト | 用途 |
|---|---|---|
| `RenderVolumetricLighting` | 1 (ON) | godrays composite 全体の master switch |
| `RenderVolumetricLightingResolution` | 16 | shadow march sample 数 (GPU コスト線形) |
| `RenderVolumetricLightingMultiplier` | 50.0 | 光条強度 (BD 比 +50×、ACES 補正) |
| `RenderVolumetricLightingFalloffMultiplier` | 1.0 | 距離減衰の強さ |
| `RenderVolumetricLightingDirectional` | 1 (ON) | 太陽方向 fade gate (permutation、再起動必要) |

### 2.3 View Mode UI (P1 既存、P5 で改修)

`indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` line 320-343:

- line 320: `View Mode (restart required):` ラベル (変更不要)
- line 329: tool_tip 内 `Cinematic (preview): r30+ photo viewer mode; in r30 P1 this renders identically to AYAstorm View, and per-phase upgrades (velocity buffer, Volumetric Light, Motion Blur, BD DoF chain) will be enabled in subsequent releases.` (要 P5 改修: 「正式」昇格 + 機能リスト最新化)
- line 340: combo_box.item `label="Cinematic (preview)"` (要 P5 改修: `label="Cinematic"`)

---

## 3. 改修項目

### 3.1 Cinematic Controls floater への section 追加

既存 floater XML の Footer 直前に 2 section 追加:

#### 3.1.1 Volumetric Lighting section

5 cvar を以下の UI 配置で:
- `RenderVolumetricLighting` (checkbox / master)
- `RenderVolumetricLightingResolution` (slider 4〜64, デフォルト 16)
- `RenderVolumetricLightingMultiplier` (slider 0〜200, デフォルト 50)
- `RenderVolumetricLightingFalloffMultiplier` (slider 0〜10, デフォルト 1.0)
- `RenderVolumetricLightingDirectional` (checkbox + restart 注意ラベル)

#### 3.1.2 Motion Blur + SMAA T2x section

4 cvar を以下の UI 配置で:
- `RenderMotionBlurStrength` (slider, range 要確認)
- `RenderMotionBlurSelfAvatar` (checkbox)
- `RenderMotionBlurOtherAvatars` (checkbox)
- `RenderSMAAT2x` (checkbox + 「Requires RenderFSAAType=2 + Cinematic」注意ラベル)

#### 3.1.3 floater サイズ調整

現状 height=320 → section 2 つ追加で height 概算 +160 = **height=480** (要 実装時調整)。width=320 は据え置き。

### 3.2 View Mode UI の正式モード昇格

`panel_preferences_graphics1.xml` line 329 / 340 の 2 箇所改修:

- combo label: `Cinematic (preview)` → `Cinematic`
- tool_tip: 「in r30 P1 this renders identically to AYAstorm View ...will be enabled in subsequent releases」の preview 文言を削除、現在の Cinematic mode が提供する機能 (velocity buffer / SMAA T2x / Volumetric Light / Motion Blur / HQ DoF + Chromatic Aberration) を列挙

### 3.3 release notes / README の Cinematic 紹介

P5 ship 後に Cinematic mode を「正式モード」として:
- README で AYAstorm の 3 View Mode を並列に紹介 (Firestorm View / AYAstorm View / Cinematic の比較表)
- release notes ja/en/zh で「BD 同等到達ゲート通過、Cinematic を正式 ship」を告知

---

## 4. 第三者ブラインド A/B 判定 protocol

### 4.1 判定の意図

章 §3 P5 / §6.3 で定義された「BD 同等到達」は **「BD で撮った写真 vs AYAstorm Cinematic で撮った写真を並べて、第三者が BD 同等以上と判定する」**。AYA 自身による主観判定は P2〜P4 の各 phase ship 判定で使用済、P5 は AYA 外の眼で見る gate。

### 4.2 撮影 scene の選定 (5〜7 scene 想定)

各 phase の表現を引き出す scene 群:

| scene | 引き出す表現 |
|---|---|
| 屋外昼景 (太陽が見える構図) | Volumetric Lighting (godrays) |
| 樹木 / 建物のエッジ越し太陽 | Volumetric Lighting + 構図 |
| 浅景深ポートレート | HQ DoF + 前ボケ |
| 周縁部に高コントラスト輪郭 | Chromatic Aberration |
| 動きのあるアバター pan | Motion Blur + SMAA T2x |
| 屋内 / 暗所 | Cinematic の暗部表現 |
| 水面 / 反射ありの構図 | (P4 で water chroma は取り込まず、AYAstorm 既存挙動の確認) |

### 4.3 撮影 protocol

- 同一 SL location / 同一時刻 / 同一カメラ位置で BD と AYAstorm Cinematic を交互に撮影
- snapshot 解像度・format を統一 (PNG 推奨、tone 後処理しない)
- BD 側は default 設定 (Niran 推奨 preset 等は使わない、out-of-box) で撮影
- AYAstorm Cinematic 側も default 設定 + Cinematic Controls floater は触らない撮影と、floater で調整した撮影の 2 セット用意

### 4.4 評価者選定

- SL 内 photo 系コミュニティ (要 AYA からの紹介) で **3〜5 名** に依頼
- BD ユーザー / Firestorm ユーザー / Photo viewer 中立派が混在することが望ましい
- AYAstorm 既存ユーザー (preset 派) は判定 bias がかかるため極力避ける (章 §2.2「BD 系混じり」の通り、preset 派は別 segment)

### 4.5 判定 form

各 scene について以下を 3 段階で判定 (記名 OR 匿名どちらでも可):
- A (BD) と B (AYAstorm Cinematic) のどちらが「写真として美しい」と感じるか (A / B / 差なし)
- どちらが「写真として現実的」と感じるか (A / B / 差なし)
- どちらが「撮りたい絵」と感じるか (A / B / 差なし)

### 4.6 判定通過条件

- 全 scene × 全評価者の総票で **AYAstorm Cinematic = BD 以上** を獲得 (差なし票は同等扱い)
- 1 つでも明確に劣後する scene がある場合、その scene を引き出した phase に戻って差分 release を切る

### 4.7 判定不通過時の差分 release 案

各 phase 別に再 survey 観点を事前定義:

| 劣後 phase | 差分 release で見るべき点 |
|---|---|
| Volumetric (P3) | Multiplier / Falloff の値再評価、godray sampling 増、shadow march 範囲拡張 |
| Motion Blur (P2) | strength 再評価、self/other avatar gate の見直し |
| DoF / Chroma (P4) | chroma_str default 再評価、HQ DoF サンプル数増、前ボケ表現の補強 |
| SMAA T2x (P2) | reprojection 品質、ghosting / 描画安定性 |

---

## 5. 実装ステップ案

### 5.1 step 1: P2 cvar default / range / コメントの確認

`settings.xml` で P2 motion blur 4 cvar の default / type / コメントを Read、floater UI の widget 種別 / range を決定。

### 5.2 step 2: Cinematic Controls floater に Volumetric section 追加

`floater_aya_cinematic.xml` Footer 直前に 5 cvar の section block 追加 + height 調整 + 受入確認 (cinematic mode 起動 + 各 cvar を floater 経由で変更、画面反映確認)。

### 5.3 step 3: Cinematic Controls floater に Motion Blur + SMAA T2x section 追加

同 floater に 4 cvar の section block 追加 + height 再調整 + 受入確認。

### 5.4 step 4: View Mode UI の正式モード昇格

`panel_preferences_graphics1.xml` の combo label + tool_tip を改修 + 受入確認 (Preferences 画面で Cinematic の文言が「preview」抜きで表示されることを確認)。

### 5.5 step 5: 第三者ブラインド A/B 撮影 + 評価依頼

AYA 主導で:
- 撮影 (BD + AYAstorm Cinematic、5〜7 scene × 2 セット)
- 評価者 3〜5 名へ依頼 / form 集計
- 結果を本 spec §6 に記録

### 5.6 step 6: 判定結果に基づく ship / no-ship 決定

- ship: README + release notes 起草、Cinematic を正式モード昇格、P6 着手準備
- no-ship: 劣後 scene 別に差分 release 起こす (本 spec §4.7 の表を参照)

### 5.7 step 7: 出荷物の clean up + commit

- 検証用ログを除去 (`feedback_remove_verification_logs.md`)
- ja/en/zh 3 言語の release notes を起草
- 本 spec §6 に最終 commit log を記録

---

## 6. 受入観測 (P5 実行時に追記)

(P5 着手時に書き始める。step 1〜7 の commit hash、ブラインド A/B 判定結果、ship/no-ship 決定、を時系列で残す。)

---

## 7. 未確定事項 (P5 着手時に再 fetch / 再確認が必要)

### 7.1 P2 motion blur cvar の default 値 / range / 操作 UI

`RenderMotionBlurStrength` / `RenderMotionBlurSelfAvatar` / `RenderMotionBlurOtherAvatars` の default / range / type を `settings.xml` 確認 + P2 release notes 参照で確定。step 1 で実施。

### 7.2 floater 全 cvar 集約後の高さ / レイアウト

section 4 つ (DoF / Chroma / Volumetric / Motion Blur+SMAA) で width=320 / height=480 想定だが、実装時に各 widget の高さで再調整必要。場合により height=560 まで許容、それ以上なら sibling top menu に分割。

### 7.3 第三者ブラインド A/B 評価者の確保

AYA 主導で SL コミュニティ内から 3〜5 名確保。評価者が確保できない場合、Plan B として AYA + Claude 内部 A/B + 撮影サンプルを公開して非公式 feedback 募集の流れに切り替える可能性あり (要 AYA 判断)。

### 7.4 撮影 location / 時刻 / WL preset

各 scene の SL location / SL 時刻 / Windlight preset を実機検証時に確定。BD / AYAstorm Cinematic の両方で同条件 reproducible にする。

### 7.5 README 改修方針 (ship 時のみ)

P5 通過時に README に Cinematic を正式モードとして昇格紹介する箇所と文面を確定。Firestorm 上流の README 構造との整合も要確認。

### 7.6 ブラインド A/B 評価結果の公開方針

ship / no-ship 判定後、評価結果 (匿名集計) を公開するか内部記録のみとするか要 AYA 判断。公開する場合は本 spec §6 に集計表を残す。

---

## 8. 参考: 章 spec との対応

| 章 spec 章節 | P5 spec 対応箇所 |
|---|---|
| §3 P5 (実装 + ship 判定) | 本 spec §3 / §4 / §5 |
| §4.2 P5 row (cvar 整備) | 本 spec §3.1 (floater 集約) |
| §6.3 P5 ブラインドレビュー | 本 spec §4 (判定 protocol) |
| §A.4 P5 row (中規模工数) | 本 spec §5 (step 1〜7) |

---

(本 spec は P5 着手時に作業者が「BD 同等到達ゲートを通すために何をどの順で行うか」を spec 1 本で把握できる状態を目指している。実装着手時に §7 の各項目を再 fetch / 再確認した上で、§5 ステップ順に進める。)
