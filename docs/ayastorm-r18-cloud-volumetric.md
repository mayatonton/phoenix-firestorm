# AYAstorm r18: 雲の体積化 + 色温度連動

**作成日**: 2026-05-12 (初版、r17 close-out 直後)
**対象**: AYAstorm `feature/aya-r18-cloud-volumetric-spec-draft`
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 5 弾 (= A 軸完走)、r14 (volumetric atmosphere) + r15 (godrays) + r16 (aerial perspective) + r17 (時間帯色温度) の上に積む
**スコープ変遷**: 当初「雲の体積化 (A 軸) + 色温度連動 (B 軸)」のセットで起票 → r17 drop と同伴で「A 軸単独」に re-scope → drop 後の sustained viewing で「夕焼けの世界が失われた」と AYA 判定で **r17 revert + B 軸 復活** → 最終的に当初通り「A 軸 + B 軸セット」で出荷 (詳細は `docs/ayastorm-r17-color-temperature.md` の revert 記録)

> **本書の役割**: r18 個別の **計画スナップショット**。`feedback_release_with_user_feedback.md` 流儀で「完璧な spec を組まず、shader 触りながら追記」運用。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 spec は `docs/ayastorm-r14-volumetric-atmosphere.md`、r15 spec は `docs/ayastorm-r15-godrays.md`、r16 spec は `docs/ayastorm-r16-aerial-perspective.md`、r17 revert 記録は `docs/ayastorm-r17-color-temperature.md`。

---

## 1. ゴール

r14 で「**空気が体積として見える**」、r15 で「**光線が空間を貫く**」、r16 で「**遠景が空気の中に物理的に座る**」を積んだ。r18 では **雲が体積として見える** を取りに行く。これで A 軸 (大気・空気の写真的リアリティ) が完走する。

具体的に出したい体感:

- **雲が「板」じゃなくなる** — 既存 flat texture cloud に厚みと奥行き、エッジが立体的に削れた質感
- **写真撮るに値する空の核** — 体積化で「空を見て撮りたくなる」状態を作る

技術スコープ:
- **A 軸 (体積化、本リリース)**: cloud shader の analytic + 軽量 raymarch 化。heavy raymarch (毎フレーム全画面 ray-march) には倒れない節度を保つ。AYA 実機で「とても素晴らしい」評価
- **B 軸 (色温度連動、本リリース)**: r17 helper `getR17SunModulator` を CLOUD_COLOR にも適用、体積化した雲面に sun の色温度が乗る cinematic 効果。一旦 r17 drop で同伴 drop したが、drop 後の sustained viewing で「夕焼けの世界が失われた」と判定され revert

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換 (cloud_color / cloud_pos_density / cloud_scale / cloud_shadow uniform は input 契約)、HDR scene buffer 骨格、PBR shader interface、sky dome (skyV.glsl) の見え方
- **書き換える**: `cloudsF.glsl` (class1/deferred のみ — P0 Survey で class2 系統に同名 shader が存在しないことを確認)、`applySpecial` で A 軸用の shader uniform 1 件を push
- **言い換え**: cloud uniform は input、shader 内部で既存 2D `cloud_noise_texture` を **視線方向の slab raymarch で多 sample** することで体積感を作る (3D noise asset の新規同梱は不要)、出力契約 (HDR scene buffer 上の cloud layer 合成) は保つ

### Master switch + 個別 switch
- master `AYAVisualRealismEnabled` (r14 から、r18 で **U32 化** — value=0/1 で `AYAViewMode` combo_box と直結) を共有
- 個別 switch `AYAR18CloudVolumetricEnabled` (default TRUE) を追加 — r16 / r17 と同様に master と独立 toggle 可能
- `AYAR17ColorTemperatureEnabled` (default TRUE) を r17 revert で復活、B 軸 CLOUD_COLOR mod を gate
- `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) に対しては章ごと sentinel + view mode UI 1 本の運用方針継続

### sun disc / sky dome 保護 (r14 P2.b/c の副作用を踏まない)
- **sky shader (`skyV.glsl`) のコード自体は本リリースでも触らない** — r16/r17 と同じ方針
- cloud shader は sky shader と別 GLSL ファイルなので独立に書き換え可能 (`cloudsV.glsl` / `cloudsF.glsl` の class1/class2 系統)

### 体積化方針 (heavy raymarch 不採用、2D noise の slab 化)
`docs/ayastorm-visual-realism-roadmap.md` §6 に明示された通り「重い raymarch volumetric (毎フレーム全画面 ray-march) は AYAstorm の流儀 (1 viewer で完結、3 OS) に合わない」。本リリースは:
- **analytic 主体** — cloud texture を「平面 sample」ではなく「視線方向に短い厚みのスラブ sample」として複数 step 重ねる
- **軽量 raymarch** — 4 step 固定 (P1.a 採用)、screen-space 全体ではなく per-cloud-pixel、Beer-Lambert 風 transmittance 累積
- **既存 cloud texture を input として尊重** — `cloud_noise_texture` (2D) はそのまま、3D noise asset の新規追加なし、preset の cloud_pos_density / cloud_scale が引き続き意味を持つ設計

---

## 3. スコープ

### 含む
- **P0 Survey** (完了 `636e163a3c`): cloud shader は class1/deferred のみ、CLOUD_COLOR uniform 注入点は `llsettingsvo.cpp:890` 1 箇所のみ、cloud は完全 2D、`AYA_R18_CLOUD_VOLUMETRIC_ENABLED` shader enum を新設要、HAS_METAL 0 hits、drawpool bind 経路不触
- **P1.a 体積化 (A 軸)**: cloudsF.glsl で既存 2D `cloud_noise_texture` を視線方向 slab 4 step raymarch (Beer-Lambert 風 transmittance 累積)、`AYAR18CloudVolumetricEnabled` + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外で gate
- **C++ plumbing**: settings.xml に `AYAR18CloudVolumetricEnabled` Boolean 1 件追加、shader uniform `aya_r18_cloud_volumetric_enabled` を llshadermgr.{h,cpp} に enum 追加
- **3 OS (Linux / macOS / Windows) ビルド + 体感確認** (P2)

### 含まない (drop)
- **雲影 (地表に雲の縞模様)** — r19 候補に降格。SL の shadow path 改造または cloud noise projection lighting で軽量実装する余地はあるが、r18 の scope が肥大化するため分離
- **物質側 subsurface scattering** (B 軸の正式版、r19+)
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

### P0: 実装箇所調査 + spec 確定 (完了 `636e163a3c`)

調査結果 (`doc/r18/cloud_volumetric_survey.md` 参照):

1. **cloud shader の class1/class2 系統** — `app_settings/shaders/class1/deferred/cloudsV.glsl` + `cloudsF.glsl` のみ存在 (`class2/windlight` 配下に同名 shader は **無い**)。spec §2 の class1+class2 想定は class1 単独に確定
2. **CLOUD_COLOR uniform の注入点** — `llsettingsvo.cpp:890` の **1 箇所のみ** (r17 の SUNLIGHT_COLOR/AMBIENT の 2 経路注入と違って B 軸は単純、pipeline.cpp に直接 push 経路なし)
3. **cloud uniform 一覧** — cloud_color / cloud_pos_density1/2 / cloud_scale / cloud_shadow / cloud_variance / cloud_noise_texture / cloud_noise_texture_next / blend_factor、すべて現役
4. **3D noise の必要性** — 既存 `cloud_noise_texture` (sampler2D) を視線方向に slab raymarch することで体積感を出せる目処、3D noise asset 新規同梱は不要
5. **3 OS 共通性** — `HAS_METAL` hits 0、`.metal` 専用 cloud shader なし、GLSL 共通経路で 3 OS 動作見込み
6. **個別 switch 配線** — `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` を `LLShaderMgr` enum に追加 (r16 の隣)、shader 側で `uniform int aya_r18_cloud_volumetric_enabled;` を読む配線、settings.xml に `AYAR18CloudVolumetricEnabled` 1 件追加

### P1.a: 体積化 (A 軸) 実装 (実装済 — AYA 実機 PASS)

`indra/llrender/llshadermgr.h` (enum 1 件追加):
```cpp
AYA_R18_CLOUD_VOLUMETRIC_ENABLED,   //  "aya_r18_cloud_volumetric_enabled" <FS:AYA r18>
```

`indra/llrender/llshadermgr.cpp` (uniform 名 mReservedUniforms に push):
```cpp
mReservedUniforms.push_back("aya_r18_cloud_volumetric_enabled");  // <FS:AYA r18>
```

`indra/newview/app_settings/settings.xml` (Boolean 1 件、default TRUE):
- `AYAR18CloudVolumetricEnabled` (Persist=1, Value=1)

`indra/newview/llsettingsvo.cpp::applySpecial` (B 軸 CLOUD_COLOR × r17 mod + A 軸 uniform push、master cvar U32 化済):
```cpp
// B 軸: r17 sun mod を CLOUD_COLOR に乗算 (r17 OFF 時は identity)
LLColor3 r17_sun_mod = LLSettingsVOSky::getR17SunModulator(light_direction, psky.get());
shader->uniform3fv(LLShaderMgr::CLOUD_COLOR, LLVector3((psky->getCloudColor() * r17_sun_mod).mV));

// A 軸: shader uniform で slab raymarch を gate
{
    static LLCachedControl<U32>  aya_master(gSavedSettings, "AYAVisualRealismEnabled", 1);
    static LLCachedControl<bool> aya_r18_cloud_vol(gSavedSettings, "AYAR18CloudVolumetricEnabled", true);
    bool is_legacy_midday = (psky && psky->getAssetId() == LLEnvironment::KNOWN_SKY_LEGACY_MIDDAY);
    bool r18_on = (aya_master() != 0) && aya_r18_cloud_vol && !is_legacy_midday;
    shader->uniform1i(LLShaderMgr::AYA_R18_CLOUD_VOLUMETRIC_ENABLED, r18_on ? 1 : 0);
}
```

`indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl` (A 軸 slab raymarch):
```glsl
uniform int aya_r18_cloud_volumetric_enabled;

float alpha1;
if (aya_r18_cloud_volumetric_enabled != 0)
{
    const int N = 4;
    const vec2 slab_offset = vec2(0.013, 0.008);  // UV 空間 slab 進行方向 (視線方向 proxy)
    float trans = 1.0;
    for (int i = 0; i < N; i++)
    {
        float t = (float(i) - 1.5) / 3.0;  // -0.5 ~ +0.5
        vec2 du = slab_offset * t;
        float a = (cloudNoise(uv1 + du).x - 0.5) + (cloudNoise(uv3 + du).x - 0.5) * cloud_pos_density2.z;
        a = min(max(a + cloudDensity, 0.) * 10.0 * cloud_pos_density1.z, 1.);
        a = 1.0 - a * a;
        a = 1.0 - a * a;
        trans *= 1.0 - a * 0.45;  // 各 slab 45% 透過
    }
    alpha1 = 1.0 - trans;
}
else
{
    // Legacy flat path (preset 互換)
    alpha1 = (cloudNoise(uv1).x - 0.5) + (cloudNoise(uv3).x - 0.5) * cloud_pos_density2.z;
    alpha1 = min(max(alpha1 + cloudDensity, 0.) * 10 * cloud_pos_density1.z, 1.);
    alpha1 = 1. - alpha1 * alpha1;
    alpha1 = 1. - alpha1 * alpha1;
}
```

ポイント:
- A 軸 (slab raymarch) を `AYAR18CloudVolumetricEnabled` + master + `KNOWN_SKY_LEGACY_MIDDAY` 除外で gate
- B 軸 (CLOUD_COLOR mod) は `getR17SunModulator` 内で `AYAVisualRealismEnabled` + `AYAR17ColorTemperatureEnabled` + Legacy Midday pinpoint 除外を統合判定、OFF/Legacy では identity を返す設計で素通し
- 「昼間(レガシー)」は PBR 前 noon 再現 preset の意図を歪めないよう pinpoint 除外 (r17 で確立したパターンを継承)
- shader 側 OFF パス (`aya_r18_cloud_volumetric_enabled == 0`) は **既存式と数式上完全一致** (smoothing も含む、preset 互換)

### P1.b: 実装後の体感調整 (条件付、現状は不要 — AYA 実機 PASS)

P1.a の Linux 実機検証で:
- 体積感が perceptual threshold 以下 → slab N step 数 + slab_offset 厚み / transmittance 係数 (0.45) を調整
- GPU コストが目立つ → step 数を削る、early-out 条件追加
- 体感が出ない → drop 検討 (`feedback_feature_value_in_main_usecase.md`)

**実機結果**: AYA 体感 PASS (「とても素晴らしい」) で調整不要、デフォルトパラメータ (N=4, slab_offset=(0.013, 0.008), transmittance 45%/slab) で出荷

### P2: 3 OS ビルド + 体感確認

AYA が Linux フルビルド + 体感確認。問題なければ macOS / Windows ビルドへ。`feedback_release_with_user_feedback.md` の流儀。

### P3: tag / release

r14 / r15 / r16 / r17 と同じく、**公開は r18 単独でせず A 軸完走時 (= r18 close-out 時) に一括公開判断** の運用 (AYA 方針)。A 軸 5 リリース (r14-r18) を `v7.2.4-ayastorm-r18` 等の tag でまとめて公開する想定。

---

## 5. 受け入れ条件

- [x] 既存 WindLight preset (朝・昼・夕・夜・昼間レガシー) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし) — AYA 実機 PASS
- [x] `AYAR18CloudVolumetricEnabled = TRUE` (master `AYAVisualRealismEnabled = TRUE` 前提) で:
  - 雲が flat な板ではなく **厚みと奥行き** を持って見える — AYA 実機「とても素晴らしい」
  - エッジが立体的に削れ、cloud_pos_density / cloud_scale の変化が体積方向にも反映される
  - r14/r15/r16 で出した体感が壊れない
- [x] `AYAR18CloudVolumetricEnabled = FALSE` で r16 までと同じ見え方に戻る (shader 内部で switch off 経路、既存 flat sample のみ実行) — 数式一致で構造的保証
- [x] **「昼間(レガシー)」で A 軸 (slab raymarch) が無効化** (`KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外で flat & 生 cloud_color が流れ、PBR 前 noon 再現 preset の意図を歪めない)
- [x] **sky dome の見え方が r14 P2.a refined のまま** (sun disc 健在、青空質感劣化なし) — skyV.glsl 不触で構造的保証
- [x] **B 軸 (CLOUD_COLOR × r17 sun mod)**: AYAR17 + AYAR18 ON で Sunset preset の sustained viewing で「cinematic な orange 夕焼け雲」が出る (revert の動機) — AYA 実機 PASS
- [x] **View Mode UI**: Preferences → Graphics → Shaders に `Firestorm View / AYAstorm View` combo_box が表示、切替で master cvar (U32) を 0/1 に書き換え、即時反映
- [ ] 3 OS でビルド + 起動 + 表現確認 (P2 — Linux 済、macOS / Windows は A 軸完走時 tag/release で一括)
- [ ] FPS 影響が ±15% 以内 (P2 で実測、体積化分は若干余裕枠)

---

## 6. リスク

| ID | リスク | 対策 / 現ステータス |
|---|---|---|
| R1 | raymarch step が多すぎて GPU コスト顕在化 (低スペック GPU で fps 大幅低下) | step 数 4〜8 で開始、early-out 厳格化、P1.c で実機計測しながら調整。screen-space 全体に raymarch しない方針が予防線 |
| R2 | 3D noise 入手 (texture3D 同梱 or procedural) が 3 OS で挙動差 | **解消 (P0 で 2D noise の slab 化で達成、3D noise asset 不要が確定)**。`cloud_noise_texture` (sampler2D) を視線方向 slab 4 step raymarch で疑似 volumetric 化、新規 asset 配布なし |
| R3 | 既存 preset の cloud 見た目が体積化で破壊される (cloud_pos_density / cloud_scale の意味が変わる) | preset uniform は input 契約として保つ設計、shader 内部で「新計算経路」を switch で gate、OFF で完全 backward compatible |
| R4 | r17 helper の cloud_color 適用で「昼間(レガシー)」の雲が変色 | **解消 (KNOWN_SKY_LEGACY_MIDDAY pinpoint 除外で identity)**。r17 revert で B 軸 (CLOUD_COLOR × r17 mod) 復活、ただし Legacy Midday preset では pinpoint 除外で素通し、PBR 前 noon 再現意図を歪めない |
| R5 | cloud shader (class1/class2 2 系統) の片方だけ書き換えると deferred/forward 経路で見た目が割れる | **解消 (P0 で `class1/deferred` の cloudsV/cloudsF 2 ファイルのみ存在することを確認)**。class2/windlight 配下に同名 shader は無く、deferred 経路 1 系統で完結 |
| R6 | 3 OS でビルドが通らない (macOS Metal cross-compile 等) | shader 改修 + C++ 側は settings.xml 2 件 (r17 + r18) + llshadermgr enum 1 件のみ、`.metal` / `HAS_METAL` 経路は P0 で確認、r17 helper は viewer-only C++ で OS 依存ゼロ |
| R7 | 個別 switch (`AYAR18CloudVolumetricEnabled`) を追加することで `feedback_prefer_defaults_over_config.md` (個別 cvar 量産しない) と衝突 | r14/r15/r16 と同じく「章ごと体感評価用 sentinel」運用、A 軸完走時に統合 (master へ吸収) を検討 |
| R8 | heavy raymarch に倒れて scope が膨張 | spec §3 含まないに「heavy raymarch」を永久 drop として明記、step 数 上限を P1.a で N=4 固定 |
| R9 | 体積化の体感が perceptual threshold 以下 (r16 P1.b / r14 P2.b/c のような drop pattern) | **解消 (AYA 実機 PASS「とても素晴らしい」)**。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を r17 側で実証、A 軸単独で十分体感が出ることを確認 |

---

## 7. 更新履歴

- 2026-05-12 (初版): r17 close-out (`c3d6aee734`) 直後に r18 を起票。AYA との対話で「雲の体積化 (B) + 色温度連動 (A) のセット」スコープに確定 (cloud shadow C は r19+ に分離、scope 膨張回避)。理由は「体積化された雲面に色温度が乗ると cinematic が桁違い、別リリースに分けると単体評価が難しい」。`docs/ayastorm-visual-realism-roadmap.md` §4 r18 entry のスコープ (analytic + 軽量 raymarch、heavy raymarch 不採用) を継承、r17 で実装済 helper `LLSettingsVOSky::getR17SunModulator` を CLOUD_COLOR uniform にも適用する設計。次は P0 Survey (cloud shader class1/class2 経路調査)
- 2026-05-12 (P0 Survey 完了 `636e163a3c`): `doc/r18/cloud_volumetric_survey.md` Round 1 で 6 項目すべて PASS。class1/deferred の cloudsV/cloudsF のみ存在 (class2 系統 同名 shader 無し)、CLOUD_COLOR 注入点は `llsettingsvo.cpp:890` の 1 箇所のみ、既存 cloud は完全 2D (3D noise asset 不要)、`AYA_R18_CLOUD_VOLUMETRIC_ENABLED` shader enum を新設要、HAS_METAL 0 hits、drawpool bind 経路不触。R2 (3D noise 入手) / R5 (class1/class2 割れ) は P0 で解消、spec §2/§3/§6 を実状に整合
- 2026-05-12 (P1.a 実装): `llshadermgr.{h,cpp}` enum + uniform 名追加、`settings.xml` に `AYAR18CloudVolumetricEnabled` (default TRUE)、`llsettingsvo.cpp::applySpecial` で B 軸 (CLOUD_COLOR を r17 Kelvin で乗算) + A 軸 gate uniform push、`cloudsF.glsl` で A 軸 slab raymarch (N=4 step、Beer-Lambert 風 transmittance 45%/slab)。A 軸 / B 軸 ともに `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外 (r17 と同思想)。OFF パスは既存式と数式上完全一致 (preset 互換維持)。次は AYA 実機確認 (Linux ビルド + 4 preset 切替 + AYAR18 ON/OFF / AYAR17 ON/OFF の組合せ)
- 2026-05-12 (実機検証 + r17 drop に伴う B 軸 同伴 drop): AYA 実機で AYAR18 (A 軸 slab raymarch) は「とても素晴らしい」評価で PASS、AYAR17 (色温度) は 5 preset 切替で効きが知覚できず。診断ログ (`AYA_R17` tag) で原因確定: SL の Sunrise preset は太陽 elevation=0.996 (zenith) で K=6500=identity、Sunset は warm preset でツールクリップ、Midnight は ambient に副作用、Day cycle (preset=00000000) のみ effective、という「fixed preset では機能せず」の偏りを確認。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を厳格適用し r17 全体を drop、r18 B 軸 (CLOUD_COLOR の r17 mod) も helper 消滅で自動 drop。r18 は **A 軸 (slab raymarch) 単独で出荷**、cloud_color は preset 由来素通しに戻る。spec を「A 軸単独」に re-scope (タイトル変更、§1/§2/§3/§4/§5/§6 更新)、`docs/ayastorm-r17-color-temperature.md` を drop 記録へ書き換え。次は AYA 実機 P2 (3 OS ビルド + 体感確認) — Linux 済、macOS / Windows は A 軸完走時 tag/release で一括
- 2026-05-12 (r17 revert + B 軸復活 + View Mode UI + master cvar U32 化): drop commit `380f5dc245` 後の sustained viewing で AYA が「Dropしたオレンジの夕焼けの世界は失われた」「夕焼けの美しさはなくなってしまった」と評価 → instant A/B では「効きがわからない」だった r17 が cumulative には効いていたことが判明、`feedback_doubt_self_first.md` を自分の drop 判断にも適用して **r17 全体を revert**。`getR17SunModulator` / `kelvinToRGB` / `AYAR17ColorTemperatureEnabled` を `c3d6aee734` の内容で復活、sky path / scene path / cloud path B 軸 (r18) の 3 注入点 modulator を復元。あわせて UI 露出 (Preferences → Graphics → Shaders に `AYAViewMode` combo_box) を panel_preferences_graphics1.xml (en/ja) に追加、`Firestorm View / AYAstorm View` の 2 択で master cvar `AYAVisualRealismEnabled` を 0/1 で書き換え。combo_box value="0"/"1" 文字列 ↔ Boolean cvar の LLSD 型 coercion が不安定で「差が殆どない」現象が出たため master cvar を **Boolean → U32** に変更 (default=1)、C++ 3 サイト (`llsettingsvo.cpp::applySpecial` 2 箇所、`pipeline.cpp::doGodrays`) を `LLCachedControl<U32>` + `() != 0` 判定に統一。spec タイトルを「雲の体積化 + 色温度連動」に戻し §1/§2/§3/§4/§5/§6 を B 軸復活で更新、`docs/ayastorm-r17-color-temperature.md` を「REVERTED — P1.a 復活」へ書き換え
