# 段階 III 実装計画 — 描画 worker 記録 再設計

> 設計 = `docs/vknative_draw_redesign.md`（段階 II・公理 F）。地図 = `docs/vknative_draw_structure_map.md`（段階 I）。
> **grandfather しない**（Phase A/B の現実装も置換）。各段は **①L3 型 A/B（新 vs 旧 byte 照合・同一走行内 switch）②validation 0 ③診断起動で §5 検出装置沈黙** の 3 点 gate。**PASS は AYA のみ**（憲法 1）。
> **⚠️ 憲法 4**: §5 検出器・`llvkcontract.*` 改変は AYA 承認必須。新 alarm は allowlist 無断追記禁止。
> 実装は**直列**（1 段実装→検証→次）。工事中の異常はまず当該段 kill switch で新旧帰属を確定してからコードを追う。

---

## 依存グラフ（実装順）
```
III-0 検出先行 ─┬─> III-1 背骨 ─> III-2 seed ─> III-3 uniform ─> III-4 既存移行 ─┬─> III-5 静的拡張 ─> III-6 alpha
（安全網）      │   （DrawPlan+freeze）（on-demand）  （S14/S15）   （Phase A/B）    │  （simple/FB）   （crowd 本丸）
               └───────────────── 検出装置は全段の gate に常駐 ─────────────────┘
```

---

## III-0 検出装置 先行投入（安全網・移行前診断）
- **目的**: 移行前に「現状 freeze 契約が破れている箇所」を可視化（S8-b の実発生地図）。移行の安全網を先に張る。
- **触る**: `sRecordWindowActive` フラグ新設（llvkloader）。mutator に guard 挿入 = `LLSpatialGroup::rebuildMesh`（`pipeline.cpp:8218`）/ `applyGeoStaged`（`llvovolume.cpp:6194`）/ stateSort 変異部 / VB map・unmap。窓 active 中呼びで **非 throttle alarm**。gen snapshot 足場（DrawPlanItem 前身）。
- **deliverable**: 現行 Phase A/B のまま診断起動 → guard が発火する箇所 = 現状の S8-b 発生点の実地図。
- **gate**: guard 統合・診断起動で発火箇所を採取（これは*診断*＝III-1 の設計入力。この段は「現状の violation を消す」ではなく「見える化」）。
- **憲法 4**: 検出器新設 = AYA 承認。**kill switch**: guard は flag 裏（既定 off・診断時 on）。**risk 低**（観測のみ）。

## III-1 背骨（DrawPlan + freeze schedule）★最大工事
- **目的**: 公理 F の土台。worker が live を読まず DrawPlan（不変 snapshot）を読む + 窓中 mutation ゼロ。
- **触る**:
  - **DrawPlanItem/DrawPlan 構造体 新設**（地図 §7.1 の read フィールド網羅・redesign §1.2）+ per-frame arena（A6 `allocPerDrawUBOSlice` 型の DrawPlan 版）。
  - **rebuildMesh 脱 lazy**（§1.3.1-F2）: `pipeline.cpp:4266`/`:4355` の inline rebuild を撤去 → 全可視 dirty group を `mMeshDirtyGroup`（既存 `:5160`）へ集約 → **全カメラ cull 後・plan-build 前に 1 回 batch rebuild**。
  - **world stateSort 前倒し**（§1.3.1-F3）: ph9 world stateSort を mutation phase（shadow record ph6 の前）へ hoist。
  - **plan-build 挿入**（§1.3.1-F1）: 各 stateSort 直後（その cull result が現在の間）に DrawPlan を materialize。`LLVKBucket::forEachSource`（`llvkbucket.h:114`）を plan-build で 1 回舐めて copy。
  - **worker 経路の read 源を DrawPlan に切替**（旧 forEachSource live 読みを worker から外す）。
- **deliverable**: freeze 契約 active。worker は DrawPlan のみ。
- **gate**: **III-0 window guard が SILENT**（freeze 成立の正のオラクル）+ 視覚同一 + validation 0。
- **kill switch**: `AYASTORM_DRAWPLAN`（or MT switch 拡張）。**risk 高**（frame 再構成・ただし §1.3.1 で局所と確定：rebuild 一括化＋stateSort 前倒し＋plan-build 挿入）。

## III-2 seed 統一（S6 根治）
- **目的**: 静的 seed 全廃・on-demand 化。unseeded 消失（body 消失の真犯人）根絶。
- **触る**:
  - **撤去**: `sRecordSeedMap` find 短絡（`llglslshader.cpp:3616-3648`）/ `ensureShadowWorkerSeeds`（`pipeline.cpp:14404`）/ `ensureCameraWorkerSeeds` / `vkCaptureSeedDynamicBuffers` / `RecordSeed`・`record_seed_map_t` / ctx `seeds` 項目。
  - **統一**: record job 中も main 経路の per-lane on-demand build（`mVkPerDrawLane[lane]` `:3703`＋）を使う。
  - **descriptor bypass**（§2.7.2）: record job 中は `vkCaptureEnumBoundView`（`:2215`・共有 `mVkEnumBoundView` write）を**呼ばず** view は直接解決（`live_view`）。`vkWarnL3Fallback` noise を record job で抑制。gate = `isRecordJobActive()`。
  - **audit**: `shader->bindTexture` per-draw 呼び全 pool（bump/gltf 等）列挙し bypass 網羅確認。
- **deliverable**: どの program bind でも lane で on-demand 生成。動的 pool 自動網羅。
- **gate**: `seed_build_fail=0`（§5.4 非 throttle 会計）+ shadow/materials A/B byte 一致。**risk 中**。

## III-3 uniform（S14/S15）
- **目的**: cross-cutting 未隔離ゼロ。
- **触る**: `gGLDeltaModelView`/`gGLInverseDeltaModelView`（`llrender.h:483-484`）→ **thread_local 化**（sibling `gGLModelView` と対称・**フルビルド**）+ record ctx/DrawPlan header に転写。GlobalF UBO（`writeCurrentGlobalFUBO`）→ worker window で A6 arena slice に snapshot・descriptor がそれを参照。
- **deliverable**: 単一共有 mutable が record 窓から排除。
- **gate**: A/B byte 一致。**risk 低〜中**（.h 変更＝フルビルド）。

## III-4 既存 worker 経路 移行（Phase A/B を新機構へ）
- **目的**: 新機構で既存を再実装＝非退行を先に確定してから拡張。
- **触る**: Phase A shadow + Phase B materials を DrawPlan + on-demand seed へ載せ替え。**isMapped skip 撤去**（`lldrawpool.cpp:1835`・freeze 契約が窓中不変を保証ゆえ不要）。「1 program 1 job」前提コード撤去。shadow cascade の ctx/pin 機構を DrawPlan に統合。
- **deliverable**: 既存 worker 経路が新モデル上・稀症状（体消失/関節崩れ/緑汚染）消失。
- **gate**: shadow/materials L3 A/B + 稀症状シーンで §5 検出沈黙。**risk 中**。

## III-5 静的 pool 拡張
- **目的**: 1 pass 1 shader pool を worker 化（低リスク横展開）。
- **触る**: simple/fullbright を worker 化（地図 §8: 1 pass 1 shader）。**GLTF PBR opaque pool（地図 §8.5 未精査）は 1 pass 1 shader を確認してから編入**。
- **deliverable**: gbuffer 並列度拡大。
- **gate**: 各 pool A/B + shad/gbuffer wall 短縮（段階 IV 計測）。**risk 低**。

## III-6 alpha（crowd 本丸）
- **目的**: crowd blocker（半透明）の CPU 記録並列化。北極星 gate。
- **触る**:
  - **secondary CB 基盤 新設**（§4.8・現痕跡ゼロ）: record lane に **SECONDARY level CB + `VkCommandBufferInheritanceRenderingInfo`**（`rwAcquireLaneCmd:1374` は PRIMARY 固定＝拡張）/ `beginDynamicRendering` に `VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT` / main CB で `vkCmdExecuteCommands`（span 順配列）。
  - **DrawPlan order index + sort slice 分割**（§4.3・span = sort 連続区切り）。
  - **per-draw shader 切替** = §2 on-demand seed が bind 毎に lane 生成。
  - **MoltenVK 検証**（macOS・§4.8 named wall）: secondary-CB-in-dynamic-rendering 可否を実機/ドキュメントで確認。**不可なら primary-span fallback**（ordered primary CB + inter-span color barrier・上物だけ差替・設計本体不変）。
- **deliverable**: alpha CPU 記録並列。
- **gate**: alpha A/B（blend 結果 pixel 一致）+ crowd 会場で viewer 固まらない（北極星）。**risk 高**（新基盤 + MoltenVK 壁・fallback で保証）。

---

## 🧹 コーディング規律（AYA 制定 2026-07-28・全段厳守）
- **要らなくなった処理は必ず削除**: 新機構へ移行して不要化した旧経路・旧 field・旧 flag・dead branch は**その段で削除**（残置しない）。特に本再設計は撤去物が多い（seed map/ensureXxxSeeds/isMapped skip/「1 program 1 job」前提/lazy rebuild/RecordSeed 等）= **置換 = 旧の削除とセット**。コメントアウト放置も禁止。
- **まとめられる処理は関数化**: 重複ロジック（例: shadow/camera の ctx 転写・plan-build・on-demand build・A/B 照合）は共通関数へ括り可読性を保つ。copy-paste の分岐増殖を避ける（地図の教訓「per-symptom flag は設計失敗のシグナル」と同根）。
- 既存 CLAUDE.md ルールと整合: dead code 判定は -Wunused sweep + 全出現目視（memory claim 鵜呑み禁）/ cvar 撤去基準 = GUI 到達性 / 新規コメント追加禁止（説明が要る複雑さは関数名・構造で表す）。

## 全段横断
- **L3 型 A/B ハーネス**（§5.7）: 同一走行内 kill switch 切替（起動またぎ A/B 禁止＝地図/memory 教訓）。照合 = geometry/CB は draw 列・byte / **視覚は最終 gate のみ**（「視覚を判定オラクルにするな」）。
- **kill switch は gate PASS 後即削除**（恒久 fallback 残さない・CLAUDE.md）。
- **憲法遵守**: fail-closed（新 alarm default-deny）/ 隠蔽禁止（isMapped skip 撤去は正方向）/ 品質トレード禁止（A/B で GPU 出力同一）/ read before edit / commit トレーラ禁止 / 新規コメント禁止。
- **幸運ログ禁止**（憲法 6）: 「N 走行クリーン」を安全根拠にしない。防御は「失敗したら何が起こるか」で設計。

## 申告欄（実装計画）
- **省略/未決**: DrawPlan arena サイズ・alpha span 数・DrawPlanItem MAX 定数は実装時に実測確定（設計判断には使わない＝憲法 6）。GLTF PBR opaque の 1 pass 1 shader 性は III-5 で確認。avatar pool（mode 切替）は §2.5 で seed 制約解除済ゆえ on-demand で自然に載る見込み（III-5/6 で確認・本計画では静的群の後）。
- **解釈**: 「大工事一括再実装」= 直列だが各段独立 A/B gate（一括だが検証は段毎）。段階 IV（計測）は III-5/6 の wall 短縮 gate でのみ実施。
- **AYA 決裁待ち**: III-0 の検出器新設着手（憲法 4）/ 各段の kill switch 命名 / III-6 の MoltenVK 検証の実機依頼タイミング。
