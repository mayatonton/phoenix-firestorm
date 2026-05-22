# AYAstorm r30 BD full port — Phase 4 3-mode 検証 spec

**Phase 名**: r30 BD 完全移植 Phase 4 — 3 mode 受入検証
**前提**: Phase 3.7 (C++ Cinematic dispatch) / Phase 3.8 (shader Cinematic mount) / Phase 3.9 (UI mount — Cinematic Controls floater + BD env floater) 完了
**作成日**: 2026-05-19
**ブランチ**: `feature/ayastorm-r30-bd-full-port-inventory`
**章方針**: `feedback_release_with_user_feedback.md` (exhaustive な solo acceptance を組まない、tag/release 後にユーザーフィードバックで補う) + `feedback_no_escape_full_bd_coverage.md` (BD parity から逃げない)

---

## §0 目的 / 非目的

### 目的
Phase 3.7-3.9 で landed した 3 mode dispatch / shader mount / UI mount が、AYA 1 人で 30 分以内に判定できる **smoke + parity check** を通過することを確認する。通過したら r30 BD full port を release candidate にし、tag → β release → AYA 配布まで進める。

### 非目的
- 全 shader file の per-mode visual A/B diff (Phase 3.8 で per-file commit 単位の cinematic_bd 検証が landed 済、本 phase で再走しない)
- frame profile での perf 数値受入 (Phase 5 cleanup 後の β feedback で別途観測)
- BD upstream との pixel-perfect 比較 (BD は独自 cvar default で動くため固定参照しない)

---

## §1 検証マトリクス (3 mode × 4 軸)

| 軸 | mode 0 Firestorm View | mode 1 AYAstorm View | mode 2 Cinematic |
|---|---|---|---|
| A. 起動 / sky 描画 | 起動 / sky 通常描画 | 起動 / r14-r20 AY 拡張動作 | 起動 / BD pipeline 動作 |
| B. shader permutation | AYASTORM_CINEMATIC=0 / class*/ | AYASTORM_CINEMATIC=0 / class*/ | AYASTORM_CINEMATIC=1 / cinematic_bd/ 優先探索 |
| C. UI mount | Cinematic Controls 任意 | Cinematic Controls 任意 | Cinematic Controls floater で BD pipeline 操作 |
| D. cvar refresh | 通常 (Preferences) | 通常 (Preferences) | Cinematic Controls floater slider が即時反映 |

---

## §2 検証手順 (AYA 実行)

### §2.1 前提

- AYAstorm-release branch 上で feature/ayastorm-r30-bd-full-port-inventory を merge / cherry-pick (まだの場合)
- `~/ayastorm/` に install 済 (build 手順は memory `project_build_procedure.md`)
- `~/.ayastorm_x64/cache/shader_cache/` clear 済 (memory `project_ayastorm_shader_cache_path.md`)

### §2.2 mode 0 (Firestorm View) — 5 分

1. `Debug Settings` → `AYAVisualRealismEnabled = 0` set
2. viewer 再起動
3. 起動完了 / sky / water / avatar が AY 通常通り描画されるか目視
4. `~/.ayastorm_x64/logs/AYAstorm.log` の last 50 行に shader compile error が無いか確認

**通過**: 3 / 4 green

### §2.3 mode 1 (AYAstorm View) — 5 分

1. `AYAVisualRealismEnabled = 1` set / 再起動
2. r14-r20 AY 拡張 (atmosphericsFuncs / softenLight / skyV / cloudsF) が動いて見えるか目視 (空の atmospheric perspective / cloud 立体感 / softenLight 効果)
3. log の shader compile error 確認

**通過**: 2 / 3 green

### §2.4 mode 2 (Cinematic) — 15 分

1. `AYAVisualRealismEnabled = 2` set / 再起動
2. 起動完了 (起動中 hang や crash なし)
3. メニュー `Avatar` → `Cinematic Controls...` (shortcut `Alt+C`) で `floater_aya_cinematic.xml` を開く
4. Cinematic Controls floater 内 slider / button が動く (例: shadow distance slider を動かして即時反映)
5. sky / water が BD pipeline で描画 (AY 拡張 OFF / BD baseline / cinematic_bd の shadowUtil + screenSpaceReflUtil 経路)
6. Cinematic Controls floater から `env_adjust_water` / `env_settings` が開けて即時反映する
7. log の shader compile error / NULL pointer crash 確認

**通過**: 2-7 すべて green。1 件でも red なら Phase 4 fail、原因 file を spec 化して fix commit。

### §2.5 mode 切替 round-trip — 5 分

1. mode 2 → 1 → 0 → 2 を再起動を挟まずに cvar 切替で連続実施 (3 回)
2. 各切替で hang / crash / shader cache 不整合の log が出ない
3. mode 2 戻り時に Cinematic Controls floater が引き続き操作可能 (再起動なしで slider が即時反映)

**通過**: 2 / 3 green

---

## §3 受入条件 (release gate)

| ID | 条件 | 必須 | 状態 | 失敗時の扱い |
|---|---|---|---|---|
| G1 | §2.2 mode 0 全通過 | **必須** | (AYA 検証待ち) | Phase 4 fail。Firestorm baseline 退行は release blocker |
| G2 | §2.3 mode 1 全通過 | **必須** | (AYA 検証待ち) | 同上。AYAstorm r14-r20 機能退行は release blocker |
| G3 | §2.4 mode 2 全通過 (起動 + Cinematic Controls floater + env floater) | **必須** | (AYA 検証待ち) | Phase 4 fail。BD full port 完了 = mode 2 動作が真の合格基準 |
| G4 | §2.5 round-trip 全通過 | 推奨 | (AYA 検証待ち) | 失敗時は β release note で「mode 切替は再起動推奨」明記すれば release 可 |
| G5 | log に未対応 shader compile error が無い | **必須** | ✅ (2026-05-19) | 1 件でも error あれば調査 → fix commit |

**G5 達成根拠 (2026-05-19)**:

- mode 2 post-paradigm-shift + post-volumetric-gate audit (`ayaudit_run.sh A` with `AYAVisualRealismEnabled=2`、49 cvar × low/high × 3 sample × 1 profile sweep) で **`grep -cE "C3002|Linker Error|undefined function" /tmp/aya-audit-run-A.log` = 0**
- 経緯と fix: `docs/specs/ayastorm-r30-p5-bd-parity-spec.md` §10.6.1 を参照 (`volumetricLightF.glsl` Cinematic body の `HAS_SUN_SHADOW` gate 非対称、commit `431f0157f2` で対称化)
- mode 0 / 1 (AY branch) は元から `#ifdef HAS_SUN_SHADOW` で body 全体を gate 済、`use_sun_shadow=false` 時に passthrough する設計のため、本 fix と同パターンの link error は構造的に起きない (`P5 spec §10.6.1` Strategy C 健全性 sweep で確認)

---

## §4 失敗時の対応フロー

`feedback_doubt_self_first.md` / `feedback_render_full_trace_first.md` 準拠:

1. AYA から log + 画面状況を hand-off
2. Claude が log を読む (`feedback_log_reading.md`、AYA に貼らせない)
3. 該当 path を C++/GLSL の trace で特定 (推論禁止、`feedback_render_full_trace_first.md`)
4. fix commit 1 件 / build 1 巡 / §2 の該当 mode のみ再走 (全 mode 再走は不要)
5. fix 後 fail 抜けたら G1-G5 該当項目を ✅ に更新して spec re-commit

### §4.1 実行事例: Cinematic system body velocity の lightning streak (2026-05-19)

| 項目 | 内容 |
|---|---|
| 検出 | §2.4 mode 2 検証中、AYA 目視で「緑の雷状アーティファクト」報告 |
| trace | `motionBlurF.glsl` 分岐確認 → AY branch は guards で抑止、Cinematic branch (BD-pure) は無防備 → `LLDrawPoolAvatar::renderMotionBlur` が `avatarVelocityV.glsl` を bind するが `lastMatrixPalette[45]` を upload していない (BD 本 path の未完成部分) → shader が garbage を読み巨大 velocity → diffuse が方向に引き伸ばされ streak |
| 仕様判定 | BD baseline 995a1354d8 では `LLDrawPoolAvatar::renderMotionBlur` 全体が `/* ... */` で commented out。`feedback_bd_full_port_only` に従い同じ no-op 挙動に整合させる (= BD 本線から外れていた現状を本線に戻す修正) |
| fix commit | `3cdbc28194` (`r30 BD full port: Cinematic system body velocity write を BD baseline に整合`) |
| 影響範囲 | classic / system avatar body の motion blur が Cinematic 中対象外 (BD baseline と一致)。rigged mesh attachments は他 pool 経由で blur 対象として残る (AYA 視覚確認済) |
| release note | ja/en/zh 「既知の留意点」に明示済 |

---

## §5 完了 commit

- `r30 BD full port Phase 4: 3 mode 受入検証完了 (G1-G5 all green)`
- 失敗 fix が発生した場合は fix commit を Phase 4 内で landed させ、最後に上記 1 行で締める

---

## §6 Phase 5 への引き継ぎ

Phase 4 通過後、Phase 5 で扱う cleanup:

- 検証用 LL_INFOS / hook を出荷物から除去 (`feedback_remove_verification_logs.md`)
- AYA 検証で触った debug settings の default 戻し案内表 (`feedback_restore_debug_settings.md`)
- release note 草案 (`feedback_release_notes_link_only.md` / `feedback_release_note_per_feature.md`)
- cinematic_bd/ 内 license / attribution 確認

---
