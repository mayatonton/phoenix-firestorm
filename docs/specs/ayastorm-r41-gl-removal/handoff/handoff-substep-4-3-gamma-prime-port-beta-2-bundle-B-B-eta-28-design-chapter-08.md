# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: chapter 08 起案完了 + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件確定 → AYA commit 指示待ち

**完了日**: 2026-06-03
**位置付け**: 前 handoff (chapter 07) を受けて chapter 08 (build-codegen-pipeline) 起案完了。**AYA commit 指示待ち** → 次 session で commit → chapter 09 (phase-roadmap) 起案へ。

---

## §0 現在の git state

- **unstaged**: `design/01-overview.md` (§4 進捗表 08 → ✅ 起案済)
- **untracked**: `design/08-build-codegen-pipeline.md` (774 行) + 本 handoff doc + `tests/` (= **絶対除外**)
- branch = `feature/ayastorm-r41-gl-removal`、commit 12 進み (push 未)

---

## §1 chapter 08 確定形 default (= AYA 判断仰ぎ候補)

詳細は **`design/08-build-codegen-pipeline.md` §17** で集約参照。本 handoff では pointer のみ:

| 持越 | default 採用案 | chapter 08 該当節 |
|---|---|---|
| (B1) Codegen 実装言語 | Python 3.8+ | §3.2 |
| (B2) glslang 統合 | autobuild vendoring (既存温存) | §4.2 |
| (P) GLSL parse | 独自 mini-parser + glslang -E 前処理 | §5.2 |
| (A1) std140 offset | Codegen 独自 calculator + SPIR-V reflection 二重保証 | §5.4 |
| (G/B3) perfect hash | 独自 Python frozen-table (CHD/FCH) | §5.6 |
| (B4) 増分 build cache | hash + mtime 併用 | §11.2 |
| (B5) Codegen 実行 trigger | CMake DEPENDS 自動 + 手動 `codegen_ubo_force` target 併設 | §12.2 |

その他確定: `ubo_metadata.inl` schema (§6.1) / 256 B padding (§6.4) / set=1 79 → 40/39 lexicographic sort split (§7.1) / dummy buffer host init (§8.2) / build error 7 種 E1-E7 (§9.1) / `ubo_host_loader.inl` (§10) / 3 OS 互換性 (§13)。

実装 phase 入口持越: (P-future) unused mask 参照解析の要否、(cache-grow) cache GC。

---

## §2 次 session = AYA commit 指示後 commit phase → chapter 09 起案

### §2.1 commit 単位 (明示パス、`tests/` 厳守除外)

```
git add docs/specs/ayastorm-r41-gl-removal/design/01-overview.md
git add docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md
git add docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-08.md
```

commit subject:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 UBO 全体設計 chapter 08 (build-codegen-pipeline) 起案 + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件確定
```

### §2.2 chapter 09 (phase-roadmap) scope

- Phase 番号体系再編 (= r41 sub-step 配下に UBO 化 phase を組込)
- 1 UBO ずつ migration (memory `feedback_ubo_migration_one_at_a_time` 準拠) の Phase 内訳
- Phase 0 計測 (06a-prep §3 / 07 §3.3 device limit) を各 migration phase 入口で必須前提化
- 3 OS 確証 Phase (= 08 §13.4 X-α/β/γ を具体 Phase 番号化)
- Phase 完了判定基準 (= cold launch + log + canary 等、memory `feedback_build_only_verified` 準拠)
- 持越 (V1')(V3')(S3')(W)(W2)(R1)(PSC)(RF) + 08 (A1)(P)(G/B3)(B1)(B2)(B4)(B5)(P-future)(cache-grow) を Phase に紐付け
- 起案完了見込み ~400-500 行

### §2.3 AYA 判断仰ぎ候補 (暫定)

- (R1) 第 1 UBO migration 選定 (= 最小リスク UBO vs 最頻出 UBO)
- (R2) Phase 当たり migration UBO 数 (= 1 厳守 vs 関連 cluster で 2-3 UBO 同 Phase)
- (R3) OpenGL path 維持期間 (= 全 UBO 移行完了まで並走 vs 中間 Phase で撤廃)
- (R4) 3 OS 確証 Phase 順序 (= Linux first vs 並走)
- (R5) Phase 0 計測 phase の Phase 番号化

---

## §3 必須 Read 3 件 (= **これ以上事前 Read しない**、不足は chapter 09 起案中に現場 pinpoint Read)

1. **本 handoff doc** (= chapter 08 完了状態 + 確定形 pointer + chapter 09 scope)
2. **`design/01-overview.md`** (= 全体概観 + 確定事項 13 件 + chapter 01-10 構成、Phase 番号体系の出発点)
3. **`design/06a-prep-phase0-measurement.md`** (= 既起案 Phase 0 計測 spec、chapter 09 で Phase 番号付与する対象)

**= 前 handoff §1.4.2 にあった「触れる論点 → pinpoint Read 表」(12 件) は廃止**。chapter 02-08 / inventory / その他は chapter 09 起案中に必要になった**その時のみ** pinpoint Read。事前全読禁止 (memory `feedback_handoff_minimal_pre_req_read`、AYA 指摘 2026-06-03)。

---

## §4 規律 (絶対遵守)

- **design-phase**: `indra/` 配下改変ゼロ (memory `feedback_design_phase_no_code_write`)
- **tests/**: commit / 確認 / 言及禁止 (memory `feedback_tests_dir_never_commit`)
- **commit**: AYA 明示指示まで実行しない (memory `feedback_no_auto_commit`)、`git add -A` / `git add .` 禁止 (明示パスのみ)
- **事前 Read**: §3 の 3 件のみ全読、その他は現場 pinpoint Read (memory `feedback_handoff_minimal_pre_req_read`)
- **完了時**: chapter 09 起案完了で AYA に「09 揃いました」報告 → 同様 commit 指示待ち、最終 chapter 10 (open-questions) へ
- **context 残量**: 周回境界で自分から handoff doc 提案 (memory `feedback_proactive_handoff`)

---

**= 本 handoff doc + §3 の Read 3 件で chapter 09 起案再現可能**。
