# r41 UBO 全体設計 Chapter 05: 既存 inventory link + bare uniform 集約対応表

**起案日**: 2026-06-03
**位置付け**: inventory (= `ayastorm-r41-ubo-current-state-inventory.md`) の **既存 85 UBO blueprint** と **OpenGL path bare uniform 群** を、chapter 02 命名規則 + chapter 03 cadence 軸で再配置する **意味論判断 doc**。chapter 04 Codegen の **入力 UBO 集合を決定** し、chapter 06 redirect 層の **集約 mapping 表** を提供する。
**pre-requisite**:
- `ayastorm-r41-ubo-current-state-inventory.md` (現状 85 UBO + bare uniform dispatcher 棚卸し)
- `01-overview.md` §3 (用語) / §5 (確定済事項 9-11)
- `02-naming-convention.md` (命名 + rename 表)
- `03-cadence-classification.md` (cadence 5 分類、= 旧 6 分類から per-material を per-draw + dirty flag 統合 = 本 chapter §6 MC1 確定、= 旧 G1)
- `04-codegen-ubo.md` (Codegen 入力契約)

---

## §1 本 chapter の scope

### §1.1 scope (= 本 chapter で確定するもの)

1. 既存 **85 UBO blueprint の cadence 別 mapping** (chapter 02 rename + chapter 03 cadence 適用後の最終配置)
2. **set=2 (26 個) vs set=3 (`_Legacy` 54 個) の役割重複統廃合方針** (= inventory 持越 item B、Q22-NUM 解消 A' 反映で 25→26)
3. **`MaterialUBO` vs `MaterialUBO_Legacy` の処遇** (= inventory §7 課題 #5、持越 C)
4. **per-material cadence の最終判定** (= per-draw 統合 / 独立保持、持越 MC = ex 持越 G、ID rename = chapter 04 §10 (G) perfect hash generator との別概念衝突回避、2026-06-03 査読 §5.6)
5. **bare uniform → UBO 集約対応表の書式 / owner / フロー** (= 持越 H)
6. chapter 09 (phase roadmap) への **migration 入力**: どの UBO から手を付け、どの bare uniform を先に集約するか

### §1.2 非 scope (= 他 chapter / 他 phase 譲り)

- 全 85 UBO × N bare uniform の **完全網羅 mapping**: live 表として migration 進行で順次確定 (= chapter 09 Phase ごと)
- GLSL に **UBO ブロック宣言を追加する diff** 本体: chapter 09 Phase ごとに発生する作業
- Codegen build-time 詳細 → chapter 04 / chapter 08
- runtime redirect 実装詳細 → chapter 06

---

## §2 入力契約

| 入力 source | 本 chapter での用途 |
|---|---|
| inventory §1 (OpenGL 実働 UBO 4 種) | §3 で「実働 4 × blueprint 85 = 統合後 N」の母集合 |
| inventory §2 (bare uniform dispatcher) | §7 集約表の入力源 |
| inventory §3 (85 GLSL blueprint) | §3 cadence 別 mapping の対象 |
| inventory §4.3 (host redirect 16 method) | §7 集約表が指す setter family |
| chapter 02 §3 rename 表 | §3 で新名適用 |
| chapter 03 §2 cadence 5 分類 (= 旧 6 分類から per-material → per-draw + dirty flag 統合済、本 chapter §6 MC1、= 旧 G1) | §3 各 UBO の cadence 判定 |
| chapter 04 §7.3 集約フロー | §7.4 で本 chapter 出力を Codegen に渡す |

---

## §3 既存 85 UBO blueprint の cadence 別 mapping

### §3.1 set=0 帯 (3 個): 全て per-frame、命名一致、Codegen 入力にそのまま投入

| 旧名 | 新名 | cadence | 処遇 |
|---|---|---|---|
| `FrameViewProj` | `FrameViewProj` (変更なし) | per-frame | Codegen 入力、命名規則一致 |
| `FrameLights` | `FrameLights` (変更なし) | per-frame | 同上 |
| `FrameAtmosphere_Lighting` | `FrameAtmosphere` (suffix 削除) | per-frame | chapter 02 §3.1 rename 適用 |

**所見**: 既存命名と cadence prefix が完全一致。本 chapter での意味論判断は不要、機械的 rename のみ。

### §3.2 set=1 帯 (2 個): per-material、§5 で処遇判断

| 旧名 | 新名 (暫定) | cadence | 処遇 |
|---|---|---|---|
| `MaterialUBO` | `MaterialUBO` (暫定温存) | per-material | §5 で program 単位の attach 排他確認後、最終名確定 |
| `MaterialUBO_Legacy` | `MaterialLegacyBlinn` (案) | per-material | 同上、§5 で member 比較後、統合 / 別名分離 / 廃止を決定 |

### §3.3 set=2 帯 (26 個): per-program 24 + per-draw 2、chapter 02 §3.3 機械的 rename

cadence 分類:

| binding | 新名 | cadence |
|---|---|---|
| 0 | `Draw_LightParams` | per-draw |
| 1 | `Draw_MultiLight` | per-draw |
| 2-25 | `Program_<X>` (= 24 個) | per-program |

**所見**:
- per-draw 2 個 (`LightParams` / `MultiLight`) は inventory §3.3 確認済、`Draw_*` rename
- per-program 24 個は chapter 02 §3.3 rename 表に従い `PerProgramUBO_<X>` → `Program_<X>` に変換
- Codegen 入力に rename 適用後そのまま投入、cadence 別 update site は chapter 06

**`inventory §3.3.1` の同一 binding 複数 UBO 名疑い** (= `PerDrawUBO_ClipPlane` / `SkinnedVelocity` / `AvatarVelocity` / `AvatarSkin` / `ObjectSkin` 等):

→ **§10 未確定 (E') として chapter 09 Phase 0 計測 task** に持越。binding 重複 (A 案) / dead code (B 案) / Agent 抽出誤り (C 案) のいずれかは grep + program 単位 attach 確認で判定。

### §3.4 set=3 帯 (54 個): 全て per-program、§4 で統廃合方針判断

cadence 上は set=2 帯 `Program_*` と同一 (= per-program)。命名規約のみ違い (`<Name>UBO_Legacy`)。

→ §4 で「set=2 と統合 vs rename だけ vs 個別判定」を確定。

---

## §4 set=2 vs set=3 統廃合方針 (= 持越 B)

### §4.1 論点

inventory §6.3 で指摘: 「set=2 (新規 26 個) と set=3 (`_Legacy` 54 個) は **役割重複**、両者とも per-program 寿命」。**どちらかに統合すべき** が、統合の **粒度** が未定義。

### §4.2 選択肢

| # | 案 | 影響 |
|---|---|---|
| E1 | **全件統合** = set=2 + set=3 を全部 program 単位 1 UBO に集約 (= 80 個 → ~26 個程度に集約) | shader 改変規模大、原則 1 (upstream 取込) 影響大、descriptor set rebind 数減 |
| E2 | **統合候補のみ統合** = chapter 02 §3.4 判定基準 3 つで個別判定 | 個別判定の負担、移行進行中の継続判断 |
| E3 | **rename だけ** = 命名規約違反 (`_Legacy`) を剥がし、UBO は独立保持 (= 80 個維持) | shader 改変最小、descriptor set rebind 数維持 (= 多い) |

### §4.3 採用案 = **E3 (rename だけ)** + cadence prefix 統一 (= 2026-06-03 AYA 確認)

#### 採用根拠

1. **起源 program 単位の grouping を温存**: set=2 / set=3 はいずれも program 単位の grouping、program 内で参照される member の自然な境界
2. **dirty 判定の前提保持**: 統合すると「変更が無い program でも UBO update が走る」リスク (= cadence 粒度に縛られる)。program 単位独立なら dirty 判定が program 単位で閉じる
3. **shader 改変規模最小** (= 原則 1 影響最小)
4. **descriptor set rebind 数増加は実 cost 測ってから判断**: 統合判断は migration 進捗で実 update cost を測ってから (= §4.4)

#### E3 採用に伴う作業

- 54 個全件: `<Name>UBO_Legacy` → `Program_<Name>` (= chapter 02 §3.4 機械的 rename、表は §4.4 にも一部記載)
- binding 番号は temporary に既存値温存、最終 descriptor set 設計時 (chapter 07) で再割当検討
- chapter 04 Codegen 入力に **80 個全件** (= set=2 帯 26 + set=3 帯 54) を投入、`<BlockName>Layout` 80 構造体生成

### §4.4 統合判定の再評価持越 (= chapter 09 後半 / chapter 10)

E3 採用 = 統合は本 phase では行わないが、**実 update cost 測定後の再評価判断は chapter 09 後半 / chapter 10 で扱う**。判定基準 (chapter 02 §3.4 旧 3 基準) は migration 進捗で実 upload cost が測定可能になった時点で適用:

1. host C++ redirect 対象の bare uniform 群と member 重複がある → 統合候補
2. shader file が独立 program 専属 → そのまま `Program_<Name>` で独立保持
3. shader file が複数 program で共有 → `Program_<Name>` 共有 or 廃止再構成

これらは chapter 09 後半の「per-program UBO の物理 upload cost 計測」task の output として再評価する持越事項。

---

## §5 `MaterialUBO` vs `MaterialUBO_Legacy` 処遇 (= 持越 C)

### §5.1 現状把握 (inventory §3.2)

- 両 UBO 共に set=1 binding=0
- GL spec 上「program ごとに binding namespace 独立」のため、program 単位で片方のみ attach されているはず
- `_Legacy` suffix = 別 program 用の派生形と命名規則上は読める
- inventory §7 課題 #5: **program 単位 attach 排他確認は未解消**

### §5.2 選択肢

| # | 案 | 前提 |
|---|---|---|
| F1 | **member 同一なら統合** = 1 UBO に集約、`Program_*` 化 | member 比較 = 同一 |
| F2 | **別 UBO 維持 + 別名分離** = `MaterialPBR` / `MaterialLegacyBlinn` 等の役割名で命名 | member 比較 = 別物 |
| F3 | **片方廃止** = 一方が使われていない dead UBO | program 単位 attach grep で dead 検出 |

### §5.3 Claude 推奨案 = **member 比較後の自動判定**

#### 判定フロー

1. inventory §3.2 の宣言例 (`class1/objects/simpleNoColorV.glsl:46` 他 / `class3/deferred/materialF.glsl:38`) の **member を比較**
2. 同一 → F1 採用、新名 = `MaterialPBR` (= 主用途名)、`_Legacy` 側は migration 完了で削除
3. 別物 → F2 採用、両者を `MaterialPBR` / `MaterialLegacyBlinn` に分離
4. 片方が dead → F3 採用、dead 側削除

#### 確認 task → chapter 09 Phase 0 (= 持越 F)

member 比較 / program 単位 attach grep は chapter 09 Phase 0 計測 task として持越。本 chapter §5 は **判定フローの確定** に留め、最終名は chapter 09 で確定。

### §5.4 Phase 0 Step 1 (F) 確定 + ST-3 batch Q26-MUL 確定 (= 2026-06-03)

#### §5.4.1 Phase 0 Step 1 (F) 確定 = F2 採用

2026-06-03 Phase 0 Step 1 Pre-hook Static Analysis (= 06a-prep §4.6) で member 完全別物確認:
- `MaterialUBO` = 52 件宣言 (= 全 `class1/2`-deferred V/F file 等)、PBR transform 系 member (`texture_matrix0` / `texture_base_color_transform[2]` / `texture_emissive_transform[2]` / `color` / `emissiveColor` / `_pad_emissive` = 160 B)
- `MaterialUBO_Legacy` = 1 件宣言のみ (= `class3/deferred/materialF.glsl:38` 単独)、Blinn Legacy 系 member (`morphFactor` / `specular_color` / `camPosLocal` / `emissive_brightness` / `is_mirror` / `env_intensity` / `aya_sss_skin_flag` / `_pad_material_legacy_0` = 64 B)

→ **共通 member 0 件、type / 名前 / 順序 全て差異** = §5.2 表で **F1 反証** (member 同一 ≠ 成立)、**F3 反証** (両者とも実 attach 確定、= `gDeferredMaterialProgram` の mShaderLevel=class3 path で MaterialUBO_Legacy 使用)、**F2 第一候補 narrowing 確定**。

#### §5.4.2 ST-3 batch Q26-MUL 確定 = MUL-A1 + MUL-B1

2026-06-03 ST-3 batch (= chapter 10 §1.5 (Q26-MUL) AYA 判断「全 default 採用」応答):
- **MUL-A1**: rename `MaterialUBO_Legacy` → **`MaterialUBO_Class3_Legacy`** (= specific 名、chapter 02 §3.2 命名規則 `*_{class}_{用途}` と整合)
- **MUL-B1**: 同 program 内 set=1 binding=0 二重宣言解消 = **class3 専用 V shader `class3/deferred/materialV.glsl` を新規追加** (= MaterialUBO 不宣言、F=`class3/deferred/materialF.glsl` (= MaterialUBO_Class3_Legacy 単独宣言) と組合せ)、mShaderLevel=class3 path で本 V を選択

#### §5.4.3 反映先 (= Phase 1.A 入口実装 task)

| 反映先 | 内容 | 実施 phase |
|---|---|---|
| chapter 02 §3.2 命名規則表 | `MaterialUBO_Class3_Legacy` 確定形書換、`MaterialUBO` 暫定 → 確定マーク | ✅ 2026-06-03 反映済 |
| 本 chapter §3.2 set=1 帯 mapping | `MaterialUBO_Legacy` → `MaterialUBO_Class3_Legacy` rename 反映 (= 既存 mapping table が `MaterialUBO_Legacy` を直接参照しているか別途確認) | Phase 1.A 入口実装と同時 |
| `class3/deferred/materialV.glsl` 新規 file 起案 | mShaderLevel=class3 専用 V shader (= MaterialUBO 不宣言)、attribute / varying / `main()` は `class1/deferred/materialV.glsl` から派生、ただし UBO 部分のみ削除 | Phase 1.A 入口実装 (= `indra/` 改変、本 design-phase scope 外) |
| `class3/deferred/materialF.glsl:38` UBO 名 rename | `MaterialUBO_Legacy` → `MaterialUBO_Class3_Legacy` (= 1 file 1 行書換) | 同上 |
| inventory §3.2 「2 UBO 名共存」記述補正 | 「共存ではなく Legacy 側 1 件、ただし mShaderLevel=class3 で同 program V+F 共存 risk → MUL-B1 で解消」へ書換 | Phase 1.A 入口 doc update |
| chapter 10 §1.5 (Q26-MUL) verdict マーク | ✅ A1+B1 確定 | ✅ 2026-06-03 反映済 |

---

## §6 per-material cadence の最終判定 (= 持越 MC、**確定 2026-06-03**)

**ID 注**: 本節持越 ID は当初 (G) だったが、chapter 04 §10 (G) (= perfect hash generator) と別概念衝突するため (MC) (material cadence prefix) に rename (2026-06-03 査読 §5.6)。MC1/MC2 = G1/G2 の対応。

### §6.1 論点 (chapter 03 §2 の注を再開)

chapter 03 §2 で「per-material = per-draw cadence の特化 (material が同じ draw 群を batch upload で間引く)。実体は **per-draw + dirty flag** で実装することで cadence の実体は per-draw に縮約しうる」と書いた。本 chapter で確定する。

### §6.2 選択肢

| # | 案 (= MC1/MC2 = 旧 G1/G2) | 影響 |
|---|---|---|
| MC1 (= G1) | **per-draw + dirty flag に統合** = per-material cadence は独立軸として持たない、Material* UBO は per-draw cadence で扱い dirty 判定で同 material 連続時 skip | cadence 軸 5 分類に縮約、命名 prefix は `Material*` 温存 |
| MC2 (= G2) | **独立 cadence として保持** = per-material 独立 update site / descriptor set | cadence 軸 6 分類維持、material 切替頻度に応じた最適化余地 |

### §6.3 採用案 = **MC1 (per-draw + dirty flag、= 旧 G1)** (= 2026-06-03 AYA 確認)

#### 採用根拠 (= 「現状動作にいちばん近い」軸)

1. **現状 OpenGL path に per-material 独立 cadence が存在しない**: `MaterialUBO` / `MaterialUBO_Legacy` は GLSL blueprint のみ、host 側 `glUniformBlockBinding` 経路ゼロ (inventory §1.2)。material parameters は **bare uniform setter で per-draw 投入** されている (inventory §2)
2. **既存 `mValue` cache (inventory §4.3) が dirty flag と意味論的に同じ**: 同値時 GL call 省略 = dirty 判定の言い換え。G1 dirty flag はこの cache を Vulkan UBO upload 側に **乗せ替えるだけ**
3. **MC2 (= 旧 G2) 採用には material 切替検知 hook を C++ 側に新規建設が必要** = call site 改変必須、原則 1 抵触
4. **cadence 軸が 5 分類に縮約** = chapter 06 redirect 層 / chapter 09 phase 設計が簡素化
5. **命名 prefix `Material*` は温存** = upstream 取込互換 (chapter 02 §2.1)、cadence と命名は独立軸 (chapter 03 §2.2)

### §6.4 MC1 (= 旧 G1) 採用に伴う整合 update

| update 先 | 内容 |
|---|---|
| chapter 03 §2 cadence 表 | per-material 行を削除 → **5 分類** に修正、注を MC1 確定 (= 旧 G1) に書き換え |
| chapter 03 §4.3 per-draw cadence 表 | dirty 判定行に「material 切替は本 cadence の dirty flag で吸収 (= 既存 `mValue` cache 機構を継承)」を追記 |
| chapter 02 §2.1 命名規則表 | per-material 行は **prefix `Material` 用途として温存**、cadence 列を「per-draw (material dirty flag、chapter 05 §6 確定)」に変更 |
| chapter 01 §5 確定済事項表 | #12 として追加 |
| chapter 06 redirect 層 (起案時) | Material* UBO を per-draw cadence の dirty 判定で実装、独立 update site / descriptor set は設けない |

---

## §7 bare uniform → UBO 集約対応表 (= 持越 H、本 chapter の中核)

### §7.1 bare uniform 集合の取得方法

inventory §2 で確認した dispatcher / setter:

| source | 集合取得方法 |
|---|---|
| `LLEnvironment::updateShaderUniforms(shader)` | call site 内 `shader.uniform*fv("name", ...)` を grep |
| `LLViewerShaderMgr::updateShaderUniforms()` | 同上 |
| `LLGLSLShader::uniform*fv()` 16 method の **全 caller** | code base 全件 grep |
| shader link 時 `mUniform[]` index → uniform 名 | LL_INFOS hook 一時挿入で全件 dump |

**実行計画**:
- chapter 06 起案時 (= redirect 層実装直前) に grep + LL_INFOS hook で全件 enumerate
- 本 chapter §7 では **集約 framework と表書式** までを確定、表本体は live 表として migration 進行で埋める

### §7.2 集約粒度 (= cadence prefix で決まる)

cadence 判定基準 = chapter 03 §3 cadence source rule (= 既存 C++ 呼出 path のスケジュール):

| call site | cadence | 集約先 prefix |
|---|---|---|
| `LLEnvironment::updateShaderUniforms(shader)` 内 | shader bind 毎 = per-program | `Program_<X>` |
| `LLPipeline::renderGeom()` 内 per-draw setter | draw call 毎 = per-draw | `Draw_<X>` |
| frame loop 開始時 setter | per-frame | `Frame<X>` |
| `Asset::updateNodeData()` 系 | per-asset | `Asset_<X>` |
| `Skin::updateTransforms()` | per-skin | `Skin_<X>` |
| `LLReflectionMapManager::setUniforms()` | singleton (reflection update 時) | `Global_<X>` |

### §7.3 集約表の書式 / owner

#### §7.3.1 書式

```
| # | bare uniform name | call site (file:line) | cadence | 集約先 UBO | member 名 | 状態 |
|---|---|---|---|---|---|---|
| 1 | color | llenvironment.cpp:NNNN | per-program | Program_GammaCorrect | color | 候補 / 確定 / 移行済 |
| 2 | alpha_threshold | ... | per-program | Program_AlphaParams | threshold | ... |
| ... | ... | ... | ... | ... | ... | ... |
```

#### §7.3.2 owner

- 表本体は **本 chapter 内に inline** 配置 (= live 表)
- migration 進行で **状態列**を 候補 → 確定 → 移行済 と遷移
- 表 size が肥大 (~50 行超) したら別 file `05a-bare-uniform-mapping.md` に切出し
- 切出し判断は chapter 09 Phase 進行中に発生したら本 chapter §11 update 規律に従い実施

#### §7.3.3 表本体 (= 起案時点では空、migration 進行で埋まる)

`(2026-06-03 起案時点: 空。chapter 06 起案時の grep / LL_INFOS hook 結果で埋める)`

#### §7.3.4 Phase 0 計測由来 cadence 補正 (= 2026-06-03 ST-4 batch、表本体起案時の cadence 列入力規律)

2026-06-03 Phase 0 Step 4 AYA 実機計測 + Step 4 解析 (= 06a-prep §5.5.7) で **matrix 系 group 4 件** が per-program 推定 → **per-draw 確定** に補正:

| uniform 名 | 推定 cadence | 観察 cadence (= Phase 0 計測) | 観察 rate | 観察 shader/frame |
|---|---|---|---|---|
| `modelview_matrix` | per-program | **per-draw** | 437/688 cpf | 93/97 shader (s2/s3) |
| `inv_modelview` | per-program | **per-draw** | 同 | 同 |
| `modelview_projection_matrix` | per-program | **per-draw** | 375/619 cpf | 37/46 shader (s2/s3) |
| `normal_matrix` (= group 4 件目確定 = R-MAT4) | per-program | **per-draw** (= §5.5.7 group rate > 100 → per-draw 確定 group-level 適用、shader 内 normal transform per-draw 性質と物理整合) | 個別観察値なし (= §5.5.7 group-level 適用、再計測不要) | 個別観察値なし (= 同上) |

**反映規律**: §7.3 表本体起案時 (= chapter 06b 起案中の bare uniform 集約段階) に matrix 系の cadence 列を **per-draw として記入** (= 推定欄でなく観察欄を採用)。group 4 件目 (= R-MAT4 = `normal_matrix`) は §5.5.7 group-level 観察値適用で個別 cpf 再計測不要。chapter 10 §2.7 (R-MAT1)-(R-MAT4) と連動。

**2026-06-03 ST-6 前段 (b) literal 転記完了 + R-MAT4 訂正**: R-MAT1/2/3 (= named 3 件) の literal 観察値は 7735505d03 commit (= ST-4 batch) で本表に転記済。R-MAT4 候補名は前 commit 時点で `modelview_projection_inverse` + `normal_matrix` の 2 件候補だったが、ST-6 前段 (b) grep verify で `modelview_projection_inverse` は indra/ 内 **0 件 = 存在 uniform 名でない** ことが判明 (canonical 「matrix state」block = llshadermgr.cpp:1505-1518 にも該当名なし)、`normal_matrix` (= indra/ 172 occurrences) のみ R-MAT4 group 4 件目として確定。group 5 件目 candidate (= 前 commit `normal_matrix` 5 件目位置) は撤回 (= 案 A 採用 / `modelview_delta` 等 SSR-only 候補は per-draw cadence 確信度低く除外)。matrix 系 per-draw 確定総件数 = **4 件** (= R-MAT1 modelview_matrix / R-MAT2 inv_modelview / R-MAT3 modelview_projection_matrix / R-MAT4 normal_matrix)。「再計測しない」(= 前 handoff §3.4 規律 3) 厳守で個別 cpf 再観測なし、§5.5.7 group-level 観察値 (= group rate > 100、~20 件 per-draw 補正) を group 4 件全件に適用。

**3 件 (R-AYA1)(R-AYA2)(R-AYA3) 連動**: 06a-prep §5.5.5 dead candidate 中 `aya_*` 3 件 (= `aya_alpha_plate` / `aya_alpha_plate_enabled` / `aya_sss_skin_flag`) は **本 §7.3 表起案直前に grep で hash 経由配線 / dead path / shader 種別を確認** (= chapter 10 §2.7 (R-AYA1)-(R-AYA3) と連動)。`aya_sss_skin_flag` は MaterialUBO_Class3_Legacy member とも同名 (= §5.4.1) で二重配線疑い、Q26-MUL 構造改修と整合確認必要。

### §7.4 chapter 04 Codegen との繋ぎ (= chapter 04 §7.3 集約フローの再掲)

集約フロー (= chapter 04 §7.3 の本 chapter 側責務):

1. **本 chapter 表で集約決定**: 「`color` → `Program_GammaCorrect.color`」
2. **GLSL 改変**: 該当 program GLSL に `Program_GammaCorrect` UBO ブロックを追加 / member 追記
3. **bare uniform 削除**: 集約完了したら GLSL から `uniform vec4 color;` を削除
4. **Codegen 自動処理**: chapter 04 pipeline が新 UBO ブロックを parse → layout + perfect hash entry 生成
5. **chapter 06 redirect 層**: setter `uniform4fv("color", ...)` を受けて perfect hash で offset 解決 → UBO memcpy

= **本 chapter 表は集約フローの 1 → 2 → 3 を駆動する source of truth**。Codegen は表を直接読まず、表に従って改変された GLSL を入力に取る。

### §7.5 集約判定基準

bare uniform を集約先 UBO に割り当てる際の判定優先順位:

1. **同じ call site から投入される他 bare uniform と同 UBO に集約** (= dispatcher 単位の grouping、API call 削減)
2. **同じ cadence の既存 UBO ブロックに member 追加できるなら追加** (= 新規 UBO 追加最小化)
3. **既存 UBO ブロックで適合無し / 容量不適なら新規 UBO 追加** (= cadence prefix 命名規則に従う)

判定 conflict 時は本 chapter §7.3 表に複数候補を併記、chapter 09 Phase で AYA 判断。

---

## §8 GLSL 改変規律

### §8.1 chapter 05 が GLSL を改変する範囲

chapter 04 (Codegen) は GLSL 不改変 (= 判断 A)。一方 **chapter 05 は集約に伴い GLSL を改変する**。改変範囲:

| 改変内容 | 規律 |
|---|---|
| UBO ブロック宣言追加 | §7.3 表で確定後、chapter 09 Phase ごとに 1 UBO ずつ追加 |
| UBO ブロック内 member 追記 | 同上、既存 UBO ブロックに member 追加 |
| bare uniform 宣言削除 | 集約完了確認後 (= redirect 層動作確認後)、Phase 単位で削除 |
| UBO ブロック名 rename | chapter 02 §3 機械的 rename、1 phase で全件一括可 |

### §8.2 GLSL 改変前後の検証

各 Phase での GLSL 改変 / bare uniform 削除前後で:
- shader compile 通過 (OpenGL path / Vulkan path 両方)
- 実描画結果が同一 (= regression テスト、chapter 09 で詳細)
- Codegen 生成 offset と SPIR-V reflection offset 一致 (= chapter 04 §3.2 build-time check)

### §8.3 改変できない範囲

- `#ifdef LL_VULKAN_GLSL` gate の **削除** = OpenGL path と Vulkan path 共存期間中は維持
- 既存 OpenGL 実働 UBO 4 種 (`UB_REFLECTION_PROBES` 等) の宣言改変 = upstream 取込互換 (原則 1)

---

## §9 build-time check (= chapter 04 §3.2 / §4.4 の延長)

本 chapter 集約フロー成立のため、Codegen pipeline に追加すべき check:

1. **集約候補 bare uniform が対応 UBO に member 実在**: 集約表で「`color` → `Program_GammaCorrect.color`」と書かれた場合、`Program_GammaCorrect` ブロックに `color` member が存在することを check (= 集約漏れ防止)
2. **集約済 bare uniform が GLSL から削除**: 集約表で「移行済」になった bare uniform が GLSL に **残存していない** ことを check (= 二重定義 / 古い path 残存防止)
3. **chapter 04 §3.2 std140 offset 整合 check** (再掲)
4. **chapter 04 §4.4 同名 UBO 複数宣言一致 check** (再掲)

実装詳細は chapter 08 build pipeline で配線。

---

## §10 未確定事項 (→ chapter 10 持ち越し / AYA 判断仰ぎ)

| # | 項目 | 解消先 |
|---|---|---|
| E (解消) | set=2 vs set=3 統廃合方針 → **解消 (2026-06-03 AYA 判断: E3 採用、§4.3 reflect 済)**。統合判定の再評価は §4.4 経由で chapter 09 後半 / chapter 10 へ移管 | — |
| E' | inventory §3.3.1 同一 binding 複数 UBO 名疑い (`PerDrawUBO_ClipPlane` 等) | chapter 09 Phase 0 計測 task |
| F | `MaterialUBO` vs `MaterialUBO_Legacy` 処遇 (F1 統合 / F2 別名分離 / F3 廃止) | chapter 09 Phase 0 member 比較 + program 単位 attach grep |
| MC (解消、= 旧 G、rename = chapter 04 §10 (G) との衝突回避) | per-material cadence → **解消 (2026-06-03 AYA 判断: MC1 採用 = 旧 G1、§6.3 reflect 済)** | — |
| H1 | bare uniform 集合の完全 enumerate | chapter 06 起案時 grep + LL_INFOS hook |
| H2 | 集約表の owner (inline 維持 vs 別 file `05a-` 切出し) | 表 size > 50 行で再判定 |
| H3 | 集約判定 conflict 時の AYA 判断ループ | chapter 09 Phase 進行中に case-by-case |

**(E) / (MC) ともに解消済 (2026-06-03)** = §4.3 / §6.3 確定 reflect 済、chapter 02 / chapter 03 / chapter 01 整合 update 完了。本 chapter の AYA 判断仰ぎ事項は全件クローズ、残持越は Phase 0 計測 task ((E') / (F)) と migration 進行 live 表 ((H1) / (H2) / (H3)) のみ。

**表記注**: 解消マークは他 chapter (= chapter 06b §3.4 / chapter 10 §2 等) と統一して plain text `(解消)` 接尾辞を採用 (= 取消線 `~~~~` から変更、2026-06-03 査読 §5.7)。

---

## §11 本 chapter update 規律

- §3 cadence mapping は inventory 棚卸し変化で update
- §7.3 集約表は live 表として migration 進行で update (= 候補 → 確定 → 移行済)
- 表 size > 50 行で `05a-bare-uniform-mapping.md` 切出し、本 chapter は §7.3.1 書式と §7.4 フローのみ残存
- (E)(F)(MC) AYA 判断後に §4 / §5 / §6 / chapter 03 / chapter 04 §10 と整合 reflect
- chapter 09 Phase 進行で migration 完了した bare uniform / UBO は本 chapter §3 / §7.3 表で移行済マーク

---

**= 本 chapter で既存 85 UBO blueprint の最終配置と bare uniform 集約フローが確定したため、chapter 06 で redirect 層 (= name → offset 解決後の UBO memcpy 実装) に進める**。
