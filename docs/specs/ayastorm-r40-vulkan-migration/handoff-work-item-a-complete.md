# handoff: work item (a) Vulkan portage 棚卸し 完了 → work item (b) Vulkan API 設計 着手前

**作成日**: 2026-05-28
**前 session 完了範囲**: work item (a) 全 sub-step (a-1 / a-2 / a-3 / a-4) 完了
**次 session 開始 task**: work item (b) Vulkan API 設計 着手 (+ 着手前に charter / memory 反映 judgment)

---

## 1. 完了済 work item (a) の sub-step + 出力 doc

### sub-step 完了状況

| sub-step | 内容 | 完了 |
|---|---|---|
| a-1 | repo 現状棚卸し (file list / LOC / GL call 数 / dependency 範囲) — Explore agent 4 並列 | ✅ |
| a-2 | per-file 4 軸 verdict draft (要 port / 要再設計 / 不要 port / 判定保留) — Explore agent 1 | ✅ |
| a-3 | 判定保留 17 file 確定 + AYAstorm 影響軸詳細化 + 段階 port 戦略具体ステップ — Explore agent 1 | ✅ |
| a-4 | 棚卸し総括 (4 軸全合計 final 確定 + 段階 port 戦略 final + (b) 引継ぎ事項) — Claude 統合 | ✅ |

### 出力 doc

- `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` (final 版、§1-§6 全埋め)

---

## 2. work item (a) で確定した重要数字 (b 設計の input)

### portage 真の規模 (04 doc §6.1)

| 領域 | LOC | 性質 |
|---|---|---|
| indra/llrender/ | 28,168 | GL API 直叩き層 (51 file) |
| pipeline.cpp + .h | 15,954 | 描画 orchestrator + 3 大グローバル 176 参照 |
| lldrawpool*.cpp | 7,907 | 各 pool 描画 dispatcher (13 file 全て要 port) |
| llspatialpartition.cpp | 4,416 | geometry rebuild + occlusion |
| llviewershadermgr.{cpp,h} | 4,423 | shader manager |
| llvosky.cpp + llvowlsky.cpp | 2,198 | sky dome + atmospherics (r14+ 関連) |
| **C++ critical path 合計** | **63,066 LOC ≈ 63K** | |
| GLSL shader (SPIR-V 移行対象) | **248 file** | class1/2/3 deferred + interface + lighting + windlight + cinematic_bd |
| GL header 依存 file | **189** (3,098 中 6.1%) | **99% は llgl.h wrapper 経由** = abstraction 設計の追い風 |

→ charter §4 (2) の **28.2K LOC 想定の 2.2 倍** に拡大 (lldrawpool + llviewershadermgr + llvosky/llvowlsky が見落としだった)

### 4 軸全合計 (04 doc §6.2)

| 分類 | file 数 | LOC |
|---|---|---|
| 要 port | 33 | 20,749 |
| 要再設計 | 14 | 41,121 |
| 不要 port | 4 | 1,782 |
| **合計** | **51** | **63,652** |

### shader 移行リスク評価 (04 doc §1.3)

- compute / geometry / tessellation: **全部ゼロ**
- bindless / atomic / coherent: **全部ゼロ**
- sampler 数: 206 (descriptor set 再設計対象)
- matrix uniform: 252 mat4
- → cross compile (glslang) で **~85% 素直に通る見込み**

### 段階 port 戦略 final (04 doc §6.3)

#### base LL port (r41 = GL 除去 + Vulkan 空転) 5 段階

1. GL header wrapper 置換 (llglheaders.h + llglstates.h + llgltypes.h → Vulkan header + volk loader)
2. lldrawpool 全 13 file の Vulkan command buffer 化 (terrain.cpp glTexGen → shader 側 explicit UV)
3. llgl / llrender / llimagegl / llrendertarget / llpostprocess の state machine → PSO 化
4. **pipeline.cpp 3 大グローバル → frame context 化 + render stage VkRenderPass chain (最難関)**
5. llspatialpartition / llviewershadermgr / llvertexbuffer 等の依存解決

#### AYAstorm 3 機能 patch 合成順 (r42 以降)

- r42-α: r21.1 self-rigged picker (mObjectIDBuffer → render pass attachment、shader 2 file)
- r42-β: r30 Cinematic mode (DoF state enum 化、shader 4 file)
- r42-γ: r14+ visual realism (post-process pass chain、shader 7 file + llvosky/llvowlsky port)

---

## 3. work item (b) Vulkan API 設計 への引継ぎ事項 (04 doc §6.4)

### §6.4.1 必須採用の Vulkan feature / extension (b で確定する design input)

| feature | 用途 | 段階対応 |
|---|---|---|
| **Vulkan 1.3 default + MoltenVK 1.2 互換性** | Linux/Win native + Mac 後追い | base 全体 |
| **volk loader** | header-only Vulkan loader | 段階 1 |
| **VMA (Vulkan Memory Allocator)** | llimagegl.cpp staging buffer 化 | 段階 3 |
| **glslang + spirv-cross** | shader SPIR-V crosscompile (248 + 改変 13 file) | 全段階 |
| **VkRenderPass + VkFramebuffer** (Vulkan 1.3 dynamic rendering 採用検討) | llrendertarget / llpostprocess 再設計 | 段階 3 |
| **VkQueryPool** | occlusion query (llspatialpartition) | 段階 5 |
| **descriptor set 2-3 個** (per-frame / per-material / per-draw) | sampler 206 個収容 | 全段階 |

### §6.4.2 設計検討事項 (b で詰める)

1. **frame context 設計** (LLPipelineFrameContext 仮称): 3 大グローバル sCull / sShadowRender / sCurCameraID + DoFMode enum (Cinematic) + AYAstorm picker buffer の集約方式
2. **PSO cardinality 事前列挙**: llgl / llrender state machine の state combination 全列挙、PSO cache 戦略
3. **terrain.cpp glTexGen 廃止 shader**: shader 側 explicit UV 計算の vert / frag 設計
4. **post-process pass chain**: r14+ godrays / volumetricLight / blurLight / vignette + r30 DoF を統合した VkRenderPass chain
5. **r21.1 picker attachment 統合方針**: deferred main pass 内 inline attachment vs 別 pass

### §6.4.3 設計しない / 後送り事項

- **abstraction interface 詳細**: r41.5 milestone (本線 ↔ VK repo 分離) で詰める、(b) では「分離可能な skeleton」のみ
- **Mac MoltenVK 制約詳細**: vk-RC 直前の Mac 追加 phase、(b) では「Linux 先行 + Mac 互換性 maintain 方針」のみ
- **GPU-driven culling / compute shader 化**: 段階 4 内で個別判断、(b) では「手動 port 可能性のみ確保」

### (b) 出力 doc (起草予定)

`docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` (03 doc §4 で定義済の 10 section 構成)

---

## 4. 未反映の charter / memory 更新候補 (04 doc §6.5)

### 4.1 `00-charter.md` 更新候補

| section | 旧記述 | 新記述 (a-4 final 反映) |
|---|---|---|
| **§4 (2) scope 数字** | 「indra/llrender 16 files / 28.2K LOC / 464 GL calls + GL header 212 files 全置換」 | 「**C++ critical path ~63K LOC** (llrender 28K + pipeline.cpp 16K + lldrawpool 8K + llspatialpartition 4K + llviewershadermgr 4K + llvosky/llvowlsky 2K) + **GLSL shader 248 file** + GL header 依存 189 file (**99% wrapper 経由**)」 |
| **§4 (2) 補足** | (なし) | wrapper 局在化 (llglheaders / llglstates / llgltypes 3 file 置換で 188 file 対応) を明示、abstraction 設計の追い風として記録 |
| **§6 r42+ ロードマップ** | 仮 line up (vk-β / vk-γ / vk-δ / vk-RC) | a-3 §5.4.2 AYAstorm 3 機能合成順 (r42-α picker → r42-β Cinematic → r42-γ visual realism) を本 line up に格上げ |
| **§4 (3) 工数** | 6-15 人年 / AYA life plan | フルタイム dev 7-8 人月 vs 6-15 人年 = 並走係数 3-5x + 学習曲線 + 不確実性 2-3x を補足、(c) 工程算定で精緻化と明示 |

### 4.2 `project_ayastorm_r40_cpu_parallel.md` 更新候補

- **§sub-phase 3 工程プラン確定事項 (2) scope**: charter §4 (2) と同期、portage 規模数字を a-4 final に置換
- **a-4 で判明した新発見**: lldrawpool 見落としや header wrapper 局在化追い風など、memory 達成条件の中身として記録

### 4.3 反映 timing (推奨)

**(b) 着手前** に charter + memory 数字反映を推奨。理由:
- (b) Vulkan API 設計 doc 内で charter §4 (2) scope を何度も参照する
- charter / memory が古い数字のままだと (b) doc 内の参照が混乱

ただし AYA は前 session で「ちょっと進めてみないと精度わからないよね」と言って memory 更新を後送りする判断を示した。(c) 工程算定で更に数字精緻化が起きる可能性があるので、(b) 着手前更新 vs 全 work item 完了後一括更新は AYA judgment 仰ぐ。

---

## 5. 次 session 開始時の最初の task list

### task 1: handoff doc 確認 + work item (a) final 状態の把握

1. 本 handoff doc を Read
2. `04-portage-inventory.md` final 版を Read (§6 中心、§1-§5 は参考)
3. `03-sub-phase-3-vulkan-plan.md` の §2 work item table で (a) 完了 / (b) 着手前 確認

### task 2: charter / memory 反映 judgment (AYA に質問)

§4.3 反映 timing の選択肢:
- **(BA)** (b) 着手前に charter / memory 反映 → (b) 着手 (推奨、参照混乱回避)
- **(BB)** (b) 着手 → (b) 完了後にまとめて反映 ((c) 数字精緻化を待つ)
- **(BC)** work item (a-e) 全完了後に一括反映 (charter は r40 達成 doc として final 化のタイミングで)

AYA 判断後、選択された path を実行。

### task 3: work item (b) Vulkan API 設計 起草

03 doc §4 (work item (b) Vulkan API 設計) の構成に従い、`05-vulkan-api-design.md` を起草:

- §1 Vulkan version + loader + SDK 選定根拠
- §2 shader cross compile chain (glslang / spirv-cross)
- §3 descriptor set / pipeline layout 設計図 (sampler 206 個収容)
- §4 render pass / framebuffer (deferred g-buffer の Vulkan 表現)
- §5 sync 戦略 (state diagram / barrier table)
- §6 memory allocator 方針 (VMA)
- §7 swapchain / present mode
- §8 3 OS 対応詳細 (Mac MoltenVK 制約含む)
- §9 extension 採用 list (vulkan-1.3 base + 必須 extension)
- §10 abstraction interface 設計 (r41.5 分離 skeleton のみ)

work item (a) §6.4.1 必須採用 7 件 + §6.4.2 設計検討事項 5 件 + §6.4.3 後送り事項 3 件 を直接 input として draft 化。

### task 4: 03 doc §2 work item table を (b) 着手後の status に更新

(b) 着手宣言時、03 doc §2 で (b) status を「着手前」→「draft 作成中」に更新。

---

## 6. 内部状態の特記事項 / AYA との合意事項 (前 session 内)

### 6.1 a-3 工数見積の重要注意 (04 doc §5.4.3)

a-3 で出した工数見積 (base 4-5 人月 + AYAstorm 3 人月 = **合計 7-8 人月**) は **フルタイム dev / 経験者前提**。charter §4 (3) **6-15 人年 / AYA 本職並走** とは前提条件が大きく異なる。

| 軸 | a-3 工数見積 | charter §4 (3) |
|---|---|---|
| 体制 | フルタイム dev 1 人 / 経験者 | AYA 本職並走 1 人 / Vulkan 初見 |
| 期間 | 7-8 人月 | 6-15 人年 |
| 不確実性 | 上方 1.5x | 上方 2-3x |

**乖離理由**: 並走係数 3-5x + 学習曲線 + 不確実性 2-3x + Doom 2016 / Blender Vulkan 参照点

→ (b) 設計時には a-3 工数見積を直接使わない、絶対値は (c) 工程算定で精緻化される。

### 6.2 a-2 → a-3 数字精緻化の経緯

- a-2 §2.4 で「Vulkan portage critical path 合計 = **~66K LOC**」と書いた
- a-3 §5.5 で「a-2 ~66K → a-3 ~51.6K に精緻化」と書いた (header 二重計上整理による)
- a-4 §6.1 で「**63,066 LOC ≈ 63K**」が正解と確定 (a-3 §5.5 の 51.6K は判定保留 LOC を差し引きすぎていた)

→ **final 数字は a-4 §6.1 の 63K LOC**、(b) 設計時には a-4 数字を採用。

### 6.3 AYAstorm 3 機能 modular 性の確定

a-2 / a-3 で AYAstorm 3 機能 (r21.1 picker / r30 Cinematic / r14+ visual realism) の touchpoint が **局在** していることが確定 (合計 20-30 call、base portage の 0.03% 未満)。

→ **base LL port 完成後に modular patch 合成可能** = charter §6 r42+ ロードマップを (r42-α/β/γ) 順序で確定する根拠、(b) 設計時に「base + AYAstorm 分離設計」を貫く。

### 6.4 段階 1 (GL header wrapper 置換) の優先理由

a-1 §1.2 で「**GL header 依存 99% は llgl.h wrapper 経由**」を確定 → llglheaders.h + llglstates.h + llgltypes.h の 3 file 置換で 188 file の wrapper 経由 file が変更不要、abstraction 設計の最大の追い風。

→ (b) 設計時に「**段階 1 の wrapper 置換** を最初の milestone として正式化、(c) 工程算定で 0.5 人月の confidence high を担保」。

---

## 7. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細 (CPU perf 全 REJECT)
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細 (鉱脈ゼロ)
- `03-sub-phase-3-vulkan-plan.md` — sub-phase 3 work item plan (§2 status table で (a) 完了 / (b) 着手前)
- `04-portage-inventory.md` — work item (a) final 版 (本 handoff の主要 input)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (§6.5.2 で更新候補)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 memory
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (GL 除去 + Vulkan 空転)、r40 達成後に着手
- `project_ayastorm_r21_self_rigged_picker.md` — r21.1 picker (AYAstorm 影響 §B.1 で参照)
- `project_ayastorm_r30_cinematic_chapter.md` — r30 Cinematic (AYAstorm 影響 §B.3 で参照)
- `project_ayastorm_visual_realism_chapter.md` — r14+ visual realism (AYAstorm 影響 §B.2 で参照)
- `project_r30_cinematic_control_tuning_deferred.md` — r30 BD live cvar 13 件 (DoF state 構造設計時参照)
- `feedback_proactive_handoff.md` — 本 handoff doc 作成根拠

---

## 8. 次 session キックオフ template (AYA → Claude)

次 session 開始時、AYA は以下のような開始メッセージを投げると Claude が context 即把握できる:

```
r40 sub-phase 3 work item (a) 完了で前回 handoff した。
handoff doc 確認して、(b) Vulkan API 設計に進む準備をして。
charter / memory 反映 timing は handoff §5 task 2 の選択肢で recommend してほしい。
```

Claude 側は:
1. 本 handoff doc を Read
2. 04 doc final 版を Read (§6 中心)
3. 03 doc §2 status table 確認
4. §5 task 2 (charter / memory 反映 timing) の推奨を提示
5. AYA judgment 後、選択された path を実行 → (b) 着手
