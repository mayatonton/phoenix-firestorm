# AYAstorm r30 BD 完全移植 — Phase 2 Spec

**Phase 名**: r30 BD 完全移植 Phase 2 — architectural decision + sub-phase 計画
**前提**: `docs/specs/ayastorm-r30-bd-full-port-inventory.md` (Phase 0) + `docs/specs/ayastorm-r30-bd-full-port-phase1-audit.md` (Phase 1)
**作成日**: 2026-05-19
**作成方針**: D1-D4 architectural decision を確定 (= 「Phase 3 で決める」punt 禁止、`memory/feedback_no_escape_full_bd_coverage.md`)、Phase 3 sub-phase の境界を切る

---

## §0 Phase 2 の目的

Phase 1 までで:

- Phase 0 (inventory): BD と AYAstorm の描画系全 file 5 bucket の ground truth
- Phase 1 (audit): r30 既存 commit + r14-r20 gating の inventory 照合結果

ground truth が揃ったので、本 Phase 2 で残り 4 つの architectural decision を確定し、Phase 3 (実装) を分割する。

architectural decision は **本 spec 内で確定する** = Phase 3 への punt 禁止。判断軸を明示し、`feedback_bd_full_port_only.md` の本線 (BD 描画処理の完全移植) からの逸脱を自検する。

---

## §1 D1: r14-r20 効果は mode 2 (Cinematic) で全 OFF

### §1.1 確定

mode 2 (Cinematic) では r14 / r15 / r16 / r17 / r18 / r19 / r20 すべての AYAstorm 視覚表現章機能を **完全に OFF** にする。Cinematic 内で AYAstorm 独自表現を 1 mm も混ぜない。

### §1.2 根拠

- `feedback_bd_full_port_only.md` の唯一の道 = BD 描画処理の完全移植
- BD は r14+ 機能を持たない → Cinematic で active = parity 不成立
- Phase 1b §1b.2 で確定: predicate を `== 1` に統一すれば mode 1 (AYAstorm View) は r14+ 維持、mode 2 (Cinematic) は r14+ off、mode 0 (Firestorm View) は元から off で全 mode 仕様通り

### §1.3 影響範囲

`memory/feedback_warn_aya_off_bd_line.md` への自検:

- AYAstorm View (mode 1) ユーザーは引き続き r14+ 機能の絵が見える → 退行なし
- Firestorm View (mode 0) ユーザーは元から r14+ off → 退行なし
- Cinematic (mode 2) は現状 r14+ + BD borrows のハイブリッド → r14+ 消えて純 BD パス側へ動く → これは **意図された変化** (= 章 thesis 通り)

「Cinematic で r14+ も BD borrow も両方見える方が綺麗」という意見が将来出る可能性があるが、それは BD parity 後の P6+ AYA 独自色作り phase で再導入する選択。Phase 2-5 中は r14+ 完全 off を堅守。

### §1.4 D1 → Phase 3 タスク

- **Phase 3.1**: 6 predicate site (`pipeline.cpp` x3, `llsettingsvo.cpp` x3) を `== 1` 判定に統一 (Phase 1b §1b.2 表参照)
- Shader 側 predicate (atmosphericsFuncs / godraysF / skinSSSF / skyV) は中央集権 uniform 経由のため別途修正不要

---

## §2 D2: C++ 系 file は AY 単一実装内で mode 別 dispatch、別 file 化しない

### §2.1 確定

`pipeline.cpp` / `lldrawpool*.cpp` / `llviewershadermgr.cpp` 等の C++ 描画系 file は AY 単一実装を維持し、内部で mode 別に dispatch する。BD-equivalent code path を AY file 内に追加 (= `if (cinematic) { BD-equivalent logic } else { AY logic }`)。BD 専用の別 file (= `pipeline_bd.cpp` 等) は作らない。

### §2.2 根拠

- 別 file 化すると BD upstream 取り込み時の merge 担当が二重化、保守困難
- 単一 file 内 dispatch なら r14+ AY 実装と BD 実装が同所にあり、code review で差分が見える
- LL 標準の `LLCachedControl<U32> aya_view_mode` パターンで mode 別 dispatch は既に確立 (P2/P3/P4 commit で実証)
- ただし「BD logic を 1:1 で AY に持って来る」原則は維持 = dispatch 先の関数 body は BD pipeline.cpp / drawpool*.cpp の対応部を逐行移植

### §2.3 自検 (warn rule)

`feedback_warn_aya_off_bd_line.md` 適用: 「AY の既存 r14+ 実装を活かして Cinematic でも一部使い回し」approach は **不採用** (= 増分 borrow / 推論パターン)。BD pipeline 順序と一致しない使い回しは parity 不成立を呼ぶ。

### §2.4 D2 → Phase 3 タスク

- **Phase 3.2**: Phase 1a で REDO 判定された C++ file (pipeline.cpp / lldrawpool*.cpp / llviewershadermgr.cpp 等 92 file) について、Cinematic 時 BD-equivalent dispatch を per-function spec として下層 spec (`phase3.2-cpp-dispatch-spec.md`) に書き起こす
- 下層 spec ができてから Phase 3 で実装

---

## §3 D3: cvar は Cinematic mode で BD default を hard-code、preset 機能では扱わない

### §3.1 確定

Cinematic mode (mode 2) で BD-equivalent な描画結果を得るために、影響する cvar (= bucket 3 で `value diff` または `type diff` または `BD-only` 判定された約 47 件 + AY-only Render* 系 579 件のうち render path で参照されるもの) は、mode 2 時に **hard-coded BD default を読む dispatch 層**を pipeline 内に追加する。

- `gSavedSettings` を mode 切替で書き換えない (= 他 mode に副作用が出る)
- Cinematic 用 preset (= `gSavedSettings` への一括 push) も採用しない (= 配信者が後で個別 tweak → 純 BD parity 崩壊 = `feedback_bd_full_port_only.md` 違反)
- 代わりに `LLCachedControl<T>` の代わりに mode 別 effective value を返す helper (例: `gPipeline.getRenderCvar<T>(cvar_name, bd_default, ay_default)`) を導入、mode 2 のときは BD default を返す

### §3.2 根拠

- preset 採用 = AYA が「BD-compat preset 1 発投入」を P5 で再考、`feedback_bd_full_port_only.md` で **不採用 approach として明文化**
- preset 経由だと配信者の手動 tweak で BD parity が崩れる → 純 BD パス保持の責務が pipeline 側に無いと parity が user 設定に左右される
- hard-code 方式なら mode 2 中の cvar 読みは常に BD default、parity が pipeline 側に閉じる

### §3.3 D3 → Phase 3 タスク

- **Phase 3.3**: render cvar helper (= `getRenderCvar<T>` または相当) を pipeline.h / pipeline.cpp に追加、render path から touched される cvar 群を helper 経由に置き換え
- **Phase 3.4**: bucket 3 の 47 件 (BD-only 7 + value diff 33 + type diff 7) を helper の BD default 表に組み込み
- **Phase 3.5**: AY-only Render* cvar 579 件のうち render path で参照されるもののリスト確定 (= grep) → mode 2 default を BD-default-相当 (= 機能 off 値) に hard-code

### §3.4 自検 (warn rule)

「Cinematic 用 preset を 1 発投入」案は本 spec で却下した。逆に「mode 2 中の cvar 読みを helper で BD default に固定」は推論ではなく、ground truth (= BD default 値) を mechanical に bind するだけ → 本線。

### §3.5 補足: 配信者の自由度

mode 2 では cvar tweak は無効 (helper が無視) になるため、配信者の絵作り余地は mode 2 では失われる。AYA 独自色作り (= LUT / 各種 tuning) は **mode 1 (AYAstorm View)** または **P6+ で独立 mode を追加**する方向で対応する想定。本 spec scope 外。

### §3.6 type-diff 7 cvar の dispatch (Phase 3.4 part 4 追補)

bucket 3.4 (`docs/specs/ayastorm-r30-bd-full-port-inventory.md` §3.4) の type-diff 7 cvar は、helper の typed signature (`getRenderCvarF32` 等) では一発で扱えないため、cvar 毎に dispatch 方針を確定する。

| cvar | BD shape | AY shape | dispatch 方針 | 担当 Phase |
|---|---|---|---|---|
| `RenderMotionBlurStrength` | S32 / 32 | U32 / 32 | **AY を BD 型 (S32) に揃え**、`LLCachedControl<S32>` 化、値同じ | **3.4 part 4 完了** |
| `RenderSSAOEffect` | F32 / -0.5 | Vector3 / (0.80, 1.00, 0.00) | use site で mode 2 のとき BD F32 (-0.5) を hardcode、mode 0/1 は Vector3 を読む。AY settings.xml は据置 | 3.7 |
| `RenderScreenSpaceReflectionAdaptiveStepMultiplier` | Vector3 / (1.13, 1.5, 2) | F32 / 1.6 | 同上 (Vector3 hardcode in mode 2) | 3.7 |
| `RenderScreenSpaceReflectionDepthRejectBias` | Vector3 / (1.0, 0.0, 0.001) | F32 / 0.001 | 同上 | 3.7 |
| `RenderScreenSpaceReflectionDistanceBias` | Vector3 / (10.0, 0.4, 10) | F32 / 0.015 | 同上 | 3.7 |
| `RenderScreenSpaceReflectionIterations` | Vector3 / (64, 16, 16) | S32 / 25 | 同上 (各 SSR pass 毎に異なる iteration 数) | 3.7 |
| `RenderScreenSpaceReflectionRayStep` | Vector3 / (0.025, 0.75, 1) | F32 / 0.1 | 同上 | 3.7 |

`RenderSSAOEffect` 以外の 5 SSR cvar は BD が **per-pass (Vector3 の x/y/z = SSR pass 0/1/2)** で扱う設計であり、AY の単一 scalar 設計とは shape が異なる。これは Phase 3.7 で SSR pass loop を BD 構造に移植する際に同時対応する (= shader uniform への送り方が変わる)。

helper signature 不一致のため Phase 3.4 では完結できず、本 spec で **Phase 3.7 dispatch 内で per-site 対応** と確定。

---

## §4 D4: windlight preset は data 層、Cinematic mode では BD interpretation で render

### §4.1 確定

`indra/newview/app_settings/windlight/{skies,water,days}/` の preset asset は **data 層**として扱い、Cinematic mode で AY-only preset を選択不可にする UI gate は **入れない**。配信者は任意の preset を選択でき、ただし mode 2 中は render path が **BD interpretation** で値を解釈する (= AY 拡張 cvar / AYAR17ColorTemperature 等は no-op、preset 値は BD shader chain が読む)。

bucket 5.1 で「BD-only windlight preset 63 件」が確定 → これらは Cinematic mode のために AY に追加移植 (= file copy)。

### §4.2 根拠

- preset は data、AY-only preset を「Cinematic で使えない」UI 表示は配信者の操作性を不必要に削る
- mode 2 中の render は D1 で r14+ off、D3 で cvar BD default 化 → preset 値の interpretation は自動的に BD 経路に乗る
- 配信者が「Cinematic で AY preset 使うと AYAstorm View と絵が違う」と気付くのは intended (= mode は user に明示済み)

### §4.3 D4 → Phase 3 タスク

- **Phase 3.6**: bucket 5.1 の BD-only windlight sky preset 63 件を AY 側 `windlight/skies/` に file-level copy (= AY-only preset と共存)
- bucket 5.2 (water) / 5.3 (days) も同様、件数は Phase 0 inventory 参照
- UI gate (preset 選択制限) は入れない、release note で「Cinematic 中の preset 選択結果は BD interpretation」と明示

### §4.4 自検 (warn rule)

「AY-only preset を Cinematic で非表示」案は採用すれば配信者操作性を削る → 不採用。「BD-only preset を移植」は ground truth 由来 → 本線。

---

## §5 Phase 3 sub-phase 計画

D1-D4 を受けて、Phase 3 (実装) を以下 sub-phase に分割。各 sub-phase は独立 commit / 独立 build / 独立検証可能な粒度に切る。

| Sub-phase | スコープ | 入力 | 出力 (commit) | 検証 |
|---|---|---|---|---|
| **3.1** | r14+ predicate flips (D1) | Phase 1b §1b.2 表 (6 行) | pipeline.cpp x3, llsettingsvo.cpp x3 patch | build 通る、mode 1 = AYAstorm View 退行なし、mode 2 = r14+ 効果消える |
| **3.2** | C++ Cinematic dispatch spec 起こし (D2 準備) | Phase 1a REDO C++ 92 file | `phase3.2-cpp-dispatch-spec.md` (= per-file dispatch 計画) | spec doc 完成 (実装はしない) |
| **3.3** | render cvar helper 追加 (D3) | D3 §3.3 task list | pipeline.h / pipeline.cpp に `getRenderCvar<T>` helper、47 cvar BD default 表 | helper 単体テスト (mode 1/2 で異なる値を返す) |
| **3.4** | render cvar helper 適用 (D3) | bucket 3 47 件 + render path grep | 各 render path で `LLCachedControl<T>` を helper 経由に置換 | mode 2 で BD default 値が pipeline に流れていること (LL_INFOS hook で確認) |
| **3.5** | AY-only Render* cvar dispatch list (D3) | bucket 3 AY-only 579 件 + render path grep | render path で参照される AY-only cvar 列挙 + helper BD default 追加 | mode 2 で AY-only cvar が default 動作 |
| **3.6** | BD-only windlight preset 移植 (D4) | bucket 5.1 BD-only 63 件 (skies) + 5.2/5.3 | `windlight/skies/` etc. に file-level copy | preset 一覧に BD-only が並ぶ、Cinematic mode で選択可能 |
| **3.7** | C++ Cinematic dispatch 実装 (D2 本体) | 3.2 spec 確定後 | pipeline.cpp / lldrawpool*.cpp 等の Cinematic branch 実装 | mode 2 で BD pipeline 順序 / blend / buffer 経由が BD と一致 |
| **3.8** | shader Cinematic mount (bucket 1 REDO 49 件) | Phase 1a bucket 1 REDO list | AY 側 shader を BD 中身に reset、または Cinematic-only mount | mode 2 で BD shader path が走る |
| **3.9** | UI Cinematic-only floater 整備 (bucket 4 BD-only 9 件) | bucket 4 BD-only list | floater_displaysettings / floater_quickprefs 等の BD floater 移植 | Cinematic mode で BD UI が available |

Sub-phase 3.1 / 3.2 が最初の動き出し。3.1 は mechanical で即実装可、3.2 は spec 起こしのみで実装 punt 無し (= spec 内で全 file 計画確定)。

### §5.1 sub-phase 間の依存

```
3.1 (predicate flip) ──────────┐
                                ├─→ 3.7 (Cinematic dispatch 実装)
3.2 (dispatch spec 起こし) ─────┘
3.3 (cvar helper 追加) ─────────┐
                                ├─→ 3.7 (Cinematic dispatch 実装)
3.4 (cvar helper 適用) ─────────┤
3.5 (AY-only cvar dispatch) ────┘
3.6 (preset 移植) ───────────────→ (独立)
3.8 (shader Cinematic mount) ────→ 3.7 の後段
3.9 (UI BD floater 移植) ────────→ 3.7 の後段
```

Phase 3 完了 = mode 2 が AYA self-judgment で「BD と区別がつかない」ところまで持って行けた状態。

---

## §6 Phase 2 から Phase 3 への punt 表 (= 残る未確定)

`memory/feedback_no_escape_full_bd_coverage.md` に従い、Phase 3 へ持ち越す未確定は **明示** する:

| # | 未確定項目 | 確定タイミング | 確定方法 |
|---|---|---|---|
| U1 | C++ Cinematic dispatch の per-function 計画 (各 92 file × N 関数) | Phase 3.2 | spec 起こしで mechanical に確定 |
| U2 | AY-only Render* cvar 579 件のうち render path で実際に参照されるもの | Phase 3.5 | grep + 動作 trace で確定 |
| U3 | bucket 5.2 (water) / 5.3 (days) の件数 | Phase 0 inventory 再読 | 既に inventory にある、Phase 3.6 着手時に表化 |
| U4 | windlight BD-only preset を AY 側に置く時の collision (同名 preset) | Phase 3.6 | file 名 prefix で衝突回避、Phase 3.6 着手時に確定 |

U1-U4 は **実装で判断** ではなく、Phase 3 sub-phase 着手時に inventory + grep + read で mechanical 確定 (= 推論で済まさない)。

---

## §7 章 spec / 既存 P1-P5 spec の扱い

- `ayastorm-r30-cinematic-chapter.md` / `ayastorm-r30-p1-view-mode-restart-switch.md` / `ayastorm-r30-p2-velocity-buffer-bd-trace.md` / `ayastorm-r30-p3-volumetric-lighting-bd-trace.md` / `ayastorm-r30-p4-bd-dof-chain-trace.md` / `ayastorm-r30-p5-bd-parity-spec.md` は **歴史的記録として保持**、本 BD 完全移植 spec 群 (= inventory / phase1 / phase2 / phase3.x) が新しい authority
- 既存 spec の「Cinematic = AYAstorm View r14+ stack + BD borrows」記述は D1 と矛盾 → Phase 3.x で実装が進んだ時点で「superseded by ayastorm-r30-bd-full-port-phase2-spec.md」note を各既存 spec の先頭に追記 (= Phase 5 cleanup task)

---

## §8 sub-phase commit / release 単位

- 各 sub-phase = 独立 commit (= 1 PR / 1 release tag に対応可能な粒度)
- ただし release tag (r30 Pn) は AYA self-judgment で BD 区別不能になった時点でのみ発行、sub-phase commit 単位で release はしない
- Phase 3 全部終わって AYA OK 後に **単一 release** (= r30 Cinematic) として ship
- release note は Phase 5 で `feedback_release_notes_link_only.md` に則り起草

---

## §9 Phase 2 spec の合意

本 spec の D1-D4 + sub-phase 計画 + U1-U4 punt 表を AYA が承認 (= 暗黙含む、autonomous mode 中) した時点で Phase 3.1 着手。

承認待ちのため停止する箇所はない (autonomous mode、`feedback_bd_port_autonomous_exec.md`)。

---

**End of Phase 2 Spec.**
