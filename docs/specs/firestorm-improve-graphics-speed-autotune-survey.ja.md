# Firestorm Improve Graphics Speed / AutoTune 調査報告

作成日: 2026-06-05

## 目的

AYAstorm の読み込み中 FPS 改善計画と、Firestorm 本家の `Improve Graphics Speed` / `AutoTune FPS` を併用できるかを整理する。

結論として、`Improve Graphics Speed` は残す価値がある。ただし役割は「混雑時に avatar / scene の描画負荷を落とす Firestorm 既存機能」であり、AYAstorm 側で実装している `RenderResolutionMultiplier`、dense-world LOD、texture priority、backpressure、texture memory sweep を代替しない。

併用する場合は、AutoTune を avatar / scene の持続的な負荷調整レイヤー、AYAstorm を in-world 固定 profile による 3D 描画軽量化とテクスチャ読み込み・メモリ管理レイヤーとして分ける。AYAstorm 側の通常 UI は、FPS 値や teleport/backlog を発動条件にしない。

## 参照資料

- Firestorm 開発者 Beq Janus による解説: [How to use the new Firestorm Performance Floater and Autotune feature](https://beqsother.blogspot.com/2022/03/how-to-use-new-firestorm-performance.html)
- Inara Pey による Firestorm 6.5.3 解説: [Firestorm 6.5.3: performance and photos (and more!)](https://modemworld.me/2022/03/22/firestorm-6-5-3-performance-and-photos-and-more/)
- ローカル実装: `indra/newview/fsfloaterperformance.cpp`, `indra/newview/llperfstats.cpp`, `indra/newview/llvoavatar.cpp`, `indra/newview/skins/default/xui/en/floater_fs_performance.xml`

## 公開ドキュメント上の仕様

Beq の解説では、Performance Floater は「シーンのどの部分がフレーム時間を使っているか」をリアルタイムに見せるものとして説明されている。初期実装の主眼は avatar で、近くの avatar や自分の attachment の負荷確認に使う。

入口は `World` メニューの `Improve graphics speed...`、または `Advanced -> Performance tools`、ツールバーの gauge アイコンである。ローカル XUI でも `menu_viewer.xml:1327` に同名メニューがあり、`Floater.Toggle performance` を呼ぶ。

AutoTune は、ユーザーが指定した target FPS に近づくように viewer settings を自動調整する機能である。ただし公開説明でも「魔法ではない」「高すぎる target は期待通りに到達しない」「設定が変わるので preset 保存/復帰を使うべき」とされている。

公開説明で明示されている調整対象は以下である。

| 領域 | 内容 |
| --- | --- |
| Summary | FPS と frame breakdown を見る。UI / HUD / avatar / scenery など、何が重いかを判断する |
| Scene | 公開記事上は draw distance、shadows、water reflections などが文脈として挙げられている。現行コードの AutoTune が能動的に戻し/下げを行う scene 設定は、実質 `RenderFarClip` が中心である |
| Avatar | avatar の render time を見て、重い avatar の shadow を落とし、それでも重い場合は impostor 化する |
| AutoTune | target FPS、avatars only / avatars and scene、continuous / one-shot を選ぶ |

重要なのは、公開説明の時点でも「まず measurement を見て最大コストに手を入れる」設計であり、texture 読み込み順や texture memory 破棄を扱う機能ではない点である。

## コード上の実装仕様

### UI 入口

`performance` floater は Firestorm 独自の `FSFloaterPerformance` に割り当てられている。

- upstream 系 `LLFloaterPerformance` 登録はコメントアウト: `indra/newview/llviewerfloaterreg.cpp:561`
- 有効登録: `LLFloaterReg::add("performance", "floater_fs_performance.xml", FSFloaterPerformance)` at `llviewerfloaterreg.cpp:695`
- メニュー入口: `menu_viewer.xml:1327`
- floater title: `floater_fs_performance.xml:5`

floater 上部には `TargetFPS` spinner、`AutoTuneFPS` Start/Stop、`AutoTuneLock` Continuous、`TuningFPSStrategy` combo がある。XUI 上では `TargetFPS` の説明に「viewer will attempt to achieve this」と書かれており、ここもユーザーに「必ず到達する」と誤解されやすい。

### 設定値

AutoTune 関連の主な設定は以下である。

| 設定 | 既定 | 役割 |
| --- | ---: | --- |
| `TargetFPS` | 15 | AutoTune が目標にする FPS |
| `AutoTuneFPS` | false | AutoTune の Start/Stop |
| `AutoTuneLock` | false | Continuous 動作 |
| `KeepAutoTuneLock` | 1 | 次回起動時の lock 維持 |
| `TuningFPSStrategy` | 1 | 0=avatars only, 1=avatars and scene, 2=scene only |
| `RenderAvatarMaxART` | 4.699 | avatar render time limit のログスライダー値 |
| `AutoTuneRenderFarClipMin` | 32 | AutoTune が下げてよい最小 draw distance |
| `AutoTuneRenderFarClipTarget` | 256 | AutoTune が戻そうとする draw distance |
| `AutoTuneImpostorByDistEnabled` | false | 距離ベースで full avatar 数を制限 |
| `AutoTuneImpostorFarAwayDistance` | 64 | full avatar として扱う最大距離 |

設定 change handler は `llviewercontrol.cpp:1319` 以降にあり、`TargetFPS`、`AutoTuneFPS`、`AutoTuneLock`、`RenderAvatarMaxART`、draw distance min/target、impostor distance、strategy を `LLPerfStats::tunables` へ反映する。登録は `llviewercontrol.cpp:1728` 以降で行われる。

### 更新ループ

AutoTune の中核は floater ではなく `LLPerfStats` である。

`llappviewer.cpp:1584` で、`userAutoTuneEnabled` かつ `tuningFlag != Nothing` のときだけ `Tunables::applyUpdates()` が呼ばれ、設定へ書き戻す。`applyUpdates()` の write list は `llperfstats.cpp:78` から確認できる。

`Tunables::applyUpdates()` が書き換える対象は以下に限定される。

- `RenderAvatarMaxNonImpostors`
- `RenderReflectionDetail`
- `RenderFarClip`
- `AutoTuneRenderFarClipMin`
- `AutoTuneRenderFarClipTarget`
- `AutoTuneImpostorFarAwayDistance`
- `AutoTuneImpostorByDistEnabled`
- `TuningFPSStrategy`
- `AutoTuneFPS`
- `AutoTuneLock`
- `TargetFPS`
- `RenderAvatarMaxART`

このリストに `RenderResolutionMultiplier`、texture create queue、texture memory sweep、AYAstorm の loading priority 設定は含まれない。

`RenderReflectionDetail` は write list に残っているが、現行の調整ロジックでは reflection 側の能動調整が `#if 0` で無効化されている。したがって現行実装の scene-side AutoTune は `RenderFarClip` の段階調整として扱うのが実態に近い。

### AutoTune アルゴリズム

`StatsRecorder::updateAvatarParams()` が実際のチューニングを行う。主な流れは以下である。

1. window が background のとき、または background sleep が記録されたときは tuning を止める。FPS limiter 分は frame time から差し引いて評価を継続する。
2. VSync が有効なら target FPS を monitor refresh rate で clamp する。
3. background から戻った直後は、target FPS の約 5 秒分の frame が経つまで tuning を待つ。
4. `AutoTuneImpostorByDistEnabled` が有効なら、`min(RenderFarClip, AutoTuneImpostorFarAwayDistance)` 内の avatar 数を数え、`RenderAvatarMaxNonImpostors` を更新する。
5. `target_frame_time = cpu_hertz / target_fps` を計算し、実測 frame time と比較する。
6. target に届いていない場合、非 avatar 時間が target frame time を超えていれば、`TuningFPSStrategy != avatars only` のとき `RenderFarClip` を 10m ずつ下げる。
7. それでも足りない、または avatar 時間が budget を超える場合、`TuningFPSStrategy != scene only` のとき `RenderAvatarMaxART` を下げる。
8. target を上回り余裕が出た場合、avatar render time limit を緩め、`RenderFarClip` を target draw distance へ 10m ずつ戻す。
9. one-shot の場合、安定後に内部状態は AutoTune 停止へ遷移する。Continuous の場合は維持する。

`llperfstats.h:71` 以降の定数では、avatar render time limit は下げ方向 10000ns、上げ方向 5000ns、draw distance は `DD_STEP=10` で動く。

### Avatar 側の反映

avatar の shadow / impostor 判定は `LLVOAvatar::updateTooSlow()` で行われる。

現行コードでは `LLVOAvatar::updateTooSlow()` は `llvoavatar.cpp:10033` 付近にあり、`llvoavatar.cpp:10047` 付近で AutoTune が有効、control avatar ではない、自分自身でもない場合だけ `autotune=true` になる。したがって自分の avatar は AutoTune 専用の近傍例外/AutoTune 判定から外れる。ただし AutoTune が下げた `RenderAvatarMaxART` は共通閾値なので、`AllowSelfImpostor` が有効な場合、自分 avatar の impostor 判定へ間接的に影響し得る。

重い avatar はまず `mTooSlow` になり、shadow 除外対象になる。shadow を落としても `max_art_ms` を超える場合は `mTooSlowWithoutShadows` になり、impostor 対象になる。状態変化時は geometry rebuild が要求される。

旧 AYAstorm 実装には、`AYALoadingTextureManagementEnabled=true` かつ近距離/可視 avatar の fallback 条件が成立している場合、未完成 impostor へ流す前に `isImpostor()` / `shouldImpostor()` を false 側へ倒す経路があった。この経路は AutoTune の「重い avatar は impostor へ落とす」という設計と衝突し、さらに直近ログでは `aya_simple_fallback=true` が 0 件で、complete impostor 側へ戻って body pass が suppress される個体が多かった。

この差分は設計上の衝突点だったため撤回済みである。Improve Graphics Speed は avatar 描画負荷を下げるが、AYA の UX 要件は「Firestorm 標準の最大アバター数低下より悪くしない」ことである。AYAstorm は AutoTune の avatar policy を上書きせず、texture priority / cap / memory sweep の範囲に責務を戻す。併用時は FPS 改善だけでなく `AYA avatar_draw_state` / `AYA avatar_render_skip` と scene texture starvation のログを同時に見る必要がある。

## AutoTune が制御しないもの

今回の AYAstorm 修正計画と比較する上で、以下は明確に AutoTune の対象外である。

| 項目 | AutoTune の扱い | AYAstorm 側の扱い |
| --- | --- | --- |
| `RenderResolutionMultiplier` | 書き換えない | 3D描画軽量化の fixed dynamic resolution profile |
| dense-world LOD | 書き換えない | scene-bound 補助として LOD bias / forced refresh |
| texture create priority | 書き換えない | テクスチャ読み込み・メモリ管理 |
| texture create backpressure | 書き換えない | テクスチャ読み込み・メモリ管理 |
| distance/offscreen texture cap | 書き換えない | テクスチャ読み込み・メモリ管理の低解像度化 |
| texture memory sweep | 書き換えない | テクスチャ読み込み・メモリ管理 |
| teleport texture sweep | 書き換えない | テクスチャ読み込み・メモリ管理 |

このため、「Improve Graphics Speed を使えば texture priority や memory sweep も解決する」という期待は誤りである。一方で、avatar / scenery が frame time を支配している場面では、AutoTune と AYAstorm の 3D描画軽量化を併用する価値がある。

shader / postprocess / FXAA は AutoTune の対象外である。これは AYAstorm の通常 UI 2系統にも直接含めず、`RenderResolutionMultiplier` 周辺の表示整合性修正として別扱いにする。

## AYAstorm 修正計画との組み合わせ

AYAstorm の通常 UI は以下の 2 系統に分ける。texture priority と texture memory cleanup は、近距離/可視/HUD を先に成立させつつ不要 texture を整理する1つの利用者向け項目として扱う。

| 系統 | 目的 | 主な対象 |
| --- | --- | --- |
| 3D描画を軽くする | チェック ON 中の in-world 固定 profile で pixel / scene-bound 負荷を下げる | `RenderResolutionMultiplier`, dense-world LOD |
| テクスチャ読み込み・メモリ管理 | 近距離 avatar / object / HUD を先に成立させ、不要 texture を整理する | texture create priority, backpressure, distance/offscreen cap, texture downscale strength, memory sweep, teleport sweep |

`Improve Graphics Speed` / AutoTune は、このうち `3D描画を軽くする` とだけ一部重なる。ただし同じ設定を書き換えるわけではない。

### 2026-06-07 チーム再議論による併用方針

直近の議論で、AYAstorm の `FPS安定化` を「FPS が低い時、teleport 直後、texture backlog がある時だけ発動する adaptive guard」として扱う方針は撤回した。ユーザーがチェックボックスを ON にした場合は、login/XUI/snapshot などの安全除外を除き、in-world では選択した固定 profile が効くべきである。

このため、AutoTune との責務分担も以下へ修正する。

- AYAstorm は、AutoTune の前段に敷く固定軽量 baseline とする。AutoTune が active でも AYA の fixed profile を勝手に止めない。
- AYAstorm は `TargetFPS`, `AutoTuneFPS`, `AutoTuneLock`, `TuningFPSStrategy`, `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors` を所有しない。
- AYAstorm の 3D 解像度 profile は `AYALoadingResolutionReliefStrength` などの強度で決まり、実測 FPS から倍率を再計算しない。
- AutoTune は AYAstorm baseline が敷かれた後の frame breakdown を見て、Firestorm 既存の avatar / draw distance 調整を行う。
- AYAstorm は `isImpostor()` / `shouldImpostor()` を上書きして Firestorm 標準の avatar visibility policy を壊さない。avatar の表示可否は Firestorm 標準の最大アバター数、impostor、visual mute、AutoTune の責務へ戻す。
- AYAstorm の texture management は、texture fetch/decode/create priority、distance/offscreen cap、memory/teleport sweep を扱う。avatar mesh / attachment mesh / object visibility は直接変えない。
- FPS、teleport、texture backlog、callback backlog は発動条件ではなく、`why slow` を分類する telemetry と A/B 評価軸として使う。

### 2026-06-07 UX 基準による再整理

直近の旧実装の実機確認では、AYAstorm 側の texture management / avatar fallback / impostor 制御が重なった結果、Firestorm 標準の「最大アバター数を下げる」運用よりも質の悪い体験が発生していた。最大アバター数を下げる場合、ユーザーは「表示人数を制限している」と理解しやすいが、旧 AYA 実装では近距離 avatar が名前だけ残る、簡易表示に落ちない、complete impostor 扱いで body pass が止まる、scene texture が後回しに見える、という混在状態が起きた。現行実装では avatar fallback / impostor 制御を Firestorm 標準 policy へ戻している。

したがって、AYAstorm の候補機能は `Improve Graphics Speed` / AutoTune との併用以前に、Firestorm 標準の負荷低減より UX が悪ければ不採用または縮小する。

新しい比較 baseline:

| baseline | 内容 | 目的 |
| --- | --- | --- |
| `FS0` | AYA loading 系 OFF、Firestorm 標準設定 | 既存挙動 |
| `FS1` | AYA loading 系 OFF、最大アバター数/AutoTune/Improve Graphics Speed で軽量化 | Firestorm 標準の軽量化 UX |
| `AYA-R` | AYA 3D描画軽量化のみ | fixed dynamic resolution / dense-world LOD の効果 |
| `AYA-T` | AYA texture management のみ | texture priority/cap/sweep の効果 |
| `AYA-C` | AYA 3D描画軽量化 + texture management | 併用時の副作用確認 |

採用条件は `FS1` より avatar visibility と scene readability が悪化しないことである。FPS が `FS1` より高くても、近距離 avatar が名前だけ、簡易表示へ落ちない、scene texture が長時間出ない、HUD/近距離看板が読めない場合は採用しない。

この基準により、AYAstorm 側は以下の責務へ縮小する。

- `3D描画軽量化`: 3D render target 解像度と non-avatar world LOD を下げる。avatar の表示可否や impostor eligibility を直接所有しない。
- `テクスチャ読み込み・メモリ管理`: texture fetch/decode/create priority、distance/offscreen cap、memory/teleport sweep を扱う。avatar mesh / attachment mesh / object visibility を直接変えない。
- `avatar visibility safety`: Firestorm 標準の max avatar / impostor / friends-only / visual mute を基準にし、AYA 独自の fallback / `isImpostor()` override は現行では所有しない。cloud particle OFF は texture management と自動連動しない独立 debug/UX 設定として扱う。

### 推奨する責務分担

| レイヤー | 担当 | 書き換えてよいもの |
| --- | --- | --- |
| Firestorm AutoTune | avatar / scene の持続的な描画負荷調整 | `TargetFPS`, `AutoTuneFPS`, `AutoTuneLock`, `TuningFPSStrategy`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`, `RenderFarClip` |
| AYA 3D描画軽量化 | in-world 固定 profile による pixel / scene-bound 負荷低減 | session-only dynamic resolution profile、dense-world LOD profile |
| AYA テクスチャ読み込み・メモリ管理 | 近距離 / 可視 / HUD の成立順と不要 texture の整理 | texture queue priority, backpressure, texture cap, downscale strength, memory sweep / teleport sweep |

AYAstorm から AutoTune の `TargetFPS` や `AutoTuneFPS` を勝手に変更しない。ユーザーが Firestorm 側の AutoTune を明示的に使っている場合、AYAstorm はその状態を telemetry として読むだけにする。AutoTune active を理由に AYA fixed profile を停止すると、ON にした機能が効いたり効かなかったりするため避ける。

### 組み合わせると効果が出やすいケース

- `Improve Graphics Speed` の breakdown で `avatars` が大きい。
- `scenery` が大きく、draw distance を下げる余地がある。
- AYA fixed dynamic resolution profile を下げても FPS が上がり切らず、`geom`、`statesort`、`lighting` が残っている。
- 混雑地で avatar impostor / shadow removal を許容できる。

このケースでは、AutoTune が avatar / draw distance を下げ、AYAstorm が render target 面積と dense-world LOD を下げるため、負荷低減の方向が重複しにくい。

### 組み合わせても効果が薄いケース

- texture decode / upload / queue backlog が支配的で、frame breakdown の avatar / scenery が主因ではない。
- process RSS が高く、texture memory を捨てられていない。
- HUD や UI が重い。AutoTune は HUD texture の読み込み優先や破棄をしない。
- `TargetFPS` が現実的でなく、AutoTune が avatar を impostor 化しても到達不能。

この場合は AutoTune より、AYAstorm のテクスチャ読み込み・メモリ管理を検証すべきである。ただし texture management は FPS 低下時だけの緊急処置ではなく、チェック ON 中の texture QoS policy として評価する。

## 実装方針

短期では、AutoTune と AYAstorm を UI 上で無理に統合しない。混ぜるべきなのは内部ロジックではなく、検証ログと運用設計である。

1. AYAstorm は `AutoTuneFPS`, `AutoTuneLock`, `TargetFPS`, `TuningFPSStrategy` を自動変更しない。
2. AYAstorm の 3D描画軽量化が有効なとき、AutoTune が有効なら `autotune_active=1` としてログに含める。
3. 同じログ行に `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`, `RenderResolutionMultiplier` 保存値, AYA fixed profile multiplier, effective multiplier, dense-world LOD profile / active 状態を出す。
4. AYAstorm の 3D描画軽量化は `RenderFarClip` と `RenderAvatarMaxART` を所有しない。そこは Firestorm AutoTune の所有範囲とする。
5. AYAstorm の 3D描画軽量化は FPS、teleport、backlog を gate にしない。gate はチェック ON/OFF、profile 0、in-world、login/XUI/snapshot safety に限定する。
6. UI では `Improve Graphics Speed` を「Firestorm 標準の avatar / scene 負荷調整」として案内するに留め、AYAstorm Controls の2系統とは別にする。

中期で統合する場合も、`AYAstorm Controls` のチェックボックスが Firestorm AutoTune を勝手に ON にする設計は避ける。入れるなら「Firestorm AutoTune と併用中」の状態表示、または `Improve Graphics Speed` floater を開くボタン程度が安全である。

## 検証マトリクス

| ID | Firestorm AutoTune | AYA 3D描画軽量化 | AYA テクスチャ読み込み・メモリ管理 | 見るべき指標 |
| --- | --- | --- | --- | --- |
| B0 | OFF | OFF | OFF | baseline FPS, frame breakdown, RSS |
| F1 | ON | OFF | OFF | avatar / scenery 低下、impostor 数、RenderFarClip |
| A1 | OFF | ON | OFF | `RenderResolutionMultiplier`, dense-world LOD, FPS |
| A2 | OFF | OFF | ON | 近距離 avatar / HUD / object の成立順、RSS、destroy/downscale 副作用 |
| C1 | ON | ON | ON | avatar / scene / pixel / texture memory の併用効果 |

検証時は `Improve Graphics Speed` floater の frame breakdown と AYAstorm ログを同時に見る。`FPS が上がった` だけでは原因を判断しない。

## リスク

| リスク | 内容 | 対策 |
| --- | --- | --- |
| target FPS の誤解 | AutoTune の `TargetFPS` は到達保証に見えやすい | AutoTune は Firestorm 標準 UI として扱い、AYAstorm 側から値を変えない |
| AYA strength の誤解 | AYAstorm の強度 profile が「この FPS まで上げる設定」に見えると誤解を招く | AYAstorm 側 UI では target 表現を避け、強度 / 画質低下 / 固定 profile として説明する |
| 設定所有権の衝突 | AutoTune と AYAstorm が同じ設定を触ると原因追跡できない | AYAstorm の 3D描画軽量化は `RenderFarClip` / `RenderAvatarMaxART` を触らない |
| avatar 未表示 | `AutoTuneImpostorByDistEnabled` / `AutoTuneImpostorFarAwayDistance` / max non-impostor / priority が近距離 avatar の成立や表示形態に影響し得る | AYA は avatar visibility policy を上書きしない。近距離 avatar / own avatar / HUD は texture priority 側で保護し、AutoTune 側の distance impostor 設定を別ログで見る |
| memory sweep の逆効果 | texture 破棄と再確保が増えると FPS が下がる | sweep は単独 A/B と combined A/B を分ける |
| background FPS | background 化で FPS が 10 程度に落ちる | AYA fixed profile は FPS を発動条件にしないため、background FPS を control input に使わない。ログ分類では background state を記録する |

## 結論

`Improve Graphics Speed` は、AYAstorm の 3D描画軽量化や texture QoS policy と競合するものではなく、avatar / scene 側の既存 Firestorm レイヤーとして併用できる。

ただし AutoTune の制御範囲は限定的で、`RenderResolutionMultiplier`、dense-world LOD、texture priority、texture memory sweep には関与しない。したがって、AYAstorm 側で必要なのは AutoTune を置き換えることではなく、AutoTune の状態をログに含めつつ、3D描画軽量化とテクスチャ読み込み・メモリ管理を分けて検証できるように保つことである。

次に実装するなら、まずは AutoTune 状態を AYAstorm perf ログへ含めるだけに留める。AutoTune の ON/OFF や target FPS を AYAstorm 側から自動変更する実装は、効果検証が終わるまで入れない。
