# Handoff: 17 周目完了 → 打ち手 C/F revert 作業

**Date:** 2026-05-26
**Branch:** `feature/ayastorm-r31-cpu-perf`
**HEAD:** `82c32ea932` (打ち手 B revert)
**Status:** uncommitted changes 3 ファイル (打ち手 C + 打ち手 F + Layer 8 zone 配線)

## AYA さん最終指示 (literal)

> 打ち手 C も合わせて revert / 効果的な方法がないなら元に戻すで

## 13-17 周目の経緯まとめ

| pass | 打ち手 | 結果 | 判定 |
|------|--------|------|------|
| 13 | (B) Hero probe 2 重 occlusion query 除去 (commit `114d3dd532`) | `doOcclusion_reflectionProbes` 4128→5603 us 悪化 | 効果なし → scene 違い疑い |
| 14 | Layer 7 hero/map 分割 → hero=0 確定 | 打ち手 B が AYA scene で効果不在と判明 | `82c32ea932` で revert |
| 15 | Layer 8 4-zone 配線 (`reflMap_doOcc_earlyReturn/queryGen/queryPoll/queryPush`) | `queryGen` = 4940 us/frame (1113 us/call × 4.4 frame) が主犯確定 | 真因特定 |
|  | mProbes = 4081 個 (octree 15-17m node 自動 register、設計通り) | – | – |
| 16 | (C) angular size culling (`r/d < 0.01` skip) | -7.4% のみ | 新規 probe は camera 近くで生まれ cull 通過、効果限定 |
| 17 | (F) query pool BATCH=256 lazy gen | `queryGen` 99% 削減 (1378→18 us/call) 成功、ただし `queryPush` 18 倍爆増 (0.05→0.92 us/call、+4700us)、net `doFrame` +7650 us 悪化 | **逆効果** |

**仮説 (打ち手 F の queryPush 爆増理由):** 256 個 pre-allocated query が GPU 上で fresh state、初 `glBeginQuery` で driver state init / sync cost が `glBeginQuery` 側に shift した可能性 (未検証)。

## 結論

- 打ち手 B: 効果不在 (revert 済 `82c32ea932`)
- 打ち手 C: 効果限定 (-7.4%、構造的に弱い) → AYA 指示で revert
- 打ち手 F: 逆効果 → revert
- **reflection probe occlusion 側で main thread CPU を削る有効策が現状なし** ということが 5 周の計測で確定

## 次 session で行う作業

### 1. 打ち手 C/F の uncommitted 変更を破棄

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm
git checkout indra/newview/llreflectionmap.cpp indra/newview/llreflectionmapmanager.cpp indra/newview/llreflectionmapmanager.h
```

これで打ち手 C + 打ち手 F + Layer 8 zone 配線が全部消える。

**注意:** Layer 8 zone 配線 (4 zone wrap) も同じ commit に乗っているので一緒に消える。「元に戻す」を literal に取れば Layer 8 も消すのが自然。AYA さんに「Layer 8 zone も計測 infra として残しますか」と 1 行確認するのが安全。

### 2. AYA 確認事項 (revert 前に必ず聞く)

1. **Layer 8 zone 配線も消していいか?** (計測 infra として残す価値はあるが、AYA は「元に戻す」と言った)
2. **18 周目 (revert 後 baseline 確認) 取るか?** 取らないなら直接 r31 章 close 方針に移行
3. **r31 章の今後の方向**: doOcclusion 以外の hot path (現状 4 番目以降) に drill するか、r31 を一旦 audio/video chapter (r30 続き) に戻すか

### 3. handoff doc + spec 更新

- 本ファイル (`handoff-17pass-to-revert.md`) の経緯は spec へ昇格
- 新規: `docs/specs/ayastorm-cpu-perf/05-doOcclusion-attempts.md` で打ち手 B/C/F の試行と revert 理由を記録
- `00-overview.md` の status を「reflection probe occlusion は drill 完了、有効策なし」に更新
- `handoff-layer6-to-13pass-analysis.md` は本ファイルに置き換え (削除でも可)

### 4. 残す commit

最終 branch 状態 (revert 後):

```
82c32ea932 Revert "fix(perf): doOcclusion 内 Hero probe 2 重 occlusion query を除去 (打ち手 B)"
114d3dd532 fix(perf): doOcclusion 内 Hero probe 2 重 occlusion query を除去 (打ち手 B)  [revert済]
171d6b90d1 perf(zone): Layer 6 LLPipeline::doOcclusion 3 分割 zone 配線
2b66da6ec7 docs(perf): CPU perf 章 spec 9 本を追加
42e3c8d773 perf(zone+fix): vwDraw zone 配線 + 打ち手 A (LLUI::setLineWidth 削除)
47395b8962 perf(zone): Layer 2-3 pipeline render zone 配線
```

Layer 6 zone 配線 (`171d6b90d1`) は計測 infra として残す価値あり、AYA 確認上で keep か decide。

## 参考: uncommitted の中身 (revert 対象)

- `indra/newview/llreflectionmap.cpp` (+15/-1): Layer 8 4-zone 配線 + 打ち手 F (acquireOcclusionQuery 呼出)
- `indra/newview/llreflectionmapmanager.cpp` (+40): 打ち手 C (angular size cull) + 打ち手 F 本体 (`acquireOcclusionQuery` 実装)
- `indra/newview/llreflectionmapmanager.h` (+9): 打ち手 F (`acquireOcclusionQuery` 宣言 + `mOcclusionQueryPool` member)

## 学び (memory 化候補)

- **glGenQueries pool 化は GPU driver state 移行 cost が glBeginQuery に shift するだけで net で勝つとは限らない** → 計測してから確定すべき
- **octree node 自動 probe register は spatial design 仕様**、reflection probe 数を絞るには別レイヤー (空間設計 or update 周期間引き) からの介入要
- **打ち手提案時は angular size cull が「新規 probe は近距離で生成」 のような構造的バイアスを持ちうる** → 案出し段階で sample distribution 確認すべき

---

**次 session 開始時の prompt 例:**

> AYA さん指示「打ち手 C も合わせて revert / 効果的な方法がないなら元に戻す」を受けて、`handoff-17pass-to-revert.md` に沿って revert 作業を進めて。まず Layer 8 zone 配線の扱い (残す/消す) を確認してから checkout。
