# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B **complete**

**作成日**: 2026-06-04
**前 commit chain** (= Phase 1.B 物理改変 + handoff、PB-1 → PB-7):
- 物理 C++ 改変 (= 14 commit、indra/llrender 配下):
  - `7fe58b7428` 副次 (a) `_PREFIX_TO_CADENCE` 拡張 (= 30 setter 識別子の cadence_tag 既知化)
  - `0ba743463c` PB-1 = LLGLSLShader 内 `ubo::UniformLocation` cache field 宣言 + `mUseUBO=false` default flag
  - `e43d93dd25` PB-2 = `mapUniforms()` 内 integer index 経路 cache 構築 (`mUniformUBOLoc`)
  - `127d25ecb6` PB-3 = `mapUniforms()` 内 LLStaticHashedString 経路 cache 構築 (`mUniformUBOLocByHash`)
  - `79966ad2f3` PB-6 = `forwardToUboUpload()` 空 stub 宣言 + 空実装 (= FWD-1 shell)
  - `772b6b985c` PB-4.1〜.3 = uniform1i / uniform1iv / uniform2iv 整数 index 経路 Vulkan path 分岐
  - `71576ff1e4` PB-4.4 = uniform2f 整数 index 経路
  - `257df5e757` PB-4.5 = uniform3f 整数 index 経路
  - `62c7a95caa` PB-4.6 = uniform4f 整数 index 経路
  - `1b54b29d3f` PB-4.7 = uniform1iv(U32 index, count, ptr) 整数 index 経路
  - `e83f76f2be` PB-4.9〜.13 = uniform1fv / 2fv / 3fv / 4fv / 4uiv 整数 index 経路 5 method batch
  - `d28b2ecf14` PB-4.14〜.17 = uniformMatrix2fv / 3fv / 3x4fv / 4fv 整数 index 経路 4 method batch
  - `1fa7555a06` PB-5.1〜.13 = LLStaticHashedString 経路 13 method batch
  - `ad4dc506f3` PB-4.8 + PB-5.14 統合 = uniform4iv 2 method (case (B) mValue 外側挿入)
  - `40f4650936` PB-7 = `mapUniforms()` 末尾 整合 check 仕込み (= `llassert` debug-only block)
- handoff doc 専用 commit (= indra/ 改変なし、docs/ のみ): 14 件 (= 各 PB sub-step complete 時 + prep 時)、本 doc が末尾 15 件目

**本 handoff doc 目的**: **Phase 1.B host-side 全終了 marker** + PB-N (= Exit Criteria 検証) 結果 record + 次 milestone (= Phase 1.A 残作業 or Phase 1.C cadence 配線) 着手 prep + 残 issue (= 上流 bug flag、AYAstorm r20 章 SSS 効き verify) を別 sub-step として独立 flag。

---

## §0 state 一行 summary

η-30 **Phase 1.B host-side 全 sub-step (= PB-1 → PB-2 → PB-3 → PB-6 → PB-4.1〜.17 → PB-5.1〜.13 → PB-4.8+PB-5.14 → PB-7 → PB-N) 完了**:

- Phase 1.B Exit Criteria literal (= spec 09 §4.2、「30 setter Vulkan path 分岐の **call site から見て transparent** = 既存 program 1 個の動作 unchanged」) を **構造的根拠で PASS** 判定
- 構造的根拠 = (1) host-side C++ 改変は全て `if (mUseUBO)` runtime gate 内、`mUseUBO=false default` 維持 (`llglslshader.h:429` 確認済)、default 下で全 bypass + (2) GLSL 改変は本 branch (= `feature/ayastorm-r41-gl-removal`、297 commit) で **0 件** (= `indra/newview/app_settings/shaders/` 配下 untouched)
- AYA 起動目視 verify (= PB-N step (4)(5)) で「全体描画に変化を感じない」literal 受領 (= Phase 1.B host-side 動作 unchanged の主観 verify 成立)
- **別 issue として AYAstorm r20 章 SSS 効き観察 issue を独立 sub-step として flag** (= 本 Phase 1.B branch 物理改変由来でないことを構造確認済、r41 milestone と独立調査要件)

---

## §1 PB-N 検証結果 record (= 本 handoff の心臓部)

### §1.1 検証手順 7 step 実施結果

| step | 主体 | 結果 | record |
|---|---|---|---|
| (1) full viewer build | Claude | ✅ PASS | 前 commit `40f4650936` の tar.xz 205 MB (= `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz`、2026-06-04 14:13) 再利用、build 再走不要 |
| (2) install | Claude | ✅ PASS | `~/ayastorm/` clean install (= `rm -rf ~/ayastorm/` 後 `./install.sh` 実行)、bin timestamp `2026-06-04 14:10:07` |
| (3) cache clear | Claude | ✅ PASS | `rm -rf ~/.ayastorm_x64/cache/shader_cache/` 削除確認 |
| (4) viewer launch | AYA | ✅ PASS | viewer 起動 → login → region entry 完了 |
| (5) AYA 起動目視 verify | AYA | ✅ PASS (host-side) | 「全体描画に変化は感じない」literal 受領 |
| (6) log fail 確認 | Claude | ✅ PASS | `~/.ayastorm_x64/logs/AYAstorm.log` 内 Phase 1.B 起因 fail (= `llassert` trip / `glGetUniformLocation` 異常 / shader link fail) **0 件** 確認 |
| (7) Phase 1.B 全終了 handoff doc 起案 + 1 commit | Claude | 🔄 本 doc | handoff doc 新規 (= 本 doc) + AYA 明示指示後 1 commit |

### §1.2 PB-N PASS 判定の構造的根拠

| # | 根拠 | 確認方法 | 結果 |
|---|---|---|---|
| (a) host-side 改変は全て `if (mUseUBO)` gate 内 | `llglslshader.cpp:1899-1971` (= PB-2/PB-3/PB-7) + PB-4.x/PB-5.x 全 method (= 各 method `if (mUseUBO)` 先頭 block) | ✅ 確認済 |
| (b) `mUseUBO=false default` 維持 (= MUSEUBO-A) | `llglslshader.h:429` `bool mUseUBO = false;` | ✅ 確認済 |
| (c) GLSL file 0 touch | `git log --oneline ayastorm-release..HEAD -- 'indra/newview/app_settings/shaders/'` = 出力 0 行 | ✅ 確認済 |
| (d) `#ifdef LL_VULKAN_GLSL` 不使用 (= GATE-B) | 全 PB sub-step commit の static verify (= LL_VULKAN_GLSL コード行 0、comment のみ) | ✅ 確認済 |
| (e) viewer log Phase 1.B 起因 fail 0 件 | `grep -i "llassert\|trip\|FATAL\|fatal\|assert.*fail\|shader.*[Ff]ail\|link.*[Ff]ail\|compile.*[Ff]ail" ~/.ayastorm_x64/logs/AYAstorm.log` で host-side 改変経路 fail 0 件 | ✅ 確認済 |

= **Phase 1.B host-side 改変は構造上 default 下で実走しない** → Exit Criteria literal「動作 unchanged」客観構造的に成立 → PB-N PASS。

---

## §2 別 issue flag (= 本 Phase 1.B 終了に独立、別 sub-step 起案候補)

### §2.1 AYAstorm r20 章 SSS 効き観察 issue

**性質**: AYA 起動目視 verify 時に「SSS が効かなくなった気がする」main subjective signal 受領。ただし以下確認で **本 Phase 1.B branch 物理改変由来ではない** と構造判定済:

- (i) AYA さん側「Phase 1.B 開始前は SSS を気にしていなかった、当時 (= r40 章「60秒計測 2 SLURL」phase) Claude が SSS について発言 → 今 PB-N verify 時に意識して観察」 → **Phase 1.B 前後比較根拠なし**
- (ii) Phase 1.B host-side 改変は構造的に SSS 影響不可 (= 上記 §1.2)
- (iii) 本 branch GLSL 改変 0 件 = AYAstorm r20 章 SSS marker (= `aya_sss_skin_flag`、`materialF.glsl` 経路) 物理改変なし

**残課題**: AYAstorm r20 章 SSS 機能が現状効いているかは別途実機 verify 要件。**別 sub-step 起案**:

- doc 名候補: `handoff-substep-...-aya-sss-r20-verify.md` (= r41 milestone と独立)
- 内容案:
  - (a) `aya_sss_skin_flag` 関連 debug settings の現値確認 (= 過去 session で Persist=1 cvar が変わって残っている可能性)
  - (b) `materialF.glsl` 内 AYAstorm SSS 経路の現コード trace (= 本 branch 0 touch ゆえ `ayastorm-release` HEAD 同等)
  - (c) 実機 ON/OFF live A/B (= `feedback_visual_decisions_need_live_ab`)
  - (d) r20 spec doc (= `docs/specs/ayastorm-r20-avatar-skin-sss.md`) との照合
- AYA 判断要件: r41 milestone と並行 or 後続 or 別 release sub-phase か

### §2.2 上流 bug flag 残件 (= 前 commit `ad4dc506f3` で flag 済)

`indra/llrender/llglslshader.cpp:2514` の `uniform4iv` method 内で `glUniform1iv` 呼出 = LL/Phoenix-Firestorm 上流既存 bug 疑い。本 Phase 1.B sub-step は Vulkan path 分岐挿入のみ、OpenGL path 既存 `glUniform1iv` 行は as-is 維持 = 別 PR 案件。

- 別 sub-step doc 名候補: `handoff-substep-...-upstream-uniform4iv-glUniform1iv-bug.md`
- scope: LL upstream への PR or AYAstorm fork 内 fix sub-step として独立、r41 milestone と独立

---

## §3 残 milestone strict 線形 (= Phase 1.A / 1.C 進捗反映)

### §3.1 spec 09 §4.2 literal による Phase 1 構成

```
Phase 1.A: 既存 85 UBO blueprint codegen + 既存 program 1 個 include + bind 不変動作確認
Phase 1.B: 30 setter Vulkan path 分岐 transparent = 動作 unchanged
Phase 1.C: test UBO 1 個 (= Phase 2 第 1 UBO の shell) per-cadence update + descriptor bind 通電
```

### §3.2 現 status

| Phase | status | 根拠 |
|---|---|---|
| Phase 1.A | 部分完了 (詳細別途 `handoff-substep-...-phase1-a-complete.md` 参照) | 既存 handoff doc 一覧で `phase1-a-PA-8-set3-complete.md` + `phase1-a-complete.md` 存在確認、ただし本 PB-N 検証時点で残作業 (= codegen pipeline 実 run + 生成 header テスト program 経路通電 verify) 有無は別途 phase1-a-complete.md 読込で確認要件 |
| **Phase 1.B** | **✅ 全完了 (本 handoff doc が marker)** | §1.1 / §1.2 構造的根拠 + AYA 起動目視 verify PASS |
| Phase 1.C | 未着手 | spec 09 §4.2 Phase 1.C Exit (= test UBO shell の 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer 成功) 未確認 |

### §3.3 次 milestone AYA 判断要件

```
✅ Phase 1.B host-side 完了
  → ⏳ 次 milestone: Phase 1.A 残作業 (= codegen pipeline 実 run) vs Phase 1.C 着手 vs §2 別 issue 着手
```

**AYA 判断要件 (= 次 session 起動時)**:
- 候補 (X): Phase 1.A 残作業 (= codegen pipeline 実 run + 生成 header テスト program 経路通電) 着手
- 候補 (Y): Phase 1.C 着手 (= test UBO 1 個 shell + per-cadence update + descriptor bind 通電)
- 候補 (Z): §2.1 AYAstorm r20 章 SSS 効き verify 別 sub-step 先行
- 候補 (W): §2.2 上流 uniform4iv 内 glUniform1iv bug fix 別 PR 先行
- 並行可能性: (X) ⊥ (Z) ⊥ (W) (= 互いに干渉なし、並行起案可能)、(Y) は (X) 完了後推奨 (= Phase 1.A 生成 header が Phase 1.C test UBO shell の input)

---

## §4 self-verify 観点 (= 本 handoff commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) Phase 1.B 全 commit list 網羅 | 上 §0 commit chain table | ✅ 14 物理 + 14 handoff + 本 doc |
| (2) PB-N 7 step 全 record | §1.1 table | ✅ 7 step 全 PASS or 🔄 (本 doc 起案中) |
| (3) 構造的根拠 5 観点 | §1.2 table | ✅ 全 ✅ 確認済 |
| (4) AYA SSS issue 別 sub-step flag | §2.1 | ✅ doc 名候補 + 内容案 + AYA 判断要件 record |
| (5) 上流 bug flag 残件 | §2.2 | ✅ doc 名候補 + scope record |
| (6) 次 milestone AYA 判断要件 | §3.3 | ✅ (X)(Y)(Z)(W) + 並行可能性 record |
| (7) commit 内容 | handoff doc 新規 1 件のみ、indra/ + scripts/ 改変 0 | ✅ |
| (8) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | ✅ |
| (9) `feedback_self_bug_no_defer_option` 遵守 | §2.1 / §2.2 は「先送り」signal でなく **別 sub-step 独立起案 flag** として扱う | ✅ |

---

## §5 引き継ぎ memory

特に重要 (= 既存 memory から、次 session 着手時参照):

- `project_ayastorm_r41_vulkan_migration.md` (= r41 milestone active marker)
- `project_ayastorm_r41_design_principles.md` (= 2 大設計原則: 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate.md` (= GATE-B 確定: `LL_VULKAN_GLSL` C++ 未使用、`mUseUBO` runtime flag 単独 gate)
- `feedback_ubo_migration_one_at_a_time` (= 次 Phase 1.A / 1.C / SSS verify も 1 sub-step ずつ、cold launch 検証挟む)
- `feedback_self_verify_before_handoff` (= 次 sub-step 着手前に AYA 提示前 self-trace)
- `feedback_self_bug_no_defer_option` (= 「先送り/disable」を提案として並べない、fix or 別 sub-step 独立起案のみ)
- `feedback_doubt_self_first` (= AYA 提示情報を疑わず、自分の改変を疑う、本 PB-N で構造的に確認)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件)
- `feedback_no_auto_commit` (= 本 doc commit は AYA 明示指示後)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B host-side 全終了 (= PB-1 → PB-7 → PB-N 全完了、本 handoff doc が marker、物理 14 commit + handoff doc 14 + 本 doc = 15 件、AYA 起動目視 verify PASS = 全体描画変化感じない literal、構造的根拠 5 観点 PASS、SSS 効き観察 issue は別 sub-step として独立 flag、上流 uniform4iv 内 glUniform1iv bug も別 sub-step 独立 flag)。本 session = AYA 判断 (X) Phase 1.A 残作業 or (Y) Phase 1.C 着手 or (Z) AYAstorm r20 章 SSS 効き verify 別 sub-step or (W) 上流 bug 別 PR のいずれか、必読 3 件 = (1) 本 handoff doc 全文 + (2) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-complete.md` (= Phase 1.A 残作業判定 if (X)) or (Y)/(Z)/(W) 着手 doc + (3) `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` §4.2 Phase 1 Exit Criteria literal、Phase 1.B host-side は本 doc で close、本 doc 以降の物理 indra/ 改変は次 milestone の sub-step 内に閉じる。」**
