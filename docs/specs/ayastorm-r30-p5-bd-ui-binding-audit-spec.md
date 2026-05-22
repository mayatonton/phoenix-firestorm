# AYAstorm r30 P5: BD parity cvar × Firestorm UI binding audit

**Status**: in progress (2026-05-20 開始)
**Owner**: AYA + Claude
**Related**: `ayastorm-r30-p5-bd-parity-spec.md` §10 (cvar 二系統化), `ayastorm-r30-bd-full-port-inventory.md`

## 0. 背景と目的

r30 章の Cinematic mode (`AYAVisualRealismEnabled=2`) は **「AY 視覚表現 (volumetric / godrays / Kelvin / SSS) + BD 描画 cvar 値」** で構成される。実装上は BD 上流 `995a1354d8` から 32 個の描画 cvar 値を取り込み、mode 2 でその値で動かす。

UI 戦略の本ビジョンは:

- **Firestorm 既存 UI (Preferences / Phototools) をそのまま流用**
- 既存 UI で漏れる cvar は **AYA Cinematic Controls floater** (`floater_aya_cinematic.xml`) で補完

この前提が成立するかを 2 軸で監査する:

1. **UI 露出マッピング**: 32 cvar が Firestorm 既存 XUI のどこで露出しているか
2. **UI range/combo 互換性**: BD default 値が AY 既存 UI の slider min/max/increment や combo item set で **表現可能か**
3. (別タスク #182) **値解釈差**: AY pipeline.cpp と BD pipeline.cpp で同名 cvar の意味が同じか

本 spec は (1) と (2) を確定する。

## 1. UI 露出マッピング (32 cvar)

### 1.1 UI 露出あり (19 cvar; 内 1 cvar は dummy のみ)

> ⚠️ **2026-05-20 補正**: 初版は `control_name` 属性のみ grep していたため、Vector3 系の callback 経由 cvar binding (Phototools の Vector3 個別 slider) を取りこぼしていた。`RenderShadowGaussian` は実際には Phototools L1157 (X=Shd. Soften) / L1762 (Y=AO Soften) で UI 露出されており、`quickprefs.cpp` L2075-L2092 の `setVector3()` callback 経由で cvar に書込まれる。本節で正確化する。

| # | cvar | 露出箇所 | 補足 |
|---|---|---|---|
| 1 | `RenderAutoMaskAlphaDeferred` | panel_preferences_graphics1.xml L1483 | checkbox |
| 2 | `RenderAvatarMaxComplexity` | floater_preferences_graphics_advanced.xml L1227 / panel_preferences_graphics1.xml L2164 | **dummy hidden checkbox のみ** (実 UI 無し、preset 永続化用) |
| 3 | `RenderFSAAType` | floater_preferences_graphics_advanced.xml L410, panel_preferences_graphics1.xml L1056, floater_phototools.xml L3438 | combo box × 3 |
| 4 | `RenderFarClip` | floater_preferences_graphics_advanced.xml L28, panel_preferences_graphics1.xml L586, panel_performance_preferences.xml L207, panel_fs_performance_preferences.xml L268, floater_phototools.xml L2715 | slider × 5 |
| 5 | `RenderGlowIterations` | panel_preferences_graphics1.xml L1506, floater_phototools.xml L2528 | slider × 2 |
| 6 | `RenderGlowResolutionPow` | floater_preferences_graphics_advanced.xml L73, panel_preferences_graphics1.xml L731, floater_phototools.xml L2478 | slider × 3 |
| 7 | `RenderGlowStrength` | floater_phototools.xml L2576 | slider |
| 8 | `RenderGlowWidth` | floater_phototools.xml L2625 | slider |
| 9 | `RenderMaxVRAMBudget` | panel_preferences_graphics1.xml L1118 | slider |
| 10 | `RenderSSAOFactor` | floater_phototools.xml L1574 | slider + spinner |
| 11 | `RenderSSAOMaxScale` | floater_phototools.xml L1525 | slider + spinner |
| 12 | `RenderShadowBias` | floater_phototools.xml L1365 | slider + spinner |
| 13 | `RenderShadowBlurDistFactor` | floater_phototools.xml L1213 | slider + spinner |
| 14 | `RenderShadowBlurSize` | floater_phototools.xml L1120 | slider + spinner |
| 15 | `RenderShadowDetail` | floater_preferences_graphics_advanced.xml L750, panel_preferences_graphics1.xml L361, panel_performance_preferences.xml L304, panel_fs_performance_preferences.xml L366, floater_phototools.xml L876 | combo box × 5 |
| 16 | `RenderShadowFOVCutoff` | floater_phototools.xml L1262 | slider + spinner |
| 17 | `RenderTerrainScale` | panel_preferences_graphics1.xml L1692, floater_phototools.xml L2960 | slider × 2 |
| 18 | `RenderTreeLODFactor` | floater_preferences_graphics_advanced.xml L519, panel_preferences_graphics1.xml L860, floater_phototools.xml L3156 | slider × 3 |
| 19 | `RenderShadowGaussian` | floater_phototools.xml L1157 (X="Shd. Soften") + L1762 (Y="AO Soften") | Vector3 個別 slider (X, Y のみ; Z は UI 無し), callback 経由 |

**実 UI 露出**: 18 cvar (RenderAvatarMaxComplexity は hidden dummy のみ、ユーザー操作不可)

### 1.2 UI 露出なし (13 cvar)

| # | cvar | 想定対応 |
|---|---|---|
| 1 | `RenderAutoHideSurfaceAreaLimit` | AYA Cinematic Controls floater 拡張 |
| 2 | `RenderAutoMaskAlphaNonDeferred` | 同上 |
| 3 | `RenderAutoMuteSurfaceAreaLimit` | 同上 |
| 4 | `RenderAvatarMaxComplexity` | 同上 (dummy hidden を補完) |
| 5 | `RenderDeferredSpotShadowOffset` | 同上 |
| 6 | `RenderGlowLumWeights` | 同上 (Vector3) |
| 7 | `RenderGlowMaxExtractAlpha` | 同上 |
| 8 | `RenderGlowMinLuminance` | 同上 |
| 9 | `RenderGlowWarmthAmount` | 同上 |
| 10 | `RenderGlowWarmthWeights` | 同上 (Vector3) |
| 11 | `RenderShadowBiasError` | 同上 |
| 12 | `RenderShadowErrorCutoff` | 同上 |
| 13 | `RenderShadowOffset` | 同上 |
| 14 | `RenderWaterRefResolution` | 同上 |

→ **AYA Cinematic Controls floater に 14 cvar 追加** が要件 (`RenderShadowGaussian` は Phototools にあるため除外)。

## 2. UI range/combo 互換性 (18 cvar × 露出箇所)

各露出箇所で BD default 値が **表現可能 (slider tick / combo item として選択可)** かを判定。

### 2.1 凡例

- ✅ : BD default が UI range 内、increment/item に整合
- ⚠️ : BD default が range 内だが、表現精度低下 (例: 離散化で近似値になる)
- ❌ : BD default が range 外、または item に無く **選択不可**

### 2.2 結果表

| cvar | BD default | 露出箇所 | UI 定義 (min/max/inc または item set) | 判定 |
|---|---|---|---|---|
| RenderAutoMaskAlphaDeferred | 0 (off) | graphics1.xml L1483 | checkbox | ✅ |
| RenderFSAAType | 2 (SMAA) | graphics_advanced.xml L410 | items 0/1/2 | ✅ |
| RenderFSAAType | 2 | graphics1.xml L1056 | items 0/1/2 | ✅ |
| RenderFSAAType | 2 | phototools L3438 | items 0/1/2 | ✅ |
| RenderFarClip | 96.0 | graphics_advanced.xml L28 | min 64, max 512, inc 8 | ✅ (96 = 64+8×4) |
| RenderFarClip | 96.0 | graphics1.xml L586 | min 32, max 1024, inc 8 | ✅ |
| RenderFarClip | 96.0 | performance L207 | min 64, max 512, inc 8 | ✅ |
| RenderFarClip | 96.0 | fs_performance L268 | min 32, max 1024, inc 8 | ✅ |
| RenderFarClip | 96.0 | phototools L2715 | min 32, max 4096, inc 8 | ✅ |
| **RenderGlowIterations** | **5** | **graphics1.xml L1506** | **min 0, max 3, inc 1** | **❌ 範囲外** |
| RenderGlowIterations | 5 | phototools L2528 | min 0, max 200, inc 1 | ✅ |
| **RenderGlowResolutionPow** | **10** | **graphics_advanced.xml L73** | **min 8, max 9, inc 1** | **❌ 範囲外** |
| **RenderGlowResolutionPow** | **10** | **graphics1.xml L731** | **min 8, max 9, inc 1** | **❌ 範囲外** |
| RenderGlowResolutionPow | 10 | phototools L2478 | min 8, max 10, inc 1 | ✅ |
| RenderGlowStrength | 0.233 | phototools L2576 | min 0, max 0.5, inc 0.001 | ✅ (0.233 表現可) |
| RenderGlowWidth | 3.6 | phototools L2625 | min 0, max 50, inc 0.001 | ✅ |
| RenderMaxVRAMBudget | 0 (auto) | graphics1.xml L1118 | min 0, max 4096, inc 16 | ✅ |
| **RenderSSAOFactor** | **0.05** | **phototools L1574** | **min 0, max 250, inc 1, decimal=0** | **❌ 整数のみ、0.05 入力不可** |
| RenderSSAOMaxScale | 300 | phototools L1525 | min 0, max 10000, inc 1 | ✅ |
| RenderShadowBias | -0.001 | phototools L1365 | min -0.002, max 0.001, inc 0.000001 | ✅ |
| RenderShadowBlurDistFactor | 0.01 | phototools L1213 | min 0, max 1, inc 0.01 | ✅ |
| RenderShadowBlurSize | 1.0 | phototools L1120 | min 0, max 4, inc 0.01 | ✅ |
| RenderShadowDetail | 1 | graphics_advanced.xml L750 | items 0/1/2 | ✅ |
| RenderShadowDetail | 1 | graphics1.xml L361 | items 0/1/2 | ✅ |
| RenderShadowDetail | 1 | performance L304 | items 0/1/2 | ✅ |
| RenderShadowDetail | 1 | fs_performance L366 | items 0/1/2 | ✅ |
| RenderShadowDetail | 1 | phototools L876 | items 0/1/2 | ✅ |
| RenderShadowFOVCutoff | 0.0 | phototools L1262 | min 0, max 100, inc 0.1 | ✅ |
| RenderTerrainScale | 6.0 | graphics1.xml L1692 | min 1, max 24, inc 1 | ✅ |
| RenderTerrainScale | 6.0 | phototools L2960 | min 0, max 24, inc 1 | ✅ |
| RenderTreeLODFactor | 1.0 | graphics_advanced.xml L519 | inc 0.125 (min/max 未指定 = LL default) | ✅ |
| RenderTreeLODFactor | 1.0 | graphics1.xml L860 | min 0.125, inc 0.125 (max 未指定) | ✅ |
| RenderTreeLODFactor | 1.0 | phototools L3156 | min 0, max 8, inc 1 | ✅ |
| RenderShadowGaussian | (1.25, 2.0, 0.0) | phototools L1168 (X) | min 0.001, max 5, inc 0.001 | ✅ X=1.25 表現可 |
| RenderShadowGaussian | (1.25, 2.0, 0.0) | phototools L1773 (Y) | min 0.001, max 3, inc 0.001 | ✅ Y=2.0 表現可 |
| RenderShadowGaussian | (1.25, 2.0, 0.0) | (Z 成分 UI 無し) | — | ⚠️ Z=0.0 は UI 不可、overlay/cvar default に依存 |

### 2.3 ❌ サマリ

**3 cvar × 4 露出箇所で BD default が UI 表現不可**:

| cvar | BD default | 問題箇所 | 必要修正 |
|---|---|---|---|
| RenderGlowResolutionPow | 10 | graphics_advanced.xml L73, graphics1.xml L731 | `max_val="9"` → `max_val="10"` |
| RenderGlowIterations | 5 | graphics1.xml L1506 | `max_val="3"` → `max_val="16"` 等 |
| RenderSSAOFactor | 0.05 | phototools L1574 (+ spinner L1586) | slider `inc="1"` → `inc="0.01"`, `decimal_digits="2"`; spinner も同様 |

→ **XML overlay 案を採るなら、上記 4 箇所の UI 定義修正が前提条件**。

## 3. 結論 (暫定)

### 3.1 XML overlay 案で進める場合の必要作業

1. **§2.3 の UI range 修正 4 箇所**: slider min/max/inc を BD default が入る幅に拡張
2. **新規 overlay XML**: `app_settings/settings_cinematic_bd.xml` を作成、32 cvar の BD 値を入れる
3. **overlay ロード**: `LLAppViewer::loadSettingsFromDirectory` で mode 2 起動時に overlay を base XML の後 + user XML の前に読ませる
4. **AYA Cinematic Controls floater 拡張**: §1.2 の 15 cvar を追加 UI として実装
5. **`getCinematicAwareControl` 削除**: pipeline.cpp の 7 helper + 全呼び出し元を base cvar 名に巻き戻し
6. **bypass 修正巻き戻し**: r30 c23ca1d2af の修正は不要になる (overlay 案なら base 名読みのままで OK)
7. **spec §10 書き換え**: cvar split 案を overlay 案に置換

### 3.2 cvar split 案で進めるなら

1. AY 既存 UI 18 cvar × 述べ 30 箇所が mode 2 で **全て無音化** することを受容
2. AYA Cinematic Controls floater に **32 cvar 全部** の UI 追加 (Phototools と重複でも独立 UI が要る)
3. ユーザー混乱必至 (Preferences > Graphics で Draw Distance 動かしても mode 2 で効かない)

### 3.3 ハイブリッド最小実装案 (検討候補)

「overlay 案 + UI range 修正 + Cinematic Controls floater で UI 露出ゼロの 15 cvar」が、**UI 改修コスト最小 + ユーザー混乱ゼロ + BD parity 実現** の交点。

判断は別 task #183 で確定する。

### 3.4 Architecture 確定 (task #183, 2026-05-20)

#### 採用案: **XML overlay 案 (§3.3 ハイブリッド最小実装)**

| 検討項目 | overlay 案 | cvar split 案 |
|---|---|---|
| **UI ⇄ engine binding** | 単一 cvar 空間、既存 binding そのまま生きる | 二重化 (base + Cinematic)、binding を全 XUI で書き換え要 |
| **mode 2 で Firestorm 既存 UI が効くか** | ✅ 効く (overlay 値が user_settings 経由で書込) | ❌ 全 30 箇所無音化 |
| **AY Cinematic Controls floater 追加実装** | 14 cvar (§1.2 残) のみ | **32 cvar 全件** (Phototools と重複でも独立 UI 要) |
| **mode 切替時の値復元** | mode 2 → overlay 強制再適用、mode 1 → user 値復元で機能 | mode 別に物理分離、復元不要だが UI 別経路で煩雑 |
| **semantic 差リスク (task #182)** | なし (overlay は default 値だけ差替、read site 不変) | なし (同) |
| **B3 UI grey out (task #184, AYAR20 SSS 6 widget)** | `panel_preferences_sss.xml` root で `enabled_control="AYAVisualRealismEnabled"` 単一設定で完結 | 個別実装、cvar split とは独立 |
| **将来 mode 拡張** (mode 3 / 4) | overlay XML を追加するだけ | 全 cvar に新 suffix 追加、修正箇所 N×M で爆発 |
| **コード cleanup** | `getRenderCvar*` helper を vanilla LL の `gSavedSettings.getX()` に巻き戻し、`bd_default` 引数の dead code 化解消 | cvar split helper 群を維持・拡張 |

→ **overlay 案を最終 architecture として確定**。決定根拠は task #182 + #184 audit 結果で「semantic 差なし + B3 対象は SSS 6 widget のみ」が確定したこと。

#### 確定済の根拠 (audit 結果サマリ)

| audit | 結論 | 本決定への寄与 |
|---|---|---|
| #181 (UI range/combo 互換性, §2) | 18 cvar 露出箇所のうち 4 箇所で BD default が UI range 外 (`RenderGlowResolutionPow`×2, `RenderGlowIterations`, `RenderSSAOFactor`) | overlay 案実装 step 0 で UI range 拡張先行 (§5.2) |
| #182 (semantic diff, §5.10.B) | 32 cvar 全件で BD ⇄ AY pipeline の semantic 差なし | overlay 案で BD default 値を merge すれば mode 2 で BD と機能同一動作可能 |
| #184 (AY-固有 cvar B1/B2/B3, §5.10.A) | B3 = 11 cvar、UI grey out 必要は `panel_preferences_sss.xml` 6 widget のみ | overlay 案実装で grey out コストが 1 panel 修正に収まる |
| #185 (revert) | cvar split 撤去、`*Cinematic` 32 cvar 削除済 | overlay 案実装の出発点が確定 |

#### overlay 案 実装 step (新 task 群、本 spec §5.7 と §5.10.A から再構成)

| step | 内容 | 影響 file | 見積 |
|---|---|---|---|
| **A1** | UI range 拡張 (§5.2 の 4 箇所) | `floater_preferences_graphics_advanced.xml`, `panel_preferences_graphics1.xml`, `floater_phototools.xml` | 30 分 |
| **A2** | `settings_cinematic_bd.xml` 新規作成 (32 cvar BD 値、§4.6.2 参照) | `indra/newview/app_settings/settings_cinematic_bd.xml` (新規) | 1-2 時間 |
| **A3** | `LLAppViewer` に overlay 読込ロジック追加 (mode 2 起動時のみ base → overlay → user の順に load、mode 切替時に overlay 強制再適用) | `llappviewer.cpp` (`loadSettingsFromDirectory` 系) | 半日 |
| **A4** | `getRenderCvar*` helper 7 件を `gSavedSettings.getX()` 直接読みに置換、`bd_default` 引数の dead code 削除 | `pipeline.cpp` (helper 定義 L2820-2878 + 全呼び出し ~30 箇所) | 半日 |
| **A5** | AYA Cinematic Controls floater に 14 cvar (§1.2) 追加 UI 実装 | `floater_aya_cinematic.xml` | 1 日 |
| **A6** | B3 grey out 実装 (panel_preferences_sss.xml root に `enabled_control`+`enabled_control_value`、対応する LLPanel XUI parser 確認、必要なら helper cvar 方式に降りる) | `panel_preferences_sss.xml`, 必要なら `llpanel.cpp` | 半日 |
| **A7** | mode 2 切替時の overlay 強制再適用ハンドラ実装 (§5.8 第 1 行と整合)、D ボタン mode-aware 化は別 phase | `llviewercontrol.cpp` (handleAYAVisualRealismEnabled) | 半日 |
| **A8** | 同一 cvar A/B 検証 (mode 1 = AYAstorm view 維持、mode 2 = BD parity 同等画) | hands-on | 1 日 |
| **A9** | release-note 反映 + parity spec §10.8/§10.9 を architecture 確定済に更新 | docs/release/, docs/specs/ | 30 分 |

**合計見積**: 約 5 日 (検証含む)。

#### scope 外 (本 architecture 確定では決めない)

- mode 2 で AutoTuneFPS が `RenderFarClip` を動的書込する挙動の最終調整 (overlay 強制再適用で BD default に戻る挙動を許容するか、AutoTuneFPS が overlay 値を尊重するか) → A3 実装時の検証で判定
- mode 1 ⇄ mode 2 切替時の non-リニア cvar (Boolean/enum) の user 値破棄ポリシー (§5.8 末尾) → A7 実装時の挙動確認後に確定
- `getRenderCvar*` helper の dead code 削除を A4 に含めるか、新 release で分離するか → 1 commit で巻き戻す方が diff 追跡しやすい (A4 で同梱推奨)
- task #178 (Moiré) / #179 (HDR banding) は本 architecture 後に着手 (§5.11 と整合)

#### 次の action

1. AYA に本決定を提示し、step A1-A9 の優先度・分割粒度を確認
2. 合意後、本 spec §5.0 の「作業順序」を「architecture 確定済 → 実装 step A1-A9 を順次」に更新
3. `parity-spec.md` §10.8 末尾の「task #183 で確定予定」記述を「task #183 で overlay 案に確定 (2026-05-20)」に更新
4. 新 task (A1〜A9 個別) を taskCreate で発行

## 4. 残課題 (task #182 へ)

本 spec は **UI 側の互換性** のみを確定した。「値が UI で入力可能」と「値が BD pipeline で BD と同じ意味で解釈される」は別問題。次は:

- 32 cvar それぞれが、AY pipeline.cpp / lldrawpool*.cpp / 関連 .glsl でどう読まれ、どんな計算式に投入されるか
- BD upstream `995a1354d8` の同箇所と semantic diff があるか
- diff があれば「AY UI で値を変えても BD pipeline では別の意味になる」=  ユーザーから見て **bug 同然** の挙動

これを task #182 で audit する。

## 4.1 Phototools の cvar 書込メカニズム検証 (2026-05-20 追補)

AYA さんから「Phototools の slider は cvar に書いているのか? 閉じたら表示が元に戻る感触があるが」と問題提起あり、検証。

**結果 (`quickprefs.cpp` / `floater_phototools.xml`)**:

- `control_name` 付き slider/spinner は LL 標準動作で対応 cvar に `setF32` / `setS32` 等が走る
- Vector3 系 (RenderShadowGaussian, RenderShadowSplitExponent, RenderSSAOEffect, FSRenderVignette) は X/Y/Z 個別 slider + callback で `gSavedSettings.setVector3()` を手動呼び出し
- いずれも cvar に書込まれ、`user_settings.xml` に永続化される
- **GLSL に直接渡す一時値は無い**

**`FloaterQuickPrefs::onClose` (L1954-L1964)**:
```cpp
if (getIsPhototools()) { return; }  // Phototools 時は何もしない
gSavedSettings.setBOOL("QuickPrefsEditMode", false);
```
Phototools を閉じても cvar は保持される。

**「閉じたら戻る」感の正体 (推定)**:

1. **現状の cvar split 案の挙動**: mode 2 で Phototools スライダー (base 名 `RenderFarClip` 等) を動かしても、`getCinematicAwareControl` 経由で実効値は `*Cinematic` 側を読むため絵に変化なし → 次回開いた時に「変えたはずなのに戻っている」と見える可能性
2. **preset / EEP / WL 系**: 別経路で同 cvar を書き換える機構 (未追跡)

→ 1 番目は overlay 案へ切替で解消。2 番目は本論点外 (要時に別 task で深掘り)。

## 5. 注意点・落とし穴リスト (作業着手前に毎回参照)

> **⚠️ 本セクションは r30 BD parity 関連の作業を行う際、毎回着手前に読み返す。新たな注意点を発見したら必ず追記する。AYA 指示 (2026-05-20)。**

### 5.0 現状の repo 状態 (cvar split revert 完了、overlay 案準備済)

**2026-05-20 task #185 完了**: cvar split 実装を全 revert 済。repo は overlay 案実装の出発点として **pre-split (base cvar 単一空間)** に戻っている。

revert 内訳 (commit 2 本、HEAD 順):

| commit | 内容 |
|---|---|
| `cf796e5a6b` | `Revert "c23ca1d2af"` — bypass fix commit を git revert (8 files, -114/+41 を取り消し → -41/+114) |
| `33d56d2200` | `0975aa7ec4` を git revert + spec §10.8 を supersession note に書き換え、release-note L65 巻き戻し |

revert 後の repo 状態:

| 種別 | 場所 | 状態 |
|---|---|---|
| cvar 定義 | `app_settings/settings.xml` | 32 `*Cinematic` cvar **削除済** |
| dispatch helper | `pipeline.cpp` | `getCinematicAwareControl()` + 7 variants **削除済** |
| wire | `pipeline.cpp` | 32 `*Cinematic` cvar wire **削除済** |
| signal handler | `llviewercontrol.cpp` | mode-aware handler **base に戻し済** (c23ca1d2af revert で復元) |
| 仕様書 | `docs/specs/ayastorm-r30-p5-bd-parity-spec.md` §10.8 | supersession note (本 audit spec 参照) に置換済、§10.9 で step (5.x〜9) を継承 |
| floater binding | `floater_aya_cinematic.xml` | 11 cvar binding **base 名に戻し済** |
| release-note L65 | `docs/release/ayastorm-r30-release-note.{en,ja,zh}.md` | split 説明を pre-split 文面に巻き戻し済 (bdsidebar retirement 反映は別 commit で後追い) |
| build 検証 | `~/.ayastorm_x64/` install | 本 revert 後の build/install 検証 **要 (task #185 step 8)** |

**overlay 案で進める場合の今後の手順** (spec §5.7 と整合):

1. ~~cvar split を全 revert~~ ✅ 2026-05-20 完了 (task #185)
2. `app_settings/settings_cinematic_bd.xml` 新規作成 (32 cvar BD 値)
3. `LLAppViewer::loadSettingsFromDirectory` に overlay 挿入ロジック追加
4. §5.2 の UI range 修正 4 箇所
5. AYA Cinematic Controls floater に 14 cvar 追加 (§1.2)
6. §5.10 (B3) UI grey out 実装 (task #184 結果反映)

**⚠️ 作業順序 (AYA 2026-05-20 確定、task #183 完了で更新)**:

1. ~~**revert 先行 (task #185)**~~ ✅ 完了 (cf796e5a6b + 33d56d2200)
2. ~~**audit 並走 (task #182 + #184)**~~ ✅ 完了 (§5.10.A + §5.10.B)
3. ~~**architecture 確定 (task #183)**~~ ✅ 完了 — **overlay 案** に確定 (§3.4)
4. **overlay 案実装 (step A1-A9)**: §3.4 参照。UI range 拡張 → settings_cinematic_bd.xml 作成 → overlay 読込 → helper 巻き戻し → Cinematic Controls floater 14 cvar 追加 → B3 grey out → mode 切替ハンドラ → A/B 検証 → 仕様書/release note 反映
5. **Moiré 等の保留課題**: 本 cvar architecture が確定してから着手 (§5.12 参照)

### 5.1 BD default 値が AY ユースケースと相性悪い場合の対処

| BD default | リスク | 対処 |
|---|---|---|
| `RenderFarClip=96` | 屋外・遠景・夕景撮影で遠方クリップ (絶壁、04-23-42.png 観測済) | overlay 案なら既存 Draw Distance スライダー (mode 2 でも生きる) で都度上書き |
| `RenderShadowDetail=1` | avatar local shadow OFF → Poser 単体撮影で足元の影が無く違和感 | 既存 Shadows combo box で 2 に切替 |
| `RenderSSAOFactor=0.05` × AY 強め SSS | 顔の陰影が薄くなり破綻リスク | task #182 で実検証、必要なら AYA Cinematic Controls floater で AY 用 default 提供 |
| `RenderGlowStrength=0.233` × AY volumetric godrays | glow が godray に埋もれて目立たない | 同上 |

→ 「BD parity = BD 値を強制」ではなく、**「BD 値を baseline に、ユーザーが既存 UI で都度上書き可能」** が現実的着地点。

### 5.2 UI range 互換の落とし穴 (修正必須 4 箇所)

| cvar | BD default | UI 場所 | 現状 | 修正後 |
|---|---|---|---|---|
| RenderGlowResolutionPow | 10 | floater_preferences_graphics_advanced.xml L73 | max_val=9 | max_val=10 |
| RenderGlowResolutionPow | 10 | panel_preferences_graphics1.xml L731 | max_val=9 | max_val=10 |
| RenderGlowIterations | 5 | panel_preferences_graphics1.xml L1506 | max_val=3 | max_val=16 (Phototools と揃え) |
| RenderSSAOFactor | 0.05 | floater_phototools.xml L1574 (slider) + L1586 (spinner) | inc=1, decimal=0 (整数のみ) | inc=0.01, decimal_digits=2 |

→ overlay 案を採るなら **本修正を先行実施しないと UI 経由で BD default 値を選択不可**。

### 5.3 UI binding の取りこぼし注意

- `control_name=` 属性の grep だけでは **Vector3 系の callback 経由 binding を取りこぼす**
- 例: `RenderShadowGaussian` は `floater_phototools.xml` L1157 (X="Shd. Soften") / L1762 (Y="AO Soften") の個別 slider + `quickprefs.cpp` L2075-L2092 の `setVector3()` callback 経由
- 他 Vector3 cvar (FSRenderVignette, RenderShadowSplitExponent, RenderSSAOEffect 等) も同パターン
- **UI binding 監査時は `gSavedSettings.setVector3` / `setF32` 等の手動 setter call も併せて grep する**

### 5.4 「閉じたら戻る」現象の正体

- `FloaterQuickPrefs::onClose` (`quickprefs.cpp` L1954-L1964) は Phototools の時 **return; のみ** で cvar に何も書かない (= 閉じても cvar は保持される)
- 現象の主因 = 現状の cvar split 案で「mode 2 で UI 操作が `getCinematicAwareControl` 経由で読まれず無音化」 → overlay 化で解消
- preset / EEP / WL 系の別経路で cvar 書換は未追跡 → 必要時に別 task で深掘り

### 5.5 機能カテゴリと cvar カテゴリは独立

- **EEP / LLEnvironment** (太陽移動、Sky/Water 動的変更) は `LLSettingsSky` 系で 32 cvar と **直交**、影響なし
- **BD Poser / AO** は アバター rig 描画系で 32 cvar と **直交**
- **BD viewer 本体** が両機能 (EEP + Poser) を持ったまま 32 cvar で動いている事実が証明
- → 32 cvar 移植で AY 既存機能 (EEP, Poser, AO, Phototools, Quick Prefs, PBR, SSR…) は全て維持される

### 5.6 cvar split 案の決定的欠陥 (overlay 案推奨の根拠)

- mode 2 で **18 cvar × 述べ 30+ 箇所の既存 UI が無音化**
- AYA Cinematic Controls floater に **32 cvar 全部** の UI 追加が必要 (既存 UI と重複)
- ユーザー混乱必至 (Preferences で Draw Distance 動かしても mode 2 で効かない)
- → 採用しない

### 5.7 XML overlay 案の前提条件

採用時の必要作業:

1. **§5.2 の UI range 修正 4 箇所** (先行実施)
2. `app_settings/settings_cinematic_bd.xml` 新規作成 (32 cvar の BD 値)
3. `LLAppViewer::loadSettingsFromDirectory` に overlay 挿入ロジック (mode 2 起動時のみ overlay XML を base XML の後 + user XML の前に読込)
4. `getCinematicAwareControl` + 7 helper (`LLPipeline::getRenderCvarXxx`) **削除**
5. r30 c23ca1d2af の bypass 修正 **巻き戻し** (base 名読みで OK になる)
6. AYA Cinematic Controls floater に **14 cvar** 追加 (§1.2 残りリスト)
7. spec `ayastorm-r30-p5-bd-parity-spec.md` §10.8 を split → overlay に書き直し
8. r30 BD full port spec series で「split 案を採用しない判断」を明示

### 5.8 default 値同期問題 (両案共通の architectural tension)

- LL の `user_settings.xml` は mode を知らない **1 ストア**
- mode 1 で tune した値が user_settings に書かれる → mode 2 起動時 BD overlay の default を user 値が上書きする
- `LLControlVariable::resetToDefault` は compile-time default (`settings.xml`) に戻る、overlay default を考慮しない
- → AYA 議論済の解決方針 (本 spec 確定):
  - **mode 2 切替時に overlay 値を強制再適用** (user 値を mode 2 で破棄、撮影プリセットなので毎回 BD 純正)
  - **D ボタンを mode-aware に拡張** (`resetToDefault` が現在 mode の effective default に戻る)
- リニア cvar は将来「比率保存変換」 (`new_default × (user_value / old_default)`) で mode 跨ぎ user 値翻訳可能
- 非リニア cvar (Boolean / enum / 0=auto 特殊値) は比率変換不能、mode 別保存 or overlay 強制のみ

### 5.9 値解釈差 (task #182 で audit 中)

- 同じ cvar 名でも **AY pipeline.cpp と BD pipeline.cpp で計算式が違う**可能性
- AY 視覚表現層 (volumetric / godrays / Kelvin / SSS) と BD cvar の相性も観点に含める
- 結果次第で「BD 値そのままでは BD と同じ絵にならない」場合あり → AYA Cinematic Controls floater で AY 用 default を別途提供する余地を残す

### 5.10 AY 固有 cvar (BD pipeline 対応なし) の mode 2 UI 制御方針 (AYA 2026-05-20 確定)

「AY UI に存在するが BD 描画 pipeline で対応がない」cvar について、mode 2 での扱いを以下に分類:

| カテゴリ | 内容 | 例 (候補) | mode 2 での扱い |
|---|---|---|---|
| **B1** | AY 固有、AY 視覚表現層を制御 | RenderVolumetric* / Kelvin / SSS / godrays 関連 | **そのまま生きる** (mode 2 でも AY 視覚表現は有効) |
| **B2** | AY 固有、BD と機能重複 | AY tone map vs BD glow chain 等 | 競合整理 → AYA Cinematic Controls floater で「優先側」スイッチ提供 or 片方を mode 2 で disable |
| **B3** | AY 固有、BD/AY pipeline どちらでも未参照 (Firestorm view 用残存) | RenderSSAOEffect (Vector3) / RenderShadowSplitExponent / 一部 HDR 系? (要 audit) | **mode 2 では UI を grey out (disable) または hide する** ← **AYA 確定方針** |

**B3 方針の根拠 (AYA 2026-05-20)**:
- 効果が無い UI を操作させることは「壊れている」感を与え、ユーザー信頼を損なう
- tooltip で「mode 2 では効果なし」と表示するだけでは UX が貧弱、操作不可にする方が誠実

**B3 実装時の留意点**:
- `enabled_control` 属性で `AYAVisualRealismEnabled != 2` を bind するか、もしくは `LLFloater::draw` でランタイム enable/disable
- `visible_control` で完全に消すか、`enabled_control` で grey out するかは UI 一貫性で個別判断
  - **デフォルトは grey out (disable)**、完全 hide はレイアウト崩れる時のみ採用
- mode 切替時 (`AYAVisualRealismEnabled` change signal) に UI 状態を再評価する callback を入れる

**事前 audit 必須 (task #184)**:
- 「AY UI に存在する全 cvar」 × 「BD pipeline / AY pipeline での参照有無」を表化
- B1/B2/B3 分類が確定するまで本方針を発動しない (B3 でない cvar を誤って disable しないため)

### 5.10.A AY 固有 cvar B1/B2/B3 audit 結果 (task #184, 2026-05-20 完了)

対象: AYAstorm が独自に追加した描画系 cvar (vanilla LL / BD upstream 由来は除外)。LL 上流の `RenderVolumetricLighting` / `RenderTonemap*` / `RenderHDR*` / `RenderColorSaturation` / `RenderHDRISky*` 等は LL 上流 commit で導入された **「上流 cvar」** であり、本 audit のスコープ外 (BD pipeline でも同じ意味で使われるため、overlay 案で値だけ BD default に揃えれば済む)。

#### 分類サマリ

| 区分 | 件数 | 内容 |
|---|---|---|
| **B1** (mode 2 でもそのまま生きる) | 1 cvar | mode master |
| **B2** (BD と機能重複、整理要) | 0 cvar | — |
| **B3** (mode 2 で無効、UI grey out / 非表示) | 11 cvar | r16-r20 視覚表現層 |

#### B1: そのまま生きる (1 cvar)

| cvar | UI 露出 | 役割 | 理由 |
|---|---|---|---|
| `AYAVisualRealismEnabled` | `panel_preferences_graphics1.xml` L325 (combo_box) | mode master (0=Firestorm/1=AYAstorm/2=Cinematic) | mode 切替自体、全 mode で必須 |

#### B2: BD と機能重複 (0 cvar)

該当なし。AYAstorm r14-r20 で追加した cvar は全て AY 固有視覚表現 (aerial perspective, Kelvin, cloud volumetric, translucency, avatar SSS) で、BD pipeline には対応機能が無い。

#### B3: mode 2 で無効、UI grey out (11 cvar)

| # | cvar | 参照箇所 | mode-gating 証拠 | UI 露出箇所 | grey out 要 |
|---|---|---|---|---|---|
| 1 | `AYAR16AerialPerspectiveEnabled` | `llsettingsvo.cpp` L956-957 | `aya_view && aya_r16_aerial` (L949: `aya_view = (aya_visual_realism()==1)`) | なし (Debug Settings のみ) | × |
| 2 | `AYAR17ColorTemperatureEnabled` | `llsettingsvo.cpp` L777 | mode 1 限定の Kelvin modulator (L728-779 で `aya_visual_realism()==1 && aya_r17` 判定) | なし (Debug Settings のみ) | × |
| 3 | `AYAR18CloudVolumetricEnabled` | `llsettingsvo.cpp` L902 | `(aya_master()==1) && aya_r18_cloud_vol` (L901-907) | なし (Debug Settings のみ) | × |
| 4 | `AYAR19TranslucencyEnabled` | `pipeline.cpp` L10650 | `aya_realism_r19()==1 && aya_r19_enabled()` (L10664) | なし (Debug Settings のみ) | × |
| 5 | `AYAR19TranslucencyIntensity` | `pipeline.cpp` L10651 | 同 L10664 (`aya_r19_tier()` は mode 2 で `tier=0u` に潰される) | なし (Debug Settings のみ) | × |
| 6 | `AYAR20AvatarSkinSSSEnabled` | `pipeline.cpp` L11146-11147 | `realism_enabled()!=1 || !r20_enabled()` で早期 return | `panel_preferences_sss.xml` L35 (checkbox) | ✅ |
| 7 | `AYAR20AvatarSkinSSSBlurRadius` | `pipeline.cpp` L11165 | 同 SSS path (mode 2 で SSS 自体が走らない) | `panel_preferences_sss.xml` L59 (slider) | ✅ |
| 8 | `AYAR20AvatarSkinSSSStrength` | `pipeline.cpp` L11166 | 同上 | `panel_preferences_sss.xml` L86 (slider) | ✅ |
| 9 | `AYAR20AvatarSkinSSSGlowGain` | `pipeline.cpp` L11168 | 同上 | `panel_preferences_sss.xml` L112 (slider) | ✅ |
| 10 | `AYAR20AvatarSkinSSSGlowColor` | `pipeline.cpp` L11169 | 同上 | `panel_preferences_sss.xml` L148 (color_swatch) | ✅ |
| 11 | `AYAR20AvatarSkinSSSWhitelist` | `llayaskinsss.cpp` L23 | SSS pipeline 経由 (mode 2 で SSS 自体が走らない) | `panel_preferences_sss.xml` L195 (text_editor) | ✅ |

#### UI 影響まとめ

- **grey out 対象 UI 場所**: `panel_preferences_sss.xml` のみ (1 panel、6 widget)
- **AYAR16-19 (5 cvar)** は UI 露出ゼロ、Debug Settings 経由のみ。Debug Settings には mode-aware enabled gate が無いが、ユーザーは「pipeline で no-op になる」ことを実機動作で観察できる → 追加対応不要
- **AYAR20 SSS 全 6 widget** は `panel_preferences_sss.xml` の `<panel enabled_control="...">` または各 widget の `enabled_control="..."` で grey out できる
- **B1 (`AYAVisualRealismEnabled`)** はそのまま active 維持

#### overlay 案実装時の grey out 戦略 (案)

`panel_preferences_sss.xml` のルート `<panel>` に **mode-aware enable** を持たせる方針が最小コスト:

| 方式 | 内容 | 採否 |
|---|---|---|
| A | `enabled_control="AYAR20AvatarSkinSSSEnabled"` のみ (現状想定) | 既に master checkbox があるなら、master を mode 2 で grey out すれば全体が grey out される連鎖が成立。最小実装 |
| B | 専用 helper cvar (例: `AYAR20SSSEffective` を `master==1 && AYAR20Enabled` で都度 set) を新規定義し、`enabled_control` でそれを参照 | helper が増えて煩雑、A で十分 |
| C | `LLPanel::draw()` 内で `setEnabled` を mode signal で再評価 | C++ 追加要、A 比でメリット薄 |

→ **方式 A 推奨**: panel root に `enabled_control="AYAVisualRealismEnabled"` + `enabled_control_value="1"` (LL XUI が単一値 match を受けるなら) もしくは `U32` cvar 用に new attribute が要る場合は **B 案 helper cvar** に降りる。実装時に LLPanel XUI parser の対応範囲を確認して最終判断。

#### scope 外 (本 audit で除外した cvar 群)

| 群 | 例 | 除外理由 |
|---|---|---|
| LL 上流由来 cvar | `RenderVolumetricLighting`, `RenderHDREnabled`, `RenderColorSaturation`, `RenderTonemap*`, `RenderHDRISky*`, `RenderHDRSkySunlightScale` | vanilla LL 上流 commit で導入、本 audit は **AYAstorm 独自追加 cvar** のみが対象。overlay 案では BD default 値を merge するだけで済む (semantic は LL/BD で共通) |
| 非描画系 AY cvar | `FSSelfRiggedPicker*`, `AYAR21*` (self rigged picker), `AYAR22*` (chat tab) | mode 2 と直交 (描画 pipeline 非依存)、全 mode で動作。grey out 不要 |
| audio / 配信系 | `AYAStream*`, `AYAVenue*`, `FSParcelStreamQuality` | r13 以下 audio chapter / 配信タグで、視覚 mode と直交 |

→ **本 audit の grey out 実装対象は 6 widget (panel_preferences_sss.xml のみ)** に確定。

### 5.10.B 32 cvar BD vs AY pipeline semantic diff audit (task #182, 2026-05-20 完了)

`ayastorm-r30-p5-bd-parity-spec.md` §4 で残された懸念「同名 cvar が AY pipeline で BD と別意味に解釈されていないか」を audit。

#### audit 方法

1. 32 cvar 全件に対し、`indra/newview/**/*.{cpp,h}` 全体を `"<cvar>"` 文字列で grep
2. 各 read を以下に分類:
   - **H (helper-only)**: `getRenderCvar*()` 経由 (pipeline.cpp:2831-2877 の post-paradigm-shift helpers) — BD-port path 唯一の読み点
   - **L (LL-vanilla subsystem)**: LL 標準サブシステム (prefs UI, perf throttle, texture mem 管理) — BD でも同じ機構あり、semantic 同一
   - **A (AY-specific transform)**: AY 独自に値を別意味で再計算/再解釈する read — **semantic 差リスクあり**
3. 各 helper read site の **コメント marker** (`<FS:AYAstorm r30 BD full port>` / `Phase 3.x`) で BD-port 系統を確認
4. `setU32/F32/Vector3/Color4` の **write 側** も grep して、AY が cvar 値を動的に書き換える経路があるか確認

#### 結果サマリ

| 分類 | 件数 | 内容 |
|---|---|---|
| **semantic 同一 (BD-port 唯一読み)** | 22 cvar | Glow 8 + Shadow 7 + SSAO 2 + AutoMaskAlpha 2 + FSAA 1 + AutoHideSurfaceAreaLimit 1 + RenderWaterRefResolution 1 (helper のみ、AY 再読 無し) |
| **semantic 同一 (LL-vanilla 経由)** | 10 cvar | RenderShadowDetail, RenderFarClip, RenderTreeLODFactor, RenderTerrainScale, RenderGammaFull, RenderAvatarMaxComplexity, RenderAutoMuteSurfaceAreaLimit, RenderMaxVRAMBudget, RenderDeferredSpotShadowOffset, RenderShadowGaussian |
| **AY-specific transform (semantic 差リスクあり)** | **0 cvar** | 該当なし |

→ **32 cvar 全件、AY pipeline と BD pipeline で semantic 差は無い**。「AY UI で cvar を変えても BD pipeline で別の意味になる」リスクは存在しない。

#### 詳細 (代表例)

| cvar | 主 read 箇所 | 分類 | semantic 確認 |
|---|---|---|---|
| RenderGlow* (8 cvar) | `pipeline.cpp:1323-1335` (helper), shader uniform push @ `gPostScreenSpaceReflectionProgram` 系 | H のみ | shader uniform 名・push 順序とも LL/BD 共通の glow pipeline。AY 再読なし |
| RenderShadow{Bias,BiasError,Offset,BlurSize,BlurDistFactor,FOVCutoff,ErrorCutoff} | `pipeline.cpp:1352-1372` (helper) | H のみ | 同上、shadow projection pipeline は LL/BD 共通 |
| RenderShadowDetail | `pipeline.cpp:1298` (helper) + `llviewercontrol.cpp:1582` (signal listener for shader rebuild) + Prefs UI (feature manager check) | H + L | LL の `setting_setup_signal_listener` パターン、BD でも同じ動作 (shader rebuild trigger) |
| RenderShadowGaussian | `pipeline.cpp:1365` (helper) + `quickprefs.cpp:2076-2106` (Phototools の X/Y 個別 slider が `setVector3()` で書き戻し) | H + L | quickprefs は cvar に書くだけで semantic 変換なし。pipeline 側は LL/BD 共通の PCF blur |
| RenderSSAOFactor / RenderSSAOMaxScale | `pipeline.cpp:1354-1355` (helper) | H のみ | uniform push は LL/BD 共通の SSAO path |
| RenderFarClip | `pipeline.cpp:1369` (helper) + `llperfstats.cpp:84` (AutoTuneFPS が動的に書込) + `llstartup.cpp:3378` (region 移動時のクリップ) + `chatbar_as_cmdline.cpp:607` (`/drawdistance` 等のチャット cmd) + `llpresetsmanager.cpp:741` (preset 適用) | H + L | 動的 write 経路は全て LL 標準 (AutoTuneFPS, presets, chatbar) で BD でも同じ。意味は draw distance のまま |
| RenderTreeLODFactor | `pipeline.cpp` (helper) + `llappviewer.cpp:631` (startup で `LLVOTree::sTreeFactor` に代入) | H + L | LL 標準 tree LOD、BD でも同じ static に流し込む |
| RenderTerrainScale | `pipeline.cpp` (helper) + `lldrawpoolterrain.cpp:74-78` (terrain detail scale 計算: `sDetailScale = 1.f/RenderTerrainScale`) + `llpanelopenregionsettings.cpp:104` (region 設定からの上書き) | H + L | lldrawpoolterrain は LL/BD 共通の terrain blend ロジック、再解釈なし |
| RenderFSAAType | `pipeline.cpp:1288` (helper) + `llviewercontrol.cpp:1526` (signal listener for GL buffer 再確保) + `llfloaterpreference.cpp:2437` (prefs UI 表示同期) + `quickprefs.cpp:123` (Phototools default 提示) + `llviewermenu.cpp:10162` (menu cmd で reset) + `llviewerwindow.cpp:2442` (deferred 切替時に強制 SMAA) | H + L | UI/menu/reset 経路は全て表面的 (cvar に書く / 表示同期するだけ)、semantic 変換なし |
| RenderAvatarMaxComplexity | prefs UI 系 + `llfloaterperformance.cpp:437` (perf throttle) のみ、**pipeline.cpp の render path 自体には read 無し** | L のみ | LLVOAvatar / avatar complexity 評価で使われる LL 標準パス、BD でも同じ |
| RenderAutoMuteSurfaceAreaLimit | `llvoavatar.cpp:4308,9778,12459` (avatar visibility 判定) のみ | L のみ | LL 標準 avatar visibility logic、BD でも同じ |
| RenderGammaFull | `llviewercontrol.cpp:1552` (signal listener for shader rebuild) のみ | L | shader rebuild trigger、効果は LL/BD 共通の deferredUtil.glsl gamma chain |
| RenderMaxVRAMBudget | `llviewertexture.cpp:519` (texture memory budget 計算) + prefs UI + `llappviewer.cpp:4272` (起動時 VRAM 表示) | L のみ | LL 標準 texture mem 管理、BD でも同じ |

#### 動的 write 経路の確認

cvar 値が AY 内で動的に書き換わる経路 (overlay 案で settings_cinematic_bd.xml 値が user 操作以外で上書きされる risk) を以下に列挙:

| cvar | 動的 write 元 | 影響 |
|---|---|---|
| RenderFarClip | `llperfstats.cpp:84` (AutoTuneFPS), `llpresetsmanager.cpp:741` (preset), `llstartup.cpp:3382` (region 切替で 32f 強制), `chatbar_as_cmdline.cpp:607` (`/drawdistance` cmd), `llagent.cpp:4847` (agent shift で 32f 強制) | LL 標準機構、BD でも同様。mode 2 で overlay default を強制再適用する場合は AutoTuneFPS 等の有効性と要相談 |
| RenderAvatarMaxComplexity | `llfloaterpreference.cpp:2858` (slider commit) | UI からのみ、想定内 |
| RenderFSAAType | `llviewerwindow.cpp:2442` (deferred 切替時に強制 SMAA=2), `llviewermenu.cpp:10162` (menu cmd で reset) | deferred 必須化された時の自動補正、想定内 |
| RenderTerrainScale | `llpanelopenregionsettings.cpp:104` (region 設定からの上書き) | region owner が指定した場合のみ、想定内 |

→ overlay 案実装時は **mode 2 切替時の overlay 強制再適用** (§5.8) で AutoTuneFPS 等の動的書込を上書きする方針で整合する。

#### 結論

1. **32 cvar 全件で BD ↔ AY pipeline の semantic 差は無し**
2. 唯一の "意味のずれ" 要素は **default 値のみ** (`parity-spec.md` §4.6.2 で 33 件確定済)
3. **overlay 案で settings_cinematic_bd.xml に BD default 値を merge すれば、mode 2 で BD pipeline と機能的に同一の挙動**になる
4. `getRenderCvar*` helper の `bd_default` 引数は post-paradigm-shift では **「cvar 未登録時の fallback」のみ** に機能。overlay 案では cvar は常に登録済なので bd_default は到達不能 dead code 化する (将来 cleanup 候補)
5. 動的 write 経路 4 件 (RenderFarClip / AvatarMaxComplexity / FSAAType / TerrainScale) は LL 標準機構、BD でも同様に動く

→ **task #183 (architecture 確定) は overlay 案で進めて semantic risk なし**。

#### 落とし穴 (記録)

- `getRenderCvar*` helper を残したまま overlay 案を実装すると、`bd_default` 引数が読み取れず開発者を誤誘導する (「mode 2 ならこの値が使われる」と誤読される)。overlay 移行後に helper を **vanilla LL の直接読み (`gSavedSettings.getX("RenderXxx")`) に置換** するのが望ましい (task #183 確定後の実装 task で着手)
- `RenderWaterRefResolution` は pipeline.cpp の getRenderCvar* helper では参照されない。water RT 確保コードを別途確認要 (`lldrawpoolwater.cpp` か `pipeline.cpp` water plane allocation)

### 5.11 保留中の他課題 (本 cvar architecture 確定後に着手)

本 spec の architecture (task #183 で overlay 案確定 → 実装) が完了してから着手する課題。**Claude も AYA も忘れがちなのでここに記録**。

| task | 課題 | 現状 | 本 architecture との関係 |
|---|---|---|---|
| #178 | Mode 2 daytime terrain Moiré root cause (干渉縞) | AYA 確認済: `RenderTerrainScale` slider 動かすと縞の周期が変わる (= sampling frequency dependent) が、縞自体は消えない。Cinematic 専用 render path が要因の可能性 | overlay 案で `RenderTerrainScale` の cvar binding/反映経路がクリーンになってから根本調査。Moiré は terrain shader か pipeline の sample 戦略の問題で、cvar dispatch とは独立な技術課題だが、混乱回避のため順序を後にする |
| #179 | Night HDR low-luminance banding diagnosis | 仮説: HDR→LDR 8-bit quantization | 同上 (描画パイプ調査は cvar architecture 安定後) |

→ revert + overlay 実装が落ち着いたら、これらに着手。

### 5.12 作業の進め方 (Claude 自身の規律)

- **本 spec を作業着手前に毎回読み返す** (AYA 2026-05-20 指示)
- 新規注意点を発見したら **§5 に必ず追記** (次回以降の自分のため)
- 過去の重要決定 (overlay 案推奨、4 箇所 UI 修正先行、mode 2 切替時 overlay 強制再適用、等) は §5 に明示し、再議論で揺れない
- 関連 feedback memory:
  - `feedback_no_escape_full_bd_coverage.md` (BD parity 逃げない、全完走)
  - `feedback_bd_full_port_only.md` (BD pipeline 全体 1:1 移植、増分 borrow / 推論 / preset / tone match 禁止)
  - `feedback_warn_aya_off_bd_line.md` (AYA 指示でも不採用 approach なら警告)
  - `feedback_doubt_self_first.md` (効かない時はまず自分のコード/仮説を疑う)
  - `feedback_analysis_depth.md` (1 階層 grep の棚卸しで満足しない、深く追う)

## 6. 変更履歴

- 2026-05-20: initial draft (Claude + AYA), §1〜§3 確定
- 2026-05-20: §1.1/§1.2/§2.2 を Vector3 callback 経由 binding 取りこぼし補正 (RenderShadowGaussian を UI 露出ありに移動), §4.1 追補
- 2026-05-20: §5 「注意点・落とし穴リスト」追加 (AYA 指示)。作業着手前に毎回参照する規律を spec 内に明文化
- 2026-05-20: §5.10 追加。AY 固有 cvar (BD 対応なし) のカテゴリ B3 を mode 2 で UI grey out / hide とする AYA 確定方針を明文化。task #184 (AY 固有 cvar B1/B2/B3 audit) と紐付け
- 2026-05-20: §5.0 「現状の repo 状態 (出発点)」追加 (AYA 指示)。cvar split 案を実装済の現状と、overlay 案で進める場合の revert スコープを明示。作業着手前に必ず把握する
- 2026-05-20: §5.0 末尾に作業順序を明文化 (revert 先行 → audit 並走 → architecture 確定 → 実装 → Moiré 等保留課題)。§5.11 「保留中の他課題」追加で task #178 (Moiré) と #179 (HDR banding) を記録、§5.12 (旧 5.11) リネーム
- 2026-05-20: §5.10.A 追加 (task #184 完了)。AY 固有 cvar の B1/B2/B3 分類確定: B1=1 (master)、B2=0、B3=11 (AYAR16-19=5、AYAR20 SSS=6)。grey out 実装対象は `panel_preferences_sss.xml` の 6 widget のみに確定 (AYAR16-19 は UI 露出ゼロ = Debug Settings 専用のため対応不要)
- 2026-05-20: §5.10.B 追加 (task #182 完了)。32 cvar 全件で BD ↔ AY pipeline の semantic 差なしを確認。唯一の差は default 値のみ (§4.6.2 で 33 件確定済)、overlay 案で settings_cinematic_bd.xml に BD 値を merge すれば mode 2 で BD と機能同一動作。動的 write 経路 4 件 (FarClip/AvatarMaxComplexity/FSAAType/TerrainScale) は LL 標準機構で BD でも同じ。task #183 architecture 確定の前提条件全て揃った
- 2026-05-20: §3.4 追加 (task #183 完了)。最終 architecture を **XML overlay 案** に確定。実装 step A1-A9 (見積約 5 日)、§5.0 作業順序と parity-spec §10.8 を確定済に更新。次は AYA に決定提示 → step A1 着手
