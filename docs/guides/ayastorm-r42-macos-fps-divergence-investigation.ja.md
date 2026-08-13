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
