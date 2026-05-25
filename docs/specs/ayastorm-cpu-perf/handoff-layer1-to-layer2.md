# Handoff: Layer 1 → Layer 2 (CPU perf 章)

**作成日**: 2026-05-26
**前セッション ID**: 4a4558d2-45ea-4404-b907-ecc33fe3305e (本書作成時の継続元)
**目的**: 次セッション Claude が Layer 2 = 7 周目 BFS B drill を即着手できるよう、現在位置と次の手を 1 ファイルに集約。

---

## 1. 現在位置 one-liner

> **Layer 1 (doFrame_total 直下の主要 zone を全部同じ粒度で内訳化) 完了。次は Layer 2 = 1 段深い全周 = 「renderGeomDeferred の 6.8 ms gap / sun_3 5.77 ms / gViewerWindow->draw 18.6 ms」の中身を全部同時に drill する。**

詳細地図: `docs/specs/ayastorm-cpu-perf/04-observed-hot-path-map.md` §3 全体 + §4.2 gap 表 + §6.2 Layer 2 候補表。

---

## 2. BFS 方針 (memory 必読)

`feedback_perf_map_bfs_drill.md` を **最初に読むこと**。要旨:
- **A = 今の層を全部回る** (横展開)
- **B = そこから 1 段深く全部回る** (深掘り)
- A → B → 更に深く ... を反復、全枝が同じ深さに揃いながら徐々に深くなる
- **「B (深掘り) を後でやり忘れない」厳禁**
- **層境界 = 安全な引き継ぎポイント** (本書は Layer 1 完成直後 = 境界、Layer 2 計測完了で次の境界)

---

## 3. Layer 2 = 7 周目 zone 追加 plan

### 3.1 追加 zone (6 個)

| zone 名 | 親 (Layer 1) | 配線位置のヒント | 狙い |
|---|---|---|---|
| `renderGeomDeferred_doOcclusion` | renderGeomDeferred | `pipeline.cpp` の `LLPipeline::renderGeomDeferred(LLCamera& camera)` 内、`doOcclusion(camera)` call wrap | 6.8 ms gap の主犯候補 |
| `renderGeomDeferred_setupHWLights` | 同上 | 同関数内 `setupHWLights()` call wrap | gap 副犯候補 |
| `renderGeomDeferred_postLoop` | 同上 | pool loop 抜けた後〜関数末まで wrap | gap 残り吸収 |
| `renderShadow_sun_setup` | renderShadow_sun_3 | `pipeline.cpp:13169` 周辺、sun cascade for loop 内 `renderShadow()` 呼ぶ前の frustum compute / FBO bind / culling 部分 | sun_3 突出原因 (setup vs draw 分離) |
| `uiRender_ui2d_vwDraw_floaterView` | uiRender_ui2d_viewerWindowDraw | **未読の `llviewerwindow.cpp`** 内 `LLViewerWindow::draw()` 関数の `gFloaterView->draw()` 周辺 | gViewerWindow->draw 18.6 ms の内訳開始 |
| `uiRender_ui2d_vwDraw_rootView` | 同上 | 同関数内 `mRootView->draw()` wrap | UI widget tree 本体 |

### 3.2 配線時の確認手順

1. **必ず grep で line 番号再確認** (本書の line 番号は古くなる可能性):
   ```
   Grep: pattern="doOcclusion|setupHWLights" path="indra/newview/pipeline.cpp"
   Grep: pattern="renderGeomDeferred" path="indra/newview/pipeline.cpp" -n
   Grep: pattern="LLViewerWindow::draw|gFloaterView->draw|mRootView->draw" path="indra/newview/llviewerwindow.cpp" -n
   ```
2. **llviewerwindow.cpp は本セッション未読 = 巨大予想**。最初は **Explore agent (medium-thorough)** で構造把握を依頼するのが効率的 (memory `feedback_use_agents_proactively.md`)。
3. **AYAPERF_ZONE は string literal 専用**。動的 name が必要なら `LLAyastormPerfZone _aya_xxx_zone(const char*)` を直接 instantiate (lifetime は `static const char*` 必須)。
4. **新規 .cpp/.h 追加なし** → configure 不要、--no-configure のみ。

### 3.3 03 spec 更新義務

Layer 2 配線完了したら **`03-perf-log-infra.md` §6.6 Group H** として catalog 追記:
- §6.5 (Group G) と同じフォーマット
- §9 file 一覧の `pipeline.cpp` 行に 6 zone 追加追記
- 必要なら `llviewerwindow.cpp` を §9 に新規行追加

---

## 4. ビルド / 起動 / 計測フロー

memory `project_build_procedure.md` 完全準拠。要点だけ:

```bash
# 1. configure (新規ファイル無しならスキップ可)
autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio -DLL_TESTS:BOOL=FALSE \
  -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE \
  --package --chan AYAstorm-release

# 2. build
autobuild build -A 64 -c ReleaseFS_open --no-configure

# 3. install (memory `project_build_procedure.md` フロー)
# rm + cp + cache clear は memory 参照

# 4. 計測前: 6 周目 CSV を退避
mv ~/.ayastorm_x64/logs/AYAstorm-perf.csv \
   ~/.ayastorm_x64/logs/AYAstorm-perf-6pass-baseline.csv

# 5. AYA さんに起動依頼 → 同条件 active session 録音 → 終了報告
# 6. CSV 解析:
awk -F, 'NR>1 {c[$3]++; s[$3]+=$4} END {
  for (z in c) printf "%-50s %8d %12d %10.2f\n", z, c[z], s[z], s[z]/c[z]
}' ~/.ayastorm_x64/logs/AYAstorm-perf.csv | sort -k3 -n -r | head -80
```

**ビルド / 起動 / 録音は AYA さん側**。Claude は configure / build / install / cache clear まで連続実行 OK (memory `feedback_build.md`)。

---

## 5. CSV 退避ファイル一覧 (時系列比較用)

| 周 | ファイル | 状態 |
|---|---|---|
| 2 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-2pass-baseline.csv` | 70 MB (B1/B2 修正前、参考用) |
| 3 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-3pass-baseline.csv` | 9.5 MB (B1/B2 修正済) |
| 4 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-4pass-baseline.csv` | uiRender scope bug 含む |
| 5 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-5pass-baseline.csv` | scope 修正済 |
| **6 周目 (current, Layer 1 完成)** | `~/.ayastorm_x64/logs/AYAstorm-perf.csv` | **本 handoff の根拠データ** |
| 7 周目 (これから取る) | 同上 path (上書き) → 計測前に 6 周目を `-6pass-baseline.csv` に退避すること | |

---

## 6. 必読 memory (Layer 2 着手前にロード推奨)

| memory | なぜ |
|---|---|
| `feedback_perf_map_bfs_drill.md` | A/B 用語定義、層境界引き継ぎ原則 (本作業の支柱) |
| `feedback_use_agents_proactively.md` | llviewerwindow.cpp 未読 / 構造把握は Explore agent 推奨 |
| `feedback_no_auto_commit.md` | Layer 2 完成後も AYA 明示指示まで commit しない |
| `feedback_restore_debug_settings.md` | 05 spec 着手前に `AYAPerfLogEnabled=0` 戻し案内 |
| `feedback_build.md` | configure→build→install→cache clear 連続実行権限 |
| `feedback_one_step_at_a_time.md` | 計測フローは AYA さんに 1 step ずつ |
| `feedback_log_reading.md` | CSV 解析は Claude が直接 awk |
| `project_build_procedure.md` | 完全フロー (autobuild flag 含む) |

---

## 7. 既知 risk / 注意

1. **観測者効果**: 6 周目時点で FPS 17.6。Layer 2 で +6 zone なら誤差範囲予想だが、もし 16 FPS 切ったら instrument overhead 警告を 05 spec 検討前に表面化させる。
2. **llviewerwindow.cpp 編集**: pipeline.cpp 同様巨大。複数箇所編集する前に Read offset 指定で必要箇所だけ読む、または Explore agent で構造把握 → 編集箇所だけ pinpoint Read。
3. **renderGeomDeferred 6.8 ms gap の真犯人**: doOcclusion / setupHWLights / postLoop で全部吸えない可能性あり。その場合は **更に pool loop 内側 (cur_type 判定 + setShaders + pool->endRenderPass 等)** に追加 wrap が必要 = Layer 3 候補に繰り上げ。
4. **uncommitted diff の積み上がり**: 本作業開始時点で perf log infra 関連で 11 file 触っている。Layer 2 で更に +2-3 file。AYA 明示指示で commit 段に入る時、まとめ方を相談する (memory `feedback_release_branch_workflow.md`)。

---

## 8. 引き継ぎ session 開始時の AYA さん発話例

AYA さんが新セッションを開く時、最初に以下のいずれかを言えば Claude は即着手できる:

```
CPU perf 章の Layer 2 続けてください。引き継ぎ書は docs/specs/ayastorm-cpu-perf/handoff-layer1-to-layer2.md です。
```

または最小:
```
docs/specs/ayastorm-cpu-perf/handoff-layer1-to-layer2.md 読んで Layer 2 進めて。
```

これで Claude は本書 → 必読 memory → 04 spec § 4.2 (gap 表) → §6.2 (Layer 2 候補) の順で読み、7 周目 zone 配線 plan を提示する流れに入れる。

---

## 9. Open task list (引き継ぎ時に TaskCreate しなおすこと)

前セッション最終時点の未完 task は以下:
- [ ] Layer 2 (7 周目) zone 6 個配線
- [ ] 7 周目 ビルド + install + cache clear
- [ ] AYA に 7 周目 active session 録音依頼
- [ ] 7 周目 CSV 解析
- [ ] 04 spec を Layer 2 結果で更新 (or 04-layer2.md 新規)
- [ ] 03 spec §6.6 Group H 追記
- [ ] Layer 2 gap <10% に縮んだら 05 spec 着手検討 (打ち手議論)
- [ ] 05 着手前に `AYAPerfLogEnabled=0` 戻し案内
