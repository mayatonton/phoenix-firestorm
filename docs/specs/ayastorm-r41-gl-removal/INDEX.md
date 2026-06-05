# AYAstorm r41 Vulkan Migration — Doc Index

**最終整理**: 2026-06-06 全 doc audit + 実装側 verification + 追加訂正 commit (= handoff Phase 1.E (sub-letter) complete = Linux primary baseline 確立 + **roadmap §2.1 統一基準 Phase 1 完走 ✅ 確定** (AYA さん合意 2026-06-06)、次 phase entry = Phase 2 = R3 (= UB_REFLECTION_PROBES 本実装) ⏳)

## §0. このディレクトリの構成

r41 milestone の全 spec doc + handoff doc を集約。2026-06-06 整理で **live (= 現状参照すべき) / archive (= 歴史記録)** の 2 layer 構成に再編。

整理前は 00-08 spec + design/01-10 chapter が parallel に並走 + handoff filename が `substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-` の冗長 prefix を引きずって現行 breakdown と不一致だった。整理で:
- (1) η-30 era handoff 100 件 = prefix 剥がし + `handoff/phase1/{a..e}/` sub-letter 別配置 + `handoff/misc/` (special task)
- (2) η-30 以前 handoff 121 件 = `handoff/archive/` 集約
- (3) 01-08 spec + audit/review 11 件 = `archive/` 集約
- (4) 00-charter は historical chain origin として root 残置
- (5) git commit log message 92 件 = `git filter-repo` で旧 path → 新 path 書換 + 47 件の intra-commit hash 参照は filter-repo 内蔵 old→new hash mapping で自動追従更新 (= force-push 後の現状)

## §1. live = 現在参照すべき doc 群

### §1.1 source of truth

> **2026-06-06 audit 訂正**: Phase 完了判定の source of truth は **`design/09-phase-roadmap.md` §2.1 + §4 + §5** (= roadmap 統一基準)。`ayastorm-r41-cross-platform-port-spec.md` §6 は **実装 sub-step (handoff sub-letter) tracking 軸** で並走、Phase 完了判定とは別軸。

| doc | 役割 |
|-----|------|
| [`design/09-phase-roadmap.md`](design/09-phase-roadmap.md) | **Phase 完了判定 source of truth** = §2.1 全 Phase マップ表 + §4 Phase 1 + §5 Phase 2..K + §6 K+1..K+3 + §7 K+4 + §8 K+5 |
| [`ayastorm-r41-cross-platform-port-spec.md`](ayastorm-r41-cross-platform-port-spec.md) | **実装 sub-step (handoff sub-letter) tracking** = §6 PC-N-* 進捗 + §A 履歴 (= chronological log)、§6 表内に roadmap §2.1 上の position 列併記 |
| [`ayastorm-r41-ubo-current-state-inventory.md`](ayastorm-r41-ubo-current-state-inventory.md) | UBO 現状棚卸し (= 起源 handoff = η-28 pivot-to-ubo-design、2026-06-03 snapshot で時間凍結、§6.4 など一部 status update 済) |
| [`reference-shader-location-map.md`](reference-shader-location-map.md) | shader file 配置 reference |

### §1.2 design chapter (= UBO 全体設計、2026-06-03 起案)

`design/` 配下 14 file = `01-overview.md`..`05-existing-inventory-link.md` (= chapter 01-05) + `06a-cache-structure-and-setter-redirect.md` + `06a-prep-phase0-measurement.md` + `06b-cadence-update-site-and-dirty.md` + `06c-descriptor-set-bind-wiring.md` (= chapter 06 を 4 sub-chapter 分割) + `07-vulkan-api-state.md`..`10-open-questions.md` (= chapter 07-10) + `literal-cross-ref-audit.md`。η-28 で「ちゃんと設計」転換時に起案、UBO 26 個積み上げ era の構造欠陥への再設計。

### §1.3 handoff (= phase 進捗記録)

`handoff/` 配下 = Phase 別 + special subdir 構成:

| subdir | count | scope |
|--------|------:|-------|
| `handoff/phase1/a/` | 13 | Phase 1.A (= foundation sub-phase) |
| `handoff/phase1/b/` | 18 | Phase 1.B (= host-side gate + bind 配線) |
| `handoff/phase1/c/` | 40 | Phase 1.C (= PC-0..PC-7ε + PC-N-1..4 = per-program UBO cadence) |
| `handoff/phase1/d/` | 13 | Phase 1.D (= PC-N-5..10 = 実 LL::GLTF::Asset 通電) |
| `handoff/phase1/e/` | 12 | Phase 1.E (= PC-N-11..15c + bridge = 実 data 通電 + worker thread) |
| `handoff/misc/` | 4 | special task (= AYA r20 SSS verify ×2 + upstream uniform4iv bug fix ×2) |

- 最新 marker = [`handoff/phase1/e/handoff-phase1-e-complete.md`](handoff/phase1/e/handoff-phase1-e-complete.md) (= Phase 1.E + Phase 1 全完了 = Linux primary 完成 marker)
- entry = [`handoff/phase1/a/handoff-phase1-a-entry.md`](handoff/phase1/a/handoff-phase1-a-entry.md)
- Phase 2 以降は新設時に `handoff/phase2/` 等を追加

### §1.4 historical chain origin (root 残置)

- [`00-charter.md`](00-charter.md) = r41 milestone charter (= closed 2026-05-28)。起案時点の thesis + 8 領域 work breakdown 起点。現在 phase tracking source of truth は §1.1 cross-platform-port-spec.md に移行済、本 charter は **historical chain origin として参照のみ**。

## §2. archive = 歴史記録

### §2.1 spec docs archive (= Pattern α/β era、2026-05-28 起案 → 2026-06-03 design/ chapter で superseded)

`archive/` 配下 11 file:

- `01-foundation.md` ~ `08-llvkrenderer-skeleton.md` (= 7 file、起案当時の foundation/portage/state-machine/frame-context/shader-spirv/descriptor-renderpass/llvkrenderer-skeleton 設計、現在は `design/` で再設計済)
- `audit-past-b-work-vs-design-2026-06-03.md` (= η-28 design pivot 時の audit)
- `design-review-2026-06-03.md` + `design-review-2026-06-03-second-pass.md` (= 設計章一次/二次査読)
- `bundle-A-skip-list.txt`

### §2.2 handoff archive (= η-30 以前)

`handoff/archive/` 配下 121 file:

- `handoff-stage-{1..4}*.md` + `handoff-r41-charter-complete.md` + `handoff-session-pause-*.md` (= 9 file、Pattern α/β era、stage 1-4 breakdown)
- `handoff-substep-{2,3,4-{1,2}}-*.md` (= 23 file、substep 2-1a..4-2 breakdown)
- `handoff-substep-4-3-{alpha,beta,gamma-prime-{prep,port-{alpha,beta-1,beta-2-bundle-{A,B-B*-eta-{1..29}}}}}-*.md` (= 89 file、bundle A/B + η-1..η-29 breakdown)

## §3. filename migration legend (= 2026-06-06 整理対応表)

### §3.1 handoff prefix 剥がし + phase sub-letter 配置 (η-30 era)

旧 → 新:
```
handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-X-*.md
  ↓
handoff/phase1/X/handoff-phase1-X-*.md   (X = a|b|c|d|e)
```

special task:
```
handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-(aya-r20|upstream-uniform4iv)-*.md
  ↓
handoff/misc/handoff-(aya-r20|upstream-uniform4iv)-*.md
```

例:
- 旧 `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-complete.md`
- 新 `handoff/phase1/a/handoff-phase1-a-complete.md`

### §3.2 handoff archive 集約 (η-30 以前)

旧 → 新:
```
handoff/handoff-(stage|r41|session|substep-(2|3|4-[12]|4-3-(alpha|beta|gamma-prime-...)))-*.md
  ↓
handoff/archive/handoff-(stage|r41|session|substep-...)-*.md
```

**変換規則**: 旧 path の `handoff/handoff-` を `handoff/archive/handoff-` に置換、ただし `handoff/handoff-phase1-*` (= 現行 η-30 era) は §3.1 規則に従う。

### §3.3 spec archive (= 01-08 + audit/review)

| 旧 | 新 |
|---|---|
| `01-foundation.md` | `archive/01-foundation.md` |
| `02-portage-execution.md` | `archive/02-portage-execution.md` |
| `03-state-machine-pso.md` | `archive/03-state-machine-pso.md` |
| `04-frame-context.md` | `archive/04-frame-context.md` |
| `06-shader-spirv.md` | `archive/06-shader-spirv.md` |
| `07-descriptor-renderpass.md` | `archive/07-descriptor-renderpass.md` |
| `08-llvkrenderer-skeleton.md` | `archive/08-llvkrenderer-skeleton.md` |
| `audit-past-b-work-vs-design-2026-06-03.md` | `archive/audit-past-b-work-vs-design-2026-06-03.md` |
| `design-review-2026-06-03.md` | `archive/design-review-2026-06-03.md` |
| `design-review-2026-06-03-second-pass.md` | `archive/design-review-2026-06-03-second-pass.md` |
| `bundle-A-skip-list.txt` | `archive/bundle-A-skip-list.txt` |

注記: `00-charter.md` + `05-*` (起案時点で欠番) は対象外。

## §4. git commit log との関係

整理前 91 commit (= origin より ahead) + doc cleanup commit 1 件 = 計 92 commit の message 内で旧 filename / 旧 path を多数参照していた。2026-06-06 整理で `git filter-repo` を実行:
- **message-callback (path 書換)** = 全 commit message 内の旧 path 文字列 (= `handoff-substep-...-eta-30-` prefix / ellipsis form / pre-η-30 forms / spec archive forms) を新 path に置換
- **filter-repo 内蔵 hash mapping** = path 書換で commit hash 全件変化 → message 内の 47 件 intra-commit hash 参照 (= 「commit `xxxxxx`」記述) を **同一 run 内で自動追従更新** (= filter-repo default 動作、`--replace-refs update-or-add` 効果)
- **force-push** で feature branch `feature/ayastorm-r41-gl-removal` 上書き

結果: 全 92 commit の message + 内部 hash 参照は **全て新構造と整合**。`git log --follow <new_path>` で rename 跨ぎ完全 history 取得可能。

### §4.1 git log --follow

特定 file の改変履歴を辿る場合は `git log --follow <new_path>` で rename を跨いだ完全 history 取得可能。

### §4.2 force-push 前 backup branch

filter-repo 実行前の 91 commit state は backup branch `feature/ayastorm-r41-gl-removal-pre-doc-cleanup` (= 2026-06-06 作成) に保存。必要時 cherry-pick / reset 可能。

## §5. 整理対象外 (= 残存する narrative 表現)

handoff doc 内の narrative shorthand `handoff-substep-...-pc-N-X-...md` (= 一部 `...` ellipsis 形式の参照) は **file path 文字列ではなく説明文の省略表記** ゆえ未改変。読み手で `handoff-phase1-X-pc-N-Y-...md` と読み替えてください。実 file path は §3.1 規則で復元可能。

00-charter.md の §A 履歴 narrative + 本文内の handoff 参照は §3.2 規則で archive/ path に変換済 (= 2026-06-06 整理時点)、§A 時系列記述自体は historical chain として原文保持。

## §6. r41 milestone state (= 2026-06-06 全 doc audit 訂正後)

### §6.1 handoff sub-letter 完了状態 (= 実装進捗 tracking 軸)

- Phase 1.A ✅ + Phase 1.B ✅ + Phase 1.C ✅ + Phase 1.D ✅ + Phase 1.E ✅ 本訂正 commit baseline = **handoff sub-letter Phase 1.E complete ✅ = Linux primary baseline 確立** (= Vulkan path 通電 + GLTF asset cluster pilot 通電 + worker thread 並列化 baseline)

### §6.2 roadmap §2.1 統一基準 (= Phase 完了判定 source of truth、AYA さん合意 2026-06-06)

- roadmap §2.1 Phase 0 (計測) ✅ (= eta-29 phase0-step1/2/4/5-complete archive + design/06a-prep-phase0-measurement.md §5.5)
- roadmap §2.1 **Phase 1 (codegen + redirect 層整備 + shell UBO 1 個) ✅ 完走** (= 2026-06-06 audit + 追加訂正で確定):
  - 1.A (codegen pipeline + 96 unique UBO name / 94 codegen block) ✅
  - 1.B (31 setter Vulkan path 分岐 transparent) ✅ (= inventory.md §6.4 「未着手」literal は 2026-06-03 snapshot、本訂正で update 済)
  - 1.C (UB_REFLECTION_PROBES shell `Global_ReflectionProbes` で 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電) ✅ (= shell 段階 = roadmap §4.2 Exit Criteria literal 充足、shell の実 member + 実 dirty + 実 flush 置換は Phase 2 仕事)
- roadmap §2.1 Phase 2..K (= 1 UBO ずつ migration、Template A 順 = UB_REFLECTION_PROBES → GLTFMaterials → GLTFNodes → GLTFJoints → ...) **次 phase entry**:
  - **Phase 2 = R3 (= UB_REFLECTION_PROBES 本実装、shell に実 data 入れる) 着手**
  - Phase 2/3 詳細 scope 定義 = AYA さん指示 2026-06-06 で Phase 2 着手前 separate session で確定
  - 現実装 handoff Phase 1.D/1.E は **Template A 順序逸脱した pilot 先回り着手** (= Skin_GLTFJoints + PerDrawUBO_LightParams「zero IS real data」semantic) 状態で実装側 baseline 整備済、Phase 2 本実装の前提条件に資する
- roadmap §2.1 Phase K+1/K+2/K+3 (3 OS 確証 Linux/Win/Mac、(Q4) C Linux 先行 → Win/Mac 並走) ⏳ = Linux primary 全 Phase 完走後着手 (= AYA さん明示 2026-06-05)
- roadmap §2.1 Phase K+4 (OpenGL path 撤廃、(Q3) A 全 UBO 移行完了まで並走) ⏳
- roadmap §2.1 Phase K+5 (release 整備) ⏳

### §6.3 Phase 1 完走判定 + Phase 2 着手 scope (= handoff-phase1-e-complete.md §5.2 参照)

**2026-06-06 audit + 追加訂正で Phase 1 完走 ✅ 確定** (= roadmap §2.1 統一基準 Phase 1.A/B/C 全 ✅ + handoff Phase 1.A..1.E 全 ✅):
- ✅ R1 (inventory.md §3 件数 update + §6.4 status update) = 本訂正 commit で消化
- ✅ R2 (Phase 1.C shell 段階 完了 verification = UB_REFLECTION_PROBES shell で 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電) = 本 audit で literal 確認
- ✅ R9 (handoff-phase1-e-complete.md 自己矛盾解消) = 本訂正 commit で消化
- ✅ R10 (Phase 番号体系 4 系統整合) = 本訂正 commit で消化
- ⏳ R7 (GLTF asset rez first-fire marker 発火確認) = minor item、AYA 実機 session 1 回で消化可能
- ⏳ R8 (design/10 残 10 件 AYA 判断) = deferred、別 batch session で消化 (= 2026-06-06 audit で 16→10 件縮小)

**Phase 2 着手 scope (= AYA さん指示 2026-06-06 = Phase 2 着手前 separate session で Phase 2/3 詳細 scope 定義)**:
- R3 = roadmap Template A 順 Phase 2 = UB_REFLECTION_PROBES 本実装 (= shell `Global_ReflectionProbes` に実 member + 実 dirty 判定 + 実 flush logic 入れる)
- R4 = 実 PBR shader 接続 (= sGltfStubAssetPipeline 撤去)
- R5 = 「zero IS real data」semantic 卒業 (= PerDrawUBO_LightParams real 内容投入)
- R6 = avatar Vulkan draw 通電 (= sPlaceholderSkin 撤去)
- R4/R5/R6 所属 Phase 番号 = Phase 2/3 定義時に AYA さん判断確定

### §6.4 詳細 reference

- 実装 sub-step tracking (= handoff sub-letter 軸): §1.1 `ayastorm-r41-cross-platform-port-spec.md` §6 phase tracking
- roadmap 統一基準: `design/09-phase-roadmap.md` §2.1 + §4 + §5
- handoff Phase 1.E (sub-letter) complete marker: [`handoff/phase1/e/handoff-phase1-e-complete.md`](handoff/phase1/e/handoff-phase1-e-complete.md)
- Phase 1 完走 gate 残件 R1-R10 詳細: 同 handoff §5.2
