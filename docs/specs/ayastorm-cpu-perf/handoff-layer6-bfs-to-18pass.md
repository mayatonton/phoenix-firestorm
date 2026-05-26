# Handoff — Layer 6 全周回 (Group M) 配線完了 → 18 周目 計測 待ち

**作成**: 2026-05-26 / **branch**: `feature/ayastorm-r40-cpu-perf` (旧 `feature/ayastorm-r31-cpu-perf`、2026-05-26 採番きりなおしで rename)

## 1. 現在位置 (one-liner)

> Layer 6 全周回 配線 (updateCull 3-split + renderDeferredLighting 4-split = +7 zone) を pipeline.cpp に commit せず実装済。spec 追記済。**build を background 開始済、install/cache clear 未実施**。AYA は build 完了後の install と CSV 取得を待っている。

## 2. 何が起きていたか (経緯 60 秒読み)

- Phase 1.1 (main thread 地図完成) 残作業 = **Layer 6 全周回 (doOcclusion 兄弟 hot zone の同深度 drill)**
- Agent survey で 2 候補確定: `updateCull` (1.04 ms、未配線) + `renderDeferredLighting` (1.80 ms、top-level only)
- AYA 承認 → pipeline.cpp に 7 zone 追加 → 03-perf-log-infra.md §6.11 Group M を追記 → self-verify PASS
- build を background で開始 (`autobuild build --no-configure`)
- context 残量懸念で AYA 指示によりこの handoff 作成

## 3. 次 session が即やること

### 3.1 build 完了確認

**background bash task ID**: `bzbmv6e48` (本 session 起動分)

出力ログ: `/tmp/claude-1000/-home-ishikawa-work-firestorm-phoenix-firestorm/491aa33c-55fe-48d9-9db2-6eb9785dd899/tasks/bzbmv6e48.output`

```
Monitor tool で bzbmv6e48 を確認 → 終了済なら 3.2 へ、走行中なら通知待ち
失敗時は再 build (下記コマンド)
task ID が無効ならゼロから再 build
```

### 3.2 再 build (build task が無効/失敗時のみ)

```bash
cd ~/work_firestorm/phoenix-firestorm
source .venv/bin/activate
export AUTOBUILD_VARIABLES_FILE=$HOME/work_firestorm/fs-build-variables/variables
autobuild build -A 64 -c ReleaseFS_open --no-configure
```
(configure 不要 — pipeline.cpp 1 ファイル編集のみ、CMakeLists.txt 不変)

### 3.3 install + cache clear (build 成功後)

```bash
cd ~/work_firestorm/phoenix-firestorm/build-linux-x86_64/newview/packaged
rm -rf ~/ayastorm/
rm -rf ~/.local/share/applications/ayastorm-viewer.desktop
./install.sh
rm -rf ~/.ayastorm_x64/cache/
```

### 3.4 AYA に viewer 起動 + 計測依頼

AYA への指示テキスト (そのまま copy-paste):

> install + cache clear 完了しました。次のお願いです:
> 1. viewer 起動 → debug settings で `AYAPerfLogEnabled=1` に設定
> 2. 8/9/10/11 周目と同じ SLurl・昼時間帯で active session を 60-90 秒録音 (※従来 doc に「夜時間帯」とあったのは誤記、16 周目以降 昼で固定)
> 3. 録音後 viewer 終了 → `~/.ayastorm_x64/logs/AYAstorm-perf.csv` を `AYAstorm-perf-18pass-baseline.csv` に rename
> 4. rename 完了したら教えてください、04 spec に Layer 6 結果として整理します

### 3.5 CSV 受領後の作業

`~/.ayastorm_x64/logs/AYAstorm-perf-18pass-baseline.csv` を読み、04-observed-hot-path-map.md に **§4.2.h Group M (Layer 6 全周回) 結果** として追記。

整理形式 (テンプレ):

```markdown
### §4.2.h 18 周目 Layer 6 全周回 結果 — updateCull / renderDeferredLighting 内訳

**計測条件**: 2026-05-26 active session (...)、AYAPerfLogEnabled=1。Group M 配線後 (updateCull 3 zone + renderDeferredLighting 4 zone)。CSV: `~/.ayastorm_x64/logs/AYAstorm-perf-18pass-baseline.csv`。時刻条件: (AYA 確認) / 同 SLurl (8-11 周目と一致)。

#### M-1 詳細: updateCull body 3 分解
| zone | total (us) | count | per-call (us) | 占有率 |
...

#### M-2 詳細: renderDeferredLighting body 4 分解
| zone | total (us) | count | per-call (us) | per-frame (ms) | 占有率 |
...

#### Layer 6 全周回 総括
- updateCull 主犯確定 / 否
- renderDeferredLighting 主犯 sub-zone (postDeferred 仮説か否か) 確定
- implicit setup gap (renderDeferredLighting parent − Σ 4 sub) の残量
- Phase 1.1 完成判定 → Phase 1.2 (Core 振り分け設計案) 着手可否
```

### 3.6 Phase 1.1 完成判定

- 各 sub-zone parent vs Σ children gap が < 10% → **Phase 1.1 完成 → Phase 1.2 へ移行**
- gap 大 (例: renderDeferredLighting で 0.8 ms 残る) → **Layer 7 drill 必要**、追加 1 周回 (新規 zone 配線 → re-build → CSV)
- 02 §D 順序の再構成 (Phase 1.2 で実施予定) は別 doc (`05-core-assignment-plan.md` 新規) で扱う

## 4. 触らない・覚えておくこと

### 4.1 絶対条件 (memory 既読)

- **r40 章で短縮系打ち手を提案しない** (`feedback_r40_no_micro_tuning.md`)
  - hot path 短縮 / cvar 値弄り / query 削減 / pool 化 全部禁止
  - 仕事は「main thread から剥がせる候補発見と剥がし実装」のみ
  - 13-17 周目で 5 周空転した実例あり、再発防止
- **BFS 原則** (`feedback_perf_map_bfs_drill.md`): Layer N 全部回ってから Layer N+1
- **commit は AYA 明示指示まで禁止** (`feedback_no_auto_commit.md`)

### 4.2 配線済 zone 一覧 (18 周目時点で測定対象)

既存 (Group A-K + L) + 今回追加 (Group M, +7):
- `updateCull_waterClip` / `_regionPartition` / `_skyRender` (pipeline.cpp:3027/3060/3094)
- `renderDeferredLighting_lightmap` / `_atmospherics` / `_localLights` / `_postDeferred` (pipeline.cpp:11190/11296/11370/11652)

詳細は 03-perf-log-infra.md §6.11 + §9。

### 4.3 既知の制約

- `updateCull` sub-zone は function-internal のため、parent (`updateCull` at llviewerdisplay.cpp:903) より count 多い (HUD render + shadow path から fire)。per-call 比較時は count 補正必要 (03 spec §6.11 注記済)。
- `renderDeferredLighting` の implicit setup gap は明示 zone 切らず parent − Σ から逆算 (40 行程度の sequential 配置、更に細分する意味薄)。

## 5. ファイル一覧 (本周回で変更、uncommitted)

```
indra/newview/pipeline.cpp                                          # Group M 7 zone 追加
docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md                   # §6.10 Group L + §6.11 Group M 追記、§9 file list 更新
docs/specs/ayastorm-cpu-perf/handoff-layer6-bfs-to-18pass.md        # 本ファイル (新規)
```

`git status` / `git diff` で確認可能。

## 6. 関連 spec / commit

| 参照先 | 用途 |
|---|---|
| `docs/specs/ayastorm-cpu-perf/00-overview.md` | r40 章 thesis、Phase 構造、13-17 周目脱線記録 |
| `docs/specs/ayastorm-cpu-perf/02-offload-feasibility.md` | §A1-A14 deep-dive 済 (Phase 1.1 part 1) |
| `docs/specs/ayastorm-cpu-perf/03-perf-log-infra.md` | 計測 infra + §6.11 Group M wiring 仕様 |
| `docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md` | Layer 1-5 全結果、本 handoff の §3.5 で §4.2.h 追記予定 |
| commit `171d6b90d1` | Layer 6 doOcclusion 3-split (Group L) |
| commit `99c771bbb6` | 打ち手 C/F revert + 13-17 周目 handoff doc 追加 |

## 7. Phase 1.2 への移行条件

Phase 1.1 完成 = (a) 02-offload-feasibility §A1-A14 deep-dive ✓ + (b) Layer 6 全周回 (今回) ✓ + (c) 18 周目 CSV 結果整理 (本 handoff §3.5)

完成判定後、Phase 1.2 (`docs/specs/ayastorm-cpu-perf/05-core-assignment-plan.md` 新規) で A1-A14 を worker A/B/C に集約する設計案を書く。これが Phase 2 (剥がし実装) 着手判断のゲート。

---

**handoff 作成者** Claude opus-4-7 (session `491aa33c-55fe-48d9-9db2-6eb9785dd899`)
