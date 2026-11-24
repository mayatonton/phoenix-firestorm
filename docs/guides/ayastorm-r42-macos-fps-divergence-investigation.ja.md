# AYAstorm R42 macOS: Viewer FPS と見た目の FPS が乖離する事象の調査記録

## 結論

macOS のメイン画面で「Viewer の FPS 表示は高いが、3D scene は低い FPS に見える」事象は、
従来ログだけでは最終原因を確定できない。

今回の実行で確認できたのは、Viewer が数える `consumer_fps` が約 38 fps だった一方、async scene command
buffer の `producer_fps`、すなわち PE への submit cadence が約 13 fps だったことまでである。
`producer_fps` は GPU 完了、front 切替、ディスプレイへの表示完了のいずれも数えていない。

従って、同じ scene の再表示は有力な仮説だが、3D scene が実際に約 13 fps で表示されたとはまだ証明できない。
FPS 表示が壊れているとも、macOS のメイン画面切替が原因だとも、このログだけからは結論づけない。

## 1. 用語

| 用語 | この調査での意味 |
| --- | --- |
| consumer | swapchain に UI と、直近で完成した 3D scene を合成して present する側。Viewer の `FPS` と `#VkPerf# fps` はこれを数える。 |
| producer | 3D world scene を別の render target へ描画して、consumer が表示できる状態にする側。 |
| `consumer_fps` | consumer が画面を合成した回数/秒。古い scene を再表示しても増える。 |
| `producer_fps` | async scene command buffer を PE へ submit した回数/秒。GPU 完了回数や scene 表示回数ではない。 |
| `ready1st` | submit 後の最初の完了確認で ready だった回数 / 後続確認を含めて ready を検出した総数。 |

## 2. 今回の実測

開発 app を `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5` で起動したログでは、次を確認した。

```text
uiscene consumer_fps=38.20 producer_fps=12.67 ready1st=0/65
frames=193 fps=38.20 avg_ms=26.18
cadence acq_fps=38.20 present_call_fps=38.39 present_ok_fps=38.39
present_done_fps=n/a present_wait_avail=1 present_wait_used=0
```

この値は次のように読む。

1. consumer は 1 秒に約 38 回、UI と直近の scene を swapchain へ送っている。
2. producer は 1 秒に約 13 回、async scene command buffer を submit している。
3. `ready1st=0/65` のため、ready を検出した 65 件のいずれも submit 後の最初の完了確認では ready でなかった。
4. consumer が同じ front scene を複数回合成した可能性は高いが、実際に表示完了した回数と、その各回の
   `scene_id` が無いため、重複表示回数と見た目の 3D FPSは未確定である。

この run では `FRAMETIME ms:` の p95 が約 70--111 ms、`SLOWFRAME` では
`M:beg=80` ms 前後も連続していた。ただし `M:beg` は `beginFrame()` 全体の集計であり、既存ログだけでは
frame timeline、producer slot、async record slot、acquire のどこで時間を使ったか分離できない。

## 3. メイン画面との関係

**VERIFIED:** 現在の UI scene async の有効/無効を、macOS の「メイン画面」フラグで切り替える
コードはない。`sUISceneSplit` と `sUISceneAsync` はどちらも `true` に固定されている。

**VERIFIED:** 今回の main display は 5K の外部画面だが、Vulkan が実際に作成した swapchain は
`2560x1387` だった。従って、この実測だけから「5K の 4 倍の描画ピクセル数が直接原因」とは
結論づけない。

**OPEN:** メイン画面にした run と、メイン画面ではない run の表示完了 cadence および fresh scene cadence の
差は未計測である。メイン画面化が原因、または単なる顕在化条件だという解釈はいずれもまだ仮説である。

## 4. 物理的なモニター FPS について

この run の present mode は `IMMEDIATE (vsync OFF)` だった。
`VK_KHR_present_wait` は利用可能として初期化されているが、present 完了待機は FIFO mode のときだけ
呼ぶ実装なので、この run では `present_wait_used=0`、`present_done_fps=n/a` になる。

したがって、次の 2 点は分けて扱う。

- **VERIFIED:** Viewer 内の consumer cadence と async producer submit cadence は約 3:1 に乖離している。
- **OPEN:** Metal drawable が実際に表示完了した cadence と、そのうち新しい `scene_id` を表示した cadence。

`acq_fps` と `present_ok_fps` は WSI が image を取得・受理した回数であり、物理走査線への表示完了を
証明する値ではない。

## 5. 実装上の根拠

- [`llvkloader.cpp`](../../indra/llrender/llvkloader.cpp) は UI scene split / async を常時有効にする。
- [`llviewerdisplay.cpp`](../../indra/newview/llviewerdisplay.cpp) は producer の timeline 完了時だけ
  `mScenePresentFront` を更新する。未完了なら consumer は前の scene を表示する。
- [`llvkpresent.cpp`](../../indra/llrender/llvkpresent.cpp) は FIFO present のときだけ present ID と
  `vkWaitForPresentKHR()` を使用する。

## 6. 次の調査境界

性能修正を選ぶ前に、表示完了と scene freshness を同じ run で取得する。

1. `VK_GOOGLE_display_timing` の `actualPresentTime` から drawable 表示完了 cadence を測る。
2. 各 Present の `presentID` と consumer enqueue 時点の `scene_id` を対応付け、fresh / duplicate / unknownを
   分離する。
3. 実表示は高いが fresh scene 表示だけが低い場合に、初めて producer内部の完了待ち、GPU実行時間、
   shadow per-layer command bufferを次の調査対象とする。

表示完了診断を入れる理由と受入条件は
[`ayastorm-r42-display-completion-diagnostics-rationale.ja.md`](../specs/ayastorm-r42-display-completion-diagnostics-rationale.ja.md)
に分離した。診断コードのbuildは完了したが、実機runの値はまだOPENである。

## 7. 2026-11-23 OpenSim ベンチでの再測定（中断時点の記録）

計測環境は `docs/guides/ayastorm-r42-opensim-bench.ja.md` の busy store、
`build-darwin-universal/newview/Release/AYAstorm.app`（arm64、HEAD `5d3512986a` に未コミット変更を加えたビルド）、
`AYASTORM_PERF_LOG=5`、`BackgroundYieldTime 0`、`--novoice`。各 60 秒。

| 状態 | 項目 | 根拠 |
|---|---|---|
| VERIFIED | 右上 fps（consumer）30.8 に対し、新しい 3D シーン（producer）は 10.3/s。シーンが更新されるのは 3 フレームに 1 回 | `uiscene consumer_fps` / `producer_fps`、`ready1st=0/51` |
| VERIFIED | GPU 使用率 93 %。このシーンは GPU 律速 | `ioreg` の `Device Utilization %` |
| VERIFIED | 以前の計測でフォーカスがなかった回は、毎フレーム `BackgroundYieldTime` 40 ms の sleep が入っていた（main thread の 66 %） | `sample`、`llappviewer.cpp:1878` |
| CLAIMED | 原因: シーンと UI だけのフレームは同じ in-order queue に積まれる。シーンが GPU にある間に記録した UI だけのフレームは、シーンの後で古いシーンを再表示するだけになる。`FRAMES_IN_FLIGHT=3` のため、3 フレーム目の `beginFrame` でシーン完了を待つ（`llvkloader.cpp` の slot 再利用待ち） | コードの読解。表示時刻での裏付けはない |
| OPEN | 実験用ゲート `AYASTORM_ASYNC_SCENE_GATE=1`（シーンが GPU にある間は `beginFrame` でその完了を待つ）の効果。1 回目: 右上 fps 12.7 = producer 12.7（ゲートなし 10.3 から +23 %）、GPU 92 %、待ち 約 4.5 秒/5 秒 | `gate_wait_ms`。反復は 1 回ずつで、ゲートなしの回は前面アプリが別アプリだった。2 回目は中断 |

ゲートを有効にすると右上 fps がシーンの更新回数と一致し、シーンの更新回数も増えた。ただし UI だけの更新も
シーンの頻度まで下がる。結論は反復計測と、体感（表示時刻）の確認の後に出す。

署名: 開発 app はローカル専用の自己署名証明書（`~/OpenSimBench/signing/`、ユーザーのキーチェーン検索リストに
追加済み）で `scripts/bench/sign_dev_app.sh` により署名済み。designated requirement は証明書に固定された
（VERIFIED）。再ビルド後に macOS の許可ダイアログが出ないことは未確認（OPEN）。

次にやること:

1. ゲートあり・なしを交互に各 2 回以上、ビューワーを前面に置いて計測する（`ab.sh` 相当の手順）。
2. `AYASTORM_VKC=1` で表示時刻（`fresh_scene_display_fps`、`duplicate_ratio`）を比べる。
3. 再ビルドして `sign_dev_app.sh` を実行し、許可ダイアログが出ないことを確かめる。
4. 結果が良ければ、ゲートを既定にするか、GPU 律速のときだけ働かせるかを決める。共通コードなので
   Windows / Linux への影響を確認する。

### 7.1 2026-11-24 再開後の A/B 計測

ビルドは上記に次の変更を加えたもの: `--novoice` のときに WebRTC を初期化しない（`llvoicewebrtc.cpp`）、
macOS の `--noaudio` のときにメディアプラグインを起動しない（`llviewermedia.cpp`）。起動は `--novoice --noaudio`。
各回ともログインから 100 秒（群衆あり）または 75 秒（群衆なし）待ってから 60 秒計測した。

**群衆なし**（OpenSim 再起動後に群衆スクリプトが止まっていた回。NPC 0 体）

| 回 | ゲート | 右上 fps | producer | 新しいシーンの表示 /s | 重複率 | GPU |
|---|---|---|---|---|---|---|
| b | なし | 76.1 | 25.3 | | | 88 % |
| b | あり | 14.7 | 14.7 | | | 93 % |
| c | なし | 77.1 | 25.7 | | | 88 % |
| c | あり | 25.9 | 25.8 | | | 88 % |
| VKC | なし | 74.3 | 24.7 | 24.8 | 66.7 % | 89 % |
| VKC | あり | 15.6 | 15.6 | 15.6 | 0 % | 92 % |

**群衆あり**（NPC 40 体）

| 回 | ゲート | 右上 fps | producer | 新しいシーンの表示 /s | 重複率 | GPU | 前面 |
|---|---|---|---|---|---|---|---|
| d | なし | 24.6 | 8.2 | | | 91 % | 別アプリ |
| d | あり | 16.3 | 16.3 | | | 90 % | 別アプリ |
| e | なし | 27.9 | 9.3 | | | 93 % | 別アプリ |
| e | あり | 7.9 | 7.9 | | | 92 % | 別アプリ |
| VKC | なし | 32.8 | 11.0 | 10.9 | 66.7 % | 94 % | AYAstorm |
| VKC | あり | 18.1 | 18.1 | 18.0 | 0 % | 91 % | AYAstorm |

| 状態 | 項目 |
|---|---|
| VERIFIED | ゲートなしでは、右上 fps の 3 分の 2 が古いシーンの再表示（`duplicate_ratio` 66.7 %）。画面に出る新しいシーンは producer と同じ（群衆あり 10.9/s、群衆なし 24.8/s）。右上 fps は体感の約 3 倍を示す |
| VERIFIED | ゲートありでは右上 fps = producer = 新しいシーンの表示で、重複は 0 % |
| VERIFIED | 群衆あり（GPU 律速）で前面が AYAstorm の回は、ゲートで新しいシーンが 10.9 → 18.0/s に増えた |
| VERIFIED | 群衆なしでは、ゲートで新しいシーンが約 25/s から 15〜26/s に減るかばらつく。CPU と GPU の重なりが失われるためと見られる（CLAIMED） |
| OPEN | 群衆ありの d・e 回は、計測中に別アプリが前面にあった。ゲートありの e 回（7.9）は外れ値の可能性がある |

結論: 右上 fps と体感のずれは、シーン 1 回に対して UI だけのフレームが 2 回入る構造から来ている。
ゲートは指標を正しくし、GPU 律速の場面では新しいシーンも増やすが、軽い場面では遅くなるため、そのまま既定にはしない。
次の候補は、(a) 前のシーンが完了したら次のフレームですぐ次のシーンを記録する（3 フレームおきの slot 待ちに縛られない）
構造の修正、(b) 右上の表示に新しいシーンの fps を併記すること。どちらも共通レンダラーの変更になる。

マイクの許可ダイアログ: 署名を証明書に固定した直後の初回は、WebRTC、FMOD、CEF（UI の先読みブラウザー 3 つ）が
入力も持つ既定の出力デバイス（集約デバイス）を開き、マイクの許可を求めていた。上記の変更後のビルドは、
`tccd` のログ上マイクを要求せず、再ビルド後もデスクトップフォルダの許可ダイアログは出なかった（VERIFIED）。

### 7.2 2026-11-24 同期分割（`AYASTORM_ASYNC_SCENE=0`）の A/B 計測

すべての submit は 1 本のキュー（`sGraphicsQueue`）に順に入る。非同期では、シーンの後ろに UI だけのフレームが
2 回並び、GPU でシーンを待ったうえで古い front を表示していた。キューが順序どおりに実行されるため、
シーンを毎フレーム通常の producer command buffer（pjob）で記録し、front を毎フレーム切り替えても、
consumer N は直前に投入された pjob N-1 のシーンを読む。これを試験用スイッチ `AYASTORM_ASYNC_SCENE=0`
（既定は従来の非同期）として追加した。この模式では display timing の scene id を front の切り替えごとに進め、
deferred 解放の完了判定に pjob の完了を含める。

条件: 群衆 40 体、`AYASTORM_VKC=1`、各 60 秒の中央値。f と g を交互に計測した。

| 回 | 方式 | 右上 fps | 新しいシーンの表示 /s | 重複率 | GPU | CPU | 前面 |
|---|---|---|---|---|---|---|---|
| f | 非同期 | 55.9 | 18.7 | 66.7 % | 87 % | 54 % | 別アプリ |
| f | 同期分割 | 33.3 | 33.3 | 0 % | 97 % | 44 % | AYAstorm |
| g | 非同期 | 32.6 | 10.9 | 66.7 % | 93 % | 29 % | AYAstorm |
| g | 同期分割 | 32.2 | 32.2 | 0 % | 98 % | 47 % | AYAstorm |

| 状態 | 項目 |
|---|---|
| VERIFIED | 同期分割では右上 fps = 新しいシーンの表示で、重複は 0 % |
| VERIFIED | 前面が AYAstorm の回同士で、新しいシーンの表示が 10.9 → 32.2〜33.3/s（約 3 倍）に増えた。右上 fps はほぼ同じ |
| VERIFIED | 同期分割の 2 回で、ログに device lost・VUID・InvalidImageLayout は出なかった（validation layer なし） |
| CLAIMED | 非同期で UI だけのフレームが GPU 時間を使っていた分が、新しいシーンに回ったと見られる |
| OPEN | 群衆なし（軽い場面）での同期分割。ゲートでは軽い場面が遅くなった |
| OPEN | 非同期の f 回は別アプリが前面で、値が他の回と合わない |
| OPEN | validation layer での確認、Windows / Linux での確認、UI だけを操作したときの応答性 |

結論: 群衆ありの GPU 律速の場面では、同期分割は非同期より新しいシーンが約 3 倍多く、右上 fps も体感と一致する。
軽い場面の確認が済むまでは試験用スイッチのままとし、既定は変えない。

### 7.3 右上 FPS を「実際に見える FPS」に変更

この調査の目的は、右上の FPS を見た目のなめらかさと一致させることである。右上の FPS（ステータスバー）は、
`AYAStatusBarShowVisibleFPS`（既定 有効）のとき、新しい 3D シーンが画面に表示された回数/秒を示す。

- `VK_GOOGLE_display_timing` を、対応する device では通常起動でも有効にする。表示完了履歴
  （`actualPresentTime`）と各 Present の `scene_id` を対応付け、直前と異なる `scene_id` が表示された回数を数える。
  古いシーンの再表示と UI だけの更新は数えない。
- リフレッシュ周期を取得できる場合、2 件目以降は直前の計数から周期の 75% 以上経過した履歴を数える。
  MoltenVK 1.4.2 が未表示の drawable に現在時刻を付けるため、短い時刻間隔での過大計数を抑える。
- 拡張が使えない環境では、main swapchain が受理した Present のうち `scene_id` が変わった回数を推定値として示す。
  ツールチップに方式と従来のレンダーループ FPS を表示する。
- 設定を無効にすると従来の表示（レンダーループの FPS）に戻る。
- `VkPerf` の表示完了診断ログは、従来どおり `AYASTORM_VKC=1` と `AYASTORM_PERF_LOG` の指定時だけ出力する。

| 状態 | 項目 |
|---|---|
| VERIFIED | macOS arm64 の通常起動（診断 env なし）で `display_timing enabled=1`。ログイン後 60 秒、device lost・VUID なし |
| VERIFIED | 同じ計数は 7.2 節の `fresh_scene_display_fps` と同じ箇所で数える（群衆ありの非同期で約 11/s、同期分割で約 32/s） |
| OPEN | 右上の表示値を目視で確認すること |
| OPEN | Windows / Linux でのビルドと、各ドライバーでの `VK_GOOGLE_display_timing` の動作 |

コードレビュー（GPT）の指摘のうち、次を修正した。

- 表示完了履歴が欠けると未回収の対応表が上限（4096）に達し、以後の Present を登録できず値が止まる。
  対応表を順序付きにし、上限では最も古い対応を捨てる。
- 履歴照会が `VK_INCOMPLETE` を返したとき、書き込まれた分も捨てていた。
- Vulkan 以外の描画では従来のレンダーループ FPS を表示する。
- XUI の既存ツールチップ（クリックで上限設定を開く案内）を消していた。元の文に説明を追記する形にした。
- 設定の切り替え直後は古い値を出さず、最初の計測まで `--` を表示する。上限到達の色はレンダーループ FPS で判定する。

| 状態 | 項目 |
|---|---|
| VERIFIED | 修正後のビルドで通常起動し、`display_timing enabled=1`、60 秒 device lost・VUID なし |
| CLAIMED | 利用者の目視で、右上の値は見た目のなめらかさと大体一致した（修正前のビルド） |
| VERIFIED | 同梱 MoltenVK 1.4.2 は `MVKImage.mm` の `endPresentation` で、`presentedTime == 0` を現在時刻に置き換える（ソースで確認） |
| VERIFIED | 実機（リフレッシュ周期 16.67 ms = 60 Hz）で `actual_display_fps` が 54〜77 になり、周期を超える「表示完了」が返った。表示されなかった drawable も表示済みとして返っている |
| VERIFIED | 制限により、5 秒ごとに新しいシーン約 115〜155 件のうち 5〜17 件（約 1 割）を除外した（`fresh_capped`、`AYASTORM_VKC=1` の診断 run） |
| CLAIMED | 周期の 75% という閾値は経験則で、置換された時刻の間隔によっては周期を超える計数が一部残りうる |
| OPEN | 履歴の回収が秒をまたいでまとまると、1 秒ごとの値が上下する。表示時刻の窓での集計は未実装 |
