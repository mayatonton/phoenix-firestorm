# image layout discipline 分類 + RECORD-CONTRACT(F4)

> 制定 2026-08-02(設計者 M1 の HEAD 転記 = 実装者が file:line 再検証・stale 是正込み)。位置づけ = layout/sync/record foundation の分類層(M1 layout-state / M3 record-threading)。設計正本 = memory `design_layout_sync_record_target_model`(M1-M4)。関連 = F1(subresource-aware transition・commit `be141c970699`)/ F2 契約 = `docs/vknative_pass_resume_contract.md` / F5(dead scaffolding 撤去 = 本 §RECORD-CONTRACT と一体)。全 file:line は 2026-08-02 HEAD(`d181c6357b8` + F1/F5)接地。
> **コード変更なしの分類 doc**。各 image family の layout 追跡規律を「全統一強制」でなく明文化し、INV(不変条件)と census 手法を確定する。

## 0. 単一の根(M1)
**transition primitive と pass/attachment モデルが single-subresource(layer0/mip0/single-view)前提なのに、resource は multi-subresource 化(layered/array/mip/cube)している。** sample/bind 側は既に layered 対応(`llrender.cpp:441` / `llglslshader.cpp:2269` 付近)であり、gap は「書込 transition + resume + guard」の 3 点に集中する。本書は書込 transition 側(layout 追跡規律)を分類する。

## 1. layout discipline 3 分類
各 image family に、以下いずれかの discipline を**明示的に**割り当てる(全統一は強制しない・family ごとに最適を選ぶ)。

| discipline | 意味 | 追跡の所在 |
|---|---|---|
| **tracked-var** | layout を変数で保持し、transition 前後で変数≡実 layout を維持 | RT メンバ変数 |
| **context-driven** | layout を変数で持たず、フレーム内の呼び出し文脈(既知の pass 順)から確定 | 呼び手のシーケンス |
| **invariant-convention** | 生成時に固定 layout を確立し、一時逸脱→必ず復帰の規約で不変を保つ | 生成規約 + 復帰規約 |

## 2. image family × discipline 割当(HEAD 現状 + target)
### ① RT world(scene/gbuffer/shadow RT)= **tracked-var**
- 追跡: `LLRenderTarget::mVkTexLayout[]`(color 添付ごと・`llrendertarget.h:272`)/ `mVkDepthLayout`(`:273`)/ `mVkDepthLayoutOwner`(共有 depth の owner 委譲・`:279-284`)/ **`mVkDepthLayerCount`(F1 追加・`:268`)**。
- transition: `transitionImageLayoutVk(..., U32 layer_count)`(`llvkloader.cpp:12076`・**F1 で layer_count 引数化 = layered whole-uniform**)。既定は layer0/mip0 単一、layered depth は `mVkDepthLayerCount` を渡し全 layer 一様遷移。
- **INV**: discipline 下で `mVkTexLayout/mVkDepthLayout` ≡ 実 layout / 全 subresource 被覆(layer_count 経由)/ 外部 untracked transition 禁止(validation で検出)。

### ② swapchain(present color / depth)= **context-driven(color)+ 部分 tracked-var(depth)**
- color: 追跡変数なし = **context-driven**(acquire→render→present の既知順で確定・`transitionImageLayoutVk(sSwapchainImages[...], ...)` を present 経路 `llvkloader.cpp:5500 / 5951 / 7152 / 7246` の 4 site で明示発行)。
- depth: `sSwapchainDepthLayout`(`llvkloader.cpp:688`・reset 点 `:4443 / :4465`)= **部分 tracked-var**。present 非対称(color は文脈駆動・depth のみ変数)。
- **INV**: swapchain image は present 前に必ず PRESENT_SRC へ・acquire 後に既知 layout から遷移。文脈駆動ゆえ「pass 順が契約」= §RECORD-CONTRACT に従属。

### ③ cube / probe / readback / mip = **invariant-convention**
- 追跡変数なし。生成時に SHADER_READ 系 layout を確立し、書込/blit で一時逸脱→**必ず復帰**の規約で不変を保つ。
- 全 subresource 被覆 transition の実経路は 2 本のみ: `createCubeArrayImageVk`(`llvkloader.cpp:10848`・full range)/ `blitCubeArrayVk`(`:10097`・layer_count 引数 `:10103`)。他は `transitionImageLayoutVk` の layer0/mip0 既定。
- **INV**: 生成時 layout = 規約の基準・逸脱区間は同一呼び手内で閉じる(away→復帰)・呼び手 constant 依存を崩さない。

## 3. census 手法(fail-closed の数え方)
**layout 追跡の完全性は「raw `vkCmdPipelineBarrier` 全数」で数える**(高位 wrapper でなく生 barrier を数える)。F1 監査で world-① 迂回の dead barrier 2 本が高位 census から漏れていた事故が根拠。
- **HEAD census(2026-08-02)= 全 42 site・全て `indra/llrender/llvkloader.cpp` に局在**(他 translation unit にゼロ)。∴ layout 遷移の主権は llvkloader.cpp 単一 = 追跡規律の検証面が 1 file に閉じる。
- 新規 barrier を他 file に足す = 本分類の外・設計違反(validation + 本 census で検出)。

## 4. RECORD-CONTRACT(M3 明示化 + F5 の前提)
main 単独 recorder + PE 単一 submitter FIFO + async producer submit-ahead。**record 順 ≠ submit 順**(`endFrame`:cjob consumer が最後 record → 最初 submit)。coherence は「disjoint image + scene RT double-buffer(`mScenePresentRT[2]`)+ CPU fence gate(display 経路)」の**未文書だった暗黙契約**に懸かっていた。以下に明文化する。

1. **(a) tracking 帰属**: layout tracking は recording domain 属・**1 domain 1 writer**。
2. **(b) PE 専任**: PE thread は submit/present 専任(record しない)。
3. **(c) 順序契約**: record 順≠submit 順を許容し、disjoint image / scene RT double-buffer / CPU fence gate で coherent に保つ。
4. **(d) domain 拡張時**: per-draw 記録改修等で record を複数 domain 化しても、各 domain が own tracking を持つか明示 handoff する。
5. **(e) scaffolding ZERO**: 本掃除(F5)で並列 record 前提の休眠 scaffolding を撤去済み = `tRecordLaneIndex`(常0)/ `sPendingPreFrameCmds`(常空)/ `MAX_RECORD_LANES`(8→1 で lane 配列自然畳み)/ **III-0 window-mutation probe(`sRecordWindowActive` / `isRecordWindowActive` / `recordWindowMutationGuard`)**。`recordWorkerCount` 系(bake worker gate `llviewertexlayer.cpp:1109`)は load-bearing = 存続。

### 4.1 未武装 guard を dead code で残さない(invariant-first・AYA 裁定 2026-08-02)
III-0 probe(「record 窓中の geometry 変異検出」)は撤去した。その撤去根拠 = **hazard 母集団が構造的に空**(正のトレース・憲法 6 充足): scene record スレッドは HEAD に不在(PE=submit/present 専任 `llvkloader.cpp:1287` 付近 / bake worker=texture job・scene geometry 非接触 `llviewertexlayer.cpp:1129` / 残りは init selftest のみ)。`sRecordWindowActive` に writer が皆無なのは「配線し忘れ」でなく「開くべき並走 record 窓が存在しない」ため。probe の 2 呼び点(`rebuildMesh` / `applyGeoStaged`)は main の変異適用点で、record も変異も main 単独 = 同一 thread 逐次 = interleave 窓は概念ごと消滅。

- **契約(意図の温存)**: 「record 窓中の geometry 変異検出」という**意図は dead code でなく本契約で残す**。∴ **将来 record を複数 domain 化する工事(per-draw 記録の並行化・multi-domain record 等)は、armed な窓 guard(III-1 後継)の設計を当該工事自身が持参する**こと。未武装 probe を温存するのは「保護ゼロなのに保護があるように見える」偽装であり、憲法 4(検出装置の信頼性)が守るものの逆になる。
