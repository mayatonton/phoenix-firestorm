# AYAstorm r30 Cinematic Controls Cleanup — Phase A spec

- **Status**: DRAFT (2026-05-22)
- **Author**: AYA / Claude (audit + spec)
- **Base commit**: `e676c52b87` (r30 BD full port Phase 6 polish — Cinematic floater 完成直後、layered DoF 実験を含まないクリーンな Phase 6 状態)
- **Work branch**: `experiment/r30-cinematic-controls-cleanup`
- **追従ポリシー**: `ayastorm-release` は他開発者が並行 commit 中のため、本ブランチへは ayastorm-release を **IN にマージ** して追従する (本ブランチを ayastorm-release へ早期 merge しない)
- **Related**:
  - `docs/specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md` (Cinematic floater 構築の原典)
  - `docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md` (BD live cvar 配線完了)
  - `docs/specs/ayastorm-r30-p5-bd-ui-binding-audit-spec.md` (前回の UI binding audit)
  - `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` (対象 floater)
  - `docs/specs/ayastorm-r30-view-mode-reshuffle.md` (後続: Cinematic を AYAstorm View に promote、r30 release 直前の rename + migration)

## 0. 背景

2026-05-21〜22 にかけて Cinematic Controls floater (`floater_aya_cinematic.xml`,
全 35 cvar / 9 tab) を 5 並列 agent で完全 trace 監査した結果、以下が判明:

- **完全 DEAD: 5 件** — XUI からは設定できるがコード上で効かない
- **部分 DEAD (dead zone あり): 7 件** — 範囲の一部が無効
- **単位 / Label 不一致: 3 件** — XUI 表記と実装が乖離
- **合成順序の罠: 1 件** — 仕様としては正しいが UX 注意要

本 spec は **完全 DEAD 5 件** を Phase A として優先処置する。
部分 DEAD / 単位不一致 / tooltip 補強は Phase B/C/D に分け、後続作業として別 branch で実施する。

## 1. Phase A スコープ (今回着手 5 件)

| ID | 対象 cvar | 現状 | 処置 |
|---|---|---|---|
| **A.1** | `RenderShadowDetail=3` | dispatch なし。**追加調査結果**: SL/LL では projector = spot light の特殊形で shadow 経路は `mSpotShadow[]` 共有、現 `>1` 既に spot+projector 両方カバー。3 で増やせる shadow は無い | **廃止**: XUI max=2 に縮小、tooltip 訂正 |
| **A.2** | `RenderFSAAType=3` | dispatch なし (`==1` FXAA / `==2` SMAA のみ) | **生かす**: `==3` 分岐追加 → SMAA T2x 強制 ON |
| **A.3** | `RenderShadowResolutionScale` (Cinematic 時) | Cinematic では Vector4 `RenderShadowResolution` 直接使用、Scale 無視 | **生かす**: Vector4 各成分に Scale を乗算 |
| **A.4** | `RenderDeferred` master toggle | `connectRefreshCachedSettingsSafe` が DEPRECATED コメント、cached 連動なし。Cinematic は deferred 前提のため OFF にすると floater 全体壊滅 | **廃止**: floater General tab から checkbox 撤去 (cvar 自体は LL 標準で touched せず) |
| **A.5** | `RenderSMAAT2x` checkbox | spec §3.1 BD parity で Cinematic 時 `t2x_active=false` 固定 → 永久 dead | **廃止**: A.2 の `FSAAType=3` で機能吸収、checkbox + cvar 撤去 |

### 1.1 A.1 / A.4 / A.5 廃止 理由

- **A.1 (RenderShadowDetail=3)**: 当初は projector light shadow を新規に有効化する想定だったが、`pipeline.cpp:13060-13219` 再 trace の結果、LL/SL では projector light は spot light の派生 (texture 投影付き spot light) で shadow generation 経路は `mShadowSpotLight[]`/`mSpotShadow[]` 配列で共有。現状の `RenderShadowDetail > 1` (level 2) で **既に spot + projector 両方の shadow が生成されている**。level 3 で **追加できる shadow が存在しない** (LL / BD どちらにも先がない)。新機能を level 3 に押し込むのは別 scope (terrain/water shadow 等、数週間) なので、Phase A では XUI を `max=2` に縮小し tooltip を訂正、cvar 自体は LL 標準のため touched せず。
- **A.4 (RenderDeferred)**: 生かすには deferred OFF path を全 dispatcher (8 pool × shader 経路) で維持する必要 → 数週間 + perpetual maintenance。AYAstorm 章 thesis (写真を撮るに値する空気) と forward-only path が完全矛盾。**Cinematic floater スコープ外**として削除。
- **A.5 (RenderSMAAT2x)**: A.2 で FSAAType enum (0/1/2/3) に T2x を吸収すれば二重制御解消。AA 経路が単一 enum で完結 → UX 明快化。

## 2. 実装詳細

### A.1 — RenderShadowDetail=3 撤去 (XUI max=2 縮小)

**現状コード** (`indra/newview/pipeline.cpp`):
- L10733: `if ((RenderDeferredSSAO && !gCubeSnapshot) || RenderShadowDetail > 0)` (SSAO blur enable)
- L13072: `bool gen_shadow = RenderShadowDetail > 1;` (spot light **+ projector light** shadow generation)
- L13060-13219: `mShadowSpotLight[]` / `mSpotShadow[]` 配列に spot/projector 両方が格納される (LL は projector を spot の派生として扱う)
- → `> 2` 分岐は存在しない、3 は 2 と完全同義 (dead 値)

**変更案**:
- **C++ 変更なし** (cvar 本体は LL 標準)
- XUI `floater_aya_cinematic.xml` (sb_ShadowDetail / s_ShadowDetail) を変更:
  - `max_val="3"` → `max_val="2"`
  - tooltip: `"0=off, 1=sun, 2=sun+spot+projector"` (現 "0=off, 1=sun, 2=sun+spot, 3=all" は誤り)
- `llviewermenu.cpp::AYAResetCinematic::parityTable()` で `RenderShadowDetail` の default 値が 3 になっていれば 2 に訂正 (要 grep 確認)

**3 OS 影響**: XUI / preset table のみ、等値。

**Migration 注意**: 既存ユーザーで `RenderShadowDetail=3` を debug settings で設定している人は LL 標準として残るが、Cinematic floater からは選べなくなる。実害なし (3 は元々 dead)。

### A.2 — RenderFSAAType=3 (SMAA T2x)

**現状コード** (`indra/newview/pipeline.cpp:10036-10068`):
```cpp
if (RenderFSAAType == 1) {
    applyFXAA(sourceBuffer, targetBuffer);
    std::swap(sourceBuffer, targetBuffer);
}
else if (RenderFSAAType == 2) {
    generateSMAABuffers(sourceBuffer);
    applySMAA(sourceBuffer, targetBuffer);
    std::swap(sourceBuffer, targetBuffer);

    // 既存 RenderSMAAT2x checkbox 経由 T2x dispatch (A.5 で撤去対象)
    const bool smaa_t2x = gSavedSettings.getBOOL("RenderSMAAT2x");
    bool t2x_active = smaa_t2x && mVelocityMap.isComplete() && mSMAAHistory.isComplete() && !gCubeSnapshot;
    sT2xJitterEnabled = t2x_active;
    if (t2x_active) {
        resolveSMAAT2x(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);
        mSMAAFrameIndex ^= 1;
    }
}
```

**変更案** (A.2 + A.5 連動):
```cpp
if (RenderFSAAType == 1) {
    applyFXAA(sourceBuffer, targetBuffer);
    std::swap(sourceBuffer, targetBuffer);
}
else if (RenderFSAAType == 2 || RenderFSAAType == 3) {
    generateSMAABuffers(sourceBuffer);
    applySMAA(sourceBuffer, targetBuffer);
    std::swap(sourceBuffer, targetBuffer);

    // <FS:AYAstorm r30 cleanup A.2/A.5> FSAAType=3 で T2x 強制 ON。
    // 旧 RenderSMAAT2x cvar は廃止 (A.5)。
    bool want_t2x = (RenderFSAAType == 3);
    bool t2x_active = want_t2x && mVelocityMap.isComplete() && mSMAAHistory.isComplete() && !gCubeSnapshot;
    sT2xJitterEnabled = t2x_active;
    if (t2x_active) {
        resolveSMAAT2x(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);
        mSMAAFrameIndex ^= 1;
    }
    // </FS:AYAstorm>
}
```

**Cinematic mode における T2x ON 是非** (要 AYA 判断):
- 現 spec §3.1 では「BD parity 維持 → Cinematic は T2x なし」
- A.2 が成立すると、Cinematic でも FSAAType=3 を選べば T2x が動作する
- **spec §3.1 を一部更新する必要**: 「BD parity は FSAAType=2 を default としつつ、user opt-in (FSAAType=3) で AY-only 拡張 T2x を許可」と再定義
- → **Cinematic preset の default は 2 のまま (BD parity)**、ユーザーが明示的に 3 を選んだら T2x、という設計が thesis と両立する

**3 OS 影響**: `resolveSMAAT2x` / `mSMAAHistory` 等は既存 cross-platform 実装。等値。

**XUI 修正**: `floater_aya_cinematic.xml`
- L271-274 (sb_FSAAType / s_FSAAType): 現状の tooltip `"0=Off, 1=FXAA, 2=SMAA (Cinematic preset), 3=SMAA T2x"` のまま継続 (今までは嘘表記、A.2 で真になる)
- **L276-277 (cb_SMAAT2x + d_SMAAT2x): 撤去** (A.5)

### A.3 — RenderShadowResolutionScale (Cinematic 時)

**現状コード** (`indra/newview/pipeline.cpp:1234-1266`):
```cpp
F32 scale = gCubeSnapshot ? 1.0f : llmax(0.f, RenderShadowResolutionScale);
U32 sun_shadow_map_width  = BlurHappySize(resX, scale);
U32 sun_shadow_map_height = BlurHappySize(resY, scale);

const bool cinematic_per_channel_shadow = isCinematicMode() && !gCubeSnapshot;

if (shadow_detail > 0) {
    for (U32 i = 0; i < 4; i++) {
        if (cinematic_per_channel_shadow) {
            U32 res = (U32)RenderShadowResolution.mV[i];  // ← scale を使ってない
            if (mRT->shadow[i].getWidth() != res) {
                if (!mRT->shadow[i].allocate(res, res, 0, true)) return false;
            }
            continue;
        }
        if (!mRT->shadow[i].allocate(sun_shadow_map_width, sun_shadow_map_height, 0, true)) return false;
    }
}
```

**変更案 (sun cascade + projector shadow 両方に対称適用)**:
```cpp
// sun cascade (4 本)
if (cinematic_per_channel_shadow) {
    U32 res = (U32)llmax(64.f, RenderShadowResolution.mV[i] * scale);
    if (mRT->shadow[i].getWidth() != res) {
        if (!mRT->shadow[i].allocate(res, res, 0, true)) return false;
    }
    continue;
}

// spot/projector shadow (2 本) — sun と同じ scale を適用
if (cinematic_per_channel_shadow) {
    U32 res = (U32)llmax(64.f, RenderProjectorShadowResolution.mV[i] * scale);
    if (!mSpotShadow[i].allocate(res, res, 0, true)) return false;
    continue;
}
```

`llmax(64.f, ...)` は安全弁: scale=0 で 0px allocate 防止 (最小 64px = 1 tile 単位)。sun と projector を同じ scale で動かすことで「shadow 全体の解像度ノブ」として直感的に動作する。

**3 OS 影響**: shadow map allocation 共通、等値。

**XUI 修正**: `floater_aya_cinematic.xml:112-114`
- 現状: Cinematic 時 disabled 提案だったが、A.3 で実効化されたので enabled のまま継続
- tooltip 追加: `"Scales the Cinematic per-cascade base resolutions (Vector4) uniformly."`

### A.4 — RenderDeferred checkbox 撤去

**変更**: `floater_aya_cinematic.xml:68-69` (General tab の 2 行) を削除。

**注**: settings.xml の `RenderDeferred` cvar 本体は LL 標準のため touched せず。Firestorm View tab / 詳細グラフィックス floater 経由でアクセスする ユーザーは引き続き可能。Cinematic Controls は **deferred 必須前提** であることを floater header の文言で明示。

**Header 文言追記** (`floater_aya_cinematic.xml:39-50` の T_Header):
- 末尾に `"All Cinematic controls require Deferred Rendering to be enabled (Preferences → Graphics)."` を追加

### A.5 — RenderSMAAT2x checkbox + cvar 撤去

**変更**:
1. `floater_aya_cinematic.xml:276-277` の checkbox + D button 2 行を削除
2. `indra/newview/app_settings/settings.xml` から `RenderSMAAT2x` cvar 定義削除 (要 line 特定)
3. `pipeline.cpp:10057` の `getBOOL("RenderSMAAT2x")` 行を削除 (A.2 の変更で代替)
4. `llviewermenu.cpp::AYAResetCinematic::parityTable()` に `RenderSMAAT2x` があれば削除 (要確認)
5. その他 grep で残党チェック

**3 OS 影響**: 設定削除のみ、等値。

**Migration 注意**: 既存ユーザーの `RenderSMAAT2x=1` 設定は cvar 削除で無視される。代わりに `RenderFSAAType=3` に migrate する必要があるが、ユーザー数は少数と推測 (BD parity で長らく Cinematic では効かなかったため)。release notes で告知。

## 3. 作業順序

| step | 内容 | 検証 |
|---|---|---|
| 1 | `ayastorm-release` から `experiment/r30-cinematic-controls-cleanup` branch 作成 | `git switch -c` 確認 |
| 2 | A.4 実装 (XUI 2 行削除 + Header 文言追加) | XUI 起動確認 |
| 3 | A.1 実装 (XUI max=3→2, tooltip 訂正 + parityTable 値確認) | XUI 起動確認、ShadowDetail スライダーが 0-2 になることを確認 |
| 4 | A.3 実装 (pipeline.cpp 1 行 + 安全弁) | ビルド + 起動 + Cinematic mode で shadow scale 変えて確認 |
| 5 | A.2 + A.5 実装 (pipeline.cpp dispatch 変更 + settings.xml cvar 削除 + XUI checkbox 削除) | ビルド + 起動 + FSAAType=3 で T2x 動作確認、cvar grep で残党なし確認 |
| 6 | Release note 草案を `docs/release/` に追加 | content review |
| 7 | AYA 動作確認 → OK ならコミット (1 step 1 commit) | git log |

各 step 完了ごとに **build + 起動確認**。AYA がスクショ撮って確認 → OK 確定後に次に進む。

**注**: A.1 / A.4 はビルド不要 (XUI のみ変更)、A.3 / A.2+A.5 はビルド必要。

## 4. Branch / Commit 戦略

- **Branch**: `experiment/r30-cinematic-controls-cleanup` を `e676c52b87` (Phase 6 polish) から分岐
- **Commit 粒度**: 1 step = 1 commit (A.4, A.1, A.3, A.2+A.5, release notes の 5 commit)
- **Co-Authored-By: Claude 行は付けない** (AYAstorm 既存方針)
- AYA さん明示指示後にコミット (autonomous commit しない)
- `ayastorm-release` の他開発者更新は `git merge ayastorm-release` で本ブランチへ取り込む。逆方向 (本ブランチ → ayastorm-release) は r30 cleanup 全完了後 AYA 判断で実施

## 5. 検証方法

### 5.1 静的検証 (Claude 自分で実施)
- 各 step 後に `grep` で関連 cvar 残党チェック
- `pipeline.cpp` build (autobuild → make) で type 確認

### 5.2 動的検証 (AYA さん実施)
- Cinematic mode 起動 → 各 cvar 操作 → 期待挙動確認
- A.1: ShadowDetail スライダーが 0-2 に縮小されていること、tooltip が新表記になっていること
- A.2: FSAAType 0→1→2→3 動かして AA 効果差分 (静止画と pan で ghosting 観察)
- A.3: ShadowResolutionScale 0.5→1.0→2.0→3.0 で shadow 解像度差分
- A.4: General tab に Deferred toggle が無いこと
- A.5: Motion Blur tab に SMAA T2x checkbox が無いこと

### 5.3 退行検証
- 既存 Cinematic preset (D button) が引き続き機能する
- AYAVisualRealismEnabled=0/1 (Firestorm View / AYAstorm View) で挙動退行なし
- 他 cvar 操作が引き続き live apply

## 6. Phase B/C/D 後続スコープ (本 spec 範囲外、別 branch で実施)

### Phase B — 部分 DEAD (dead zone) 縮小
- `RenderPostPosterizationSamples`: XUI `min_val="1"` → `"3"` (shader gate `> 2`)
- `RenderVolumetricLightingMultiplier`: XUI `max_val="200"` → `"80"` (tonemap saturate 領域排除)
- `RenderScreenSpaceReflectionAdaptiveStepMultiplier`: XUI `min_val="0.1"` → `"1.0"` (FP32 underflow 排除)
- `RenderShadowFarClip` tooltip 追加 ("capped by RenderFarClip")
- `RenderGlowMinLuminance` tooltip 追加 ("HDR=OFF では 0..1 のみ意味")
- `RenderScreenSpaceReflection RayStep × Iterations` tooltip ("積が ~50m 以下が実用")
- `RenderVolumetricLightingFalloffMultiplier` tooltip ("near-field only attenuation")

### Phase C — 単位 / Label 不一致
- `CameraFieldOfView` label を "DoF focal FOV (°)" 等に変更 (viewport FOV と区別)
- `CameraFNumber` / `FocalLength` label に "(DoF blur formula only)" 等の補足追加

### Phase D — 合成順序 tooltip 補強
- `RenderPostGreyscaleStrength` / `SepiaStrength` のいずれかに「両方 1.0 で淡茶色 (グレースケール先 → セピア後)」tooltip

## 7. リスク / 既知の不確定事項

| リスク | 対策 |
|---|---|
| ~~A.1 projector shadow path が LL/Firestorm で完全実装されていない可能性~~ | **確定**: projector = spot 派生で `>1` で既に動作、`>2` で増やせる shadow 無し。Phase A では XUI max=2 縮小に scope 変更済 (§1 / §2 A.1 参照) |
| A.2 で T2x 強制 ON すると Cinematic spec §3.1 (BD parity) と部分矛盾 | spec §3.1 を「BD parity default + user opt-in T2x」と再定義、別途 commit |
| A.5 cvar 削除で既存ユーザー設定が migrate されない | release notes に明示 + 旧 cvar 値が残っていても無害であることを確認 |
| `RenderShadowResolutionScale=0` で 0px allocate | `llmax(64.f, ...)` 安全弁で防止 |
| A.4 撤去後に "deferred が OFF だと SSAO 効かない" 等の Q&A 発生 | Header 文言で deferred 必須を明示 |

## 8. メモ

- Phase A スコープを 5 件に絞ったのは「dead を生かす / 削除する」という単一テーマで束ねるため
- 「dead zone 縮小 / 単位修正 / tooltip 補強」は意味的に別軸 → Phase B/C/D に分離
- 全 Phase を 1 branch にまとめるのは scope 単一性 (memory: feedback_experiment_branch_single_scope.md) に反するので避ける

---

**次のアクション**: AYA さん本 spec を review → OK なら step 1 (branch 作成) から着手。
