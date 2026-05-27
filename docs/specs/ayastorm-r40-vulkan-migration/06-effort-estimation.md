# r40 sub-phase 3 work item (c): 工程算定

**status**: **確定 (AYA review PASS 2026-05-28)** — work item (c) 全 §1-§8 完成 → work item (d) r42+ 区切り確定 着手 (`07-r42-plus-milestone-mapping.md`)
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (c)
**前置 doc**:
- `04-portage-inventory.md` (work item (a)) — per-file 工数の input source
- `05-vulkan-api-design.md` (work item (b)) — per-section 設計から派生する工数算定 input
- `00-charter.md` §4 (3) — 参照点 (Doom / Blender) + 6-15 人年 + 本職並走 ratio
- `00-charter.md` §8 (B) — plan B trigger 条件の閾値設定対象
**達成条件**: §1-§8 全 section draft 完成 + AYA review PASS → work item (d) r42+ 区切り確定 着手 — **達成 (2026-05-28)**

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

### §3.0 振り分け方針

foundation §1+§2 算出の **12.92 PM (フルタイム dev)** を milestone に振り分け、a-3 §B.1-§B.3 で foundation 外計上指示のあった「AYAstorm 3 機能 C++ + テスト残分」と、charter §6 で確定済の r41.5 / r42-δ / r43-r44 milestone 新規 work を追加計上、最後に余裕係数 +30-50% を **per-milestone 適用**。

#### foundation 12.92 PM の milestone 帰属

foundation 算出値は per-file / per-shader 単位で領域別に積み上げ、milestone 区切りで分解すると以下の通り:

| 出処 | 工数 (PM) | milestone 帰属 |
|---|---|---|
| §1.2 llrender 51 file | 3.53 | r41 (段階 1-5 全領域 GL 除去対象) |
| §1.3 pipeline.cpp + .h | 1.05 | r41 (3 大グローバル frame context 化、最難関) |
| §1.4 lldrawpool 13 file | 0.97 | r41 (段階 2、Vulkan command buffer 化 base) |
| §1.5 llspatialpartition | 0.50 | r41 (段階 5、occlusion query Vulkan 化) |
| §1.5 llviewershadermgr | 0.45 | r41 (段階 5、shader manager Vulkan 化) |
| §1.5 llvosky + llvowlsky | 0.27 | **r42-γ** (r14+ visual realism 基盤、a-3 §B.2) |
| §1.6 wrapper + platform-specific | 1.00 | r41 (段階 1 + 例外 10 file) |
| §2.2 base 248 file SPIR-V 化 | 2.32 | r41 (build integration + cross compile main pass) |
| §2.3 AYAstorm picker shader 2 file | 0.10 | **r42-α** (a-3 §B.1) |
| §2.3 AYAstorm Cinematic shader 4 file | 0.40 | **r42-β** (a-3 §B.3) |
| §2.3 AYAstorm visual realism shader 7 file | 0.60 | **r42-γ** (a-3 §B.2) |
| §2.4 descriptor set 反映 (script 整備 + 全 261 file 波及) | 1.73 | r41 (script 整備 + 全 file 一括 layout 注入、AYAstorm 改変 shader は §2.3 工数内で smoke test 込み) |
| **foundation 合計** | **12.92** | r41 11.55 + r42-α 0.10 + r42-β 0.40 + r42-γ 0.87 |

#### a-3 §B.1-§B.3 で foundation 外計上指示済 work

foundation §2.3 注 (06 doc §2.3 末尾) で「shader 部分のみ抽出、C++ + テスト工数は §3 milestone で別計上」と明示済の残分:

| 出処 | 残分 work | 工数 (PM) | milestone |
|---|---|---|---|
| a-3 §B.1 picker 0.5 PM | pipeline.cpp 4 LOC 0.05 + render pass attachment 設計 0.20 + read-pick テスト 0.15 | **0.40** | r42-α |
| a-3 §B.3 Cinematic 2-3 PM 中央値 2.5 | pipeline.cpp 6 分岐 frame context bleed 0.20 + DoF state enum 化 0.50 + visual quality テスト 1.0 + 余 0.15 | **1.85** | r42-β |
| a-3 §B.2 visual realism 2-3 PM 中央値 2.5 | post-process descriptor set 整備 (05 doc §3) 0.30 + pipeline.cpp post-process chain 5 LOC 0.10 + performance profile 0.50 + visual A/B 0.50 | **1.40** | r42-γ |

a-3 §B.1-§B.3 中央値合計 5.5 PM = foundation 帰属 (0.10 + 0.40 + 0.87) + 追加 (0.40 + 1.85 + 1.40) = 1.37 + 3.65 = 5.02 PM (整合 ≈、a-3 中央値内)。

#### a-3 範囲外の新規 milestone work

a-3 段階 port + §B.x AYAstorm 3 機能 だけでカバーされない milestone (charter §6 で確定済) の新規算定:

| milestone | 内容 | 工数 (PM) |
|---|---|---|
| r41.5 (VK repo 分離) | LLVKRenderer interface 詳細化 (05 doc §10.2) 0.30 + directory 移動 + dynamic link 化 0.20 + 本線側 header / build 整備 0.20 + license 分離手続 0.30 | **1.00** |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | r25-r29 3D stream Vulkan 描画 stage 接続 0.50 + r1-r13 audio 系の Vulkan 非依存確認 0.30 + vk-RC 直前 regression sweep + 残機能 polish 0.70 | **1.50** |
| r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) | Mac MoltenVK 詳細化 (05 doc §8.3 / §9.4 portable subset check + t-noami さん workflow 連携) 1.00 + Win driver matrix 対応 (05 doc §8.2) 0.50 + 性能 polish (frame in flight tuning / barrier sequence / VMA strategy) 0.50 | **2.00** |
| **a-3 範囲外 新規 合計** | — | **4.50** |

§4 (3 OS 増分) と一部 overlap する Mac/Win 部分は §4 で重複排除予定 (本 §3 はあくまで milestone 軸、§4 は OS 軸)。

#### 余裕係数の per-milestone 適用方針

charter §4 (3) + 03 doc §5 算定軸 5 = work 工数 + **30-50%** 余裕。本 §3 では milestone 性質別に分配:

| milestone | 余裕係数 | 根拠 |
|---|---|---|
| r41 | +40% | 新領域 (Vulkan 初見) + 段階 4 設計議論 1-2 月 (a-3 §5.4.1 段階 4 注記) |
| r41.5 | +50% | license 設計議論 + 初分離 unknown (proprietary / permissive 選択 + 法的 review) |
| r42-α | +30% | touchpoint 局在 + AYAstorm 既存実装あり、低 risk |
| r42-β | +40% | DoF state enum 化 + visual quality verify (live A/B 必須) |
| r42-γ | +40% | post-process descriptor 整備 + perf profile (sky/atmospherics は r14+ 章再演) |
| r42-δ | +50% | vk-RC 直前 unknown (parity 残機能の発掘 cost) |
| r43-r44 | +50% | Mac t-noami workflow 不確定性 + 性能 polish の iterate |

平均余裕 ≈ +43% (charter §4 (3) 30-50% 範囲内、後半 milestone に重み付け)。本 §3 は **中央値想定**、§6 で uncertainty band の上下振れを別途反映。

### §3.1 r41 (GL 除去 + Vulkan 空転) work 工数

#### r41 達成基準 (charter §5)

- C++ critical path 約 63K LOC + GLSL shader 248 file の完全置換
- Vulkan 空転 (描画は最低限、segfault せず frame loop が回る + 何らかの描画が出る)
- AYAstorm 固有機能の port は r42+ で行う、r41 では parity 不要

#### foundation 帰属 (11.55 PM)

| 領域 | 工数 (PM) |
|---|---|
| §1.2 llrender 51 file | 3.53 |
| §1.3 pipeline.cpp + .h | 1.05 |
| §1.4 lldrawpool 13 file | 0.97 |
| §1.5 llspatialpartition | 0.50 |
| §1.5 llviewershadermgr | 0.45 |
| §1.6 wrapper + platform-specific 10 file | 1.00 |
| §2.2 base 248 file SPIR-V 化 | 2.32 |
| §2.4 descriptor set 反映 (script 整備 + 全 261 file layout 注入 + smoke test) | 1.73 |
| **r41 foundation 帰属** | **11.55** |

#### 追加 work

なし (a-3 段階 1-5 = foundation §1 + §2 で全カバー、r41 で foundation 外追加 work は計上しない)。

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (foundation 帰属) | 11.55 |
| 余裕係数 +40% (新領域 + 段階 4 設計議論) | +4.62 |
| **r41 total (フルタイム dev 換算)** | **~16.17** |

### §3.2 r41.5 (VK repo 分離) work 工数

#### r41.5 達成基準 (charter §6 + 05 doc §10)

- Vulkan code の abstraction 化 (本線 ↔ VK repo 間の API surface = C++ pure virtual + C ABI entry point、05 doc §10.2 skeleton 詳細化)
- AYAstorm VK repo の新規立ち上げ (GitHub 二次 fork 制約に依らない完全独立 git init)
- 物理分離 = directory 単位移動 (Vulkan layer を本線 `ayastorm-release` から VK repo へ)
- dynamic link 構成 (本線 LGPL ↔ VK repo 独自 license、05 doc §10.3 で license 境界の合法性確定済)
- ビルド統合 (本線 build script から VK repo を fetch + build + link)

描画 stage は進行しない構造 refactor milestone (charter §6 r42+ ロードマップ表 r41.5 「描画 stage 進行なし」)。

#### foundation 帰属

なし (foundation §1+§2 は r41 までで完結、r41.5 構造 refactor は a-3 / foundation 範囲外)。

#### 追加 work

| 項目 | 工数 (PM) |
|---|---|
| LLVKRenderer interface 詳細化 (05 doc §10.2 skeleton から実装定義、handle opaque 化 + Vulkan header 非露出) | 0.30 |
| directory 単位移動 + dynamic link 化 (`.so` / `.dll` / `.dylib` 構成、05 doc §10.3) | 0.20 |
| 本線側 header / build script 整備 (autobuild 連携、3 OS で dlopen / LoadLibrary 経路化) | 0.20 |
| license 分離手続 (legal review + repo init + 初版 release 整備 + LGPL ↔ 独自 license 境界の社内 audit) | 0.30 |
| **r41.5 追加 work 合計** | **1.00** |

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (追加) | 1.00 |
| 余裕係数 +50% (license 設計議論 + 初分離 unknown) | +0.50 |
| **r41.5 total (フルタイム dev 換算)** | **~1.50** |

注: r41.5 は r41 達成後の構造 refactor、描画 stage 進行なしのため visual regression risk は低く、余裕係数は license + 法的 review の不確定性が支配的。

### §3.3 r42-α (r21.1 self-rigged picker port) work 工数

#### r42-α 達成基準 (a-4 §6.3.2 + a-3 §B.1)

- mObjectIDBuffer (gbuffer3 inline attachment) を Vulkan render pass attachment に統合 (05 doc §3.4 + §4.5)
- picker shader 2 file (`fsObjectIDV.glsl` + `fsObjectIDF.glsl`) SPIR-V 化
- read-pick (CPU 側 ObjectID readback) の Vulkan staging buffer + transfer queue 経路化
- 単一回 click → ObjectID 取得 (existing AYAstorm r21.1 機能 parity)

#### foundation 帰属 (0.10 PM)

| 領域 | 工数 (PM) |
|---|---|
| §2.3 picker shader 2 file (fsObjectIDV/F.glsl SPIR-V 化) | 0.10 |
| **r42-α foundation 帰属** | **0.10** |

#### 追加 work (a-3 §B.1 残分)

| 項目 | 工数 (PM) |
|---|---|
| pipeline.cpp 4 LOC (mObjectIDBuffer setup → Vulkan attachment binding) | 0.05 |
| render pass attachment 設計 + inline 統合 (05 doc §4.5、deferred main pass 内 inline) | 0.20 |
| read-pick テスト (single click + drag select、existing AYAstorm test 流用) | 0.15 |
| **r42-α 追加 work 合計** | **0.40** |

a-3 §B.1 工数感 0.5 PM 合計 ≈ foundation 0.10 + 追加 0.40 = 0.50 PM、整合 ✓。

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (foundation + 追加) | 0.50 |
| 余裕係数 +30% (touchpoint 局在 + 既存実装あり、低 risk) | +0.15 |
| **r42-α total (フルタイム dev 換算)** | **~0.65** |

### §3.4 r42-β (r30 Cinematic mode port) work 工数

#### r42-β 達成基準 (a-4 §6.3.2 + a-3 §B.3)

- DoF state enum 化 (現 pipeline.cpp 6 分岐 → frame context 統合)
- Cinematic 関連 shader 4 file (volumetricLightF class1/class3 + screenSpaceReflUtil class3 + DoF 関連 1 file) SPIR-V 化
- visual quality テスト (r30 Cinematic Controls 13 件 BD live cvar との live A/B、既存 release branch `experiment/r30-bd-improvement-cinematic-optin` ベース)
- AYAstorm View (mode==2 = Cinematic) の parity 完遂

#### foundation 帰属 (0.40 PM)

| 領域 | 工数 (PM) |
|---|---|
| §2.3 Cinematic shader 4 file (volumetricLightF×2 + screenSpaceReflUtil + DoF) | 0.40 |
| **r42-β foundation 帰属** | **0.40** |

#### 追加 work (a-3 §B.3 残分)

| 項目 | 工数 (PM) |
|---|---|
| pipeline.cpp 6 分岐 frame context bleed (Cinematic 関連 DoF mode 分岐の LLPipelineFrameContext 統合) | 0.20 |
| DoF state enum 化 (現 hardcoded → enum class + frame context 経由配信) | 0.50 |
| visual quality テスト (BD live cvar 13 件 + Cinematic Controls の visual A/B、live screenshot 比較) | 1.00 |
| 余 (regression sweep / 残細部 polish) | 0.15 |
| **r42-β 追加 work 合計** | **1.85** |

a-3 §B.3 工数感 2-3 PM 中央値 2.5 PM ≈ foundation 0.40 + 追加 1.85 = 2.25 PM、整合 ✓ (中央値内、適切)。

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (foundation + 追加) | 2.25 |
| 余裕係数 +40% (DoF state enum 化 + visual quality verify) | +0.90 |
| **r42-β total (フルタイム dev 換算)** | **~3.15** |

### §3.5 r42-γ (r14+ visual realism port) work 工数

#### r42-γ 達成基準 (a-4 §6.3.2 + a-3 §B.2)

- post-process pass chain 統合 (05 doc §4.4 r14+ post-process 7 sub-pass の Vulkan render pass chain 化)
- visual realism 関連 shader 7 file (vignette / tone map / godrays / bloom / glow / atmospherics 関連) SPIR-V 化
- llvosky + llvowlsky (2.2K LOC) の sky dome + atmospherics Vulkan 化
- performance profile (post-process pass chain の per-pass cost 計測)
- visual A/B (r14+ visual realism の既存 AYAstorm 実装と Vulkan port の visual 同等性確認)

#### foundation 帰属 (0.87 PM)

| 領域 | 工数 (PM) |
|---|---|
| §1.5 llvosky + llvowlsky (r14+ 基盤、sky dome + atmospherics) | 0.27 |
| §2.3 visual realism shader 7 file | 0.60 |
| **r42-γ foundation 帰属** | **0.87** |

#### 追加 work (a-3 §B.2 残分)

| 項目 | 工数 (PM) |
|---|---|
| post-process descriptor set 整備 (05 doc §3 7 sub-pass 分の sampler binding 設計) | 0.30 |
| pipeline.cpp post-process chain 5 LOC (post-process pass dispatch の Vulkan render pass chain 接続) | 0.10 |
| performance profile (post-process per-pass cost、`VK_EXT_calibrated_timestamps` 経由) | 0.50 |
| visual A/B (godrays / volumetricLight / vignette / scene buffer alpha invariant 確認、memory `project_aya_visual_realism_alpha_protect.md`) | 0.50 |
| **r42-γ 追加 work 合計** | **1.40** |

a-3 §B.2 工数感 2-3 PM 中央値 2.5 PM ≈ foundation 0.87 + 追加 1.40 = 2.27 PM、整合 ✓ (中央値内、適切)。

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (foundation + 追加) | 2.27 |
| 余裕係数 +40% (post-process descriptor 整備 + perf profile + visual A/B の iterate) | +0.91 |
| **r42-γ total (フルタイム dev 換算)** | **~3.18** |

### §3.6 r42-δ (parity 残機能 / vk-RC 直前 polish) work 工数

#### r42-δ 達成基準 (charter §6 仮 line up + a-4 §6.3.2 補完)

- r25-r29 3D stream (audio 系) の Vulkan 描画 stage 接続確認 (描画 stage 軽依存だが Vulkan 化で接続 cleanup 必要)
- r1-r13 audio 系の Vulkan 非依存確認 (FMOD + Dullahan callback path が Vulkan に invariant、charter §6 仮 line up で audio は r42 並行 port)
- vk-RC 直前 regression sweep (r41 / r41.5 / r42-α/β/γ で発見の bug の集中 polish)
- AYAstorm r1-r30 全機能 parity 完遂 (charter §4 (1) 完遂 goal の前段、Linux/Win baseline)

#### foundation 帰属

なし (foundation §1+§2 は r41 + AYAstorm 3 機能 で完結、r42-δ parity 残機能 は a-3 範囲外)。

#### 追加 work

| 項目 | 工数 (PM) |
|---|---|
| r25-r29 3D stream Vulkan 描画 stage 接続 (NDI / OBS 等の Vulkan-side hook 検討、05 doc §9.2 `VK_KHR_external_memory_*` 予約のみ採用判断) | 0.50 |
| r1-r13 audio 系 Vulkan 非依存確認 (FMOD callback + Dullahan path の Vulkan-agnostic invariant 検証) | 0.30 |
| vk-RC 直前 regression sweep (r41-r42-γ 残 bug 集中 fix、parity 残機能 polish) | 0.70 |
| **r42-δ 追加 work 合計** | **1.50** |

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (追加) | 1.50 |
| 余裕係数 +50% (vk-RC 直前 unknown + parity 残機能の発掘 cost) | +0.75 |
| **r42-δ total (フルタイム dev 換算)** | **~2.25** |

### §3.7 r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) work 工数

#### r43-r44 達成基準 (charter §6 仮 line up + 05 doc §8.3 / §9.4)

- Mac MoltenVK portable subset 詳細化 (05 doc §9.4 で vk-RC 直前 phase 詳細化指示済、本 milestone で実施)
- Win driver matrix 対応 (05 doc §8.2 NVIDIA / AMD / Intel Arc 完全動作、`VK_EXT_swapchain_maintenance1` Intel Arc 一部未対応 fallback 等)
- 性能 polish (frame in flight tuning / barrier sequence / VMA allocation strategy の本格 tuning、05 doc §5 + §6)
- 3 OS parity 完遂 (charter §4 (1) vk-RC 相当、AYAstorm r1-r30 全機能 Linux + Win + Mac 全部で Vulkan 上に再現)

#### foundation 帰属

なし (foundation は Linux baseline、Mac/Win 増分は §4 で OS 軸別途算出、本 §3.7 では milestone 軸の追加 work を計上)。

#### 追加 work

| 項目 | 工数 (PM) |
|---|---|
| Mac MoltenVK 詳細化 (portable subset check + t-noami さん workflow 連携、05 doc §8.3 + §9.4) | 1.00 |
| Win driver matrix 対応 (NVIDIA / AMD / Intel Arc 全 driver 動作確認 + 旧 driver fallback、05 doc §8.2) | 0.50 |
| 性能 polish (frame in flight tuning / barrier sequence / VMA allocation strategy 微調整) | 0.50 |
| **r43-r44 追加 work 合計** | **2.00** |

注: Mac / Win 増分の OS 軸別積算は §4 で重複排除しつつ別途算出、本 §3.7 は **milestone 軸の Linux baseline 上の追加 polish 工数** (3 OS 増分の OS 軸算定は §4)。

#### work 工数 + 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| base work (追加) | 2.00 |
| 余裕係数 +50% (Mac t-noami workflow 不確定性 + 性能 polish の iterate) | +1.00 |
| **r43-r44 total (フルタイム dev 換算)** | **~3.00** |

### §3.8 r45+ (visual realism 次世代 / ray tracing / HDR / GPU-driven) — 本算定範囲外

#### charter §6 + 05 doc §9.5 の位置付け

- vk-RC parity 完遂 (= r44 達成) **後**、AYAstorm visual realism 次世代の自由扱い章
- 05 doc §9.5 で `VK_KHR_ray_tracing_pipeline` + `VK_KHR_acceleration_structure` + `VK_EXT_mesh_shader` を将来 extension として **予約のみ採用、本 design 範囲外**
- charter §3 「無期限 / AYA life plan」前提で、r45+ work 工数は本算定 (work item (c)) では算定しない

#### 本算定範囲外の根拠

- charter §3 「時間軸では撤退条件を設けない」 → r45+ 着手 timing は r44 達成後の AYAstorm 体制 / industry 状況 / LL 着地 status 次第で判断 (charter §7 LL 着地時判断指針 + §8 plan B trigger 評価対象)
- 本算定の目的 = r40 達成 (= 工程プラン完成) 時点で r41-r44 の vk-RC 完遂までの算定、r45+ は parity 完遂後の self-driven 章として独立算定 (将来別 work item or 別章 charter)

### §3.9 milestone 工数積算 sum (フルタイム dev 換算)

#### milestone 別 work 工数 sum (余裕係数前 + 適用後)

| milestone | foundation 帰属 (PM) | 追加 work (PM) | base work (PM) | 余裕係数 | total (PM) |
|---|---|---|---|---|---|
| r41 (GL 除去 + Vulkan 空転) | 11.55 | 0 | 11.55 | +40% | **16.17** |
| r41.5 (VK repo 分離) | 0 | 1.00 | 1.00 | +50% | **1.50** |
| r42-α (r21.1 picker port) | 0.10 | 0.40 | 0.50 | +30% | **0.65** |
| r42-β (r30 Cinematic port) | 0.40 | 1.85 | 2.25 | +40% | **3.15** |
| r42-γ (r14+ visual realism port) | 0.87 | 1.40 | 2.27 | +40% | **3.18** |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | 0 | 1.50 | 1.50 | +50% | **2.25** |
| r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) | 0 | 2.00 | 2.00 | +50% | **3.00** |
| **§3.9 sum (Linux baseline, vk-RC parity 完遂まで)** | **12.92** | **8.15** | **21.07** | — | **~29.90** |
| r45+ (visual realism 次世代) | (本算定範囲外、charter §3 / §6) | — | — | — | — |

#### a-3 工数感との突合

| 算定 | 工数 (PM、フルタイム dev) |
|---|---|
| a-3 §5.4.1 段階 1-5 (base port、r41 相当) 中央値 | 4.5 |
| a-3 §5.4.2 AYAstorm 3 機能 (r42-α/β/γ) 中央値 | 3.0 |
| a-3 合計 | **7.5** |
| 本 §3 r41 + r42-α/β/γ (余裕係数前 base work) | 11.55 + 0.50 + 2.25 + 2.27 = **16.57** |
| 本 §3 r41 + r42-α/β/γ (余裕係数適用後) | 16.17 + 0.65 + 3.15 + 3.18 = **23.15** |
| 差 (base) | +9.07 PM (+121%) |
| 差 (余裕係数込) | +15.65 PM (+209%) |

差の主因:
1. **per-file 精緻化** (foundation §1.7 の +3.27 PM、§1.7 で説明) → +3.27 PM
2. **shader 工数の新規可視化** (foundation §2.5 の +4.05 PM、a-3 で未明示の SPIR-V cross compile + descriptor set 反映) → +4.05 PM
3. **AYAstorm 3 機能の C++ + テスト残分** (a-3 §B.x で foundation 外計上指示済、§3.0 で取込) → 0.40 + 1.85 + 1.40 = +3.65 PM (a-3 §B.x 内含は元から計上のため重複に見えるが、foundation §2.3 は shader 部分のみ抽出のため §3 で C++ 部分が新規に表面化)
4. **余裕係数 +30-50% per milestone 適用** → +6.58 PM (差の余裕係数込 - 差 base = 15.65 - 9.07 = 6.58、本 §3 で初適用、a-3 は work 工数のみ提示)

合計差 ≈ 3.27 + 4.05 + 3.65 = 10.97 PM (base) + 6.58 (余裕係数) = 17.55 PM、概算 ≈ 15.65 PM と整合 (差は r42-α/β/γ の foundation 帰属 + 追加 の per-milestone 配分微差)。

#### a-3 範囲外の新規 milestone 分

r41.5 + r42-δ + r43-r44 = 1.00 + 1.50 + 2.00 = 4.50 PM (work 工数) → 余裕係数適用後 = 1.50 + 2.25 + 3.00 = 6.75 PM。

a-3 段階 port + AYAstorm 3 機能では未明示の milestone で、charter §6 仮 line up + 05 doc §8/§10 で本算定段階に新規導入。

#### r45+ 範囲外の宣言

§3.8 の通り、r45+ visual realism 次世代は本算定範囲外。vk-RC parity 完遂 (= r44 達成) までの 29.90 PM (フルタイム dev、Linux baseline) が本 §3 算出範囲。

#### 本算定 base 値の確定

**§3.9 sum 21.07 PM (work 工数、foundation 12.92 + 追加 8.15) / 29.90 PM (余裕係数適用後、平均 +42%) を本算定の milestone work 値 (Linux baseline、フルタイム dev、本職並走 ratio + 学習曲線 適用前) として採用**。§4 で 3 OS 増分、§5 で本職並走 ratio + 学習曲線、§6 で uncertainty band 適用予定。

---

## §4 3 OS per-OS 増分

### §4.0 算定方針

§3 milestone 工数は **Linux baseline** (charter §4 (2) Linux 先行 + AYAstorm 開発機 = AMD/Linux baseline)。本 §4 で Win / Mac の **増分工数** を OS 軸で算出。

#### 3 OS 大前提と Linux 先行の整合 (memory `project_ayastorm_three_platforms`)

- AYAstorm は 3 OS 完遂が大前提、Linux 単独判断は明示指示が無い限り取らない
- charter §4 (2) で「Linux 先行 → Win/Mac 後追い」が明示指示の例外、3 OS 完遂自体は維持
- 段階対応 = r41 = Linux only OK / r42-α 以降 Win 追加 / r42-β-γ-δ で Mac 追加 (a-4 §6.3.2 + 05 doc §8.5)
- vk-RC parity 完遂 (= r44 達成) 時点で 3 OS parity を達成

#### 増分の OS 軸算定方針

- Linux baseline = §3 milestone work 全体 (foundation §1+§2 12.92 PM + a-3 §B.x 残分 3.65 PM + a-3 範囲外新規 4.50 PM = base 21.07 PM、余裕係数 +42% 適用後 ≈ 29.90 PM)
- Win 増分 = Linux baseline に対する +Δ (driver matrix / WSI win32 / 旧 driver fallback 等、05 doc §8.2)
- Mac 増分 = Linux baseline に対する +Δ (MoltenVK 経由 / portable subset 制約対応 / t-noami さん workflow 連携、05 doc §8.3 + §9.4)
- §3.7 r43-r44 milestone work 内で **「Mac MoltenVK 詳細化 1.00 + Win driver matrix 0.50」** は milestone 軸で算出済、本 §4 で OS 軸別途算出する分は **重複排除済の純増分**

### §4.1 Linux first-class baseline 工数

#### Linux baseline 確定 (charter §4 (2) + 05 doc §8.1)

- 開発機 = AMD RX 7900 XTX + Mesa RADV (本線検証 baseline、05 doc §8.1 Linux driver capability matrix)
- Vulkan 1.3 default、Mesa 22.x 以降 + NVIDIA proprietary 525+ で 1.3 安定 (05 doc §8.1)
- WSI = `VK_KHR_xcb_surface` / `VK_KHR_wayland_surface` (LLWindow Linux implementation の X11/Wayland backend に応じて分岐、05 doc §8.4)
- driver coverage = Mesa RADV (first-class) + Mesa ANV (first-class) + NVIDIA proprietary (first-class) + AMDGPU-PRO (second-class) + LLVMpipe (non-goal)

#### Linux baseline 工数

§3.9 sum で算出済:

| 項目 | 工数 (PM) |
|---|---|
| Linux baseline base work (foundation + 追加) | 21.07 |
| Linux baseline 余裕係数適用後 (per-milestone、平均 +42%) | 29.90 |

注: §3 で算出した work 工数 全体が Linux baseline。Win / Mac 増分は別途 §4.2 / §4.3 で OS 軸算出。§3.7 r43-r44 milestone 内で「Mac MoltenVK 詳細化 1.00 + Win driver matrix 0.50」を計上済のため、本 §4.2 / §4.3 では r43-r44 milestone 計上分以外の **純増分** のみ算出 (重複排除)。

### §4.2 Win 追加 増分工数

#### Win 追加の Vulkan 設計反映 (05 doc §8.2 + §8.4)

- driver coverage = NVIDIA GeForce/Quadro (1.3 full、RTX 20/30/40 系 stable) + AMD Radeon Software (1.3 full、RDNA 1/2/3 stable) + Intel ARC/Iris Xe (1.3、`VK_EXT_swapchain_maintenance1` 一部 driver 未対応)
- WSI = `VK_KHR_win32_surface` (LLWindow Win32 implementation の HWND 流用、現 GL WGL 経由は廃止)
- ICD registry は driver installer が登録、loader (volk) が自動列挙
- LunarG SDK Win 版 (Linux と path 構造の差分、autobuild Win 統合)
- WHCK (Windows Hardware Compatibility Kit) Vulkan logo program 経由 driver 認定 minimum 版数の release note 整備

#### Win 増分 work 内訳

| 項目 | 工数 (PM) | 出処 |
|---|---|---|
| LLWindow Win32 surface 化 (WGL 廃止 → `VK_KHR_win32_surface`) | 0.20 | 05 doc §8.2 + §8.4 |
| Win driver matrix 動作確認 (NVIDIA / AMD / Intel Arc) — §3.7 計上分超過 | 0.30 | 05 doc §8.2 (§3.7 r43-r44 で 0.50 計上済 → §4.2 では超過分の +0.30 のみ重複排除後) |
| 旧 driver fallback path 追加実装 (`VK_EXT_swapchain_maintenance1` Intel Arc 未対応 → `vkDeviceWaitIdle` fallback、05 doc §7.6) | 0.20 | 05 doc §7.6 + §8.2 |
| LunarG SDK Win 統合 (autobuild、path 構造差吸収) | 0.15 | 05 doc §1.3 |
| Win-specific bug fix 余裕 (driver-specific quirks、起動時 instance 初期化 timing 等) | 0.50 | (経験則、Linux baseline の +20-30% 想定) |
| **Win 増分 base work 合計** | **1.35** | — |

注: §3.7 r43-r44 milestone で Win driver matrix 0.50 計上済。本 §4.2 では「LLWindow Win32 surface 化 + 旧 driver fallback + LunarG SDK 統合 + Win-specific bug fix 余裕」の純増分 1.05 PM + r43-r44 内 driver matrix 超過分 0.30 PM = 1.35 PM。

#### Win 増分 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| Win 増分 base work | 1.35 |
| 余裕係数 +40% (Win-specific bug fix +20-30% / driver matrix の不確定性 +10% / WHCK 認定の手続 +10%) | +0.54 |
| **Win 増分 total (フルタイム dev 換算)** | **~1.89** |

### §4.3 Mac 追加 増分工数

#### Mac 追加の Vulkan 設計反映 (05 doc §8.3 + §9.4 + §8.4)

- MoltenVK 経由 (Vulkan 1.2 core + 一部 1.3 KHR portable subset、05 doc §8.3)
- Apple Silicon (M1/M2/M3) UMA → VMA `_AUTO_PREFER_HOST` で実質 ReBAR 相当 (05 doc §6.2 + §8.3)
- macOS 14+ minimum (Metal 3 minimum、MoltenVK 1.2.x 安定動作の前提、05 doc §8.3)
- WSI = `VK_EXT_metal_surface` 採用 (NSView → CAMetalLayer、05 doc §8.4)
- Mac で利用不可 / 制約のある機能 (05 doc §8.3): geometry shader 非対応 (shader 棚卸しゼロ確定で影響なし) / D24S8 → D32_SFLOAT_S8_UINT 内部置換 (MoltenVK 自動) / transfer queue unified queue 兼用 fallback / `VK_EXT_swapchain_maintenance1` 非対応 → `vkDeviceWaitIdle` fallback / ray tracing 非対応 (r45+ 範囲外)
- t-noami さん Mac 移植 workflow との連携 (Linux build 完成 → t-noami さん検証 → Mac 固有問題 patch return、memory `feedback_credit_t_noami_equal_billing`)

#### Mac 増分 work 内訳

| 項目 | 工数 (PM) | 出処 |
|---|---|---|
| LLWindow Mac surface 化 (CGL/AGL 廃止 → `VK_EXT_metal_surface`、NSView → CAMetalLayer) | 0.30 | 05 doc §8.4 |
| MoltenVK portable subset 対応 (本 design 利用 feature の MoltenVK 実装確認 + `VK_KHR_portability_subset` enable) — §3.7 計上分超過 | 0.40 | 05 doc §8.3 + §9.4 (§3.7 r43-r44 で 1.00 計上済 → §4.3 では超過分の +0.40 のみ重複排除後) |
| Apple Silicon UMA 対応 (VMA `_AUTO_PREFER_HOST` 動作確認、`VK_EXT_memory_budget` query 整合) | 0.20 | 05 doc §6.2 + §6.6 + §8.3 |
| MoltenVK 1.2 core fallback path (1.3 機能の 1.2 fallback コード追加、dynamic rendering / sync2 / push descriptor 等の MoltenVK 実装 path) | 0.40 | 05 doc §8.3 + §9.1-§9.3 |
| MSL (Metal Shading Language) 経由 shader 動作確認 (spirv-cross MSL 変換 + MoltenVK runtime、248 + 13 file の Mac 上 smoke test) | 0.50 | 05 doc §2 + §9.4 |
| t-noami さん移植 workflow 連携 (Linux build 完成 → 検証 → patch return cycle、本 §8.3 確定方針の事前共有 + review) | 0.30 | 05 doc §8.3 |
| Mac-specific bug fix 余裕 (macOS 14+ Metal 3 制約、MoltenVK 個別 quirk) | 0.60 | (経験則、Linux baseline の +30-40% 想定、t-noami さん workflow の cycle 待ち含む) |
| **Mac 増分 base work 合計** | **2.70** | — |

注: §3.7 r43-r44 milestone で Mac MoltenVK 詳細化 1.00 計上済。本 §4.3 では「LLWindow Mac surface 化 + UMA 対応 + 1.2 fallback path + MSL 動作確認 + t-noami workflow 連携 + Mac-specific bug fix 余裕」の純増分 2.30 PM + r43-r44 内 MoltenVK 詳細化超過分 0.40 PM = 2.70 PM。

#### Mac 増分 余裕係数

| 項目 | 工数 (PM) |
|---|---|
| Mac 増分 base work | 2.70 |
| 余裕係数 +50% (t-noami さん workflow 不確定性 + MoltenVK portable subset の untested feature + macOS 14+ minimum の動作確認 cycle) | +1.35 |
| **Mac 増分 total (フルタイム dev 換算)** | **~4.05** |

### §4.4 OS 別 milestone 着手 timing

charter §4 (2) + a-4 §6.3.2 + 05 doc §8.5 + 03 doc §3 進め方:

| milestone | Linux | Win | Mac |
|---|---|---|---|
| r41 (GL 除去 + Vulkan 空転) | first-class baseline (全 work) | (本線 GL build 維持、未着手) | (本線 GL build 維持、未着手) |
| r41.5 (VK repo 分離) | first-class baseline (構造 refactor) | (Win/Mac 着手前) | (Win/Mac 着手前) |
| r42-α (r21.1 picker port) | first-class | **Win 追加開始** (LLWindow Win32 surface 化 + driver matrix 着手) | (本線 GL 維持) |
| r42-β (r30 Cinematic port) | first-class | Win 並走 (driver matrix 継続) | **Mac portable subset check 開始** (t-noami さん事前共有、05 doc §8.3) |
| r42-γ (r14+ visual realism port) | first-class | Win 並走 | Mac 並走 (t-noami さん検証 cycle 始動 + MoltenVK 1.2 fallback path 整備) |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | first-class | Win polish (旧 driver fallback + WHCK 整備) | **Mac MoltenVK 詳細化** (05 doc §9.4 / §8.3 vk-RC 直前 phase) |
| r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) | first-class | Win parity 補強 | **Mac parity 完遂** (vk-RC 達成、3 OS 全 parity) |

#### Win / Mac 着手 timing の根拠

- **Win 着手 = r42-α**: r41 で Linux baseline 確定 + Vulkan 空転動作確認後、最初の AYAstorm 機能 (picker) port と並走で Win surface 化を着手。Win driver matrix は r42-α/β/γ で incremental に網羅、r42-δ で polish。
- **Mac 着手 = r42-β**: t-noami さん workflow の cycle を要するため Win より遅延、r42-β Cinematic port 開始時点で Mac portable subset check を t-noami さんに事前共有。r42-γ で並走確立、r42-δ で MoltenVK 詳細化、r43-r44 で parity 完遂。
- **3 OS parity 完遂 = r44 達成**: charter §4 (1) vk-RC 相当、AYAstorm r1-r30 全機能を 3 OS で Vulkan 上に再現。

### §4.5 3 OS 合計 工数 (フルタイム dev 換算)

#### OS 別 work 工数 sum (余裕係数前 + 適用後)

| OS | base work (PM) | 余裕係数 | total (PM) |
|---|---|---|---|
| Linux baseline (§3.9 sum) | 21.07 | 平均 +42% (per-milestone) | **29.90** |
| Win 増分 (§4.2) | 1.35 | +40% | **1.89** |
| Mac 増分 (§4.3) | 2.70 | +50% | **4.05** |
| **3 OS 合計** | **25.12** | — | **~35.84** |

#### a-3 / charter §4 (2) との突合

| 算定 | 工数 (PM、フルタイム dev) |
|---|---|
| a-3 §5.4 base port (4-5 PM) + AYAstorm 3 機能 (3 PM) | **7.5** (Linux のみ表現) |
| charter §4 (2) 「Linux 先行 → Win/Mac 後追い」 | (per-OS 増分明示なし、3 OS 完遂のみ) |
| 本 §4.5 3 OS 合計 (余裕係数適用後) | **~35.84** |

差の主因:
1. **Linux baseline の per-file 精緻化** (§1.7 + §2.5 + §3.0 追加 + r41.5/r42-δ/r43-r44 新規 milestone) → 29.90 PM
2. **Win 増分の OS 軸新規可視化** (a-3 / charter §4 (2) では Linux 先行明示のみ、本 §4.2 で +1.89 PM 新規)
3. **Mac 増分の OS 軸新規可視化** (charter §4 (2) で t-noami さん workflow 言及あるが工数算定なし、本 §4.3 で +4.05 PM 新規)

#### 3 OS 合計 ≠ Linux × 3 の根拠

- Win/Mac 増分は **Linux baseline 上の追加 work** であり、各 OS で独立 Linux baseline を再実施する work ではない
- Win 増分 1.89 PM / Mac 増分 4.05 PM は本 §4.2 / §4.3 で「Linux baseline に対する +Δ」として算出済
- 3 OS 合計 35.84 PM = Linux baseline 29.90 + Win Δ 1.89 + Mac Δ 4.05 が本算定の妥当な集約 (per-OS 重複排除済)

#### Mac 増分 > Win 増分 の根拠

- Win 増分 1.89 PM vs Mac 増分 4.05 PM = Mac 約 2.1 倍の理由:
  1. MoltenVK 経由 (Vulkan 1.2 core + 一部 1.3 portable subset) = 1.3 機能の fallback path コード必要
  2. MSL 経由 shader 動作確認 (spirv-cross + MoltenVK runtime) = Linux/Win より 1 段階多い変換 chain
  3. t-noami さん workflow cycle = Linux build 完成 → 検証 → patch return の lead time が増加
  4. macOS 14+ minimum (Metal 3) = OS 動作 baseline の制約 (Linux/Win より厳しい minimum)

#### 本算定 base 値の確定

**§4.5 3 OS 合計 25.12 PM (work 工数、Linux 21.07 + Win 1.35 + Mac 2.70) / 35.84 PM (余裕係数適用後、Linux +42% / Win +40% / Mac +50%) を本算定の 3 OS 合計工数 (フルタイム dev、本職並走 ratio + 学習曲線 適用前) として採用**。§5 で本職並走 ratio + 学習曲線、§6 で uncertainty band 適用予定。

---

## §5 各 milestone の所要月数 / 年数 (本職並走前提)

### §5.0 算定方針

§3 + §4 で算出した **フルタイム dev 換算 PM** を、AYA さん **本職並走** の **暦月** に変換する。変換係数は 2 段:

1. **Vulkan 学習曲線** (milestone 別 weight、charter §4 (3) +20-30% 想定の精緻化) — PM の上方修正
2. **本職並走 ratio** (フルタイム 1 PM = AYA 並走 N 暦月、charter §4 (3) 想定 3-5x の中央値確定) — PM → 暦月変換

#### 不確実性 (§6) との分離

本 §5 では **中央値のみ** を算出。

- charter §4 (3) 乖離理由 4 要素 (a-3 7.5 PM → 6-15 人年) のうち、**並走係数 + 学習曲線** を本 §5 で適用
- **不確実性 2-3x** は §6 uncertainty band で扱う、本 §5 中央値には含めない

#### 本 §5 範囲

- §3 + §4 で算出済 (Linux 29.90 PM / Win 1.89 PM / Mac 4.05 PM / 3 OS 合計 35.84 PM、余裕係数込) を input
- 学習曲線 + 並走 ratio 適用後の **暦月 / 年** を milestone 別 + 累積で算出
- 本算定 base = vk-RC parity 完遂 (= r44 達成、charter §4 (1)) までの total year scale
- r45+ は本算定範囲外 (§3.8、charter §3 / §6)

### §5.1 AYA 本職並走 ratio 確定

#### charter §4 (3) 想定の精緻化

charter §4 (3) 注 「**乖離理由 = 並走係数 3-5x + 学習曲線 + 不確実性 2-3x**」の **並走係数 3-5x** を本 §5.1 で具体値に確定。

並走係数の定義: **フルタイム dev 1 PM の作業量を AYA さん本職並走で達成する暦月数**。

#### AYA さん本職並走の time budget 推定

| 項目 | 時間 | 注 |
|---|---|---|
| AYA さん本職 | 平日 full time (40 時間 / 週) | 並走負荷の base |
| AYAstorm work 平日夜 | 2-3 時間 × 5 日 = 10-15 時間 / 週 | 本職後の集中時間 |
| AYAstorm work 週末 | 4-8 時間 × 2 日 = 8-16 時間 / 週 | 連続時間が取れる時間帯 |
| AYAstorm work weekly hours (生) | 18-31 時間 / 週、中央値 ~25 時間 | フルタイム 40 時間の 62% |

#### 並走係数の補正要因

| 補正要因 | 係数 | 根拠 |
|---|---|---|
| 生 work hours ratio (フルタイム 40 hr/週 ÷ AYAstorm 25 hr/週) | × 1.60 | 上記 time budget の単純比 |
| Context switch loss (本職 ↔ AYAstorm 切替で ramp-up 30-50% 時間損失) | × 1.40 | 本職集中作業との切替で 1 session 30-60 min が ramp-up に消える |
| 体調 / 疲労 / 個人事情で AYAstorm work skip 週が 1/4 程度 | × 1.33 | charter §8 (C) AYA life plan trigger は band §6 (1) で扱う、ここは恒常的な skip 率のみ |
| **合成 並走係数 (本 §5 中央値採用)** | **× ~3.0 〜 ~5.0** | 1.60 × 1.40 × 1.33 ≈ 2.97、加えて r41 初期の Vulkan 設計 cycle が長い場合に上方振れで 5x、charter §4 (3) 想定 3-5x と整合 |

#### 本 §5.1 確定値

**並走係数 中央値 = 4x** (フルタイム dev 1 PM = AYA 並走 4 暦月)。

- 中央値 4x は charter §4 (3) 想定 3-5x の middle、補正要因合成の中央値 ~3.0 と Vulkan 設計 cycle 上方振れ ~5.0 の median
- 並走係数の振れ幅 ±25% (3x 〜 5x) は §6 uncertainty band (1) 体制変動 + (5) personal life event + (6) Vulkan 学習曲線実測偏差 で扱う
- 学習曲線は §5.2 で milestone 別に上乗せ (本 §5.1 並走係数とは別軸)

### §5.2 Vulkan 学習曲線の milestone 別反映

#### charter §4 (3) 想定の精緻化

charter §4 (3) 注 「学習曲線 +20-30%」を milestone 別に分配。Vulkan 経験は r41 序盤で集中、r41.5 で abstraction 設計、r42+ で inline 化想定。

#### milestone 別 学習曲線 weight

| milestone | 学習曲線 weight | 根拠 |
|---|---|---|
| r41 (GL 除去 + Vulkan 空転) | **+30%** | Vulkan instance / swapchain / render pass / descriptor / sync / VMA の初回学習、charter §4 (3) 上限 |
| r41.5 (VK repo 分離) | **+20%** | Vulkan API は r41 で習得済、abstraction interface 設計 + license 分離手続が新規、charter §4 (3) 下限近 |
| r42-α (r21.1 picker port) | **+10%** | Vulkan API inline 化済、AYAstorm picker 知識は既習、新規 work は render pass attachment の確立済 pattern 適用 |
| r42-β (r30 Cinematic port) | **+10%** | DoF frame context refactor は確立済 pattern + Cinematic 機能知識は既習 |
| r42-γ (r14+ visual realism port) | **+10%** | post-process pass chain は確立済 pattern、visual A/B は AYAstorm 既習 |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | **+10%** | regression sweep + parity polish、新規学習は少 |
| r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) | **+0%** | Vulkan API + Mac MoltenVK + Win driver matrix の knowledge は r42-δ までで習得済、polish 期は inline 化済 |
| Win 増分 (r42-α 以降 並走) | **+10%** | Win 側 LunarG SDK + WGL → `VK_KHR_win32_surface` 移行の初回学習 |
| Mac 増分 (r42-β 以降 並走) | **+10%** | MoltenVK + MSL + t-noami workflow cycle の初回学習 |

#### 学習曲線 weighted PM

| 出処 | base PM (Linux baseline、余裕係数込) | 学習曲線 weight | weighted PM |
|---|---|---|---|
| r41 | 16.17 | × 1.30 | 21.02 |
| r41.5 | 1.50 | × 1.20 | 1.80 |
| r42-α | 0.65 | × 1.10 | 0.72 |
| r42-β | 3.15 | × 1.10 | 3.47 |
| r42-γ | 3.18 | × 1.10 | 3.50 |
| r42-δ | 2.25 | × 1.10 | 2.48 |
| r43-r44 | 3.00 | × 1.00 | 3.00 |
| **Linux baseline 合計** | **29.90** | weighted avg +20.4% | **~35.97** |
| Win 増分 | 1.89 | × 1.10 | 2.08 |
| Mac 増分 | 4.05 | × 1.10 | 4.46 |
| **3 OS 合計** | **35.84** | weighted avg +18.7% | **~42.51** |

#### 学習曲線 weight の振れ幅

- 本 §5.2 中央値 = 上記 milestone 別 weight (charter §4 (3) +20-30% 範囲内)
- r41 学習曲線の実測偏差で +20% → +50% に上方振れる risk は §6 (6) Vulkan 初見学習曲線実測偏差で扱う
- 学習曲線 inline 化が想定より早く r42-α 以降 +0% に下方振れる可能性は低い (Vulkan extension / driver-specific quirk で継続的に学習発生)

### §5.3 milestone 別 所要暦月 中央値

#### 計算式

**milestone 別 所要暦月 = base PM × 学習曲線 weight × 並走係数 4x**

Linux baseline + 3 OS 増分の中央値:

| milestone | base PM | 学習曲線 weight | weighted PM | × 並走 4x | 所要暦月 (中央値) | 年換算 |
|---|---|---|---|---|---|---|
| r41 (GL 除去 + Vulkan 空転) | 16.17 | × 1.30 | 21.02 | × 4 | **~84.1 暦月** | ~7.01 年 |
| r41.5 (VK repo 分離) | 1.50 | × 1.20 | 1.80 | × 4 | **~7.2 暦月** | ~0.60 年 |
| r42-α (r21.1 picker port) | 0.65 | × 1.10 | 0.72 | × 4 | **~2.9 暦月** | ~0.24 年 |
| r42-β (r30 Cinematic port) | 3.15 | × 1.10 | 3.47 | × 4 | **~13.9 暦月** | ~1.16 年 |
| r42-γ (r14+ visual realism port) | 3.18 | × 1.10 | 3.50 | × 4 | **~14.0 暦月** | ~1.17 年 |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | 2.25 | × 1.10 | 2.48 | × 4 | **~9.9 暦月** | ~0.83 年 |
| r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化) | 3.00 | × 1.00 | 3.00 | × 4 | **~12.0 暦月** | ~1.00 年 |
| **Linux baseline 小計** | **29.90** | — | **35.97** | × 4 | **~143.9 暦月** | **~11.99 年** |
| Win 増分 (r42-α 以降 並走) | 1.89 | × 1.10 | 2.08 | × 4 | **~8.3 暦月** | ~0.69 年 |
| Mac 増分 (r42-β 以降 並走) | 4.05 | × 1.10 | 4.46 | × 4 | **~17.8 暦月** | ~1.49 年 |
| **3 OS 合計** | **35.84** | — | **42.51** | × 4 | **~170.0 暦月** | **~14.17 年** |

#### 注記

- 本 §5.3 は **AYA 1 人体制** が前提のため、Win/Mac 増分は Linux baseline と calendar 上 serialize (本職並走 1 dev は同時並列不可、time-slice で interleave 可能だが累計 calendar には影響しない)
- 並走 4x は本 §5.1 確定値、§5.2 学習曲線と独立に適用
- r45+ は §3.8 / charter §3 / §6 で本算定範囲外、本 §5.3 表に含めない

### §5.4 milestone 累積 所要暦月

§5.3 の milestone 別暦月を順次積算 (charter §4 (2) Linux 先行 → Win/Mac 後追い + §4.4 OS 別 milestone 着手 timing 反映):

| 達成段階 | 累積 base PM | 累積 weighted PM | 累積暦月 (中央値) | 暦年マーカー (2026-05-28 着手起点) |
|---|---|---|---|---|
| r40 達成 (工程プラン完成、本算定 close) | 0 | 0 | 0 暦月 | 2026-05-28 |
| r41 達成 (Linux GL 除去 + Vulkan 空転) | 16.17 | 21.02 | ~84.1 暦月 | **~2033 年中** |
| r41.5 達成 (VK repo 分離) | 17.67 | 22.82 | ~91.3 暦月 | ~2034 年初 |
| r42-α 達成 (r21.1 picker port、Win 着手始動) | 18.32 | 23.54 | ~94.1 暦月 | ~2034 年前半 |
| r42-β 達成 (r30 Cinematic port、Mac 着手始動) | 21.47 | 27.01 | ~108.0 暦月 | ~2035 年中 |
| r42-γ 達成 (r14+ visual realism port、Win/Mac 並走) | 24.65 | 30.51 | ~122.0 暦月 | ~2036 年後半 |
| r42-δ 達成 (parity 残機能 / vk-RC 直前 polish) | 26.90 | 32.99 | ~131.9 暦月 | ~2037 年中 |
| r43-r44 達成 (Linux baseline parity 補強完遂) | 29.90 | 35.97 | ~143.9 暦月 | ~2038 年中 |
| + Win 増分完遂 | 31.79 | 38.05 | ~152.2 暦月 | ~2039 年初 |
| + Mac 増分完遂 (= vk-RC 3 OS parity 完遂、charter §4 (1) 達成) | 35.84 | 42.51 | **~170.0 暦月** | **~2040 年後半** |

#### r41 達成までの year scale

- **~7 年 (2033 年中)** — Linux GL 除去 + Vulkan 空転、charter §4 (1) parity 完遂 goal の最初の足場
- r41 work は本算定の最大集中 milestone (16.17 PM = base work の 54%、学習曲線 weight も最大 +30%)

#### vk-RC 3 OS parity 完遂までの total year scale

- **~14 年 (2040 年後半)** — AYAstorm r1-r30 全機能を 3 OS で Vulkan 上に再現、charter §4 (1) 達成
- Linux baseline 完遂が ~12 年 (2038 年中)、Win/Mac 増分の絶対量 (合計 5.94 PM = 全体の 14%) は r42-α/β 以降 並走で進めても calendar は ~2 年加算

### §5.5 charter §4 (3) 6-15 人年想定との整合性 verification

#### charter §4 (3) の想定値

charter §4 (3) より:
- **a-3 §5.4 フルタイム dev / 経験者前提**: 合計 **7-8 人月** (base 4-5 + AYAstorm 3)
- **本 (3) AYA 本職並走 / Vulkan 初見前提**: **6-15 人年** (= 72-180 PM)、本職並走 calendar = **15-30 年**
- 乖離理由 = 並走係数 3-5x + 学習曲線 + 不確実性 2-3x

#### 本算定との突合

| 指標 | charter §4 (3) | 本算定 §5 (3 OS 合計、中央値) | 整合判定 |
|---|---|---|---|
| a-3 base 工数感 (フルタイム dev 経験者、Linux only) | 7-8 PM | 21.07 PM (本 §3.9 base、3 OS 増分 + 余裕係数前 = a-3 + per-file 精緻化 + 新規 milestone 反映、§3.9 self-trace で整合確認済) | per-file 精緻化反映、a-3 概算オーダーから +180% |
| 1 人 full-time 換算 (フルタイム dev、本職並走 ratio 適用前) | 6-15 人年 (= 72-180 PM) | 35.84 PM = ~2.99 人年 (本 §4.5、3 OS 余裕係数込) | charter 下限 6 人年 (72 PM) の 50%、下回る |
| 1 人 full-time 換算 (本 §5.2 学習曲線適用後) | (上記と同) | 42.51 PM = ~3.54 人年 | charter 下限 6 人年の 59%、下回る |
| 本職並走 calendar (本 §5.3 並走 4x 適用後) | 15-30 年 | **~14.17 年** | charter 下限 15 年の 94%、**ほぼ整合 (下限近接)** |
| 本算定 + §6 uncertainty band 上方 (本職並走 calendar) | (charter 上限) 30 年 | ~27 年 (§6.5 上方シナリオ、§6.4 累積) | **整合範囲内 (charter 上限 30 年の 90%)** |

#### 整合判定の解釈

- **本算定 中央値 (本職並走 calendar 14.17 年) は charter §4 (3) 下限 15 年に近接、整合範囲内 (charter 想定の下限近)**
- charter §4 (3) は Doom 6-12 か月 (3 名経験者) / Blender 7 年未完 (rotating contributor) との比較で broad-stroke の top-down 推定、本算定は a-3 / a-4 棚卸し からの bottom-up 精緻化
- 本算定の bottom-up 結果が charter top-down 推定の下限近に着地 = charter §4 (3) の broad 6-15 人年想定が本算定で **下限寄りに精緻化** された結果

#### 過小評価 risk の整理

本算定が charter §4 (3) 下限 (1 人 full-time 換算 6 人年 = 72 PM) を下回る分 (本算定 42.51 PM、下限の 59%) の解釈:

1. **a-3 経験者前提**: 本算定は a-3 段階 port 戦略 (フルタイム dev / 経験者前提) を base に精緻化、Vulkan 経験者の作業量を baseline、AYA 本職並走の不熟練 work は学習曲線で +20.4% のみ反映
2. **不確実性 2-3x は §5 中央値には含めない**: charter §4 (3) 乖離理由 4 要素のうち「不確実性 2-3x」は §6 uncertainty band で扱う、§5 中央値に含めると band 算定と二重計上になる
3. **不確実性 を含めた upper bound**: 本算定 §6.5 上方シナリオ (+90%) = 27 年 (3 OS parity 完遂、本職並走 calendar) → charter §4 (3) 上限 30 年に近接、整合範囲内
4. **結論**: 本算定 §5 中央値 14.17 年 + §6 uncertainty band 上方 27 年は、charter §4 (3) 想定 15-30 年帯の **下限 〜 上限近接** に着地、整合 ✓

#### 過大評価 risk の整理

本算定 §6 下方シナリオ (-30%) = ~10 年 (3 OS parity 完遂、本職並走 calendar) は charter §4 (3) 下限 15 年を下回るが、これは **low-likelihood シナリオ** (scope shrink + 体制好転 + 学習曲線 inline 化が同時に発生):

- 本算定の下方振れは scope creep が想定外に少ない + AYA 体制が想定外に好転 + Vulkan 学習曲線が +0% に inline 化 の同時発生が必要
- charter §4 (3) は不確実性 2-3x を含む top-down 推定、本算定下方シナリオ -30% は §6 (1)-(8) 要因合成の最良ケース
- 過大評価 risk = §6 (4) scope creep 下方振れ -5% が限定的なので、現実的には -15% 程度 (~12 年)、charter §4 (3) 下限 15 年に近接

### §5.6 marker 暦年 (進捗 marker)

#### marker 暦年の位置付け

- 本 §5.6 暦年は **進捗 marker** (charter §3 / §8 (A) 「時間軸では撤退条件を設けない」遵守)
- **撤退条件には使わない**、charter §8 (B) 工程プラン破綻 trigger 閾値は §8 (group C) で別途定義
- 暦年は AYA さん / 外部観察者 が r40 章の進捗を時系列で参照できる目安として記録

#### marker 暦年 table (中央値、§5.4 累積暦月 + 着手日 2026-05-28 起点)

| 達成段階 | 中央値暦月 (累積) | 暦年マーカー | charter §8 (B) plan B trigger 関連 (§8 で詳細化) |
|---|---|---|---|
| r40 達成 (本算定 close、r41 着手起点) | 0 | 2026-05-28 | — |
| r41 達成 (Linux GL 除去 + Vulkan 空転) | ~84 暦月 (~7 年) | **~2033 年中** | charter §8 (B) 「r41 達成が 3 年経過しても未達」trigger は本 marker の半分、§8 で確認 |
| r41.5 達成 (VK repo 分離) | ~91 暦月 (~7.6 年) | ~2034 年初 | — |
| r42-α 達成 (r21.1 picker port、Win 着手始動) | ~94 暦月 (~7.9 年) | ~2034 年前半 | — |
| r42-β 達成 (r30 Cinematic port、Mac 着手始動) | ~108 暦月 (~9 年) | ~2035 年中 | — |
| r42-γ 達成 (r14+ visual realism port) | ~122 暦月 (~10.2 年) | ~2036 年後半 | — |
| r42-δ 達成 (parity 残機能 / vk-RC 直前 polish) | ~132 暦月 (~11 年) | ~2037 年中 | — |
| r43-r44 達成 (Linux baseline parity 補強完遂) | ~144 暦月 (~12 年) | ~2038 年中 | — |
| + Win 増分完遂 | ~152 暦月 (~12.7 年) | ~2039 年初 | — |
| **vk-RC 3 OS parity 完遂 (charter §4 (1) 達成)** | **~170 暦月 (~14.2 年)** | **~2040 年後半** | charter §8 (D) 「5 年経過 (2031-05-28) で LL Vulkan release ETA も公開されない」も別 trigger、本 marker は vk-RC 達成 marker のみ |

#### 進捗 marker としての使い方

- **中央値の暦年が経過しても milestone 達成しない場合** → §6 uncertainty band 上方シナリオに振れている兆候、§8 plan B trigger 閾値 (§8 で詳細化) と比較して charter §8 (B) plan B trigger 発動判断
- **中央値より早く milestone 達成した場合** → §6 下方シナリオ、進捗良好、次 milestone への着手前倒し可
- **暦年マーカーで進捗を比較する際は、各 milestone の §6 uncertainty band (上方 +N% / 下方 -M%) を併用** (§6.3 milestone 別 band 表参照)

#### r45+ marker は本算定範囲外

- §3.8 / charter §3 / §6 で r45+ visual realism 次世代は本算定範囲外
- vk-RC parity 完遂 (= r44 達成、~2040 年後半) 後の AYAstorm 体制 / industry 状況 / LL 着地 status 次第で r45+ 着手 timing は判断
- 本 §5.6 marker 暦年は **vk-RC parity 完遂 (~2040 年後半) で終了**

---

## §6 算定の uncertainty band (上方 / 下方)

### §6.0 算定方針

§5 で算出した **中央値** に対する **uncertainty band** (上方 = 最悪シナリオ / 下方 = 最良シナリオ) を本 §6 で算出。

#### band 算定の対象

- §5.3 milestone 別 暦月 中央値、§5.4 累積暦月、§5.6 marker 暦年 すべてに band を適用
- band 適用後の上方 = §8 plan B trigger 閾値 (charter §8 (B)) の設定対象 (§8 group C で詳細化)
- band 適用後の下方 = 進捗良好時の前倒し marker (charter §3 「時間軸では撤退条件を設けない」、下方も marker のみ)

#### charter §4 (3) 不確実性 2-3x との関係

- charter §4 (3) 注 「不確実性 2-3x」は本 §6 で扱う、§5 中央値には含めない (§5.0 算定方針再掲)
- 本 §6 で算出する uncertainty band 上方 (~+90%) は charter §4 (3) 不確実性 2-3x の下限 ~2x に近接、整合範囲内
- band 上方の合成根拠は §6.2 で 8 要因 × 振れ幅 として詳細化

#### 本 §6 範囲

- §6.1 不確実性要因の分類 (8 要因)
- §6.2 各要因の振れ幅 (中央値 ±%、合成 ±%)
- §6.3 milestone 別 band (r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / Win / Mac)
- §6.4 累積 band (r41 達成までの band / r44 vk-RC parity 完遂までの band)
- §6.5 上方 / 中央 / 下方 の 3 シナリオ tabulation
- §6.6 band を縮める方策 (sub-milestone 区切り強化 / 早期 prototype / 並走外注検討の閾値)

### §6.1 不確実性要因の分類

charter §4 (3) 乖離理由 + a-3 / a-4 棚卸し棚卸し + 本 §5 算定経緯から、本算定に効く不確実性要因を **8 件** に分類:

| # | 要因 | 性質 | charter / 算定 source |
|---|---|---|---|
| (1) | **体制変動** | AYA 本職 / 健康 / 家庭の状況変化、AYAstorm work hours の長期変動 (本職並走の負荷累積) | charter §8 (C) AYA life plan trigger に近接 |
| (2) | **技術選定 drift** | Vulkan SDK / extension / driver 仕様変更、MoltenVK 進化、glslang 等 tooling 進化 | charter §5 LL 着地時判断 + 業界動向 / 05 doc §1 (loader / SDK 選定) |
| (3) | **外部 dependency** | LL upstream merge cost、t-noami workflow cycle、third-party lib (VMA / volk / spirv-cross) 進化 | charter §4 (2) 3 OS / charter §7 LL 着地時判断 / 05 doc §1.2 (volk) + §6 (VMA) |
| (4) | **scope creep** | a-3 範囲外 milestone (r41.5 / r42-δ / r43-r44) の追加発見 work、parity 残機能 expand | charter §4 (1) parity 完遂 / §3 r41.5 + r42-δ + r43-r44 新規 milestone |
| (5) | **personal life event** | charter §8 (C) trigger 相当の個人的事情変化、project pause / restart | charter §8 (C) |
| (6) | **Vulkan 初見学習曲線の実測偏差** | charter §4 (3) +20-30% 想定が +50% に超過の可能性 (設計 cycle が想定より長い、Vulkan extension 学習が再発) | charter §4 (3) 並走係数 / 学習曲線 / §5.2 milestone 別 weight |
| (7) | **余裕係数の過小 / 過大** | §3 で per-milestone +30-50% 設定、実工程で +60% 以上 or +20% 以内に収束 | §3 per-milestone 余裕係数 (charter §4 (3) + 03 doc §5 算定軸 5) |
| (8) | **per-file 工数原単位の偏差** | foundation §1 / §2 の原単位値 (PM/file、PM/shader) が想定外で偏差 (LOC 加重補正の ±50% を超える file が発見される、shader 複雑度の想定外偏差) | foundation §1.1 + §2.1 原単位 |

#### 要因間の correlation 仮定

要因間は完全独立ではなく、**正 correl** で連動する傾向 (本算定では保守側に倒して採用):

- (1) 体制変動 ↔ (4) scope creep ↔ (6) Vulkan 学習曲線超過 = 連動 (体制悪化期に学習曲線が伸びる + scope 縮小も難しい)
- (3) 外部 dependency ↔ (2) 技術選定 drift = 連動 (lib 進化が技術選定再評価を引き起こす)
- (7) 余裕係数偏差 ↔ (8) per-file 偏差 = 連動 (per-file 偏差が大きいと余裕係数も外れる傾向)
- (5) personal life event = 独立 (他要因と connection 弱、突発性高い)

合成方法は §6.2 で 「**正 correl 寄せの中間合成**」 を採用。

### §6.2 各要因の振れ幅 (中央値 ±%)

各要因の振れ幅と合成方法:

#### 要因別 振れ幅

| # | 要因 | 上方 (PM 増加) | 下方 (PM 減少) | 根拠 |
|---|---|---|---|---|
| (1) | 体制変動 | **+30%** | **-10%** | 体制悪化が改善より発生確率高い (本職並走の負荷累積、健康変動、家庭事情の不可逆性) |
| (2) | 技術選定 drift | **+20%** | **-10%** | Vulkan SDK 進化で portage cost 減 (downward) or extension 仕様変更 cost 増 (upward)、過去 7 年の Vulkan 進化が示す両方向の振れ |
| (3) | 外部 dependency | **+25%** | **-15%** | LL upstream merge cost / t-noami workflow cycle 想定超え (upward) / lib 進化で workaround 不要 (downward) |
| (4) | scope creep | **+30%** | **-5%** | parity 完遂 goal で scope 縮小は難しい (downward 限定的)、a-3 範囲外発掘 work の追加発見 risk (upward) |
| (5) | personal life event | **+50%** | **-5%** | pause / restart が work 延長を引き起こす、短縮は稀、charter §8 (C) trigger 一歩手前の状況も含む |
| (6) | Vulkan 初見学習曲線 | **+20%** | **-10%** | 学習曲線が +30% (charter §4 (3) 上限) を超え +50% に振れる可能性 (上方) / Vulkan API inline 化が想定より早い (下方) |
| (7) | 余裕係数偏差 | **+20%** | **-15%** | per-milestone +30-50% 設定が想定外で偏差、a-3 範囲外 milestone (r41.5 / r42-δ / r43-r44) で余裕推定の信頼度が低い |
| (8) | per-file 偏差 | **+20%** | **-15%** | foundation §1 / §2 原単位の実工程偏差、LOC 加重補正 ±50% で吸収しきれない file の存在可能性 |

#### 合成 ±%

合成方法と結果:

| 合成方法 | 上方 ±% | 下方 ±% | 解釈 |
|---|---|---|---|
| 独立性仮定 (root-sum-square 近似) | +60% | -25% | 全要因が独立、振れが部分相殺 (本算定では非採用、根拠 §6.1 correlation 仮定参照) |
| 完全 correl 仮定 (単純合算、worst case) | +215% | -85% | 全要因が完全連動、最悪 (本算定では過大、現実的でない) |
| **正 correl 寄せの中間合成 (本算定採用)** | **+90%** | **-30%** | 連動要因群が同方向に振れ、独立要因 ((5) personal life event) は部分独立で合成 |

#### 中間合成の根拠

- 連動要因 (1)/(4)/(6) は同方向に振れて upper bound +30% + +30% + +20% = +80% (連動寄り、互いに reinforce)
- 連動要因 (2)/(3) は upper +20% + +25% = +45% だが (1)/(4)/(6) と部分独立、本算定では +20% で wash-out
- 独立要因 (5) personal life event は upper +50% で他要因と部分独立、本算定では +30% で部分加算 (発生確率を考慮した加重)
- (7)/(8) は (1)/(4)/(6) と部分連動、上方 +20% を (1)/(4)/(6) に吸収して合算しない
- **上方 +90%** = (1)+(4)+(6) 連動 (+30+30+20=+80) + (5) 加重 (+10、+50% × 確率 0.2) ≈ +90%
- 下方 (1)/(4)/(6) 反対方向は同程度の発生確率が低い (-10-5-10=-25)、(5) 反対方向はゼロ、(2)/(3) 下方 -10/-15、合成 ~-30% (保守的に下げる)

#### 本 §6.2 確定 band

**uncertainty band 上方 = +90%、下方 = -30%** を §6.3 milestone 別 band の base 値として採用。milestone 別の局所 band は §6.3 で +/- 補正。

### §6.3 milestone 別 uncertainty band

§6.2 base band (+90% / -30%) を milestone 別に補正 (要因の milestone 集中度を反映):

| milestone | base PM (3 OS 合計の Linux baseline 分、§3.9) | 上方 band | 下方 band | 上方 PM | 下方 PM | 性質 |
|---|---|---|---|---|---|---|
| **r41 (GL 除去 + Vulkan 空転)** | 16.17 | **+120%** | **-30%** | 35.57 | 11.32 | 最大不確実性 (Vulkan 初見学習曲線 + a-3 範囲の絶対 work 最大、要因 (1)(4)(6)(8) 集中) |
| **r41.5 (VK repo 分離)** | 1.50 | **+80%** | **-25%** | 2.70 | 1.13 | 中程度 (新規 milestone、a-3 範囲外、構造 refactor 経験少 + license 分離手続 unknown) |
| **r42-α (r21.1 picker port)** | 0.65 | **+50%** | **-25%** | 0.98 | 0.49 | 軽量、不確実性低 (foundation 帰属が大半、追加 work 軽量) |
| **r42-β (r30 Cinematic port)** | 3.15 | **+60%** | **-25%** | 5.04 | 2.36 | 中程度 (Cinematic frame context refactor + DoF state enum、知識は AYAstorm 既習) |
| **r42-γ (r14+ visual realism port)** | 3.18 | **+70%** | **-25%** | 5.41 | 2.39 | 中程度 (visual realism post-process descriptor + perf profile + visual A/B iterate) |
| **r42-δ (parity 残機能 / vk-RC 直前 polish)** | 2.25 | **+90%** | **-25%** | 4.28 | 1.69 | 高不確実性 (a-3 範囲外、parity 残機能発掘 cost が高 unknown、vk-RC 直前の regression sweep) |
| **r43-r44 (parity 補強 / 性能 polish / Mac portable subset 詳細化)** | 3.00 | **+100%** | **-30%** | 6.00 | 2.10 | 高不確実性 (Mac MoltenVK + Win driver matrix の untested feature + 性能 polish iterate) |
| Win 増分 (r42-α 以降 並走) | 1.89 | +50% | -20% | 2.84 | 1.51 | 中程度 (driver matrix + 旧 driver fallback、現 GL viewer の Win 対応経験を踏まえる) |
| Mac 増分 (r42-β 以降 並走) | 4.05 | +80% | -25% | 7.29 | 3.04 | 高不確実性 (t-noami workflow cycle + MoltenVK portable subset + macOS 14+ Metal 3 minimum 制約) |
| **3 OS 合計 (base PM)** | **35.84** | weighted avg **+90%** | weighted avg **-30%** | **~68.1** | **~25.1** | base + 全 milestone band の weighted 合成 |

#### milestone 別 band 補正の根拠

- **r41 +120%**: 16.17 PM の絶対量 + Vulkan 初見学習曲線が最大 + a-3 範囲内なので per-file 工数原単位の偏差 (8) も最大、上方 band を §6.2 base +90% から +30% 上方修正
- **r41.5 +80%**: 1.50 PM の絶対量小、license 分離手続の unknown が局所的、上方 band を §6.2 base -10% 下方修正
- **r42-α +50%**: 0.65 PM の絶対量最小、foundation 帰属が大半、上方 band を §6.2 base -40% 下方修正
- **r42-δ +90%**: a-3 範囲外、parity 残機能発掘の unknown、上方 band を §6.2 base 並み
- **r43-r44 +100%**: Mac MoltenVK + Win driver matrix の untested feature が集中、§6.2 base +10% 上方修正
- **Mac 増分 +80%**: t-noami workflow cycle + MoltenVK portable subset で §6.2 base -10% 下方修正 (driver matrix unknown は r43-r44 で吸収)

#### r45+ band

§3.8 / charter §3 / §6 で本算定範囲外、本 §6.3 表に含めない。

### §6.4 累積 uncertainty band

§6.3 milestone 別 band を順次積算 (charter §4 (2) Linux 先行 → Win/Mac 後追い):

| 達成段階 | 中央値 累積暦月 | 上方 累積暦月 (+%) | 下方 累積暦月 (-%) | 上方 暦年マーカー | 下方 暦年マーカー |
|---|---|---|---|---|---|
| r41 達成 (Linux GL 除去 + Vulkan 空転) | ~84 暦月 (~7 年) | +120% = **~185 暦月 (~15.4 年)** | -30% = ~59 暦月 (~4.9 年) | ~2042 年 | ~2031 年 |
| r41.5 達成 (VK repo 分離) | ~91 暦月 (~7.6 年) | +110% weighted = ~192 暦月 (~16.0 年) | -28% weighted = ~66 暦月 (~5.5 年) | ~2042 年 | ~2032 年 |
| r42-α 達成 (Win 着手始動) | ~94 暦月 (~7.9 年) | +108% weighted = ~196 暦月 (~16.3 年) | -28% weighted = ~68 暦月 (~5.6 年) | ~2042 年 | ~2032 年 |
| r42-β 達成 (Mac 着手始動) | ~108 暦月 (~9 年) | +103% weighted = ~219 暦月 (~18.3 年) | -27% weighted = ~79 暦月 (~6.6 年) | ~2044 年 | ~2033 年 |
| r42-γ 達成 | ~122 暦月 (~10.2 年) | +99% weighted = ~243 暦月 (~20.2 年) | -27% weighted = ~89 暦月 (~7.4 年) | ~2046 年 | ~2034 年 |
| r42-δ 達成 | ~132 暦月 (~11 年) | +97% weighted = ~260 暦月 (~21.7 年) | -26% weighted = ~98 暦月 (~8.1 年) | ~2048 年 | ~2034 年 |
| r43-r44 達成 (Linux baseline 完遂) | ~144 暦月 (~12 年) | +95% weighted = ~281 暦月 (~23.4 年) | -27% weighted = ~105 暦月 (~8.8 年) | ~2049 年 | ~2035 年 |
| + Win 増分完遂 | ~152 暦月 (~12.7 年) | +93% weighted = ~294 暦月 (~24.5 年) | -27% weighted = ~111 暦月 (~9.3 年) | ~2051 年 | ~2036 年 |
| **vk-RC 3 OS parity 完遂 (charter §4 (1) 達成)** | **~170 暦月 (~14.2 年)** | **+90% weighted = ~323 暦月 (~26.9 年)** | **-30% weighted = ~119 暦月 (~9.9 年)** | **~2053 年** | **~2036 年** |

#### r41 達成までの band

- **上方 (最悪) = ~15 年 (2042 年)** — charter §8 (B) plan B trigger 「r41 達成が 3 年経過しても未達」とは別軸、本 §6 上方 band は中央値の 2.2 倍に振れた場合の到達点
- **中央値 = ~7 年 (2033 年中)**
- **下方 (最良) = ~5 年 (2031 年)** — scope shrink + 体制好転 + 学習曲線 inline 化 の同時発生 (low-likelihood)

#### vk-RC 3 OS parity 完遂までの band

- **上方 (最悪) = ~27 年 (2053 年)** — charter §4 (3) 上限 30 年に近接、整合範囲内 (charter §8 (B) plan B trigger 評価対象は §8 group C で詳細化)
- **中央値 = ~14 年 (2040 年後半)**
- **下方 (最良) = ~10 年 (2036 年)** — low-likelihood、scope shrink + 体制好転

### §6.5 上方 / 中央 / 下方 の 3 シナリオ tabulation

#### 3 シナリオ の charter §4 (3) 想定との対比

| シナリオ | 性質 | r41 達成 | vk-RC parity 完遂 (3 OS) | charter §4 (3) 想定 (本職並走 15-30 年) との関係 |
|---|---|---|---|---|
| **上方 (最悪、+90% weighted)** | charter §8 (B) plan B trigger 評価対象、体制悪化 + scope creep + 学習曲線超過 同時発生 | ~15 年 (2042 年) | **~27 年 (2053 年)** | charter 上限 30 年の 90%、**整合範囲内 (上限近)** |
| **中央値 (本算定 §5)** | 本算定 base 値 | ~7 年 (2033 年中) | **~14 年 (2040 年後半)** | charter 下限 15 年の 94%、**ほぼ整合 (下限近)** |
| **下方 (最良、-30% weighted)** | low-likelihood、scope shrink + 体制好転 + 学習曲線 inline 化 同時発生 | ~5 年 (2031 年) | **~10 年 (2036 年)** | charter 下限 15 年を下回る、本算定は a-3 経験者前提 base を反映 |

#### 各シナリオの発生確率 (定性評価)

| シナリオ | 推定確率 | 根拠 |
|---|---|---|
| 上方 (最悪) | ~10-15% | 全 8 要因が同時に上方振れる確率は低い、(1)(4)(6) 連動要因が同時悪化する場合 |
| 上方 75 percentile (+45% weighted) | ~25-30% | (1)(4)(6) のいずれかが想定超え |
| **中央値** | ~50% (中央値 ± 25% 内) | 本算定の base 値、各要因が想定範囲内で着地 |
| 下方 25 percentile (-15% weighted) | ~25-30% | scope shrink + lib 進化 + 余裕係数余りのいずれか |
| 下方 (最良) | ~5-10% | 全 8 要因が同時に下方振れる確率は最低 |

#### 3 シナリオの暦年 marker (中央値 + ±band)

| 達成段階 | 上方 (最悪) | **中央値** | 下方 (最良) |
|---|---|---|---|
| r41 達成 | ~2042 年 | **~2033 年中** | ~2031 年 |
| r41.5 達成 | ~2042 年 | ~2034 年初 | ~2032 年 |
| r42-α 達成 | ~2042 年 | ~2034 年前半 | ~2032 年 |
| r42-β 達成 | ~2044 年 | ~2035 年中 | ~2033 年 |
| r42-γ 達成 | ~2046 年 | ~2036 年後半 | ~2034 年 |
| r42-δ 達成 | ~2048 年 | ~2037 年中 | ~2034 年 |
| r43-r44 達成 | ~2049 年 | ~2038 年中 | ~2035 年 |
| **vk-RC 3 OS parity 完遂** | **~2053 年** | **~2040 年後半** | **~2036 年** |

#### charter §4 (3) との最終整合確認

- charter §4 (3) 本職並走 15-30 年想定 → 本算定上方 27 年 (上限 30 年の 90%) + 中央値 14 年 (下限 15 年の 94%) で整合範囲内
- charter §4 (3) 1 人 full-time 換算 6-15 人年想定 (= 72-180 PM) → 本算定 35.84 PM (中央値) + 上方 68.1 PM (charter 下限 72 PM の 95%) で **本算定上方が charter 下限に接続**
- 本算定の bottom-up 精緻化 (a-3 / a-4 → §1-§4) は charter §4 (3) top-down 推定の **下限-上限 範囲内に着地**、broad scale 整合 ✓

### §6.6 band を縮める方策

§6.4 累積 band の振れ幅を実工程で縮める方策 (charter §8 (B) plan B trigger 発動を回避する積極的な手段):

#### 方策 list

| # | 方策 | 効果 (band 縮小) | 発動 trigger / threshold |
|---|---|---|---|
| (a) | **sub-milestone 区切り強化** (r41 内 phase 別 acceptance criteria、r42-α/β/γ/δ 内 sub-phase 別 review) | 1 milestone 過度遅延の早期検出、scope creep 抑制、要因 (1)(4) 上方振れの早期 detect | r41 着手時の sub-milestone phase 設計で導入 (r41 charter 起草時) |
| (b) | **早期 prototype** (foundation 完了直後の Vulkan 空転 minimum spike) | 学習曲線 +30% 想定の実測検証、要因 (6) 上方振れの早期 detect、+50% 超過 risk の早期見極め | r41 着手 1-3 か月内 |
| (c) | **並走外注検討** (Mac portage / driver matrix 検証等) | t-noami さん workflow cycle 対価支払、要因 (3) Mac 増分 bottleneck 解消、Mac band -50% 圧縮可能性 | r42-β Mac 着手時に判断、r43-r44 Mac MoltenVK 詳細化前 |
| (d) | **LL 着地時の reset 判断** (charter §7 判断指針) | LL 公式 VK が AYAstorm 用途に十分なら work loss 回避、要因 (2)(3) 同時改善 | charter §7 LL 着地時判断指針 (charter §8 (A) trigger 連動) |
| (e) | **scope 縮小** (parity 完遂を一部 feature 単位で r45+ に押し出す) | charter §4 (1) parity 完遂 goal を一部 r45+ に押し出し、要因 (4) scope creep を下方振れに転換 | r42-δ までで進捗 evaluation、上方 band 上限近接時に発動 |
| (f) | **abstraction interface の前倒し導入** (r41.5 構造 refactor を r41 内で前倒し or r41 着手時から interface 経由実装) | r41 → r41.5 移行 cost 削減、要因 (4) scope creep 上方振れ抑制 | r41 着手時の設計判断 (05 doc §10 skeleton 参照) |
| (g) | **shader cross compile chain の自動化強化** (a-3 §B.x で 248 + 13 file 半自動化、cross compile 失敗 file の self-trace 自動化) | shader 工数の偏差 (8) 上方振れ抑制 | foundation §2 完了後の tool 整備 |

#### 方策の優先順位 (本算定からの推奨)

1. **(a) sub-milestone 区切り強化** — 最優先、r41 charter 起草時に詳細化、上方 band 早期 detect の base
2. **(b) 早期 prototype** — r41 着手 1-3 か月内、要因 (6) 学習曲線実測偏差の早期 detect
3. **(f) abstraction interface 前倒し** — r41 着手時の設計判断、r41.5 → r41 内化で scope creep 抑制
4. **(c) 並走外注検討** — r42-β Mac 着手時、Mac 増分 band -50% 圧縮効果
5. **(g) shader 自動化強化** — foundation §2 完了後の継続的整備
6. **(d) LL 着地時 reset 判断** — charter §7 + §8 (A) trigger 連動、本算定範囲外の積極策
7. **(e) scope 縮小** — r42-δ までで進捗 evaluation、最終手段

#### 方策発動の cadence

- **r41 charter 起草時**: (a) (f) の前提を charter に組込
- **r41 着手 1-3 か月内**: (b) Vulkan 空転 minimum spike
- **r41 進捗 review (annual or milestone 完遂時)**: (a) sub-milestone 区切り review、上方 band 近接の早期 detect
- **r42-β Mac 着手時**: (c) 並走外注検討
- **r42-δ までで進捗 evaluation**: (e) scope 縮小発動判断、charter §8 (B) plan B trigger 評価対象
- **LL 着地時 (charter §8 (A) trigger)**: (d) reset 判断

#### band 縮小目標

- 上方 band を §6.4 累積 +90% から **+50-60%** に圧縮できれば、vk-RC 3 OS parity 完遂上方が ~27 年 → ~22 年に短縮、charter §4 (3) 上限 30 年から余裕確保
- 中央値の前倒しは方策 (e) (g) 中心、ただし scope 縮小は charter §4 (1) parity 完遂 goal との trade-off
- 下方 band の改善 (-30% → -40%) は scope shrink + lib 進化 + 学習曲線 inline 化 の同時発生で 10% 程度の前倒し可能性

---

## §7 Doom / Blender 参照点との比較

### §7.0 算定方針

charter §4 (3) で提示された 2 つの参照点 (Doom 2016 + Blender) と本算定 §5 中央値 / §6 uncertainty band を突き合わせ、本算定の broad scale 妥当性を cross check する。

#### 本 §7 の役割

- charter §4 (3) top-down 推定 (6-15 人年 / 15-30 年並走) と本算定 bottom-up 精緻化 (§1-§4 → §5 中央値) の **整合 verification**
- 参照点との **構造的差異** を明示 (体制 / abstraction 有無 / 規模 / 性質) して本算定の妥当性根拠を強化
- 参照点に無い AYAstorm 固有要因 (撮影章用途 / 13 file 追加 / Mac t-noami workflow / 本職並走) を明示
- §7 出力 = §8 plan B trigger 閾値設定の参照基盤 (charter 想定 vs 本算定 の乖離 % が trigger 判断軸の 1 つ)

#### §7 範囲

- §7.1 Doom 2016 比較
- §7.2 Blender Vulkan 比較
- §7.3 charter §4 (3) 6-15 人年想定 cross check
- §7.4 本算定中央値 vs 参照点 のズレ要因分析
- §7.5 参照点に無い AYAstorm 固有要因

### §7.1 Doom 2016 (id Tech 6 OpenGL → Vulkan) との比較

#### 参照点プロファイル

| 項目 | 値 | 出典 |
|---|---|---|
| 体制 | 3 名 (full-time 経験者) | charter §4 (3) + 一般公表情報 |
| 期間 | 6-12 か月 | charter §4 (3) |
| 実工数 | 18-36 PM (3 名 × 6-12 か月) | charter §4 (3) |
| abstraction | clean abstraction あり (RHI 抽象層) | 一般公表情報 (Bethesda/id Software talk) |
| 規模 | id Tech 6 描画 engine 全体 | game engine 描画部 |
| 性質 | shipping 製品の DX11/12 + GL → Vulkan 化、profile 計測しながら段階移行 | game engine 業界の参照例 |

#### 本算定との対比

| 軸 | Doom 2016 | AYAstorm (本算定) | 差 |
|---|---|---|---|
| 体制 | 3 名 経験者 | 1 名 (AYA) 初見 | 体制 -67%、経験 -100% |
| 並走 | full-time | 本職並走 (charter §4 (3) ratio 3-5x、§5.1 確定 4x) | 並走 -75% |
| abstraction | あり (clean RHI) | なし → 05 doc §10 で **interface skeleton を r41.5 で導入予定** | abstraction 有無の差は r41.5 で部分緩和 |
| 規模 (LOC) | id Tech 6 描画 engine (公表値なし、~50-100K LOC 推定) | C++ critical path ~63K LOC + shader 248 file (~30-50K LOC 相当) | broad scale 同水準 |
| shader 数 | (非公開、~100-200 file 推定) | 248 + AYAstorm 13 file | broad scale 同水準 |
| 性質 | profit 駆動 ship product、profile 計測完備 | open-source viewer、profile 計測 base のみ | tool 整備差あり |
| **実工数換算** | **18-36 PM** | 本算定 §4.5 = **35.84 PM (余裕係数適用後)** | 本算定が **Doom 上限 36 PM の 99%** |

#### 整合判定

- 本算定 §4.5 = 35.84 PM (3 OS、フルタイム dev) は Doom 18-36 PM レンジの **上限近接**
- 整合 ✓ : AYAstorm は abstraction 不在 (Doom は有) + 13 file 独自機能追加 + 248 shader を含めた合計で Doom 上限近くに着地、broad scale 妥当
- 本算定の 35.84 PM が Doom 18 PM の **下限を 2x 超え** ないのは、AYAstorm 規模 (63K LOC) が Doom 描画 engine (~50-100K LOC) の broad scale 内に収まること + a-3 経験者前提の per-file 工数原単位を採用したことの整合
- charter §4 (3) 「Doom 6-12 か月 (実 18-36 PM)」と本算定 35.84 PM は **同 order**、broad scale 整合 ✓

#### 本算定の Doom 上限への接近理由

1. **abstraction 不在** → pipeline.cpp 3 大グローバル → frame context 集約 (§3.1 r41 で 5.5 PM 計上) が Doom にはない work、本算定で +1-2 PM
2. **AYAstorm 13 file 追加** → §2.3 で +0.6 PM (shader 部分)、§3.4-§3.5 で +5-6 PM (機能 port 含む)、Doom にはない work
3. **3 OS 増分** → §4.5 で Win +1.89 PM / Mac +4.05 PM の余裕係数込、Doom は 3 OS と性質が違う (ship 1 platform の game engine)
4. **余裕係数 +30-50%** → §3 で per-milestone +30-50% 適用、Doom 18-36 PM は recorded actual で余裕係数含むか未確認 (本算定は保守側に余裕係数加算)

### §7.2 Blender Vulkan との比較

#### 参照点プロファイル

| 項目 | 値 | 出典 |
|---|---|---|
| 体制 | 多数 (rotating contributor、core dev 1-2 + 外部 contributor) | Blender 公表情報 (developer.blender.org) |
| 着手時期 | 2019 年 (Blender 2.81 で Vulkan POC 開始) | Blender release note |
| 現状 | 2026 現在 7 年未完 (Blender 4.x で Vulkan backend は EEVEE Next 限定 + experimental flag) | Blender 4.x release note |
| 暦月 | ~84 暦月 (7 年) | 着手から現在まで |
| abstraction | あり (Blender RHI = GPU module、2019 整備) | Blender source / developer doc |
| 規模 | Blender 3D viewport + EEVEE renderer (Blender 全体 5-10M LOC のうち描画 module は ~100K-200K LOC、shader は EEVEE で ~200-400 file 規模) | Blender repo の broad estimate |
| 性質 | rotating contributor、本職以外の余暇 contribute も含む、年次目標は柔軟 | Blender Foundation 運営モデル |

#### 本算定との対比

| 軸 | Blender Vulkan | AYAstorm (本算定) | 差 |
|---|---|---|---|
| 体制 | 多数 rotating | 1 名 (AYA) 固定 | 安定性 +、効率 -、振れ幅 - |
| 並走 | core dev は full-time だが Vulkan 専従でない (rotating)、外部 contributor は余暇 | 本職並走 4x | 並走 ratio は近似 (Blender 実効稼働率も大幅減算) |
| abstraction | あり (GPU module = RHI 抽象、2019 整備) | r41.5 で interface 導入予定 (05 doc §10) | abstraction 設計の **time line 差** (Blender 先行整備、AYAstorm 後追い) |
| 規模 | Blender 描画 module ~100-200K LOC + shader ~200-400 file | C++ ~63K LOC + shader 248+13 file | AYAstorm は Blender の broad scale 60-80% |
| 期間実績 | **2019-2026 = 7 年 (84 暦月) で未完** | 本算定 §5.6 中央値 vk-RC 3 OS parity = **170 暦月 (14.17 年)** | 本算定が Blender 実績の **2x** (parity 完遂までの差) |
| Blender 残作業 | EEVEE Next 完了 + Cycles + sculpt mode + 各種 modifier、推定残 ~3-5 年 (合計 10-12 年規模) | r43-r44 + Win/Mac 完遂 | Blender parity 想定 10-12 年 vs 本算定 14 年、broad scale 同水準 |

#### 整合判定

- Blender Vulkan 7 年未完 + 推定残 3-5 年 = **合計 10-12 年規模** vs 本算定 vk-RC 3 OS parity 中央値 **14.17 年**、broad scale 同水準
- AYAstorm が Blender より **+2-4 年長い** 主因:
  1. **体制 1 名 vs 多数** (Blender は rotating でも実効並走数 2-3 倍以上、AYAstorm は単独固定)
  2. **abstraction 後追い** (Blender は 2019 整備済、AYAstorm は r41.5 で導入、抽象設計 cost が前段に乗る)
  3. **3 OS parity 完遂が AYAstorm 仕様** (Blender は Linux baseline + Win/Mac は contributor-driven で順次)
  4. **AYAstorm 13 file 独自機能 + 撮影描画用途の visual realism** が parity 完遂条件に含まれる
- 整合 ✓ : 本算定 14 年中央値は Blender 実績 (7 年未完 + 推定残) と broad scale 整合、AYAstorm 固有要因で +2-4 年の差は合理的

#### Blender 7 年未完が本算定上方 band の根拠

- Blender が 7 年で未完なのは本算定 §6 上方 band (+90% = ~27 年) の **下限根拠**
- Blender でさえ rotating contributor 多数 + abstraction 整備済で 7 年で完遂してない → AYAstorm 単独 1 名 + abstraction 後追い + 3 OS parity の本算定が中央値 14 年は **decidedly 楽観寄り** の可能性
- §6 上方 band +90% (27 年) は Blender 実績の 4x、charter §4 (3) 想定 30 年の 90%、broad scale 妥当

### §7.3 charter §4 (3) 6-15 人年想定の妥当性 cross check

#### charter §4 (3) 想定の再掲

| 軸 | charter §4 (3) | 換算 |
|---|---|---|
| 1 人 full-time | 6-15 人年 | = 72-180 PM |
| 本職並走 | 15-30 年 | = 180-360 暦月 |
| 不確実性 | 2-3x | §6 で別途扱い (本算定 §6 base band +90% / -30%) |

#### 本算定との突き合わせ (group B §5.5 / §6.5 拡張)

| 算定 | 1 人 full-time 換算 | 本職並走 calendar | charter 想定との比 |
|---|---|---|---|
| charter §4 (3) 下限 | 72 PM (6 人年) | 180 暦月 (15 年) | 本算定上方 (68.1 PM / 27 年) が charter 下限の **95%** |
| charter §4 (3) 上限 | 180 PM (15 人年) | 360 暦月 (30 年) | 本算定上方 (68.1 PM / 27 年) が charter 上限の **38% (1 人 full-time 換算)** / **90% (本職並走)** |
| 本算定 §5 中央値 | 35.84 PM (本 §4.5、3 OS 余裕係数込) / 学習曲線適用後 42.51 PM | 170 暦月 (14.17 年) | charter 下限 15 年の **94%** |
| 本算定 §6 上方 (最悪) | 68.1 PM | 323 暦月 (26.9 年) | charter 上限 30 年の **90%** |
| 本算定 §6 下方 (最良) | 25.1 PM | 119 暦月 (9.9 年) | charter 下限を下回るが low-likelihood (発生確率 5-10%) |

#### charter top-down vs 本算定 bottom-up の乖離パターン分析

1. **1 人 full-time 換算で差が大** : charter §4 (3) 下限 72 PM vs 本算定 中央値 35.84 PM → 本算定中央値は charter 下限の 50%
   - 差の主因: charter §4 (3) は Doom/Blender top-down 比較 + 不確実性 2-3x 込の broad estimate、本算定 bottom-up は a-3/a-4 棚卸し + 経験者前提の per-file 工数 + 余裕係数 +42%、不確実性は §6 別途
   - charter §4 (3) の 6-15 人年に **不確実性 2-3x を中央値内に含む** 解釈なら本算定 35.84 PM × 2x = 71.68 PM で charter 下限 72 PM に整合
   - 整合 ✓ : 本算定 §5 中央値 + §6 band 上方を統合すると charter 想定 6-15 人年帯に収まる

2. **本職並走 calendar で整合** : charter §4 (3) 15-30 年 vs 本算定 14.17 年 (中央値) / 26.9 年 (上方) → 中央値が charter 下限の 94%、上方が charter 上限の 90%、broad scale 整合
   - 整合 ✓ : 本算定 §5.5 (group B で実施済) + §6.5 で確認した整合判定を本 §7.3 で再確認

3. **本算定上方 band が charter 下限に接続** : 68.1 PM (上方) が 72 PM (charter 下限) の 95%、本算定の bottom-up 精緻化は charter top-down の下限を裏付け
   - 整合 ✓ : charter §4 (3) が「6-15 人年」と broad estimate した下限が本算定上方 band で再現された

#### 結論

- 本算定中央値 (14 年) + 上方 band (27 年) は charter §4 (3) 15-30 年想定の **下限-上限 範囲内** に着地、broad scale 整合 ✓
- charter §4 (3) top-down 推定の broad estimate は本算定 bottom-up 精緻化で **裏付けられた** (本 §7.1 Doom 上限近接 + §7.2 Blender 整合 で cross 検証)
- 本算定中央値が charter 下限近 (14 年 vs 下限 15 年) なのは a-3/a-4 経験者前提 + per-file 工数原単位の bottom-up 精緻化の効果、charter §4 (3) が想定した不確実性 2-3x は §6 で別途扱い

### §7.4 本算定中央値 vs 参照点中央値のズレ要因分析

#### ズレ要因表

| ズレ | 値 | 主因 |
|---|---|---|
| 本算定 35.84 PM (3 OS フルタイム) vs Doom 18-36 PM | +0 〜 +99% (Doom 上限近接) | abstraction 不在 (+1-2 PM) / AYAstorm 13 file (+5-6 PM) / 3 OS 増分 (+6 PM) / 余裕係数 (+30-50%) |
| 本算定 14.17 年 vs Blender 7 年未完 (推定残 3-5 年で 10-12 年規模) | +2-4 年 | 体制 1 名固定 vs Blender 多数 rotating (-50% efficiency) / abstraction 後追い (+abstraction 設計 cost) / 3 OS parity 仕様 (Blender は Linux baseline + Win/Mac contributor-driven) / AYAstorm 撮影描画用途の visual realism parity 含む |
| 本算定中央値 14 年 vs charter §4 (3) 下限 15 年 | -6% (本算定中央値 < charter 下限) | a-3/a-4 経験者前提の per-file 工数原単位 / 不確実性 2-3x を §6 別途扱い (charter 下限は不確実性込解釈) |
| 本算定上方 27 年 vs charter §4 (3) 上限 30 年 | -10% (本算定上方 < charter 上限) | §6 base band +90% は charter §4 (3) 不確実性 2-3x の下限 ~2x 近接、charter 想定の broad estimate 上限に整合 |

#### ズレ要因の本算定への取込

- **本算定中央値 14 年** = bottom-up 精緻化 (a-3/a-4 棚卸し + 経験者前提 per-file 原単位) + 並走係数 4x + 学習曲線 weighted +18.7% + 余裕係数 weighted +42%
- **本算定上方 27 年** = §6 base band +90% (連動要因 (1)(4)(6) 正 correl 寄せ合成 + 独立要因 (5) 加重)
- charter §4 (3) 6-15 人年は本算定の **中央値 (下限近) + 上方 band (上限近)** で範囲確保、整合 ✓

### §7.5 参照点に無い AYAstorm 固有要因

charter §4 (3) 参照点 (Doom + Blender) に存在せず、本算定で別途加算した AYAstorm 固有要因:

#### (i) 撮影描画用途 (r30 Cinematic + r14+ visual realism) parity

- AYAstorm 固有の撮影章用途 (r30 Cinematic mode + r14+ visual realism = atmospheric / volumetric / DoF / SMAA / SSAO 等の AYAstorm 独自実装) が parity 完遂条件に含まれる
- Doom は ship 1 game の描画、Blender は 3D viewport + EEVEE の汎用 renderer、いずれも撮影描画用途の visual realism は AYAstorm 独自仕様
- 本算定 §3.4 (r42-β Cinematic port 3.15 PM) + §3.5 (r42-γ visual realism port 3.18 PM) で計上、合計 6.33 PM (~18% of 35.84 PM 全体)
- 参照点に無い work、AYAstorm 固有要因として本算定 +6.33 PM の根拠

#### (ii) AYAstorm 機能 13 file 追加 (a-4 §B.x)

- AYAstorm 独自 shader 13 file (visual realism 7 file / Cinematic 4 file / picker 2 file) の SPIR-V 化 + descriptor set 整合
- 参照点 (Doom / Blender) に該当する additive work なし
- 本算定 §2.3 で +0.6 PM (shader 部分のみ)、§3.3-§3.5 で +5 PM (機能 port 含む)
- 参照点に無い work、AYAstorm 固有要因として本算定 +5.6 PM の根拠

#### (iii) Mac t-noami さん workflow (3 OS parity の Mac 増分)

- AYAstorm の Mac build は t-noami さんが担当 (memory: `feedback_credit_t_noami_equal_billing.md` + `feedback_mac_only_fixes_accept_as_is.md`)、Mac MoltenVK + Metal 3 制約 + macOS 14+ minimum 制約に対応した portable subset 詳細化が必要
- Doom は ship 1 platform (Win + console)、Blender は Win/Mac contributor-driven、いずれも本算定の workflow とは性質が違う
- 本算定 §4.3 (Mac 増分 4.05 PM 余裕係数 +50% 込) で計上、§6.6 方策 (c) で並走外注検討 (band 縮小策) を提案
- 参照点に無い work、AYAstorm 固有要因として本算定 +4.05 PM の根拠

#### (iv) 本職並走 4x

- AYA 1 人 + 本職並走 = フルタイム dev 1 PM が AYA 並走 4 暦月に展開、Doom/Blender 参照点には存在しない並走 ratio
- charter §4 (3) で想定 3-5x、本算定 §5.1 で中央値 4x 確定
- 本算定 calendar 換算 (14 年 vs フルタイム 3 年) の主因、参照点換算で Doom 6-12 か月が AYAstorm 換算で 24-48 か月に拡大

#### (v) 単独 1 人体制 (rotating contributor 不在)

- AYAstorm は AYA 1 人固定、Blender の rotating contributor model と異なる、Doom の 3 名 full-time とも異なる
- 1 人体制の特性: 効率 + 一貫性 (decision overhead ゼロ) / 振れ幅 + (体制変動 1 要因に集中) / 学習曲線が個人に依存 / scope creep 抑制力 (1 人が見渡せる) / personal life event の影響大
- charter §8 (C) AYA life plan trigger と直結、§6.1 不確実性要因 (1) 体制変動 + (5) personal life event の本算定への影響大
- 参照点に無い体制要因、本算定 §6 band 上方 +90% の根拠の一部 (要因 (1) 体制変動 +30% + (5) personal life event +50% の本算定への寄与)

#### AYAstorm 固有要因 合計

| 要因 | 本算定への寄与 |
|---|---|
| (i) 撮影描画用途 parity | +6.33 PM (3 OS 余裕係数前)、§3.4 + §3.5 |
| (ii) AYAstorm 13 file 追加 | +5.6 PM (shader + 機能 port、3 OS 余裕係数前) |
| (iii) Mac t-noami workflow | +4.05 PM (Mac 増分 余裕係数込) |
| (iv) 本職並走 4x | calendar 拡大 (フルタイム 3 年 → 並走 14 年中央値 / 27 年上方) |
| (v) 単独 1 人体制 | §6 band 上方 +90% の根拠の一部 (体制変動 + personal life event) |

→ 参照点に無い AYAstorm 固有要因が本算定 +16 PM (work 部分) + calendar 4x 拡大 + band +90% 上方 に直接寄与、参照点との差 (Doom +99% / Blender +2-4 年) の根拠が **明示的に説明可能**。

---

## §8 plan B trigger 条件 (charter §8 (B) 工程プラン破綻判定の閾値設定)

### §8.1 charter §8 (A)(C)(D)(E) との切り分け方針

charter §8 で trigger は 5 種類 ((A)-(E)) 定義済、本 §8 は **(B) 工程プラン破綻のみ** を扱う。他 trigger との切り分け:

| trigger | 性質 | 本 §8 範囲 |
|---|---|---|
| **(A) LL Vulkan 先着地** | 外部条件 (LL の release schedule) | 範囲外 (charter §7 LL 着地時判断指針 で別途扱い) |
| **(B) 工程プラン破綻** | **内部進捗 evaluation** (本算定 §5 中央値 / §6 上方 band 基準) | **本 §8 で扱う** |
| (C) AYA life plan 変更 | 外部条件 (個人事情) | 範囲外 (発動時の対処は charter §8 の trigger 機構で AYA 判断) |
| (D) 5 年経過 (2031-05-28) で LL Vulkan release ETA 未公開 | 外部条件 + 時間 trigger | 範囲外 (charter §8 で別 trigger として確立済、本 §8 とは独立) |
| (E) LL Vulkan release 着地 quality 不足 | 外部条件 (LL の質) | 範囲外 (charter §7 判断軸 2 で別途扱い) |

#### 本 §8 が (B) のみを扱う理由

- (A)(C)(D)(E) は **外部条件** に依存、本算定 §5/§6 から閾値を引けない
- (B) は **本算定 §5 中央値 + §6 band 上方** から定量的に閾値設定可能
- 本算定 §6.4 累積 band の上方 marker (r41 ~2042 年 / vk-RC ~2053 年) が (B) trigger 評価対象の reference

### §8.2 plan B trigger 閾値の定義方針

charter §8 (B) は例として「**r41 達成が 3 年経過しても未達** / **棚卸しで判明する portage 規模が想定の 2 倍以上**」を示している。本 §8 はこれを本算定 §5/§6 に基づいて **多軸 trigger** として定量化する。

#### 閾値設定の 5 軸

| 軸 | 性質 | 本算定参照 | 閾値設定の利点 |
|---|---|---|---|
| (a) **絶対暦月** (charter §8 (B) 例 1 と同型) | 1 milestone 達成までの実暦月が閾値超 | §5.6 marker 暦年 (r41 = ~2033 / vk-RC = ~2040 後半) + §6.4 上方 band | 単純 / 観測容易 / charter §8 (B) 例 1 を継承 |
| (b) **本算定中央値からの乖離 %** | 実暦月 / §5 中央値 が threshold 超 | §5.3 milestone 別 中央値 | 算定 base との直接比較 / 進捗 evaluation cadence と整合 |
| (c) **§6 上方 band 上限突破** | 実暦月 が §6 上方 band の累積上限突破 | §6.4 累積 band 上方 (r41 +120% / vk-RC +90% weighted) | 不確実性込の最悪シナリオ上限 / 突破は plan B 確定 signal |
| (d) **portage 規模の想定乖離** (charter §8 (B) 例 2 を継承) | 実 work LOC / shader 数 が a-3/a-4 想定の 2x 超 | foundation §1/§2 原単位 / §3 milestone work | scope creep 早期 detect / 棚卸し再評価 trigger |
| (e) **sub-milestone 完遂率** (charter §4 (3) 余裕係数 evaluation) | sub-milestone 達成率が予定の 50% 未満 (1 年 cycle) | §6.6 方策 (a) sub-milestone 区切り強化 | 早期 detect / annual review との整合 |

#### 閾値発動条件の優先

- **単独軸で threshold 突破**: 単一軸の突破 = warning 段階 (plan B 検討開始)
- **複数軸で threshold 突破** (例: (a) + (c) or (b) + (d)): plan B trigger 発動 (charter §8 (B) 正式判定)
- **(c) §6 上方 band 上限突破**: 単独で plan B trigger 発動 (本算定の最悪シナリオを超えた = 算定外、再評価必須)

#### 閾値設定の保守原則

- 本算定 §6 上方 band は **発生確率 ~10-15%** の最悪シナリオ、上限突破は本算定 base 完全外し
- 閾値設定は **本算定中央値の 1.5-2x** + **§6 上方 band 上限** の 2 段階構成
- 上方 band 上限突破は plan B trigger 自動発動、中央値 1.5-2x 突破は warning + 多軸 evaluation

### §8.3 r41 達成までの trigger 閾値

#### r41 達成 関連数値 (再掲)

| 算定 | 値 |
|---|---|
| §5.3 r41 中央値 | 84.08 暦月 (~7 年) |
| §5.6 r41 marker 暦年 | ~2033 年中 (着手 2026-05-28 + 7 年) |
| §6.4 r41 上方 (+120%) | 185 暦月 (~15.4 年、~2042 年) |
| §6.4 r41 下方 (-30%) | 59 暦月 (~4.9 年、~2031 年) |
| charter §8 (B) 例 1 | 「r41 達成が 3 年経過しても未達」 |

#### r41 trigger 閾値表

| 軸 | warning (plan B 検討開始) | trigger 発動 (plan B 正式判定) | 根拠 |
|---|---|---|---|
| (a) 絶対暦月 | r41 着手から **3 年経過しても未達** (charter §8 (B) 例 1 継承) | r41 着手から **10 年経過しても未達** | charter §8 (B) 例 1 が warning、§6 r41 中央値 7 年に対し 3 年は ~43% 進捗 |
| (b) 中央値からの乖離 | r41 着手後 5 年経過 (中央値 7 年の 71%) で進捗 30% 以下 | r41 着手後 12 年経過 (中央値 7 年の 171%) で未達 | 中央値の 1.71x 経過で未達 = §6 上方 band +120% の中盤 |
| (c) §6 上方 band 上限突破 | (該当なし、warning 段階で既に発動) | r41 着手から **15 年経過しても未達** (§6 上方 band 上限) | §6.4 r41 上方 ~15.4 年、上限突破 = 算定 base 完全外し |
| (d) portage 規模乖離 | a-3/a-4 棚卸しが追加 file (+30% 以上) 発掘 | 追加 file +100% 以上 (charter §8 (B) 例 2 「2 倍以上」) | charter §8 (B) 例 2 継承 |
| (e) sub-milestone 完遂率 | annual review で sub-milestone 完遂率 50% 未満 (§6.6 方策 (a) trigger) | 2 年連続で sub-milestone 完遂率 30% 未満 | §6.6 方策 (a) sub-milestone 区切り強化と連動 |

#### r41 trigger 発動時の判断

- **warning 段階**: §6.6 方策 (a)-(g) を発動、band 縮小努力 (sub-milestone 区切り強化 + 早期 prototype + LL 着地 reset 判断)
- **trigger 発動**: charter §8 (B) plan B 正式判定、対処 (α)-(ε) (§8.5) から選択

### §8.4 r42-α/β/γ/δ + r43-r44 達成までの trigger 閾値

#### 機能 milestone 別 trigger 閾値表

| milestone | warning 閾値 | trigger 閾値 (§6 上方 band 上限基準) | 根拠 |
|---|---|---|---|
| **r41.5 (VK repo 分離)** | 着手後 8 か月経過 (中央値 7.2 か月の 110%) | 着手後 16 か月経過 (中央値の 222%、§6 上方 band 13 か月の 123%) | r41.5 構造 refactor の short cycle、license 分離手続 unknown |
| **r42-α (picker port)** | 着手後 4 か月経過 (中央値 2.86 か月の 140%) | 着手後 8 か月経過 (中央値の 280%、§6 上方 band 4.3 か月の 186%) | foundation 帰属が大半、追加 work 軽量、long delay は別要因 |
| **r42-β (Cinematic port)** | 着手後 18 か月経過 (中央値 13.86 か月の 130%) | 着手後 30 か月経過 (中央値の 217%、§6 上方 band 22.2 か月の 135%) | Cinematic frame context refactor + DoF state enum |
| **r42-γ (visual realism port)** | 着手後 18 か月経過 (中央値 13.99 か月の 129%) | 着手後 32 か月経過 (中央値の 229%、§6 上方 band 23.8 か月の 134%) | visual realism post-process + perf profile + visual A/B iterate |
| **r42-δ (parity 残機能 / vk-RC 直前 polish)** | 着手後 14 か月経過 (中央値 9.90 か月の 141%) | 着手後 26 か月経過 (中央値の 263%、§6 上方 band 18.8 か月の 138%) | a-3 範囲外、parity 残機能発掘 cost 高 unknown |
| **r43-r44 (parity 補強 / Mac portable subset)** | 着手後 16 か月経過 (中央値 12.00 か月の 133%) | 着手後 30 か月経過 (中央値の 250%、§6 上方 band 24 か月の 125%) | Mac MoltenVK + Win driver matrix + 性能 polish iterate |
| **vk-RC 3 OS parity 完遂 (累積)** | r40 着手から **18 年経過しても未達** (中央値 14 年の 129%) | r40 着手から **28 年経過しても未達** (§6 上方 band 27 年の 104%、charter 上限 30 年の 93%) | charter §4 (3) 上限 30 年に近接、本算定 §6 上方の 104% で 算定外 |

#### 機能 milestone 別 warning 設計の原則

- warning 閾値は **中央値 +30-40% 超** (個別 milestone の不確実性を考慮、§6.3 milestone 別 band +50-120% の半分程度を warning に設定)
- trigger 閾値は **§6 上方 band 上限超 (+5-90% upward)** で算定 base 完全外しを判定 (milestone 別の band 補正を反映)
- 各 milestone trigger 発動時は同 milestone の 修正 + 後続 milestone への trigger 閾値 再評価が necessary

#### portage 規模乖離 (charter §8 (B) 例 2 継承) の機能 milestone 別判定

- r42-α/β/γ/δ で追加 file +50% 以上発掘で warning、+100% 以上 (charter §8 (B) 例 2) で trigger
- r43-r44 で Mac MoltenVK + Win driver matrix の untested feature が想定 +100% 以上発掘で trigger
- portage 規模乖離は milestone 着手前 (棚卸し再評価) と着手中 (実装中の発掘) の 2 timing で判定

### §8.5 trigger 発火時の対処

charter §8 (B) plan B trigger 発動時の対処を 5 案として定義 (charter §7 LL 着地時判断指針 + §6.6 band 縮小方策と連動):

#### 対処 5 案

| # | 対処 | 内容 | 適用 case |
|---|---|---|---|
| **(α) scope 縮小** | charter §4 (1) parity 完遂 goal を一部 feature 単位で r45+ に押し出し | §6.6 方策 (e) sustained 発動、charter §4 (1) parity 範囲の re-define、AYAstorm 機能の trim (撮影描画 core 維持 + edge feature drop) | r41 trigger 発動 (絶対暦月 10 年経過) / r42-δ 以降の trigger 発動 |
| **(β) quality 緩和** | parity 完遂条件を質的緩和 (例: visual realism は AYAstorm r14-r20 のみ port、r21-r24 drop) | §3.5 r42-γ 工数 50% reduction、charter §4 (1) との trade-off | r42-γ trigger 発動 (visual realism iterate cost が想定超え) |
| **(γ) 別 viewer base 接続** | AYAstorm 機能を別 viewer base (例: Alchemy Vulkan / 別 third-party) に port、本線 = 機能 host、別 viewer = Vulkan engine | r41.5 達成後の defensibility 活用 (charter §4 (4) 法的分離 + dynamic link 構成 + §7 判断軸 3)、AYAstorm GUI = LGPL 維持 + 別 viewer Vulkan engine 採用 | r43-r44 trigger 発動 (Mac MoltenVK + driver matrix で別 OS 完遂 unlikely) |
| **(δ) LL 着地 reset** | charter §7 LL 着地時判断指針発動、LL 公式 VK に乗り換え (vk-α 前 / vk-α 直後で reset cost 低い場合) | charter §7 判断軸 1 (進捗 stage) + 判断軸 2 (LL 質的評価) + 判断軸 3 (r41.5 後 UI 変更時 defensibility) | LL 着地 trigger との同時発火、r41 trigger 発動 + LL 着地で reset 検討 |
| **(ε) 撤退** | r40 章を close、AYAstorm を GL 維持 mode (LL upstream WebRTC 移行で延命) で continue | charter §8 (C) AYA life plan trigger との同時発火、最終手段、charter §3 「時間軸では撤退条件設けない」と切り分け (本 (ε) は工程破綻 + 個人事情の同時発火に限定) | (C) AYA life plan trigger と (B) trigger 同時発火、r43-r44 trigger 発動 + 体制崩壊の同時発火 |

#### 対処の優先順位 (本算定からの推奨)

1. **(δ) LL 着地 reset** (charter §7 + §8 (A) trigger 連動時、最優先)
2. **(α) scope 縮小** (charter §4 (1) parity 範囲 re-define、§6.6 方策 (e) 継承)
3. **(γ) 別 viewer base 接続** (r41.5 達成後の defensibility 活用、Mac 等の specific OS 局所適用)
4. **(β) quality 緩和** (AYAstorm 撮影描画 core 維持 + edge feature drop、最終手段の 1 つ前)
5. **(ε) 撤退** ((C) との同時発火に限定、最終手段)

#### trigger 発動時の判断主体

- 判断は AYA さん、Claude は判断材料 ((a)-(e) trigger 軸の現状 + (α)-(ε) 対処の比較表) を提供
- charter §7 「判断は AYA さんが行う、Claude は判断材料を提供する」と整合
- 判断 cadence は §8.6 で定義

### §8.6 trigger 判定 cadence

#### 判定 cadence の 4 種

| cadence | 頻度 | 判定軸 | 判定 trigger |
|---|---|---|---|
| **annual review** | 1 年に 1 回 (毎年 5 月末 = r40 着手 2026-05-28 周年) | (a) 絶対暦月 + (b) 中央値乖離 + (e) sub-milestone 完遂率 | warning 閾値突破の早期 detect、§6.6 方策 (a) sub-milestone 区切り強化と整合 |
| **milestone 完遂時** | 各 milestone 完遂時 (r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC parity) | (a) 絶対暦月 + (b) 中央値乖離 + (c) §6 上方 band 上限 | milestone 単位の trigger 評価、後続 milestone trigger 閾値の再評価 |
| **棚卸し再評価時** | r41 着手前 (work item (a) 再評価) + r42 着手前 (a-3/a-4 補完) + r43 着手前 (Mac portable subset 詳細化) | (d) portage 規模乖離 | charter §8 (B) 例 2 「想定の 2 倍以上」の判定、scope creep 早期 detect |
| **emergency** | trigger 発動 (上記 cadence で warning + trigger 同時突破時) | 全軸 | charter §8 (B) plan B 正式判定 + 対処 (α)-(ε) 選択、AYA 判断 |

#### cadence と §6.6 方策との整合

- **annual review** = §6.6 方策 (a) sub-milestone 区切り強化 / (b) 早期 prototype の trigger 評価 cadence
- **milestone 完遂時** = §6.6 方策 (a)-(g) の発動 cadence (各方策の trigger 条件と整合)
- **棚卸し再評価時** = §6.6 方策 (g) shader cross compile chain 自動化強化 の trigger 評価 cadence (foundation §2 完了後の継続的整備)
- **emergency** = 対処 (α)-(ε) の発動 cadence、AYA 判断

#### 判定結果の記録

- 各 cadence の判定結果は本算定 §5/§6 と同 doc (06-effort-estimation.md) の **§8.7 progress log** (本 §8 完成後の operational 区分として placeholder 化) に記録
- annual review log は AYAstorm release note に summary 出力 (AYA 判断、Claude が draft 作成)
- milestone 完遂時 log は milestone 各 charter doc (例: r41 charter / r41.5 charter) に記録
- emergency 判定は charter §8 (B) trigger 発動として本 charter (00-charter.md) の更新

### §8.7 progress log (operational placeholder)

- 本 §8.7 は work item (c) draft 完了後の **operational log** として継続更新、本 (c) 完了宣言には含めない
- annual review log の placeholder (2027-05-28 第 1 回 review 時に追加)
- milestone 完遂時 log の placeholder (r41 達成 ~2033 年中の予定)
- emergency 判定 log の placeholder (charter §8 (B) 発動時のみ)

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
3. ~~group A (§3 per-milestone + §4 3 OS) draft 着手~~ ✓ 完了 (§3.9 Linux baseline = 21.07 PM work / 29.90 PM 余裕係数適用後 平均 +42%、§4.5 3 OS 合計 = 25.12 PM work / 35.84 PM 余裕係数適用後、フルタイム dev)
4. ~~group B (§5 milestone 月数 + §6 uncertainty band) draft 着手~~ ✓ 完了 (§5 並走 4x × 学習曲線 weighted +18.7% / vk-RC 3 OS parity 完遂 中央値 ~170 暦月 = 14.17 年 / §6 band 上方 +90% = ~27 年 / 下方 -30% = ~10 年)
5. ~~group C (§7 Doom Blender 比較 + §8 plan B trigger) draft 着手~~ ✓ 完了 (§7 Doom 18-36 PM の 99% 整合 ✓ + Blender 10-12 年規模との broad scale 整合 ✓ + charter §4 (3) 15-30 年想定の下限-上限近接 整合 ✓、§8 plan B trigger 多軸 5 案 + 対処 5 案 + 判定 cadence 4 種を確定)
6. **group C review → work item (c) 完了宣言 → work item (d) r42+ 区切り確定 着手** ← 次

### foundation 算出値 (group A draft で消化済)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §1.7 C++ critical path 合計 | **7.77 PM** | フルタイム dev | §3 milestone に振り分け済 |
| §2.5 shader 合計 | **5.15 PM** | フルタイム dev | §3 milestone に振り分け済 |
| **foundation 合計** | **~12.92 PM** | フルタイム dev | §3 で消化、§5/§6 で本職並走 ratio + 学習曲線 + uncertainty band 適用済 |

### group A 算出値 (group B draft で消化済)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §3.9 milestone work sum (Linux baseline、余裕係数前) | **21.07 PM** | フルタイム dev | §5 で本職並走 ratio + 学習曲線適用済 |
| §3.9 milestone work sum (Linux baseline、余裕係数適用後 平均 +42%) | **29.90 PM** | フルタイム dev | §5 で暦月変換済、§6 で uncertainty band 適用済 |
| §4.2 Win 増分 (余裕係数適用後 +40%) | **1.89 PM** | フルタイム dev | §5 OS 別 timing 反映済 |
| §4.3 Mac 増分 (余裕係数適用後 +50%) | **4.05 PM** | フルタイム dev | §5 OS 別 timing 反映済 |
| §4.5 3 OS 合計 (余裕係数適用後) | **~35.84 PM** | フルタイム dev | §5 で 3 OS 完遂までの暦月変換済 |

### group B 算出値 (group C draft で消化済)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §5.1 本職並走 ratio 確定 | **4x** (フルタイム dev 1 PM = AYA 並走 4 暦月) | calendar/PM | §5.3 milestone 別暦月変換、§6 band 算定の base |
| §5.2 学習曲線 weighted PM (Linux baseline) | **~35.97 weighted PM** (weighted avg +20.4%) | フルタイム dev | §5.3 計算式 input |
| §5.2 学習曲線 weighted PM (3 OS 合計) | **~42.51 weighted PM** (weighted avg +18.7%) | フルタイム dev | §5.3 計算式 input |
| §5.3 vk-RC 3 OS parity 完遂 中央値 | **~170 暦月 (~14.17 年)** | 本職並走 calendar | §5.4 累積、§6 band 適用 base、§7.2/§7.3/§7.4 参照点比較で再消化 |
| §5.5 charter §4 (3) 整合判定 | 中央値 14.17 年 (charter 下限 15 年の 94%) + 上方 27 年 (charter 上限 30 年の 90%) で **整合範囲内** | 本職並走 calendar | §7.3 charter cross check で再消化 |
| §5.6 marker 暦年 (r41 達成 / vk-RC 3 OS parity 完遂) | **r41 = ~2033 年中 / vk-RC = ~2040 年後半** | 進捗 marker (撤退条件には使わない) | §8.3 r41 trigger 閾値 + §8.4 vk-RC trigger 閾値の reference |
| §6.2 base band (上方 +90% / 下方 -30%) | 8 要因 × 振れ幅 の正 correl 寄せ合成 | PM | §6.3 milestone 別 band の base、§8.2 trigger 閾値 (c) 軸の base |
| §6.4 累積 band (vk-RC 3 OS parity 完遂) | **上方 +90% = ~323 暦月 (~26.9 年、~2053 年) / 下方 -30% = ~119 暦月 (~9.9 年、~2036 年)** | 本職並走 calendar | §8.4 vk-RC trigger 閾値 28 年 (上方 27 年の 104%) の reference |
| §6.5 3 シナリオ tabulation (charter §4 (3) 想定 15-30 年 との整合) | 上方 27 年 (上限 90%) + 中央 14 年 (下限 94%) + 下方 10 年 (下限を下回るが low-likelihood) で **整合範囲内** | 本職並走 calendar | §7.3 charter cross check で再消化 |
| §6.6 band を縮める方策 (a)-(g) 7 件 | 上方 band を +90% → +50-60% に圧縮で ~22 年に短縮可能 | 方策 list | §8.5 trigger 発火時の対処 (α)(δ)、§8.6 判定 cadence と連動 |

### group C 算出値 (work item (c) 完了宣言 base 値)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §7.1 Doom 整合判定 | 本算定 35.84 PM = Doom 18-36 PM 上限 36 PM の **99%**、broad scale 整合 ✓ | PM 比較 | work item (c) 完了宣言の妥当性根拠 |
| §7.2 Blender 整合判定 | 本算定 14.17 年 = Blender 10-12 年規模との **+2-4 年差** (体制 1 名固定 + abstraction 後追い + 3 OS parity + AYAstorm 撮影描画用途 で説明可能)、broad scale 整合 ✓ | 年比較 | work item (c) 完了宣言の妥当性根拠 |
| §7.3 charter §4 (3) cross check | 本算定中央値 + 上方 band が charter 15-30 年想定の **下限-上限 範囲内** (中央値 14 年 = 下限 94% + 上方 27 年 = 上限 90%)、整合 ✓ | 年比較 | charter §4 (3) top-down 推定の bottom-up 裏付け確認 |
| §7.4 ズレ要因 (Doom +99% / Blender +2-4 年 / charter 下限近接) | abstraction 不在 / 13 file 追加 / 3 OS / 本職並走 / 単独 1 名体制 の 5 要因で説明可能 | 要因 list | work item (c) 完了後 AYA review 材料 |
| §7.5 参照点に無い AYAstorm 固有要因 5 件 | (i) 撮影描画 +6.33 PM / (ii) 13 file +5.6 PM / (iii) Mac +4.05 PM / (iv) 並走 4x / (v) 1 名体制 が band +90% 寄与 | 要因 list | work item (c) 完了後 AYA review 材料 |
| §8.2 trigger 閾値 5 軸 | (a) 絶対暦月 / (b) 中央値乖離 / (c) §6 上方 band 上限 / (d) portage 規模乖離 / (e) sub-milestone 完遂率 | 軸 list | charter §8 (B) plan B 判定の運用 base |
| §8.3 r41 trigger 閾値 (warning 3 年 / trigger 10 年 / band 上限 15 年) | charter §8 (B) 例 1 「3 年経過未達」を warning に継承、§6.4 r41 上方 band 15.4 年を trigger 閾値 (c) に採用 | 閾値表 | r41 着手後の annual review base |
| §8.4 r42-α/β/γ/δ + r43-r44 + vk-RC trigger 閾値 | 各 milestone 中央値 × 1.30-1.41 = warning、§6 上方 band 上限 × 1.05-1.25 = trigger、vk-RC 累積 28 年で trigger | 閾値表 | 機能 milestone 着手後の milestone 完遂時 review base |
| §8.5 trigger 発火時の対処 5 案 | (α) scope 縮小 / (β) quality 緩和 / (γ) 別 viewer base 接続 / (δ) LL 着地 reset / (ε) 撤退 (charter §8 (C) 同時発火限定) | 対処 list | trigger 発動時の AYA 判断材料 |
| §8.6 判定 cadence 4 種 | annual review (5 月末) + milestone 完遂時 + 棚卸し再評価時 + emergency | cadence list | r41 着手後の運用 base |

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
