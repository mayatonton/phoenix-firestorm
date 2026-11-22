# AYAstorm R42 ローカル OpenSim ベンチ（busy store）

## 目的

macOS / MoltenVK 版 AYAstorm の最適化を、毎回同じシーン・同じ条件で比較するための
ローカルグリッドを用意する。Second Life 本番は混雑やアセットが日々変わるため、
ビルド間の比較には使わない。

シーンは、wgpu 系ビューワーの macOS GPU 調査で使われた「混んだ店」デモベンチと
同じ構成を OpenSim 上に再現する。

| デモベンチの要素 | このベンチでの再現 |
|---|---|
| 広場に向けた静的ミラー | 3 × 2 m の静的ミラー（reflection probe + mirror flag）を視点の 12 m 先、床から 2.5 m に置く |
| アバター 40 体の群衆 | OpenSim NPC 40 体。各体が 7 prim のリンクセットを 8 か所（頭・胸・両肩・両手・両足）に装着 |
| ローカルライト 64 個 | 直径 6 cm の光る球 64 個。頭の高さに 1.6 m 間隔、ローカルライト半径 3 m |
| カーテン 12 枚と光る紐 12 本 | 14 × 8 m のレースのカーテン 12 枚（1 m 間隔、4 枚に 3 枚は視線を横切る）と光る紐 12 本（80 % 不透明） |

描画方式が違うため、wgpu 系ビューワーと AYAstorm の数値は直接比較しない。このベンチは
AYAstorm のビルド同士を同じ条件で比べるための固定シーンである。

- 対象 app: `build-darwin-dev/newview/Release/AYAstorm.app`（OpenSim 対応 `OPENSIM=ON` の arm64 開発ビルド）
- 開発用 profile: `~/Library/Application Support/AYAstorm-dev/`（build tree 内の app が自動で使う）
- ベンチ環境の置き場所: `OPENSIM_BENCH_DIR`（既定 `~/OpenSimBench`、repo の外）

## 状態

| 項目 | 状態 | 根拠 |
|---|---|---|
| OpenSim 0.9.3.0 が arm64 の .NET 8 で起動し、ログインできる | VERIFIED | 2026-11-22、M2 Pro。`STATE_STARTED` まで到達 |
| OAR の読み込み（91 objects、asset 4 個、地形） | VERIFIED | `Successfully loaded archive` |
| カーテン・紐・ライトの表示 | VERIFIED | ビューワーのスクリーンショット |
| NPC 40 体の生成と 8 か所の装着物 | VERIFIED | `Complete movement of Crowd 1..40`、拡大画像で装着物を確認 |
| ミラーが実際に映り込むこと（`RenderMirrors 1`） | OPEN | 表示は暗い板のみ。映り込みは未確認 |
| Agent Pilot による経路の記録と再生 | OPEN | 未着手 |
| 計測手順（設定・ウィンドウサイズ・読み取る値） | OPEN | 未確定 |

上の確認は、`build-darwin-universal/newview/Release/AYAstorm.app`（2026-08-14 ビルド、arm64、
`OPENSIM=ON`）で行った。現在の HEAD の `build-darwin-dev` ビルドでは未確認である。

## 1. 一度だけ行うセットアップ

### 1.1 .NET 8 runtime と OpenSim

```bash
export B="$HOME/OpenSimBench"
mkdir -p "$B/downloads"
curl -fsSL -o "$B/downloads/dotnet-install.sh" https://dot.net/v1/dotnet-install.sh
bash "$B/downloads/dotnet-install.sh" --channel 8.0 --runtime dotnet --install-dir "$B/dotnet" --no-path
curl -fsSL -o "$B/downloads/opensim-0.9.3.0.tar.gz" http://opensimulator.org/dist/opensim-0.9.3.0.tar.gz
tar -xzf "$B/downloads/opensim-0.9.3.0.tar.gz" -C "$B" && mv "$B/opensim-0.9.3.0" "$B/opensim"
```

2026-11-22 に取得した `opensim-0.9.3.0.tar.gz` の SHA-256 は
`169421336640c5a10d83826443160ad177e4185c51fb69107f64b327b52287f2`。

### 1.2 ネイティブライブラリとツール

```bash
brew install mono-libgdiplus openjpeg
ln -sf /opt/homebrew/lib/libgdiplus.dylib "$B/opensim/bin/libgdiplus.dylib"
```

- `libgdiplus` がないと、地図画像の生成で `DllNotFoundException` が出て起動途中で終了する。
- `DYLD_FALLBACK_LIBRARY_PATH` は使わない。`nohup` など SIP 保護下のバイナリを経由すると
  消えるため、`bin/` に symlink を置く。
- `openjpeg` の `opj_compress` は OAR 生成時のテクスチャ変換に使う。

### 1.3 OpenSim の設定

`$B/opensim/bin/` で次を設定する（いずれも OpenSim 0.9.3 の既定ファイルを編集）。

1. `Regions/Regions.ini` に 1 リージョンだけを置く。

   ```ini
   [BenchRegion]
   RegionUUID = <uuidgen で生成>
   Location = 1000,1000
   InternalAddress = 0.0.0.0
   InternalPort = 9000
   AllowAlternatePorts = False
   ExternalHostName = 127.0.0.1
   ```

2. `OpenSim.ini` の `[Estates]` で `DefaultEstateName`、`DefaultEstateOwnerName`、
   `DefaultEstateOwnerUUID`、`DefaultEstateOwnerEMail`、`DefaultEstateOwnerPassword` を
   有効にする。初回起動時に、このアカウントがエステートオーナーとして作られる。
   これがベンチ用アバターになり、群衆スクリプトの所有者にもなる。
3. `OpenSim.ini` の `[Startup]` で `physics = ubODE` と `meshing = ubODEMeshmerizer` を有効にする。
4. `OpenSim.ini` の `[NPC]` に `MaxNumberNPCsPerScene = 64` を追加する。既定の 40
   ちょうどでは、40 体目が作られないことがあった。

アカウント情報は `$B/bench-credentials.txt`（パーミッション 600）に置く。
`launch_bench_viewer.sh` はここから読み込む。

```text
first=Bench
last=Tester
password=<ランダムな値>
```

`OpenSim.ini` の `http_listener_port` 9000 は全インターフェイスで待ち受ける。macOS のファイアウォールで
受信の許可を求められたら、拒否してもローカルからの接続には影響しない。

## 2. ベンチの起動

```bash
scripts/bench/generate_busy_store_oar.py --out "$B/oar/busy_store.oar"
scripts/bench/opensim_bench.sh start
scripts/bench/opensim_bench.sh load "$B/oar/busy_store.oar"
scripts/bench/launch_bench_viewer.sh build-darwin-dev/newview/Release/AYAstorm.app
```

- `generate_busy_store_oar.py` は同じ入力から毎回同じ OAR を書き出す。asset の UUID は
  中身のハッシュから決める（OpenSim は既存の UUID の asset を上書きしないため）。
- `opensim_bench.sh` は OpenSim を `-console=basic`（TTY を使わず標準入力から 1 行ずつ読む）で
  起動する。既定のローカルコンソールは TTY がないと CPU 100 % で空回りし、`screen` 経由では
  ログの書き出しで止まった。コマンドは `opensim_bench.sh cmd '<command>'` で送る。
- `launch_bench_viewer.sh` はグリッドを `--grid http://127.0.0.1:9000/` で指定し、
  開始位置（`secondlife://BenchRegion/128/92/23`）でログインして in-world まで待つ。
  第 2 引数以降はビューワーにそのまま渡す（例: `--set RenderMirrors 1`）。
- 群衆はログイン後に作られる。群衆スクリプトはオーナーがリージョンに入ってから 30 秒待ち、
  外見を保存したうえで NPC 40 体を作る。進み具合は `opensim-console.log` の
  `Complete movement of Crowd N` で確認できる。

停止は次のとおり。

```bash
scripts/bench/opensim_bench.sh stop
```

### 2.1 要素を外した A/B 比較

`--omit` に `veils`、`mirror`、`lights`、`crowd` をカンマ区切りで指定すると、
その要素を外した OAR を作れる。

```bash
scripts/bench/generate_busy_store_oar.py --out "$B/oar/busy_store_noveils.oar" --omit veils
```

## 3. シーンの構成

ベンチの視点は (128, 96) から北（+Y）を向く。地形は 21.5 m の平地、広場の床面は 22.0 m。

| オブジェクト | 位置・数 |
|---|---|
| `bench plaza` | 96 × 96 m の石畳。リージョンの中央 |
| `bench veil 00`〜`11` | y = 98.5〜109.5。4 枚に 1 枚は縦向き（90°）か斜め（35°）。alpha blend |
| `bench string 00`〜`11` | 高さ 7 m の円柱。fullbright、glow 0.25 |
| `bench mirror` | (128, 108, 24.5)。ミラーの法線はプリムのローカル +Z（`llheroprobemanager.cpp`）で、-Y 向きに回転させる |
| `bench light 00`〜`63` | 8 × 8 格子。x = 122.4〜133.6、y = 105〜116.2、高さ 23.6 m |
| `bench crowd controller` | (140, 88)。`busy_store_crowd.lsl` と `bench wearable` を中に持つ |
| NPC `Crowd 1`〜`40` | 8 × 5 格子。y = 110〜119.6、-Y 向き |

NPC はインベントリを持たないため、プリムの中身から装着させること
（`osForceAttachToOtherAvatarFromInventory`）はできない。群衆スクリプトは、オーナーの外見を
ノートカードに保存し、その `attachments` 配列に `bench wearable` の asset を 8 か所分書き足す。
NPC はこのノートカードから作られ、OpenSim が asset から装着物を生成する
（`AttachmentsModule.RezAttachments`）。

チャンネル 7 でオーナーが `crowd`（作り直し）、`clear`（削除）、`appearance`（外見の再保存）を
送ると、群衆を操作できる。

## 4. 注意事項

- **`--loginuri` は使わない。** OpenSim 版の AYAstorm は `--loginuri` を読まず、
  `--grid` がないと Second Life 本番にログインしようとする。テスト用のアカウント情報が
  本番に送られるので、必ず `--grid` を付けるか `launch_bench_viewer.sh` を使う。
- ビューワーや OpenSim を強制終了した直後のログインは、古いセッション（`reason: presence`）を
  理由に 1 回だけ拒否される。OpenSim はこの拒否で古いセッションを片付けるので、
  `launch_bench_viewer.sh` は 1 回だけ再試行する。
- ローカルグリッドには音声サーバーがない。ログインのたびに `SLVoice` のクラッシュレポートが
  `~/Library/Logs/DiagnosticReports/` に残る。原因は未確認（OPEN）。
- ログイン直後に通貨ポータルの案内ダイアログが出る。計測の前に閉じる。
- `RenderMirrors` の既定値は 0。featuretable でも、オンになるのは最上位のグラフィック設定だけである。
  ミラーを含めて計測するときは、使った設定を記録する。
- 製品相当の計測では、`VK_ICD_FILENAMES`、`VK_LAYER_PATH`、`DYLD_LIBRARY_PATH`、
  `DYLD_INSERT_LIBRARIES`、`AYASTORM_VKCMD_MEMO` を設定しない。`launch_bench_viewer.sh` は、
  これらが設定されていると警告を出す。
