# AYAstorm R42 表示完了・3D scene freshness 診断の必要性

## 1. 結論

AYAstorm の内部 FPS と、ディスプレイ上で新しい 3D scene が表示される FPS が一致しているかを判定するには、
**Metal drawable の表示完了時刻と、その表示に使用した 3D scene の世代を対応付ける診断が必要**である。

従来のログは、Viewer の frame loop、3D scene producer の submit、swapchain image の acquire、
`vkQueuePresentKHR()` の受理までを観測できる。しかし、次の 2 点を観測できない。

1. Present した drawable が macOS の表示系で実際に表示完了した時刻
2. その表示が新しい 3D scene だったか、直前の scene の再表示だったか

この 2 点が無いままでは、内部 FPS が高いときに、実際の 3D 表示も同じ FPS で更新されているとは証明できない。

2026-08-14 時点で診断コードの実装と Viewer build は完了している。ただし、以下の実測値はまだ得ていないため、
実機 run と `AYAstorm.log` の確認までは OPEN とする。本書は、その診断を入れる理由、証拠の境界、ログの判定方法を定める。

## 2. 解決したい問い

最終的に確認したいのは、単なる Present 要求数ではなく、次の一致である。

```text
Viewer の内部 frame cadence
    ≒ 実際に表示完了した drawable cadence
    ≒ 新しい 3D scene が表示完了した cadence
```

たとえば、内部 frame が 30 fps でも、3D scene が 10 fps でしか更新されず、同じ scene を 3 回ずつ
Present している場合、画面の動きは 10 fps 相当になる。Present 要求を 30 回受理したという情報だけでは、
この重複を判定できない。

## 3. 区別しなければならない 4 つの cadence

| cadence | 代表指標 | 意味 | 証拠範囲 |
| --- | --- | --- | --- |
| Consumer | `fps` / `consumer_fps` | UI 合成と Present 用 frame loop の回数 | 観測可能 |
| 3D Producer | `producer_fps` | async scene command buffer を PE へ投入した回数 | submit 回数のみ観測可能 |
| WSI Present | `present_call_fps` / `present_ok_fps` | `vkQueuePresentKHR()` の呼出・受理回数 | 観測可能 |
| 実表示 | `actual_display_fps` | Metal drawable が実際に表示完了した回数と時刻 | 今回の診断有効 run で観測する |

`producer_fps` は GPU 完了回数、front 反映回数、実表示回数ではない。現在は
`sAsyncProducerSubmitCount` の増分を数えているため、名称だけから「新しい scene の完成 FPS」と解釈してはならない。

`present_ok_fps` も実表示 FPS ではない。これは Vulkan/MoltenVK が Present 要求を受理したことを示すが、
drawable の表示完了を示さない。

## 4. 従来の診断だけでは原因を決定できない理由

従来の値から、Consumer と Producer の submit cadence が乖離していること、および acquire / Present 受理が
Consumer cadence に追従していることまでは確認できる。しかし、次の異なる状態が同じようなログになる。

### 状態 A: 同じ scene の再表示

```text
Consumer 30 fps / Present 受理 30 fps / 新しい 3D scene 10 fps
```

同じ scene を複数回表示している。内部 FPS は高いが、3D の動きは低い。

### 状態 B: Present 後の表示経路も低下

```text
Consumer 30 fps / Present 受理 30 fps / drawable 表示完了 10 fps
```

AYAstorm は Present を要求しているが、Metal/macOS の表示完了 cadence が低い。

### 状態 C: 全 cadence が一致

```text
Consumer 30 fps / drawable 表示完了 30 fps / 新しい 3D scene 表示完了 30 fps
```

内部処理と新しい 3D 表示が一致している。目標とする状態である。

従来のログは A、B、C を分離できないため、性能修正の対象を決める最終証拠にならない。

## 5. Metal 表示完了を取得する経路

AYAstorm は `CAMetalDrawable` を直接所有しない。drawable の取得と Present は MoltenVK が管理しているため、
AYAstorm 側から直接 `addPresentedHandler` を登録する設計にはしない。

同梱 MoltenVK 1.4.2 は、内部で `MTLDrawable.addPresentedHandler` を使用し、
`MTLDrawable.presentedTime` を取得している。この結果は `VK_GOOGLE_display_timing` を通して Vulkan アプリへ
公開される。

AYAstorm は次の正式な Vulkan 境界を使用する。

1. device extension 列挙で `VK_GOOGLE_display_timing` の利用可否を確認する。
2. 利用可能な診断起動でのみ extension を有効化する。
3. main swapchain の各 Present に一意な `presentID` を付ける。
4. `vkGetPastPresentationTimingGOOGLE()` から `actualPresentTime` を回収する。
5. `presentID` と、その Present が使用した `scene_id` を対応付ける。

MoltenVK の改造や private API への依存は必要ない。

## 6. `scene_id` が必要な理由

Metal の表示完了回数だけを数えても、3D scene の滑らかさは証明できない。同じ front image を 30 回表示した場合も、
drawable の表示完了は 30 回になるためである。

新しい async Producer が GPU 完了し、front の scene render target が切り替わった時だけ、単調増加する
`scene_id` を更新する。各 Present は使用した `scene_id` を保持する。

表示完了履歴を回収したとき、直前に表示完了した `scene_id` と比較する。

- `scene_id` が変化した: 新しい 3D scene の表示完了
- `scene_id` が同じ: 直前の 3D scene の再表示
- 対応表に存在しない: 証拠不足として unknown。新しい scene と推測しない

この対応により、「実表示 FPS」と「新しい 3D scene の実表示 FPS」を別々に測定できる。

## 7. 必須のログ項目

診断ログは既存の `AYASTORM_PERF_LOG` 集計区間に合わせ、少なくとも次を出力する。

```text
display_timing_available
display_timing_source
actual_display_fps
fresh_scene_display_fps
duplicate_scene_count
duplicate_ratio
unknown_present_count
display_timing_history_query_errors
display_timing_history_last_result
present_margin_ms_p50
present_margin_ms_p95
present_margin_ms_max
```

比較のため、同じ区間の既存値も併記または隣接行で参照できるようにする。

```text
fps                  # Consumer cadence の既存キー
producer_fps         # Producer submit cadence の既存キー
present_ok_fps
```

`fresh_scene_display_fps` は、実表示完了履歴のうち `scene_id` が変化した回数だけから算出する。
`duplicate_ratio` の分母は、`scene_id` を確定できた fresh と duplicate の合計とする。unknown は分母へ入れず、
別項目で残す。

`actual_display_fps` は、連続する `actualPresentTime` の差分から算出する。present margin の分位値には、
MoltenVK が同じ timing 経路で返す `presentMargin` を使用する。AYAstorm の `steady_clock` と
`actualPresentTime` を直接減算してはならない。両者が同じ clock domain であることを
Vulkan API は保証しないためである。

## 8. 診断の有効条件

表示完了診断は通常動作へ影響させない。次の両方が指定された起動だけで有効化する。

```text
AYASTORM_VKC=1
AYASTORM_PERF_LOG=<正の秒数>
```

通常起動では、表示 timing extension の有効化、Present ID 対応表、履歴照会、追加ログ出力を行わない。

この条件が制御するのは、今回追加する**表示完了診断**である。`AYASTORM_PERF_LOG` だけで既に出ている
従来の性能ログまで `AYASTORM_VKC` 必須へ変更するものではない。両方を指定した診断 run では、
表示完了の集計も通常の Viewer ログである `AYAstorm.log` へ記録する。

extension、関数ポインタ、swapchain のいずれかが利用できない場合は、次のように明示する。

```text
display_timing_available=0
actual_display_fps=n/a
fresh_scene_display_fps=n/a
```

`vkQueuePresentKHR()` の成功回数から実表示完了を推定してはならない。

## 9. 実装上の不変条件

1. main swapchain だけを対象とし、aux window の Present を混ぜない。
2. `presentID` と `scene_id` は単調 ID とし、wrap と再利用を安全に扱う。
3. swapchain 再生成時に古い対応表を誤って新 swapchain の履歴へ結び付けない。
4. 表示完了履歴は非同期に遅れて到着する前提で扱う。
5. 対応表は有界とし、履歴が返らない場合も無制限に増やさない。
6. 表示完了を待つ blocking wait を追加しない。PE thread から返却済み履歴だけを poll する。
7. `actualPresentTime` が返った履歴だけを実表示完了として数える。
8. 同じ `scene_id` の再表示を fresh scene に数えない。
9. 既存の `VK_KHR_present_wait` 診断と共存させる。
10. 診断値は性能制御、pacing、Present 分岐に使用しない。

## 10. 診断コード導入後の判定表

| Consumer | 実表示 | fresh 3D 表示 | 判定 |
| --- | --- | --- | --- |
| 高い | 高い | 低い | 同じ 3D scene を再表示。Producer または scene 更新制御を調査する。 |
| 高い | 低い | 低い | Present 後の Metal/macOS 表示経路を調査する。 |
| 低い | 低い | 低い | Viewer frame loop または Producer 待機を調査する。 |
| 高い | 高い | 高い | 内部処理、新しい 3D scene、実表示が一致している。 |

この判定を得た後に、Producer の直列記録、shadow per-layer command buffer、GPU 実行時間、
Present pacing のどこを修正対象にするか決める。

## 11. 受入条件

### VERIFIED とできる条件

- 診断無効起動で追加の extension、履歴照会、追加ログが動かない。
- 診断有効起動で利用可否と timing source が明示される。
- `actual_display_fps` は `actualPresentTime` を持つ履歴だけから算出される。
- present margin の p50 / p95 / max は履歴の `presentMargin` から算出され、異なる clock domain の時刻を直接減算しない。
- `fresh_scene_display_fps` は `presentID → scene_id` の対応と scene 変化だけから算出される。
- duplicate と unknown が別集計され、unknown を fresh と推定しない。
- swapchain 再生成と終了時に対応表が安全に処理される。
- aux window の値が main window の表示 FPS に混入しない。

### OPEN のまま残すもの

- 計測前の実際の `actual_display_fps` と `fresh_scene_display_fps`
- Producer が次の consumer frame までに完成しない内部ボトルネック
- shadow per-layer record 並列化が Producer FPS を改善する量
- ディスプレイパネルの発光を外部センサーで測る物理計測

## 12. 非目標

この診断コードは性能修正ではない。次は本診断の範囲外である。

- UI/scene async の有効・無効を変更すること
- `FRAMES_IN_FLIGHT` や 1-in-flight 制御を変更すること
- shadow per-layer command buffer を実装すること
- Present mode や VSync 設定を変更すること
- macOS のメイン画面設定を原因と断定すること

診断の目的は、修正を先に決めることではなく、内部 FPS、新しい 3D scene、実表示のどこで cadence が
分岐しているかを、推測ではなく同一 run の一次情報で確定することである。
