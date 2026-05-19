# AYAstorm 負荷分析レポート

日時: 2026-05-19 03:25 JST
リポジトリ: `/Users/takayukinoami/Desktop/WorkNOW/Firestorm_Develop/phoenix-firestorm-mayatonton`
レポート保存先: `docs/research/ayastorm-load-analysis-2026-05-19.md`
Git 管理: `docs/research/` 配下に移動済み。通常の git 管理対象。

## 概要

現在の負荷は CEF/SLPlugin 子プロセスではなく、メインの `AYAstorm` プロセスが支配的です。観測中、メインプロセス単体でおよそ `54-91%` CPU を使用しており、`ps` の一時点では AYAstorm 系プロセス全体で約 `110%` CPU に達していました。これは CPU 1 コアをほぼ使い切り、瞬間的に追加の処理も走っている状態です。

メモリ圧迫も無視できません。`ps` で見た AYAstorm 系プロセスの RSS 合計は約 `7.6 GB`、`top` ではメイン `AYAstorm` プロセスが約 `7.98 GB` MEM、`ps` では RSS 約 `3.8 GB` でした。システム全体では `31 GB used`、未使用メモリは `453-503 MB` 程度、compressor は約 `9.2 GB` でした。観測時点で swap は出ていませんが、compressor が大きいため、すでに実質的なメモリ圧迫状態です。

ビューアログにも、現在の実行中にフレームレートが不安定になっている痕跡があります。`2026-05-18 18:19-18:23 UTC` ごろ、FPS は何度も `14.8-23.5` まで落ち、その後 `30-52 FPS` まで回復しています。これは起動時だけの負荷ではなく、実行中に発生している性能問題と見てよいです。

## 観測したプロセス状態

メインプロセス:

- PID `43578`: `AYAstorm`
- 実行時間: 約 `03:46:43`
- CPU: `top` で `54.4%`、近いタイミングの `ps` で `90.6%`
- メモリ: `top` で `7977M+`、`ps` で RSS `3978000 KB`
- スレッド数: `35`

AYAstorm 系プロセス全体:

- 対象数: `AYAstorm`、`SLPlugin`、`DullahanHelper` 合計 `47` プロセス
- `ps` 上の CPU 合計: `109.7%`
- `ps` 上の RSS 合計: `7595.9 MB`
- CPU を使っていた主な子プロセス:
  - PID `70179` Dullahan renderer: 約 `4.7-5.3%` CPU、約 `270-281 MB`
  - PID `70174` Dullahan GPU helper: 約 `3.0-3.9%` CPU
  - PID `70166` SLPlugin: 約 `2.1-2.7%` CPU
  - 古い Dullahan helper の大半はほぼ idle

CEF plugin グループが複数残っています:

- 起動初期から残っている `SLPlugin` 子プロセスが 4 つ、実行時間は約 `03:45:48`
- 実行時間約 `01:30:48` の `SLPlugin` 子プロセスが 1 つ
- 比較的新しい実行時間約 `09:37` の `SLPlugin` 子プロセスが 1 つ

これらは CPU 主因ではありません。ただし、数百 MB 単位のメモリ、スレッド数、時々の CPU 使用を積み増しています。メディアや UI パネルを閉じたあとも古い CEF インスタンスが残る設計なのかは確認した方がよいです。

## ログから見えたこと

対象ログ: `/Users/takayukinoami/Library/Application Support/Firestorm/logs/AYAstorm.log`

現在実行中のフレーム関連ログ:

- `2026-05-18T18:19:27Z`: FPS `18.50`
- `2026-05-18T18:19:37Z`: FPS `16.20`
- `2026-05-18T18:19:41Z`: 直近 600 秒の最低フレームレート `16.0`; resident memory `4238320 KB`
- `2026-05-18T18:19:57Z`: FPS `15.60`
- `2026-05-18T18:20:27Z`: FPS `14.80`
- `2026-05-18T18:21:27Z`: viewer stats packet では平均的な値として `fps: 60.5805` が出ているが、同じ時間帯の即時 display FPS は `23.50`
- `2026-05-18T18:23:27Z`: FPS `25.50`
- `2026-05-18T18:23:37Z`: FPS `30.60`

メモリ関連ログ:

- `2026-05-18T18:16:27Z`: viewer stats `mem_use: 4.7163e+06`
- `2026-05-18T18:19:41Z`: resident memory `4238320 KB`
- `2026-05-18T18:20:01Z`: resident memory `3436160 KB`
- `2026-05-18T18:21:35Z`: allocated physical memory `3493.17 MB`
- `2026-05-18T18:23:35Z`: allocated physical memory `1735.44 MB`

レンダリング/バックエンド関連:

- Renderer: `Apple M2 Pro`
- OpenGL パス: `GL_VERSION: 4.1 Metal - 89.4`
- reported hardware concurrency: `10`
- reported RAM: `33554432 KB`
- reported VRAM field: `21845`

目立つログパターン:

- 実行中の早い段階で `GL Error happens before reading back texture. Error code: 1282` が繰り返し出ています。
- `SLPlugin` の起動と終了が複数回あり、現時点のプロセスツリーにも複数の plugin グループが残っています。
- 最新の FPS 低下付近で HTTP capability timeout が繰り返されています。ただし、これだけでメインプロセスの CPU 使用を説明するのは難しいです。
- HUD detail ログでは、以前のタイミングで HUD object/texture cost が高めに出ています。現在のシーンやアバター状態によっては、HUD 描画負荷が寄与している可能性があります。

## システム負荷の状態

`top` と `vm_stat` からの観測:

- Load average: 約 `2.78-3.87`
- 観測中の CPU idle: 約 `61-75%`
- 物理メモリ: `31G used`
- free/unused memory: `453-503 MB`
- compressor: 約 `9.2 GB`
- 観測時点の swapins/swapouts: `0`
- `vm_stat` の累積 pageouts: `331693`

解釈としては、マシン全体が CPU 飽和しているわけではありません。ただし AYAstorm が約 1 コア分を使い続けており、メモリはかなり詰まっています。ブラウザ、ビルド、エディタなどの追加負荷が乗ると、swap が出る前でも compression/decompression によって体感遅延が増える可能性があります。

## 原因候補

1. メインビューアの render/update loop が主な CPU 使用源です。
   観測した CPU の大半はメイン `AYAstorm` プロセスが占めています。CEF 子プロセスは副次的です。

2. メモリ圧迫が体感の重さを増幅しています。
   AYAstorm と CEF helper の合計 RSS は `ps` 上で約 `7.6 GB` あり、システムの空きメモリは非常に少なく、compressor も大きい状態です。

3. CEF/SLPlugin の lifetime が余分な負荷を足している可能性があります。
   複数の plugin グループが数時間残っています。ほぼ idle ではありますが、メモリ、スレッド、時々の CPU を消費します。

4. 現在の OpenGL-on-Metal パスは、シーンや UI の負荷に敏感です。
   ログ上は native Metal renderer ではなく `OpenGL 4.1 Metal - 89.4` です。texture readback 周辺の GL エラーと FPS 低下があるため、まず render path と texture/media workload を優先してプロファイルすべきです。

## 次に確認すべきこと

1. メディア無効、または全 media panel を閉じた状態で再現する。
   期待する確認点は、`SLPlugin`/`DullahanHelper` プロセス数と RSS が減るかどうかです。FPS が安定するなら、CEF/media の lifetime や preload 挙動を追う価値があります。

2. 同じシーンで HUD を外す、または無効化して再現する。
   ログ上は HUD object/texture count が高い場面がありました。CPU/FPS が改善するなら、アバター HUD 描画が主要因の一つです。

3. Instruments Time Profiler で PID `43578` を採取する。
   CLI の `sample` はこの sandbox セッションでは正常終了せず、停止しました。Instruments なら、hot time が draw traversal、avatar update、texture upload/readback、UI/media、frame pacing のどこにあるか確認できます。

4. Activity Monitor の GPU History、または Instruments の Metal/OpenGL テンプレートで GPU 側も見る。
   CPU はシステム全体では飽和していません。重要なのは、メインスレッドが CPU-bound なのか、driver-bound なのか、GPU/GL 同期待ちなのかです。

5. このシーン専用の baseline note を残す。
   記録すべき項目:
   - viewer location/scene
   - graphics preset と draw distance
   - FPS min/current
   - main process CPU/RSS
   - SLPlugin/Dullahan process count
   - media と HUD の有効/無効

## 使用したコマンド

```sh
ps -axo pid,ppid,pcpu,pmem,rss,vsz,etime,state,comm
top -l 2 -s 2 -o cpu -stats pid,command,cpu,mem,threads,state,time
vm_stat
ls -lt /Users/takayukinoami/Library/Application\ Support/Firestorm/logs
rg -n -i 'FPS:|FrameWatcher|Current allocated|Basic resident|GL Error|readBackRaw|HUD textures|Dullahan|CEF|SLPlugin|Texture Cache|GL_RENDERER|GL_VERSION|Memory info' /Users/takayukinoami/Library/Application\ Support/Firestorm/logs/AYAstorm.log
```
