# per-draw 描画記録 回復 — 3戦略 作業計画(AYA 承認 2026-07-23)

> 位置づけ: crowd 本体回復の次フェーズ。**avatar relocate(off-main)line は Phase 2 で決着**(crowd off-main = motion 激安ゆえ低配当・[[handoff_avatar_relocate_design]])し、真のボトルネックが **main の per-draw 描画記録** と実測で確定したことを受けた新 line。真実源の診断 = memory `finding_crowd_bottleneck_vsync_busywait_not_cpu`。統治 = `docs/vknative_recovery_plan.md`。

## 0. 診断(本 doc の前提・2026-07-23 実測で確定)

- **main の実負荷 = per-draw 描画記録**(~15k draw/frame)。TID 隔離 perf で確定: top 関数 = `LLTexUnit::bindFast` / `LLVKLoader::bindGraphicsPipelineOnce` / `populateAndBindUniversalDescriptorSet` / `ScenePerDrawCacheKey` 探索 / `LLVertexBuffer::setBuffer` / `LLVKLoader::ensureObjectSkinUploaded` / `buildAndOverrideScenePerDrawSet`。**上位20関数で25.8%・全879関数 = death by a thousand cuts**(単一 hot spot なし = per-draw 操作の分散)。
- **rigged avatar draw は static MDI から除外**(`llvkbucket.cpp:378` `info->mAvatar.isNull()` が is_static 条件)→ dynamic per-draw 経路で毎frame 記録。= 15k の主要部。
- GPU = 28-36%(餓え・main が直列記録で GPU を待たせてる)。decode/音声 = 既に off-main。pose compute / vkQueueSubmit = 激安(標的でない)。
- **side 問題**: present スレッド(aya-present)が FIFO vsync(`llvkloader.cpp:3764` hardcode)を **busy-wait スピン**で待って1コア焼き(perf: 50% vdso clock_gettime + 40% nvidia driver + 自前コード 0.07%)。`__GL_YIELD=USLEEP` で 93%→2%・fps 不変・純勝ち(実証済)。
- 並列化(T系/PE)は **既に +10fps 検証済**(直列38 vs 並列48)= 正しい投資。crowd off-main(単一 worker)は「crowd をコアに分散」思想を **cheap な motion に当てた矮小化** = 低配当と判明(AYA 指摘)。正しい標的 = この per-draw 記録。

## 1. 3戦略マップ

| # | 戦略 | 標的 | 品質 | コスト | 状態 |
|---|---|---|---|---|---|
| **1** | vsync 待ちを spin→sleep | present の FIFO busy-wait(1コア焼き) | 不変(vsync/pacing そのまま) | 極小 | ✅ **完了 commit `e8cfdc466f`**(present_wait + __GL_YIELD・aya-present 93%→2-3%・fps 不変) |
| **2** | **rigged draw の indirect 化 + 記録の per-core 分散** | main ~60% = 15k draw の per-draw 記録 | 不変(発行畳み込み・byte gate 必須) | 最大(E系本丸) | 未着手 |
| **3** | 静的な再 sync skip | 不変 palette/descriptor の毎frame 再送 | 不変(不変データ送らない) | 中 | 未着手 |

**効き方**: 戦略1 = fps 非改善・CPU/電力を返す(独立・side-win)。**戦略2・3 = main の per-draw 記録を削る → 軽い crowd で fps 微増・本命は重い crowd(50-100人)で main 飽和=freeze を防ぐ(北極星直撃)。**

## 2. 作業計画(順序と各戦略のスコープ/gate)

### 段階0(常時・全戦略の前提)= 計測を常時 ON
- **普段の起動を `AYASTORM_PERF_LOG=5` 常時 ON に**(コストほぼゼロ)。週1の重い 50-100人会場に入った瞬間、VkPerf が `cpu main=%` / `draws/f` / `ph`/`idl` 内訳を自動採取 → 後追い mine(一発勝負を回避)。戦略2/3 の北極星配当(重い会場で main 飽和を防ぐ)の定量化はこれで得る。狙って混雑イベントに行っても可。

### 戦略1 ✅ 完了(commit `e8cfdc466f`・2026-07-23)= vsync 待ちを sleep 化
- **目的** = present の busy-wait スピンを sleep にしてコア1本返却。**fps は上げない**(GPU 餓え=データ供給律速で上がらない)= 狙いは CPU/電力/熱。
- **実装(2層・3 OS 前提)**: (b) portable = `VK_KHR_present_wait`+`present_id` 有効化 → peExecute で presentId + `vkWaitForPresentKHR` block(未対応 device は no-op fallback・3 OS 促しコメント付き)/ (a) NVIDIA-Linux 完成手 = `llappviewerlinux.cpp` で `__GL_YIELD=USLEEP` setenv(overwrite=0)。
- **gate 実測(Linux+NVIDIA)**: aya-present 93%→2-3%・全体 CPU ~129%→~95-105%・fps 不変(A/B: present_wait ON 27 vs OFF 26.5 @同一 24k draws)・tear なし。
- **⚠️ 実測で判明した真実**: **NVIDIA では __GL_YIELD が効き手・present_wait は NVIDIA 内部でも spin して単独では 30% までしか落ちない**(93→30)。__GL_YIELD 単独で 3% まで落ちる = present_wait は NVIDIA-Linux では冗長。present_wait は害なく portable 機構として残置(他 vendor/OS 用・各 OS maintainer が自 driver で完成)。
- **申告**: fps は上げない(per-draw 律速は戦略2/3)。present mode は FIFO のまま(vsync 切らない=暴走させない・画質不変)。Windows/macOS の present sleep は未検証=各 maintainer 対応(コメント明記)。

### 戦略3(中コスト・戦略2 の前哨)= 静的な再 sync skip
- **目的** = pose/state 不変でも毎frame 再 build+upload してる無駄を削る(`ensureObjectSkinUploaded`/`uploadMatrixPalette` の skin palette 無条件再 build ~2.9% + descriptor set 再構築 ~3.6%)。
- **手**: ①skin palette = pose 不変 avatar で再 build/upload を skip(半静的の差分化・dirty flag)②descriptor set = 内容不変なら cache 再利用(既存 ScenePerDrawCache の hit 率改善)。
- **gate**: 視覚同一(不変データを送らないだけ)+ VkPerf の該当項(skin_up/set build)減 + fps 中立以上。
- **⚠️ 戦略2B との結合**: 2B(rigged indirect)は skin を indirect buffer 化して skin handling を再構築するので、戦略3 の skin skip と設計が干渉する。**選択**: (i) 戦略3 を現アーキで独立に先行(安く早い配当)→ 2B が後で再構築、(ii) 戦略3 の skin 分を 2B 設計に畳む。**推奨 = descriptor set 再利用(2B 非干渉)を先行 / skin skip は 2B と同時設計**。

### 戦略2(本丸・最終・独自設計フェーズ)= rigged indirect + per-core 記録
2つの形(同じ per-draw 記録への別アプローチ・連続工事):

- **(B) draw COUNT↓ = rigged draw の indirect 化**:
  - `mAvatar.isNull()` 除外(llvkbucket.cpp:378)を畳み、rigged を MDI/indirect 経路へ。
  - 前提 = per-avatar skin matrix palette を **indirect+bindless でアクセス**(全 avatar palette を1本の buffer + per-draw index)。terrain MDI collapse より一段重い(rigged は per-avatar skin データ持ち)。
  - E系本丸「drw」= per-draw 発行単価そのもの。品質 = 発行畳み込みで **byte 同一**(L3 型 A/B オラクル + 視覚 gate 必須・terrain 実績)。
- **(C) 記録の per-core 分散 = secondary command buffer**:
  - 15k draw の記録を N コアに割る(各コアが draw 部分集合を自 secondary cmd buffer に記録 → main が execute)。
  - **これが「crowd をコアに分散」思想の正統**(矮小化の答え・§3 参照)。
- **順序(戦略2 内)**: **B を先**(indirect 化で draw 単価↓・E系 proven pattern・incremental)→ 計測 → 記録がまだ律速なら **C**(multi-thread 記録 = 最難関の癌本体)。
- **着手前**: 着手プロトコルに従い JIT 詳細設計(触る file・罠・gate・申告欄)→ AYA 承認 → 実装。規模が大きいので分割着手。

## 3. doctrine 整合(重要 = per-core 記録は死案でない)

[[project_vk_doctrine_eliminate_not_parallelize]] の「並列化=死案」は **per-avatar 粒度で avatar の塊を割る**場合(join ~31µs/av が支配・前任 2 名の実帰結)に限る。**戦略2C(secondary command buffer で描画記録を分散)は別物**: join = secondary buffer を execute するだけ(安い)で per-avatar join の死は起きない。Vulkan の標準 multi-thread 記録パターン。∴ **記録の per-core 分散は doctrine に反しない**(off-main + join 無し regime = parallelize 解禁の正統適用)。

## 4. gate 基準(共通・全戦略)

- **視覚同一**(最終 gate のみ・視覚を途中オラクルにしない)。
- **validation 0**・診断起動で全層オラクル沈黙(`docs/vknative_detection_apparatus.md`)。
- **経路移行(B/C = worker/indirect 化)は L3 型 A/B オラクル(worker 出力 vs 旧経路の byte 照合・verdict=src|kernel)を伴わない限り受け入れない。**
- **品質トレード禁止**(発行畳み込み・sync skip は全て lossless = 品質不変。draw を "減らす" のは発行畳み込みであって描く物の間引き〔LOD/cull〕ではない = 後者は product 決裁)。
- gate 様式 = 命題 + 未証明項併記 + 実効設定確認欄。

## 5. 縮小・省略・解釈申告(OPEN)

1. **重い 50-100人会場での main 飽和は未実測**(このシーンは軽い可能性)。戦略2/3 の北極星配当は段階0(常時 PERF_LOG)で後追い定量化。方向(main が per-draw 記録 bound・draw 数に線形)は現データで確定ゆえ着手は正当。
2. **戦略2B の skin indirect+bindless の実装規模は未見積**(terrain より重い・JIT 設計で確定)。
3. **戦略2C の secondary command buffer 化の per-thread descriptor/状態管理コスト**は未設計(funnel の N-way 安全性含む)。
4. relocate 設計 doc の前提「CPU-main-bound」は**正しい**(main は bound)が、標的が pose でなく per-draw 記録だった = 本 doc が上書き。
