# 検知装置アーキテクチャ(描画不体裁の機械検知体制)— 装置設計 v1

## ⚡ 担当者向けクイックリファレンス(まずここだけ読めば使える)

**鉄則: 描画がおかしい時、コードを推理する前に装置に聞く。装置が犯人を名指しするまで fix を書かない。**

1. **診断起動**: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 ~/ayastorm/ayastorm`(通常起動では装置はほぼ無音・コストゼロ。`AYASTORM_VKC=1` で全装置 ON)
2. **症状を再現**(移動なら移動・外見編集なら外見編集)→ 終了 → `~/.ayastorm_x64/logs/AYAstorm.log` の `VKC` 行を読む。
3. **行の読み方**:
   - `VKC-SUM 10s skips=… cause{…}` — 10 秒毎の失敗全集計。**ここに出ない失敗種は存在しない**(出ないのに症状がある = 装置のギャップ → 装置を伸ばす。fix 仮説に行かない)。
   - `VKC skip cause=… obj=… tex=… av=…` — draw 不発火とその原因・対象(L1)。
   - `VKC geoab_* attr=… maxdiff=… verdict=… obj=…` — worker 幾何出力と旧経路の byte 照合(L3)。**verdict=src** = カーネル無罪・入力データが worker 読取り後に変わった / **verdict=kernel** = カーネルの数式バグ / `geoab_input_drift fields=0x…` = stage↔apply 間で入力パラメータが変化。
4. **fix を書いたら**: 同じ診断起動で該当 cause が 0 になること + 全層沈黙が機械検収。視覚確認はその後の最終 gate。

実例(2026-07-18): 「カメラ移動で透明の穴」— 前任は視覚と推理で 6 連打・全外し。装置導入後、起動 2 回で `geoab_source_drift attr=pos maxdiff=4.8`(= worker が変異中の volume を読む)まで機械確定した。

---

- 発注: AYA(2026-07-18)「あまりにも簡単に描画不体裁が起きる。ソースは正しいはず→おかしい→写真見せろ→これでどう? のあてずっぽうループを何とかしないとプロジェクトは前に行かない。このためなら 1 週間払う価値がある」
- 目的: **描画不体裁を AYA の目でなく装置が identity 付きで名指しする体制**。全バグ検知ではない。「簡単に起きる族」に恒久の網を張り、あてずっぽうループを構造的に不可能にする。
- 先行実証: LLVKContract v1.2(`indra/llrender/llvkcontract.{h,cpp}`)は「描かない」族(髪消失・ちらつき)を起動 1 回で名指し・起動 1 回で検収した。本設計はその守備範囲を「消えた」「間違って描いた」へ拡張する。

## 0. 統治規律(装置と同格の成果物)

1. **装置が犯人を名指しするまで fix を打たない。** AYA からの不体裁報告は「fix せよ」の入力ではなく「装置に穴がある」の報告。log が沈黙していたら、先に伸ばすのは装置であって fix 仮説ではない。
2. **AYA 起動は 2 種のみ**: ①装置が名指しした後の fix 検収 ②装置拡張の稼働確認。「これでどう?」起動は禁止。
3. **成功基準**: AYA が異常を 1 回報告 → 次の起動 1 回で log が対象(obj/face/frame/原因段)を名指しする。できなければ装置側の欠陥として扱う。
4. **gate 基準の拡張**: 従来「視覚同一 + validation 0」に「**標準ストレス走行(カメラ移動・TP・LOD 嵐)下で全層オラクル沈黙**」を追加。オラクルなき経路移行は受け入れない(移行 = L3 の A/B 参照実装を伴うこと)。

## 1. 装置の階層

| 層 | 名 | 守備範囲(症状族) | 状態 |
|---|---|---|---|
| L1 | 発火契約(LLVKContract v1.2) | draw が発火しない / fallback で描いた / fire-skip 振動 | **既設・実証済** |
| L3 | 内容オラクル(移行 A/B 照合) | **経路移行後のカーネルが同一入力で違う内容を書く / 入力が非同期窓で drift** | 本設計・移行ごと(第 1 号 = T2 fill カーネル) |
| L4 | 束縛 checksum(誤 texture/UBO 内容) | 誤 lighting・誤色(描かれるが状態が違う) | 未設計・必要になった時点で起工 |

L1/L3 で「描かない(L1)/違う内容を描く(L3)」を覆う。本プロジェクトで実際に湧いた不体裁の多くはこのどれかに落ちる(髪消失=L1・ちらつき=L1・E2 白=L1 fallback 計上・誤 lighting=L4 予備)。

## 3. L3: 内容オラクル第 1 号 = T2 fill カーネル A/B 照合

### 3.1 守る不変条件
**「経路移行は同一入力に対して同一出力を書く」**。worker 経路 = `LLFace::buildVkGeoFill/runVkGeoFill`(llface.cpp:2170/2668)、参照 = 旧 `LLFace::getGeometryVolume`(llface.cpp:1129)。両者は同じ SIMD 系譜(index 転記・xform4a・planarProjection 等が同形)で、同一入力なら byte 同一が期待できる作り。

### 3.2 機構
実行点 = `applyGeoStaged` 成功直後(llvovolume.cpp:5766・main thread・fields 適用済・buffer 内容 = worker 出力)。標本 face(既定: job 毎 max 4 face・hash 回転で face クラスを巡回・`AYASTORM_GEOAB=full` で全数、`=0` で OFF)に対し:

1. **入力 drift 検出**: 現時点の入力で `buildVkGeoFill` を再実行(scratch LLGeoFaceFill)→ stage 時に保存した fill の param 部(mMatVert/mMatNormal/mTC[]/mColorRGBA/… dst ポインタ以外)と比較。不一致 → `geoab_input_drift field=<bitmask>` = 非同期窓で入力が動いた族。
2. **カーネル照合**: worker 出力領域(mDst* 各属性 × mNumVertices/mNumIndices 分)を scratch へ copy → `getGeometryVolume(force_rebuild=true)` を実行(旧経路が同領域へ書く)→ 属性毎 memcmp。不一致 → `geoab_kernel_mismatch attr=pos|norm|tc0|tc1|tc2|color|emissive|tangent|weights|index bytes=N first_off=… maxdiff=…` + face provenance(obj LocalID・face idx・flags: rigged/planar/texgen/bump/texanim/texmat・LOD・num_verts)。
3. 実装形(2026-07-18・設計から改良): 旧経路の strider 書込み先は mega slice でなく **staging バッファ**(`mMappedData`)と判明。よって「worker 出力(mega slice)vs 旧経路出力(staging)」を別メモリ同士で直接比較でき、**表示内容は一切置換されない純観測**になった。staging の stale 内容による偽陽性は「照合前に worker 出力を staging へ prime」で殺す(旧経路が書かない領域は prime 値のまま = 無音)。照合後は MappedRegion 簿記を空に戻し flush を発生させない(A/B 前に簿記が非空の buffer は照合自体を skip = 安全側)。

このため stage 時の `LLGeoFaceFill` を apply まで保持する(現行 `LLGeoStagedRebuild::mFills` は既に job に同居 = 追加保持コストなし)。

### 3.3 罠(実装時に必ず踏むトレース)
- `getGeometryVolume` の副作用(setState(GLOBAL)・mTexExtents 等)は正規 rebuild と同一の副作用 = 安全側。ただし GLTF selected 分岐(llface.cpp:1216 の再帰)は標本から除外(`tep->isSelected()` は skip)。
- 比較は byte 厳格から始め、maxdiff レポートで 1-ulp noise と実バグを仕分ける。noise 実在が判明した属性のみ epsilon 化(申告の上)。
- worker が同 fill を apply 後に再度触らないこと(現行 publish→apply で終端 = 触らない)を実装時に再確認。

### 3.4 これが鳴ると分かること
- `geoab_kernel_mismatch attr=tc0 texgen=planar rigged=0 …` = 939 行カーネルの特定 face クラスの内容バグが属性名・face クラス付きで名指し(前任の最有力仮説の直接検証)。
- `geoab_input_drift` = 非同期窓の入力ずれ族。
- 沈黙 = カーネル内容は白(仮説を 1 起動で棄却できる — これも成果)。

## 4. 穴バグへの初適用(本体制の実証運転)

1. L3 を実装・ビルド・3-path deploy(fix 仮説は一切実装しない)。
2. AYA 起動 1 回(カメラ移動で穴を再現 → 数分 → 終了 OK)。log = `~/.ayastorm_x64/logs/AYAstorm.log`。
3. 判定表:

| log 出力 | 確定すること | 次の一手 |
|---|---|---|
| L3 `geoab_kernel_mismatch` | カーネル内容バグ(属性・face クラス名指し) | 当該属性の kernel 差分を旧コードと突合して fix |
| L3 `geoab_input_drift` | 非同期入力窓 | stage 時 snapshot の設計是正 |
| 全装置沈黙のまま穴再現 | 症状族が 3 層の外(装置ギャップ) | 網を拡張(fire-level 連続性 or L4)— fix 仮説には行かない |

4. 名指し → fix → 起動 1 回で検収(オラクル沈黙 + AYA 視覚)。

## 4.5 運転モード(AYA 要望 2026-07-18 夜・初回起動で「重い」判明)

- **通常起動(既定)**: 装置は counting + `VKC-SUM`(1 行/10s・失敗ゼロなら無音)のみ。per-event WARNS・L3 A/B は全 OFF ≒ 追加コストほぼゼロ。
- **診断起動**: `AYASTORM_VKC=1` を付けて起動(例: `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 ~/ayastorm/ayastorm`)→ 全装置 full。`AYASTORM_GEOAB=full/0` は L3 の明示上書き。
- **判別実験モード**: `AYASTORM_GEOAB=repair` = L3 を全数化し、不一致検出時に参照出力(旧経路の正内容)をその場で buffer へ反映(`repaired=1` を log)。**穴が repair で消える → 幾何内容(source drift 族)が犯人と機械確定 / 消えない → 幾何は無罪確定**。適用範囲 = 入力 drift なし面のみ(volume 差替え面はサイズ危険のため非修復・申告)。診断専用(恒久 fix ではない)。
- `AYASTORM_PERF_LOG` に束ねない理由: AYA が常用するフラグのため「取る/取らない」の分離が成立しない。

## 4.6 初回運転の結果(2026-07-18 夜・穴再現起動 1 回)

3 層全て発報・skips=0 維持。判定表の複数行が同時に鳴った:
- **L3 `geoab_kernel_mismatch attr=norm`(全件 norm・非 rigged 面・maxdiff 最大 2.7・NaN 混入例あり)**: 両カーネルの norm 計算は同一 SIMD 系譜のため、最有力仮説 = 「volume の mNormals 内容が worker 読取り時と apply 時で異なる」(未確定 volume の読取り競合)。**断定せず装置を増強 = 3-way 判別**(worker-then vs 旧経路-now vs カーネル-now)を実装。次回診断起動の `verdict=src|kernel|both` が機械確定する。
- L3 `geoab_input_drift` = volume swapped(LOD 切替)+ fields=0x200(色)— 非同期入力窓の実在を定量化。

## 5. 恒久化と撤去

- L3 = T2 移行の gate 装置。**T2 gate 確定後も既定 ON の標本照合のまま残す**(コスト無視可・以後の geometry 改修の恒久見張り)か縮退させるかは gate 時に AYA 決裁。次の移行(E2 等)では同型の A/B を移行側が建てる(規律 §0-4)。
- 本 doc は装置の台帳を兼ねる(サイト表・cause 名は実装と同期して更新)。

## 縮小・省略・解釈申告(設計+実装)

- L4(束縛 checksum)は設計しない(必要事象が出た時点で起工)— 「全部検知」はしない、の合意に基づく。
- L3 表示置換なし(staging 比較に改良・§3.2)。設計時の「旧内容が残る」記述は撤回。
- L3 は apply 成功 job のみ・buffer に既存 MappedRegion がある場合は skip・staging 確保失敗属性は無音 skip。
- L3 の drift 検査で `buildVkGeoFill` を再実行するため標本 face に stage 同等の副作用(GLOBAL/TEXTURE_ANIM state・mTexExtents 更新)が再発生する(正規 rebuild と同一の作用 = 安全側と解釈)。
- L3 の実行は volume 族のみ(worker 化されたのが volume 族のみのため)。
- 標準ストレス走行の「スクリプト化(自動カメラ走行)」は本設計に含めない(現状は AYA の手動走行 1 回で足りる。起動回数が再び問題化したら別途起工)。
