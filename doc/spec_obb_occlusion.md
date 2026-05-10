# OBB タグベース遮蔽 (occlusion) 仕様書 (r13)

> **対象**: AYAstorm `v7.2.5-ayastorm-r13` (想定)
> **前提**: `doc/spec_5_1ch_placement.md` (r10 5.1ch placement 仕様)
> **前提**: `doc/spec_binaural_venue_reverb.md` (r11 lite-HRTF + venue convolution reverb、配信者主導モデル)
> **前提**: `doc/spec_stereo_upmix.md` (r12 stereo→5.1 upmix)
> **背景文書**: `docs/ayastorm-positional-stream.md` (M1〜Post-M9 実装記録)
> **roadmap 整合**: `docs/ayastorm-stream3d-roadmap.md` §3 r13 (本書策定で「OBB タグベース遮蔽 (フラグシップ) + chat font live-apply 同梱」に確定、Steam Audio / SOFA / 個人 HRTF / 公開 README / air abs 客観 FFT は r14+ に降格)

## 1. 目的とスコープ

r13 は **r10/r11/r12 で築いた音響レンダリング** (placement → upmix → lite-HRTF → venue reverb) に対し、**SL 世界の物理ジオメトリによる遮蔽** を初めて持ち込むリリース。

r12 までの AYAstorm は「配信者プリムから出る音」を空間定位 + 音響キャラクタで届けることに集中してきたが、**音と空間の関係は音源側の設定だけで完結している**。会場の壁・天井・扉などの物理オブジェクトが音を遮ることは想定されず、配信者プリムから出る音はリスナーまで遮蔽ゼロで直線的に届く。これは「狭い室内の音を遠くから聴く」「扉を開けると音が漏れてくる」といったリアル空間で当たり前の音響体験が SL の中で再現されないことを意味する。

r13 はこの欠落を埋める。会場運営 (= プリム/メッシュで建物を建てる人) がプリム Desc にタグ `[ayastorm:occlude]` / `[ayastorm:door]` を貼ると、viewer は当該プリムを **OBB (oriented bounding box) として FMOD geometry に登録** し、listener と各音源 (3D stream / `llPlaySound`) の line-of-sight 上で遮蔽を計算して direct/reverb 両方を減衰させる。

主目的:

- 会場運営が貼るタグ (`[ayastorm:occlude]` / `[ayastorm:door]`) で建物が音を遮るようになる
- スピーカーを部屋に閉じ込めると外で muffled、扉を開けると line-of-sight が抜けて漏れる、室内に踏み込むと clear
- 形状近似は **OBB のみ** (sphere/cylinder も box 近似、torus は穴を再現しない既知の妥協)
- r10/r11/r12 の音響レンダリングと **直交** して動く (placement / upmix / lite-HRTF / venue reverb をすべて維持、変更ゼロ)
- 役割分担を明確化: **occlusion は会場運営が決める物理現実**、**venue reverb (r11) は配信者が決める音響キャラクタ**、両者は意図的に直交

**設計思想 — 役割分担の明確化**:

r11 で確立した「配信者主導モデル」(= 配信者がスピーカープリム Desc に書いた表現意図を listener viewer は忠実にレンダリングする) を尊重しつつ、**新たに「会場運営主導」の概念を導入** する。SL では建物オーナーと配信者は別人格であり、建物の物理 (壁/扉) は建設時に固定され、配信者は表現としての音響キャラクタ (venue reverb) を演奏ごとに選ぶ。このモデルでは:

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
- **動的 occluder の壁追従**: 壁 prim が動いても登録は再評価しない (= 動くのは door だけ)。建設後に壁が移動するユースケースは想定しない
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
- **r14+ Steam Audio に対する基盤投資**: r13 で作る geometry 登録基盤 (タグ parser / OBB 抽出 / UUID→polygon map / 静的 occluder lifecycle / 動的 door 追従) は、Steam Audio が同じ入力を要求するためそのまま流用できる。r14 で「(a) → (d)」に置き換えるとき、置換対象は FMOD geometry → Steam Audio engine の 1 点のみ
- **配布負債ゼロ**: viewer 内 DSP 完結 (新 binary 不要)、3 OS でのビルド差なし (FMOD geometry API は platform 共通)
- **配信者主導モデルの拡張パターン継承**: r11 で確立した「タグ root truth + listener UI 改修ゼロ + sentinel 付き debug settings」を会場運営側にも展開、運用モデルが揃う

旧 r13+ basket (SOFA / Steam Audio / 個人 HRTF / 公開 README / air absorption 客観 FFT / VenueReverb CPU 最適化) は捨てるのではなく r14+ への持ち越しとする (§9 で詳述)。

### 2.3 役割分担を新たに導入する根拠

SL の社会構造として、**会場運営 (建物オーナー) と配信者 (演奏者) は別人格** である。建物オーナーはライブ会場をプリム/メッシュで建て、PA スピーカーを設置し、内装を整える。配信者はその会場に来て、自分のストリームを既設のスピーカーに繋いで演奏する。

この実態に viewer 側のタグ設計を合わせると:

| 役割 | 持ち物 | 変える頻度 | viewer タグ |
|---|---|---|---|
| **会場運営** | 壁/天井/床/扉/スピーカー筐体 | 構築時のみ、以後固定 | r13 新規: `[ayastorm:occlude]` / `[ayastorm:door]` |
| **配信者** | 3D stream URL + 表現キャラクタ | 演奏ごと | r5-r12: `[3dstream...]` / `[3dstream-stereo...]` 系 |
| **聴衆** | (UI 設定不要) | — | (debug settings のみ、平時は不使用) |

役割分担を明確化することで:

- 配信者は occlusion タグを覚えなくて済む
- 会場運営は r5-r12 配信タグを意識しなくて済む (デフォルトで pass-through、置いただけで occlusion 不参加)
- listener は両者の組合せの結果を体験するだけ (UI 設定不要)

### 2.4 r14 着手前にやっておきたいこと

r13 で済ませておくと r14+ Steam Audio 着手時に手戻りが少ない事項:

- geometry 登録基盤 (`LLOcclusionGeometryMgr`) の入出力 API を **「prim 集合 → polygon 集合 + per-polygon occlusion 値」** で確立。Steam Audio engine も同じ入力を取れる
- タグ parser を `LLPositionalStreamMgr` から独立させ、occluder/door 用の別 mgr (`LLOcclusionGeometryMgr`) として実装。Steam Audio に切り替える際も parser は再利用可能
- 動的 door の per-frame transform 追従経路 (ObjectUpdate hook → setRotation/setPosition) を r13 で確立。r14 で Steam Audio に同じ hook を流用
- material 表 (wood/stone/glass/metal/...) を独立 helper として実装。Steam Audio の material parameter (absorption / scattering coefficient) にもマッピング可能な構造で書く

---

## 3. ゴール / 非ゴール

### ゴール

- G1. **`[ayastorm:occlude]` タグ**: 静的遮蔽プリム (壁/天井/床) を OBB として FMOD geometry に登録、listener↔音源の line-of-sight で direct/reverb の両方を減衰
- G2. **`[ayastorm:door]` タグ**: 動的遮蔽プリム (扉) を OBB として登録、prim transform 変化に追従して FMOD geometry の polygon を毎フレーム更新
- G3. **OBB 抽出**: 全 shape code (box / cylinder / sphere / prism / torus / hemisphere / sculpt / mesh) を OBB に統一近似。linkset 走査で root の transform に子 prim も合わせる
- G4. **material 表**: SL の prim material flag (none / wood / metal / glass / stone / flesh / plastic / rubber) → preset table → directOcclusion / reverbOcclusion 値マッピング
- G5. **タグ引数による override**: `[ayastorm:occlude:0.8]` / `[ayastorm:door:0.5]` 形式で material preset を上書き可 (1 つの float 値、direct/reverb 両方に適用)
- G6. **listener↔音源 line-of-sight 自動計算**: FMOD geometry に登録するだけで listener 位置 (既存 audio engine が更新済) と各 3D channel 位置から FMOD が raycast を毎フレーム実行
- G7. **適用先音源**: 3D stream prim (r5-r12 系) + `llPlaySound` (オブジェクト効果音) — parcel music / voice (Vivox/WebRTC) は対象外
- G8. **debug settings 経由の listener 側 override**: occlusion を強制 OFF にする緊急 sentinel (`Stream3DOcclusion`) と、global gain (`Stream3DOcclusionDirectGain` / `Stream3DOcclusionReverbGain`) を提供
- G9. **既存配置の自動恩恵**: r5-r12 で過去に置かれた全 stream prim は、会場運営が occlusion タグを建物に貼った瞬間から遮蔽の恩恵を受ける (stream 側再配置不要)
- G10. **r10/r11/r12 受入条件すべて維持**: dropout / CPU / URL 切替 / 互換マトリクス / 回帰、すべて r12 から劣化なし
- G11. **r14+ で Steam Audio を載せる場合の hook point** を仕様書とコードに明記
- G12. **chat font live-apply 同梱**: `feature/ll-chat-livetune-font-plaintext` ブランチの ChatFontSize / PlainTextChatHistory live-apply fix on LL-style chat (commit 2689a35f8f) を r13 にマージ

### 非ゴール

- NG1. **回折 / 反射 / 共鳴**: 直線 raycast モデル、跳ね返りなし、吸音は周波数非依存 (r14+ Steam Audio)
- NG2. **形状精度の昇格**: sphere → icosahedron / cylinder → 16-prism 等の per-shape 近似は r13.x or r14+
- NG3. **mesh prim の実 triangle 利用**: 全 mesh prim は OBB として近似 (r14+ Steam Audio で再検討)
- NG4. **動的 occluder の壁追従**: 壁 prim が動いても登録は再評価しない (動的更新は door のみ)
- NG5. **parcel music / voice への適用**: parcel music は位置を持たない、voice (Vivox/WebRTC) は別音響系統で別議論
- NG6. **listener 側 Preferences UI 改修ゼロ**: 配信者主導モデル + 会場運営主導モデルを維持、debug settings のみ
- NG7. **タグ整合性チェック**: 「狭い箱の中で野外 venue」のような物理と表現の不一致を viewer 側でブロックしない、警告も出さない (§1 設計思想)
- NG8. **動的 material 変化**: prim material flag が動的に変わってもタグ再評価のタイミングまで反映しない
- NG9. **公開 README / changelog 開示**: 機能成熟後 r14+ で一括 (r12 NG7 と同方針)

---

## 4. 設計

### 4.1 タグ書式 — `occlude` / `door` 新規タグ

#### 4.1.0 新規タグ一覧

| タグ | 値 | 適用 | 効果 |
|---|---|---|---|
| `[ayastorm:occlude]` | (引数なし) | 静的遮蔽プリム (壁/天井/床) | material 表に従って direct/reverb occlusion 適用 |
| `[ayastorm:occlude:N]` | F32 [0.0〜1.0] | 同上 | material 値を override、direct/reverb 共通の occlusion 値 |
| `[ayastorm:door]` | (引数なし) | 動的遮蔽プリム (扉) | material 表に従う + prim transform に毎フレーム追従 |
| `[ayastorm:door:N]` | F32 [0.0〜1.0] | 同上 | material 値を override |

書式は r12 までと異なり、**プリム Desc 内に独立して書ける** (= `[3dstream...]` 系のような linkset 集約は無し)。理由は §4.1.2 で詳述。引数のフォーマット規則は §4.3 共通ルールに準拠 (大文字小文字非区別、空白 trim、未知タグ silent ignore)。

不正値 (上記以外、例: `[ayastorm:occlude:abc]`) は **silent ignore + chat 通知 1 回** (r11 の不正タグ通知 throttle 機構を流用)。

#### 4.1.1 設計判断: タグ名の `ayastorm:` プレフィクス選択

r5-r12 の配信者タグはすべて `[3dstream...]` プレフィクス (= 機能群名)。一方 occlude/door はストリーム機能ではなく **会場運営による空間ジオメトリ宣言** なので、別系統のプレフィクスを採用:

- `[ayastorm:occlude]`: viewer 名 (`ayastorm`) + 機能名 (`occlude`)。viewer 固有機能であることを明示
- 他の AYAstorm 固有機能 (将来追加されうる) も `[ayastorm:...]` に統一する余地を残す
- `[3dstream...]` は配信者ストリームの設定として独立進化、`[ayastorm:...]` は viewer 固有の空間/物理機能 — の二系統に分離

#### 4.1.2 設計判断: linkset 集約しない理由

r5-r12 の `[3dstream-stereo:...]` 系は linkset 全体を 1 binding として扱う (root に音源宣言、子に `{ch}`)。一方 occlude/door は:

- 1 prim = 1 occluder という単純な関係 (linkset 全体で 1 occluder にする意味がない)
- 建物の壁が linkset の場合、各 prim を個別の occluder として登録するほうが OBB 近似の精度が高い (linkset 全体の bounding box は中身がスカスカでも全体を塞いでしまう)
- 会場運営が複雑な linkset を管理する負担を増やしたくない (= prim ごとに 1 タグで済む簡潔さ)

このため **occlude/door タグは prim 単位で評価**、linkset の root/子 を区別しない。同じ linkset の各 prim が独立に occluder になりうる。

#### 4.1.3 引数による override の用途

material 表 (§4.4) の preset 値で大半の建物用途は十分カバーできるが、特殊な素材や演出的な調整が必要な場合に引数で override できる。例:

- `[ayastorm:occlude]` (引数なし、material flag に従う)
- `[ayastorm:occlude:0.5]` (material 表を無視、直接 0.5 を direct/reverb 両方に適用)
- `[ayastorm:door:0.0]` (扉だが完全に透過 — 開け閉めしても遮蔽ゼロ、診断/デバッグ用)
- `[ayastorm:occlude:1.0]` (完全遮蔽、material flag が flesh 等の薄い設定でも全周遮断)

引数は direct/reverb 両方の occlusion を同じ値にする (= 単一値で十分単純化)。direct と reverb で別値を設定する必要は r13 では発生しないと判断。r13.x で要望が出れば `[ayastorm:occlude:0.7,0.5]` 形式に拡張余地。

### 4.2 viewer 内部経路

#### 4.2.1 新規 mgr クラス — LLOcclusionGeometryMgr

occlude/door タグの parsing と FMOD geometry 管理を専担する新規 mgr クラスを `indra/newview/llocclusiongeometrymgr.{h,cpp}` に新設。`LLPositionalStreamMgr` とは独立した singleton として動作。

| 責務 | 具体 |
|---|---|
| プリム scan | 範囲内の全 prim Desc を polling、occlude/door タグを抽出 |
| OBB 抽出 | 該当 prim の `LLViewerObject::getPositionRegion / getRotationRegion / getScale` から 8 vertices 12 triangles を計算 |
| FMOD geometry 操作 | `System::createGeometry` で 1 region 1 geometry object、`addPolygon` / `setRotation` / `setPosition` で polygon を管理 |
| material 解決 | `LLViewerObject::getMaterial()` → preset 表 → directOcclusion/reverbOcclusion 決定 |
| lifecycle | rez/derez/move 検知、UUID→polygon index map を維持 |
| 動的 door | ObjectUpdate hook で door prim の transform を毎フレーム反映 |
| cap | 範囲内の occluder 数が `Stream3DOccluderMaxCount` (default 200) を超えたら距離順で打切り |

`LLPositionalStreamMgr` (r5-r12 系) との関係:
- 両 mgr は **完全独立** で動作
- listener 位置は `LLAudioEngine` 経由で両者が共有
- occlusion 効果は FMOD geometry が listener と各 channel 位置から自動 raycast するので、`LLPositionalStreamMgr` の channel 出力は変更不要

#### 4.2.2 FMOD geometry の構造

| 項目 | 値 |
|---|---|
| Geometry object 数 | 1 region につき 1 個 (全 occluder polygon を 1 つの geometry にまとめる) |
| 最大 polygon 数 | `Stream3DOccluderMaxCount × 6` (default 200 × 6 = 1200) |
| 最大 vertex 数 | `Stream3DOccluderMaxCount × 8` (default 200 × 8 = 1600) |
| Polygon 1 個の vertex 数 | 4 (quad、FMOD は内部で 2 triangle に分解) |
| Polygon 数/prim | 6 (OBB の 6 face) |
| doubleSided | true (raycast がどちら向きでも遮蔽、薄壁 prim での raycast 入射方向ロバスト性のため) |

FMOD `System::setGeometrySettings(maxworldsize)` は region 上限 (256m) で初期化。

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
[r11 LiteHrtfDsp (per-channel mono in/out)]
        ↓ Channel built-in panner (set3DLevel)
[r13: FMOD geometry が listener↔channel 位置で raycast]    ← 新規追加点
[      → FMOD が channel に set3DOcclusion(direct, reverb)] ← 自動適用
        ↓
[FMOD::ChannelGroup "Stream3D"]
        ↓ Group::addDSP(tail)
[r11 VenueReverbDsp (reverb send が reverbOcclusion でゲートされる)]
        ↓
[Master group → output]
```

**r13 は既存 DSP chain を一切変更しない**。FMOD geometry は side channel として動作し、各 3D channel の `set3DOcclusion(direct, reverb)` 値を毎フレーム自動更新するだけ。lite-HRTF / venue reverb / placement / upmix はすべて occlusion の存在を意識しない。

### 4.3 OBB 抽出 (LLOcclusionGeometryHelper)

#### 4.3.1 入出力

入力: `LLViewerObject*` (occlude/door タグが付いた prim)
出力: 8 vertices (region 座標系) + 6 quads + per-quad occlusion 値

```cpp
struct OccluderOBB {
    LLVector3 vertices[8];      // region 座標
    LLVector3 quad_normals[6];  // 各面の法線 (debug viz 用)
    F32 direct_occlusion;       // material 表 or タグ override
    F32 reverb_occlusion;       // 同上
};
```

#### 4.3.2 OBB 計算

```cpp
// 1. prim local AABB (scale が prim 半径相当)
LLVector3 half_scale = obj->getScale() * 0.5f;

// 2. 8 vertices in local frame
LLVector3 local_verts[8] = {
    {-h.x, -h.y, -h.z}, {+h.x, -h.y, -h.z},
    {+h.x, +h.y, -h.z}, {-h.x, +h.y, -h.z},
    {-h.x, -h.y, +h.z}, {+h.x, -h.y, +h.z},
    {+h.x, +h.y, +h.z}, {-h.x, +h.y, +h.z},
};

// 3. region 座標へ transform
LLQuaternion rot = obj->getRotationRegion();
LLVector3 pos = obj->getPositionRegion();
for (int i = 0; i < 8; ++i) {
    obb.vertices[i] = (local_verts[i] * rot) + pos;
}

// 4. 6 quads (face indices)
// 各 quad は 4 vertices を時計回り (FMOD 推奨順序)
const int face_indices[6][4] = {
    {0, 3, 2, 1},  // -Z (bottom)
    {4, 5, 6, 7},  // +Z (top)
    {0, 1, 5, 4},  // -Y (front)
    {2, 3, 7, 6},  // +Y (back)
    {0, 4, 7, 3},  // -X (left)
    {1, 2, 6, 5},  // +X (right)
};
```

linkset の子 prim は `getPositionRegion / getRotationRegion` がリンクされた状態の region 座標を返すので、追加の transform 合成は不要。

### 4.4 material 表

SL の prim material flag は `LL_MCODE_*` で 8 種定義 (`indra/llprimitive/llprimitive.h`)。viewer は `LLViewerObject::getMaterial()` で読める。material → occlusion 値の preset 表:

| SL material | preset name | direct | reverb | 用途想定 |
|---|---|---|---|---|
| `LL_MCODE_NONE` (default) | concrete | 0.7 | 0.5 | 一般的な壁 |
| `LL_MCODE_STONE` | stone | 0.9 | 0.7 | 厚い石壁、地下、城 |
| `LL_MCODE_METAL` | metal | 0.85 | 0.6 | 金属隔壁、コンテナ |
| `LL_MCODE_GLASS` | glass | 0.3 | 0.2 | 窓、薄い透明壁 |
| `LL_MCODE_WOOD` | wood | 0.6 | 0.4 | 木製の壁、扉 |
| `LL_MCODE_FLESH` | flesh | 0.4 | 0.3 | カーテン、薄い布 |
| `LL_MCODE_PLASTIC` | plastic | 0.5 | 0.3 | 軽量パーティション |
| `LL_MCODE_RUBBER` | rubber | 0.55 | 0.4 | 防音材、緩衝材 |

direct > reverb の関係は意図的: 反射音 (reverb) は壁を回り込みやすい (低周波数成分が多い) ので、直接音より遮蔽が薄い。値は Steam Audio の material database を参考に決定。r13 リリース後の実機聴感で再 tuning する余地あり (リスク R3)。

タグ引数 (`[ayastorm:occlude:0.8]`) が指定された場合は preset を完全に無視し、direct=reverb=0.8 として登録。r13.x で必要になれば `[ayastorm:occlude:0.8,0.5]` 形式 (direct,reverb 別値) に拡張可能な parser 構造で実装。

### 4.5 lifecycle

#### 4.5.1 静的 occluder (`occlude` タグ)

| イベント | 動作 |
|---|---|
| プリム rez (`LLViewerObjectList::createObject`) | タグ scan → 該当なら OBB 計算 + `addPolygon` ×6 |
| プリム derez | UUID → polygon index map を引いて `setPolygonAttributes(active=false)`、map から削除 |
| プリム move (`getPositionRegion` 変化) | 100ms 以上の遅延で再評価 (壁が動くケースは想定しない、誤動作の防止) |
| Description 編集 | 既存 ObjectProperties 通知経路で再評価、タグ変化なら再登録 |
| material 変化 | 同上 |

scan の頻度: r5-r12 と同じ `Stream3DPollInterval` (default 30s) で範囲内 prim を polling。範囲は `Stream3DOccluderRange` (default 64m、`Stream3DRolloffMax` の上限を考慮)。

#### 4.5.2 動的 door (`door` タグ)

| イベント | 動作 |
|---|---|
| プリム rez/derez/タグ変化 | 静的 occluder と同じ |
| 毎フレーム `LLOcclusionGeometryMgr::update()` | door 登録 prim をすべて反復、`getPositionRegion/getRotationRegion` の変化を検出して `Geometry::setRotation/setPosition` で polygon を再配置 |

door 1 個あたりの毎フレームコスト: prim transform 変化検出 (vector 比較) + FMOD `setRotation/setPosition` 呼び出し各 1 回。SL での扉数は通常 1〜10 個なので毎フレーム 10〜20 API call、無視できる。

しきい値ロジックは入れない (r13 P0 議論で「扉数が現実的に少ないので不要」と判断、AYA 2026-05-10)。

#### 4.5.3 cap

範囲内の occluder + door 合計が `Stream3DOccluderMaxCount` (default 200) を超えた場合:

- listener から距離順にソート
- 近い 200 個まで登録、残りは登録しない
- 範囲外 (Stream3DOccluderRange = 64m 超) の occluder も登録対象外

cap は `update()` poll 時に毎回再評価 (= listener が動くと近い順が変わる、追従)。

### 4.6 r11 venue reverb / r12 upmix との関係

#### 4.6.1 venue reverb (r11) との直交性

§1 / §2.3 で詳述したとおり、venue reverb (r11) と occlusion (r13) は **意図的に直交**。viewer 側で:

- venue reverb タグ (`{venue}` / `{wetgain}`) はスピーカープリム Desc に書く (= 配信者所有)
- occlusion タグ (`[ayastorm:occlude]` / `[ayastorm:door]`) は建物プリム Desc に書く (= 会場運営所有)
- 両者の整合性チェック / 自動補正 / 警告は入れない (NG7)

FMOD geometry の `reverbOcclusion` パラメータは r11 venue reverb の wet send を自動でゲートする:

| 位置 | direct (raycast 経由) | reverb send (raycast 経由) | 体感 |
|---|---|---|---|
| 屋外 | 壁で減衰 | 壁で減衰 | 「遠くで何かやってる、残響も聞こえない」 |
| 扉前 (扉開) | 開口通過 | 開口通過 | 「扉から音 + 室内残響が漏れてくる」 |
| 室内 | 遮蔽なし | 遮蔽なし | 「会場の中、フル venue reverb」 |

#### 4.6.2 upmix (r12) との関係

upmix の出力 6ch は r10 placement の per-channel 配置に流れ、occlusion は placement の channel 出力に対して FMOD geometry が自動 raycast するだけなので、upmix 側に変更ゼロ。「stereo 配信 + upmix on + 室内」のフルチェインも自然に動作。

#### 4.6.3 `llPlaySound` (オブジェクト効果音) への適用

SL のオブジェクト効果音 (`llPlaySound` / `llPlaySoundSlave` 等) も FMOD 3D channel として鳴っているため、occlusion の対象になる。配信者主導モデルとは無関係に、建物内で鳴っている小道具の音 (時計の音、機械音、足音) も建物の外では muffled に聴こえる。

parcel music は位置を持たない (parcel 全体の 2D 音源) ため対象外。voice (Vivox/WebRTC) は別 audio engine を経由し本系統に乗らないため対象外。

### 4.7 debug settings 経由の listener 側 override

平時は **会場運営がプリム Desc に書いた `[ayastorm:occlude]` / `[ayastorm:door]` タグが root truth** として動作する。listener viewer は自動的にタグから値を読み出し、Preferences UI から override する経路は提供しない (NG6)。

実装/検証時の独立 toggle / 微調整用途のみ、debug settings 経由の override を 4 件提供する。`Stream3DOcclusion` のみ sentinel 値 (= 「タグ通り」default) を持ち、残り 3 件はパラメータの実値 default を持つ:

| キー | 型 | default | 効果 |
|---|---|---|---|
| `Stream3DOcclusion` | int | `-1` (sentinel = タグ通り) | `0` = occlusion を強制 OFF (全タグを無視) / `1` = 強制 ON (default 動作と同じ、明示有効化用) |
| `Stream3DOcclusionDirectGain` | F32 | `1.0` | 全 occluder の direct occlusion 値の global multiplier (0.0 = 遮蔽無効、2.0 = 倍掛け) |
| `Stream3DOcclusionReverbGain` | F32 | `1.0` | 全 occluder の reverb occlusion 値の global multiplier |
| `Stream3DOccluderMaxCount` | int | `200` | 範囲内 occluder 登録上限 (cap) |

範囲設定は別カテゴリで:

| キー | 型 | default | 効果 |
|---|---|---|---|
| `Stream3DOccluderRange` | F32 | `64.0` (m) | listener から occluder を scan する範囲。これより遠い occluder は登録対象外 |

**動作優先順位** (先に評価される側ほど強い):

1. `Stream3DOcclusion == 0` → occlusion 強制 OFF (どのタグも無視)
2. プリム Desc タグ `[ayastorm:occlude]` / `[ayastorm:door]` → タグ値を採用
3. タグ未指定 → 該当プリムは occluder にならない (default OFF)

`OcclusionDirectGain` / `OcclusionReverbGain` は global multiplier として常時有効 (sentinel なし)。配信者は触れないし、listener も平時は触らない。実装/検証時の調整専用。

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
| O3 | scene B 扉閉で listener 真正面に立つと muffled、扉開で clear に変化 | 扉 LSL toggle して聴感差 |
| O4 | scene B 扉開、listener 扉斜め前 30° で direct がそこそこ抜ける | 角度を変えながら確認 |
| O5 | scene B 扉開、listener 建物の真横 (扉から離れた壁の外) で muffled 維持 | FMOD は回折しないので壁越し直線が遮蔽される — 仕様通り |
| O6 | material 別 prim (wood/glass/stone) で遮蔽強度の差が出る | 同じ scene A で material 切替 |
| O7 | タグ引数 `[ayastorm:occlude:0.5]` が material 表より優先 | preset wood (0.6) と override (0.5) で差確認 |
| O8 | `Stream3DOcclusion = 0` で occlusion 強制 OFF (タグ全無視) | debug 切替確認 |
| O9 | `Stream3DOcclusionDirectGain = 0.0` で direct 遮蔽消える、reverb は維持 | global multiplier 確認 |
| O10 | `Stream3DOccluderMaxCount` を 5 に下げると 6 個目の壁が透過 | cap 動作確認 |
| O11 | 範囲外 (64m 超) の occluder が登録されない | 距離変えて確認 |
| O12 | door prim を移動 (回転/並進) すると遮蔽位置が即追従 | scripted door で確認 |
| O13 | 配信者主導 r11 venue タグと occlusion が独立動作 (狭箱で野外 venue が許容される) | 不一致組合せで動作確認 |
| O14 | `llPlaySound` (オブジェクト効果音) も occlusion される | 室内に音源 prim、室外で muffled 確認 |

### 6.2 r10 / r11 / r12 互換 (回帰)

- r10 §5.3 受入条件全行が回帰なし (occluder タグなしの環境で完全互換)
- r11 §5.5 受入条件全行が回帰なし (同上)
- r12 §6 受入条件全行が回帰なし (同上)
- 5.1 native 配信 + occlusion (scene A 内に 6 spk 配置) で 6 spk placement と occlusion が両立
- chat font live-apply fix (commit 2689a35f8f) が同梱、LL-style chat で ChatFontSize / PlainTextChatHistory が即時反映

### 6.3 安定性 / CPU

- 5min 連続再生 dropout 0 (3D stream + occlusion ×100 occluder + door ×3 + venue=hall_medium + binaural=on + upmix=on)
- URL 切替 ×10 で crash / 二重再生なし
- prim rez/derez ×20 で geometry leak なし (FMOD geometry polygon 数が安定)
- door prim 連続回転 (5min × 60Hz) で geometry update が遅延なく追従
- CPU 増分: r12 baseline (= occluder ゼロ) との比較で **+2pp 未満** を目標 (FMOD geometry は内部で空間分割済、polygon 200 個は raycast 軽量)

---

## 7. リスク

| ID | 内容 | 縮退策 |
|---|---|---|
| R1 | mesh prim の OBB 近似が「斜め屋根」「アーチ」等で **明確にズレ** て体感に影響 | 縮退 A: 該当 prim だけ `[ayastorm:occlude]` を貼らない、配信会場側のガイドで「平面/box 形状の prim を occluder に使う」運用推奨。r13.x で形状特化近似 (NG2) を昇格検討 |
| R2 | torus prim を occluder に貼られた場合、穴も塞がれて体感がおかしい | 縮退 B: spec / tag-guide に「torus は OBB 近似で穴を再現しない」と明記、運用上の地雷を文書で外す。r14+ Steam Audio で改善 |
| R3 | material 表の preset 値 (concrete=0.7 等) が実機聴感で **強すぎ / 弱すぎ** | 縮退 C: P11 close-out 時に default 値を 0.1 単位で再 tune、リリース後も `Stream3DOcclusionDirectGain` でユーザ側微調整可 |
| R4 | door 動的更新が **prim animation (smooth rotation script) で 60Hz update** され FMOD API call が増える | 縮退 D: door 数の cap を別途設ける (`Stream3DDoorMaxCount` default 16)、超過分は静的扱い |
| R5 | `Stream3DOccluderRange` (64m default) の prim scan が **大規模建造物 (sim 全体に建物)** で重い | 縮退 E: range default を 32m に下げる、または scan 頻度を `Stream3DPollInterval` 60s に延長 |
| R6 | r11 venue reverb との合成で **reverb send が二重ゲート** されてしまい体感弱い | 縮退 F: reverb_occlusion 値を direct より弱め (preset 表で direct > reverb の比率を強化) |
| R7 | listener が **rapid teleport** で範囲を超えて動くと FMOD geometry の再評価が間に合わない | 縮退 G: teleport 検知で全 occluder を強制再 scan、scan 完了まで occlusion を一時停止 |
| R8 | linkset の 1 prim だけにタグを貼った場合の挙動が **会場運営に直感的でない** (linkset 全体ではなく当該 prim のみ occluder) | 縮退 H: tag-guide 改訂時にこの仕様を明記、運用 FAQ 整備 |
| R9 | 既存 `[ayastorm:...]` 名前空間が将来別機能と衝突 | 縮退 I: r13 で `[ayastorm:occlude]` / `[ayastorm:door]` に限定、新機能追加時はタグ命名 review |

---

## 8. 実装フェーズ概要 (詳細は impl record)

詳細フェーズ分解と依存関係は `docs/ayastorm-r13-occlusion.md` を参照。本書では概要のみ:

- **P0**: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査
- **P1**: `LLOcclusionGeometryMgr` skeleton + FMOD `System::createGeometry` lifecycle (新規 mgr クラス、未配線)
- **P2**: タグ parser (`[ayastorm:occlude]` / `[ayastorm:door]` / 引数付き形式) + 範囲内 prim scan
- **P3**: OBB 抽出 helper (`LLOcclusionGeometryHelper` static methods、OBB → 12 triangle)
- **P4**: material 表実装 + タグ override 適用
- **P5**: 静的 occluder lifecycle (rez/derez/move 検知、UUID→polygon index map)
- **P6**: 動的 door 追従 (毎フレーム `setRotation/setPosition`)
- **P7**: cap (Stream3DOccluderMaxCount) + range cap (Stream3DOccluderRange) + listener 距離順ソート
- **P8**: debug settings 4 件 (Stream3DOcclusion sentinel + 3 件 multiplier/cap) 配線
- **P9**: 検証 scene 構築手順ドキュメント化 (`doc/r13/build_test_scene.md`)
- **P10**: 検証実行 (O1〜O14)
- **P11**: r10/r11/r12 回帰確認 + chat font live-apply fix の同梱確認 (commit 2689a35f8f)
- **P12**: CPU benchmark + spec close-out

工数感: **5〜7 日** (実装 4〜5 日 + 検証 1〜2 日)、r12 と同等。

---

## 9. r14+ への持ち越し

旧 r13+ basket から本書策定時に降格 + 本 r13 で発生した持ち越し:

### 9.1 r14: Steam Audio integration (本命の物理音響)

- world geometry を Steam Audio に食わせ **回折 / 反射 / 共鳴** を simulate
- r13 で確立した geometry 登録基盤 (`LLOcclusionGeometryMgr`) の入出力 API を Steam Audio 向けに拡張
- FMOD `set3DOcclusion` (= r13 直線 raycast) を Steam Audio の物理シミュレーションで置換
- venue reverb (r11) との関係: Steam Audio reflection ON 時は r11 convolution reverb を auto disable する条件分岐
- Linux / macOS の Steam Audio binary build 問題 (旧 RR1) を再評価
- **r13 で建てた geometry 登録 / OBB 抽出 / lifecycle / door 動的追従はすべて流用可能** (= r13 投資が無駄にならない)

### 9.2 r14+: 形状精度の昇格

- sphere → icosahedron (20 tri)、cylinder → 16-prism (32 tri) などの shape 特化近似
- タグ引数で精度モード `[ayastorm:occlude:fit]` を選択
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

- material 表 default 値 (R3 縮退) の追加調整
- direct/reverb 個別指定 `[ayastorm:occlude:0.7,0.5]` 形式の検討 (要望次第)
- door しきい値ロジックの追加 (R4 縮退、必要なら)

### 9.9 r13.x: 動的 occluder の壁追従

- 壁 prim が動くケース (建物オーナーの編集中など) でも遮蔽が追従するモード
- r13 では NG4 で対象外、需要が出たら検討

---

## 10. 変更履歴

- 2026-05-10: 初版作成。r12 リリース直後の議論で AYA から「3D stream で prim から音を出せるようになったが、音を遮る/反射する prim を作れないか検討したい」提案 (2026-05-10)。OBB 近似で v1 を出荷、回折 / 反射 / 共鳴は r14+ Steam Audio に保留することを確定。タグ書式 `[ayastorm:occlude]` / `[ayastorm:door]` (引数オプション付き)、material 表 (SL prim material flag → preset) を採用。役割分担として「会場運営が建物タグを貼る、配信者が表現タグ (r11 venue) を貼る、両者は意図的に直交」を明文化 (project memory `project_venue_occlusion_orthogonal.md` 参照)。debug settings 4 件 (sentinel 1 件 + multiplier/cap 3 件) + range 設定 1 件。chat font live-apply fix (commit 2689a35f8f、`feature/ll-chat-livetune-font-plaintext` ブランチ) を r13 同梱バグ修正として組込み。旧 r13+ basket (SOFA / Steam Audio / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air abs 客観 FFT) は r14+ へ繰下げ。
