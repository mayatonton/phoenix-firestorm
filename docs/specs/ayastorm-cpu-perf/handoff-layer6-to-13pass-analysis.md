## Handoff: Layer 6 配線完了 → 13th pass 検証 (CPU perf 章)

**作成日**: 2026-05-26
**目的**: Layer 6 (`LLPipeline::doOcclusion` 3 分割 zone 配線) + 12 周目計測 + 打ち手 B (Hero probe 2 重 occlusion query 除去) commit 完了状態を引き継ぐ。**次 phase は 13 周目計測 (打ち手 B 効果検証) + spec 章更新 + commit aggregation 戦略**。

---

## 1. 現在位置 one-liner

> **12 周目計測で `LLPipeline::doOcclusion` 内訳が確定: `doOcclusion_reflectionProbes` = 4128 us/frame、占有率 94.7% (`spatialGroups` 192 us / `voCache` 38 us は誤差)。事前想定の「spatial group polling 主犯」を完全 falsify。`reflectionProbes` block を git blame で読むと upstream LL bug が判明: 2024-06-21 b01c6ef0674 で (A) block に `mHeroProbeManager.doOcclusion()` を追加した際、2023-08-21 ef057c7b268 で導入された (B) Hero only block を消し忘れ → Hero probe occlusion query が毎 frame 2 回発行されていた。1.5 年間 unfixed。打ち手 B として (B) block を削除 (commit 114d3dd532)、13 周目で `reflectionProbes` ~4128 → ~2000-2500 us への低下を検証する段階。mirror probe を含むシーンでの視覚 regression A/B は AYA 普段 use では遭遇しにくいため deferred (memory 化済)。**

詳細は `04-observed-hot-path-map.md` §4.2.g (打ち手 A 検証) + 次 session で §4.2.h (Layer 6 + 打ち手 B) を新規追記、`03-perf-log-infra.md` §6.9 + 次 session で §6.10 Group L (Layer 6 catalog) 新規追記。

---

## 2. BFS 方針 / 必読 memory

`feedback_perf_map_bfs_drill.md` を最初に。本 phase で新たに load-bearing になった memory:

- **`project_uchite_b_mirror_followup.md`** (新規): 打ち手 B 適用後の mirror probe 視覚 regression 確認は、r31 release 前 or AYA 普段 use での自然な遭遇で実施する deferred task。次 session 開始時に release date チェック → 未確認なら release note に明記。
- **`feedback_release_with_user_feedback.md`**: 視覚 A/B 用シーンが揃わない時に exhaustive solo acceptance を組まずに release + user feedback で補う原則。打ち手 B の mirror 確認方針の根拠。
- **`feedback_feature_value_in_main_usecase.md`**: 元から「Mirror Off」機能を持つほど重い機能で、AYAstorm 主流 use case で mirror probe シーンが流行っていない事実を踏まえて検証粒度を判断。

---

## 3. Layer 6 確定 finding (12 周目、本 session で fix)

### 3.1 `LLPipeline::doOcclusion` 3 分割 (12 周目、CSV: `AYAstorm-perf-12pass-baseline.csv`)

active frame 3001、frame >= 30688 filter。

| zone | per-active-frame (us) | doOcclusion 内訳 |
|---|---|---|
| **doOcclusion_reflectionProbes** | **4128** | **94.7% ← 主犯確定** |
| doOcclusion_spatialGroups | 192 | 4.4% |
| doOcclusion_voCache | 38 | 0.9% |
| Σ ≈ doOcclusion | 4358 | (Layer 2 観測 6170 us との差 ~1800 us は parent 直下の `LLGL*` ctor/dtor + gGL state change 推定、誤差として打ち切り) |

事前想定 (Layer 5 §4.3 の handoff doc): 「spatial group iteration + queryAvailable polling が主犯」→ **完全 falsify**。**ほぼ全てが reflection probe occlusion**。

### 3.2 reflection probe block の構造 (静的解析 + git blame)

`pipeline.cpp:3173-3192` の (A) block:

```cpp
if (sReflectionProbesEnabled && sUseOcclusion > 1 && !sShadowRender && !gCubeSnapshot) {
    gGL.setColorMask(false, false);
    LLGLDepthTest depth(GL_TRUE, GL_FALSE);
    LLGLDisable cull(GL_CULL_FACE);
    gOcclusionCubeProgram.bind();
    if (mCubeVB.isNull()) { mCubeVB = ll_create_cube_vb(...); }
    mCubeVB->setBuffer();

    mReflectionMapManager.doOcclusion();
    mHeroProbeManager.doOcclusion();   // ← b01c6ef0674 (2024-06-21) で追加
    gOcclusionCubeProgram.unbind();
    gGL.setColorMask(true, true);
}
```

打ち手 B 適用前は、これと**ほぼ同条件 + 同 state setup** で Hero only を呼ぶ (B) block が直後に存在していた:

```cpp
// (旧 B block、打ち手 B で削除済)
if (sReflectionProbesEnabled && sUseOcclusion > 1 && !sShadowRender && !gCubeSnapshot) {
    // 同じ state setup (setColorMask / LLGLDepthTest / LLGLDisable cull / bind program / mCubeVB setup)
    mHeroProbeManager.doOcclusion();   // ← Hero のみ 2 回目
    // 同じ teardown
}
```

### 3.3 git blame による upstream bug 確定

```
ef057c7b268 (Geenz, 2023-08-21) "Readd occlusion culling for hero probes"
  → (B) Hero only block を導入

b01c6ef0674 (Dave Parks, 2024-06-21) "#1814 and #1517 Fix mirror update rate and occlusion culling"
  → (A) block に mHeroProbeManager.doOcclusion() を追加
  → (B) block の存在に気付かず残置 ← bug
```

`sl-upstream/main` を直接読んで確認: **2026-05-26 時点でも upstream に同 bug が残存** (1.5 年間 unfixed)。AYAstorm 側で先行 fix。

### 3.4 idempotent 性検証

`LLHeroProbeManager::doOcclusion()` は内部で各 `LLReflectionMap` の `doOcclusion(eye)` を呼び、その先で GPU occlusion query (`glBeginQuery` / `glEndQuery`) を発行する。**`glBeginQuery` は idempotent ではない** (前 query を強制終了し新 query 開始、CPU/GPU 両方で work 発生)。

→ 2 回呼ぶと per-Hero-probe で query が二重発行され、`doOcclusion_reflectionProbes` の CPU/GPU 両方が ~2 倍化。打ち手 B = (B) block 削除で CPU work ~50% 削減見込み。

### 3.5 打ち手 B 適用済の実コード変更 (commit 114d3dd532)

`indra/newview/pipeline.cpp:3194-3206`:

```cpp
// <FS:AYAstorm> CPU perf 章 r31 P0 打ち手 B: 旧 (B) block (Hero only) 削除。
// upstream secondlife/viewer 2024-06-21 b01c6ef0674 (Dave Parks
// "#1814 and #1517 Fix mirror update rate and occlusion culling") で
// 上記 (A) block に mHeroProbeManager.doOcclusion() を追加した際、
// 既存の (B) Hero only block (2023-08-21 ef057c7b268 Geenz
// "Readd occlusion culling for hero probes") を消し忘れた結果、
// 同条件で Hero probe occlusion query が毎 frame 2 回発行されていた。
// LLHeroProbeManager::doOcclusion() は内部で probe->doOcclusion(eye) を
// 呼び GPU occlusion query を発行するため idempotent ではなく、2 回呼ぶと
// GPU/CPU work が両方 2 倍化する。12 周目計測で doOcclusion_reflectionProbes
// が doOcclusion 内 95% (4.13 ms/frame) と支配的だったため削除。
// </FS:AYAstorm>
} // </FS:AYAstorm> doOcclusion_reflectionProbes scope end
```

---

## 4. 次 phase = 13 周目計測 + spec 章更新 + commit aggregation

### 4.1 13 周目計測 (次 session 最初)

**前提**: branch `feature/ayastorm-r31-cpu-perf` の最新 (commit 114d3dd532) を build + install + cache clear した状態で AYA さんに active session 録音依頼。同 SLurl、時刻条件不問。

**検証目標**:
| zone | 12 周目 (打ち手 B 前) | 13 周目想定 (打ち手 B 後) | 判定 |
|---|---|---|---|
| doOcclusion_reflectionProbes | 4128 us | **~2000-2500 us を期待** (~50% 減) | -40% 以上で打ち手 B 成功 |
| doOcclusion_spatialGroups | 192 us | ~192 us (変化なし期待) | sanity check |
| doOcclusion_voCache | 38 us | ~38 us (変化なし期待) | sanity check |
| doFrame_total | 33.8 ms | -2 ms 程度? | 直接効果 |

**もし期待ほど下がらない場合の追加調査**: GPU 側 query が事前に 2 重 issued されたまま fence 待ちが mainthread block している可能性 → `LLHeroProbeManager::doOcclusion()` 内部 (probe ループ × occlusion query state machine) の更に 1 段深い zone 配線が必要 (Layer 7 候補)。

### 4.2 spec 章更新 (次 session 中盤)

- **`04-observed-hot-path-map.md` §4.2.h 新規**: Layer 6 + 打ち手 B 結果を 表 + 図 で記述 (12 周目分解表 + 13 周目検証表 + git blame 系統図)
- **`03-perf-log-infra.md` §6.10 Group L 新規**: Layer 6 配線 catalog (3 zone) + `LLPipeline::doOcclusion` 構造図
- **`05-mitigations.md` 新規**: 打ち手 A (`LLUI::setLineWidth` 削除) + 打ち手 B (Hero probe 2 重削除) の両方を出荷判断としてまとめる文書を新規作成
  - 打ち手 A: 効果 -3 ms、UI 回帰なし AYA 確認済 (focus border / world map / 選択枠 / manip 軸)
  - 打ち手 B: 効果 -2 ms 想定 (13 周目で確定)、mirror 視覚 regression は deferred 確認 (memory: `project_uchite_b_mirror_followup.md`)

### 4.3 commit aggregation 戦略 (次 session 終盤、AYA 相談必須)

現状 branch `feature/ayastorm-r31-cpu-perf` の **5 commit構成 (base からの追加 7 commit)**:

```
114d3dd532  fix(perf): doOcclusion 内 Hero probe 2 重 occlusion query を除去 (打ち手 B)
171d6b90d1  perf(zone): Layer 6 LLPipeline::doOcclusion 3 分割 zone 配線
2b66da6ec7  docs(perf): CPU perf 章 spec 9 本を追加
42e3c8d773  perf(zone+fix): vwDraw zone 配線 + 打ち手 A (LLUI::setLineWidth 削除)
47395b8962  perf(zone): Layer 2-3 pipeline render zone 配線
c7ccb128a9  perf(zone): Layer 2 spatial / 非同期系 zone 配線
ef1defa450  perf(zone): Layer 1 広域 zone + AYAPerfLog init/shutdown 配線
128d79d0c0  infra(perf): AYAstorm CPU perf 章 zone 計測 CSV writer + cvar 追加
```

**論点** (AYA 相談):
- そのまま 8 commit で merge するか、`fix(perf)` 系 (打ち手 A + B) のみ release branch に cherry-pick して残りは feature branch 保留にするか
- 13 周目検証完了後、§4.2.h と §5 (打ち手まとめ) を追記する commit を 9 個目として足すか、`docs(perf)` 既存 commit (2b66da6ec7) に amend するか
- AYAPerfLogEnabled=0 戻し案内を release note に入れるか、05 spec に閉じるか

### 4.4 リリース反映方針 (案、要 AYA 確認)

r31 本体 release に含める打ち手:
- 打ち手 A (`LLUI::setLineWidth(1.f)` 削除): UI 回帰確認済、効果確定、独立 commit で release branch cherry-pick が安全
- 打ち手 B (Hero probe 2 重削除): mirror 視覚未確認、release note に「異常があれば issue 報告」明記する条件で含める案 (memory `project_uchite_b_mirror_followup.md` §How to apply)
- zone 配線 (Layer 1-6) + spec docs: 計測 infra なので release 自体には不要だが、`AYAPerfLogEnabled` default=0 で persist しないなら同梱でも user 影響ゼロ → 同梱推奨

---

## 5. ビルド / 計測フロー (前 handoff §5 と同じ、差分のみ)

```bash
# 1. 12 周目 CSV は退避済
#    ~/.ayastorm_x64/logs/AYAstorm-perf-12pass-baseline.csv (50MB, 1.16M rows)

# 2. branch feature/ayastorm-r31-cpu-perf の最新 (114d3dd532) を確認
git -C /home/ishikawa/work_firestorm/phoenix-firestorm log --oneline -1

# 3. build (Claude 連続実行 OK、autonomous 期間中に build 全権 — feedback_bd_port_autonomous_exec.md は r30 期間外なので明示確認は取る)
# 4. install + cache clear
# 5. AYA に 13 周目 active session 録音依頼 (同 SLurl、時刻不問、AYAPerfLogEnabled=1 のまま)
# 6. CSV を AYAstorm-perf-13pass-baseline.csv にリネーム
# 7. CSV 解析 (active session filter 必須、frame range は session 開始 frame >= ~N から)
```

---

## 6. CSV 退避ファイル一覧 (Layer 6 完了時点)

| 周 | ファイル | 用途 |
|---|---|---|
| 2-6 周目 | `~/.ayastorm_x64/logs/AYAstorm-perf-{2,3,4,5,6}pass-baseline.csv` | Layer 0-1 |
| 7 周目 (Layer 2) | `AYAstorm-perf-7pass-baseline.csv` | doOcclusion 6.17 ms 主犯特定 |
| 8 周目 (Layer 3) | `AYAstorm-perf-8pass-baseline.csv` | vwDraw_setup 13.15 ms 主犯特定 |
| 9 周目 (Layer 4) | `AYAstorm-perf-9pass-baseline.csv` | matrixInit 99.7% 占有 (setup 3.57 ms outlier) |
| 10 周目 (Layer 5) | `AYAstorm-perf-10pass-baseline.csv` | setLineWidth 16.64 ms 主犯確定 |
| 11 周目 (Layer 5 打ち手 A 検証) | `AYAstorm-perf-11pass-baseline.csv` | 打ち手 A 適用後 stall 移動確定 |
| **12 周目 (Layer 6)** | **`AYAstorm-perf-12pass-baseline.csv`** | **doOcclusion 3 分割 = reflectionProbes 94.7% 占有確定** |
| 13 周目 (Layer 6 打ち手 B 検証、次 session) | 同 path 上書き → 退避 | reflectionProbes ~4128 → ~2000-2500 us 検証 |

---

## 7. 配線済 zone 全リスト (Layer 1 - 6)

### Layer 1 広域 (commit ef1defa450)
- `doFrame_total`, `display`, `idleNetwork`, `idle_other`, `renderGeom`, `renderGeomDeferred`, `renderGeomPost`, etc. (詳細 03 spec §6.5 Group F)

### Layer 2 spatial / 非同期 (commit c7ccb128a9)
- `doOcclusion` (parent, line 6 で 3 分割)
- `updateGL`, `updateImagesCreateTextures`, `updateImagesUpdateStats`, etc. (詳細 03 spec §6.6 Group G)

### Layer 2-3 pipeline render (commit 47395b8962)
- `renderShadow_body_cull`, `renderShadow_body_geom`, `renderShadow_body_alpha` (Layer 3)
- `renderShadow_body_matrixSetup`, `renderShadow_body_innerOcclusion`, `renderShadow_body_cubeTeardown` (Layer 4)

### Layer 3 / 4 / 5 vwDraw (commit 42e3c8d773)
- 詳細 前 handoff doc §7 を引き継ぎ

### **Layer 6 doOcclusion 3 分割** (commit 171d6b90d1):
- `doOcclusion_reflectionProbes` (line 3172、reflection map + Hero probe occlusion + 旧 (B) block 含む parent scope)
- `doOcclusion_spatialGroups` (line 3230、`sCull->beginOcclusionGroups()` iter × `group->doOcclusion`)
- `doOcclusion_voCache` (line 3243、region list × `vo_part->processOccluders`)

### 次 session で配線候補 (Layer 7、必要なら)
- `doOcclusion_reflectionProbes` の更に内側分解: `mReflectionMapManager.doOcclusion()` vs `mHeroProbeManager.doOcclusion()` の 2 分割
- 13 周目で打ち手 B 効果が想定の半分以下しか出ない場合のみ着手

---

## 8. 必読 memory (前 handoff §9 と同じ、+ 追加)

前 handoff §9 全項目 + 以下:

| memory | 追加理由 |
|---|---|
| `project_uchite_b_mirror_followup.md` (新規) | **打ち手 B 視覚検証 deferred、release 前必須チェック** |
| `feedback_release_with_user_feedback.md` | exhaustive solo acceptance を組まない原則、打ち手 B mirror 判断の根拠 |
| `feedback_feature_value_in_main_usecase.md` | AYAstorm 主流 use case で mirror probe シーンが流行っていない事実踏まえて検証粒度判断 |
| `feedback_doubt_self_first.md` | **Layer 5 で「spatial group polling が主犯」と推論したが Layer 6 で完全 falsify、Layer 7 で更なる drill が必要なら自分の推論を疑って実測優先** |

---

## 9. 既知 risk

1. **未 push の 7 commit が branch `feature/ayastorm-r31-cpu-perf` に積まれている**: `feedback_release_flow.md` 通り push は AYA 側、次 session でも勝手に push しない。commit aggregation 戦略を相談してから AYA 実行依頼。
2. **AYAPerfLogEnabled=1 が persist 状態**: 13 周目計測継続 OK、ただし release 出荷前に必ず default=0 戻し案内 (05 spec / release note 両方)。
3. **観測者効果の累積**: Layer 1-6 で zone 数 = 広域 ~15 + spatial / 非同期 ~10 + render ~6 + vwDraw ~14 + doOcclusion 3 = 約 48 zone。13 周目で既存 zone (renderShadow / vwDraw / matrixInit など) が過去周回値と整合しているか sanity check 必須。
4. **mirror probe 視覚 regression は未検証**: `project_uchite_b_mirror_followup.md` に集約済、release 前に必ず参照。検証用シーンが揃わない場合は release note に「mirror に異常があれば issue 報告ください」明記。
5. **doOcclusion ~1800 us 差分 (Σ 4358 vs Layer 2 観測 6170)**: parent 直下の `LLGL*` ctor/dtor / gGL state change と推定、現時点は誤差として打ち切り。13 周目で打ち手 B 後も差分が同程度 (~1800 us) なら無視で OK、もし差分が縮まる場合は何か別の挙動。

---

## 10. 引き継ぎ session 開始時の AYA さん発話例

```
docs/specs/ayastorm-cpu-perf/handoff-layer6-to-13pass-analysis.md 読んで進めて。
```

または:
```
handoff-layer6-to-13pass-analysis.md 読んで、13 周目計測の準備お願いします。
```

または commit 戦略を先に相談したい場合:
```
handoff-layer6-to-13pass-analysis.md 読んで、まず 7 commit の集約戦略を相談したい。
```

---

## 11. Open task list (引き継ぎ時に TaskCreate)

- [ ] branch `feature/ayastorm-r31-cpu-perf` の最新 (114d3dd532) 確認 + build + install + cache clear
- [ ] AYA に 13 周目 active session 録音依頼 (同 SLurl、時刻不問)
- [ ] 12 周目 CSV を `AYAstorm-perf-12pass-baseline.csv` に退避済確認、13 周目を上書き → 退避
- [ ] 13 周目 CSV 解析 (`doOcclusion_reflectionProbes` 想定 ~2000-2500 us を検証)
- [ ] 04 spec §4.2.h に Layer 6 + 打ち手 B 結果追記
- [ ] 03 spec §6.10 Group L (Layer 6 catalog) 追記
- [ ] 05 spec 新規 (打ち手 A + B まとめ + 出荷判断)
- [ ] commit aggregation 戦略 AYA 相談 (7 commit そのまま merge / fix のみ cherry-pick / docs amend など)
- [ ] `AYAPerfLogEnabled=0` 戻し案内文を 05 spec に明記
- [ ] `project_uchite_b_mirror_followup.md` の release 前確認 task 化 (r31 release branch merge 前に必ず参照)
- [ ] (打ち手 B 効果が想定半分以下の場合のみ) Layer 7 配線 = `mReflectionMapManager.doOcclusion()` vs `mHeroProbeManager.doOcclusion()` 2 分割
