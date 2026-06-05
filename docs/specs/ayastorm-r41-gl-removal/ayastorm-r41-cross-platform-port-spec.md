# AYAstorm r41 Cross-Platform Port Spec (stub)

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**起案契機**: AYA 指示「(PC-8-doc-1) B + (PC-8-doc-2) A で起案お願いします」literal 受領 (2026-06-05)。PC-8 marker doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-8-linux-primary-marker.md`) §4 で stub 起案 record。
**目的**: AYAstorm r41 Vulkan migration の Linux primary 完成後、Mac/Win 開発者 (= AYAstorm Mac は @t-noami モデル踏襲想定 + Windows 開発者) が補完作業を実施する際に必要な **OS 依存性 + 既知制約 + Linux 環境 spec** を集約する spec doc の **stub**。Phase 1.D 以降の各 PC-* sub-step で随時 OS 依存懸念追記、**Phase 1 全完了時に確定形で Mac/Win 開発者に提供**。

> **本 doc 位置付け**: AYA 方針 (= 「Linux primary 完成 → 他者補完」literal 2026-06-05) に基づく Mac/Win 補完 phase 向け資料の起点。memory `project_ayastorm_three_platforms` 「3 OS 揃え原則」は **「Linux primary 完成 → Mac/Win 派生 fix で補完」モデル**で実現。

---

## §0. AYA 方針 record

AYA literal「WindowsとMacOSですが、同時に開発する計画にありません。まずLinuxで完成の後、それを提供してつないでもらう（足りないところを追加してもらう）そういう考えています」(2026-06-05)

採用根拠:
- macOS MoltenVK 制約 (= descriptor set 数 + Vulkan version) は本質的、Linux 先行で検出不可
- 早期 macOS 制約考慮で Linux 開発進行遅延回避
- Phase 1 全完了 = Linux 動作確定 → macOS / Windows 移植時に制約発覚 → Linux 設計から派生 fix で対応 (= Linux primary 完成度高いほど派生 fix 範囲明確)
- 他開発者補完戦略 (= Mac は @t-noami 既往モデル + Windows 開発者) = AYAstorm の従来モデル踏襲

---

## §1. Linux 環境 spec (= 取得要確認箇所は Phase 1.D 以降 literal 確定)

| 項目 | 値 | 確定状況 |
|------|---|---------|
| 開発 OS | Linux x86_64 (Ubuntu 想定) | ✅ literal |
| build system | autobuild + CMake | ✅ literal |
| compiler | gcc / clang (autobuild 設定依存) | ⏳ literal 取得要 |
| Vulkan SDK version | (= 取得要確認、autobuild 経由 install) | ⏳ literal 取得要 |
| Vulkan loader | libvulkan.so.1 | ✅ literal |
| GPU driver (検証用) | Mesa (NVidia/AMD/Intel) or proprietary NVidia driver | ⏳ AYA 環境 literal 取得要 |
| GLSL → SPIR-V compiler | glslang (= autobuild 経由 install 想定) | ⏳ literal 取得要 |
| VMA (Vulkan Memory Allocator) version | (= autobuild package 経由) | ⏳ literal 取得要 |
| volk version | (= autobuild package 経由) | ⏳ literal 取得要 |
| build cmd (literal) | `cd build-linux-x86_64 && make -j4 <target>` | ✅ literal |
| integration test cmd (literal) | `INTEGRATION_TEST_lluboringbuffer` / `llassetubopool` / `llpipelinecachestorage` 実行 | ✅ literal |
| codegen unittest cmd (literal) | `scripts/ubo_codegen` 配下 unittest 実行 | ✅ literal |

---

## §2. r41 設計の Vulkan 要求 version (= literal 確認要、Phase 1.D 以降で確定)

現状 design doc 群からの想定:
- **Vulkan 1.2 以上** (= timeline semaphore + descriptor indexing + maintenance3 等の機能使用前提想定)
- 具体的 extension list = Phase 1.D 以降で literal 取得要 (= `instance extension` + `device extension` 一覧)
- shader Vulkan 要求機能 = Phase 1.D 以降で literal 取得要 (= `VK_KHR_*` extension list)

---

## §3. OS 依存性表 6 項 (= 現時点で literal 確認可能な範囲)

| # | 構造 | OS 依存性 | Linux 先行可否 | macOS 派生 fix 候補 | Windows 派生 fix 候補 |
|---|------|----------|---------------|---------------------|----------------------|
| 1 | Vulkan API 本体 | cross-platform | ✅ 完全先行可 | macOS は MoltenVK 経由 (= Vulkan → Metal 翻訳)、一部機能未対応 | ✅ full Vulkan = 派生 fix なし想定 |
| 2 | GLSL → SPIR-V codegen | cross-platform | ✅ 完全先行可 | shader binary 自体は共通 | shader binary 自体は共通 |
| 3 | VMA / volk dependency | autobuild package 配線次第 (= Phase 1.A 想定済) | ✅ Linux 配線で骨格確定 | macOS 用 autobuild package 別途配置要 | Windows 用 autobuild package 別途配置要 |
| 4 | descriptor set 数 | **macOS MoltenVK 制約あり** (= `maxBoundDescriptorSets=4` 可能性) | ⚠️ r41 設計 5 set (= 0/1a/1b/2/3) なら macOS 派生 fix 要 | Linux 完成時の使用 set 数 literal 記録 → MoltenVK 限界と照合 → set 統合 fix 候補 | 影響なし (= full Vulkan で 8 set 標準サポート想定) |
| 5 | push descriptor | macOS MoltenVK 未対応の可能性 | ✅ **r41 は H10-A で disable 済 = macOS 整合** (= PC-7δ / PC-N-3 で確認済) | 設計時点で disable 経路採用ゆえ macOS 移植時の影響なし | 影響なし |
| 6 | Vulkan version 要件 | macOS MoltenVK は遅れ気味 (= 1.2 subset) | ⚠️ Linux 完成時の要求 version 明示要 | §2 で確定後 MoltenVK 対応 version と照合 | 影響なし |

---

## §4. macOS MoltenVK 既知制約 list (stub、= Phase 1.D 以降で literal 確認要、現状は推測 list)

| 項目 | 推測状況 | 確認方法 | r41 影響候補 |
|------|---------|---------|-------------|
| `maxBoundDescriptorSets` | 4 (= Vulkan 標準 spec 最小値、MoltenVK 実装次第) | MoltenVK SDK doc + `vkGetPhysicalDeviceProperties` literal | ⚠️ r41 設計 5 set ≥ 4 ゆえ macOS で set 統合 fix 候補 |
| push descriptor (`VK_KHR_push_descriptor`) | 未対応の可能性 | MoltenVK SDK doc + extension list literal | ✅ r41 は disable 経路 H10-A 採用済ゆえ影響なし |
| timeline semaphore (`VK_KHR_timeline_semaphore`) | 対応済 (= Vulkan 1.2 標準) | MoltenVK SDK doc | (= r41 使用有無は Phase 1.D 以降確認) |
| variable rate shading | 未対応 | MoltenVK SDK doc | (= r41 不使用想定) |
| ray tracing | 未対応 | MoltenVK SDK doc | (= r41 不使用) |
| 16-bit storage | 部分対応 | MoltenVK SDK doc | (= r41 使用有無は Phase 1.D 以降確認) |
| 8-bit storage | 部分対応 | MoltenVK SDK doc | (= r41 使用有無は Phase 1.D 以降確認) |
| `VK_KHR_dynamic_rendering` | 対応済 (= 比較的新しい) | MoltenVK SDK doc | (= r41 使用有無は Phase 1.D 以降確認) |
| MSL (Metal Shading Language) 翻訳精度 | MoltenVK 経由 SPIR-V → MSL 自動翻訳、複雑 shader で degenerate 可能性 | 実機検証 | shader 群 literal 動作確認要 |

---

## §5. Windows 既知考慮事項 (stub、= 派生 fix 候補少想定)

| 項目 | 状況 | r41 影響候補 |
|------|------|-------------|
| compiler | MSVC (= MSBuild 経由) | autobuild 経由で吸収想定 |
| Vulkan SDK | LunarG SDK + Vulkan loader (`vulkan-1.dll`) | autobuild 経由 install |
| driver | NVidia / AMD / Intel proprietary | full Vulkan サポート |
| descriptor set 数 | full Vulkan 標準 (= 8 set 想定) | r41 設計 5 set ≤ 8 ゆえ影響なし |
| push descriptor | full サポート | r41 は disable 経路採用ゆえ影響なし |
| Vulkan version | Linux と同じく最新追随 | 影響なし想定 |
| path separator | `\` (= Linux `/` から fix 要箇所が出る可能性) | source 内 path literal は autobuild + CMake で抽象化想定 |
| ABI | x86_64 ABI (= Linux と互換) | r41 source code 影響なし想定 |

---

## §6. 各 PC-* sub-step での OS 依存懸念記録欄 (= Phase 1.D 以降で着手時追記)

| PC-* sub-step | OS 依存懸念 | macOS 派生 fix 候補 | Windows 派生 fix 候補 | 状態 |
|---------------|------------|---------------------|----------------------|------|
| PC-0..PC-7ε | (= 着手済、本 stub では遡及記録なし、Phase 1 全完了時に retrospective 記録予定) | - | - | ✅ |
| PC-N-1 | (= 着手済) | - | - | ✅ |
| PC-N-2 | (= 着手済、bindV3aRigged set=2 復活 = descriptor set 数 5 → macOS 派生 fix 候補 §3 #4) | descriptor set 統合 fix 候補 | - | ✅ |
| PC-N-3 | (= 着手済、placeholder skin sentinel 経路 = layering 制約遵守 address-only pattern、cross-platform 影響なし) | - | - | ✅ |
| PC-N-4 | (= 着手済、ring buffer grow 自動 re-wire = vkUpdateDescriptorSets endFrame() 末尾 hook、cross-platform 影響なし) | - | - | ✅ |
| **PC-N-5** | (= **Phase 1.D 着手起点 = 実 GLTF Vulkan draw 通電 1 stub**、(N5-1) A 採用 = Skin_GLTFJoints UBO bind 経由 rigged draw、第 2 sentinel-like skin `sGltfStubSkin` 並走通電 + `recordGltfAssetDraw` 新設 + `AYAGltfStubDrawEnabled` cvar 切替) | descriptor set 数は PC-N-3 と同 5 set 維持 (= macOS MoltenVK 派生 fix は §3 #4 / PC-N-2 と一括対応) + sentinel pattern は layering 制約遵守ゆえ MoltenVK 側追加考慮なし + `AYAGltfStubDrawEnabled` cvar は cross-platform | full Vulkan ゆえ派生 fix 候補なし想定 | ✅ design-lock + 実装 complete (commit 675529a891) |
| **Phase 1.D decomposition design-lock** | (= **PC-N-6..PC-N-10 5 sub-step 分解**: PC-N-6 = 実 LL::GLTF::Asset 経由 vertex buffer upload + PC-N-7 = index buffer upload + vkCmdDrawIndexed + PC-N-8 = material/transform UBO 実 bind 配線 + PC-N-9 = GLTFSceneManager::render 統合 + `AYAGltfRealDrawEnabled` cvar gate + PC-N-10 = cleanup + `AYAGltfStubDrawEnabled` deprecate) | 各 sub-step は PC-N-5 同形の sentinel/UBO 経路踏襲ゆえ macOS MoltenVK 影響増なし (= descriptor set 数 5 維持) | full Vulkan ゆえ派生 fix 候補なし想定 | ⏳ design-lock complete 本 commit / 各 sub-step 実装 ⏳ |
| PC-N-6 | (= 実 LL::GLTF::Asset 経由 vertex buffer upload、Phase 1.D 内 1st sub-step) | (= 後述 design-lock phase で詳細確定) | (= 後述) | ⏳ |
| PC-N-7 | (= 実 LL::GLTF::Asset 経由 index buffer upload + vkCmdDrawIndexed) | (= 後述) | (= 後述) | ⏳ |
| PC-N-8 | (= material/transform UBO 実 bind 配線) | (= 後述) | (= 後述) | ⏳ |
| PC-N-9 | (= GLTFSceneManager::render 統合 + `AYAGltfRealDrawEnabled` cvar gate) | (= 後述) | (= 後述) | ⏳ |
| PC-N-10 | (= cleanup + `AYAGltfStubDrawEnabled` deprecate) | (= 後述) | (= 後述) | ⏳ |
| ... | ... | ... | ... | ⏳ |

---

## §7. Mac/Win 開発者向け提供 checklist (= Phase 1 全完了時に確定形で提供)

- [ ] §1 Linux 環境 spec 全項目 literal 確定
- [ ] §2 r41 設計 Vulkan 要求 version + extension list literal 確定
- [ ] §3 OS 依存性表 6 項 (= 本 stub 既配置)
- [ ] §4 macOS MoltenVK 既知制約 list literal 確認
- [ ] §5 Windows 既知考慮事項 literal 確認
- [ ] §6 各 PC-* sub-step での OS 依存懸念記録 retrospective + 全 sub-step 完成
- [ ] AYAstorm r41 Phase 1 全完了 marker (= 別 doc) reference
- [ ] AYA 提供承認

---

## §A. 更新履歴

- **2026-06-05**: 本 stub 起案 (= PC-8 marker doc §4 で起案 record、AYA literal「(PC-8-doc-1) B + (PC-8-doc-2) A で起案お願いします」record)
- **2026-06-05**: §6 PC-N-5 行追記 (= Phase 1.D 着手起点 design-lock complete、AYA literal「OK」record + (N5-1) A 採用 = Skin_GLTFJoints UBO bind 経由 rigged draw、第 2 sentinel-like skin `sGltfStubSkin` 並走通電 + macOS MoltenVK 派生 fix 候補は §3 #4 / PC-N-2 と一括対応 + Windows 派生 fix 候補なし想定)
- **2026-06-05**: §6 PC-N-5 状態 ✅ 反映 (= commit 675529a891 で実装完了) + Phase 1.D decomposition design-lock 行追記 (= PC-N-6..PC-N-10 5 sub-step 分解、AYA literal「OK」record + 12 件 ambiguity (D-1)..(D-12) 全件推奨案採用 + PC-N-6..PC-N-10 5 行 stub 追記、各 sub-step は PC-N-5 同形の sentinel/UBO 経路踏襲ゆえ macOS MoltenVK 影響増なし + Windows 派生 fix 候補なし想定)

---
