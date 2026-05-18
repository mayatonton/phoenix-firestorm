# AYAstorm r30 P4 — BD DoF chain (HQ DoF + 色収差) 取り込み 事前依存マップ (BD trace)

**作成日**: 2026-05-18
**最終更新**: 2026-05-18 (Tier-1 trace 完了、§5 改修項目 / §6 実装ステップ案 策定)
**親 spec**: `ayastorm-r30-cinematic-chapter.md` §3 P4 / §4.2 取り込みリスト
**前段 spec**: `ayastorm-r30-p3-volumetric-lighting-bd-trace.md` (P3 = Volumetric Lighting trace、shader 取り込み未実装)
**スコープ**: P4 着手前の事前 trace。BD repo 内の DoF chain (HQ DoF shader + chromatic aberration feature) を上から下まで追い、AYAstorm 側に取り込む際の改修ポイントを file:line 単位で確定する。**実装は含まない**。
**BD 参照 commit**: `995a1354d8` (Version to 5.6.2, 2026-04-19) — P2/P3 と同じ参照点
**Firestorm 参照ブランチ**: `ayastorm-release` HEAD = P3 spec ship 時点 (branch `feature/ayastorm-r30-p4-bd-dof-chain-spec`、本 spec を commit する branch)

---

## 1. 概要

### 1.1 P4 スコープの再定義 (chapter §3 P4 からの変更点)

`ayastorm-r30-cinematic-chapter.md` §3 P4 の当初記述は **Motion Blur + BD DoF chain** だったが、Motion Blur は P2 (`renderMotionBlurComposite` + step 5b/5c/5d/5e) で composite 経路まで含めて ship 済。よって P4 残スコープは **BD DoF chain 部分のみ**:

1. **HQ DoF shader 取り込み**: `class1/deferred/postDeferredHQDoFF.glsl` (新規ファイル、BD でのみ存在) + 既存 `gDeferredPostProgram` 登録ルートに HQ/標準切替分岐を追加
2. **Chromatic Aberration 機能**: 3 件の cvar (`RenderDepthOfFieldChroma`, `RenderChromaStrength`, `RenderDepthOfFieldHighQuality`) + `HAS_DOF_CHROMA` permutation + `DEFERRED_CHROMA_STRENGTH` uniform + 既存 4 shader 内 chroma サンプリング block 追加
3. **既存 DoF shader への chroma block 注入**: `postDeferredF.glsl` / `postDeferredHQDoFF.glsl` / `postDeferredNoDoFF.glsl` の 3 本に `#if HAS_DOF_CHROMA` ガード付き色収差 sampling コードを追加
4. **uniform push 配線**: pipeline.cpp の 3 箇所 (renderDoF / renderFinalize / bindDeferredShader) に `DEFERRED_CHROMA_STRENGTH` uniform push を追加
5. **Front Blur 取り込み** (2026-05-18 追加、§7.2 で AYA 判断確定): `RenderDepthOfFieldFront` cvar (BD default=1) + `FRONT_BLUR` permutation を Cinematic+HQ DoF 同時有効時に付与。`postDeferredHQDoFF.glsl` 内の `#if FRONT_BLUR` ガード block は BD 由来コードのまま byte-preserve。**判断根拠**: 章 §1.1「BD と並走する撮影 viewer」thesis 整合、控えめ default で出荷すると初見比較で BD に劣後する印象 (`feedback_match_bd_defaults_on_borrow.md`)

スコープ外:
- BD UI (`panel_preferences_graphics1.xml` の DoF/chroma スライダー UI / `panel_machinima.xml` の Photo Tools chroma UI) — chapter §1.2 「BD UI を取り込まない」方針通り
- BD `RenderDepthOfFieldAlphas` (alpha pass DoF) — P3 §4.5 で取り込み判断が懸案中、P4 でも別 phase 扱い
- (削除) BD `RenderDepthOfFieldFront` (front blur 経路) — 当初スコープ外だったが、AYA さん 2026-05-18 判断で **取り込み確定** (§5.6/§7.2 参照、BD default=1 / 章 §1.1 整合 / 初見比較で BD と勝負)
- `lldrawpoolwater.cpp` の chroma uniform push (BD 独自の water chroma) — Firestorm の water pipeline は別物、無理に踏むと深層 regression のリスクが高いので P4 スコープ外
- `LLPipeline::renderMotionBlurComposite` 系統 (P2 で ship 済、変更不要)

### 1.2 既存 AYAstorm の DoF 機構との関係

AYAstorm (= Firestorm fork HEAD) は FS:Beq の **FIRE-16728 free-aim DoF** mechanism を保持している。BD は別のメカニズム (`CameraFreeDoFFocus` / `CameraDoFLocked` 静的メンバ + `setRenderFocusPoint` 経由) を使う。

**P4 では BD のフォーカス機構は取り込まない**。理由:

- FIRE-16728 は Firestorm 既存ユーザー (Beq による FS 標準 free-aim) との互換性を破壊しないために維持必須
- BD の `CameraFreeDoFFocus` / `CameraDoFLocked` は Photo Tools UI と密結合しており、AYAstorm が `panel_machinima.xml` を取り込まない以上「UI なしで static cvar だけ吊るす」になる → 死蔵
- HQ DoF shader と chroma feature は **focus 機構非依存** (CoF buffer の `s.a` 値だけ参照、focus point の計算経路は変えない) なので、FS:Beq 機構を残したまま HQ shader + chroma を盛ることが可能

つまり P4 は **「DoF 内部の sample 精度 + 色収差表現」のみ** を BD から borrow し、「ピントをどこに合わせるか」の機構は Firestorm 既存 (FIRE-16728) を踏襲する hybrid 構成になる。

### 1.3 Cinematic gate 方針

P2/P3 と同じ流儀 (`AYAVisualRealismEnabled == 2` で gate)。ただし以下 2 点で P3 までと挙動が異なる:

- **HQ DoF shader 取付け** (`gDeferredPostProgram` の shader file 分岐) は **起動時 1 回構築**。Cinematic mode 切替時に shader を入れ替える設計ではなく、起動 mode に応じて shader file を選択 → そのまま使う (= P1 で確立した「再起動切替」方針通り)
- **Chroma permutation + uniform push** はランタイム判定ではなく shader compile 時 permutation で完全制御。Cinematic 起動時のみ HAS_DOF_CHROMA 付き shader が compile される

これにより per-frame cost は permutation 切替の分岐すらない (compile out される) ため、AYAstorm View / Firestorm View 側に一切の性能影響を出さない。

### 1.4 本 spec の位置づけ

P4 着手時に作業者 (AYA さん / Claude) が「BD のどこを見ればよいか」「Firestorm のどこに何を入れるか」を spec 1 本で把握できる状態にする。trace は §3 で完了済、§5 改修項目・§6 実装ステップ案までセット。§7 未確定事項は P4 着手時に再 fetch / 再確認が必要。

---

## 2. shader ファイル内容要約

### 2.1 `class1/deferred/postDeferredHQDoFF.glsl` (BD 新規、AYAstorm 不在)

BD repo 内に存在、AYAstorm には**ファイル自体が無い**。実体は LL 標準 `postDeferredF.glsl` の HQ 版で、以下が違う:

| 違い | 標準 (`postDeferredF.glsl`) | HQ (`postDeferredHQDoFF.glsl`) |
|---|---|---|
| `dofSample()` 内 CoF 計算 | `sc = abs(s.a*2.0-1.0)*max_cof` | `sc = abs(s.a*2.0-1.0)*(max_cof*4)` (4 倍係数) |
| `dofSample()` 内 depth ガード | 無し (常に sample) | `if(s.a <= depth*0.50)` (sampled pixel の CoF が depth の 50% 以下なら採用) |
| `dofSample()` 引数 | `(diff, w, min_sc, tc)` | `(diff, w, min_sc, tc, depth)` (depth が追加引数) |
| FRONT_BLUR permutation | 暗黙 (`if (sc > 0.5)` 直入り) | `#if FRONT_BLUR` ガード (front blur を opt-out 可能) |
| chroma サンプリング | 無し | `#if HAS_DOF_CHROMA` block で per-channel offset サンプリング |
| uniform 追加 | `diffuseRect`, `inv_proj`, `screen_res`, `max_cof`, `res_scale` | 同上 + `depthMap` + `chroma_str` (HAS_DOF_CHROMA 時のみ effective) |
| clampHDRRange 呼び出し | 有り (末尾) | 無し (BD は HDR clamp を上流で実施する前提) |

**ファイル header**: BD のファイルは header コメントが stale で `@file postDeferredF.glsl` のまま (BD が標準ファイルから duplicate して編集した痕跡)。AYAstorm 取り込み時は header の `@file` を `postDeferredHQDoFF.glsl` に修正し、その下に provenance コメントを追加する。

### 2.2 chroma サンプリング block (HQ DoF 内)

`dofSample()` 内 (BD postDeferredHQDoFF.glsl line 48-57):

```glsl
#if HAS_DOF_CHROMA
    vec3 col_offset = vec3(0.0015, 0.0000, 0.0005);
    float mult = sc * (chroma_str * 0.2);
    col_offset *= vec3(mult);

    s.r = texture(diffuseRect, tc + vec2(col_offset.x)).r;
    s.g = texture(diffuseRect, tc + vec2(col_offset.y)).g;
    s.b = texture(diffuseRect, tc + vec2(col_offset.z)).b;
    s.a = texture(diffuseRect, tc).a;
#endif
```

**意図**: CoF (= sc) に比例して R/B チャネルを ±方向にオフセットさせ、ボケが大きい領域ほど色収差が強く見えるようにする。G チャネルは中心 (offset = 0)、R が +0.0015、B が +0.0005 (= R/B 非対称、いわゆる「purple fringing」を弱めに模擬)。

**`chroma_str` の effective 範囲**: BD default は `RenderChromaStrength = 0.0` (= effect 完全 off)。permutation `HAS_DOF_CHROMA` が compile に乗っていても strength 0 なら追加 sampling コスト分だけかかって結果は標準 sampling と同じ。AYAstorm 出荷時 default も同様 (0.0) で「機能は compile in、強度は UI で opt-in」というモデルにする。

### 2.3 chroma サンプリング block (NoDoF 経路)

`postDeferredNoDoFF.glsl` 内 (BD line 82-):

```glsl
#if HAS_DOF_CHROMA == 0
    // 標準パス (chroma off): texture をそのまま読む
#else
    // HAS_DOF_CHROMA == 1 経路: chroma offset サンプリング
#endif
```

NoDoF (= DoF 無効時の post pass) は ボケが無いので CoF (sc) ベースの強度変調が出来ない。BD は固定 offset で chroma を乗せる (全画面で薄く色収差が乗る)。**P4 ではこの NoDoF chroma 経路も取り込む** (BD と挙動を揃えるため)。

### 2.4 `postDeferredF.glsl` (LL 既存) への chroma block 追加

BD line 45 で `#if HAS_DOF_CHROMA` ガードが入っている (AYAstorm 既存ファイルには存在しない)。標準 (非 HQ) DoF 経路にも chroma 表現を入れるため、AYAstorm の `class1/deferred/postDeferredF.glsl` 末尾の `dofSample()` 関数内に同等の chroma block を追加する必要がある。

---

## 3. BD pipeline トレース (cvar / shader register / uniform push)

### 3.1 cvar 3 件 (BD `settings_blackdragon.xml`)

| cvar | Type | Value | Persist | Comment (BD 原文) |
|---|---|---|---|---|
| `RenderDepthOfFieldHighQuality` | Boolean | 0 | 1 | "Enable the old, slow, high quality Depth of Field." |
| `RenderDepthOfFieldChroma` | Boolean | 1 | 1 | "Use Chromatic Aberration in Depth of Field, disables Chromatic Aberration everywhere else." |
| `RenderChromaStrength` | F32 | 0.0 | 1 | "Set the strength of chromatic abberation." |

**BD default 解釈**:
- HQ DoF は default off (重い処理、user opt-in)
- Chroma permutation は default on (compile out 戻しのコストを払うより compile in しておく、strength で実効制御)
- Chroma strength は default 0 (= 機能存在するが見た目変化なし、UI で dial in する設計)

cvar 3 件はすべて `settings_blackdragon.xml` 側 (BD 独自 settings file)。AYAstorm では P2/P3 同様に **Firestorm 標準 `settings.xml` に統合** (理由: BD UI を取り込まない以上、独立 settings file を読む infrastructure を新設しない方針)。

### 3.2 cvar 1 件 (BD `llviewercontrol.cpp:1217-1218`)

```cpp
gSavedSettings.getControl("RenderDepthOfFieldHighQuality")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
gSavedSettings.getControl("RenderDepthOfFieldChroma")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
```

`RenderDepthOfFieldHighQuality` と `RenderDepthOfFieldChroma` は **shader rebuild trigger** に配線されている。値を変えると `handleSetShaderChanged` 経由で shader 全体 (mShaderList) が createShader() で再生成される。AYAstorm 取り込み時は **これを取り込まない** (理由: chapter §1.2 「再起動切替に統一」方針通り、shader rebuild はランタイムで走らせない)。

`RenderChromaStrength` は uniform push なので shader rebuild 不要、signal 配線無し。

### 3.3 shader register (BD `llviewershadermgr.cpp:2894-3007`)

#### 3.3.1 `gDeferredPostProgram` (BD:2894-2919)

```cpp
gDeferredPostProgram.mShaderFiles.clear();
gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
if (gSavedSettings.getBOOL("RenderDepthOfFieldHighQuality"))
{
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredHQDoFF.glsl", GL_FRAGMENT_SHADER));
}
else
{
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER));
}
gDeferredPostProgram.mShaderLevel = mShaderLevel[SHADER_DEFERRED];
if (gSavedSettings.getBOOL("RenderDepthOfFieldChroma"))
{
    gDeferredPostProgram.addPermutation("HAS_DOF_CHROMA", "1");
}
if (gSavedSettings.getBOOL("RenderDepthOfFieldFront"))
{
    gDeferredPostProgram.addPermutation("FRONT_BLUR", "1");
}
success = gDeferredPostProgram.createShader();
```

AYAstorm 現在 (`llviewershadermgr.cpp:2987-2997`) は HQ 分岐なし、permutation なし、`postDeferredF.glsl` 直結。

#### 3.3.2 `gDeferredPostNoDoFProgram` (BD:2975-2988)

```cpp
gDeferredPostNoDoFProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoDoFF.glsl", GL_FRAGMENT_SHADER));
if (gSavedSettings.getBOOL("RenderDepthOfFieldChroma"))
{
    gDeferredPostNoDoFProgram.addPermutation("HAS_DOF_CHROMA", "1");
}
```

AYAstorm 現在 (`llviewershadermgr.cpp:3022-3032`) は permutation なし。

#### 3.3.3 `gDeferredPostNoDoFNoiseProgram` (BD:2991-3006)

```cpp
gDeferredPostNoDoFNoiseProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoDoFF.glsl", GL_FRAGMENT_SHADER));
if (gSavedSettings.getBOOL("RenderDepthOfFieldChroma"))
{
    gDeferredPostNoDoFNoiseProgram.addPermutation("HAS_DOF_CHROMA", "1");
}
gDeferredPostNoDoFNoiseProgram.addPermutation("HAS_NOISE", "1");
```

AYAstorm 現在 (`llviewershadermgr.cpp:3035-3048`) は HAS_NOISE のみ、chroma permutation なし。

### 3.4 static 定義 + cvar refresh 配線 (BD `pipeline.cpp`)

| 改修箇所 | BD line | 内容 |
|---|---|---|
| static 定義 | 235 | `F32 LLPipeline::RenderChromaStrength;` |
| signal 配線 | 682 | `connectRefreshCachedSettingsSafe("RenderChromaStrength");` |
| refresh 反映 | 1313 | `RenderChromaStrength = gSavedSettings.getF32("RenderChromaStrength");` |

`RenderDepthOfFieldHighQuality` / `RenderDepthOfFieldChroma` は static 定義無し (shader compile-time にのみ参照、ランタイム static 不要)。

### 3.5 uniform push 3 箇所 (BD `pipeline.cpp`)

| 場所 | 関数 | BD line | 内容 |
|---|---|---|---|
| HQ DoF 経路 | `LLPipeline::renderDoF` (BD:8251) | 8428 | `gDeferredPostProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, RenderChromaStrength);` (CoF 計算後の post pass) |
| 最終 present pass | `LLPipeline::renderFinalize` (BD:8499) | 8649 | `gDeferredPostNoDoFNoiseProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, RenderChromaStrength);` (画面出力直前 noise + chroma) |
| 共通 bind helper | `LLPipeline::bindDeferredShader` (BD:8743) | 8969 | `shader.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, RenderChromaStrength);` (deferred shader bind 時の bulk uniform setter、chroma 非対応 shader は GL 側で silent drop) |

**8969 site の挙動**: `bindDeferredShader()` は deferred shader 全般の bind helper で、chroma uniform を持たない shader にも push しようとする。GL は `getUniformLocation == -1` を silent drop するので副作用ゼロ。chroma_str を読む shader (HQ DoF / NoDoFNoise / 標準 DoF with chroma permutation) のみが値を受ける。

### 3.6 llshadermgr enum + 文字列追加

`LLShaderMgr::DEFERRED_CHROMA_STRENGTH` enum と対応文字列 `"chroma_str"` を `eGLSLReservedUniforms` enum + `mReservedUniforms` 列に追加が必要。

BD では `llshadermgr.h` / `llshadermgr.cpp` に当該 enum + 文字列があるはず (静的 survey で確定済 / §7 で具体 line を再 fetch)。

### 3.7 lldrawpoolwater 経由 (P4 スコープ外、参考)

BD `lldrawpoolwater.cpp:257` で `shader->uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, gPipeline.RenderChromaStrength);` が呼ばれている (water shader にも chroma を載せる)。**P4 では取り込まない** (water pipeline は Firestorm/BD 間で差分が大きく、無理に踏むと未知の regression)。

---

## 4. AYAstorm 側の現在状態 (file:line)

`ayastorm-release` HEAD (= P3 spec ship 後) を起点に。

### 4.1 cvar — 全 3 件**存在せず**

- `RenderDepthOfFieldHighQuality`: `settings.xml` / `settings_per_account.xml` / `settings_firestorm.xml` どこにも無し (grep 結果ゼロ)
- `RenderDepthOfFieldChroma`: 同上
- `RenderChromaStrength`: 同上

### 4.2 shader file — HQ DoF**存在せず**

`indra/newview/app_settings/shaders/class1/deferred/` の DoF 関連 file:

```
dofCombineF.glsl         (LL 既存、AYAstorm に存在、Firestorm から不変)
postDeferredNoDoFF.glsl  (LL 既存、AYAstorm に存在、Firestorm から不変)
postDeferredF.glsl       (LL 既存、AYAstorm に存在、chroma block 無し)
postDeferredHQDoFF.glsl  (★ AYAstorm に**無し**、新規取り込み対象)
```

### 4.3 `LLPipeline::RenderChromaStrength` static — **存在せず**

AYAstorm `pipeline.cpp` には `RenderChromaStrength` の static 定義無し、`connectRefreshCachedSettingsSafe` 配線無し、`refreshCachedSettings` 反映無し。

### 4.4 shader register — 全 3 program で chroma permutation **存在せず**

AYAstorm `llviewershadermgr.cpp`:
- 2987-2997: `gDeferredPostProgram` (HQ 分岐なし、`postDeferredF.glsl` 直結、permutation なし)
- 3022-3032: `gDeferredPostNoDoFProgram` (permutation なし)
- 3035-3048: `gDeferredPostNoDoFNoiseProgram` (`HAS_NOISE` のみ、chroma permutation なし)

### 4.5 uniform push — 全 3 site で **存在せず**

AYAstorm `pipeline.cpp` で `DEFERRED_CHROMA_STRENGTH` の uniform push は **0 件** (grep 確認)。

### 4.6 llshadermgr — `DEFERRED_CHROMA_STRENGTH` enum **存在せず**

AYAstorm `indra/llrender/llshadermgr.{h,cpp}` に `DEFERRED_CHROMA_STRENGTH` も `"chroma_str"` も無し (要 §7 で再確認)。

### 4.7 既存 FIRE-16728 機構との関係

AYAstorm が保持している関連 cvar / static:
- `LLPipeline::sLastFocusPoint` (FS:Beq)
- `LLPipeline::sDoFEnabled` (FS:Beq) — `RenderDepthOfFieldInEditMode` + `RenderDepthOfField` + build mode + RlvActions::hasPostProcess() の AND
- `FSFocusPointRender` cvar
- `FSFocusPointFollowsPointer` cvar
- `gPipeline.mDOFFocusPointDirty` 等の dirty flag 機構

これらは **P4 で一切変更しない**。HQ shader 取り込みは「CoF buffer (gDeferredCoFProgram の出力 alpha) から sample する側の精度向上」であり、CoF を生成する側 (= focus point の計算) を変えない。

---

## 5. 取り込み時の改修・修正項目

### 5.1 BD バグ修正は無し

P3 trace で見つかった GODRAYS_FADE 取付先誤り (BD:2937 で `gDeferredSoftenProgram` に permutation 付けるが volumetric 側で参照、というバグ) のような誤配線は **P4 該当範囲内では発見されず**。3 shader register 全て自分自身に正しく permutation を付けており、uniform push も対応 shader への参照になっている。

### 5.2 shader file header の修正

BD の `postDeferredHQDoFF.glsl` は header コメント `@file postDeferredF.glsl` が stale。取り込み時に:
- `@file postDeferredHQDoFF.glsl` に修正
- LL viewerlgpl header はバイト保存 (LGPL 継承条件)
- header 直下に provenance コメント:
  ```glsl
  // AYAstorm: imported from BlackDragon Viewer (NiranV Dean),
  //           commit 995a1354d8 (2026-04-19), LGPL-2.1-only license inheritance.
  ```

### 5.3 既存 `postDeferredF.glsl` への chroma block 追加方針

AYAstorm の `class1/deferred/postDeferredF.glsl` (line 39-56 の `dofSample()` 関数) に `#if HAS_DOF_CHROMA` ガード付き block を BD と同等の形で追加。LL 既存ファイルへの追記なので header / provenance はそのまま、AYAstorm 改修箇所のみ簡易コメントで明示:

```glsl
void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
    vec4 s = texture(diffuseRect, tc);
    float sc = abs(s.a*2.0-1.0)*max_cof;
#if HAS_DOF_CHROMA  // <FS:AYA> r30 P4: chromatic aberration sampling
    vec3 col_offset = vec3(0.0015, 0.0000, 0.0005);
    float mult = sc * (chroma_str * 0.2);
    col_offset *= vec3(mult);
    s.r = texture(diffuseRect, tc + vec2(col_offset.x)).r;
    s.g = texture(diffuseRect, tc + vec2(col_offset.y)).g;
    s.b = texture(diffuseRect, tc + vec2(col_offset.z)).b;
    s.a = texture(diffuseRect, tc).a;
#endif
    if (sc > min_sc)
    {
        ...
    }
}
```

**注意**: 標準 (非 HQ) `postDeferredF.glsl` の `dofSample()` は depth 引数を持たない (HQ 版とシグネチャが違う)。`depth*0.50` のガードは標準版には適用できない。これは BD の `postDeferredF.glsl` chroma block も同様 (BD line 45 を参照、ガードなしで sample)。**「HQ では depth ガード付き chroma、標準では ガード無し chroma」** という意図的な非対称を保つ。

### 5.4 `dofCombineF.glsl` への変更要否

BD repo の `dofCombineF.glsl` を AYAstorm 側と diff した結果、**chroma 関連の追記は無い** (combine pass は CoF と diffuse を blend するだけで、color channel 個別操作は不要)。**P4 では `dofCombineF.glsl` は触らない**。

chapter §3 P4 取り込みリストには `dofCombineF.glsl` が含まれているが、これは「DoF chain に属するファイル」という意味であって BD 側に AYAstorm との差分があるという意味ではない。

### 5.5 既存 LL `postDeferredNoDoFF.glsl` への chroma block 追加

NoDoF 経路にも chroma block を追加。BD line 82 の `#if HAS_DOF_CHROMA == 0` 構造を踏襲 (NoDoF は CoF が無いので sample 方法が違う、§2.3 参照)。

### 5.6 Cinematic gate 戦略

shader 側の permutation 付与 (HAS_DOF_CHROMA / FRONT_BLUR / HQ DoF 分岐) は **起動時 1 回確定**。Cinematic mode が ON か OFF かで shader file 選択 + permutation 付与を分岐する:

| AYAVisualRealismEnabled | RenderDepthOfFieldHighQuality | RenderDepthOfFieldChroma | RenderDepthOfFieldFront | 結果 (gDeferredPostProgram) |
|---|---|---|---|---|
| 0 / 1 (Firestorm View / AYAstorm View) | - (ignore) | - (ignore) | - (ignore) | 標準 `postDeferredF.glsl`、permutation なし (現状と同じ) |
| 2 (Cinematic) | 0 (default) | 1 (default) | - (HQ 無効なので参照されない) | 標準 `postDeferredF.glsl` + `HAS_DOF_CHROMA` permutation |
| 2 (Cinematic) | 1 (user opt-in) | 1 (default) | 1 (default) | `postDeferredHQDoFF.glsl` + `HAS_DOF_CHROMA` + `FRONT_BLUR` permutation (**BD default 完全再現**) |
| 2 (Cinematic) | 1 (user opt-in) | 1 (default) | 0 (user opt-out) | `postDeferredHQDoFF.glsl` + `HAS_DOF_CHROMA` permutation のみ |
| 2 (Cinematic) | 0 (default) | 0 (user opt-out) | - | 標準 `postDeferredF.glsl`、permutation なし |
| 2 (Cinematic) | 1 (user opt-in) | 0 (user opt-out) | 1 (default) | `postDeferredHQDoFF.glsl` + `FRONT_BLUR` permutation のみ |

`llviewershadermgr.cpp` 内 register block で `if (gPipeline.sViewModeCinematic && gSavedSettings.getBOOL("RenderDepthOfFieldChroma"))` のような 2 段 gate を入れる。Cinematic 以外では BD の cvar 値を完全 ignore = 一切影響を出さない。

### 5.7 出荷 default

| cvar | Cinematic ON default | Cinematic OFF 時の効果 |
|---|---|---|
| `RenderDepthOfFieldHighQuality` | 0 (= 標準 DoF、user opt-in で HQ) | 影響なし (Cinematic gate で ignore) |
| `RenderDepthOfFieldChroma` | 1 (= 機能 compile in、strength で実効制御) | 影響なし (Cinematic gate で ignore) |
| `RenderChromaStrength` | **2026-05-18 更新**: BD は 0.0、AYAstorm は `feedback_match_bd_defaults_on_borrow.md` 適用で **BD と同値 0.0** を踏襲。ただし「Chroma permutation は compile in されているが strength 0 で実効ゼロ」の状態は **BD と同じ** であり、初見比較負けにはならない。strength を上げる動線は §7.5 の UI 判断 (debug settings or Preferences) に従う | 影響なし |
| `RenderDepthOfFieldFront` | **1 (= BD default、front blur ON)** ※ 2026-05-18 追加。HQ DoF user opt-in 時のみ effective。Cinematic+HQ DoF で初見 = BD と同じ前ボケ挙動 | 影響なし (Cinematic gate で ignore) |

### 5.8 UI 取り込み = AYAstorm 独自 Cinematic sub-tab (B 案、SSS パターン踏襲)

**2026-05-18 AYA 判断**: BD UI そのものは取り込まないが、`panel_preferences_sss.xml` (r20 で確立した SSS sub-tab) と同じ流儀で **AYAstorm 独自の Cinematic 専用 Preferences sub-tab を P4 で先行新設** する。

**判断根拠**: `feedback_match_bd_defaults_on_borrow.md` を UI 動線にも適用。BD には slider/checkbox が露出されているのに AYAstorm 側で debug settings 経由を要求すると、初見比較で動線負け。Cinematic mode は撮影 viewer ポジションなので、撮影者が Preferences の Cinematic タブを開いて即 dial in できる UX を最初から提供する。

**実装方針 (SSS パターン踏襲)**:

1. 新規 panel xml: `indra/newview/skins/default/xui/en/panel_preferences_cinematic.xml`
   - header コメント `<!-- <FS:AYA r30 P4> Cinematic preferences panel (Graphics > Cinematic sub-tab) -->` で wrap (SSS パターン同様)
   - 「This panel only affects Cinematic mode (View Mode を Cinematic に切替後の再起動で適用)」の説明文を冒頭に配置
   - controls 4 件:
     - `<check_box control_name="RenderDepthOfFieldHighQuality" label="High Quality Depth of Field (heavy, photo only)" />`
     - `<check_box control_name="RenderDepthOfFieldFront" label="Foreground Blur (front of focus point)" />` (HQ DoF 有効時のみ effective、tooltip で明示)
     - `<check_box control_name="RenderDepthOfFieldChroma" label="Chromatic Aberration (DoF only)" />`
     - `<slider control_name="RenderChromaStrength" label="Chroma Strength" min_val="0.0" max_val="3.0" decimal_digits="2" />`

2. `panel_preferences_graphics1.xml` 末尾 (`</tab_container>` 直前、SSS sub-tab の隣) に sub-tab 埋め込み追加:

   ```xml
   <!-- <FS:AYA r30 P4> Cinematic sub-tab inside Graphics -->
       <panel
        class="panel_preference_cinematic"
        filename="panel_preferences_cinematic.xml"
        top_pad="5"
        bottom="-1"
        left="1"
        right="-1"
        follows="all"
        label="Cinematic"
        name="Cinematic" />
   <!-- </FS:AYA> -->
   ```

3. `llpanelpreferencecinematic.{h,cpp}` の C++ skeleton (`LLPanelPreferenceCinematic` 派生クラス) — SSS panel の class 構造 (`LLPanelPreferenceSSS`) を参考に最小実装。値の Apply/Cancel は LLControlGroup 自動 binding を活用、独自フックは出さない。

4. ローカライゼーション file (ja/zh) は P4 出荷時に Volumetric/Motion Blur 等他 Cinematic cvar の sub-tab 拡張を見越して **panel_preferences_cinematic.xml 自体を多言語 lproj に複製** (SSS 同様、`indra/newview/skins/default/xui/{ja,zh}/panel_preferences_cinematic.xml`)。

**将来の拡張**: chapter §3 P5+ で予定の AYAstorm UI 整理 phase で Volumetric / Motion Blur / Velocity buffer / DoF combine 等の P2-P5 Cinematic cvar を同じ Cinematic sub-tab に追加していく。P4 で枠を切ったことで P5+ は controls 追加だけで済む。

**取り込まないもの**:
- BD `panel_preferences_graphics1.xml:5600-5622, 6944-6961` (BD 独自の chroma/HQ DoF controls) — Firestorm XUI 規約と整合しないため
- BD `panel_machinima.xml:681-692, 1775` (BD Photo Tools sidebar) — chapter §1.2 通り、BD UI は不取り込み

---

## 6. 実装ステップ案 (P4 本実装時の順序)

### 6.1 step 1: shader file 取り込み (1 ファイル + 2 ファイル改修)

- 新規取り込み: `indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl` (BD repo `995a1354d8` から copy → header `@file` 修正 → provenance コメント追加)
- 既存改修: `class1/deferred/postDeferredF.glsl` に `#if HAS_DOF_CHROMA` block 追加 (§5.3)
- 既存改修: `class1/deferred/postDeferredNoDoFF.glsl` に `#if HAS_DOF_CHROMA` block 追加 (§5.5)
- shader 構文検証: `glslangValidator` で 3 ファイル check (permutation 両方 = `-DHAS_DOF_CHROMA=1` / 無印)

### 6.2 step 2: cvar 追加 (`settings.xml` に 3 件)

P3 の Volumetric Lighting cvar 群直後に追加 (chapter §3 順序通り):

```xml
<key>RenderDepthOfFieldHighQuality</key>
<map>
  <key>Comment</key>
  <string>Enable old, slow, high quality Depth of Field shader. Cinematic mode only.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
<key>RenderDepthOfFieldChroma</key>
<map>
  <key>Comment</key>
  <string>Compile chromatic aberration permutation into Depth of Field shaders. Cinematic mode only.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>1</integer>
</map>
<key>RenderChromaStrength</key>
<map>
  <key>Comment</key>
  <string>Strength of chromatic aberration effect in Depth of Field. 0.0 = off, typical 0.5-2.0.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>F32</string>
  <key>Value</key>
  <real>0.0</real>
</map>
```

### 6.3 step 3: `llshadermgr.{h,cpp}` enum + 文字列追加

- `llshadermgr.h` の `eGLSLReservedUniforms` enum 内 P3 で追加した GODRAY_* 系 enum の直後に `DEFERRED_CHROMA_STRENGTH` 1 件追加
- `llshadermgr.cpp` の `mReservedUniforms.push_back()` 列の対応位置に `"chroma_str"` 追加 (enum 順 = 文字列順を厳守)
- §7 で BD 側の具体 line / 既存 enum 順を再 fetch して確認

### 6.4a step 4a: AYAstorm 独自 Cinematic Preferences sub-tab 新設 (§5.8 / §7.5 B 案)

- 新規: `indra/newview/skins/default/xui/en/panel_preferences_cinematic.xml` (SSS パターン踏襲、controls 4 件 = HQ DoF checkbox + Front Blur checkbox + Chroma checkbox + Chroma Strength slider)
- 新規: `indra/newview/llpanelpreferencecinematic.{h,cpp}` (SSS の `LLPanelPreferenceSSS` を参考に最小 class skeleton)
- 既存改修: `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` の SSS sub-tab (line 2145-2156) の隣 (= `</tab_container>` 直前) に Cinematic sub-tab を追加
- 多言語複製: `indra/newview/skins/default/xui/{ja,zh}/panel_preferences_cinematic.xml` (SSS パターン同様、label/tooltip のみ翻訳)
- `CMakeLists.txt` への新規 cpp/h 登録 (必要なら)

### 6.4 step 4: `pipeline.cpp` / `pipeline.h` 配線

- `pipeline.h`: `static F32 RenderChromaStrength;` 宣言追加 (P3 の Volumetric 系 static 直下)
- `pipeline.cpp`:
  - static 定義 1 行追加 (`F32 LLPipeline::RenderChromaStrength;`)
  - `connectRefreshCachedSettingsSafe("RenderChromaStrength");` 配線追加
  - `refreshCachedSettings()` 内 `RenderChromaStrength = gSavedSettings.getF32(...);` 反映追加
  - `renderDoF()` 内 `gDeferredPostProgram.bind()` 周辺で uniform push (BD:8428 相当)
  - `renderFinalize()` 内 `gDeferredPostNoDoFNoiseProgram.bind()` 周辺で uniform push (BD:8649 相当)
  - `bindDeferredShader()` 内 chroma uniform push (BD:8969 相当、bulk uniform setter section)

### 6.5 step 5: `llviewershadermgr.cpp` register 改修

3 program それぞれに Cinematic gate 付き permutation / HQ shader file 分岐を追加:

```cpp
// gDeferredPostProgram
gDeferredPostProgram.mShaderFiles.clear();
gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredNoTCV.glsl", GL_VERTEX_SHADER));
if (gPipeline.sViewModeCinematic && gSavedSettings.getBOOL("RenderDepthOfFieldHighQuality"))
{
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredHQDoFF.glsl", GL_FRAGMENT_SHADER));
}
else
{
    gDeferredPostProgram.mShaderFiles.push_back(make_pair("deferred/postDeferredF.glsl", GL_FRAGMENT_SHADER));
}
if (gPipeline.sViewModeCinematic && gSavedSettings.getBOOL("RenderDepthOfFieldChroma"))
{
    gDeferredPostProgram.addPermutation("HAS_DOF_CHROMA", "1");
}
```

`gDeferredPostNoDoFProgram` と `gDeferredPostNoDoFNoiseProgram` も同様の Cinematic gate + chroma permutation。

### 6.6 step 6: 受入検証

- shader compile 4 ケース (Cinematic ON × HQ ON/OFF × Chroma ON/OFF) で 起動時に shader 全部 link 成功
- `RenderChromaStrength = 2.0` で空 / 雲 / 樹木の高 CoF 領域に R/B 色ずれが視認できる
- `RenderChromaStrength = 0.0` で chroma block が compile in されていても見た目変化なし (sampling cost は変わるが視覚差分なし)
- HQ DoF ON で boku 領域の sample 精度向上 (round bokeh が滑らかになる)、framerate 低下を確認 (= BD と同じ「重いが綺麗」を再現)
- Cinematic OFF (AYAstorm View / Firestorm View) で全 cvar 無効、既存挙動と完全一致

### 6.7 step 7: 出荷物の clean up + commit

- 検証用 LL_INFOS hook 全削除 (`feedback_remove_verification_logs.md`)
- 各 step 単位で commit、最終 step で release note 反映

---

## 7. 未確定事項 (P4 着手時に再 fetch / 再確認が必要)

**ステータス**: 6 項目中 4 件 resolved (2026-05-18 spec commit 直後の追補 trace で fetch 済)、残 2 件は design decision で AYA さん判断待ち。

### 7.1 ✓ Resolved (2026-05-18) — `llshadermgr.{h,cpp}` の BD 具体 line + AYAstorm 挿入点

**BD 側 (`origin/master:indra/llrender/llshadermgr.h`)**:
- enum 配置: `DEFERRED_CHROMA_STRENGTH` は line 391。`DEFERRED_NUM_COLORS` (388) → `DEFERRED_GREYSCALE_STRENGTH` (389) → `DEFERRED_SEPIA_STRENGTH` (390) → `DEFERRED_CHROMA_STRENGTH` (391) のクラスタ末尾。
- 関連 enum (P3 で扱う): `GODRAY_RES` (370) / `GODRAY_MULTIPLIER` (371) / `FALLOFF_MULTIPLIER` (372)

**BD 側 (`origin/master:indra/llrender/llshadermgr.cpp`)**:
- push_back 列: line 1596 `mReservedUniforms.push_back("chroma_str");` (cluster: 1593 `"num_colors"` → 1594 `"greyscale_str"` → 1595 `"sepia_str"` → 1596 `"chroma_str"`)

**AYAstorm 側 (`indra/llrender/llshadermgr.h`)**:
- P3 で `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` が **既に line 393-398 に追加 commit 済** (`// <AYAstorm r30 P3 step 3>` / `// </AYAstorm r30 P3>` ガード付き)。本 P4 spec 起草時の前提が一部食い違っており、P3 は spec only ではなく enum 部分は実装済 (要 git log 確認)。
- P4 挿入点: line 398 `// </AYAstorm r30 P3>` の直後 / line 400 `END_RESERVED_UNIFORMS` の直前:

```cpp
        // </AYAstorm r30 P3>

        // <AYAstorm r30 P4> BD DoF chain — chromatic aberration uniform.
        // Imported from BlackDragon Viewer 995a1354d8 with no semantic change.
        DEFERRED_CHROMA_STRENGTH,           //  "chroma_str"
        // </AYAstorm r30 P4>

        END_RESERVED_UNIFORMS
```

**AYAstorm 側 (`indra/llrender/llshadermgr.cpp`)**:
- P3 push_back は line 1606-1608 (`"godray_res"` / `"godray_multiplier"` / `"falloff_multiplier"`) に既に追加済。
- P4 挿入点: line 1608 直後 (P3 ガード閉じの直後 / 既存 `attribsAndUniforms()` 関数末尾の `}` 直前):

```cpp
    mReservedUniforms.push_back("godray_res");
    mReservedUniforms.push_back("godray_multiplier");
    mReservedUniforms.push_back("falloff_multiplier");
    // </AYAstorm r30 P3>

    // <AYAstorm r30 P4> BD DoF chain — chromatic aberration uniform.
    mReservedUniforms.push_back("chroma_str");
    // </AYAstorm r30 P4>
```

enum 順 (line 401 の `END_RESERVED_UNIFORMS` 直前に追加) と push_back 順 (line 1609 で追加) が 1 対 1 対応であることを §6.3 step 3 で必ず double-check。

### 7.2 ✓ Resolved (2026-05-18) — `RenderDepthOfFieldFront` 取り込み判断 = **A**

**AYA 判断** (2026-05-18): 「BDに合わせて強い絵で勝負しないと初見で負けた感じになっちゃうね」 → **A 案 (BD default 1 のまま取り込み)** で確定。本 spec §1.1 / §5.6 / §5.7 を A 案で更新済。

**確定後の実装内容**:
- `settings.xml` に `RenderDepthOfFieldFront` 1 件追加 (Type=Boolean / Value=1 / Persist=1 / Comment は BD 原文 "Enable blurring the foreground with Depth of Field." を継承)
- `llviewershadermgr.cpp` の `gDeferredPostProgram` register block 内で Cinematic+HQ DoF 両方有効時に追加で `if (gSavedSettings.getBOOL("RenderDepthOfFieldFront")) gDeferredPostProgram.addPermutation("FRONT_BLUR", "1");`
- `postDeferredHQDoFF.glsl` 内の `#if FRONT_BLUR` ガード block (BD 由来コード) はバイト保存で取り込み、削除しない

**判断ルール記録**: BD から borrow する機能は default を BD と合わせる、控えめ default は初見比較で負けて印象を損なう → `feedback_match_bd_defaults_on_borrow.md`

### 7.3 ✓ Resolved (2026-05-18) — `dofCombineF.glsl` byte-diff

```
$ diff /tmp/bd_check/.../class1/deferred/dofCombineF.glsl \
       /home/.../class1/deferred/dofCombineF.glsl
(no output)
```

**BD と AYAstorm で byte-identical**。§5.4 の判断 (「P4 では `dofCombineF.glsl` を触らない」) は正しい。chapter §3 P4 取り込みリストに `dofCombineF.glsl` が含まれているのはあくまで「DoF chain に属するファイル」のラベリングであり、ファイル自体の変更は無し。

### 7.4 ✓ Resolved (2026-05-18) — `bindDeferredShader` の AYAstorm 側 line 番号

**BD**: `pipeline.cpp:8743` から `bindDeferredShader()`、chroma uniform push は line 8969 (関数末尾近く、bulk uniform setter section、`//BD - Post Process` コメント付き)。
**AYAstorm**: `pipeline.cpp:9906` から `bindDeferredShader(LLGLSLShader& shader, LLRenderTarget* light_target, LLRenderTarget* depth_target)`。BD より 1163 行後ろ (= Firestorm 独自関数群が前段に挿入されている)。

**chroma uniform push 挿入点**: AYAstorm `bindDeferredShader()` 関数末尾近く、`LLPipeline::unbindDeferredShader()` 定義 (要 line 確認) の直前。具体 line は §6.4 step 4 実装時に当該関数末尾の `}` を Read で特定 → 直前の uniform push 群 (BD で言うと `DEFERRED_LIGHT_STRENGTH` push の隣) に挿入。

P3 で同様の `bindDeferredShader` 系操作をした実装が commit 済の可能性があるので、P4 着手時に git log で P3 commit を見て参考にする。

### 7.5 ✓ Resolved (2026-05-18) — cvar UI 取り込み判断 = **B (SSS パターン踏襲)**

**AYA 判断** (2026-05-18): 「B で Cinematic 専用の Graphic preference タブを用意したほうが良いかもしれませんね　SSS みたいに」 → **B 案 + r20 SSS sub-tab 構造を踏襲** で確定。本 spec §5.8 を B 案で更新済。

**踏襲する r20 SSS 構造の確認結果**:
- `panel_preferences_sss.xml` 独立 panel file (label="SSS", name="sss")
- `panel_preferences_graphics1.xml:2145-2156` に sub-tab として埋め込み (`<!-- <FS:AYA r20 Phase A> SSS sub-tab inside Graphics -->` / `<!-- </FS:AYA> -->` ガード)
- `class="panel_preference_sss"` で C++ class binding

Cinematic 用は同形式で `panel_preferences_cinematic.xml` を新規追加 + `panel_preferences_graphics1.xml` の SSS sub-tab の隣に sub-tab を追加。詳細実装は §5.8。

**判断ルール記録**: BD borrow 機能の UI 動線も BD と対等に。「BD UI は不取り込み」原則は守りつつ、AYAstorm 流儀で対等な動線を提供する (`feedback_match_bd_defaults_on_borrow.md` の UI 適用)。

### 7.6 ✓ Resolved (2026-05-18) — BD repo の再 fetch

`git fetch origin` 実行結果: BD `origin/master` の HEAD は依然 `995a1354d89feef858c0893f6d5cc2a1466ecd4b` (Version to 5.6.2, 2026-04-19)。本 spec の参照 commit から進行無し。**P4 着手時に BD HEAD が更新されていれば再度 chroma / DoF 関連変更を grep 確認**。

### 7.7 ✓ Resolved (2026-05-18) — P3 実装ステータス確認

`git log --grep='r30 P3'` で確認、**P3 は step 1 から release notes まで完全 ship 済**:

| commit | 内容 |
|---|---|
| `31c6310c81` | r30 P3 spec: volumetric light trace + plumbing map |
| `bf269b8671` | step 1: import volumetric light shaders from Black Dragon Viewer |
| `4e6a97bf86` | step 2: add 5 volumetric light cvars to settings.xml |
| `64aa990f7c` | step 3: register gVolumetricLightProgram (BD lineage 995a1354d8) |
| `0df903736d` | step 4: pipeline plumbing — renderVolumetric() + renderFinalize hook |
| `d0a695c2ac` | step 5: alpha pool depth-write extension for godrays (Cinematic gated) |
| `4f0215d3a8` | step 6: hotfix + default Multiplier retune (50.0) + spec §8 受入観測 |
| `775573ada9` | r30 P3 release notes (ja/en/zh) |

P4 spec の前提 (= P3 完了済) は正しい。本 spec 起草時に持ち込まれた conversation summary の「P3 not pushed」記述は古く、実態は ship 完了。**P4 は P3 tip から積めば良い** (本 spec の branch `feature/ayastorm-r30-p4-bd-dof-chain-spec` は既にそうなっている、`git log` 上 `2670934d28` (P2 release notes) より前段 = P3 ship 後の HEAD)。

---

## 8. 参考: BD UI 取り込み判断の根拠

BD `panel_preferences_graphics1.xml` の chroma UI (line 5600-5622, 6944-6961) は **slider + checkbox の組み合わせ** で:
- Chroma checkbox: `RenderDepthOfFieldChroma` (Boolean、compile-time permutation toggle)
- HQ DoF checkbox: `RenderDepthOfFieldHighQuality` (Boolean、shader file 分岐)
- Chroma strength slider: `RenderChromaStrength` (F32 0.0-3.0 程度)

`panel_machinima.xml` (BD Photo Tools floater) でも同等の UI が露出している。

**AYAstorm が取り込まない理由**:
- Firestorm Preferences の Graphics tab は LL 流儀で組まれており、BD 流儀 (`panel_machinima.xml` 等) の追加 UI 規約と整合しない
- chapter §1.2 で確定済 (「BD UI は取り込まない / Firestorm 流儀にも翻訳しない」)
- 撮影特化 user 向けの設定は debug settings 経由でも実用上問題なし (`feedback_release_notes_link_only.md` 流儀)
- 将来 Cinematic 専用 Preferences セクションを作る場合 (chapter §3 P5 以降の AYAstorm UI 整理) で再判断

---

(本 spec は P4 着手時に作業者が file:line 単位で「BD のどこを見て、AYAstorm のどこに何を入れるか」を spec 1 本で把握できる状態を目指している。実装着手時に §7 の各項目を再 fetch / 再確認した上で、§6 ステップ順に進める。)
