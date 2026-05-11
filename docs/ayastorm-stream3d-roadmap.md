# AYAstorm 3D Stream 長期ロードマップ (r7 → r13)

**作成日**: 2026-05-03
**対象**: AYAstorm 3D Stream 機能の中長期計画
**位置づけ**: 各リリースの実装工程 / 仕様書とは別軸の、リリース間を貫く全体像と工数感を保持する文書

---

## 1. ねらい

3D Stream 機能を **「分散記述ステレオ」(r8)** から **「lite-HRTF + 会場残響を持つ配信者主導の SL Viewer」(r11)** へ、さらに **「stereo 配信でも 6 spk placement の体験が届く viewer」(r12)**、そして **「SL 世界の物理ジオメトリが音を遮る viewer」(r13)** まで段階的に発展させる。各リリースは独立に完結し、個別にユーザ価値を届けられる構成にする。途中で中断しても価値が積み上がるのが本ロードマップの強み。

---

## 2. アーキテクチャの 4 層モデル

設計の核心は次の **直交した 4 層構造**:

| Layer | 何を解く問題か | 担当リリース |
|---|---|---|
| **Layer 0: ソース整形** | 入力 ch を Layer 1 の前提 (= 6ch) に揃える | r12 (stereo→5.1 upmix) |
| **Layer 1: 配置** | 音源は空間のどこにあるか? | r8 (stereo × N) → r10 (5.1 venue placement) |
| **Layer 2: レンダリング** | ある方向から音が来ることをどう耳で感じさせるか? / 会場感をどう作るか? | r11 (lite-HRTF + venue convolution reverb、配信者主導モデル) |
| **Layer 3: 空間ジオメトリ** | listener と音源の間に物理オブジェクトがあるとどう減衰するか? | r13 (OBB タグベース遮蔽、会場運営主導モデル) |

各層は **競合せず、掛け算で効果**が出る。実物の 5.1 サラウンドでは Layer 2 (HRTF) は頭・耳介・胴の物理現象として、Layer 3 (occlusion) は壁・天井・扉の物理現象として無料で発生するが、SL のヘッドホン視聴ではこれを計算で代替する必要がある。

```
┌──────── Layer 1: WHERE (位置) ────────┐
│ ・mono prim 1 個 → 1 点               │
│ ・stereo prim 2 個 → 2 点              │
│ ・5.1 prim 6 個 → 6 点                 │ ← r8 / r10
└────────────────────────────────────────┘
                  ▼
┌──────── Layer 2: HOW (届き方) ────────────┐
│ ・FMOD 既定: ILD のみ (ITD なし)           │
│ ・lite-HRTF: ITD + ILD shadow + air abs    │ ← r11
│ ・venue convolution reverb (9 種 IR)       │ ← r11
│ (SOFA per-source HRTF / Steam Audio 物理   │
│  シミュは 2026-05-11 議論で永久 drop)      │
└────────────────────────────────────────────┘
                  ▲
┌──────── Layer 0: SOURCE (整形) ────────────┐
│ ・stereo → 5.1 upmix (DPL2 系 + 帯域分離)  │ ← r12
│   matrix decode + LFE LPF + rear decorr    │
│   {upmix:on|off} 配信者タグで制御          │
└────────────────────────────────────────────┘
                  ▲
┌──────── Layer 3: GEOMETRY (空間遮蔽) ──────┐
│ ・viewer 側 segment vs OBB raycast         │ ← r13
│   listener↔音源 line-of-sight で direct/   │
│   reverb 減衰 (per-speaker LOWPASS_SIMPLE) │
│   [ayastorm:occlude{direct:N}{reverb:N}]   │
│   会場運営タグで制御 (配信者タグと直交)    │
└────────────────────────────────────────────┘
```

---

## 3. リリース計画

### r7 (完了): decode thread 化

- リリース: `v7.2.4-ayastorm-r7` (2026-05-02)
- 仕様: `doc/spec_stream3d_decode_thread.md`
- 工程: `docs/ayastorm-r7-stream3d-decode-thread.md`
- 効果: ステレオストリームのデコードを main thread から分離、配信受信中の cark を解消

### r8 (完了): 分散記述ステレオ

- リリース: `v7.2.4-ayastorm-r8`
- 仕様: `doc/spec_distributed_stereo.md`
- 工程: `docs/ayastorm-r8-distributed-stereo.md`
- 主要変更: `[3dstream-stereo:{ch:L|R|M}{range:N}{volume:N}]` で N スピーカー (上限 16) 同期再生
- **r10 への布石 (重要)**: F3 で **ring buffer の track 数を実装定数化せず汎用化**したため、r10 で N=6 への切り替えコストを 5-7 日節約できた

### r9 (完了): 5.1ch ソース受入 + 形式判定

- リリース: `v7.2.4-ayastorm-r9`
- 仕様: `doc/spec_5_1ch_source.md`
- 工程: `docs/ayastorm-r9-5_1ch-source.md`
- 6ch Vorbis/FLAC を内部 L/R downmix で受入、未対応形式 (Opus 等) を失敗分類して再接続抑止
- Opus は FMOD 不支持で実 viewer 未対応 (codec plugin 自作で対応案あり、別軸)、配信は ffmpeg primary

### r10 (完了): 5.1 venue placement

- リリース: `v7.2.4-ayastorm-r10` (+ `r10.x`、`r10.x-bugfix-1`)
- 仕様: `doc/spec_5_1ch_placement.md`
- 工程: `docs/ayastorm-r10-5_1ch-placement.md`、`docs/ayastorm-r10.x-routing-diag-chat.md`
- `ch` 値拡張 (`FL` / `FR` / `C` / `LFE` / `SL` / `SR`)
- source ch 数検証、codec 別 channel mapping table
- 配信側の前提: **Icecast 2.4+ + Opus surround** が事実上唯一の現実解 (Shoutcast は Opus 非対応、AAC は Linux ビルド問題、AC-3 はライセンス障壁)
- **「sweet spot 不在」を仕様で明文化**: 自由視点モデルでは cinematic 5.1 体験ではなく **venue placement モード** (= 6 個の固定スピーカーから 5.1 ソースを撒く PA 的発想)
- LFE は通常スピーカー同等扱い (推奨案 (p))
- r10.x で漏れ commit (r9-opus 等) を再リリース、r10.x-bugfix-1 でレガシー SessionSettingsFile 環境の起動クラッシュ修正

### r11 (実装完了 / リリース判断保留中): lite-HRTF + venue convolution reverb (配信者主導モデル)

- ブランチ: `feature/aya-r11-binaural-venue-reverb` (実装完了、リリース判断保留中)
- 仕様: `doc/spec_binaural_venue_reverb.md` (PR #31, #36 マージ済、本リリースで実装着手)
- 工程: `docs/ayastorm-r11-binaural-venue-reverb.md`
- リリース告知文案: `docs/ayastorm-r11-release-note.md`
- 主要変更:
  - **lite-HRTF DSP** (`lllitehrtfdsp.{h,cpp}` 新設): ITD + ILD shadow + air absorption を per-channel 適用、FMOD 既定 panner (ILD のみ) を反転して肩代わり
  - **venue convolution reverb DSP** (`llvenuereverbdsp.{h,cpp}` 新設): bundled IR 9 種 (`dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor`) で会場残響を Stream3D ChannelGroup 末尾に挿入、`{wetgain}` で wet 強度倍率
  - **配信者主導モデル**: 制御は配信者プリム Desc タグ (`{venue:NAME}` / `{binaural:on|off}` / `{wetgain:N}`) で完結、listener 側 Preferences UI 改修ゼロ。実装/検証用に debug settings 4 件のみ追加 (`Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DUrlPreResolve`、いずれも sentinel 値で「タグ通り / 既定有効」が default)
  - **Stream3D ChannelGroup 分離** (P1): `LLAudioEngine_FMODSTUDIO` に Stream3D 専用 group を追加、両 DSP を group 単位で gate。master volume / mute 伝播は P1 完了直後に検証 (R5)
  - **URL pre-resolve (P10、毛色違い同梱)**: HTTPS→HTTP redirect (Cloudflare/CDN 経由 Shoutcast/Icecast) を libcurl HEAD + FOLLOWLOCATION で事前解決、FMOD netstream のクロスプロトコル redirect 非追従問題を回避 (`indra/llaudio/llstream3durlresolve.{h,cpp}` 新設)
- **ヘッドホン視聴前提** — スピーカー視聴時は配信者が `{binaural:off}` でタグ宣言する運用
- **既存配置の自動恩恵**: r8/r10 で過去に置かれた全 prim が r11 投入時点で自動的にリッチ化される (ユーザの再配置不要)
- **r5 NG1 (HRTF/binaural 不要) を反転**: r10 で 5〜30m 会場モデル + 6 spk 同時定位に拡張された結果、ITD なし / air abs なし / 会場残響なしの薄さが体感的に目立つようになったため (詳細は `doc/spec_binaural_venue_reverb.md` §2.3)
- **SOFA per-source HRTF / Steam Audio は r13+ に降格**: 配布負債 (3 OS binary)、CPU 負荷 (16 spk × HRTF × 4 stream)、個人 SOFA 問題 (非個人 SOFA は逆効果になり得る) が大きく、AYAstorm 主用途 (virtual live venue / ヘッドホン視聴) で実際に効く順序を再評価した結果、優先度を「会場感 ≫ ITD/ILD > HRTF」と判断 (詳細は同 §2.2)。**当初は r12 main を想定していたが、r11 完了後の議論で更に r13+ へ降格** (理由は r12 entry 参照)

### r12 (完了): stereo→5.1 upmix (Layer 0 整形、配信者主導モデル維持)

- リリース: r12 main (PR #46) / r12.1 (PR #52、`{lfegain}` 追加 + wetgain default 1.0→0.2 + listener side debug live-tuning 回帰修正)
- ブランチ: `feature/aya-r12-stereo-upmix` / `feature/r12.1-lfegain`
- 仕様: `doc/spec_stereo_upmix.md`
- 工程: `docs/ayastorm-r12-stereo-upmix.md`
- 主要変更:
  - **stereo→5.1 upmix helper class** (`llstereoupmix.{h,cpp}` 新設、`LLMultichannelDownmix` 並行構造): 2 track ring から speaker 役割 (FL/FR/C/LFE/SL/SR) ごとに 1ch を生成、DPL2 系 matrix decode + center bleed 除去 + rear decorrelation + LFE LPF (Butterworth biquad、80Hz default)。**`SpeakerCallback::OpKind::Upmix` 拡張** (r10 Bs775 dispatch の対称構造) で `pcmReadCallback` から呼ぶ — P0 調査で FMOD DSP 経路 (A/B 案) は不適合と判明、C 案として確定 (詳細は `doc/r12/dsp_insertion_survey.md`)
  - **配信者タグ `{upmix:on|off}`** 追加 (default `off`): 配信者が opt-in した瞬間から 6 spk placement の体験を獲得
  - **5.1 native 配信の auto bypass**: source ch>=6 のとき `{upmix:on}` でも DSP 非挿入、chat 通知 1 回。二重処理防止
  - **debug settings 4 件**: `Stream3DUpmix` (sentinel `-1` = タグ通り) + パラメータ微調整 3 件 (`Stream3DUpmixLfeCutoff` 80Hz / `Stream3DUpmixCenterBleed` 1.0 / `Stream3DUpmixRearDelayMs` 16ms)。listener 平時は不使用、実装/検証用
  - **データフロー順序**: source stream → mRing (per-track) → `pcmReadCallback` `OpKind::Upmix` dispatch (r12) → per-channel placement (r10) → lite-HRTF (r11) → Stream3D group → venue reverb (r11)
- **アルゴリズムは決め打ち** (DPL2 系 matrix decode + 帯域分離): 配信者にも listener にも選ばせない (r5 / r11 流儀)。Logic 7 / SRS / ML 系は r13+ で再検討
- **既存配置の自動恩恵**: r8/r10 で過去に置かれた全 prim は、配信者が `{upmix:on}` を明示的に追加した瞬間から 6 spk placement の体験を得る (再配置不要)
- **r10/r11 受入条件すべて維持**: dropout / CPU / URL 切替 / 互換マトリクス / 回帰、すべて r11 から劣化なし
- **設計判断 — 旧 r12 計画 (SOFA + Steam Audio) からの再定義**: 「世間の SL 配信はほぼ stereo、6 spk placement の元を取りたい」(AYA、2026-05-07) という認識転換が起点。r10 で 6 spk placement を、r11 で venue reverb と lite-HRTF を載せたフルチェインを **5.1 配信前提でしか体感できない** という機会損失を、viewer 内 DSP で埋める。配布負債ゼロ + 既存配信全部に効く + r10/r11 投資の元を取る、の三拍子で ROI が最大 (詳細は `doc/spec_stereo_upmix.md` §1 / §2.2)
- **配信者主導モデルの維持**: r11 で確立した「配信者プリム Desc タグ = root truth、listener UI 改修ゼロ」を継承。新タグは 1 種 (`{upmix}`) のみ追加、debug settings は sentinel + 微調整 3 件で計 4 件

### r13: OBB タグベース遮蔽 (Layer 3 空間ジオメトリ、会場運営主導モデル + chat font 同梱)

- ブランチ: `feature/r13-occlusion-spec-draft` → `feature/aya-r13-obb-occlusion` (実装ブランチ、spike 完了済 / 残工程進行中)
- 仕様: `doc/spec_obb_occlusion.md` (final scope 確定 2026-05-11)
- 工程: `docs/ayastorm-r13-occlusion.md` (§5 が canonical な実装ログ)
- 同梱バグ修正: cherry-pick `d66bdb74fc` (元 `feature/ll-chat-livetune-font-plaintext` の `2689a35f8f`、ChatFontSize / PlainTextChatHistory live-apply on LL-style chat)
- 主要変更:
  - **新規 mgr クラス** `LLOcclusionGeometryMgr` (`indra/newview/llocclusiongeometrymgr.{h,cpp}` 新設): タグ parse + OBB レジストリ + 毎 tick 全件 transform 追従 + segment vs OBB slab test + `Channel::set3DOcclusion` + per-speaker `LOWPASS_SIMPLE` cutoff の同時適用を専担、`LLPositionalStreamMgr` と完全独立な singleton
  - **会場運営タグ** `[ayastorm:occlude]` 1 種のみ追加: 引数なしは hardcoded default (direct 0.7 / reverb 0.5)、`[ayastorm:occlude{direct:N}{reverb:N}]` で per-prim 個別値を上書き可。`[ayastorm:door]` 専用タグは設けない (`refreshOccluders` の毎 tick 全件追従で扉動作も成立)
  - **形状判定は OBB pre-cull + 実プリム三角形 raycast の 2 段** (P15 で OBB 単独から昇格): listener-source segment を全 occluder の OBB と先に slab test し (~95% reject)、ヒットした OBB のみ `LLVolume::getVolumeFace` から抽出した三角形 (Möller-Trumbore) に対して raycast。path cut / hollow / mesh の実形状が遮蔽計算に反映され、斜め屋根 / アーチ / 曲面 / ドーナツ穴抜けも正しく扱える。`kMaxTrisPerOccluder = 2000` 超過時は OBB-only にフォールバック (LL_WARNS_ONCE、典型 SL 建築は範囲内)。Steam Audio による回折/反射/共鳴の物理シミュは 2026-05-11 議論で永久 drop、SOFA per-source HRTF も同日永久 drop
  - **occlusion 値は default + per-prim override の 2 層** (material 表は採用しない): `LL_MCODE_*` → preset 写像は実機聴感の根拠が薄く、tag-guide で推奨セット (石壁 0.9/0.7 / 木壁 0.6/0.4 / ガラス 0.3/0.2) を提示する運用に振る
  - **lifecycle**: タグ付与は `onObjectPropertiesReceived` で OBB 登録、`refreshOccluders` が毎 tick 全件 `getPositionGlobal` / `getRotationRegion` / `getScale` を再評価して center/half/rot を上書き、消えた prim を drop。扉のような動的プリムも同経路で自動追従
  - **適用先音源**: 3D stream prim (r5-r12 系) + `llPlaySound` (オブジェクト効果音、残工程で SFX channel へ visitor 経路拡張)。parcel music (位置を持たない 2D) と voice (Vivox/WebRTC、別 audio engine) は対象外
  - **debug settings 2 件**: `Stream3DOcclusion` (sentinel `-1` = タグ通り、`0` = 強制 OFF) + `Stream3DOccluderRange` (64m、距離 cull)。`*DirectGain` / `*ReverbGain` / `OccluderMaxCount` 設定化は永久 drop (per-prim タグ + hardcoded `kMaxOccluders = 256` で代替)。既出荷 2 件 (`Stream3DOcclusionRampMs` 250ms / `Stream3DShowOccluders` Alt+Shift+O) と合わせて計 4 件
  - **データフロー**: source → mRing → pcmReadCallback (OpKind dispatch) → **r13: per-speaker `LOWPASS_SIMPLE` (cutoff = direct 値の 22kHz→300Hz exponential mapping)** → r11 LiteHrtfDsp → Channel built-in panner → **r13: viewer 側 OBB pre-cull + Möller-Trumbore 三角形 raycast → `Channel::set3DOcclusion` 直接 set** → Stream3D group → r11 VenueReverbDsp (reverb send が reverbOcclusion でゲートされる)。LOWPASS_SIMPLE は per-speaker channel に挿入されるだけで lite-HRTF / venue reverb / placement / upmix は occlusion の存在を意識しない
  - **歴史的経緯**: P1 着手時に同梱 `libfmod 2.03.07` の `System::createGeometry` が `FMOD_ERR_INTERNAL` を返し機能しないことが判明、当初計画の FMOD geometry API 経路を放棄して viewer 側 segment vs OBB slab test に pivot (memory `project_fmod_geometry_unavailable.md`)
- **会場運営主導モデルの新規導入** (r11 配信者主導モデルの対パターン):
  - 建物オーナー (= 壁/扉を建てる人) と配信者 (= ストリームを流す人) を **別人格** として扱う
  - 建物の物理 (occlusion) は会場運営所有、表現キャラクタ (r11 venue reverb) は配信者所有
  - 両者は viewer 側で **意図的に直交** (整合性チェック / 自動補正 / 警告は入れない)
  - 「狭い箱の中で野外 venue」「屋外で cathedral venue」のような物理と表現の不一致を仕様として許容 (詳細は memory `project_venue_occlusion_orthogonal.md`)
- **chat font live-apply fix 同梱**: 単独 release を切るほどではないバグ修正は次の planned release の train に乗せる方針。cherry-pick `d66bdb74fc` (元 `feature/ll-chat-livetune-font-plaintext` の `2689a35f8f`) を r13 ブランチに merge
- **起動時 unresponsive dialog の根本対策同梱 (C)**: r13 spike 中に並行発生した別案件として、libcurl HEAD pre-resolve の同期ブロックを完全非同期化 (`LLStream3DUrlResolve` worker thread + request-id ベース API、`LLPositionalStreamMulti` 状態機械を `Idle → Resolving → Opening → Buffering → Playing → Failed` に拡張、commit `f336d43abc` + `5c3487ff06`)。Linux ビルドで踏んだ X11 `#define Status int` 罠は memory `project_linux_xlib_status_define_trap.md` に記録
- **既存配置の自動恩恵**: r5-r12 で過去に置かれた全 stream prim は、会場運営が occlusion タグを建物に貼った瞬間から遮蔽の恩恵を受ける (stream 側再配置不要)
- **r10/r11/r12 受入条件すべて維持**: dropout / CPU / URL 切替 / 互換マトリクス / 回帰、すべて r12 から劣化なし
- **設計判断 — 旧 r13+ basket からの再定義**: 「3D stream を実装してプリムから音を出せるようになったが、音を遮る/反射する prim を作れないか検討したい」(AYA、2026-05-10) という提案が起点。OBB 単独で出荷、Steam Audio (回折/反射/共鳴) は r14+ に保留することで、SL viewer 史上初の空間音響遮蔽機能を viewer 内 DSP 完結で実現。配布負債ゼロ + 既存配置全部に効く + r14+ Steam Audio の geometry 登録基盤として再利用可能、の三拍子 (詳細は `doc/spec_obb_occlusion.md` §1 / §2.2)
- **SL viewer 史上初の空間音響遮蔽**: 3D stream + r10 5.1ch placement + r11 venue reverb + lite-HRTF + r12 upmix + r13 occlusion で、AYAstorm は「リアル音響体験を SL で構築する viewer」として完成形に近づく

---

## 4. 工数見積り

「集中作業日換算」と「個人プロジェクトペース暦週」の二軸。

| リリース | 集中工数 | 個人ペース暦週 | 不確実性 | 実績 |
|---|---|---|---|---|
| r8 | 14-25 日 | 5-7 週 | 中 (F3 ring 設計次第) | 実績 ~2 日 (memory: F3 汎用化が短工数で完了) |
| r9 | 0-3 日 | 0-2 週 (F6 吸収可) | 低 | 完了 (独立リリース化) |
| r10 | 6-11 日 | 2-3 週 | 低-中 (r8 設計依存) | 実績 4 日 |
| r11 | 9.5-10.5 日 | 3-5 週 | 中 (R5 master volume 伝播 / IR ライセンス / lite-HRTF 体感) | 実装完了 (リリース判断保留中) |
| r12 | 5-7 日 | 1-2 週 | 低-中 (R6 DSP 挿入位置 A/B 判断 / R2-R4 default 値の聴感調整) | 完了 (PR #46 + r12.1 PR #52) |
| r13 | 5-7 日 | 1-2 週 | 中 (R1 mesh OBB ズレ / R3 default 値 tuning / R7 rapid teleport) | spike 完了 / 残工程進行中 |
| **合計** | **r8-r12 実績 + r13 計画 ~6 日** | **r13 のみ約 1-2 週** | - | - |

### r8 の内訳
- F1 パーサ拡張: 1-2 日
- F2 linkset 集約 + 子 prim 監視: 3-5 日
- F3 `LLPositionalStreamMulti` 新設: 5-10 日 (★最大)
- F4 throttle 通知: 1-2 日
- F5 設定追加: 0.5 日
- F6 実機ベンチ: 3-5 日

### r10 の内訳
- `ch` enum 拡張: 1 日
- source ch 数検証: 1-2 日
- codec 別 channel mapping table: 1-2 日
- LFE 扱い実装: 0-1 日 (推奨案なら実質ゼロ)
- N-track ring 拡張: 0.5-1 日 (r8 で汎用化済み前提)
- 仕様書整備: 1 日
- 実機ベンチ: 2-3 日

### r11 の内訳

詳細は `doc/spec_binaural_venue_reverb.md` §11 (実装フェーズ) / §12 (工数見積)。

- P0 仕様確定 + roadmap doc 同時更新: 着手中 (本コミット)
- P1 Stream3D ChannelGroup 分離 + R5 master volume 伝播確認: 0.5 日
- P2-P4 LiteHrtfDsp 実装 (骨格 → ITD+ILD shadow+air abs → per-frame param): 2.5 日
- P5 `{binaural}` タグ + debug `Stream3DBinauralRender` + hook 反転: 0.5 日
- P6-P7 VenueReverbDsp + bundled IR (9 種) + CREDITS.md: 3 日
- P8 `{venue}` タグ + debug `Stream3DVenueOverride`: 0.5 日
- P9 `{wetgain}` タグ + debug `Stream3DVenueWetGain` + group 末尾挿入: 0.5 日
- P10 URL pre-resolve (HTTPS→HTTP redirect 解決): 0.5 日 (P0 後に独立並行可)
- P11 検証材料生成 + AYA Icecast hosting: 0.5 日
- P12-P14 検証 + r8/r9/r10 回帰 + CPU ベンチ + 仕様クローズ: 2 日
- 合計 9.5〜10.5 日 (Preferences UI 改修ゼロ、debug settings 4 件のみ)
- リスク発火時の振れ幅: 最速 7 日 / 平均 9.5-10.5 日 / 最悪 13.5 日

### r12 の内訳

詳細は `doc/spec_stereo_upmix.md` §8 / `docs/ayastorm-r12-stereo-upmix.md` §2 (実装フェーズ)。

- P0 仕様確定 + roadmap doc 同時更新 + 実装箇所調査 (A 案 / B 案): 0.5 日
- P1 StereoUpmixDsp skeleton (2ch passthrough、未配線): 0.5 日
- P2 Matrix decode + center bleed 除去 + rear decorrelation: 1 日
- P3 LFE LPF (Butterworth biquad): 0.5 日
- P4 source ch 数判定 + auto bypass: 0.5 日
- P5 `{upmix:on|off}` タグ parser + `Stream3DUpmix` debug 配線: 0.5 日
- P6 debug settings 3 件 (LfeCutoff / CenterBleed / RearDelayMs) 配線: 0.5 日
- P7 検証材料生成スクリプト: 0.5 日
- P8 配信者 LSL に Upmix UI 追加: 0.5 日
- P9 検証実行 (U1〜U9): 0.5-1 日
- P10 r10/r11 回帰確認: 0.5 日
- P11 CPU benchmark + spec close-out: 0.5 日
- 合計 5〜7 日 (Preferences UI 改修ゼロ、debug settings 4 件のみ、r11 と同等の改修コスト感)
- リスク発火時の振れ幅: 最速 4 日 / 平均 5-7 日 / 最悪 10 日 (R6 で DSP 挿入位置を P1 着手後に切替する場合 +2-3 日)

### r13 の内訳

詳細は `doc/spec_obb_occlusion.md` §8 / `docs/ayastorm-r13-occlusion.md` §5 (実装ログ canonical)。

**spike 完了済 (~3 日)**:
- P0 仕様確定 + roadmap doc 同時更新 + 実装箇所調査 (FMOD geometry API → 同梱 `libfmod 2.03.07` の `createGeometry` が `FMOD_ERR_INTERNAL` を返し機能不全と判明、viewer 側 segment vs OBB raycast に pivot、memory `project_fmod_geometry_unavailable.md`)
- P1 `LLOcclusionGeometryMgr` skeleton + タグ parser (`[ayastorm:occlude]` bare、`onObjectPropertiesReceived` で OBB 登録)
- P2 OBB 抽出 + segment vs OBB slab test + multiplicative 累積 (`final = 1 - prod(1 - direct_i)`) + per-speaker `LOWPASS_SIMPLE` cutoff (22kHz→300Hz exponential mapping) 配線
- P3 occluder lifecycle (rez/derez/move/Description 編集 + UUID→OBB map + `refreshOccluders` 毎 tick 全件 `getPositionGlobal` / `getRotationRegion` / `getScale` 再評価で center/half/rot 上書き、扉のような動的プリムも同経路で自動追従)
- P4 既出荷 debug 2 件 (`Stream3DOcclusionRampMs` 250ms ramp / `Stream3DShowOccluders` Alt+Shift+O hotkey で OBB wireframe 可視化)
- P5 同梱バグ修正 cherry-pick `d66bdb74fc` (元 `feature/ll-chat-livetune-font-plaintext` の `2689a35f8f`、ChatFontSize / PlainTextChatHistory live-apply on LL-style chat)
- P6 起動 unresponsive dialog 根本対策 (`LLStream3DUrlResolve` worker thread + request-id ベース API、`LLPositionalStreamMulti` 状態機械を `Idle → Resolving → Opening → Buffering → Playing → Failed` に拡張、commits `f336d43abc` + `5c3487ff06`、X11 `#define Status int` 罠は memory `project_linux_xlib_status_define_trap.md` に記録)

**残工程 (~3-4 日)**:
- P7 per-prim override args parser (`[ayastorm:occlude{direct:N}{reverb:N}]`、tag-guide 推奨セットの調整余地を per-prim で確保): 0.5 日
- P8 `llPlaySound` occlusion 適用 (オブジェクト効果音、SFX channel に visitor 経路拡張): 0.5 日
- P9 残 debug settings 2 件 (`Stream3DOcclusion` master sentinel `-1`/`0` + `Stream3DOccluderRange` 64m range cull) 配線 + `kMaxOccluders` 64 → 256 hardcode 変更: 0.5 日
- P10 検証 scene 構築手順 (`doc/r13/build_test_scene.md`、推奨セット 石壁 0.9/0.7 / 木壁 0.6/0.4 / ガラス 0.3/0.2 の聴感確認用): 0.5 日
- P11 検証実行 (O1〜O12) + r10/r11/r12 回帰確認 + chat font live-apply 同梱確認: 1 日
- P12 tag-guide ja/en/zh 加筆 (`[ayastorm:occlude]` 1 種、bare + `{direct:N}{reverb:N}` override) + Release Notes (詳細資料リンクのみ): 0.5 日
- P13 CPU benchmark + spec close-out: 0.5 日

- 合計 5〜7 日 (Preferences UI 改修ゼロ、debug settings 既出荷 2 件 + 残工程追加 2 件で計 4 件、`[ayastorm:occlude]` タグ 1 種のみ、material 表は採用しない、`[ayastorm:door]` 専用タグも作らない、r11/r12 と同等の改修コスト感)
- リスク発火時の振れ幅: 最速 4 日 / 平均 5-7 日 / 最悪 9 日 (R1 で形状特化近似を r13 内に取り込む場合 +2 日、R3 で default 値 / 推奨セット再 tune が必要 +0.5 日、R7 で teleport hook 追加 +0.5 日)

---

## 5. リスクと圧縮策

### 主要リスク

| ID | 内容 | 対象 |
|---|---|---|
| ~~RR1~~ | ~~Steam Audio + FMOD Studio 2.02 の Linux 動作確認が薄い領域。詰まるとリリース全体停止~~ | **解消 (2026-05-11)**: Steam Audio integration / 回折・反射・共鳴の物理シミュは永久 drop。反射/共鳴は r11 convolution venue reverb (9 IR) で先取り表現済、回折は r13 OBB occlusion の lowpass+減衰で知覚的に近似。3 OS binary 配布負債 + engine 依存をゼロに戻す |
| ~~RR2~~ | ~~KU100.sofa など個人 SOFA の入手元によっては再配布禁止。viewer 同梱可否を要調査~~ | **解消 (2026-05-11)**: SOFA per-source HRTF / 個人 HRTF は永久 drop。r11 lite-HRTF (ITD + ILD shadow + air abs) で AYAstorm の目指す音響リアリティ閾値は越えた、CPU 高 × 個人測定済ユーザ限定の ROI 薄、再配布ライセンス調査負債を回避 |
| ~~RR3~~ | ~~16 spk × HRTF 畳み込み × 4 stream 並走で CPU 想定超なら DSP プールや音源数制限が必要~~ | **解消 (2026-05-11)**: SOFA per-source HRTF を永久 drop した結果として消失 (r11 lite-HRTF は per-channel × ITD/ILD shadow/air abs のみで畳み込みなし、CPU 軽量) |
| RR4 | r8 F3 で N-track ring 汎用化を怠ると r10 で再設計コスト発生 (5-7 日) | r8 → r10 (回避済) |
| RR5 | 配信側コミュニティの Icecast 移行が進まないと r10/r11 の価値が活かしきれない | r10 (運用面) |
| RR6 | r11 P1 で Stream3D ChannelGroup 分離後、master volume / mute / 全体 setVolume が Stream3D group に伝播しない → 縮退 C (group 分離撤回 + per-channel addDSP) | r11 (実機検証で回避済) |
| RR7 | r11 bundled IR (9 種) のライセンス確認が時間掛かる → 縮退 D (1〜2 種に絞って r11 出荷、残りは r11.x) | r11 (OpenAIR CC-BY 4.0 で 8 IR 確保、回避済) |
| RR8 | r11 lite-HRTF (ITD+ILD shadow) の体感が薄い → 縮退 A (default off + opt-in 化) | r11 (主観 PASS で default on のまま出荷) |
| RR9 | r11 venue reverb の CPU が +10pp 超 → 縮退 B (IR 上限 3s→2s、`hall_large`/`cathedral` mono 化、9→5 venue) | r11 (hall_medium 以上で +8〜10pp 、絶対値・dropout 共に問題なく ship-with-note 判断) |
| ~~RR10~~ | ~~r12 P0 で StereoUpmixDsp 挿入位置 (A 案: stream-level group 入力段 / B 案: per-stream / per-binding) を確定できず P1 着手後に切替が必要~~ | **解消 (2026-05-07)**: r12 P0 第 2 弾で実コード調査の結果、A 案 / B 案ともに不適合 (per-speaker channel が mono、Stream3D group は source 2ch を見えない) と判明。代わりに **C 案 = `SpeakerCallback::OpKind::Upmix` 拡張** (r10 Bs775 dispatch の対称構造) として確定 (`doc/r12/dsp_insertion_survey.md`)。Bs775 と並行構造で実装難易度小、P1.5 不要 |
| **RR11** | **r12 DPL2 系 matrix decode の phase 依存性が想定以上に強く、特定の stereo 素材 (vocal が片側 only の cinematic mix 等) で center 抽出が不自然** → 縮退 (アルゴリズム多択化はせず、`{upmix:off}` を配信者がタグで明示することで個別 stream を回避) | r12 |
| **RR12** | **r12 debug settings 3 件 (LfeCutoff / CenterBleed / RearDelayMs) の default 値が P11 検証で範囲超えで再 tune 必要** → P11.x として default 調整 phase を追加 (+0.5 日) | r12 |
| **RR13** | **r12 source ch 判定が stream 開始タイミングで間に合わない** (codec layer の遅延) → ch 数判定 timeout を設定、判定不可なら upmix 無効 (= 安全側、5.1 として誤動作させない) | r12 |
| ~~RR14~~ | ~~r13 mesh prim の OBB 近似が「斜め屋根 / アーチ」で明確にズレ、体感ミスマッチ~~ | **解消 (2026-05-11、P15)**: r13 内で OBB 近似 → 実プリム三角形 raycast (OBB pre-cull + Möller-Trumbore) に昇格。`LLVolume::getVolumeFace` から path cut / hollow / mesh の実形状を抽出して segmentHitsShape で判定するため斜め屋根 / アーチ / 曲面でも正確に遮蔽。kMaxTrisPerOccluder=2000 超過時は OBB-only にフォールバック (LL_WARNS_ONCE)、典型 SL 建築は範囲内 |
| **RR15** | **r13 default 値 (direct 0.7 / reverb 0.5) と tag-guide 推奨セット (石壁 0.9/0.7 / 木壁 0.6/0.4 / ガラス 0.3/0.2) が実機聴感で強すぎ/弱すぎ** → 縮退 B: P13 close-out 時に default 値 / 推奨セットを 0.1 単位で再 tune、リリース後も per-prim タグ `{direct:N}{reverb:N}` override で会場運営側が個別微調整可 (`*DirectGain` / `*ReverbGain` 設定化は永久 drop で代替) | r13 |
| ~~RR16~~ | ~~r13 door 動的更新が prim animation (smooth rotation script) で 60Hz update され FMOD API call が増える~~ | **解消 (2026-05-11)**: 当初の `[ayastorm:door]` 専用タグ + 動的追従経路を廃し、`refreshOccluders` の毎 tick 全件 transform 再評価に一本化。扉専用 cap (`Stream3DDoorMaxCount`) は不要、occluder 全体の `kMaxOccluders=256` cap で吸収。同梱 `libfmod 2.03.07` の geometry API 機能不全により FMOD API call そのものが消えた (segment vs OBB slab test は viewer 側、CPU 軽量) ため raycast 側のリスクとしても発火しない |
| **RR17** | **r13 listener が rapid teleport で範囲を超えて動くと viewer 側 OBB レジストリの再評価が間に合わない** → 縮退 D: teleport 検知で全 occluder を `Stream3DOccluderRange` 内で強制再 scan、scan 完了まで occlusion を一時停止 | r13 |
| **RR18** | **r13 大規模建造物 (sim 全体に建物) で Stream3DOccluderRange (64m default) の prim scan が重い** → 縮退 E: range default を 32m に下げる、または scan 頻度を `Stream3DPollInterval` 60s に延長 | r13 |

### 工数圧縮の選択肢

1. **r9 を独立リリースしない** (r8 F6 で Opus/FLAC URL を一緒に試す) → 1-2 週節約 [採用せず: 形式判定の独立価値が高く r9 として切り出し]
2. **r8 F3 で N-track ring 汎用化を仕込む** (仕様書 §4.5 の布石を実装で履行) → r10 で 5-7 日節約 [採用、r10 で実証]
3. **r11 で Steam Audio / SOFA を不採用、lite-HRTF + venue reverb で代替** → 配布負債 (3 OS binary) 回避、r11 工数を 19-33 日 → 9.5-10.5 日へ大幅圧縮、Linux 沼回避 [採用、本書策定で確定]
4. **r11 で Preferences UI 改修ゼロ、配信者主導モデルで debug settings 4 件のみ** → 0.5 日節約、一般 listener UI 増殖を回避、配信者主導モデル (タグ root truth) と整合 [採用]
5. **r11 で `{binaural}` default off + opt-in 化** → lite-HRTF 体感が薄い場合の縮退 A (リリース判断時、結果として default on で出荷)
6. **r12 を「stereo upmix のみ」に絞り、SOFA / Steam Audio / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air absorption 客観 FFT は r13+ へ降格** → r12 工数を 4-8 週 → 1-2 週へ大幅圧縮、配布負債ゼロ、r10/r11 投資の元を取る ROI 最大 [採用、2026-05-07 議論で確定]
7. **r12 アルゴリズムを DPL2 系 matrix decode + 帯域分離で決め打ち** (Logic 7 / SRS / ML 系は r13+) → 配信者にも listener にも選ばせない (= 表現の不確定性を増やさない、r5 / r11 流儀)、実装コスト最小化 [採用]
8. **r12 配信者タグは `{upmix:on|off}` の 1 種のみ**、debug settings は sentinel + 微調整 3 件で計 4 件 → タグ多択化を回避、r11 と同等の改修コスト感に収める [採用]
9. **r13 形状近似を OBB 単独で決め打ち** (sphere/cylinder/torus も box 近似、形状特化近似は r14+): 建築用途の 98% で十分、実装コスト最小。配信者にも会場運営にも形状モードを選ばせない [採用、2026-05-10 議論で確定。当初は「r14+ Steam Audio で OBB 基盤を流用可」を根拠の一つにしていたが、Steam Audio は 2026-05-11 議論で永久 drop (項 13)。OBB レジストリ基盤は r14+ で viewer 側 mesh raycast 経路に流用する形に変更]
10. **r13 を「OBB タグベース遮蔽 + chat font 同梱」のみに絞り、Steam Audio / SOFA / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air abs 客観 FFT は r14+ へ降格** → r13 工数を 数週 → 1-2 週に大幅圧縮、配布負債ゼロ、SL viewer 史上初の空間音響遮蔽機能を最短で出荷 [採用、2026-05-10 議論で確定]
11. **r13 chat font live-apply fix を同梱**: 単独 release を切るほどではないバグ修正は次の planned release の train に乗せる方針。`feature/ll-chat-livetune-font-plaintext` (commit 2689a35f8f) を r13 にマージ、独立リリース工数 (verify / release-note / 3 OS build) を節約 [採用]
12. **r13 タグは `[ayastorm:occlude]` 1 種のみ** (bare で hardcoded default、`{direct:N}{reverb:N}` で per-prim override)、debug settings は既出荷 2 件 (`Stream3DOcclusionRampMs` / `Stream3DShowOccluders`) + 残工程追加 2 件 (`Stream3DOcclusion` master sentinel + `Stream3DOccluderRange` 64m range cull) で計 4 件。`[ayastorm:door]` 専用タグ + material 表 + `*DirectGain`/`*ReverbGain`/`OccluderMaxCount` 設定化は永久 drop (`refreshOccluders` 毎 tick 全件追従 / tag-guide 推奨セット / per-prim タグ override / `kMaxOccluders=256` hardcode で代替)。`[ayastorm:...]` プレフィクスで viewer 物理タグ系統を `[3dstream...]` 配信タグ系統と分離 [採用、2026-05-11 final scope 確定]
13. **SOFA per-source HRTF / Steam Audio integration / 回折・反射・共鳴の物理シミュ / 個人 HRTF を r14+ 候補から永久 drop** → r14+ の対象を「VenueReverb CPU 最適化 / 公開 README / air abs 客観 FFT / venue IR ユーザアップロード / アルゴリズム多択化」に絞り込む。根拠: (a) r11 lite-HRTF (ITD + ILD shadow + air abs) で AYAstorm の目指す音響リアリティ閾値は越えた、(b) 反射/共鳴は r11 convolution venue reverb (9 IR) で先取り表現済、回折は r13 occlusion の lowpass+減衰で知覚的に近似、(c) Steam Audio engine の存在意義 (SOFA も drop した今) もほぼ消失、(d) 3 OS binary 配布負債 / engine 依存 / listener UI 増殖 / 再配布ライセンス調査負債をゼロに戻す。物理シミュは「reverb 拡張で目的達成を試したあとの『さらに』段階」に再検討 [採用、2026-05-11 議論で確定。なお当初は「形状特化近似 viewer 側 mesh raycast」を r14+ 候補に含めていたが項 14 で r13 内に取り込んだため除外]
14. **r13 内で OBB 近似 → 実プリム三角形 raycast に昇格 (P15)** → 当初 r13 spec では形状特化近似 / mesh raycast は r14+ 候補に置いていたが、AYA の「PrimのMeshExport機能で形状確定できる」洞察を起点に LLVolume::getVolumeFace 由来の三角形を OBB pre-cull + Möller-Trumbore で raycast する経路を実装。OBB 256-prim cap + 64m range cull で CPU 影響は OBB-only 版と同オーダー、path cut / hollow / mesh の実形状が遮蔽計算に反映され「ドーナツの穴を抜ける音は通る」直感を満たす。build floater 選択中プリムは `refreshOccluders` で毎 tick 再抽出してライブ追従。これにより r14+ から「形状特化近似 viewer 側 mesh raycast」項を消化、項 13 の r14+ スコープから当該項を除外 [採用、2026-05-11 議論で確定、P15.1〜P15.5 実装済]

---

## 6. 依存関係

```
r7 (done)
  └→ r8 (done)
       ├→ r9 (done — 独立リリース化)
       └→ r10 (done)
            └→ r10.x / r10.x-bugfix-1 (done)
                 └→ r11 (完了)
                      └→ r12 main (完了 PR #46)
                           └→ r12.1 (完了 PR #52)
                                └→ r13 (current — タグベース occlusion: 実プリム mesh raycast + chat font 同梱)
                                     └→ r14+ (VenueReverb CPU 最適化 / 公開 README / air absorption 客観 FFT / venue IR ユーザアップロード / アルゴリズム多択化)
                                          ※ Steam Audio (回折/反射/共鳴の物理シミュ) と SOFA per-source HRTF / 個人 HRTF は 2026-05-11 議論で永久 drop
                                          ※ 形状特化近似 / mesh raycast は r13 P15 で取り込み済 (r14+ から除外)
```

- **r8 → r10**: F3 の N-track ring 汎用化に強く依存
- **r10 → r11**: 独立に着手可 (Layer 1 と Layer 2 が直交)。だが順序的に r10 を先にした利点:
  - r10 が安定動作している状態で r11 の実機試聴を行うことで lite-HRTF / venue reverb の効果が分かりやすい
  - r10.x-bugfix-1 まで含めた安定化後に CPU ヘッドルームが把握できているので、r11 の DSP 2 個追加の余地評価が容易
- **r11 → r12**: r12 は **r10/r11 完了が前提**。stereo→5.1 upmix の出力 6ch を r10 placement にそのまま流し、r11 lite-HRTF / venue reverb がその上に乗る構造のため、r10/r11 が安定動作していないと r12 単体評価が成立しない
  - r12 DSP 挿入位置は **r10 placement の前段** (= Layer 0)、placement 側の改修ゼロ
  - 5.1 native 配信 (source ch>=6) は r12 を auto bypass するので r10/r11 と完全同一経路 (= 既存配信は無影響)
- **r12 → r13**: r13 は **r10/r11/r12 完了が前提**。occlusion は既存 Layer 0-2 (upmix / placement / lite-HRTF / venue reverb) に対する **side channel** として動作 (viewer 側 OBB pre-cull + Möller-Trumbore 三角形 raycast が listener↔channel 位置で実行 → `Channel::set3DOcclusion` 直接 set + per-speaker `LOWPASS_SIMPLE` cutoff を自動更新)、既存 DSP chain は一切変更しない
  - r13 の `LLOcclusionGeometryMgr` は `LLPositionalStreamMgr` (r5-r12 系) と完全独立な singleton、両者の責務境界は parser から完全に分離
  - 既存 r5-r12 配置は occluder タグなしの環境で完全互換 (= タグ未指定なら従来動作)
  - chat font live-apply fix (commit 2689a35f8f) は occlusion 機能と独立だが、r13 train に同梱
- **r13 → r14+**: r13 で確立した geometry 登録基盤 (タグ parser / OBB + triangle 抽出 / UUID→OccluderShape レジストリ map / `refreshOccluders` 毎 tick 全件追従 lifecycle / build floater 選択中プリムのライブ再抽出) は r14+ でも流用可能。r13 P15 で実プリム mesh raycast まで取り込んだため、当初 r14+ に置いていた「形状特化近似 / mesh raycast」項は消化。Steam Audio による物理シミュ経路 (回折/反射/共鳴) は永久 drop で、r13 基盤を Steam Audio に渡す計画は廃止 (memory `project_ayastorm_r13_obb_occlusion.md` 参照)

---

## 7. 完成時のユーザ価値

r11 完成時点で AYAstorm が提供する音響体験:

- **水平定位の改善**: lite-HRTF (ITD + ILD shadow) で「左の耳だけ大きい」から「左から音が来る」へ。FMOD 既定 panner (ILD のみ) の薄さを解消
- **距離感の改善**: air absorption で 50m 先のスピーカーが 5m 先と同じ明るさに聴こえる問題を解消
- **会場感の獲得**: venue convolution reverb で dry 素材に「ホール / クラブ / カテドラル / 野外」など 9 種の会場残響を viewer 側で着替え。配信者は dry 素材を流すだけで OK
- **配信者主導の表現**: 配信者がプリム Desc タグに `{venue}` / `{binaural}` / `{wetgain}` を書くだけで、全 listener が同じ意図でレンダリングされた音を聴ける。listener 設定 UI 改修ゼロ
- **ライブ会場 venue placement** (r8 / r10 + r11): 6 個のスピーカー prim から 5.1 ソースを撒き、ChannelGroup 末尾の venue reverb で会場感を載せ、各 channel に lite-HRTF で水平定位を整える → ヘッドホン視聴で **「ホール体験」が成立** (ただし 5.1 配信前提)
- **既存配置の自動恩恵**: r8 / r10 で過去に置かれた全 prim が r11 投入時点で自動的にリッチ化 (ユーザの再配置不要)
- **HTTPS Shoutcast/Icecast 配信の信頼性向上** (P10 同梱): Cloudflare/CDN 経由 HTTPS フロントの HTTP redirect を viewer 側で事前解決し、FMOD netstream のクロスプロトコル redirect 非追従問題を回避

→ 全体として **「SL のヘッドホン視聴体験を一段階引き上げ」つつ、「配信者が会場の音響を作る」運用モデルを成立させる** 規模の変化になる。

r12 完成時の更なる獲得 — **stereo 配信にも 6 spk placement の体験を届ける**:

- **stereo 配信での 6 spk placement 体験**: 世間の SL 配信ほぼ全部を占める stereo 配信で、配信者が `{upmix:on}` を opt-in した瞬間から viewer 内で 5.1 化、r10 の 6 spk placement と r11 の lite-HRTF / venue reverb が全部活きる
- **5.1 native 配信は完全互換**: source ch>=6 で auto bypass、r10/r11 と同一経路。「5.1 配信できる人もできない人も、どちらも 6 spk 体験が得られる」運用モデル
- **配布負債ゼロ**: viewer 内 DSP 完結で新 binary 不要、既存配置 (r8/r10 で置かれた全 prim) も再配置不要
- **r10/r11 投資の元を取る**: r10 で作った 6 spk placement と r11 の venue reverb + lite-HRTF が、stereo 配信でも全部活きる状態 — これまでの投資への最大 ROI

r13 完成時の更なる獲得 — **SL 世界の物理ジオメトリが音を遮る**:

- **会場運営による物理空間の音響表現**: 建物プリムに `[ayastorm:occlude]` を貼ると壁/天井/床が音源と listener の line-of-sight 上で direct/reverb 両方を減衰 (per-prim `{direct:N}{reverb:N}` で個別微調整可)。扉のような動的プリムも `refreshOccluders` の毎 tick 全件追従で自動同期、開閉や移動でリアルな音漏れ体験
- **ライブ会場の体験完成**: r10 6 spk placement + r11 venue reverb + lite-HRTF + r12 upmix + r13 occlusion で「外で muffled に聴こえる音楽 → 扉から漏れてくる → 中に入るとフル venue reverb」というリアル空間の音響体験が SL viewer で初めて成立
- **会場運営主導モデルの新規導入**: r11 配信者主導モデル (= 配信者がスピーカープリム Desc に書いたタグが root truth) の対パターンとして、建物オーナーが建物プリム Desc に書いたタグが root truth として動く運用モデル。両者は意図的に直交 (整合性チェックなし、不一致を仕様として許容)
- **既存配置の自動恩恵**: r5-r12 で過去に置かれた全 stream prim は、会場運営が occlusion タグを建物に貼った瞬間から遮蔽の恩恵を受ける (stream 側再配置不要)
- **配布負債ゼロ**: viewer 側 OBB pre-cull + Möller-Trumbore 三角形 raycast + per-speaker `LOWPASS_SIMPLE` で完結、新 binary 不要、3 OS でのビルド差なし (同梱 `libfmod 2.03.07` の geometry API 機能不全を viewer 側 raycast で迂回した結果として platform 共通性が向上)
- **r10/r11/r12 投資の元を取る**: r10 で作った 6 spk placement と r11 の venue reverb + lite-HRTF と r12 upmix が、occlusion と組合せることで「リアル建築空間の中で再生される音楽」として活きる状態 — これまでの投資への最大 ROI

→ r13 完成時点で AYAstorm は **「リアル音響体験を SL で構築する viewer」** として完成形に近づく。SL viewer 史上初の空間音響遮蔽機能。

r14 以降での更なる発展余地:

- venue IR ユーザアップロード UI / dynamic venue (位置依存残響)
- VenueReverb CPU 最適化 (NUPC、hall_medium 以上の +8〜10pp 低減)
- air absorption 客観 FFT 測定 (r11 P12 で主観 PASS、客観未実施)
- 公開 README / changelog 一括開示 (r8〜r13 機能成熟後)
- アルゴリズム多択化 (Logic 7 / SRS / ML 系 upmix の聴感ベース評価)

**永久 drop (2026-05-11 確定)**:

- ~~Steam Audio integration (回折 / 反射 / 共鳴の物理シミュレーション)~~ — 反射/共鳴は r11 convolution venue reverb (9 IR) で先取り表現済、回折は r13 occlusion の lowpass+減衰で知覚的に近似。まず reverb 拡張で目的達成を試すべきで、物理シミュは「さらに」段階に再検討。Steam Audio engine の存在意義 (SOFA も drop した今) もほぼ消失、3 OS binary 配布負債と engine 依存をゼロに戻す
- ~~per-source SOFA HRTF (個人 SOFA、KU100 等)~~ — r11 lite-HRTF (ITD + ILD shadow + air abs) で AYAstorm の目指す音響リアリティ閾値は越えた、CPU 高 × 個人測定済ユーザ限定の ROI 薄、再配布ライセンス調査負債回避、listener UI 増殖を避ける
- ~~個人 HRTF measurement / personalization~~ — SOFA per-source HRTF を永久 drop した結果として消失

---

## 8. 検討中で本ロードマップに入っていない事項

- **mono タグ `[3dstream:...]` の N 化**: 同設計を mono にも展開。需要が出たら別リリース
- **per-speaker delay / EQ / pan offset**: ステージスピーカーの時間差再現など
- **リンクセット跨ぎのスピーカー連携**: 複数 root を 1 グループに束ねる大規模配置
- **スピーカー数上限の 32 / 64 への昇格**: 設定既定値変更で済むよう設計時点で配慮済
- **HLS / AAC / AC-3 ソース対応**: ライセンスや実装コストが大きく、現時点では非対象

---

## 9. 更新履歴

- 2026-05-03: 初版作成 (r8 着手時点)。Layer 1/2 直交モデル、r7→r11 計画、工数見積り、リスク策定
- 2026-05-06: r8/r9/r10/r10.x/r10.x-bugfix-1 完了状態を反映。**r11 案を Steam Audio + 任意 SOFA から「lite-HRTF + venue convolution reverb (配信者主導モデル)」に再定義**、SOFA per-source HRTF / Steam Audio は r12+ に降格。工数 19-33 日 → 9.5-10.5 日。Preferences UI 改修ゼロ + debug settings 4 件 + 配信者主導モデル方針を明記。リスク表に R5/IR ライセンス/lite-HRTF 体感/reverb CPU を追加、旧 RR1-3 (Steam Audio/SOFA 系) は r12+ 印つけ。仕様詳細は `doc/spec_binaural_venue_reverb.md` 参照。P0 (本書改訂) として `feature/aya-r11-p0-roadmap-update` ブランチで実施
- 2026-05-07: r11 実装完了 (リリース判断保留中) を反映。**r12 案を SOFA per-source HRTF + Steam Audio から「stereo→5.1 upmix のみ」に再定義**、SOFA / Steam Audio / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air absorption 客観 FFT は **r13+ に降格**。ロードマップ題名を `r7 → r11` から `r7 → r12` に拡張、Layer 0 (ソース整形) を §2 に追加、§3 r12 entry 新設、§4 r12 工数行 (5-7 日 / 1-2 週) と内訳追加、§5 RR1-3 を r13+ ラベル変更 + RR10-13 (DSP 挿入位置 / DPL2 phase 依存性 / default 値再 tune / source ch 判定 timeout) を r12 リスクとして追加、§5 工数圧縮 6-8 を追加、§6 依存関係に r12 を追加、§7 ユーザ価値に r12 完成時 stereo 配信での 6 spk 体験を追加、r13 以降を r12 以降から繰り下げ。仕様詳細は `doc/spec_stereo_upmix.md` / `docs/ayastorm-r12-stereo-upmix.md` 参照
- 2026-05-07 (P0 第 2 弾): r12 P0 で実コード (`indra/llaudio/llpositionalstream*.{h,cpp}`、`llaudioengine_fmodstudio.cpp`) を読んで DSP 挿入位置を判定。当初 spec §4.2.1 の **A 案 (`createStream3DGroup` 入力段) / B 案 (per-binding `Channel::addDSP`) はいずれも実アーキテクチャに不適合** と判明 (per-speaker channel が mono `numchannels=1`、Stream3D group は per-speaker mono の合成しか見えず source 2ch 不可視)。代わりに **C 案 = `SpeakerCallback::OpKind::Upmix` 拡張** (r10 Bs775 dispatch の対称構造、`pcmReadCallback` で 2 track ring から L/R を pull、speaker 役割で upmix matrix + 帯域分離 + state を適用して 1ch 出力) として確定。新規ヘルパは `LLStereoUpmix` (`indra/llaudio/llstereoupmix.{h,cpp}`、`LLMultichannelDownmix` 並行構造)。これに伴い §3 r12 entry の主要変更欄を `llstereoupmix.{h,cpp}` 名 + 「helper class」呼称 + データフロー記述に修正、§5 RR10 を解消マーク。詳細調査記録は `doc/r12/dsp_insertion_survey.md`
- 2026-05-10: r12 main / r12.1 完了 (PR #46 / PR #52) を反映。**r13 案を旧 r13+ basket (SOFA / Steam Audio / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air abs 客観 FFT) から「OBB タグベース遮蔽 (フラグシップ) + chat font live-apply 同梱」に再定義**、Steam Audio / SOFA / 形状特化近似 / VenueReverb CPU 最適化 / 個人 HRTF / 公開 README / air abs 客観 FFT は **r14+ に降格**。ロードマップ題名を `r7 → r12` から `r7 → r13` に拡張、Layer 3 (空間ジオメトリ) を §2 に追加 (4 層モデルへ)、§3 r13 entry 新設 (OBB occlusion + 会場運営主導モデル + chat font 同梱)、§4 r13 工数行 (5-7 日 / 1-2 週) と内訳追加、§5 RR1-3 を r14+ ラベル変更 + RR14-18 (mesh OBB ズレ / material 表 tuning / door 60Hz update / rapid teleport / 大規模建造物 prim scan) を r13 リスクとして追加、§5 工数圧縮 9-12 を追加 (OBB 単独決め打ち / Steam Audio r14+ 降格 / chat font 同梱 / タグ多択化回避)、§6 依存関係に r13 → r14+ を追加 (geometry 登録基盤の Steam Audio 流用)、§7 ユーザ価値に r13 完成時 SL 史上初空間音響遮蔽 + 会場運営主導モデル新規導入を追加、r14 以降を r13 以降から繰り下げ。仕様詳細は `doc/spec_obb_occlusion.md` / `docs/ayastorm-r13-occlusion.md` 参照。役割分担 (会場運営 vs 配信者の直交性) は memory `project_venue_occlusion_orthogonal.md`、r13 フラグシップ + 同梱 fix 方針は memory `project_ayastorm_r13_obb_occlusion.md` 参照
- 2026-05-10 (r13 spike 着手): 同梱 `libfmod 2.03.07` の `System::createGeometry` が機能しない (`FMOD_ERR_INTERNAL`、memory `project_fmod_geometry_unavailable.md`) ため FMOD geometry 経路を放棄、**listener-source segment vs OBB の自前 slab test を viewer 側で実装**して `Channel::set3DOcclusion` に直接適用する経路に pivot。spike 出荷スコープは `[ayastorm:occlude]` 単独タグ + per-speaker `LOWPASS_SIMPLE` DSP (壁越し muffled 聴感、22kHz→300Hz exponential cutoff) + 250ms ramp + debug overlay (View メニュー `Alt+Shift+O`)。`[ayastorm:door]` / material 表 / debug settings 4 件のうち 3 件 / O2〜O14 通し検証は **r13.x 持ち越し**。同 commit に **起動時 OS unresponsive dialog 緩和 (A+B、drain rate-limit + curl timeout 短縮)** を同梱、根本対応 (curl 非同期化、C) は別 workstream 着手予定。実装詳細は `docs/ayastorm-r13-occlusion.md` §5 を canonical とする。commit 記録: `66ddab6eb4` (P0 spec/工程資料/roadmap 初版) / `58c5ad7c14` (実装本体 + A+B 緩和) / `6fcd078250` (View メニュー + `Alt+Shift+O`)
- 2026-05-11 (r13 final scope 確定): spike 結果を踏まえて r13 出荷スコープを最終確定。**永久 drop**: `[ayastorm:door]` 専用タグ (`refreshOccluders` 毎 tick 全件追従で吸収) / material 表 (`LL_MCODE_*` → preset 写像、聴感根拠薄、tag-guide 推奨セットで代替) / `Stream3DOcclusionDirectGain` / `Stream3DOcclusionReverbGain` / `Stream3DOccluderMaxCount` 設定化 (per-prim タグ `{direct:N}{reverb:N}` override + hardcoded `kMaxOccluders=256` で代替)。**r13 残工程**: per-prim override args parser / `llPlaySound` occlusion 適用 / `Stream3DOcclusion` master sentinel + `Stream3DOccluderRange` 64m + `kMaxOccluders` 256 化 / 検証 O1〜O12 / chat font cherry-pick (`d66bdb74fc`、元 `2689a35f8f`) / tag-guide ja/en/zh 加筆 / Release Notes。**起動 unresponsive dialog 根本対策 (C)** は別 workstream で完了 (`f336d43abc` = URL pre-resolve 非同期 worker / `5c3487ff06` = X11 `#define Status int` 罠回避、memory `project_linux_xlib_status_define_trap.md`)。本書改訂: §2 ASCII 図の Layer 3 を viewer 側 raycast + per-prim タグ書式に更新、§3 r13 entry を final scope に書き換え、§4 r13 内訳を「spike 完了済 (~3 日) / 残工程 (~3-4 日)」二段構成に再編、§4 工数表の r13 実績欄を "spike 完了 / 残工程進行中" に変更、§5 RR15 を default 値 + tag-guide 推奨セット tuning に書き換え、§5 RR16 (door 60Hz update) を解消マーク (`refreshOccluders` 一本化)、§5 RR17 を viewer 側 OBB レジストリ参照に修正、§5 工数圧縮 12 を 1 種タグ + 4 件 settings に書き換え、§6 依存関係 r12→r13 / r13→r14+ を viewer 側 raycast + UUID→OBB レジストリ map 表現に修正、§7 r13 ユーザ価値を `[ayastorm:door]` 削除版 + 配布負債ゼロの根拠を明確化。同期して `doc/spec_obb_occlusion.md` (commit `2e02a63ac8`) と `docs/ayastorm-r13-occlusion.md` (commit `d143a4dbb7`) を final scope 反映済
- 2026-05-11 (SOFA / Steam Audio / 物理シミュ / 個人 HRTF 永久 drop): r14+ basket の更なる絞り込み議論。**永久 drop 追加**: (a) SOFA per-source HRTF / 個人 HRTF — r11 lite-HRTF (ITD + ILD shadow + air abs) で AYAstorm の目指す音響リアリティ閾値は越えた、CPU 高 × 個人測定済ユーザ限定の ROI 薄、再配布ライセンス調査負債回避、listener UI 増殖を避ける。(b) Steam Audio integration / 回折・反射・共鳴の物理シミュ — 反射/共鳴は r11 convolution venue reverb (9 IR) で先取り表現済、回折は r13 occlusion の lowpass+減衰で知覚的に近似。まず reverb 拡張で目的達成を試すべきで、物理シミュは「さらに」段階に再検討。Steam Audio engine の存在意義 (SOFA も drop した今) もほぼ消失、3 OS binary 配布負債と engine 依存をゼロに戻す。本書改訂: §2 ASCII 図 Layer 2 から「SOFA per-source HRTF / Steam Audio ← r13+」行を削除し永久 drop 注記に置換、§3 r13 entry の「r14+ Steam Audio で形状特化近似を再検討」を「形状特化近似が必要になった段階で viewer 側 mesh raycast 経路を改めて検討」に書き換え、§5 RR1/RR2/RR3 を解消マーク (永久 drop 根拠を明記)、§5 工数圧縮 13 を新設、§6 依存関係 r13→r14+ ツリーを Steam Audio/SOFA/個人 HRTF 削除版に書き換え、§6 r13→r14+ 説明文を viewer 側 mesh raycast 経路に書き換え (Steam Audio 流用計画を廃止)、§7 r14 以降リストから Steam Audio / SOFA / 個人 HRTF を削除し「永久 drop (2026-05-11 確定)」サブセクションを追加。memory `project_ayastorm_r13_obb_occlusion.md` の永久 drop セクションも同期更新
- 2026-05-11 (r13 P15 mesh raycast 取り込み): r14+ basket に残していた「形状特化近似 viewer 側 mesh raycast」項を **r13 内に前倒し取り込み**。AYA の「Prim の Mesh Export 機能で形状確定できる」洞察を起点に、`LLVolume::getVolumeFace` から path cut / hollow / mesh の実プリム三角形を抽出し、segment-vs-triangle を **OBB pre-cull (~95% reject) + Möller-Trumbore raycast** の 2 段で判定する経路を実装。`kMaxTrisPerOccluder = 2000` 超過時は OBB-only にフォールバック (LL_WARNS_ONCE)。build floater で選択中のプリムは `refreshOccluders` で毎 tick 再抽出してライブ追従。`Stream3DShowOccluders` の debug overlay は実形状三角形メッシュ (シアン fill + wireframe、face normal 方向 0.02m offset で z-fight 回避) に置換、オレンジ OBB 描画は撤去。本書改訂: §3 r13 entry を「形状判定は OBB pre-cull + 実プリム三角形 raycast の 2 段」+ データフロー「OBB pre-cull + Möller-Trumbore 三角形 raycast」に更新、§5 RR14 (mesh OBB ズレ) を解消マーク (P15)、§5 工数圧縮 13 の r14+ スコープから「形状特化近似 viewer 側 mesh raycast」を除外、§5 工数圧縮 14 を新設 (P15 取り込み判断記録)、§6 依存ツリーの r13 行を「タグベース occlusion: 実プリム mesh raycast」に書き換え + r14+ から該当項目を消化 + r13→r14+ 説明文を triangle 抽出 + ライブ再抽出含む基盤表現に修正、§7 ユーザ価値の「viewer 側 segment vs OBB raycast」を「OBB pre-cull + Möller-Trumbore 三角形 raycast」表記に統一、§7 r14 以降リストから「形状特化近似 / mesh prim 実 triangle 利用」を削除。同期して `docs/ayastorm-r13-occlusion.md` §5.5 commit log 表に P7〜P15.7 の 19 行を追記、memory `project_ayastorm_r13_obb_occlusion.md` を mesh raycast + ライブ追従仕様に更新。commit 記録: `cd4d64dc3b` (P15.1 OccluderShape 容器) / `84dbdc7141` (P15.2 三角形 raycast wire-up) / `5e527916be` (P15.3 三角形 wireframe overlay) / `dd8ada0ab5` (P15.4 シアン fill + 0.02m offset、オレンジ OBB 削除) / `ae66fffe44` (P15.5 build floater 選択中ライブ再抽出) / `409dea6f53` (P15.6 tag-guide ja/en/zh) / `44df4e93a8` (P15.7 Release Notes ja/en/zh)
- 2026-05-11 (r13 P15.9 TP/login freeze 対策): P15 で `extractTriangles` (LLVolume → 三角形抽出) を `onObjectPropertiesReceived` から **同期実行** していたため、TP / login 直後の `ObjectPropertiesFamily` 集中到着で N 個の occluder が同フレームに登録 → N × ~50µs-2ms の主スレッド占有 → 100-200 ms 級 hitch (固まる体感) のリスクを AYA が指摘。**対策 A+B 同梱**: (A) `mPendingExtract` queue + `refreshOccluders` 末尾の per-tick budget=6 drain (100 prim バーストを ~17 tick = 0.3s に均す、pending 中は OBB-only fallback で audio 正常)、(B) 既存エントリ更新時の Desc 同値 (`scale_changed || tris.empty()` 不成立) re-extract スキップ (TP の冗長 ObjectProperties 再配信で空回り extract しない)。検証は AYA 実機ログイン + Claude による `~/.ayastorm_x64/logs/AYAstorm.log` grep で 7 prim 会場で register==drain==7 / 同 tick 最大 drain=4 / queue_remaining 即 0 を確認。検証用 hook (drain LL_INFOS) は shipping 前に除去。本書改訂: §9 本エントリ追記のみ (機能仕様変更ではない内部最適化のため §3 r13 entry / §6 / §7 は触らない)。同期して `docs/ayastorm-r13-occlusion.md` §5.5 commit log + §5.4 残リスク追記 + §5.7 設計判断ログを追加、memory `feedback_remove_verification_logs.md` 新設 (verify hook commit 前除去ルール)。commit 記録: `2887e598f7` (P15.9)
