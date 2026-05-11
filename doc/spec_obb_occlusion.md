# OBB タグベース遮蔽 (occlusion) 仕様書 (r13)

> **対象**: AYAstorm `v7.2.5-ayastorm-r13` (想定)
> **前提**: `doc/spec_5_1ch_placement.md` (r10 5.1ch placement 仕様)
> **前提**: `doc/spec_binaural_venue_reverb.md` (r11 lite-HRTF + venue convolution reverb、配信者主導モデル)
> **前提**: `doc/spec_stereo_upmix.md` (r12 stereo→5.1 upmix)
> **背景文書**: `docs/ayastorm-positional-stream.md` (M1〜Post-M9 実装記録)
> **roadmap 整合**: `docs/ayastorm-stream3d-roadmap.md` §3 r13 (本書策定で「OBB タグベース遮蔽 (フラグシップ) + chat font live-apply 同梱」に確定、Steam Audio / SOFA / 個人 HRTF / 公開 README / air abs 客観 FFT は r14+ に降格)

## 1. 目的とスコープ

r13 は **r10/r11/r12 で築いた音響レンダリング** (placement → upmix → lite-HRTF → venue reverb) に対し、**SL 世界の物理ジオメトリによる遮蔽** を初めて持ち込むリリース。

r12 までの AYAstorm は「配信者プリムから出る音」を空間定位 + 音響キャラクタで届けることに集中してきたが、**音と空間の関係は音源側の設定だけで完結している**。会場の壁・天井などの物理オブジェクトが音を遮ることは想定されず、配信者プリムから出る音はリスナーまで遮蔽ゼロで直線的に届く。これは「狭い室内の音を遠くから聴く」といったリアル空間で当たり前の音響体験が SL の中で再現されないことを意味する。

r13 はこの欠落を埋める。会場運営 (= プリム/メッシュで建物を建てる人) がプリム Desc にタグ `[ayastorm:occlude]` を貼ると、viewer は当該プリムを **OBB (oriented bounding box) として登録** し、listener と各音源 (3D stream / `llPlaySound`) の line-of-sight 上で遮蔽を計算して direct/reverb 両方を減衰させる。occlude プリム自体が動けば毎 tick 自動追従するため、扉用の専用タグは設けない。

主目的:

- 会場運営が貼るタグ `[ayastorm:occlude]` で建物が音を遮るようになる
- スピーカーを部屋に閉じ込めると外で muffled、室内に踏み込むと clear
- 形状近似は **OBB のみ** (sphere/cylinder も box 近似、torus は穴を再現しない既知の妥協)
- r10/r11/r12 の音響レンダリングと **直交** して動く (placement / upmix / lite-HRTF / venue reverb をすべて維持、変更ゼロ)
- 役割分担を明確化: **occlusion は会場運営が決める物理現実**、**venue reverb (r11) は配信者が決める音響キャラクタ**、両者は意図的に直交

**設計思想 — 役割分担の明確化**:

r11 で確立した「配信者主導モデル」(= 配信者がスピーカープリム Desc に書いた表現意図を listener viewer は忠実にレンダリングする) を尊重しつつ、**新たに「会場運営主導」の概念を導入** する。SL では建物オーナーと配信者は別人格であり、建物の物理 (壁) は建設時に固定され、配信者は表現としての音響キャラクタ (venue reverb) を演奏ごとに選ぶ。このモデルでは:

- **occlusion タグ** は建物プリムに貼られる (会場運営の所有物)
- **venue reverb タグ** は配信者プリムに貼られる (配信者の所有物)
- 両者は viewer 側で **意図的に直交させる** (整合性チェック / 自動補正 / 警告は入れない)

これにより「狭い箱の中で野外 venue を選ぶ」「屋外イベントで cathedral venue を選ぶ」のような **物理と表現の不一致を仕様として許容**。レコーディング業界の「狭いスタジオで cathedral リバーブを掛けて録音」と同じ自由度を SL で保証する。

**OBB 単独で出荷する根拠**:

形状近似を OBB に絞り、回折 (diffraction) / 反射 (reflection) を持たないシンプルなモデルで shipping する。これにより:

- 実装コストが小さい (FMOD geometry API が raycast / 減衰累積を全部やるので viewer 側はタグ parser + OBB 抽出 + lifecycle のみ、3〜5 日相当)
- **建築用途の 98% が完全一致 or 軽微なズレ** で済む (壁/天井/平板 mesh が大半、sphere/cylinder の隅は OBB がやや過剰だが acoustic 体験的には軽微)
- r14+ Steam Audio で本格的な物理音響 (回折 / 反射 / 共鳴) を載せる際、**r13 で作った geometry 登録基盤がそのまま再利用できる** (タグ parser / OBB 抽出 / lifecycle が同じ入力)

**SL viewer 史上初の空間音響遮蔽**: 3D stream + r10 5.1ch placement + r11 venue reverb + lite-HRTF + r12 upmix に r13 occlusion が加わることで、AYAstorm は「リアル音響体験を SL で構築する viewer」として完成形に近づく。

**過去仕様との関係**:

- `doc/spec_5_1ch_placement.md` (r10) で確立した per-channel placement の経路は本書でも維持。occlusion は placement の出力に対する FMOD `set3DOcclusion` 自動適用として乗る (= placement 側に変更ゼロ)
- `doc/spec_binaural_venue_reverb.md` (r11) の venue reverb との関係は §4.6 で明記 (occlusion の `reverbOcclusion` が r11 ChannelGroup 末尾の reverb send をゲートする)
- `doc/spec_stereo_upmix.md` (r12) の upmix 経路にも変更なし (per-speaker channel の位置は r10 placement で決まり、occlusion はその位置を入力にするだけ)
- `docs/ayastorm-stream3d-roadmap.md` §3 r13 (旧 r13+ basket: SOFA / Steam Audio 等) の項目は本書策定で再定義され、Steam Audio / SOFA / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air absorption 客観 FFT は **r14+ へ繰下げ**

非対象 (r13 では触らない):

- **回折 (diffraction)**: 直線 raycast のみ。扉開口の脇から音が回り込む現象は再現しない (r14+ Steam Audio 領域)
- **反射 (reflection)**: ポリゴンに当たっても跳ね返らない、減衰のみ (r14+ Steam Audio)
- **吸音特性の周波数依存**: occlusion 値は一律、低音だけ通り抜ける挙動は r14+
- **共鳴 / 室内モード**: 部屋寸法による定在波等は r14+
- **形状近似の昇格**: sphere → icosahedron / cylinder → 16-prism などの精度モードは r13.x or r14+
- **mesh prim の実 triangle list 利用**: 全 mesh prim は OBB として近似 (r14+ Steam Audio で再検討)
- SOFA per-source HRTF / Steam Audio integration / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air absorption 客観 FFT — r14+

---

## 2. 問題定義

### 2.1 r12 までの体感の限界

r12 完了時点で AYAstorm は **5 本のレイヤー** (Layer 0 upmix / Layer 1 placement / Layer 2 lite-HRTF + venue reverb) を持つ。受入指標 (r10/r11/r12 §13) はすべて達成しているが、**音と空間ジオメトリの関係が無い** という根本的な限界がある:

- 配信者プリムから listener までの直線経路に壁・床・天井があっても **音は素通りする**
- 「ライブ会場の建物に近づくと muffled に聴こえ、扉から音が漏れ、入場すると clear になる」という SL ユーザが期待する体験が成立しない
- venue reverb (r11) はホール感を作るが、それは **音源そのものが纏う音響キャラクタ** であり、listener と音源の間の空間ジオメトリとは無関係
- 結果として「素晴らしい dry 配信を素晴らしい hall reverb で纏った音」が、壁の外でも壁の内でも同じ音量で聞こえる

この欠落は技術的な不足ではなく、**「音響レンダリング (Layer 0-2) と空間ジオメトリ (新 Layer)」の直交性が SL の実態に対して足りていない** ことが原因。viewer 側で空間ジオメトリ層を備えれば、音響レンダリング側を一切変えずに「物理空間が音を遮る」体験が成立する。

### 2.2 OBB 単独で shipping する根拠

occlusion 機能は理論的には複数の精度モードがありうる:

- (a) **OBB (oriented bounding box)**: 6 quad / 12 triangle、prim 1 個あたり最小コスト
- (b) **形状特化近似**: sphere → icosahedron 20 tri / cylinder → 16-prism 32 tri 等、shape code ごとに最適化
- (c) **実 mesh triangle list**: mesh prim から実 triangle を抽出、最高精度
- (d) **Steam Audio 統合**: 上記 (a)〜(c) の geometry を Steam Audio に渡し、回折 / 反射 / 共鳴を物理シミュレート

本書では **(a) OBB 単独** で shipping し、(b)(c)(d) は r13.x / r14+ への保留とする。理由:

- **建築用途の 98% は OBB で十分**: SL の建物に occluder として貼られるプリムの大半は壁 / 天井 / 床 / 角柱で、いずれも OBB との形状一致度が高い。sphere / cylinder の隅は OBB がやや過剰遮蔽になるが acoustic 体験的には軽微 (人の耳は遮蔽境界をピンポイントで識別できない、現実空間でも回折でぼける)。torus (ドーナツ) の穴は OBB で塞がれてしまうが、配信会場の遮蔽要素として torus が使われる例は実質ゼロ
- **実装コストが最小**: FMOD geometry API は raycast / 減衰累積をすべて内製。viewer 側はタグ parser + OBB 抽出 + lifecycle のみで 3〜5 日規模
- **r14+ Steam Audio に対する基盤投資**: r13 で作る geometry 登録基盤 (タグ parser / OBB 抽出 / UUID→occluder map / occluder lifecycle / 毎 tick transform 追従) は、Steam Audio が同じ入力を要求するためそのまま流用できる。r14 で「(a) → (d)」に置き換えるとき、置換対象は raycast engine → Steam Audio engine の 1 点のみ
- **配布負債ゼロ**: viewer 内 DSP 完結 (新 binary 不要)、3 OS でのビルド差なし (FMOD geometry API は platform 共通)
- **配信者主導モデルの拡張パターン継承**: r11 で確立した「タグ root truth + listener UI 改修ゼロ + sentinel 付き debug settings」を会場運営側にも展開、運用モデルが揃う

旧 r13+ basket (SOFA / Steam Audio / 個人 HRTF / 公開 README / air absorption 客観 FFT / VenueReverb CPU 最適化) は捨てるのではなく r14+ への持ち越しとする (§9 で詳述)。

### 2.3 役割分担を新たに導入する根拠

SL の社会構造として、**会場運営 (建物オーナー) と配信者 (演奏者) は別人格** である。建物オーナーはライブ会場をプリム/メッシュで建て、PA スピーカーを設置し、内装を整える。配信者はその会場に来て、自分のストリームを既設のスピーカーに繋いで演奏する。

この実態に viewer 側のタグ設計を合わせると:

| 役割 | 持ち物 | 変える頻度 | viewer タグ |
|---|---|---|---|
| **会場運営** | 壁/天井/床/スピーカー筐体 | 構築時のみ、以後固定 (移設時は移動に追従) | r13 新規: `[ayastorm:occlude]` |
| **配信者** | 3D stream URL + 表現キャラクタ | 演奏ごと | r5-r12: `[3dstream...]` / `[3dstream-stereo...]` 系 |
| **聴衆** | (UI 設定不要) | — | (debug settings のみ、平時は不使用) |

役割分担を明確化することで:

- 配信者は occlusion タグを覚えなくて済む
- 会場運営は r5-r12 配信タグを意識しなくて済む (デフォルトで pass-through、置いただけで occlusion 不参加)
- listener は両者の組合せの結果を体験するだけ (UI 設定不要)

### 2.4 r14 着手前にやっておきたいこと

r13 で済ませておくと r14+ Steam Audio 着手時に手戻りが少ない事項:

- geometry 登録基盤 (`LLOcclusionGeometryMgr`) の入出力 API を **「prim 集合 → OBB 集合 + per-occluder occlusion 値」** で確立。Steam Audio engine も同じ入力を取れる
- タグ parser を `LLPositionalStreamMgr` から独立させ、occluder 用の別 mgr (`LLOcclusionGeometryMgr`) として実装。Steam Audio に切り替える際も parser は再利用可能
- occluder の毎 tick transform 追従経路 (`refreshOccluders` で全件 `getPositionGlobal`/`getRotationRegion`/`getScale` 反映) を r13 で確立。r14 で Steam Audio に同じ hook を流用

---

## 3. ゴール / 非ゴール

### ゴール

- G1. **`[ayastorm:occlude]` タグ**: 遮蔽プリム (壁/天井/床/扉等) を OBB として登録、listener↔音源の line-of-sight で direct/reverb の両方を減衰。occlude プリム自体が動けば毎 tick 追従するため、扉用の専用タグは設けない (`refreshOccluders` で全件 transform 反映)
- G2. **OBB 抽出**: 全 shape code (box / cylinder / sphere / prism / torus / hemisphere / sculpt / mesh) を OBB に統一近似。`getPositionGlobal` / `getRotationRegion` / `getScale` から計算し、linkset 子 prim はリンク状態の region 座標を直接使う
- G3. **per-prim タグ override**: `[ayastorm:occlude{direct:N}{reverb:N}]` 形式で direct/reverb 個別の occlusion 値をプリム単位で上書き可 (引数なしは hardcoded default = direct 0.7 / reverb 0.5)
- G4. **listener↔音源 line-of-sight 自動計算**: viewer 側 raycast (segment vs OBB の slab test) を毎 tick 実行し、複数 occluder は乗算累積で減衰、direct 値は per-channel `LOWPASS_SIMPLE` cutoff にも写像 (22kHz→300Hz exponential)
- G5. **適用先音源**: 3D stream prim (r5-r12 系) + `llPlaySound` (オブジェクト効果音) — parcel music / voice (Vivox/WebRTC) は対象外
- G6. **debug settings — listener 側 override**: occlusion を強制 OFF にする緊急 sentinel (`Stream3DOcclusion`) と、scan 範囲 (`Stream3DOccluderRange`) を提供。direct/reverb 個別 gain や cap 数値は調整余地が薄いため設定化しない (default 値 hardcode)
- G7. **既存配置の自動恩恵**: r5-r12 で過去に置かれた全 stream prim は、会場運営が occlusion タグを建物に貼った瞬間から遮蔽の恩恵を受ける (stream 側再配置不要)
- G8. **r10/r11/r12 受入条件すべて維持**: dropout / CPU / URL 切替 / 互換マトリクス / 回帰、すべて r12 から劣化なし
- G9. **r14+ で Steam Audio を載せる場合の hook point** を仕様書とコードに明記
- G10. **chat font live-apply 同梱**: `feature/ll-chat-livetune-font-plaintext` ブランチの ChatFontSize / PlainTextChatHistory live-apply fix on LL-style chat (commit 2689a35f8f) を r13 にマージ

### 非ゴール

- NG1. **回折 / 反射 / 共鳴**: 直線 raycast モデル、跳ね返りなし、吸音は周波数非依存 (r14+ Steam Audio)
- NG2. **形状精度の昇格**: sphere → icosahedron / cylinder → 16-prism 等の per-shape 近似は r13.x or r14+
- NG3. **mesh prim の実 triangle 利用**: 全 mesh prim は OBB として近似 (r14+ Steam Audio で再検討)
- NG4. **material 表 (prim material flag → preset)**: SL の `LL_MCODE_*` を occlusion 値にマッピングする preset テーブルは作らない。引数なしのデフォルト値 (direct 0.7 / reverb 0.5) で大半の建物用途を賄い、特殊な素材/演出は per-prim タグ override で対応する (実機聴感での tuning 余地が薄いため、引数なしの default + per-prim override で十分との判断)
- NG5. **parcel music / voice への適用**: parcel music は位置を持たない、voice (Vivox/WebRTC) は別音響系統で別議論
- NG6. **listener 側 Preferences UI 改修ゼロ**: 配信者主導モデル + 会場運営主導モデルを維持、debug settings のみ
- NG7. **タグ整合性チェック**: 「狭い箱の中で野外 venue」のような物理と表現の不一致を viewer 側でブロックしない、警告も出さない (§1 設計思想)
- NG8. **公開 README / changelog 開示**: 機能成熟後 r14+ で一括 (r12 NG7 と同方針)

---

## 4. 設計

### 4.1 タグ書式 — `occlude` 新規タグ

#### 4.1.0 新規タグ一覧

| タグ | 値 | 適用 | 効果 |
|---|---|---|---|
| `[ayastorm:occlude]` | (引数なし) | 遮蔽プリム (壁/天井/床/扉等) | hardcoded default (direct=0.7 / reverb=0.5) で direct/reverb occlusion 適用、prim transform は毎 tick 追従 |
| `[ayastorm:occlude{direct:N}{reverb:N}]` | F32 [0.0〜1.0] × 2 | 同上 | per-prim 個別値で default を上書き。`{direct:N}` のみ / `{reverb:N}` のみの片側指定も可 (もう片方は default) |

書式は r12 までと異なり、**プリム Desc 内に独立して書ける** (= `[3dstream...]` 系のような linkset 集約は無し)。理由は §4.1.2 で詳述。引数のフォーマット規則は §4.3 共通ルールに準拠 (大文字小文字非区別、空白 trim、未知タグ silent ignore)。

扉のような動的プリムも `[ayastorm:occlude]` を貼るだけで自動追従する (`refreshOccluders` が毎 tick `getPositionGlobal` / `getRotationRegion` / `getScale` を全件再評価)。専用の `[ayastorm:door]` タグは設けない。

不正値 (上記以外) は **silent ignore** (r11 の不正タグ通知 throttle 機構と同様)。

#### 4.1.1 設計判断: タグ名の `ayastorm:` プレフィクス選択

r5-r12 の配信者タグはすべて `[3dstream...]` プレフィクス (= 機能群名)。一方 occlude はストリーム機能ではなく **会場運営による空間ジオメトリ宣言** なので、別系統のプレフィクスを採用:

- `[ayastorm:occlude]`: viewer 名 (`ayastorm`) + 機能名 (`occlude`)。viewer 固有機能であることを明示
- 他の AYAstorm 固有機能 (将来追加されうる) も `[ayastorm:...]` に統一する余地を残す
- `[3dstream...]` は配信者ストリームの設定として独立進化、`[ayastorm:...]` は viewer 固有の空間/物理機能 — の二系統に分離

#### 4.1.2 設計判断: linkset 集約しない理由

r5-r12 の `[3dstream-stereo:...]` 系は linkset 全体を 1 binding として扱う (root に音源宣言、子に `{ch}`)。一方 occlude は:

- 1 prim = 1 occluder という単純な関係 (linkset 全体で 1 occluder にする意味がない)
- 建物の壁が linkset の場合、各 prim を個別の occluder として登録するほうが OBB 近似の精度が高い (linkset 全体の bounding box は中身がスカスカでも全体を塞いでしまう)
- 会場運営が複雑な linkset を管理する負担を増やしたくない (= prim ごとに 1 タグで済む簡潔さ)

このため **occlude タグは prim 単位で評価**、linkset の root/子 を区別しない。同じ linkset の各 prim が独立に occluder になりうる。

#### 4.1.3 per-prim タグ override の用途

引数なしのデフォルト (direct 0.7 / reverb 0.5) で大半の建物用途は十分カバーできるが、特殊な素材や演出的な調整が必要な場合は per-prim 引数で上書きできる。例:

- `[ayastorm:occlude]` (引数なし、default direct 0.7 / reverb 0.5)
- `[ayastorm:occlude{direct:0.9}{reverb:0.7}]` (厚い石壁相当、direct/reverb 両方を強める)
- `[ayastorm:occlude{direct:0.3}{reverb:0.2}]` (ガラス窓相当、薄壁)
- `[ayastorm:occlude{direct:0.0}]` (完全に透過 — 診断/デバッグ用)
- `[ayastorm:occlude{reverb:0.0}]` (direct のみ default、reverb send は素通し)

### 4.2 viewer 内部経路

#### 4.2.1 新規 mgr クラス — LLOcclusionGeometryMgr

occlude タグの parsing と OBB レジストリ管理を専担する新規 mgr クラスを `indra/newview/llocclusiongeometrymgr.{h,cpp}` に新設。`LLPositionalStreamMgr` とは独立した singleton として動作。

| 責務 | 具体 |
|---|---|
| タグ parse | プリム Desc から `[ayastorm:occlude]` / `[ayastorm:occlude{direct:N}{reverb:N}]` を抽出、`onObjectPropertiesReceived` 経路で発火 |
| OBB 抽出 | 該当 prim の `LLViewerObject::getPositionGlobal` / `getRotationRegion` / `getScale` から center + half-extent + quat を計算 |
| OBB レジストリ | `std::map<LLUUID, OBB>` で 1 prim 1 occluder。direct/reverb 値もここに保持 |
| 毎 tick 追従 | `refreshOccluders()` が登録済み全 occluder の transform を再評価し、消えた prim をレジストリから drop |
| 適用 | `applyToChannel(channel, source_pos)` で listener↔source segment vs 全 OBB slab test を実行、複数 occluder は乗算で pass-through を累積、`Channel::set3DOcclusion` + per-channel `LOWPASS_SIMPLE` cutoff に書き戻し |
| 滑らか化 | 適用値は `Stream3DOcclusionRampMs` (default 250ms) で線形 ramp し、扉開閉等の 1-frame ジャンプを回避 |
| cap | 登録上限は hardcoded (`kMaxOccluders` = 256)、超過分は到来順で discard |

`LLPositionalStreamMgr` (r5-r12 系) との関係:
- 両 mgr は **完全独立** で動作
- listener 位置は `LLAudioEngine` 経由で両者が共有
- occlusion は viewer 側 raycast で計算し `Channel::set3DOcclusion` + `LOWPASS_SIMPLE` cutoff に直接書き戻すため、`LLPositionalStreamMgr` の channel 経路は変更不要

#### 4.2.2 raycast / DSP の構造

| 項目 | 値 |
|---|---|
| 形状 | OBB 1 個 = center + half-extent + quat、12 triangle/8 vertex に展開せず slab test で直接判定 |
| segment-vs-OBB | listener 位置 → source 位置の線分を OBB ローカル空間に変換し、3 軸で Liang-Barsky 風 slab test |
| 複数 occluder | listener↔source の同一 segment 上で hit した全 OBB の (1.0 − occlusion) を乗算累積 (= 透過率の積)、最終 occlusion = 1.0 − 累積透過率 |
| LOWPASS_SIMPLE | per-speaker channel に挿入、direct 値を 22kHz → 300Hz の exponential mapping で cutoff に変換 (set3DOcclusion 単体では音量しか減衰しないため、聴感上の "muffled" は cutoff で作る) |
| ramp | direct/reverb の適用値は `Stream3DOcclusionRampMs` (default 250ms) で線形 ramp、frame-to-frame jump 排除 |

> **歴史的経緯**: 当初は FMOD `System::createGeometry` + `addPolygon` の組合せで polygon database を作り FMOD に raycast させる設計だったが、同梱 `libfmod 2.03.07` で `createGeometry` が `FMOD_ERR_INTERNAL` を返し機能しないことが P1 着手で判明 (memory `project_fmod_geometry_unavailable.md`)。viewer 側で segment-vs-OBB slab test を自前実装する経路に pivot した。FMOD geometry API への将来的な復帰は r14+ で再評価する。

#### 4.2.3 r10/r11/r12 既存 DSP との関係

データフロー (= 信号が source から speaker output に至るまでの順序):

```
[Source stream]
        ↓ pumpSource() / decode thread
[mRing (per-track ring buffer)]
        ↓ pcmReadCallback (mixer thread, per speaker)
[OpKind dispatch — Silent / Track / StereoSum / Bs775 / Upmix (r10+r12)]
        ↓ 1ch output
[FMOD::Channel (mono, OPENUSER, per speaker)]
        ↓
[r13: per-speaker LOWPASS_SIMPLE DSP (cutoff = direct 値の写像)]  ← 新規追加点
        ↓
[r11 LiteHrtfDsp (per-channel mono in/out)]
        ↓ Channel built-in panner (set3DLevel)
[r13: Channel::set3DOcclusion(direct, reverb) を viewer 側 raycast から直接 set]  ← 新規追加点
        ↓
[FMOD::ChannelGroup "Stream3D"]
        ↓ Group::addDSP(tail)
[r11 VenueReverbDsp (reverb send が reverbOcclusion でゲートされる)]
        ↓
[Master group → output]
```

**r13 は既存 DSP chain の順序を一切変更しない**。`LOWPASS_SIMPLE` は per-speaker channel に挿入されるだけで lite-HRTF / venue reverb / placement / upmix は occlusion の存在を意識しない。

### 4.3 OBB 抽出 (LLOcclusionGeometryHelper)

#### 4.3.1 入出力

入力: `LLViewerObject*` (`[ayastorm:occlude]` タグが付いた prim)
出力: 8 vertices (region 座標系) + 6 quads + per-quad occlusion 値

```cpp
struct OBB {
    LLVector3 center;       // region 座標 (getPositionGlobal を region-relative に変換)
    LLVector3 half;         // half-extent (getScale() * 0.5)
    LLQuaternion rot;       // getRotationRegion()
    F32 direct = 0.7f;      // タグ default または per-prim override
    F32 reverb = 0.5f;      // 同上
};
```

#### 4.3.2 OBB 計算

```cpp
// prim transform を 1 OBB に変換 (slab test 用)
obb.center = LLVector3(obj->getPositionGlobal() - region_origin_global);
obb.half   = obj->getScale() * 0.5f;
obb.rot    = obj->getRotationRegion();
// direct/reverb はタグ parse 結果から (引数なし → kDefaultDirect=0.7 / kDefaultReverb=0.5)
```

slab test (segment vs OBB) は OBB ローカル空間で 3 軸 Liang-Barsky 風判定で実装。8 vertex / 12 triangle の polygon list は維持しない (FMOD geometry 経路を放棄したため不要)。

linkset の子 prim は `getPositionGlobal / getRotationRegion` がリンクされた状態の region/global 座標を返すので、追加の transform 合成は不要。

### 4.4 occlusion 値の決定 — タグ default + per-prim override

引数なしの `[ayastorm:occlude]` は **hardcoded default (`kDefaultDirect = 0.7f` / `kDefaultReverb = 0.5f`)** を採用。一般的な壁 (concrete 相当) を想定した値で、大半の建物用途はこの 1 種で十分体感が出る。

特殊な素材や演出的な調整は **per-prim タグ引数で上書き** する:

| 用途想定 | 推奨タグ |
|---|---|
| 一般的な壁 (default) | `[ayastorm:occlude]` |
| 厚い石壁・地下 | `[ayastorm:occlude{direct:0.9}{reverb:0.7}]` |
| 木壁・扉 | `[ayastorm:occlude{direct:0.6}{reverb:0.4}]` |
| ガラス・窓 | `[ayastorm:occlude{direct:0.3}{reverb:0.2}]` |
| カーテン・薄布 | `[ayastorm:occlude{direct:0.4}{reverb:0.3}]` |
| 完全透過 (診断用) | `[ayastorm:occlude{direct:0.0}{reverb:0.0}]` |

direct > reverb の関係は意図的に default で示している: 反射音 (reverb) は壁を回り込みやすい (低周波数成分が多い) ので、直接音より遮蔽が薄い。配信会場側でこの方針を踏襲することを tag-guide で推奨する。

なお SL の prim material flag (`LL_MCODE_*`) を preset 表に写像する経路は r13 では採用しない (NG4)。実機聴感で material flag 自体が「演出や見た目で選ばれている」運用が大半で、occlusion 値と相関させる根拠が弱いため、default + per-prim override の 2 層に絞った。

### 4.5 lifecycle

#### 4.5.1 occluder lifecycle

| イベント | 動作 |
|---|---|
| タグ付与 (Description 編集経由) | `onObjectPropertiesReceived` 経路でタグ parse、OBB 計算、`mOccluders[uuid] = obb` で登録。子 prim 由来の `ObjectPropertiesFamily` で誤って auto-unregister しないよう、タグ無し path では erase しない |
| プリム move / rotate | `refreshOccluders()` が毎 tick 全件 `getPositionGlobal` / `getRotationRegion` / `getScale` を再評価し center/half/rot を上書き — 静的壁の修正も扉の開閉も同じ経路 |
| プリム derez | `refreshOccluders()` で `LLViewerObject*` が引けなくなった登録を drop |
| Description 編集でタグ削除 | 該当プリムの ObjectProperties 通知 → tag-absent → 明示 unregister (auto-erase ではないが手動リネームで再登録から外せる) |

scan / refresh の頻度: `refreshOccluders()` は `LLOcclusionGeometryMgr::update()` から毎 tick 呼ばれる。範囲は `Stream3DOccluderRange` (default 64m) で、これより遠い登録済み occluder は raycast 計算をスキップ (登録自体は維持し、listener が近づいたら再活性化する)。

#### 4.5.2 動的プリム (扉等) の追従

専用タグは設けず、`[ayastorm:occlude]` 単独で動的追従が成立する:

- 扉 prim に `[ayastorm:occlude]` を貼る (引数なし default で OK、薄壁なら `{direct:0.4}{reverb:0.3}` 等)
- LSL `llSetRot` / `llSetPos` で開閉 → SL の通常 ObjectUpdate で `getRotationRegion` / `getPositionGlobal` が変わる
- 次の tick で `refreshOccluders()` が transform を反映、raycast の slab 形状が即座に追従

frame-to-frame の occlusion 値ジャンプは `Stream3DOcclusionRampMs` (default 250ms) の線形 ramp で吸収するため、開閉瞬間に「ガラッ」とノイズめいた変化はしない。

しきい値ロジックや扉専用 cap は入れない (r13 P0 議論で「扉数が現実的に少ないので不要」と判断、AYA 2026-05-10)。

#### 4.5.3 cap

登録 occluder の総数は hardcoded `kMaxOccluders` (= 256) を上限とし、超過分は到来順で discard する。`Stream3DOccluderRange` (default 64m) で範囲外の occluder は raycast コストから自動的に除外されるため、距離ソートによる動的入れ替えは行わない (実装単純化)。256 という値は SL の通常 sim 内 occluder 数 (建物 1 棟あたり 10〜30 prim、sim 内合計でも 100 前後) を 2x 余裕で吸収する設定。

### 4.6 r11 venue reverb / r12 upmix との関係

#### 4.6.1 venue reverb (r11) との直交性

§1 / §2.3 で詳述したとおり、venue reverb (r11) と occlusion (r13) は **意図的に直交**。viewer 側で:

- venue reverb タグ (`{venue}` / `{wetgain}`) はスピーカープリム Desc に書く (= 配信者所有)
- occlusion タグ (`[ayastorm:occlude]`) は建物プリム Desc に書く (= 会場運営所有)
- 両者の整合性チェック / 自動補正 / 警告は入れない (NG7)

`Channel::set3DOcclusion` の reverb 引数は r11 venue reverb の wet send を自動でゲートする:

| 位置 | direct (raycast 経由) | reverb send (raycast 経由) | 体感 |
|---|---|---|---|
| 屋外 | 壁で減衰 | 壁で減衰 | 「遠くで何かやってる、残響も聞こえない」 |
| 扉前 (扉開) | 開口通過 | 開口通過 | 「扉から音 + 室内残響が漏れてくる」 |
| 室内 | 遮蔽なし | 遮蔽なし | 「会場の中、フル venue reverb」 |

#### 4.6.2 upmix (r12) との関係

upmix の出力 6ch は r10 placement の per-channel 配置に流れ、occlusion は placement の channel 出力に対して viewer 側 raycast が走るだけなので、upmix 側に変更ゼロ。「stereo 配信 + upmix on + 室内」のフルチェインも自然に動作。

#### 4.6.3 `llPlaySound` (オブジェクト効果音) への適用

SL のオブジェクト効果音 (`llPlaySound` / `llPlaySoundSlave` 等) も FMOD 3D channel として鳴っているため、occlusion の対象になる。配信者主導モデルとは無関係に、建物内で鳴っている小道具の音 (時計の音、機械音、足音) も建物の外では muffled に聴こえる。

parcel music は位置を持たない (parcel 全体の 2D 音源) ため対象外。voice (Vivox/WebRTC) は別 audio engine を経由し本系統に乗らないため対象外。

### 4.7 debug settings 経由の listener 側 override

平時は **会場運営がプリム Desc に書いた `[ayastorm:occlude]` タグが root truth** として動作する。listener viewer は自動的にタグから値を読み出し、Preferences UI から override する経路は提供しない (NG6)。

実装/検証時の独立 toggle / 微調整用途のみ、debug settings 経由の override を最小限に絞る。`Stream3DOcclusion` のみ sentinel 値 (= 「タグ通り」default) を持つ:

| キー | 型 | default | 効果 |
|---|---|---|---|
| `Stream3DOcclusion` | int | `-1` (sentinel = タグ通り) | `0` = occlusion を強制 OFF (全タグを無視) / `1` = 強制 ON (default 動作と同じ、明示有効化用) |
| `Stream3DOccluderRange` | F32 | `64.0` (m) | listener↔source segment の長さがこの値を超えた場合 raycast をスキップ。長距離 stream で raycast コストを抑える役 |

それぞれの位置づけ:

- `Stream3DOcclusion`: 配信中に「occlusion 自体を疑いたい」(タグ設定が悪いのか viewer 側の問題なのか切り分けたい等) ときに使う緊急 toggle。普段は `-1` で運用、配信主や会場主が問い合わせを受けたときの A/B 切り分け用
- `Stream3DOccluderRange`: 大規模 sim で occluder が多すぎて raycast が重い場合に下げる調整値。逆に超大規模建造物 (city sim 等) で 64m を超える距離まで遮蔽させたい場合に上げる。default 64m は SL stream の `Stream3DRolloffMax` 上限と整合

ramp と debug viz は別系統の runtime 設定として既出荷 (`Stream3DOcclusionRampMs` / `Stream3DShowOccluders`)。direct/reverb の global multiplier や occluder 数 cap は r13 では設定化しない — multiplier はリリース後の実機 tuning 用に残しておきたい誘惑があるが、per-prim タグ override で代替できる (会場側を直せば済む) ため debug settings には載せない方針。cap は hardcoded 256 で SL の通常用途を吸収する。

**動作優先順位** (先に評価される側ほど強い):

1. `Stream3DOcclusion == 0` → occlusion 強制 OFF (全タグを無視)
2. プリム Desc タグ `[ayastorm:occlude]` → タグ値 (引数なしは default 0.7/0.5、引数ありは per-prim override) を採用
3. タグ未指定 → 該当プリムは occluder にならない (default OFF)

### 4.8 r10 / r11 / r12 受入条件への影響

r10 受入 (`spec_5_1ch_placement.md` §13) / r11 受入 (`spec_binaural_venue_reverb.md` §13) / r12 受入 (`spec_stereo_upmix.md` §6) はすべて **occluder ゼロ状態 (= タグなしの default 環境)** で従来通り維持される。occlusion 有効状態は新規受入として §6 で追加。

---

## 5. 検証材料

occlusion 検証は audio file 単独では完結せず、in-world で建物 prim を組み立てる必要がある。

### 5.1 検証 scene の構成

`doc/r13/build_test_scene.md` (新規) として scene 構築手順をドキュメント化。最小構成:

- **scene A (静的壁のみ)**: 8m × 8m × 4m の box room (6 face すべて壁)、内部中央にスピーカープリム 1 個。`{ch:M}` / `{url:test_voice.ogg}` で配信
- **scene B (扉付き)**: scene A + 1 face の中央に 2m × 2m の box prim (= 扉)、扉 prim には scripted rotation で 90° 開閉
- **scene C (linkset 壁)**: 4 face を 1 linkset (4 prim) で組んだ場合の挙動確認
- **scene D (mesh 壁)**: 平板 mesh prim を壁として使用、OBB 近似精度の体感確認

### 5.2 配信材料

audio 側は r12 の検証材料 (`doc/r12/gen_upmix_test_material.sh`) を流用:

- **stereo voice**: dialog のみのモノローグ。中央定位 + 室内/室外で muffled 度を確認
- **wide stereo music**: 楽器配置の素材。空間感が室内で広がり、室外で狭まることを確認
- **5.1 native (regression)**: 6ch source。6 spk placement + occlusion の合成挙動確認

### 5.3 検証手順は 1 ステップずつ

memory `feedback_one_step_at_a_time` に従い、検証は 1 メッセージ 1 アクションで進める (P9 で詳述)。

---

## 6. 受入条件

### 6.1 r13 新規

| # | 条件 | 判定方法 |
|---|---|---|
| O1 | scene A (壁のみ箱) で listener 室外 → 室内で direct sound が clear に変化 | 移動しながら主観確認 |
| O2 | scene A で listener 室外 → reverb (r11 venue=hall_medium) も同様に muffled→clear | venue タグ on で確認 |
| O3 | scene B 扉閉で listener 真正面に立つと muffled、扉開で clear に変化 | 扉プリムに `[ayastorm:occlude]`、LSL toggle で開閉 |
| O4 | scene B 扉開、listener 扉斜め前 30° で direct がそこそこ抜ける | 角度を変えながら確認 |
| O5 | scene B 扉開、listener 建物の真横 (扉から離れた壁の外) で muffled 維持 | 直線 raycast なので壁越しは遮蔽される — 仕様通り |
| O6 | per-prim 引数 `[ayastorm:occlude{direct:0.3}{reverb:0.2}]` (薄壁) と引数なし default の差が出る | 同じ scene A で 2 prim 切替 |
| O7 | `Stream3DOcclusion = 0` で occlusion 強制 OFF (タグ全無視) | debug 切替確認 |
| O8 | `Stream3DOccluderRange` を 8m に下げると遠い occluder が遮蔽しない | 距離変えて確認 |
| O9 | 動的 prim ([ayastorm:occlude] 付き扉) を移動 (回転/並進) すると遮蔽位置が即追従 | scripted prim で確認、ramp 250ms で滑らかに変化 |
| O10 | 配信者主導 r11 venue タグと occlusion が独立動作 (狭箱で野外 venue が許容される) | 不一致組合せで動作確認 |
| O11 | `llPlaySound` (オブジェクト効果音) も occlusion される | 室内に音源 prim、室外で muffled 確認 |
| O12 | `Stream3DShowOccluders` (Alt+Shift+O) で OBB が wireframe + fill で可視化される | View メニュー toggle |

### 6.2 r10 / r11 / r12 互換 (回帰)

- r10 §5.3 受入条件全行が回帰なし (occluder タグなしの環境で完全互換)
- r11 §5.5 受入条件全行が回帰なし (同上)
- r12 §6 受入条件全行が回帰なし (同上)
- 5.1 native 配信 + occlusion (scene A 内に 6 spk 配置) で 6 spk placement と occlusion が両立
- chat font live-apply fix (commit 2689a35f8f) が同梱、LL-style chat で ChatFontSize / PlainTextChatHistory が即時反映

### 6.3 安定性 / CPU

- 5min 連続再生 dropout 0 (3D stream + occlusion ×30 occluder + 動的扉 ×3 + venue=hall_medium + binaural=on + upmix=on)
- URL 切替 ×10 で crash / 二重再生なし
- prim rez/derez ×20 で occluder leak なし (`mOccluders` 件数が安定)
- 扉 prim 連続回転 (5min × LSL `llTargetOmega`) で raycast slab が遅延なく追従
- CPU 増分: r12 baseline (= occluder ゼロ) との比較で **+2pp 未満** を目標 (segment vs OBB slab test は数十 occluder 規模で軽量、`Stream3DOccluderRange` で長距離分は自動 skip)

---

## 7. リスク

| ID | 内容 | 縮退策 |
|---|---|---|
| R1 | mesh prim の OBB 近似が「斜め屋根」「アーチ」等で **明確にズレ** て体感に影響 | 縮退 A: 該当 prim だけ `[ayastorm:occlude]` を貼らない、配信会場側のガイドで「平面/box 形状の prim を occluder に使う」運用推奨。r13.x で形状特化近似 (NG2) を昇格検討 |
| R2 | torus prim を occluder に貼られた場合、穴も塞がれて体感がおかしい | 縮退 B: spec / tag-guide に「torus は OBB 近似で穴を再現しない」と明記、運用上の地雷を文書で外す。r14+ Steam Audio で改善 |
| R3 | default direct 0.7 / reverb 0.5 が実機聴感で **強すぎ / 弱すぎ** | 縮退 C: per-prim タグ override で会場運営側が再 tune、tag-guide に推奨セット (石壁 0.9/0.7 / 木壁 0.6/0.4 / ガラス 0.3/0.2 等) を提示。Default 自体の差し替えは r13.x で評価 |
| R4 | 動的扉が **LSL `llTargetOmega` で連続回転** され、毎 tick `refreshOccluders` の transform 比較が増える | 縮退 D: `kMaxOccluders` (= 256) で全体 cap が効くため局所的に扉が増えても segment 距離 cull で raycast skip。回転自体は cheap (vector 比較 + slab test 用 quat 上書き) |
| R5 | `Stream3DOccluderRange` (64m default) で `mOccluders` を全件 raycast すると **大規模建造物 (sim 全体に建物)** で重い | 縮退 E: range default を 32m に下げる、または `Stream3DOcclusion = 0` で一時無効化 |
| R6 | r11 venue reverb との合成で **reverb send が二重ゲート** されてしまい体感弱い | 縮退 F: per-prim タグで reverb 値を direct より弱め (default も direct > reverb の比率) |
| R7 | listener が **rapid teleport** で範囲を超えて動くと raycast が一瞬合わない | 縮退 G: ramp 250ms で吸収、teleport 後の最初の tick で `refreshOccluders` が距離 cull を再評価する |
| R8 | linkset の 1 prim だけにタグを貼った場合の挙動が **会場運営に直感的でない** (linkset 全体ではなく当該 prim のみ occluder) | 縮退 H: tag-guide 改訂時にこの仕様を明記、運用 FAQ 整備 |
| R9 | 既存 `[ayastorm:...]` 名前空間が将来別機能と衝突 | 縮退 I: r13 で `[ayastorm:occlude]` に限定、新機能追加時はタグ命名 review |

---

## 8. 実装フェーズ概要 (詳細は impl record)

詳細フェーズ分解と依存関係は `docs/ayastorm-r13-occlusion.md` を参照。本書では概要のみ:

- **P0**: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査
- **P1**: `LLOcclusionGeometryMgr` skeleton (registry + tag parser + OBB 抽出)、FMOD geometry 経路放棄判断 (memory `project_fmod_geometry_unavailable.md` 参照) → 自前 slab test に pivot
- **P2**: タグ parser (`[ayastorm:occlude]` / `{direct:N}{reverb:N}` 引数) + `onObjectPropertiesReceived` 経路統合
- **P3**: segment vs OBB slab test 実装 + 複数 occluder 乗算累積 + `Channel::set3DOcclusion` 適用
- **P4**: per-speaker `LOWPASS_SIMPLE` DSP 挿入 + direct→cutoff exponential mapping
- **P5**: occluder lifecycle (`refreshOccluders` 全件再評価、derez 検知)
- **P6**: 動的プリム追従 (per-tick transform 反映で `[ayastorm:occlude]` だけで扉も追従、専用タグ無し)
- **P7**: range cap (`Stream3DOccluderRange`) + hardcoded `kMaxOccluders` (= 256)
- **P8**: debug settings 2 件 (`Stream3DOcclusion` sentinel + `Stream3DOccluderRange`) + 既出荷 2 件 (`Stream3DOcclusionRampMs` / `Stream3DShowOccluders`) の配線確認
- **P9**: `llPlaySound` 系チャネルへの occlusion 適用拡張
- **P10**: 検証 scene 構築手順ドキュメント化 (`doc/r13/build_test_scene.md`) + 検証実行 (O1〜O12)
- **P11**: r10/r11/r12 回帰確認 + chat font live-apply fix の同梱確認 (commit 2689a35f8f / cherry-pick d66bdb74fc)
- **P12**: tag-guide 改訂 + Release Notes + spec close-out

工数感: **5〜7 日** (実装 4〜5 日 + 検証 1〜2 日)、r12 と同等。

---

## 9. r14+ への持ち越し

旧 r13+ basket から本書策定時に降格 + 本 r13 で発生した持ち越し:

### 9.1 r14: Steam Audio integration (本命の物理音響)

- world geometry を Steam Audio に食わせ **回折 / 反射 / 共鳴** を simulate
- r13 で確立した geometry 登録基盤 (`LLOcclusionGeometryMgr`) の入出力 API を Steam Audio 向けに拡張
- viewer 側 segment vs OBB raycast (r13) を Steam Audio の物理シミュレーションで置換
- venue reverb (r11) との関係: Steam Audio reflection ON 時は r11 convolution reverb を auto disable する条件分岐
- Linux / macOS の Steam Audio binary build 問題 (旧 RR1) を再評価
- **r13 で建てた registry / OBB 抽出 / `refreshOccluders` 全件追従 / per-speaker LOWPASS_SIMPLE はすべて流用可能** (= r13 投資が無駄にならない)

### 9.2 r14+: 形状精度の昇格

- sphere → icosahedron (20 tri)、cylinder → 16-prism (32 tri) などの shape 特化近似
- タグ引数で精度モード `[ayastorm:occlude{shape:fit}]` を選択
- mesh prim の実 triangle list 利用 (Steam Audio 統合と相性良)

### 9.3 r14+: SOFA per-source HRTF (旧 r12 main、r13 で再降格)

- KU100 等 reference HRTF を per-channel convolution に挿入、lite-HRTF を SOFA HRTF に置き換え
- 配信者タグ `{binaural:lite|sofa|off}` 等 enum 化を r14 着手時に検討
- 個人 SOFA 再配布制限の調査 (旧 RR2 の解像度を上げる)

### 9.4 r14+: VenueReverb CPU 最適化

- NUPC (non-uniform partition convolution) 等で hall_medium 以上の +8〜10pp を低減
- r11 spec §13.5 / §13.6 で識別済み

### 9.5 r14+: air absorption の客観 FFT 測定

- `-15dB @ 4kHz` を実機で対 dry スペクトル比較
- r11 P12 で主観 PASS、客観測定は r14+ へ

### 9.6 r14+: 個人 HRTF measurement / personalization

- 自家計測 (microphone-in-ear) の HRTF を SOFA 形式で読み込む経路
- r14 (SOFA HRTF) 完了後に自然に拡張可能

### 9.7 r14+: 公開 README / changelog 一括開示

- README / 公開ドキュメントで r8〜r13 の書式と使い方を一括開示
- 配置者向けガイド (venue 種別の選び方、配信側 dry 推奨、upmix の opt-in 推奨、occlusion タグの貼り方など) を公開

### 9.8 r13.x: occlusion パラメータの実機 tuning

- default direct 0.7 / reverb 0.5 が実機聴感で外れていれば差し替え (R3 縮退)
- per-prim 引数 `{direct:N}{reverb:N}` 以外の形式 (例: `{material:wood}` 等の preset 名) を要望に応じて検討
- 動的扉の rapid update が CPU を圧迫する場合のしきい値ロジック追加 (R4 縮退、必要なら)

### 9.9 r13.x: material flag の preset 写像 (再評価)

- r13 では NG4 として材質→occlusion 値 preset 表は採用しなかったが、運用で「per-prim タグを毎 prim 貼るのが面倒」「壁の material flag を変えるだけで遮蔽が変わってほしい」という要望が出れば、`LL_MCODE_*` → preset の写像を追加検討
- 採用判断は r13 リリース後の会場運営フィードバック次第

---

## 10. 変更履歴

- 2026-05-10: 初版作成。r12 リリース直後の議論で AYA から「3D stream で prim から音を出せるようになったが、音を遮る/反射する prim を作れないか検討したい」提案 (2026-05-10)。OBB 近似で v1 を出荷、回折 / 反射 / 共鳴は r14+ Steam Audio に保留することを確定。タグ書式 `[ayastorm:occlude]` / `[ayastorm:door]` (引数オプション付き)、material 表 (SL prim material flag → preset) を採用。役割分担として「会場運営が建物タグを貼る、配信者が表現タグ (r11 venue) を貼る、両者は意図的に直交」を明文化 (project memory `project_venue_occlusion_orthogonal.md` 参照)。debug settings 4 件 (sentinel 1 件 + multiplier/cap 3 件) + range 設定 1 件。chat font live-apply fix (commit 2689a35f8f、`feature/ll-chat-livetune-font-plaintext` ブランチ) を r13 同梱バグ修正として組込み。旧 r13+ basket (SOFA / Steam Audio / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air abs 客観 FFT) は r14+ へ繰下げ。
- 2026-05-10 (spike 実装): 同梱 `libfmod 2.03.07` の `System::createGeometry` が機能しない (`FMOD_ERR_INTERNAL`) ことが P1 着手で判明 (memory `project_fmod_geometry_unavailable.md`)。FMOD geometry API 経路 (§4.2.2) を放棄し、**listener-source segment vs OBB の slab test を viewer 側で自前実装**して `Channel::set3DOcclusion` に直接適用する経路に pivot。spike では `[ayastorm:occlude]` のみ shipping (`[ayastorm:door]` / material 表 / debug settings 4 件のうち 3 件は r13.x 持ち越し)。追加で **per-speaker `FMOD_DSP_TYPE_LOWPASS_SIMPLE`** を `LLPositionalStreamMulti::SpeakerRuntime` に持たせ、direct 値を 22kHz→300Hz の exponential mapping で cutoff に変換することで「壁越しに muffled」聴感を実現 (set3DOcclusion 単体だと音量減衰のみで質感変化しない)。`Stream3DOcclusionRampMs` (default 250ms) で direct/reverb 因子を線形 ramp、開閉ドアの 1-frame ジャンプを回避。debug overlay は `Stream3DShowOccluders` toggle で fill (α=0.25) + wireframe + halo (+5cm、z-fight 回避) の 2-pass 描画、View メニュー直接公開 (`Alt+Shift+O`、EN/JA 両ローカライズ)。同 commit に **起動時 OS unresponsive dialog 緩和 (A+B)** を同梱 — `mPendingLinksetEval` drain を 1 root/frame に rate-limit、libcurl HEAD pre-resolve timeout を 3000/2000ms → 1500/1000ms に短縮。AYA 確認で完全には消えなかったため、**根本対応 (curl 非同期化、C)** は別 workstream として r13 内 / r13.x で着手予定。実装詳細は `docs/ayastorm-r13-occlusion.md` §5 を canonical とする。
- 2026-05-10 (C 完了): r13 C を同日中に shipping (`f336d43abc` + `5c3487ff06`)。`LLStream3DUrlResolve` を **完全非同期 API** に再構築 — 専用 worker thread + lazy 起動 + request-id ベース `submit/poll/cancel/shutdown`。`LLPositionalStreamMulti` 状態機械を `Idle → Resolving → Opening → Buffering → Playing → Failed` に拡張、main thread の libcurl 同期ブロックを完全消滅。Linux 初回ビルドで `enum class Status` が X11 Xlib `#define Status int` (newview PCH 経由 `llglheaders.h → glx.h → X11/Xlib.h` で全 TU に漏洩) と衝突 → `ResolveStatus` にリネームで復旧、罠を `project_linux_xlib_status_define_trap.md` メモリに記録。Linux + Windows 両 OS で起動確認完了。残リスクは `LLPositionalStreamMgr::update()` 内の他重処理 (occlusion raycast / per-frame DSP 更新) で、実機で重さが観測されたら raycast hysteresis 等で次の最適化に進む。
- 2026-05-11 (r13 final scope 確定): C 完了後にコード現況と spec を突き合わせ、残工程を確定。**永久 drop**: `[ayastorm:door]` 専用タグ (`refreshOccluders` の毎 tick 全件追従で `[ayastorm:occlude]` だけで扉動作が成立、専用タグ不要)、material 表 (`LL_MCODE_*` → preset 写像、default + per-prim 引数で十分との判断)、`Stream3DOcclusionDirectGain` / `Stream3DOcclusionReverbGain` (per-prim タグ override で代替できる)、`Stream3DOccluderMaxCount` 設定化 (hardcoded `kMaxOccluders = 256` で吸収)。**r13 残工程**: (1) `[ayastorm:occlude]` 引数形式を `[ayastorm:occlude{direct:N}{reverb:N}]` に確定 (per-prim direct/reverb 個別指定可)、(2) `llPlaySound` への occlusion 適用 (G5)、(3) `Stream3DOccluderRange` 設定 ship + 距離 cull、(4) `Stream3DOcclusion` master sentinel ship、(5) `kMaxOccluders` 64 → 256 へ引き上げ、(6) chat font live-apply fix を r13 へ cherry-pick (`d66bdb74fc`、元 `2689a35f8f`)、(7) tag-guide ja/en/zh への `[ayastorm:occlude]` 項追記、(8) Release Notes (リンク + 差分ハイライト)。本 commit は (上記の永久 drop + 残工程確定) を spec に反映する scope-finalization commit、impl record / roadmap 側を続けて整える。
- 2026-05-11 (P15 mesh raycast 昇格): OBB 単独判定では「斜め屋根 / アーチ / Path Cut の切れ目 / Hollow 内部 / mesh の細部」が物理形状と一致しない件を解消するため、判定を **OBB pre-cull (~95% reject) → Möller-Trumbore 三角形 raycast** の 2 段に昇格 (P15.1〜P15.8)。`LLVolume::getVolumeFace` 経由でプリムの実形状三角形を抽出し OBB-local 座標で保持。`kMaxTrisPerOccluder = 2000` 超過時は OBB-only に自動 fallback (`LL_WARNS_ONCE`、典型 SL 建築は範囲内)。`Stream3DShowOccluders` overlay はオレンジ OBB から **シアン三角形メッシュ (半透明 fill + wireframe)** に一本化、編集中プリム (build floater 選択中) は毎 tick 再抽出してライブ追従。
- 2026-05-11 (P15.9 TP/login freeze 対策): 三角形抽出 (`extractTriangles`) を `onObjectPropertiesReceived` 同期実行していた経路が TP/login バースト時に主スレッド hitch (100-200ms 級) を起こす懸念を P15.9 で消化 (commit `2887e598f7`)。**A. per-tick budget=6 の遅延ドレイン** (新規登録は OBB だけ即時セットし `mPendingExtract` に積み、`refreshOccluders` 末尾で 6 件/tick だけ drain) と **B. Desc 同値時 re-extract スキップ** (TP 中の冗長 ObjectProperties 再配信で空回りしない) の A+B 同梱で根治。drain 中も `segmentHitsShape` は `tris.empty()` のとき OBB-only fallback を返すので audio raycast は継続。
- 2026-05-12 (r13 受入 PASS + close-out): impl record §4.1 / §4.2 / §4.3 の全項目を AYA 実機検証で PASS。**§4.1 r13 新規**: O1〜O12 (室外/室内 muffled→clear / venue × occlusion 直交 / 扉開閉 / `{direct:N}{reverb:N}` 引数差 / master sentinel / 距離 cull / 動的追従 + ramp / `llPlaySound` 適用 / Show Occluders 可視化) 全 12 項目 PASS、安定性 5 項目 (5min dropout / URL 切替 ×10 / rez/derez ×20 / 動的扉 5min / CPU r12+2pp 未満) 全 PASS。**§4.2 r10/r11/r12/r9/r8 互換**: occluder タグなし環境で既存配置の完全互換 PASS、Vorbis 実機 / Opus・FLAC コードレビュー完了。**§4.3 chat font live-apply**: `ChatFontSize` / `PlainTextChatHistory` 即時反映 + FS-style chat に regression なし、全 PASS。**r13 ブランチに同居の独立 4 件** (`617716ced8` V3 skin `FSUseNearbyChatConsole` 既定 1 揃え / `1dfa52d0e9` `[parcelhide]` altitude 高度ゲート / `e99d7c9abf` `FSIgnoreObjectIM` / `33c3afaf62` 描画パフォーマンス調査メモ) も r13 release train に同梱、Release Notes ja/en/zh と README parcelhide 表に追記済。push / tag / 3 OS build / GitHub release publish は AYA 側で実施。
