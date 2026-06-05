# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-7 complete + PB-N prep 統合

**作成日**: 2026-06-04
**前 session 物理 commit**:
- `ad4dc506f3` (= PB-4.8 + PB-5.14 統合 sub-step、uniform4iv 2 method case (B) mValue 外側挿入、+38 line + 06b spec record)
- `40f4650936` (= PB-7、mapUniforms() 末尾 整合 check 仕込み、+27 line + 06a §4.4.1 spec record)

**本 handoff 目的**: Phase 1.B 全 host-side 物理 sub-step (PB-2 → PB-3 → PB-6 → PB-4.1〜.17 → PB-5.1〜.13 → PB-4.8+PB-5.14 → PB-7) **完了 marker** + 残 PB-N (= Phase 1.B Exit Criteria 検証、AYA 同伴) **着手 prep** を 1 doc 統合 (= 前例 `d7e80dda42` の 2 doc 1 commit pattern を 1 doc 1 commit に圧縮)。

---

## §0 state 一行 summary

η-30 **Phase 1.B host-side 全 sub-step 完了 state** (= 物理 code 改変は本 Phase 完了、残 PB-N = AYA 同伴起動目視 verify):

- **本 doc 目的** = PB-7 complete marker + PB-N (= Phase 1.B Exit Criteria 検証) 着手 prep
- **PB-N 性質** = **物理 code 改変なし** (= 検証 phase)、AYA さん起動目視「変化していないと思う」literal 確認 + Claude side log/build verify 並走、合格後 Phase 1.B 全終了 handoff doc 起案 + 本 sub-step 1 commit (= AYA 判断 record + verify 結果 commit)
- **想定 file 改変**: cpp/h 0 件、spec doc 0〜2 件 (= 09 §4.2 / 本 handoff successor doc)、handoff doc 1 件 (= Phase 1.B 全終了 handoff)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。本 PB-N 着手時は **3 件のみ** 読む。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | PB-N 検証手順 + AYA 同伴起動要件 + verify 観点 + commit 要件 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` §4.2 (line 204-224) | Phase 1.B Exit Criteria literal | call site から見て transparent = 既存 program 1 個動作 unchanged の判定軸 |
| 3 | `project_build_procedure.md` (memory) | full build → install → cache clear flow | PB-N 実行手順の literal 再確認 |

### §1.2 pinpoint Read 用 reference (= 必要時のみ)

| file | 参照箇所 |
|---|---|
| `indra/llrender/llglslshader.cpp:1899-1980` | PB-2/PB-3/PB-7 が集中する `mapUniforms()` 末尾 block + PB-6 stub |
| `indra/llrender/llglslshader.h:427-457` | mUseUBO=false default + forwardToUboUpload 宣言 + mUniformUBOLoc / mUniformUBOLocByHash 定義 |
| `project_r41_phase1b_vulkan_host_gate.md` (memory) | C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate の確認 |

---

## §2 PB-7 complete state (= 前 commit `40f4650936` 物理確定済)

### §2.1 物理改変 summary

| file | 改変位置 | 内容 | size |
|---|---|---|---|
| `indra/llrender/llglslshader.cpp` | `mapUniforms()` 末尾 (= PB-3 block line 1944 直後 / `unbind()` 直前) | PB-7 `if (mUseUBO) { llassert(...) + loop assert(...) }` block + 13 line comment header (= `(3)` AYA 判断 (a) 注釈記述) | +27 line |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §4.4 直後 | §4.4.1 PB-7 実装確定 section 新設 (= AYA 判断 2026-06-04 (a) 案採択根拠 3 件 + 実装位置 line 1944 直後 明記) | +14 line |

### §2.2 build verify 段階記録

- full viewer build EXIT 0、tar.xz 205 MB (= Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz)
- 初回 build パス (= 前 session 確立 packaged/ clean rebuild 救済 protocol 不発動)
- static verify §3.4 (2) 全 6 観点 PASS (= LL_VULKAN_GLSL コード行 0、mUseUBO=true 新規 0、Co-Authored-By 不在、llassert 実 2 件、glUniform 削除 0、.h diff 0)

### §2.3 残 sub-step strict 線形反映

```
✅ PB-2 (cache 構築 integer index)
✅ PB-3 (cache 構築 LLStaticHashedString)
✅ PB-6 (forwardToUboUpload 空 stub)
✅ PB-4.1〜.17 (整数 index 経路 17 method、PB-4.8 deferred 含む)
✅ PB-5.1〜.13 (LLStaticHashedString 経路 13 method、PB-5.14 deferred 含む)
✅ PB-4.8 + PB-5.14 統合 sub-step (uniform4iv 2 method、(U4) AYA 判断 (B) mValue 外側挿入)
✅ PB-7 (mapUniforms() 末尾 整合 check 仕込み、AYA 判断 (a) コメント注釈のみ)
  → ⏳ PB-N (= Phase 1.B Exit Criteria 検証、AYA 同伴起動目視 verify)
```

---

## §3 PB-N prep (= Phase 1.B Exit Criteria 検証 着手準備、本 handoff の心臓部)

### §3.1 PB-N 内容 (= spec 09 §4.2 + handoff PB-6 §2 PB-N row literal)

**spec 09 §4.2 Phase 1.B Exit Criteria literal**:
> 30 setter Vulkan path 分岐の **call site から見て transparent** = 既存 program 1 個の動作 unchanged

**handoff PB-6 §2 PB-N row literal**:
> Phase 1.B Exit Criteria 検証 = full build + install + cache clear + viewer launch + bind 不変動作確認 + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案

= **mUseUBO=false default 下で本 milestone 物理改変 (= PB-2/PB-3/PB-6/PB-4.1〜.17/PB-5.1〜.13/PB-4.8+PB-5.14/PB-7) が viewer 動作に一切影響しないことを実機検証**。

### §3.2 検証手順 (= 7 step)

| step | 主体 | 内容 |
|---|---|---|
| (1) full viewer build | Claude | `cmake --build build-linux-x86_64 -j$(nproc)` = EXIT 0 + tar.xz 生成 (= 前 PB-7 commit 時点で完了済、再走不要) |
| (2) install | Claude | `project_build_procedure.md` flow に従い `~/ayastorm/` へ install (= 前例 patterns 参照) |
| (3) cache clear | Claude | `~/.ayastorm_x64/cache/shader_cache/` 削除 (= `project_ayastorm_shader_cache_path.md` 参照) |
| (4) viewer launch | **AYA さん** | viewer 起動 → login → region entry → 通常使用感の確認 |
| (5) AYA 起動目視 verify | **AYA さん** | 「変化していないと思う」literal 確認 (= 既存 program 動作 unchanged の主観判定、Phase 1.B Exit Criteria literal) |
| (6) log fail 確認 | Claude | `~/.ayastorm_x64/logs/AYAstorm.log` grep で Phase 1.B 起因 fail (= llassert trip / glGetUniformLocation 異常 / shader link fail) 0 件確認 |
| (7) Phase 1.B 全終了 handoff doc 起案 + 1 commit | Claude | 本 sub-step 結果 record + 残 Phase (= 1.A codegen / 1.C cadence 配線) 線形整理 + 1 commit (= 物理改変なし、handoff doc のみ) |

### §3.3 「AYA と一緒に実行」明記理由

- (4) (5) は AYA さん起動目視 = Claude 単独で実行不可
- viewer login + region entry の動作確認は user-level subjective verification ゆえ「変化なし」literal 判定は AYA さん主観に依存
- Claude は (1)(2)(3)(6)(7) を担当、(4)(5) は AYA 起動待ちで stop

### §3.4 検証完了後の handoff doc 起案要件

PB-N 完了時に **Phase 1.B 全終了 handoff doc** を新規起案 (= Phase 1.A / 1.C の次 milestone への引継):

- doc 名: `handoff-substep-...-eta-30-phase1-b-COMPLETE.md` (= Phase 1.B 全完了 marker)
- 内容: Phase 1.B 全 commit hash list + 全 sub-step 完了状態 + Phase 1.A / 1.C 進捗 + 次 milestone (= Phase 1.A codegen 着手 or Phase 1.C cadence 配線着手) 線形整理 + AYA 判断要件 (= 次 Phase の着手順序 / 並行 / 優先)
- 物理改変: handoff doc 新規 1 件 + 06a / 06b / 09 spec record annotation 必要に応じ追記
- commit: 1 commit (= AYA 確認後)

### §3.5 build verify 戦略

PB-N は物理 code 改変なし → build 再走不要 (= 前 PB-7 commit `40f4650936` の build artifact をそのまま使用)。ただし install / cache clear / launch は新規実行が必要。

初回 launch fail 時の救済:
- llassert trip = mUseUBO=false default 下で本 block 走らない想定だが、もし trip するなら sCurBoundShaderPtr / mUniform.size() / mUniformUBOLoc.size() の初期化 timing 問題が考えられる
- log で trip 時 stack trace を確認 → root cause 特定 → fix sub-step として切出し (= PB-N 内では fix しない、別 sub-step として起案)

### §3.6 上流 bug flag 残件 (= 本 PB-N scope 外、別 PR 案件)

`indra/llrender/llglslshader.cpp:2514` uniform4iv method 内で `glUniform1iv` 呼出 = LL/Phoenix-Firestorm 上流既存 bug 疑い (= 前 sub-step PB-4.8+PB-5.14 統合 commit `ad4dc506f3` の commit message 内に flag 済)。Phase 1.B Exit Criteria 検証 (= PB-N) 完了後、Phase 1.A codegen / 1.C cadence 配線着手前 or 並行に、別 PR の別 sub-step として扱う候補。

---

## §4 残 sub-step strict 線形 (= 本 handoff 反映)

```
✅ PB-7 (= 本 handoff 完了 marker、commit 40f4650936)
  → ⏳ PB-N (= Phase 1.B Exit Criteria 検証、AYA 同伴起動目視 verify)
    → Phase 1.A 着手 (= codegen pipeline) or Phase 1.C 着手 (= cadence 配線) AYA 判断
```

Phase 1.B 完了後の次 milestone 順序 (= Phase 1.A / 1.C):
- Phase 1.A (= codegen) と Phase 1.C (= cadence) は spec 09 上は順序未確定
- handoff PB-6 §2 では PB-N 後を「Phase 1 全 Exit 後 Phase 2 へ」と書いており、Phase 1.A / 1.C の着手順序は別途 AYA 判断要件

---

## §5 self-verify 観点 (= PB-N 着手時 commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) install 完了 | `~/ayastorm/` の `app_settings/shaders/...` 配下 timestamp 更新確認 | OK |
| (2) cache clear 完了 | `~/.ayastorm_x64/cache/shader_cache/` 空 or 古い entry 削除済 | OK |
| (3) viewer launch PASS | AYA さん起動目視「変化していないと思う」literal 受領 | OK |
| (4) log fail 0 件 | `grep -i "llassert\|trip\|fatal" ~/.ayastorm_x64/logs/AYAstorm.log` 0 件 | OK |
| (5) Phase 1.B 全終了 handoff doc 起案 | 本 handoff の次 doc 物理作成 | OK |
| (6) commit 内容 | handoff doc 新規 + 必要なら spec annotation のみ、cpp/h 改変 0 | OK |
| (7) Co-Authored-By 不在 | commit message 行頭 `Co-Authored-By:` 0 件 | OK |
| (8) 残 sub-step 線形維持 | §4 table | OK |

---

## §6 引き継ぎ memory (= PB-7 complete handoff 固有追加なし)

特に重要 (= 既存 memory から):

- `feedback_self_verify_before_handoff` — PB-N で AYA 起動目視前に Claude 「3 経路 (= integer index / hashed / mapUniforms 末尾整合 check) 非到達 verify」を mUseUBO=false default 下で再走
- `feedback_one_step_at_a_time` — PB-N は 7 step、各 step 完了確認後に次 step、並列実行禁止
- `project_build_procedure.md` — install flow の literal 参照
- `project_ayastorm_shader_cache_path.md` — cache clear 対象 dir literal
- `project_r41_phase1b_vulkan_host_gate.md` — mUseUBO=false default 維持確認 (= MUSEUBO-A)
- `feedback_no_auto_commit` — PB-N 7 step 完了後の commit は AYA 明示指示後
- `feedback_no_claude_coauthor` — Co-Authored-By 行不在

---

## §7 次 session 着手 1 line

**「前 session で Phase 1.B host-side 全 sub-step (= PB-2 → PB-3 → PB-6 → PB-4.1〜.17 → PB-5.1〜.13 → PB-4.8+PB-5.14 → PB-7) 完了 (前 2 commit = `ad4dc506f3` PB-4.8+PB-5.14 統合 + `40f4650936` PB-7、tar.xz 205 MB)。本 session = Phase 1.B Exit Criteria 検証 (= PB-N) 着手 = 物理 code 改変なし検証 phase = §3.2 7 step (= (1) build 既存活用 + (2) install + (3) cache clear + (4) AYA viewer launch + (5) AYA 起動目視「変化していないと思う」literal 確認 + (6) Claude side log fail 0 件確認 + (7) Phase 1.B 全終了 handoff doc 起案 + 1 commit)、必読 3 件 = (1) 本 handoff doc 全文 + (2) `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` §4.2 Phase 1.B Exit Criteria literal + (3) `project_build_procedure.md` memory、Claude 担当 = (1)(2)(3)(6)(7)、AYA 担当 = (4)(5)、初回 launch fail 時 = llassert trip 等は別 fix sub-step 切出し PB-N 内では fix しない、PB-N 完了後 = Phase 1.B 全終了 handoff doc 起案 → Phase 1.A (codegen) or 1.C (cadence) AYA 判断、上流 bug flag (= llglslshader.cpp:2514 uniform4iv method 内 glUniform1iv 呼出) は別 PR 候補で本 PB-N scope 外。」**
