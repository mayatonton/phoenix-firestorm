# r40 sub-phase 3 work item (c): 工程算定

**status**: foundation + group A (§3 per-milestone + §4 per-OS) draft 完了 — §5-§8 は group B/C で順次 draft 予定
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

**status**: group B (次) で draft

draft 予定の項目:
- §5.1 AYA 本職並走 ratio 確定 (フルタイム dev 1 人月 = AYA 並走 N 暦月、charter §4 (3) 想定 3-5x の精緻化)
- §5.2 Vulkan 学習曲線 反映 (r41 序盤 + r41.5 で +20-30% / r42+ 以降は inline 化想定)
- §5.3 milestone 別 所要暦月 (r41 / r41.5 / r42-α/β/γ/δ / r43 / r44) 中央値
- §5.4 milestone 累積 所要暦月 (r41 達成までの year scale + r45+ までの total year scale)
- §5.5 charter §4 (3) 6-15 年想定との整合性 verification
- §5.6 marker 暦年 (例: r41 達成は 202X 年頃、r42-γ は 202Y 年頃) — 撤退条件には使わない (charter §8 (A))、進捗 marker のみ

---

## §6 算定の uncertainty band (上方 / 下方)

**status**: group B (次) で draft

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
3. ~~group A (§3 per-milestone + §4 3 OS) draft 着手~~ ✓ 完了 (§3.9 Linux baseline = 21.07 PM work / 29.90 PM 余裕係数適用後 平均 +42%、§4.5 3 OS 合計 = 25.12 PM work / 35.84 PM 余裕係数適用後、フルタイム dev)
4. **group A review → group B (§5 月数 + §6 uncertainty) draft 着手** ← 次
5. group C (§7 Doom Blender + §8 plan B) draft
6. 8 section 揃ったら work item (c) 完了宣言、work item (d) r42+ 区切り確定 着手

### foundation 算出値 (group A draft で消化済)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §1.7 C++ critical path 合計 | **7.77 PM** | フルタイム dev | §3 milestone に振り分け済 |
| §2.5 shader 合計 | **5.15 PM** | フルタイム dev | §3 milestone に振り分け済 |
| **foundation 合計** | **~12.92 PM** | フルタイム dev | §3 で消化、§5/§6 で本職並走 ratio + 学習曲線 + uncertainty band 適用予定 |

### group A 算出値 (group B 以降の base 値)

| section | 算出値 | 単位 | 用途 |
|---|---|---|---|
| §3.9 milestone work sum (Linux baseline、余裕係数前) | **21.07 PM** | フルタイム dev | §5 で本職並走 ratio + 学習曲線適用の base |
| §3.9 milestone work sum (Linux baseline、余裕係数適用後 平均 +42%) | **29.90 PM** | フルタイム dev | §5 で暦月変換、§6 で uncertainty band |
| §4.2 Win 増分 (余裕係数適用後 +40%) | **1.89 PM** | フルタイム dev | §5 OS 別 timing 反映、§4.4 と整合 |
| §4.3 Mac 増分 (余裕係数適用後 +50%) | **4.05 PM** | フルタイム dev | §5 OS 別 timing 反映、§4.4 と整合 |
| §4.5 3 OS 合計 (余裕係数適用後) | **~35.84 PM** | フルタイム dev | §5 で 3 OS 完遂までの暦月変換 base |

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
