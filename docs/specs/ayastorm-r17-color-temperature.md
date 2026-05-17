# AYAstorm r17: 時間帯色温度 — **REVERTED — P1.a 復活**

**作成日**: 2026-05-12 (初版起票)
**Drop 日 (一旦)**: 2026-05-12 (r18 実機検証中に「効きがわからない」と判明、即日 drop)
**Revert 日**: 2026-05-12 (drop 直後の sustained viewing で「夕焼けの美しさが失われた」と判明、即日 revert)
**Survey 記録**: `docs/archive/r17/color_temperature_survey.md` (P0 Survey、3 注入点設計の確定)

---

## 1. Revert 通知

r17 (時間帯色温度、太陽 elevation 駆動の Kelvin modulator) は **一旦 drop したのち revert した**。

### drop した経緯

r18 P1.a の AYA 実機検証中に AYAR17 ON/OFF を切り替えた **instant A/B 比較で「効きがわからない」** と AYA 判定。診断ログで以下の偏りを確認したため `feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を厳格適用して drop した:

- Sunrise preset で太陽 elevation=0.996 (zenith) → K=6500=identity (no-op)
- Sunset preset で warm preset + R 飽和 → 知覚閾値以下
- Midnight preset で elev=0 → ambient warm 偏り副作用
- Estate day cycle (preset=00000000) でのみ effective

### revert した経緯

drop commit 後に AYA が実機で sustained viewing したところ **「Dropしたオレンジの夕焼けの世界は失われた」「夕焼けの美しさはなくなってしまった」** と評価。

つまり:

- **instant A/B 切替**では r17 ON/OFF の差が「効きがわからない」レベルだった (これは事実、tone mapping 飽和や ambient 副作用の偏りで perceptual threshold 以下)
- **drop 後の世界**を sustained で見ると「色温度が乗っていた時の orange 夕焼けが失われた」ことが明確に分かった (= cumulative には効いていた)

**「instant A/B で効いていない」と「sustained で効いていない」は別物**。`feedback_doubt_self_first.md` を自分の drop 判断にも適用すると、AYA の最初の「効きがわからない」発言は instant A/B 文脈であり、それを「機能として効いていない」と読み替えて全体 drop した私の判断が早計だった。

### 採用する方針

- **P1.a 実装 commit `c3d6aee734` の内容を復活** (sky path + scene path + cloud path B 軸の 3 注入点 modulator)
- **個別 switch `AYAR17ColorTemperatureEnabled`** を sentinel として残す (default TRUE)
- spec 本体 (本ファイル) を「REVERTED — P1.a 復活」記録に書き換え
- r18 B 軸 (CLOUD_COLOR の r17 mod) も同伴復活

## 2. 採取した実機データ (drop 判断時の診断ログ — 知見として保持)

`AYAR17ColorTemperatureEnabled = TRUE` で `getR17SunModulator` に診断ログ (`AYA_R17` tag) を追加して 5 preset (朝方/昼間/夕方/夜中/昼間レガシー) で実機計測:

| Preset | UUID | sun lightnorm.z (elev) | t (smoothstep) | K | mod RGB |
|---|---|---|---|---|---|
| Day cycle (起動時) | `00000000-...` | 0.17 | 0.40 | 3928 | (1.00, 0.80, 0.65) — **強い warm** |
| Day cycle (時間進行) | `00000000-...` | 0.07 | 0.08 | 2539 | (1.00, 0.63, 0.29) — **深い amber** |
| 朝方 (Sunrise) | `01e41537-...` | **0.996** | 1.00 | 6500 | **(1, 1, 1) identity = no-op** |
| 昼間 (Midday) | `c46226b4-...` | 0.37 | 0.98 | 6428 | (1, 0.996, 0.993) ≒ identity |
| 夕方 (Sunset) | `084e26cd-...` | **0** | 0 | 2200 | (1, 0.58, 0.16) **強い warm が乗る** |
| 夜中 (Midnight) | `8a01b97a-...` | **0** | 0 | 2200 | (1, 0.58, 0.16) (副作用: ambient が warm に偏る) |
| 昼間レガシー | `KNOWN_SKY_LEGACY_MIDDAY` | — | — | — | identity (UUID pinpoint 除外) |

### Sunset preset の (1, 0.58, 0.16) の意味

instant A/B では「すでに warm な preset がさらに warm 化する」差が R 飽和で読み取りにくいが、**drop して mod が外れると preset の SunlightColor だけになり、cinematic な orange 夕焼けが失われる**。AYA はこの差を sustained viewing で「悲しい」と表現した。

### Day cycle (preset=00000000) で深い効きが出る理由

Day cycle は時間進行で太陽が elevation を実際に動かすため、r17 helper が想定通りに smoothstep 0..0.4 で K=2200..6500 を出す。これは「physical/elevation-driven 設計」がもっとも効く場面であり、fixed preset (太陽位置が固定) では限定的な効きにとどまる。

→ revert 後の運用方針: **Day cycle と Sunset/Day-cycle 系 fixed preset で効くツール**として位置付け、Sunrise/Midnight 等の「helper が空回りする preset」については `feedback_release_with_user_feedback.md` 流で「効く preset / 効かない preset」が混在することを受け入れる。

## 3. 実装内容 (revert で復活したもの)

### `LLSettingsVOSky::getR17SunModulator(lightnorm, psky)`

`llsettingsvo.{h,cpp}` で復活。`AYAVisualRealismEnabled` (U32) + `AYAR17ColorTemperatureEnabled` (Boolean) の両 ON で modulator を返し、それ以外は `LLColor3::white` (identity) を返す。`KNOWN_SKY_LEGACY_MIDDAY` asset UUID pinpoint 除外で「昼間 (レガシー)」preset では identity を返す。

カーブ: `smoothstep(0, 0.4, lightnorm.z)` で `t` を出し、`K = mix(2200, 6500, t)` で Kelvin を出し、Tanner Helland 2012 の Kelvin→RGB 公開式で modulator RGB を生成。

### 3 注入点

1. **sky path** (`llsettingsvo.cpp::applySpecial`): SUNLIGHT_COLOR + CLOUD_COLOR (r18 B 軸復活) + ambient を `r17_sun_mod` で乗算
2. **scene path** (`pipeline.cpp::setupHWLights`): `mSunDiffuse` + ambient (`gGL.setAmbientLightColor` 前) を `r17_sun_mod` で乗算
3. **cloud path B 軸** (r18 と共通の sky path 内 CLOUD_COLOR 経路) — sky path に統合

### settings.xml

- `AYAR17ColorTemperatureEnabled` (Boolean, Persist=1, default=1) を r16 と r18 の間に復活

### master cvar の型変更 (r17 とは独立だが同時にこの commit で入る)

- `AYAVisualRealismEnabled` を **Boolean → U32** に変更 (default=1)
- 理由: `AYAViewMode` combo_box (Firestorm View=0 / AYAstorm View=1) の value="0"/"1" 文字列 ↔ Boolean cvar の LLSD 型 coercion が不安定で「差が殆どない」現象が出たため
- C++ 側は 3 サイト (`llsettingsvo.cpp::applySpecial` 2 箇所、`pipeline.cpp::doGodrays`) を `LLCachedControl<U32>` + `() != 0` 判定に変更

## 4. r19+ への teach-back

- **instant A/B と sustained viewing の効きは別軸で評価する**: instant 切替で「変化が知覚しにくい」機能でも sustained 視聴で cumulative に効いている場合がある。`feedback_feature_value_in_main_usecase.md` の「動いた ≠ 効いた」を判定するときは A/B 文脈と sustained 文脈の両方で観測してから drop 判断する
- **自分の判断にも `feedback_doubt_self_first.md` を適用する**: AYA の発言を「機能 NG」と一般化する前に、その発言が出た文脈 (instant A/B? sustained?) を確認する。drop は revert より低コストだが、「悲しい」評価が出てから巻き戻すよりも先に文脈確認した方が筋
- **SL preset の lightnorm と絵作り色の独立性**は事実で、physical/elevation-driven 設計が SL 慣行と衝突するのも事実。ただし「効く preset / 効かない preset」が混在することを受け入れる運用も成立する (Day cycle と Sunset で効く、Sunrise で効かない、を release notes で説明)
- **`KNOWN_SKY_LEGACY_MIDDAY` asset UUID pinpoint 除外**のパターンは r17 と r18 で確立、visual realism 章の修飾系コードで「Legacy preset を保護」する標準パターン

## 5. 関連 commit 履歴

- `0655908da0` r17 P0 起票 (spec)
- `d68c67891f` r17 P0 Survey Round 1 完了
- `c3d6aee734` r17 P1.a 時間帯色温度実装 (drop commit `380f5dc245` で巻き戻し → revert commit で復活)
- `380f5dc245` r17 drop + r18 P1.a (A 軸単独) close-out
- (本 revert commit) r17 revert + r18 B 軸復活 + View Mode combo_box 配線 + master cvar U32 化

## 6. 受け入れ条件 (revert 後)

- [x] `AYAVisualRealismEnabled = TRUE` (= AYAstorm View) かつ `AYAR17ColorTemperatureEnabled = TRUE` で:
  - Day cycle (preset=00000000、時間進行) で太陽が低くなるにつれ sun / ambient / cloud が warm 寄りに変化
  - Sunset preset で sustained に「cinematic な orange 夕焼け」が出る (AYA 実機 PASS、revert の動機)
  - 昼間 (レガシー) preset では identity (PBR 前再現 preset の意図を歪めない)
- [x] `AYAVisualRealismEnabled = FALSE` (= Firestorm View) で r17 mod が完全 no-op (sky/scene/cloud すべて preset 由来素通し)
- [x] `AYAR17ColorTemperatureEnabled = FALSE` で r17 mod が完全 no-op (master ON でも個別 OFF で識別)
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2 — Linux 済、macOS / Windows は A 軸完走時 tag/release で一括)
