# AYAstorm r41 Phase 1.B 自動進行 runtime

## 概要

AYAstorm r41 Phase 1.B (= GLSL → Vulkan UBO migration の host C++ redirect 層) の
残 sub-step (= PB-4.1〜.17 / PB-5.1〜.13 / PB-7) を schedule skill (Anthropic cloud remote
agent) で自動進行させる仕組み。

- **trigger**: claude.ai 側の cron-fired remote agent (= `RemoteTrigger` API で管理)
- **interval**: 1 時間 (= API 最小)、5-field cron `0 * * * *`
- **scope per tick**: 3 sub-step (= 3 method の Vulkan path 分岐追加 + commit + push)
- **prompt source of truth**: `tick-prompt.md` (= 本 dir 内、git 管理)
- **state**: fresh session 毎回、state は git log + 最新 handoff doc から検出

## ファイル構成

| file | 役割 |
|---|---|
| `README.md` | 本 file = 仕組み全体説明 + AYA wake protocol |
| `tick-prompt.md` | tick が読む詳細 instruction (= source of truth、trigger prompt 内から参照) |
| `aya-decision-pending.md` | (runtime 生成) AYA 判断要時 sentinel |
| `ALERT-tick-error.md` | (runtime 生成) tick の static verify / push fail 等 |
| `ALERT-build-fail.md` | (runtime 生成、将来 cloud build 可能化時) |
| `COMPLETE-phase1b.md` | (runtime 生成) PB-4〜PB-7 全完了、PB-N (= ローカル動作確認) のみ残 |

## trigger 仕様

- **name**: `AYAstorm-r41-Phase1B-auto-progression`
- **environment**: `Default` (id: `env_01PePY4cHwrqnxjq7hpqvL31`、anthropic_cloud kind)
- **model**: `claude-sonnet-4-6`
- **git source**: `https://github.com/mayatonton/phoenix-firestorm`
- **branch**: `feature/ayastorm-r41-gl-removal` (= tick 開始時 checkout)
- **allowed_tools**: `Bash`, `Read`, `Write`, `Edit`, `Glob`, `Grep`, `RemoteTrigger`
- **trigger_id**: `trig_01Aadosy3szE85V8PWM4H2Q5` (= 2026-06-04 create 時発行)
- **status URL**: https://claude.ai/code/scheduled/trig_01Aadosy3szE85V8PWM4H2Q5
- **next_run_at**: enable 後 `0 * * * *` 直近 UTC 時刻

## tick 動作 flow

```
fire (1h ごと)
  ↓
git fetch + checkout feature branch + pull
  ↓
Read tick-prompt.md
  ↓
最新 handoff doc Read → 次着手 sub-step 検出
  ↓
stop 条件 pre-check (= AYA 判断要 literal / 全完了)
  ├─ 該当 → sentinel 書込 + timer 自己 disable → stop
  └─ 非該当 → 進行
  ↓
3 method batch (= 物理 edit × 3)
  ↓
static verify (= GATE-B / MUSEUBO-A / Co-Authored-By 不在 / 参照 grep)
  ├─ fail → ALERT sentinel + 自己 disable → stop
  └─ pass → 進行
  ↓
commit (物理改変) + commit (handoff doc) + push
  ↓
9 観点 self-verify
  ├─ fail → ALERT sentinel + 自己 disable → stop
  └─ pass → 完了 (次 tick 待ち)
```

## stop 条件 (= 4 sentinel)

| sentinel | 検出条件 | AYA wake action |
|---|---|---|
| `aya-decision-pending.md` | spec / handoff に「AYA 判断要」「AYA 確認要」「proposal」literal 検出 | 内容 Read → 判断 → 手動 commit + push → timer 再 enable |
| `ALERT-tick-error.md` | static verify fail / push fail / handoff mismatch 等 | 内容 Read → 原因確認 → 必要なら revert → fix push → timer 再 enable |
| `ALERT-build-fail.md` | (将来用、cloud build 可能化時) | log Read → fix → timer 再 enable |
| `COMPLETE-phase1b.md` | 残 sub-step = PB-N (= ローカル動作確認のみ) | PB-N 実行 (= build + install + cache clear + viewer launch + bind 不変動作確認) → Phase 1.B 完了 marker commit |

## AYA wake protocol

起床後の手順:

```bash
# 1. ローカル repo を最新化
cd ~/work_firestorm/phoenix-firestorm
git fetch origin
git checkout feature/ayastorm-r41-gl-removal
git pull --ff-only

# 2. runtime/ の sentinel 確認
ls docs/specs/ayastorm-r41-gl-removal/runtime/

# 3. 該当 sentinel の中身 Read + AYA wake action 実行
#    (= Claude session を立ち上げて "AYA wake protocol を実行" と伝える)

# 4. timer status 確認 (= claude.ai 側 https://claude.ai/code/scheduled/{TRIGGER_ID})
#    enabled=false なら sentinel 該当、enabled=true なら正常進行中
```

**timer 再 enable**:

```
claude.ai 側で AYAstorm-r41-Phase1B-auto-progression trigger を enable
  https://claude.ai/code/scheduled/trig_01Aadosy3szE85V8PWM4H2Q5
または Claude Code session で:
  RemoteTrigger {action: "update", trigger_id: "trig_01Aadosy3szE85V8PWM4H2Q5", body: {enabled: true}}
```

## AYA active 時の overlap 回避

schedule skill remote agent は **cloud 側で完全 isolated** ゆえ、AYA さんがローカル
Claude session で並行作業しても **直接的 conflict なし** (= ローカル機 / file 操作 / 設定
書込みが衝突しない)。

ただし **git push の race condition** はありえる:
- AYA さんがローカルで commit + push した直後に tick が fire → tick が `git pull --ff-only`
  で安全に取込 → tick が新 3 method 進行 → push (= ff-only 維持)
- 逆順 (= tick push 後に AYA push) も同様

**衝突 risk**: AYA さんが手元で同じ method の Vulkan path 分岐を編集して push、tick も
同 method を編集して push、で `git pull --ff-only` 失敗 → tick が `ALERT-tick-error.md`
書込み + 自己 disable → AYA wake 時に解消。

**運用推奨**: AYA さん起床中 (= 手元で作業する時間帯) は trigger を disable しておくと
race 完全回避。寝る前に enable、起きたら disable + sentinel 確認の運用。

## 関連 memory

- `project_ayastorm_r41_vulkan_migration` = r41 章全体
- `project_ayastorm_r41_design_principles` = 2 大設計原則
- `project_r41_phase1b_vulkan_host_gate` = GATE-B (= mUseUBO runtime flag only)
- `feedback_ubo_migration_one_at_a_time` = 1 method 1 sub-step (= 本自動進行で batch 3
  に緩和、cloud session 経路 cold launch 検証不可ゆえ commit 単位として ~100 line edit /
  commit で revert 容易性維持)
- `feedback_no_claude_coauthor` = 全 commit message で Co-Authored-By: Claude 行不在
- `feedback_handoff_minimal_pre_req_read` = handoff pre-req は最小 3 件読み
- `feedback_self_verify_before_handoff` = 9 観点 self-verify を AYA 確認依頼前に実施

## 作成日

2026-06-04
