# r40 sub-phase 3 work item (c): 工程算定

**status**: foundation group (§1 per-file + §2 per-shader) draft 完了 — §3-§8 は group A/B/C で順次 draft 予定
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)
**前置 doc**:
- `04-portage-inventory.md` (work item (a)) — per-file 工数の input source
- `05-vulkan-api-design.md` (work item (b)) — per-section 設計から派生する工数算定 input
- `00-charter.md` §4 (3) — 参照点 (Doom / Blender) + 6-15 人年 + 本職並走 ratio
- `00-charter.md` §8 (B) — plan B trigger 条件の閾値設定対象
**達成条件**: §1-§8 全 section draft 完成 + AYA review PASS → work item (d) r42+ 区切り確定 着手

---

## 算定方針 (全 section 共通の原則)

### 単位

- 工数の基本単位は **人月 (PM, person-month)** = フルタイム dev 1 人月相当の作業量
- 期間表記は **暦月 (calendar month) 換算** = 体制 + 並走 ratio を掛けた所要月数
- 人月 ↔ 暦月 変換は本 doc §5 で確定する **AYA 本職並走 ratio** に従う

### AYA 体制前提 (charter §4 (3) 再掲)

- **体制**: AYA 1 人、本職並走 (full-time 不可)
- **Vulkan 経験**: 初見 (学習曲線織込み)
- **time horizon**: 無期限 (撤退条件は時間軸でなく外部条件、charter §8 (A))
- **本職並走 ratio**: フルタイム dev 1 人月 = AYA 並走何暦月、§5 で確定 (charter §4 (3) 想定 = 3-5x)

### 参照点 (charter §4 (3) 再掲)

| 事例 | 体制 | 工数 / 期間 | 性質 |
|---|---|---|---|
| Doom 2016 (id Tech 6 OpenGL → Vulkan) | 3 名 (経験者) | 6-12 か月 (実工数 18-36 人月) | clean abstraction あり |
| Blender Vulkan | 多数 (rotating contributor) | 2019 着手 → 2026 現在 7 年未完 | AYAstorm 規模に近い (3D viewport renderer のみで Blender 全体は除外) |
| AYAstorm Vulkan (本算定対象) | AYA 1 人 + 本職並走 | charter §4 (3) 想定 6-15 人年 | abstraction 不在 → §10 skeleton で部分緩和、3 大グローバル + 248 shader + AYAstorm 13 file |

### 余裕係数 (review / test / bug fix)

- 各 milestone work 工数に **+30-50% の余裕** (charter §4 (3) + 03 doc §5 算定軸 5)
- 余裕係数は milestone 内訳 §3 で per-milestone 適用、§6 uncertainty band で上下振れ反映

### 算定 source map

| 算定対象 | 主要 source |
|---|---|
| per-file 工数 (§1) | 04 doc §6 棚卸し総括 (file 種別 × LOC × verdict) |
| per-shader 工数 (§2) | 04 doc §1.3 + §5.x AYAstorm 13 file shader 棚卸し + 05 doc §2 cross compile chain |
| per-milestone 工数 (§3) | 04 doc §5.4 段階 port 戦略 5 段階 + AYAstorm 3 機能合成順 + 05 doc 全 §1-§10 設計確定範囲 |
| 3 OS 増分 (§4) | 04 doc + 05 doc §8 OS 別 + §9.4 MoltenVK portable subset + charter §4 (2) Linux 先行 |
| milestone 月数 (§5) | §1-§4 積算 + 本職並走 ratio + 余裕係数 |
| uncertainty band (§6) | §5 中央値 ± Doom/Blender 参照点 + 単独 fork 不確実性 |
| Doom Blender 比較 (§7) | §1-§6 結果と参照点の校正、本算定の妥当性 cross check |
| plan B trigger (§8) | charter §8 (B) 工程プラン破綻判定の閾値、§5 中央値 / §6 upper bound から逆算 |

---

## §1 per-file 工数算定 (棚卸し (a) 出力に基づく)

### §1.1 file 種別 × LOC × verdict ごとの per-file 工数原単位

#### 算定 base = a-3 §5.4 段階 port 戦略 (フルタイム dev、経験者前提)

04 doc §5.4.1 が提示する base port 5 段階の工数感を per-file 単位に分解、後段 §1.2-§1.6 で各領域に適用:

| 段階 | 範囲 file | 04 doc §5.4.1 工数 | 平均 per-file 換算 (LOC 加重前) |
|---|---|---|---|
| 1 GL header wrapper 置換 + volk loader | wrapper 3 file (+ 188 上流対応) | 0.5 PM | 0.17 PM/file (wrapper) + 上流 188 file は ~0 PM/file (wrapper 経由で自動対応) |
| 2 lldrawpool Vulkan 化 | 13 file | 1.0 PM | 0.077 PM/file |
| 3 state machine → PSO 化 | llrender 主要 5 file (llgl/llrender/llimagegl/llrendertarget/llpostprocess) | 1.5 PM | 0.30 PM/file |
| 4 pipeline.cpp 3 大グローバル → frame context | 1 file (15,954 LOC、最難関) | 1.0 PM | 1.0 PM/file (1 file で 1 PM、最高密度) |
| 5 llspatialpartition / llviewershadermgr / llvertexbuffer 依存解決 | 3 area (~7-9 file) | 0.5 PM | 0.06-0.17 PM/file |

**base port 合計 = 4.5 PM (フルタイム dev、フルタイム経験者、a-3 §5.4 13-17 週間 / 4-5 人月 の中央値)**。

#### per-file 工数原単位の verdict × LOC density 表

a-3 工数感を **per-file 原単位** (LOC 加重前 PM/file) に正規化:

| verdict | 性質 | 原単位 (PM/file) | 根拠 |
|---|---|---|---|
| 要 port (直訳) | GL call → Vulkan equivalent 置換 | **0.05-0.10 PM/file** | lldrawpool 13 file 1 PM = 0.077 PM/file の中央値帯 |
| 要再設計 (中規模) | state machine → PSO 化、interface 残置 | **0.25-0.35 PM/file** | llrender 主要 5 file 1.5 PM = 0.30 PM/file |
| 要再設計 (大規模 / 最難関) | pipeline.cpp 3 大グローバル / 4K LOC 超 | **0.5-1.5 PM/file** | pipeline.cpp 1 PM (16K LOC で 1 file)、llspatialpartition 4K LOC で 0.3-0.5 PM 想定 |
| 不要 port (cleanup) | 削除 or skip、依存切り離し | **0.02-0.05 PM/file** | dependent code refactor 程度 |
| wrapper 3 file (局在化 hub) | 188 file の上流 wrapper 経由対応 | **0.17 PM/file 単独 + 上流 0** | 段階 1 全体 0.5 PM / 3 file、wrapper 経由で 188 file は file 単位 0 |

#### LOC 加重補正

原単位は file 数で粗算定、LOC density (1 file 内の GL call 密度 / 構造複雑度) で ±50% 補正:

- **LOC > 5K** or **GL call > 40**: + 50% (例: llgl.cpp 3K LOC 81 GL call、llrendertarget.cpp 35 GL call は加重対象)
- **LOC < 500** and **GL call < 5**: - 50% (small utility, 例: llcubemap.cpp 343 LOC 3 GL call)
- 標準帯 (500 ≤ LOC ≤ 5K かつ 5 ≤ GL call ≤ 40): 補正なし

### §1.2 indra/llrender/ 28,168 LOC の per-file 工数表

04 doc §1.1 (a-1) + §6.2 (a-4) から、llrender 全 51 file (header 25 + source 26) を verdict 別に分類して per-file 工数を算出:

#### 要 port 16 file (6,518 LOC、a-4 §6.2 final)

主要 file (a-1 §1.1 TOP 14 のうち要 port verdict 該当):

| file | LOC | GL call | 原単位 (PM) | LOC density 補正 | 工数 (PM) |
|---|---|---|---|---|---|
| llvertexbuffer.cpp | 1,942 | 40 | 0.075 | 0% | 0.075 |
| llglslshader.cpp | 2,089 | 41 | 0.075 | 0% | 0.075 |
| llfontgl.cpp | 1,458 | 0 | 0.075 | -25% (GL 少) | 0.056 |
| llfontregistry.cpp | 830 | 0 | 0.075 | -50% | 0.038 |
| llshadermgr.cpp | 1,689 | 32 | 0.075 | 0% | 0.075 |
| llrender2dutils.cpp | 1,872 | 2 | 0.075 | -25% (GL 少) | 0.056 |
| llcubemap.cpp | 343 | 3 | 0.075 | -50% | 0.038 |
| llcubemaparray.cpp | (~200) | (~3) | 0.075 | -50% | 0.038 |
| その他 interface header 8 個 | (含む) | (含む) | 0.025 | -50% | 0.20 (合計) |
| **要 port 16 file 小計** | **~6,518** | — | — | — | **~0.65 PM** |

#### 要再設計 10 file (7,937 LOC、a-4 §6.2 final、wrapper 3 file 含む)

| file | LOC | GL call | 原単位 (PM) | LOC density 補正 | 工数 (PM) |
|---|---|---|---|---|---|
| llgl.cpp | 3,027 | 81 | 0.30 | +50% (GL 多) | 0.45 |
| llrender.cpp | 2,207 | 44 | 0.30 | +50% (GL 多) | 0.45 |
| llimagegl.cpp | 2,663 | 59 | 0.30 | +50% (GL 多 + threading) | 0.45 |
| llrendertarget.cpp | 589 | 35 | 0.30 | 0% (35 GL call 標準帯) | 0.30 |
| llpostprocess.cpp | 454 | 11 | 0.30 | 0% | 0.30 |
| llglheaders.h (wrapper hub) | (~300) | — | 0.17 | — | 0.17 |
| llglstates.h (wrapper hub) | (~200) | — | 0.17 | — | 0.17 |
| llgltypes.h (wrapper hub) | (~150) | — | 0.17 | — | 0.17 |
| 関連 header 2 個 (llglmanager.h 等) | (~547) | — | 0.10 | — | 0.20 (合計) |
| **要再設計 10 file 小計** | **~7,937** | — | — | — | **~2.66 PM** |

#### 不要 port 3 file (1,820 LOC、a-1 §1.1 暫定)

| file | LOC | 工数 (PM) |
|---|---|---|
| llfontfreetype.cpp | 1,171 | 0.03 (cleanup) |
| llfontfreetypesvg.cpp | 259 | 0.02 |
| llfontbitmapcache.cpp | 271 | 0.02 |
| **不要 port 3 file 小計** | **~1,701** | **~0.07 PM** |

#### 判定保留 17 file (~5,000 LOC、a-1 §1.1 + a-4 §6.5)

- header 系 utility (LLGLNamePool 等)、a-2 で要 port verdict 確定済の対応 cpp と pair 計上、本 §1.2 では重複排除のため 0 計上
- 残る独立 utility (a-3 §5.4.5 判定保留 = ~7-9 file 1-2K LOC) は 0.02-0.05 PM/file × 5 file = 0.15 PM

#### llrender 全 51 file 合計

| 分類 | file 数 | LOC | 工数 (PM) |
|---|---|---|---|
| 要 port | 16 | 6,518 | 0.65 |
| 要再設計 | 10 | 7,937 | 2.66 |
| 不要 port | 3 | 1,701 | 0.07 |
| 判定保留 (独立 utility) | ~5 | ~1,500 | 0.15 |
| 判定保留 (pair 計上済) | ~12 | ~2,500 | (重複排除で 0) |
| **llrender 全 51 file 合計** | **51** | **~20,156** (純 portage 対象) | **~3.53 PM** |

注: a-3 段階 1 (wrapper 0.5 PM) + 段階 3 (state machine 1.5 PM) + 段階 5 一部 (llrender 系依存解決 ~0.2 PM) = ~2.2 PM が llrender 主因部分、§1.2 算出 3.53 PM は要 port file の追加 (lldrawpool には含まれない llrender 内 utility) を反映、a-3 工数感より +60% 上方 (a-3 は最大コア絞り、§1 は全 file 列挙ベース)。

### §1.3 indra/newview/pipeline.cpp + .h 15,954 LOC の per-section 工数表

pipeline.cpp は **単一 file で 1 PM** (a-3 §5.4.1 段階 4) の最難関。15,954 LOC を per-section に分解して内訳を提示:

| section | LOC (概算) | 主要 work | 工数 (PM) |
|---|---|---|---|
| frame context 設計 + 3 大グローバル統合 (sCull / sShadowRender / sCurCameraID + 176 参照) | ~3,000 | LLPipelineFrameContext 仮称 設計 + 176 参照を per-frame instance 経由に書換 | 0.40 |
| render stage dispatch → VkRenderPass chain (05 doc §4.3 7 pass chain 配置) | ~4,000 | 各 stage の vkCmdBeginRendering 配置 + render stage flow 整理 | 0.30 |
| cull / occlusion / mark logic (GL call 内蔵) | ~2,500 | checkOcclusion / markOccluder の Vulkan query pool 化 (a-3 §5.4.1 段階 5 と一部 overlap) | 0.15 |
| state setup / state restore + GL state machine 連携 (段階 3 と境界) | ~2,500 | 段階 3 state machine 廃止に伴う pipeline.cpp 側 cleanup | 0.10 |
| pipeline.h interface (declaration) | ~1,375 | header 整合維持 (LLPipelineFrameContext interface 出口) | 0.05 |
| その他 utility / debug / glow / post-process glue | ~2,579 | minor cleanup | 0.05 |
| **pipeline.cpp + .h 合計** | **15,954** | — | **~1.05 PM** |

注: a-3 段階 4 = 1 PM/file の見積 = ~1.05 PM 配分結果と整合。段階 4 が「設計議論 1-2 月含む」と a-3 §5.4.1 で示唆、§5 milestone 月数 換算では design overhead を別途反映。

### §1.4 lldrawpool*.cpp 7,907 LOC × 13 file の per-file 工数表

a-2 §2.2 + 04 doc §6.2 から 13 file all 要 port、a-3 §5.4.1 段階 2 = 1 PM 合計:

| file | LOC (概算) | 特殊性 | 工数 (PM) |
|---|---|---|---|
| lldrawpoolavatar.cpp | ~1,800 | rigged mesh + BoM、AYAstorm r21.1 picker touchpoint | 0.12 |
| lldrawpoolalpha.cpp | ~1,200 | forward alpha + particles、05 doc §4.3 pass 4 | 0.10 |
| lldrawpoolwlsky.cpp | ~521 | windlight sky、05 doc §4.3 pass 5 | 0.06 |
| lldrawpoolsky.cpp | ~57 | 最小 sky | 0.02 |
| lldrawpoolbump.cpp | ~800 | normal map、shadow eligible | 0.08 |
| lldrawpoolmaterials.cpp | ~600 | legacy material | 0.07 |
| lldrawpoolpbropaque.cpp | ~700 | PBR opaque、05 doc §4.3 pass 2 | 0.08 |
| lldrawpoolterrain.cpp | ~700 | **glTexGen 廃止 shader explicit UV 計算 (a-3 §5.4.1 段階 2 注記)** | 0.10 (+特殊性) |
| lldrawpooltree.cpp | ~300 | tree (a-2 § tree pool 特殊配線) | 0.05 |
| lldrawpoolwater.cpp | ~500 | water | 0.06 |
| lldrawpoolground.cpp | ~150 | ground | 0.03 |
| lldrawpoolsimple.cpp | ~400 | simple non-deferred | 0.05 |
| lldrawpool.cpp (基底) | ~200 | interface 残置 | 0.05 |
| **lldrawpool 13 file 合計** | **~7,907** | — | **~0.97 PM** |

注: a-3 段階 2 = 1.0 PM/13 file と整合 (0.97 ≈ 1.0)。terrain.cpp は glTexGen 廃止の追加 design 1-2 日込み (+0.05 PM 想定だが file 工数原単位 0.10 PM/file で吸収済)。

### §1.5 llspatialpartition 4,416 LOC + llviewershadermgr 4,423 LOC + llvosky/llvowlsky 2,198 LOC

#### llspatialpartition.cpp (4,416 LOC、要再設計 大規模)

| section | LOC (概算) | 主要 work | 工数 (PM) |
|---|---|---|---|
| occlusion query (GL_QUERY_RESULT → vkGetQueryPoolResults async、05 doc §5 sync2) | ~1,200 | VkQueryPool 化 + async result | 0.20 |
| geometry rebuild (rebuildMesh、a-1 §1.4 cull/stateSort 内 GL 呼出 root cause) | ~1,500 | 05 doc §5.5 worker thread staging buffer → vkCmdCopyBuffer 経路化 | 0.20 |
| spatial partition / octree update | ~1,200 | logic 自体は Vulkan 非依存、interface 残置で対応 | 0.05 |
| その他 (debug / interface header pair) | ~516 | minor cleanup | 0.05 |
| **llspatialpartition 合計** | **4,416** | — | **~0.50 PM** |

#### llviewershadermgr.{cpp,h} (4,423 LOC、要 port)

| section | LOC (概算) | 主要 work | 工数 (PM) |
|---|---|---|---|
| shader load + compile (GLSL → 05 doc §2 SPIR-V chain への置換) | ~2,000 | LLGLSLShader → LLVulkanShader rename、vkCreateShaderModule 経路化 | 0.20 |
| capability detect + feature gate (LL の class1/2/3 切替) | ~1,000 | VkPhysicalDeviceFeatures + 05 doc §9.1-§9.3 enable list との整合 | 0.10 |
| descriptor set layout setup (05 doc §3 で 3 set 確定) | ~1,000 | shader reflection (glslang --reflect) 経由 layout 自動算定 | 0.10 |
| その他 | ~423 | minor cleanup | 0.05 |
| **llviewershadermgr 合計** | **4,423** | — | **~0.45 PM** |

#### llvosky.cpp + llvowlsky.cpp (2,198 LOC、要 port、r14+ visual realism 基盤)

| file | LOC | 主要 work | 工数 (PM) |
|---|---|---|---|
| llvosky.cpp | ~1,300 | sky dome geometry + atmospherics state、05 doc §4.6 pass 5 forward 描画化 | 0.15 |
| llvowlsky.cpp | ~898 | windlight sky dome、atmospherics LUT 連携 | 0.12 |
| **llvosky + llvowlsky 合計** | **2,198** | — | **~0.27 PM** |

注: llvosky/llvowlsky は r14+ visual realism の基盤、a-3 §B.2 で「Vulkan 化の要 port 範囲は GLSL → SPIR-V 化 + uniform 配信方式変更 (push constant / per-frame UBO) のみ、interface 変更最小」と確定、純粋 file 工数 0.27 PM (shader work は §2 で別途計上)。

### §1.6 GL header wrapper 3 file (llglheaders + llglstates + llgltypes) 工数

#### wrapper hub 3 file (上流 188 file 自動対応)

| file | LOC | 工数 (PM) | 追加効果 |
|---|---|---|---|
| llglheaders.h | ~300 | 0.17 | `#include <GL/gl.h>` 系 → `#include <volk.h>` + Vulkan core type alias、上流 file への影響なし |
| llglstates.h | ~200 | 0.17 | OpenGL state machine wrapper → Vulkan pipeline state placeholder (本格 PSO 化は段階 3) |
| llgltypes.h | ~150 | 0.17 | GL type alias (GLuint/GLfloat) → Vulkan type alias (VkBuffer 等) + 既存 type の using 維持 |
| **wrapper 3 file 単独** | **~650** | **0.5 PM** | (a-3 §5.4.1 段階 1 と整合) |

#### 上流 188 file (wrapper 経由) の工数

a-1 §1.2 確定: 188 file (indra/newview 48 / indra/llui 19 / indra/llwindow 5 / etc.) は **wrapper 経由で自動対応**、per-file 工数 0 PM。

例外 (a-1 §1.2 終盤):
- `indra/llwindow` 5 file (platform-specific WGL/GLX 系) と `media_plugins` 5 file (platform-specific GL 処理) は要個別調査
- 工数概算: 各 0.05 PM × 10 file = **0.5 PM** (個別 work、wrapper では吸収できない)
- a-3 工数感では段階 1 内訳 or 段階 5 依存解決 内に潜伏、§1.6 で独立可視化

#### wrapper + 上流 合計

| 範囲 | 工数 (PM) |
|---|---|
| wrapper 3 file 本体 | 0.50 |
| 上流 188 file (wrapper 経由) | 0 |
| 例外 platform-specific 10 file | 0.50 |
| **wrapper + 上流 合計** | **1.0 PM** |

注: a-3 段階 1 = 0.5 PM は wrapper 本体のみ計上、§1.6 で platform-specific 10 file の +0.5 PM が追加 (a-3 で潜伏していた cost を可視化)。

### §1.7 C++ critical path ~63K LOC 合計 工数 (フルタイム dev 換算)

#### 領域別 sum

| 領域 | 出処 § | file 数 | LOC | 工数 (PM) |
|---|---|---|---|---|
| indra/llrender/ | §1.2 | 51 | ~20,156 (純 portage) | 3.53 |
| pipeline.cpp + .h | §1.3 | 2 | 15,954 | 1.05 |
| lldrawpool*.cpp | §1.4 | 13 | 7,907 | 0.97 |
| llspatialpartition | §1.5 | 1 | 4,416 | 0.50 |
| llviewershadermgr | §1.5 | 2 | 4,423 | 0.45 |
| llvosky + llvowlsky | §1.5 | 2 | 2,198 | 0.27 |
| GL header wrapper + platform-specific | §1.6 | 13 (wrapper 3 + platform 10) | ~1,150 | 1.00 |
| **C++ critical path 合計 (per-file 算出)** | — | **84** | **~56,200 (純 portage)** | **~7.77 PM** |

#### a-3 段階 port 工数感との突き合わせ

| 算定 | 工数 (PM、フルタイム dev) |
|---|---|
| a-3 §5.4.1 段階 1-5 合計 (中央値) | **4.5 PM** |
| §1.7 per-file 算出 合計 | **7.77 PM** |
| 差 (§1.7 − a-3) | **+3.27 PM (+73%)** |

#### 差 +73% の理由分析

a-3 § 5.4 段階 port 戦略は「**段階順序 + 概算オーダー** の確定のみが目的、絶対値の精緻化は work item (c) で実施」(04 doc §5.4.3) と明示しており、§1.7 算出 7.77 PM が本算定の **per-file 精緻化結果**。

差の主因:

1. **a-3 が「main file 絞り」、§1.7 が「全 file 列挙」**: a-3 段階 3 = 5 file (主要)、§1.7 段階 3 該当 = llrender 全 10 要再設計 (+5 utility) = 10+ file 算出、+1.5 PM 程度の差
2. **platform-specific 10 file の潜伏 cost 可視化** (§1.6): a-3 では潜伏、§1.7 で +0.5 PM
3. **llspatialpartition + llviewershadermgr + llvosky/llvowlsky の per-file 算出**: a-3 段階 5 = 0.5 PM (3 area まとめ) vs §1.7 = 0.50 + 0.45 + 0.27 = 1.22 PM (各 area 個別算出)、+0.72 PM
4. **判定保留 utility (5 file × 0.03 PM)**: a-3 では非計上、§1.7 で +0.15 PM

合計差 ≈ a-3 過小評価 +1.5 + 0.5 + 0.72 + 0.15 = **+2.87 PM**、概算 ≈ §1.7 の +3.27 PM と整合。

#### 本算定の base 値採用

**§1.7 算出 7.77 PM を本算定の C++ critical path base 値として採用**、a-3 4.5 PM は「main file 絞りでの概算」として参照点維持。

a-3 §5.4.3 表「a-3 工数見積はフルタイム dev / 経験者前提、charter §4 (3) は AYA 本職並走 / Vulkan 初見」の前提乖離は §5 milestone 月数で別途反映 (本職並走 ratio + 学習曲線)。

#### per-shader 工数とは別途加算

§1.7 は C++ critical path のみ、shader 248 file + AYAstorm 13 file の SPIR-V 化 + descriptor set 再設計工数は §2 で別途算出、§3 milestone 積算で C++ + shader を統合。

---

## §2 per-shader 工数算定 (棚卸し (a) + 設計 (b) 出力に基づく)

### §2.1 base 248 file の per-shader 工数原単位

#### 算定 base = 04 doc §1.3 (a-1) + 05 doc §2 (b)

04 doc §1.3 で確定: 248 file (vertex 110 / fragment 124 + その他 14) = compute / geometry / tessellation **全部ゼロ**、bindless / atomic / coherent **全部ゼロ**、~85% は GLSL 3.3 core subset。

05 doc §2 で確定: glslang cross compile 経路 = `.glsl` → glslang → `.spv` (build time) → `VkShaderModule` (runtime load)、Vulkan SDK 1.3.x 同梱 binary 使用。

#### per-shader 工数原単位の分類

248 file を cross compile 通過率で 3 階層に分類:

| 分類 | 推定 file 数 | per-file 工数 (PM) | 主要 work |
|---|---|---|---|
| **A. cross compile 素通り** (~85% = ~211 file) | ~211 | 0.005 PM/file | build integration setup + smoke test (visual diff なし想定) |
| **B. cross compile 要修正** (~15% = ~37 file) | ~37 | 0.025 PM/file | deprecated 関数 (`texture2D` → `texture`) / built-in 座標系 (y 軸反転 / `gl_FragCoord` 等) / `layout(location)` 整合 |
| **C. AYAstorm 改変 + descriptor 影響** (13 file + sampler 206 個 binding 再設定) | 13 + (全 248 影響) | §2.3 + §2.4 で個別算出 | 機能整合確認 + descriptor set binding 再設定 |

A 階層 per-file 0.005 PM = 1 PM で ~200 file 処理、build integration + smoke test (起動確認 + 該当 pass 描画確認のみ) の粒度想定。

B 階層 per-file 0.025 PM = 半日/file 換算、a-1 §1.3 の「extension 使用: GL_ARB_shader_texture_lod / GL_ARB_texture_rectangle / GL_EXT_gpu_shader4」「~15% は core 外」を反映、05 doc §2 の「glslang `--invert-y` で吸収」等の機械置換でカバーできる範囲想定。

#### LOC density 補正 (shader 版)

shader は C++ と異なり 1 file あたり LOC 振れ幅が小さい (典型 50-200 LOC)、補正は限定的に適用:

- **LOC > 500** or **sampler > 5**: +50% (例: volumetricLightF.glsl 系、複数 sampler + 行列演算多数)
- **LOC < 30** (interface stub 等): -50%
- 標準帯 (30 ≤ LOC ≤ 500 かつ sampler ≤ 5): 補正なし

### §2.2 directory 別 per-file 工数 (base 248 file)

04 doc §1.3 (a-1) 確定の class 別分布を base に算出:

| directory | file 数 | 性質 | 階層 (§2.1) | per-file 工数 (PM) | 工数 (PM) |
|---|---|---|---|---|---|
| class1/deferred | 120 | deferred rendering core (g-buffer / lighting / SSAO 等) | A 90 + B 30 | A 0.005 / B 0.025 | 0.45 + 0.75 = 1.20 |
| class1/interface | 44 | UI (HUD / cursor / 2D blit) | A 40 + B 4 | A 0.005 / B 0.025 | 0.20 + 0.10 = 0.30 |
| class3/deferred | 16 | high-end feature (SSAO / DoF / reflection probe 等) | A 12 + B 4 | A 0.005 / B 0.025 | 0.06 + 0.10 = 0.16 |
| class1/objects | 14 | object 描画 (alpha / blend / fullbright) | A 12 + B 2 | A 0.005 / B 0.025 | 0.06 + 0.05 = 0.11 |
| class1/lighting | 9 | forward lighting | A 7 + B 2 | A 0.005 / B 0.025 | 0.035 + 0.05 = 0.085 |
| class1/windlight | 8 | windlight sky / atmosphere | A 6 + B 2 | A 0.005 / B 0.025 | 0.03 + 0.05 = 0.08 |
| class1/effects | ~10 (推定) | 特殊効果 (glow / postdeferred) | A 8 + B 2 | A 0.005 / B 0.025 | 0.04 + 0.05 = 0.09 |
| class2/deferred (class1 fallback) | ~10 (推定) | class2 high quality variant | A 8 + B 2 | A 0.005 / B 0.025 | 0.04 + 0.05 = 0.09 |
| その他 (avatar / environment / cinematic_bd / 他 class) | ~17 (残) | r30 cinematic_bd 含む | A 12 + B 5 | A 0.005 / B 0.025 | 0.06 + 0.125 = 0.185 |
| **base 248 file 合計** | **248** | — | A 195 + B 53 | — | **~2.32 PM** |

注: directory 内訳の細部 file 数 (effects / class2 等) は 04 doc §1.3 では「その他 54」とまとめられており、本 §2.2 で 10/10/17/etc. と分解は推定値。base 248 file 合計値は §2.1 階層比 (A 85% / B 15%) を満たすよう一致化 (A 195 + B 53 = 248、A:B = 78.6:21.4 で §2.1 階層比からわずかに B 寄り、AYAstorm 改変分の 13 file が C 階層別計上なので B 比率がやや高めに出る妥当範囲)。

### §2.3 AYAstorm 改変 13 file の per-shader 工数

04 doc §1.3 + §B.1-§B.3 (a-3) で確定の AYAstorm 改変 shader を 3 章別に算出。a-3 §B.x の概算工数感 (フルタイム dev) には pipeline.cpp + render pass 配置 + shader の 3 要素が混在、本 §2.3 では **shader 部分のみ** 抽出 (C++ 部分は §1 + §3 milestone で計上済 / 計上予定)。

#### r21.1 self-rigged picker shader 2 file

a-3 §B.1: 関連 shader = `class1/deferred/fsObjectIDV.glsl` + `fsObjectIDF.glsl` (2 file, ~200 LOC)

| shader | 性質 | 工数 (PM) |
|---|---|---|
| fsObjectIDV.glsl | vertex output に LocalID/ObjectID 引渡 (interpolation 設定) | 0.05 |
| fsObjectIDF.glsl | fragment output write (layout 宣言のみ、cross compile 素通り見込) | 0.05 |
| **r21.1 picker shader 小計** | **2 file** | **0.10 PM** |

注: a-3 §B.1 工数感 0.5 PM 合計のうち、shader 部分 0.10 PM (残 0.40 PM = pipeline.cpp 4 LOC 0.05 + render pass attachment 設計 0.20 + read-pick テスト 0.15 は §1.3 + §3.3 r42-α milestone で計上)。

#### r30 Cinematic BD shader 4 file

a-3 §B.3: 関連 shader 4 file = volumetricLightF.glsl (class1 + class3 pair) + screenSpaceReflUtil.glsl (cinematic_bd/class3) + DoF 関連 1 file

| shader | 性質 | 工数 (PM) |
|---|---|---|
| volumetricLightF.glsl (class1) | uniform 4 sampler + 5 mat4、SPIR-V cross compile + uniform layout 整合 | 0.10 |
| volumetricLightF.glsl (class3) | high-end variant、複数 sampler | 0.10 |
| cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl | SSR utility、uniform 2 sampler + 6 mat4 | 0.10 |
| DoF 関連 shader (1 file) | DoF state enum 化に伴う uniform 経路変更 | 0.10 |
| **r30 Cinematic BD shader 小計** | **4 file** | **0.40 PM** |

注: a-3 §B.3 工数感 2-3 PM 合計のうち、shader 部分 0.40 PM (残 1.6-2.6 PM = pipeline.cpp 6 分岐の frame context 統合 + DoF state enum 化 + visual quality テスト は §1.3 + §3.4 r42-β milestone で計上)。

#### r14+ visual realism shader 7 file

a-3 §B.2: 関連 shader 7 file = post-process pass chain + atmospherics 関連

| shader | 性質 | 工数 (PM) |
|---|---|---|
| post-process pass 1 (vignette) | r14+ 追加、descriptor set 設計影響 | 0.10 |
| post-process pass 2 (tone map / color grading) | scene buffer alpha 保護 invariant (memory `project_aya_visual_realism_alpha_protect.md`) | 0.10 |
| post-process pass 3 (godrays / volumetricLight) | additive blend、`frag_color.a = 0` invariant | 0.10 |
| post-process pass 4-5 (bloom / glow 系) | multi-pass、intermediate sampler binding | 0.15 |
| atmospherics 関連 shader 1-2 file (sky integration) | llvosky/llvowlsky uniform 連携 | 0.10 |
| その他 visual realism shader | 残 | 0.05 |
| **r14+ visual realism shader 小計** | **~7 file** | **0.60 PM** |

注: a-3 §B.2 工数感 2-3 PM 合計のうち、shader 部分 0.60 PM (残 1.4-2.4 PM = post-process descriptor set 設計 + pipeline.cpp post-process chain 5 LOC + performance profile + visual A/B は §3.5 r42-γ milestone で計上)。

#### AYAstorm 改変 13 file 合計

| 章 | file 数 | 工数 (PM) |
|---|---|---|
| r21.1 picker | 2 | 0.10 |
| r30 Cinematic BD | 4 | 0.40 |
| r14+ visual realism | 7 | 0.60 |
| **AYAstorm 改変 13 file 合計** | **13** | **1.10 PM** |

C 階層 (AYAstorm 改変) per-file 平均 = 1.10 / 13 = **0.085 PM/file**、A 階層 0.005 PM/file の 17 倍、B 階層 0.025 PM/file の 3.4 倍。AYAstorm 改変 shader は単に cross compile 通すだけでなく **AYAstorm 機能整合確認 (live A/B 含む)** が必要なため割増。

### §2.4 descriptor set 再設計 (05 doc §3) の per-shader 反映工数

05 doc §3 で確定: descriptor set 3 構成 (set=0 per-frame / set=1 per-material / set=2 per-draw) + sampler 206 個を 3 set に分配 + `VK_KHR_push_descriptor` + `VK_KHR_inline_uniform_block` 採用。

#### per-shader 反映工数の構造

descriptor set 再設計は **全 shader 248 + AYAstorm 13 = 261 file に layout 宣言の binding 変更が波及**、ただし機械的置換が主体:

| work | 対象 | per-file 工数 (PM) | 合計 (PM) |
|---|---|---|---|
| `layout(set=N, binding=M)` 注入 (glslang `--reflect` で自動算定 → script で一括書換) | 全 261 file | 0.002 PM/file (script 1 度作成 + 全 file 適用 + smoke test) | 0.52 |
| sampler 206 個の set 配置確定 (05 doc §3.2 で 30 / 80 / 96 配分済) | 06 doc work item (c) 全体で 1 度確定、shader 側調整は §2.4 内に閉じる | — (一括) | 0.30 |
| push descriptor 経路適用 (per-draw 高速化、05 doc §3.1 で確定) | per-draw 系 shader 80 個程度 | 0.005 PM/file | 0.40 |
| inline uniform block 適用 (05 doc §3.1 で確定) | 小 material 定数を持つ shader 50 個程度 | 0.005 PM/file | 0.25 |
| descriptor set layout 反映後の全 shader recompile + smoke test | 全 261 file | 0.001 PM/file | 0.26 |
| **descriptor set 再設計 反映工数 小計** | — | — | **~1.73 PM** |

注: descriptor set 再設計 は 05 doc §3 で **設計確定**、本 §2.4 は **shader 側への反映 (layout 注入 + smoke test)** のみ計上。layout 自動算定 script 作成 (glslang `--reflect` JSON parse + 一括書換) は 1 度の整備で全 file カバー可、per-file 工数は smoke test cost が支配的。

### §2.5 shader 合計工数 (フルタイム dev 換算)

#### 領域別 sum

| 領域 | 出処 § | file 数 | 工数 (PM) |
|---|---|---|---|
| base 248 file (cross compile 主体) | §2.2 | 248 | 2.32 |
| AYAstorm 改変 13 file (機能整合確認込み) | §2.3 | 13 | 1.10 |
| descriptor set 再設計 反映 (全 261 file 波及 + 1 度の整備) | §2.4 | (全 261) | 1.73 |
| **shader 合計** | — | **261** | **~5.15 PM** |

#### a-3 工数感との突き合わせ

a-3 §B.1-§B.3 の AYAstorm 改変分 (picker 0.5 + Cinematic 2-3 + visual realism 2-3 = 4.5-6.5 PM) には pipeline.cpp / render pass / shader が混在、本 §2 では **shader 部分のみ抽出**:

| 算定 | 工数 (PM、フルタイム dev) |
|---|---|
| a-3 AYAstorm 3 機能 合計 (中央値 5.5 PM の shader 抽出分推定 ~1-1.5 PM) | **~1-1.5 PM** |
| §2.3 AYAstorm 改変 shader 抽出 | **1.10 PM** |
| 整合性 | a-3 抽出推定範囲内、整合 ✓ |

base 248 file の SPIR-V 化工数 (2.32 PM) + descriptor set 反映 (1.73 PM) は a-3 では「§5.4.1 段階 3 state machine → PSO 化 1.5 PM」に含まれていた可能性、ただし a-3 段階 3 は **C++ state machine 側** が主、shader 側 SPIR-V 化 + descriptor 反映は a-3 で **未明示**。本 §2.5 で +4.05 PM を **新規可視化** (§2.2 2.32 + §2.4 1.73)。

#### shader 工数の C++ critical path との関係

§1.7 C++ critical path 7.77 PM は **shader 工数を含まない** (a-3 §5.4.1 段階 3 で「PSO 化」 = C++ state machine 廃止のみカウント、SPIR-V cross compile + descriptor 反映は別)。

→ **§1 + §2 合計 = 7.77 + 5.15 = ~12.92 PM** が C++ + shader 統合 base 値 (フルタイム dev、経験者前提、charter §4 (3) の本職並走 ratio + 学習曲線 適用前)。

§3 milestone 積算で本 §1 + §2 合計を milestone に振り分け、§5 で本職並走 ratio + Vulkan 学習曲線を適用、§6 で uncertainty band 評価。

#### shader 階層分布 確認

base 248 file の階層分布が §2.1 「A 階層 ~85% / B 階層 ~15%」と整合しているか確認:

| 階層 | file 数 | per-file 工数 (PM) | 合計 (PM) |
|---|---|---|---|
| A 素通り | 195 | 0.005 | 0.975 |
| B 要修正 | 53 | 0.025 | 1.325 |
| **base 248 合計** | **248** | — | **2.30 PM (≈ §2.2 算出 2.32)** |

整合 ✓。AYAstorm 改変 13 file は **C 階層** として §2.3 で別計上 (per-file 平均 0.085 PM)、合算 base + C = 248 + 13 = 261 file が §2.4 descriptor set 反映の対象。

---

## §3 per-milestone 工数積算 (r41 / r41.5 / r42 / r43 / r44 / r45+)

**status**: 次々 group で draft

draft 予定の項目:
- §3.1 r41 (GL 除去 + Vulkan 空転) work 工数 + 余裕係数 (04 doc §5.4.1 段階 1-5 base port 4-5 人月 を起点に AYA 並走化)
- §3.2 r41.5 (VK repo 分離) work 工数 + 余裕係数 (05 doc §10 skeleton 詳細化 + dynamic link 化 + license 分離手続)
- §3.3 r42-α (r21.1 picker port) work 工数 + 余裕係数
- §3.4 r42-β (r30 Cinematic port) work 工数 + 余裕係数
- §3.5 r42-γ (r14+ visual realism port) work 工数 + 余裕係数
- §3.6 r42-δ (parity 残機能 / vk-RC 直前 polish) work 工数 + 余裕係数
- §3.7 r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) work 工数 + 余裕係数
- §3.8 r45+ (visual realism 次世代 / ray tracing / HDR / GPU-driven 検討) 範囲外 (本算定では「予約のみ」、charter §6 で明示)
- §3.9 milestone 工数積算 sum (フルタイム dev 換算)

---

## §4 3 OS per-OS 増分

**status**: 次々 group で draft

draft 予定の項目:
- §4.1 Linux first-class baseline 工数 (§3 milestone work 工数の全体)
- §4.2 Win 追加 増分工数 (WSI win32 / driver matrix 対応 / Win-specific bug fix 余裕、05 doc §8.2)
- §4.3 Mac 追加 増分工数 (MoltenVK 経由 + portable subset 制約対応 + t-noami さん移植 workflow との連携、05 doc §8.3 + §9.4)
- §4.4 OS 別 milestone 着手 timing 反映 (charter §4 (2) Linux 先行 → r42-α Win 追加 → r42-β-γ-δ Mac 追加の段階対応)
- §4.5 3 OS 合計 工数 (フルタイム dev 換算、Linux × 1.0 + Win 増分 + Mac 増分)

---

## §5 各 milestone の所要月数 / 年数 (本職並走前提)

**status**: 次々々 group で draft

draft 予定の項目:
- §5.1 AYA 本職並走 ratio 確定 (フルタイム dev 1 人月 = AYA 並走 N 暦月、charter §4 (3) 想定 3-5x の精緻化)
- §5.2 Vulkan 学習曲線 反映 (r41 序盤 + r41.5 で +20-30% / r42+ 以降は inline 化想定)
- §5.3 milestone 別 所要暦月 (r41 / r41.5 / r42-α/β/γ/δ / r43 / r44) 中央値
- §5.4 milestone 累積 所要暦月 (r41 達成までの year scale + r45+ までの total year scale)
- §5.5 charter §4 (3) 6-15 年想定との整合性 verification
- §5.6 marker 暦年 (例: r41 達成は 202X 年頃、r42-γ は 202Y 年頃) — 撤退条件には使わない (charter §8 (A))、進捗 marker のみ

---

## §6 算定の uncertainty band (上方 / 下方)

**status**: 次々々 group で draft

draft 予定の項目:
- §6.1 不確実性要因の分類 (体制変動 / 技術選定 drift / 外部 dependency / scope creep / personal life event)
- §6.2 各要因の振れ幅 (中央値 ±%)
- §6.3 milestone 別 uncertainty band (r41 = ±N% / r42-α/β/γ = ±M% / r45+ = ±K%)
- §6.4 累積 uncertainty band (r41 達成までの band / r42-δ までの band)
- §6.5 上方 (最悪) / 中央 / 下方 (最良) の 3 シナリオ tabulation
- §6.6 band を縮める方策 (sub-milestone 区切り強化 / 早期 prototype / 並走外注検討の閾値)

---

## §7 Doom / Blender 参照点との比較

**status**: 最終 group で draft

draft 予定の項目:
- §7.1 Doom 2016 (id Tech 6) との比較 (体制 3 名経験者 + clean abstraction × 6-12 か月 = 実工数 18-36 人月)、AYAstorm 換算と本算定の差分根拠
- §7.2 Blender Vulkan との比較 (rotating contributor + 7 年未完)、AYAstorm 1 人体制との差分根拠
- §7.3 charter §4 (3) 6-15 人年 (本職並走で 15-30 年) 想定の妥当性 cross check
- §7.4 本算定中央値 vs 参照点中央値 のズレ要因分析
- §7.5 参照点に無い AYAstorm 固有要因 (撮影章用途 / AYAstorm 機能 13 file 追加 / Mac t-noami さん workflow / 等)

---

## §8 plan B trigger 条件 (charter §8 (B) 工程プラン破綻判定の閾値設定)

**status**: 最終 group で draft

draft 予定の項目:
- §8.1 charter §8 (A) 外部条件 trigger (LL Vulkan 先着地 / AYA life plan 変更) は本 §8 範囲外、charter §8 (B) 工程プラン破綻のみ本 §8 で扱う
- §8.2 plan B trigger 閾値の定義方針 (絶対月数 / charter 想定との乖離 % / 1 milestone 過度遅延 / 累積遅延 / sub-milestone 完遂率 / 等)
- §8.3 r41 達成までの trigger 閾値 (uncertainty band §6 upper bound を超えた場合の対処)
- §8.4 r42-α/β/γ 達成までの trigger 閾値 (機能 milestone 単位の遅延判定)
- §8.5 trigger 発火時の対処 (scope 縮小 / quality 緩和 / 別 viewer base 接続 (§10.4 defensibility 活用) / 撤退)
- §8.6 trigger 判定 cadence (年次 review / milestone 完遂時 / etc.)

---

## draft 進行状況の整理

### group 分け案 (本 doc 起草の進行方針)

work item (b) と同様に group 分けで進行、各 group が密接に絡む section をまとめる:

| group | 含む section | 主眼 | 想定行数 |
|---|---|---|---|
| **foundation** | §1 per-file 工数 + §2 per-shader 工数 | 棚卸し → per-unit 工数の確定 | ~400 行 |
| **group A** | §3 per-milestone 積算 + §4 3 OS 増分 | milestone 構造 + OS 増分 (per-unit を組合せ) | ~400 行 |
| **group B** | §5 milestone 月数 + §6 uncertainty band | time 軸変換 (人月 → 暦月 + 振れ幅) | ~400 行 |
| **group C** | §7 Doom Blender 比較 + §8 plan B trigger | validation + 撤退条件 (中央値を外部基準で検証) | ~300 行 |

### 各 group の前提依存

- **foundation**: 04 doc + 05 doc を直接参照、独立 draft 可
- **group A**: foundation の per-file/per-shader 工数を base に milestone と OS で集約、foundation 必須
- **group B**: group A の milestone 別 work 工数 + 本職並走 ratio + 学習曲線、foundation + group A 必須
- **group C**: 全 §1-§6 確定後に validation、最後

### 次 step

1. ~~本 doc skeleton + 算定方針 の AYA review~~ ✓ 完了
2. ~~foundation group (§1 per-file + §2 per-shader) draft 着手~~ ✓ 完了 (§1 = 7.77 PM / §2 = 5.15 PM / 合計 12.92 PM フルタイム dev)
3. **foundation group review → group A (§3 per-milestone + §4 3 OS) draft 着手** ← 次
4. group B (§5 月数 + §6 uncertainty) draft
5. group C (§7 Doom Blender + §8 plan B) draft
6. 8 section 揃ったら work item (c) 完了宣言、work item (d) r42+ 区切り確定 着手

### foundation group 算出値 (group A 以降の base 値)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §1.7 C++ critical path 合計 | **7.77 PM** | フルタイム dev | §3 milestone に振り分け |
| §2.5 shader 合計 | **5.15 PM** | フルタイム dev | §3 milestone に振り分け |
| **foundation 合計** | **~12.92 PM** | フルタイム dev | §5 で本職並走 ratio + 学習曲線適用、§6 で uncertainty band |

---

## 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§4 (3) 参照点 + 本職並走 想定 / §8 (B) plan B 閾値の元定義)
- `03-sub-phase-3-vulkan-plan.md` — work item (c) 親 doc (§5 算定軸 8 項目 + 進め方)
- `04-portage-inventory.md` — work item (a) 全完了 (§6 棚卸し総括 + §5.4 段階 port 戦略 5 段階 / AYAstorm 3 機能合成順 = §1/§3 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (全 §1-§10、§2 cross compile chain = §2 input / §8 OS 別 = §4 input / §10 skeleton = §3.2 r41.5 input)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (work item (b) 完了 + (c) 着手で更新済)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 (c)(d)(e) 完了後に着手)
- `feedback_proactive_handoff.md` — group 境界 handoff の根拠
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace の根拠
