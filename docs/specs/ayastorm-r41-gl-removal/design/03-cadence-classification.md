# r41 UBO 全体設計 Chapter 03: cadence 分類体系

**起案日**: 2026-06-03
**位置付け**: 設計 doc 全 chapter で使う **update cadence の分類体系** を確定する doc。各 UBO がどの cadence に属するかは別 chapter (05 existing-inventory-link) で個別 mapping、本 chapter は分類軸そのものを定義。
**pre-requisite**: `01-overview.md` (storage lifetime vs update cadence の用語定義) / `02-naming-convention.md` (cadence prefix 規則)

---

## §1 なぜ cadence 軸を最初に確定するか

UBO の **update 頻度** が異なれば、以下が全て変わる:

- upload site (どの C++ 経路で `vmaMapMemory` + memcpy するか)
- upload thread (main thread / worker thread どちらか = 原則 2 影響)
- descriptor set 構成 (set=0/1/2/3 振り分け = Vulkan PSO compatibility 影響)
- dirty 判定粒度 (= 不必要 upload 抑制)

= **cadence 分類を曖昧にしたまま UBO 設計を進めると、上記 4 軸が全部後付け patching になる**。本 chapter で分類体系を確定してから chapter 04+ に進む。

---

## §2 cadence 分類体系 (= 6 分類確定版)

| cadence | update 契機 | 典型 owner | 典型 member | 名前 prefix (chapter 02) | 典型 update 回数 / frame |
|---|---|---|---|---|---|
| **per-frame** | frame loop 開始時に必ず 1 回 | global (LLGLSLShader frame slot 等) | `view` / `proj` / `time` / `light direction sun` | `Frame*` | 1 |
| **per-program** | shader program bind 時に 1 回 | per-program slot | tonemap parameters / atmospheric coeffs | `Program_*` | bind されている program 数 (典型 20-50) |
| **per-draw** | draw call ごとに 1 回 | per-shader local slot | per-light parameters (in multi-light fragment) | `Draw_*` | 数百〜数千 |
| **per-asset** | GLTF asset state 変化時 | `gltf::Asset` per-instance | `mNodes` / `mMaterials` | `Asset_*` | N (rezzed GLTF 数、典型 1-10) |
| **per-skin** | rigged animation 毎 frame | `gltf::Skin` per-instance | joint palette matrices | `Skin_*` | M (rigged skin 数、典型 1-10) |
| **per-material** | material 切替時 (cadence 上は per-draw の partial 化) | (TBD chapter 05) | PBR material params | `Material*` | per-draw を material change 単位で間引いた回数 |

**注**: `per-material` は **per-draw cadence の特化** (material が同じ draw 群を batch upload で間引く)。実体は **per-draw + dirty flag** で実装することで cadence の実体は per-draw に縮約しうる。chapter 05 で確定。

### §2.1 cadence 軸の独立性

各 cadence は **互いに独立** な軸。同じ UBO に複数 cadence の member を混在させてはならない:

- ❌ NG: `FrameMixed { mat4 view; vec4 per_draw_light_color; }`
- ✅ OK: `FrameViewProj { mat4 view; }` + `Draw_LightParams { vec4 color; }`

理由: 高頻度 cadence (per-draw) が低頻度 cadence (per-frame) member を引き連れる = upload 量が無駄に膨らむ。

### §2.2 cadence と storage lifetime の独立性 (再掲)

chapter 01 §3.2 で確定済。storage lifetime (= owner の生存期間) は cadence と無関係:

- `FrameViewProj`: cadence = per-frame、storage lifetime = app 起動から終了まで (LLGLSLShader 内 slot)
- `Asset_GLTFNodes`: cadence = per-asset、storage lifetime = Asset 生成から破棄まで
- 同じ buffer が **frame ごとに何度も上書き update される** だけで、buffer 自体は再生成しない

= Vulkan の `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT` + `VMA_MEMORY_USAGE_CPU_TO_GPU` で「persistent map + 毎 frame memcpy」する典型パターンに自然に乗る。

---

## §3 cadence source rule (= 最重要設計判断)

### §3.1 「cadence の判定 source」は何か

新規 UBO を導入する際、**どの cadence に分類するか** を決める判定 source。本設計では:

> **既存 C++ 呼出 path のスケジュールを cadence source とする**

つまり、UBO に upload する値が **現状の OpenGL path で どの C++ call site から bare uniform に設定されているか** を観察し、その call site の呼出頻度を cadence とみなす。

### §3.2 cadence source の具体例

| 現状 C++ call site | 呼出頻度 | UBO cadence |
|---|---|---|
| `LLEnvironment::updateShaderUniforms(shader)` (per shader bind) | shader bind 毎 (= per-program) | per-program |
| `LLPipeline::renderGeom()` 内の per-draw `uniform*fv` 呼出 | draw call 毎 | per-draw |
| `LLViewerCamera::updateProjection()` で view/proj 行列計算 | frame 毎 1 回 | per-frame |
| `gltf::Asset::updateNodeData()` | asset state 変化時 | per-asset |
| `gltf::Skin::updateTransforms()` | rigged animation 毎 frame | per-skin |

= **新規 viewer settings (cvar) を導入せず、既存 path の呼出頻度を素直に踏襲する**。

### §3.3 cadence source rule の根拠

1. **設定追加禁止 (= AYA 指示 memory `feedback_prefer_defaults_over_config`)**: 「多数の tuning キーより 1 つの妥当値を選ぶ」 = cadence を user settings 化しない
2. **原則 1 (call site 温存)**: 既存 call site のスケジュール = cadence のため、call site を変更せず cadence も変更しない
3. **既存 LL の frame loop に sync**: LL は元々無限ループだった frame loop に **60 FPS 設定 (描画更新設定) が後付けで入った** 現状を踏襲。per-frame cadence は **その frame loop に sync** すれば足りる。AYA 確認済 (2026-06-03):
   > 「LL ってそもそも frame 設定なんてなくただただ無限ループさせてただけだったのを、ようやく 60 FPS 設定とかできるようにはしたんだよね。そういう意味では frame の更新はそれに準じて揃えれば一旦は収まる気がする」

### §3.4 cadence source rule の例外条件

以下のいずれかに該当する場合のみ、新規判断として cadence を変更可能:

- 現状 call site が **本質的に間違い** (e.g., per-frame で更新すべき値を per-draw で reset している = bug)
- 現状 call site の頻度が **Vulkan で再現するとパフォーマンス致命的** (= 実測必要)
- 現状 path に該当 call site が存在しない (= 新規概念の追加)

= **本設計初期では例外発生を想定しない**。発生したら chapter 10 (open-questions) に記載して AYA に判断を仰ぐ。

---

## §4 cadence 別 update site 設計

各 cadence の update site (= Vulkan UBO に memcpy する C++ 呼出点) は以下に固定する。詳細実装は chapter 06 (redirect-layer-design) で詰める。

### §4.1 per-frame cadence

| 項目 | 設計 |
|---|---|
| update site | frame loop の開始直後 (`LLPipeline::renderGeom()` の前段) |
| upload thread | main thread (= rendering thread と同一) |
| dirty 判定 | 毎 frame 必ず upload (頻度上 dirty 判定 overhead が無意味) |
| descriptor set | set=0 (= 全 shader 共通、stable) |
| 60 FPS 設定との関係 | frame loop の 1 tick = 1 cadence、frame rate cap 設定変更時も自動追従 |

### §4.2 per-program cadence

| 項目 | 設計 |
|---|---|
| update site | shader program bind 時 (`LLGLSLShader::bind()` 内) |
| upload thread | main thread (bind と同一 thread) |
| dirty 判定 | program param 変化時のみ (= 大半の bind では skip) |
| descriptor set | set=1 (= program 単位、program 切替で descriptor set rebind) |
| `LLEnvironment::updateShaderUniforms` との関係 | dispatcher 経由で bare uniform → Program_* UBO に集約 |

### §4.3 per-draw cadence

| 項目 | 設計 |
|---|---|
| update site | draw call 直前 (`LLDrawPool` 系の geom render 直前) |
| upload thread | main thread (draw call と同一 thread) |
| dirty 判定 | draw 毎 update が前提 (dirty 判定 overhead と update の差が小さい) |
| descriptor set | set=2 (= per-draw、頻繁 rebind) |
| 物理 instance scaling | 数百〜数千 / frame、ring buffer / dynamic offset 等の Vulkan 最適化が必要 (chapter 06+) |

### §4.4 per-asset / per-skin cadence

| 項目 | 設計 |
|---|---|
| update site | 既存 C++ 呼出と同位置 (`Asset::updateNodeData()` / `Skin::updateTransforms()`) |
| upload thread | 既存 C++ thread (= 現状 main thread、worker thread 化は原則 2 影響、別 phase) |
| dirty 判定 | 既存実装そのまま (state 変化検知済の場所で upload) |
| descriptor set | set=2 帯の中で per-owner offset、または set=3 ?  (chapter 07 で確定) |
| 物理 instance scaling | N + M (scene 規模次第)、owner 単位で並列化可能な構造を維持 |

---

## §5 cadence と Core 分散の関係 (= 原則 2)

各 cadence は将来的に **独立 thread / 独立 cmdbuf に分離可能** な設計を維持する:

| cadence | 並列化先 | 並列化 phase |
|---|---|---|
| per-frame | main thread から外す (= upload を frame 開始前に worker thread で先行実行) | Phase Y (TBD) |
| per-program | program 単位で worker thread 分散 (= shader 種別ごとに独立 cmdbuf) | Phase Y+1 |
| per-draw | draw call 単位での分散は Vulkan secondary cmdbuf で再現 | Phase Y+2 |
| per-asset | per-Asset 単位で worker thread (= asset ごとに独立 upload) | Phase Y |
| per-skin | per-Skin 単位で worker thread (= skin animation を CPU 並列計算 + 直接 upload) | Phase Y |

= **本 chapter で確定する分類体系は core 分散実装の前提**。cadence 軸が曖昧だと並列化対象を切り出せない。

---

## §6 cadence 分類 update 規律

- 新 cadence の追加は AYA 確認後 (= 言語規約の追加と同等)
- 既存 cadence の分類変更 (e.g., per-program → per-draw) は **設計理由 + 影響範囲 + chapter cross-ref** を本 doc に追記
- §4 の update site / thread / dirty 判定は chapter 06 (redirect-layer-design) と整合維持
- §5 の Core 分散 phase は chapter 09 (phase-roadmap) と整合維持

---

**= 本 chapter で cadence 軸が確定したため、chapter 04 (codegen-ubo) で各 cadence の Codegen-UBO 生成規則、chapter 05 で既存 84 UBO blueprint の cadence 別 mapping に進める**。
