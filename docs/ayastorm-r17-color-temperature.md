# AYAstorm r17: 時間帯色温度 — **DROPPED**

**作成日**: 2026-05-12 (初版起票)
**Drop 日**: 2026-05-12 (r18 実機検証中に効きが見えないと判明、即日 drop)
**Drop 経緯記録**: `doc/r17/color_temperature_survey.md` (P0 Survey、3 注入点設計の確定)

---

## 1. Drop 通知

r17 (時間帯色温度、太陽 elevation 駆動の Kelvin modulator) は **drop した**。r18 (雲の体積化) P1.a の AYA 実機検証中に AYAR17 ON/OFF 比較で「効きがわからない」と判明、診断ログで原因を確定したため。

- r17 P1.a 実装 commit `c3d6aee734` (push 済) は **本 drop commit で巻き戻し**
- r18 P1.a で予定していた **B 軸 (CLOUD_COLOR の r17 mod) も同時に drop** (r17 helper が消えるため自動 no-op、r18 は A 軸単独で出荷)
- spec 本体 (本ファイル) は「drop 記録」として残存、survey doc も「設計過程の痕跡」として残存

## 2. Drop 理由 (実機検証での観察)

`AYAR17ColorTemperatureEnabled = TRUE` で `getR17SunModulator` に診断ログ (`AYA_R17` tag) を追加して 5 preset (朝方/昼間/夕方/夜中/昼間レガシー) で実機計測した結果:

| Preset | UUID | sun lightnorm.z (elev) | t (smoothstep) | K | mod RGB |
|---|---|---|---|---|---|
| Day cycle (起動時) | `00000000-...` | 0.17 | 0.40 | 3928 | (1.00, 0.80, 0.65) — **強い warm** |
| Day cycle (時間進行) | `00000000-...` | 0.07 | 0.08 | 2539 | (1.00, 0.63, 0.29) — **深い amber** |
| 朝方 (Sunrise) | `01e41537-...` | **0.996** | 1.00 | 6500 | **(1, 1, 1) identity = no-op** |
| 昼間 (Midday) | `c46226b4-...` | 0.37 | 0.98 | 6428 | (1, 0.996, 0.993) ≒ identity |
| 夕方 (Sunset) | `084e26cd-...` | **0** | 0 | 2200 | (1, 0.58, 0.16) 数値上は強い warm |
| 夜中 (Midnight) | `8a01b97a-...` | **0** | 0 | 2200 | (1, 0.58, 0.16) **副作用** (ambient が warm に偏る) |
| 昼間レガシー | `KNOWN_SKY_LEGACY_MIDDAY` | — | — | — | identity (UUID pinpoint 除外) |

### 致命的問題
1. **朝方 (Sunrise) preset で太陽 elevation = 0.996 (zenith)** — SL の Sunrise preset は「朝の絵」だが太陽位置は真上に置かれており、helper は `t=1.0 / K=6500 / mod=(1,1,1) = identity` で no-op。AYA さんが「朝は変化を感じず」と評価した直接原因
2. **夕方 (Sunset) preset で elev=0、mod=(1, 0.58, 0.16) の強い warm シフトが数値上はかかっている** が、preset の SunlightColor が既に warm orange で R が tone mapping で飽和、G/B 減のみは知覚閾値以下 → AYA さんが「夕方は以前の設定でオレンジになってるのでやはり変化は感じません」と評価
3. **夜中 (Midnight) preset で elev=0、mod=(1, 0.58, 0.16) が ambient に適用** されると、月夜の青さが warm に偏る副作用 (sun が地下にいる時間帯にも sun elevation 駆動の Kelvin が ambient を縛る設計が不適切)

### 設計上の根本問題
r17 helper は **sun elevation を物理 Kelvin の入力と想定** していたが、SL の WindLight/EEP preset は **太陽位置 (lightnorm) と絵作りの色 (SunlightColor/AmbientColor/CloudColor) が独立に preset 制作者の経験で設定** されている。「Sunrise preset = 太陽が低い」「Sunset preset = 太陽が水平」という物理的整合は SL preset の慣行に存在しない。

→ r17 helper は **fixed preset では機能せず、Estate day cycle (preset=00000000、太陽が時間で動く) でのみ effective** という偏った効き方になっていた。これは `feedback_feature_value_in_main_usecase.md` の「動いた ≠ 効いた、主流ユースケースで効果が出ないなら捨てる」を厳格適用すると drop が筋。

## 3. 残された知見 (memory / 後章で再利用予定)

- **`kelvinToRGB` 公開式 (Tanner Helland 2012)** は別 helper として残せば D 軸 (カメラ/exposure) や音響 venue (?) で再利用可能。本 drop では削除したが、再導入が必要になったらこの spec の git 履歴を参照
- **`KNOWN_SKY_LEGACY_MIDDAY` asset UUID pinpoint 除外** のパターンは、`LLSettingsSky::canAutoAdjust()` の axis が「PBR 互換有無」を表しており「PBR 前再現意図」と一致しないため広い gate に使えないという発見と一緒に有用 (visual realism 章の修飾系コードで「Legacy preset を保護」する標準パターンになる)
- **SL preset の lightnorm と絵作り色の独立性** という観察は、将来「物理的に整合する preset 群」を出荷する選択肢 (例: Estate/Region 別 preset セット) を考えるときの前提
- **r17 helper の 3 注入点設計** (sky path SUNLIGHT_COLOR + AMBIENT、scene path mSunDiffuse + setAmbientLightColor、cloud path B 軸) は preset 制作者の絵作りに viewer 側から「物理整合」を上書きする設計の一例として記録

## 4. Drop による影響 (実コード)

- **削除**: `LLSettingsVOSky::getR17SunModulator` (llsettingsvo.{h,cpp})、`kelvinToRGB` 無名 namespace helper、`AYAR17ColorTemperatureEnabled` (settings.xml)、診断ログ `AYA_R17` (llsettingsvo.cpp 内)
- **巻き戻し**: `llsettingsvo.cpp::applySpecial` の sky path (SUNLIGHT_COLOR / AMBIENT の r17 mod)、`pipeline.cpp::setupHWLights` の scene path (mSunDiffuse / ambient の r17 mod)、`llsettingsvo.cpp::applySpecial` の r18 B 軸 (CLOUD_COLOR の r17 mod)
- **見た目への影響**: 既に fixed preset で「効いていない」と確認済なので、AYA 主流ユースケース (menu から朝/昼/夕/夜/レガシー選択) では **見た目変化はほぼゼロ**。Estate day cycle (preset=00000000) 利用時のみ「太陽 elevation 駆動の Kelvin shift」が失われる
- **r18 への影響**: P1.a 実装の B 軸 (CLOUD_COLOR mod) が drop 同伴で消える。**r18 は A 軸 (slab raymarch) 単独で出荷**、雲の体積感 (AYA 実機で「とても素晴らしい」評価) は単独で完結

## 5. 関連 commit 履歴

- `0655908da0` r17 P0 起票 (spec)
- `d68c67891f` r17 P0 Survey Round 1 完了
- `c3d6aee734` r17 P1.a 時間帯色温度実装 (本 drop で巻き戻し対象)
- (本 drop commit) r17 drop + r18 P1.a (A 軸単独) close-out

## 6. r19+ への teach-back

- 「主流ユースケースで効果が出ない機能は default ON で出荷しない」を A 軸第 4 弾でも実証 (cf. r14 P2.b/c sun dazzle drop)
- 「**SL preset の太陽位置と絵作りは独立**」という前提は今後も A 軸関連改修で再発しうるので、physical/elevation-driven 設計を入れる前に必ず 5 preset 実計測でカーブを確認する
