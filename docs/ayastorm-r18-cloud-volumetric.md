# AYAstorm r18: 雲の体積化 + 色温度連動

**作成日**: 2026-05-12 (初版、r17 close-out 直後)
**対象**: AYAstorm `feature/aya-r18-cloud-volumetric-spec-draft`
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 5 弾 (= A 軸完走)、r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) + r17 (色温度) の上に積む

> **本書の役割**: r18 個別の **計画スナップショット**。`feedback_release_with_user_feedback.md` 流儀で「完璧な spec を組まず、shader 触りながら追記」運用。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`、r16 spec は `docs/ayastorm-r16-aerial-perspective.md`、r17 spec は `docs/ayastorm-r17-color-temperature.md`。

---

## 1. ゴール

r14 で「**空気が体積として見える**」、r15 で「**光線が空間を貫く**」、r16 で「**遠景が空気の中に物理的に座る**」、r17 で「**時間帯の色が物理的に決まる**」を積んだ。r18 では **雲が体積として見え、時間帯の色温度に焼ける/冷える** を取りに行く。これで A 軸 (大気・空気の写真的リアリティ) が完走する。

具体的に出したい体感:

- **雲が「板」じゃなくなる** — 既存 flat texture cloud に厚みと奥行き、エッジが立体的に削れた質感
- **夕方の雲が橙〜赤に焼ける、朝方は冷色寄り** — r17 Kelvin modulator が cloud color にも連動、sky / scene / cloud で時間帯色温度が完全整合
- **写真撮るに値する空の核** — 体積化 + 色温度で「空を見て撮りたくなる」状態を作る

技術的には 2 軸:
- **A 軸 (体積化)**: cloud shader の analytic + 軽量 raymarch 化。heavy raymarch (毎フレーム全画面 ray-march) には倒れない節度を保つ
- **B 軸 (色温度連動)**: r17 で実装済の `LLSettingsVOSky::getR17SunModulator` を `CLOUD_COLOR` uniform push 直前にも適用、sky + scene + cloud の 3 経路同期

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換 (cloud_color / cloud_pos_density / cloud_scale / cloud_shadow uniform は input 契約)、HDR scene buffer 骨格、PBR shader interface、sky dome (skyV.glsl) の見え方
- **書き換える**: `cloudsV.glsl` / `cloudsF.glsl` (class1/deferred + class2/windlight) の cloud shader 内部、および `applySpecial` の CLOUD_COLOR uniform push 直前
- **言い換え**: cloud uniform は input、shader 内部で 3D noise の analytic + 軽量 raymarch を介して体積感を作る、出力契約 (HDR scene buffer 上の cloud layer 合成) は保つ

### Master switch + 個別 switch
- master `AYAVisualRealismEnabled` (r14 から) を共有
- 個別 switch `AYAR18CloudVolumetricEnabled` (default TRUE) を追加 — r16/r17 と同様に master と独立 toggle 可能
- 色温度連動 (B 軸) は r17 master `AYAR17ColorTemperatureEnabled` の生死を継承 (= 別 switch を追加しない)。r17 OFF なら r18 体積化のみ、r17 ON なら体積化 + 色温度連動の両方が効く
- `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) に対しては章ごと sentinel 1 本の運用方針継続

### sun disc / sky dome 保護 (r14 P2.b/c の副作用を踏まない)
- **sky shader (`skyV.glsl`) のコード自体は本リリースでも触らない** — r16/r17 と同じ方針
- cloud shader は sky shader と別 GLSL ファイルなので独立に書き換え可能 (`cloudsV.glsl` / `cloudsF.glsl` の class1/class2 系統)

### 体積化方針 (heavy raymarch 不採用)
`docs/ayastorm-visual-realism-roadmap.md` §6 に明示された通り「重い raymarch volumetric (毎フレーム全画面 ray-march) は AYAstorm の流儀 (1 viewer で完結、3 OS) に合わない」。本リリースは:
- **analytic 主体** — cloud texture を「平面 sample」ではなく「視線方向に短い厚みのスラブ sample」として複数 step 重ねる
- **軽量 raymarch** — 4〜8 step 程度の固定 step、early-out 付き、screen-space 全体ではなく per-cloud-pixel
- **既存 cloud texture を input として尊重** — preset の cloud_pos_density / cloud_scale が新方式でも引き続き意味を持つ設計

---

## 3. スコープ

### 含む
- **P0 Survey**: cloud shader (cloudsV/cloudsF) の class1/deferred + class2/windlight 2 系統の現状確認、CLOUD_COLOR uniform 注入点列挙、preset → shader 経路の Round 1
- **P1.a 体積化**: cloudsV/cloudsF に analytic + 軽量 raymarch を実装、既存 preset の cloud_pos_density / cloud_scale を活かす形で厚みを生成
- **P1.b 色温度連動**: r17 helper `LLSettingsVOSky::getR17SunModulator` を CLOUD_COLOR uniform push 直前にも適用 (`applySpecial` 内、cloud_color modulator)
- **C++ plumbing**: settings.xml に `AYAR18CloudVolumetricEnabled` Boolean 1 件追加、shader 側で個別 switch を読む uniform 配線 (llshadermgr.{h,cpp})
- **3 OS (Linux / macOS / Windows) ビルド + 体感確認** (P2)

### 含まない (→ r19+)
- **雲影 (地表に雲の縞模様)** — r19 候補に降格。SL の shadow path 改造または cloud noise projection lighting で軽量実装する余地はあるが、r18 の scope が肥大化するため分離
- **物質側 subsurface scattering** (B 軸、r19+)
- **カメラ表現** (DoF / auto-exposure / scene-referred 露出階調、C 軸 r21+)
- **preset 制作・新 preset 出荷** (AYA 方針: preset を作り直さない)
- **heavy raymarch** (毎フレーム全画面 ray-march、roadmap §6 で永久 drop)

### 永久 drop
- preset を破壊する後方非互換変更
- LUT / color grade による「雲の絵作り」誤魔化し (`docs/ayastorm-visual-realism-roadmap.md` §6 と整合)
- cloud shadow を r18 で同梱 (scope 膨張防ぐため明確に r19+ に分離)

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定

調査ターゲット (P0 完了条件):

1. **cloud shader の class1/class2 系統** — `app_settings/shaders/class1/deferred/cloudsV.glsl` + `cloudsF.glsl` と `app_settings/shaders/class2/windlight/cloudsV.glsl` + `cloudsF.glsl` の差分、どちらが現在の deferred / forward 経路で使われているか
2. **CLOUD_COLOR uniform の注入点** — `applySpecial` (`llsettingsvo.cpp`) と pipeline.cpp (もしあれば) で push されている箇所を列挙、r17 のように複数注入点でないか確認
3. **cloud uniform 一覧** — cloud_pos_density / cloud_scale / cloud_shadow / cloud_color / cloud_variance / cloud_pos_density1 等の現役 uniform、shader 内部での使われ方
4. **3D noise の入手手段** — cloud texture は 2D scroll、3D noise が必要なら別途 (procedural Worley / Perlin、texture3D 同梱、または procedural GLSL) — どれが軽量で 3 OS 互換か
5. **3 OS 共通性** — Metal/HLSL 特殊化が必要な GLSL 文法があるか、`.metal` / `HAS_METAL` ヒット確認
6. **個別 switch 配線** — settings.xml + llshadermgr の uniform 名追加、shader 側で `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` を読む

### P1.a: 体積化実装 (cloud shader 改修)

cloud shader 内部で:
- 既存 2D cloud texture sample を維持しつつ、視線方向に短い slab (例: 4〜8 step) で sample を重ねる
- 各 step で density を accumulate、early-out で empty space を skip
- raymarch 範囲は per-cloud-pixel に限定 (screen-space 全体には触らない)
- preset の cloud_pos_density / cloud_scale が体積方向にも反映されるよう係数を取る

### P1.b: 色温度連動 (r17 helper の再利用)

`llsettingsvo.cpp::applySpecial` の `CLOUD_COLOR` uniform push 直前で:
```cpp
LLColor3 cloud_color_modulated = psky->getCloudColor() * r17_sun_mod;
shader->uniform3fv(LLShaderMgr::CLOUD_COLOR, LLVector3(cloud_color_modulated.mV));
```

- r17 helper は `KNOWN_SKY_LEGACY_MIDDAY` で no-op、master/個別 switch で no-op なので呼び出し側に追加分岐不要
- ambient/sun と同じ modulator を使うため、sky / scene / cloud の 3 経路で時間帯色温度が完全整合
- cloud_color が legacy preset で warm 寄りなら r17 OFF/legacy 経路で生値が流れる

### P1.c: 実装後の体感調整 (条件付)

P1.a/b の Linux 実機検証で:
- 体積感が perceptual threshold 以下 → raymarch step 数 + slab 厚みを調整、または 3D noise scale 調整
- GPU コストが目立つ → step 数を削る、early-out 条件を緩める
- 体感が出ない → drop 検討 (`feedback_feature_value_in_main_usecase.md`)、ただし r17 helper 再利用部分 (P1.b) は軽量なので残す方針

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` の流儀。

### P3: tag / release

r14 / r15 / r16 / r17 と同じく、**公開は r18 単独でせず A 軸完走時 (= r18 close-out 時) に一括公開判断** の運用 (AYA 方針)。A 軸 5 リリース (r14-r18) を `v7.2.4-ayastorm-r18` 等の tag でまとめて公開する想定。

---

## 5. 受け入れ条件

- [ ] 既存 WindLight preset (朝・昼・夕・夜・昼間レガシー) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし)
- [ ] `AYAR18CloudVolumetricEnabled = TRUE` (master `AYAVisualRealismEnabled = TRUE` 前提) で:
  - 雲が flat な板ではなく **厚みと奥行き** を持って見える
  - エッジが立体的に削れ、cloud_pos_density / cloud_scale の変化が体積方向にも反映される
  - r14/r15/r16/r17 で出した体感が壊れない
- [ ] `AYAR17ColorTemperatureEnabled = TRUE` と組み合わせて:
  - 夕方の雲が **橙〜赤に焼ける**、朝方は冷色寄り、昼は preset 通りの白〜灰色
  - sky / scene / cloud の 3 経路で時間帯色温度が **完全整合** (雲だけ色がずれない)
- [ ] `AYAR18CloudVolumetricEnabled = FALSE` で r17 までと同じ見え方に戻る (shader 内部で switch off 経路、既存 flat sample のみ実行)
- [ ] **「昼間(レガシー)」で r17 経由の cloud_color modulator が無効化** (r17 helper の UUID pinpoint 除外を継承)
- [ ] **sky dome の見え方が r14 P2.a refined のまま** (sun disc 健在、青空質感劣化なし) — skyV.glsl 不触で構造的保証
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2)
- [ ] FPS 影響が ±15% 以内 (P2 で実測、体積化分は若干余裕枠)

---

## 6. リスク

| ID | リスク | 対策 / 現ステータス |
|---|---|---|
| R1 | raymarch step が多すぎて GPU コスト顕在化 (低スペック GPU で fps 大幅低下) | step 数 4〜8 で開始、early-out 厳格化、P1.c で実機計測しながら調整。screen-space 全体に raymarch しない方針が予防線 |
| R2 | 3D noise 入手 (texture3D 同梱 or procedural) が 3 OS で挙動差 | P0 Survey で procedural Worley/Perlin の GLSL 互換を確認、texture3D 同梱は配布サイズ + 3 OS asset path で重い側、procedural を default |
| R3 | 既存 preset の cloud 見た目が体積化で破壊される (cloud_pos_density / cloud_scale の意味が変わる) | preset uniform は input 契約として保つ設計、shader 内部で「新計算経路」を switch で gate、OFF で完全 backward compatible |
| R4 | r17 helper の cloud_color 適用で「昼間(レガシー)」の雲が変色 | r17 helper の UUID pinpoint 除外がそのまま継承される (`getR17SunModulator` が identity を返す)、追加対応不要 |
| R5 | cloud shader (class1/class2 2 系統) の片方だけ書き換えると deferred/forward 経路で見た目が割れる | P0 Survey で両系統列挙、両方同じ実装を入れる方針。SL の shader 配置構造 (class1 = main, class2 = enhanced) を踏襲 |
| R6 | 3 OS でビルドが通らない (macOS Metal cross-compile 等) | shader 改修のみ、C++ 側は r17 helper 再利用 + settings.xml 1 件、`.metal` / `HAS_METAL` 経路は P0 で確認 |
| R7 | 個別 switch (`AYAR18CloudVolumetricEnabled`) を追加することで `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) と衝突 | r14/r15/r16/r17 と同じく「章ごと体感評価用 sentinel」運用、A 軸完走時に統合 (master へ吸収) を検討 |
| R8 | heavy raymarch に倒れて scope が膨張 | spec §3 含まないに「heavy raymarch」を永久 drop として明記、step 数 上限を P1.c で固定 |
| R9 | 体積化の体感が perceptual threshold 以下 (r16 P1.b / r14 P2.b/c のような drop pattern) | `feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) 念頭、Linux 実機で評価、threshold 以下なら drop (色温度連動部分は残す方針で半保険) |

---

## 7. 更新履歴

- 2026-05-12 (初版): r17 close-out (`c3d6aee734`) 直後に r18 を起票。AYA との対話で「雲の体積化 (B) + 色温度連動 (A) のセット」スコープに確定 (cloud shadow C は r19+ に分離、scope 膨張回避)。理由は「体積化された雲面に色温度が乗ると cinematic が桁違い、別リリースに分けると単体評価が難しい」。`docs/ayastorm-visual-realism-roadmap.md` §4 r18 entry のスコープ (analytic + 軽量 raymarch、heavy raymarch 不採用) を継承、r17 で実装済 helper `LLSettingsVOSky::getR17SunModulator` を CLOUD_COLOR uniform にも適用する設計。次は P0 Survey (cloud shader class1/class2 経路調査)
