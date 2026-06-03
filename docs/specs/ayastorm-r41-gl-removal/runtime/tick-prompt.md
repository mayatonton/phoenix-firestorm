# AYAstorm r41 Phase 1.B 自動進行 tick prompt

**source of truth** = 本 file。trigger 内 short prompt は本 file の Read + 実行を指示するのみ。

## §0 役割宣言

あなたは AYAstorm r41 Phase 1.B (= GLSL → Vulkan UBO migration の host C++ redirect 層)
自動進行 tick として cron fire された Claude session です。

- **repo**: `https://github.com/mayatonton/phoenix-firestorm` (= 既に cloud 側で clone 済想定)
- **branch**: `feature/ayastorm-r41-gl-removal`
- **1 tick scope**: 3 sub-step (= 3 method の Vulkan path 分岐追加 + 2 commit + push) batch
- **元 rule** (= memory `feedback_ubo_migration_one_at_a_time` = 1 method 1 sub-step) は
  自動進行期間中 **batch 3 に緩和**:
  - 理由 = cloud session 経路ゆえ cold launch 検証物理根拠なく commit 単位として ~100
    line edit / commit は revert 容易性維持
  - 制約 = 各 method ごとに個別 comment header + spec literal 引用維持 (= 段階退避路
    確保の趣旨は維持)
- **commit policy**: Co-Authored-By: Claude 行を **絶対に付けない** (= memory
  `feedback_no_claude_coauthor`、AYA さん全 commit で rewrite 済の absolute rule)

## §1 pre-requisite minimal read

(= memory `feedback_handoff_minimal_pre_req_read` 順守、全件読み禁止)

### 必読 3 件のみ

1. `git log --oneline -20` で前 tick 状態確認
2. `docs/specs/ayastorm-r41-gl-removal/handoff/` 内最新 (= timestamp が最大) handoff doc
   を Read (= Glob `handoff-substep-*-PB-*-complete.md` で最新検出、`ls -t | head -1`
   でも可)
3. `docs/specs/ayastorm-r41-gl-removal/design/06a-bare-uniform-host-redirect.md` の §4.2
   (= integer index 経路 code shape) + §5.2 (= LLStaticHashedString 経路 code shape) を
   pinpoint Read

### 追加 pinpoint Read

- 次着手 method 周辺 ±50 line のみ Read (= 全 file 読み禁止、grep + offset/limit 使用)
- `indra/llrender/llglslshader.h:443-457` (= PB-6 で追加した `forwardToUboUpload()` 宣言
  確認)

## §2 state 検出

最新 handoff doc §6「次 sub-step」literal から着手対象 method 特定。

### 残 sub-step 一覧

```
PB-4.1〜.17 = 17 method integer index 経路 Vulkan path 分岐追加
  PB-4.1  uniform1i(U32 index, GLint x)
  PB-4.2  uniform1iv(U32 index, U32 count, const GLint* v)
  PB-4.3  uniform1f(U32 index, GLfloat x)
  PB-4.4  uniform2f(U32 index, GLfloat x, GLfloat y)
  PB-4.5  uniform2fv(U32 index, U32 count, const GLfloat* v)
  PB-4.6  uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z)
  PB-4.7  uniform3fv(U32 index, U32 count, const GLfloat* v)
  PB-4.8  uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
  PB-4.9  uniform4fv(U32 index, U32 count, const GLfloat* v)
  PB-4.10 uniform1fv(U32 index, U32 count, const GLfloat* v)
  PB-4.11 uniform2iv(U32 index, U32 count, const GLint* v)
  PB-4.12 uniform3iv(U32 index, U32 count, const GLint* v)
  PB-4.13 uniform4iv(U32 index, U32 count, const GLint* v)
  PB-4.14 uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
  PB-4.15 uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
  PB-4.16 uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
  PB-4.17 uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)

PB-5.1〜.13 = 13 method LLStaticHashedString 経路 Vulkan path 分岐追加
  PB-5.1  uniform1i(const LLStaticHashedString& uniform, GLint i)
  PB-5.2  uniform1f(const LLStaticHashedString& uniform, GLfloat v)
  PB-5.3  uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y)
  PB-5.4  uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
  PB-5.5  uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z)
  PB-5.6  uniform4f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
  PB-5.7  uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
  PB-5.8  uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
  PB-5.9  uniform2iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
  PB-5.10 uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
  PB-5.11 uniformMatrix3fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
  PB-5.12 uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
  PB-5.13 uniform1iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)

PB-7 = mapUniforms() 末尾 debug build llassert (= 整合 check 仕込み)
PB-N = ローカル動作確認 (= 自動進行 scope 外、AYA 起床後実行)
```

**注**: 上記 method list は spec から推定したもの。実 method 名 / 引数は
`indra/llrender/llglslshader.h` の現状実装に従う (= 不一致時は最新 handoff doc §6 を優先)。

## §3 stop 条件判定 (= 着手前 pre-check)

### (a) AYA 判断要 literal 検出

- 最新 handoff doc + 06a spec + 04 spec の改変差分で「AYA 判断要」「AYA 確認要」
  「proposal」「要相談」「decide」literal を Grep
- ヒットあれば即 §7 へ (= sentinel = `aya-decision-pending.md`)

### (d) 残 sub-step = PB-N のみ

- handoff doc §6「次 sub-step」が PB-N or 全完了 literal
- 該当なら §7 完了 stop へ (= sentinel = `COMPLETE-phase1b.md`)

### (c) handoff mismatch 検出

- 前 tick が想定した「次着手 method」と git log の最新 commit message 内 PB 番号が
  連続性失う場合 (= 想定 PB-X.Y → 現状 PB-X.Z で Z != Y)
- 該当なら §7 へ (= sentinel = `ALERT-tick-error.md`、type=handoff-mismatch)

## §4 物理 edit (= 3 method batch)

着手 method 3 件を順次:

### 共通手順

1. `indra/llrender/llglslshader.cpp` 内の対象 method 該当 line を Grep で特定
2. 既存 glUniformXxx call 直前 (= mUseUBO=true path) に下記 pattern 挿入
3. GATE-B 順守 = `#ifdef LL_VULKAN_GLSL` 不使用、runtime `if (mUseUBO)` gate のみ
4. MUSEUBO-A 順守 = mUseUBO=false default 既存 OpenGL 挙動 100% 維持 (= 本 block 走らず)

### PB-4.x (= integer index 経路) code shape

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-4.{N}:
// integer index 経路 Vulkan path 分岐追加。spec 06a §4.2 literal 準拠。
// GATE-B = #ifdef LL_VULKAN_GLSL 不使用、mUseUBO runtime flag 単独 gate。
// MUSEUBO-A = mUseUBO=false default で本 block 走らず既存 OpenGL 挙動 100% 維持。
if (mUseUBO) {
    llassert(index < mUniformUBOLoc.size());
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];
    if (loc.cadence_tag == 0xFFFFFFFFu) return;  // miss sentinel
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;  // sampler 非該当
    forwardToUboUpload(loc, &x, sizeof(GLfloat));  // type / size は method ごと
    return;
}
glUniformXxx(mUniform[index], ...);  // 既存 OpenGL path 保持
```

### PB-5.x (= LLStaticHashedString 経路) code shape

```cpp
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-5.{N}:
// LLStaticHashedString 経路 Vulkan path 分岐追加。spec 06a §5.2 literal 準拠。
// GATE-B / MUSEUBO-A 整合。
if (mUseUBO) {
    const U64 hash = static_cast<U64>(uniform.Hash());
    auto it = mUniformUBOLocByHash.find(hash);
    if (it == mUniformUBOLocByHash.end()) return;  // miss = silent skip
    const ubo::UniformLocation& loc = it->second;
    if (loc.cadence_tag == 0xFFFFFFFFu) return;
    if (loc.cadence_tag == 5 /* CADENCE_SAMPLER */) return;
    forwardToUboUpload(loc, &v, sizeof(GLfloat));  // type / size は method ごと
    return;
}
// 既存 LLStaticHashedString path (= getUniformLocation 経由) 保持
```

### type / size 早見表

| method 系 | data ptr | size |
|---|---|---|
| `uniform1i` | `&x` | `sizeof(GLint)` |
| `uniform1iv` | `v` | `count * sizeof(GLint)` |
| `uniform1f` | `&x` | `sizeof(GLfloat)` |
| `uniform2f` | `(GLfloat[]){x, y}` (一時 array) | `2 * sizeof(GLfloat)` |
| `uniform2fv` | `v` | `count * 2 * sizeof(GLfloat)` |
| `uniform3f` / `uniform3fv` | 同上 vec3 | `count * 3 * sizeof(GLfloat)` |
| `uniform4f` / `uniform4fv` | 同上 vec4 | `count * 4 * sizeof(GLfloat)` |
| `uniformMatrix2fv` | `v` | `count * 4 * sizeof(GLfloat)` |
| `uniformMatrix3fv` | `v` | `count * 9 * sizeof(GLfloat)` |
| `uniformMatrix4fv` | `v` | `count * 16 * sizeof(GLfloat)` |
| `uniformMatrix3x4fv` | `v` | `count * 12 * sizeof(GLfloat)` |

**uniform2f / uniform3f / uniform4f の data ptr 注意**: 直接アドレス取れる variable が
ない scalar 引数 (`x, y, z, w`) は一時 array 作成 (`const GLfloat tmp[] = {x, y, z, w};`
等) してから `&tmp[0]` を渡す。

## §5 static verify (= grep based)

cloud env で cmake build 可能性不確実ゆえ静的 verify:

1. `git diff` の改変箇所に `LL_VULKAN_GLSL` 文字列が追加されてない事 grep 確認
2. `mUseUBO = true` / `mUseUBO=true` literal が新規追加されてない事確認 (= MUSEUBO-A 順守)
3. commit message draft に `Co-Authored-By` 行が含まれない事確認
4. PB-4.x path = `mUniformUBOLoc[index]` 参照確認
5. PB-5.x path = `mUniformUBOLocByHash.find(hash)` 参照確認
6. `forwardToUboUpload(` call site 数 += 3 確認 (= 本 tick で 3 method 追加ゆえ +3)
7. 各 method の `glUniformXxx(mUniform[index], ...)` 既存 line が削除されてない事確認

**失敗時** → §7 へ (= sentinel = `ALERT-tick-error.md`、type=static-verify-fail)

## §6 commit + push (= 2 commit 分割)

### commit 1: 物理改変

```bash
git add indra/llrender/llglslshader.cpp
git commit -m "$(cat <<'EOF'
r41: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-{X}.{Y}〜PB-{X}.{Y+2} (3 method batch) = ...
EOF
)"
```

**注**: Co-Authored-By 行を **絶対に含めない**。

commit message には:
- sub-step marker (= `PB-{X}.{Y}〜PB-{X}.{Y+2}` 3 method)
- 各 method の追加 line 数 + 挿入位置
- GATE-B 順守 + MUSEUBO-A 順守 + spec literal 引用
- batch 3 緩和明記 (= 「自動進行期間中 batch 3、cloud session 経路 cold launch 検証
  物理根拠なく commit 単位として ~100 line edit / commit で revert 容易性維持」)
- 既存 PB-3 / PB-6 commit message template 踏襲

### commit 2: handoff doc 起案

```bash
# 新規 handoff doc 作成
NEW_DOC="docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-{X}.{Y}-{Y+1}-{Y+2}-complete.md"

# structure:
#   §0 state 一行 summary
#   §1 pre-requisite 最小読み 3 件
#   §2 残 sub-step strict 線形構成 (= 本 tick 完了反映)
#   §3 本 tick 新規確定 設計判断 (= 該当時のみ、無ければ「設計判断 0 件」記載)
#   §4 self-verify 9 観点 PASS
#   §5 引き継ぎ済 memory (= 追加 0 件 / 既存継承)
#   §6 次 sub-step (= 次 tick 着手対象 1 line)

git add ${NEW_DOC}
git commit -m "$(cat <<'EOF'
r41: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-{X}.{Y}〜PB-{X}.{Y+2} complete handoff doc 起案 = ...
EOF
)"
```

### push

```bash
git push origin feature/ayastorm-r41-gl-removal
```

**push fail (= ff-only 失敗 / network error 等) 時** → §7 へ (= sentinel =
`ALERT-tick-error.md`、type=push-fail)

## §7 stop sentinel + timer 自己 disable

stop 条件該当時:

### (a) sentinel file 作成

```bash
SENTINEL="docs/specs/ayastorm-r41-gl-removal/runtime/{SENTINEL_NAME}.md"
# {SENTINEL_NAME} は条件に応じて:
#   aya-decision-pending / ALERT-tick-error / COMPLETE-phase1b
```

内容 = 検出 sub-step / 検出 literal 引用 / 推奨 AYA wake action / 関連 commit hash

### (b) timer 自己 disable

```
RemoteTrigger {action: "update", trigger_id: "{TRIGGER_ID_PLACEHOLDER}", body: {enabled: false}}
```

**trigger_id** = trigger create 時に発行された ID (= trigger 短 prompt 内に hardcoded、
本 file は placeholder 表記)。

### (c) sentinel commit + push

```bash
git add ${SENTINEL}
git commit -m "r41: Phase 1.B 自動進行 sentinel ({SENTINEL_NAME}) = ..."
git push origin feature/ayastorm-r41-gl-removal
```

## §8 9 観点 self-verify

(= memory `feedback_self_verify_before_handoff`、commit 着手前 / push 直前で実施)

| # | 観点 |
|---|---|
| 1 | llglslshader.cpp +X line 物理確認 (= 3 method 連続改変、`git diff --stat`) |
| 2 | 各 method 挿入位置 = glUniformXxx call 直前 (= 既存 OpenGL path より前に Vulkan path 分岐) |
| 3 | GATE-B 順守 = `#ifdef LL_VULKAN_GLSL` 不使用 (= `git diff | grep LL_VULKAN_GLSL` 0 件) |
| 4 | MUSEUBO-A 順守 = mUseUBO=false default 維持 (= `llglslshader.h:429` 初期化に diff なし) |
| 5 | static verify §5 全 7 観点 PASS |
| 6 | 残 sub-step strict 線形維持 (= handoff §2 list 反映) |
| 7 | batch 3 緩和明記 + revert 容易性根拠記述 (= commit message + handoff §0) |
| 8 | git working tree = llglslshader.cpp + handoff doc + (該当時) sentinel のみ、他 file 改変なし |
| 9 | Co-Authored-By: Claude 行不在 (= `git log -1 --format=%B | grep Co-Authored-By` 0 件) |

**全 9 観点 PASS で push**、1 件でも fail なら §7 へ (= sentinel = `ALERT-tick-error.md`、
type=self-verify-fail)

## §9 完了時の挙動

(d) 残 sub-step = PB-N のみ 検出時:

1. sentinel = `COMPLETE-phase1b.md` 作成
   - 内容 = 「PB-4 全 17 method + PB-5 全 13 method + PB-7 完了、残 PB-N (= ローカル
     動作確認) のみ、AYA wake action = build + install + cache clear + viewer launch +
     bind 不変動作確認 = Phase 1.A complete §2.3 経路非到達 3 観点 verify を Phase 1.B
     後 state で再走」
2. timer 自己 disable (= §7 (b))
3. sentinel commit + push (= §7 (c))

## §10 error handling

### git fetch / checkout / pull 失敗

- network error → 1 度 retry
- 2 度目 fail → `ALERT-tick-error.md` (type=git-network-fail) + 自己 disable
  - 注: 自己 disable も network 要、disable 失敗時は AYA wake で claude.ai 側 trigger
    UI から手動 disable

### handoff doc が見つからない

- Glob 結果 0 件 → `ALERT-tick-error.md` (type=handoff-not-found) + 自己 disable

### 不明な method 名

- 残 sub-step list に無い method 名が handoff §6 に出現 → `aya-decision-pending.md`
  (= AYA に method 名確認依頼) + 自己 disable

### cloud session time limit 到達

- 1 tick が 60 分以内に完了しない場合、session 切れる可能性あり
- 対策 = 各 method 終了ごとに進捗 (= `git add` + WIP commit) を retained
  - ただし WIP commit を push すると次 tick が中途状態を見ることになるため push せず
  - 推奨 = 3 method 全完了 + verify PASS まで commit 単位を分けず、最後に 2 commit 分割
  - **本仕様では time limit hit 時の partial work は捨てる** (= 次 tick が同 method から
    やり直し、git working tree clean を git reset で確保)

## §11 まとめ (= tick 1 行 summary)

`fetch → state 検出 → stop pre-check → 3 method edit → static verify → 2 commit + push →
9 観点 verify → 次 tick 待ち / stop 時 sentinel + 自己 disable`
