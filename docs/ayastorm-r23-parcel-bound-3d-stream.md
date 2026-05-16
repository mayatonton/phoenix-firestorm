# AYAstorm r23 — Parcel-bound 3D stream 仕様 (draft)

**作成日**: 2026-05-16
**最終更新**: 2026-05-16 (M5 release notes 完成、tag 切り前)
**ステータス**: M1〜M5 release notes 完成、release tag 待ち
**対象ブランチ**: `feat/ayastorm-r23-parcel-bound-3d-stream`

このドキュメントは r23 で実装予定の「AYAstorm 3D stream を SL の parcel "Restrict gestures and object sounds to this parcel" フラグに連動させる」機能の仕様 draft。AYA との対話で順次確定していく。

---

## 1. 狙い

AYAstorm の 3D stream (positional / distributed-stereo / 5.1ch placement / venue reverb / binaural — r6 系列以降) が **隣接 parcel まで無条件で漏れて聞こえる** 現状を解消する。

SL の vanilla flow では gesture / object sound / parcel music は **送出 parcel 側 or 受信 parcel 側のいずれかが `PARCEL_FLAG_SOUND_LOCAL` (UI 上は "Restrict gestures and object sounds to this parcel") を立てている時、parcel boundary を越えて聞こえなくなる** という規約がある。3D stream はこの規約から外れているため、配信を仕込んだ parcel の外でも音が届いてしまい、近隣区画の体験を侵害する。

r23 は 3D stream をこの SL 既存規約に "素直に乗せる" 拡張。

---

## 2. スコープ

| 項目 | 対象 / 対象外 |
|---|---|
| AYAstorm 3D positional stream (r6+) | ✅ 対象 |
| 5.1ch placement (r10) | ✅ 対象 (source 1 点で判定、全 ch 一律) |
| distributed stereo (r8) の N 拠点 | ✅ 対象 (拠点ごとに source 1 点で判定) |
| venue reverb (r11) | ✅ stream 本体が mute されたら reverb tail も自然に消える |
| binaural / HRTF (r11) | ✅ 上位 mute が効くので追加対応不要 |
| 非 positional な parcel music (SL vanilla URL ストリーム) | ❌ 対象外 (sim 側で既に parcel boundary を尊重) |
| voice chat | ❌ 対象外 |
| gesture / object sound | ❌ 対象外 (既に vanilla で対応済み) |

---

## 3. 確定方針 (M1 仮)

### 3.1 リスナー側 parcel 判定モデルを採用

SL の `LLViewerParcelMgr::canHearSound(pos_global)` を **そのまま** 3D stream channel にも適用する。

`canHearSound()` の判定ロジック (既存):
1. listener (agent) と source の position が **同じ parcel** → 聞こえる
2. 違う parcel で agent parcel が `SOUND_LOCAL` → 聞こえない
3. 違う parcel で source parcel が `SOUND_LOCAL` → 聞こえない
4. それ以外 → 聞こえる

→ 配信者主導タグ (r11 流儀) を新規追加せず、**parcel フラグそのものを root truth** として扱う。SL の既存規約と 100% 一致するので、ユーザーの直感に合致。

### 3.2 mute の実装単位 — source 1 点で判定、全 ch 一律

判定は **配信タグ prim (3D stream の root) の position 1 点** で `canHearSound()` を呼ぶ。結果を 5.1ch / distributed stereo の **全 channel に一律適用** する (全鳴る or 全 mute)。

per-channel position はあくまで **音像 (空間音響)** のためのもので、parcel 規約判定には使わない。たとえば 5.1ch の FR speaker が物理的に隣 parcel にはみ出ていても、parcel 判定そのものは配信タグ prim 位置で完結する:

- 配信者 parcel 内の listener → 全 ch 聞こえる (FR の音像が隣 parcel 方向から聞こえても OK)
- 隣 parcel の listener → 全 ch mute (はみ出た FR が物理的にすぐ近くにあっても mute)

mute の実装は FMOD channel の `volume` を 0 にする (paused にしない)。

理由:
- volume=0 は channel position / spatial attenuation の計算を続けるので、parcel boundary を跨いだ瞬間に滑らかに復帰
- paused だと再開時にクリックノイズ / 再シーク必要のリスク
- per-channel 別判定モデルは音像が「片寄せ」になって不自然なので **採用しない** (M1 確定)

### 3.3 評価頻度 — 2 tier モデル

**Tier 1: 据置 stream (大多数、source 位置が静止)**
- `LLAgent::addParcelChangedCallback` シグナルでだけ再評価
- avatar が parcel を跨いだ瞬間に走り、それ以外は完全に静止
- source 位置が動かないので source 側の監視は不要 → 平時の per-frame コストはゼロ

**Tier 2: 装着 stream (稀、source 位置 = 装着 avatar 位置)**
- binding 構築時に `LLViewerObject::isAttachment()` で判定し、attachment フラグを binding に立てる
- attachment フラグが立っている binding に限り `update()` (per-frame) で `canHearSound()` 再評価
- 装着 stream が無い場面では Tier 2 ループ自体が走らない
- 装着 stream が一部存在する場面でも、追加コストは「装着 binding 数 × 1 query × 60 fps」で十分に安い

判定結果が変わったときだけ `setVolume()` を呼ぶ idempotent ガードを入れることで、FMOD への無駄な API call を抑える。

precedent: `LLAudioSourceVO::updateMute()` (object sound) も canHearSound を毎 audio tick で評価しているが、3D stream は据置が大多数なので Tier 分離して負荷を最小化する。

---

## 4. 判定境界

| ケース | 例 | 振る舞い |
|---|---|---|
| 同一 parcel 内 | 配信タグの prim と listener が同じ区画 | 聞こえる (現状維持) |
| 隣接 parcel・どちらも non-SOUND_LOCAL | 公共音楽イベント | 聞こえる (現状維持) |
| 隣接 parcel・source parcel が SOUND_LOCAL | 個室 DJ が囲い込み設定 | **聞こえない (new)** |
| 隣接 parcel・listener parcel が SOUND_LOCAL | 静寂を求めて自宅区画に隔離設定 | **聞こえない (new)** |
| 5.1ch の一部 channel が parcel 境界をはみ出た | 配置失敗 / 境界跨ぎ | **判定は配信タグ prim 1 点で完結、speaker 位置は無関係** (全鳴る or 全 mute) |
| 装着 stream を持つ avatar が parcel A→B を walk | 装着型 3D stream の移動 | source position が avatar に追従、跨いだ瞬間に新 parcel の SOUND_LOCAL 設定で再判定。装着者自身は常に same parcel なので聞こえ続ける、周囲は装着者と同 parcel の listener だけ聞こえる |
| 配信者が region 跨ぎ移動 | TP / fly | 移動直後の reevaluation で正しい状態に |

---

## 5. データ層

### 5.1 source position の取得

判定には **配信タグ prim (3D stream の root) の position** を使う。per-channel position は使わない (§3.2 参照)。

- `LLPositionalStreamMgr` (or 同等) が保持する stream の root position
- parcel フラグは sim から `LLViewerRegion::mParcelOverlay` 経由で取得 (既に viewer cache されている)

### 5.2 listener position

`gAgent.getPositionGlobal()` で取得。`canHearSound()` は内部で `inAgentParcel()` 経由で listener parcel を解決するので、追加実装不要。

---

## 6. 実装スケッチ

### 6.1 影響ファイル (現時点の想定)

- `indra/newview/llpositionalstreammgr.cpp` / `.h` — 3D stream channel の volume 更新 hook に `canHearSound()` チェック追加
- `indra/newview/llviewerparcelmgr.h` (既存)、`llparcel.h` (既存) — 追加変更なし、API を利用するのみ
- 設定追加なし (§8 で確定)。SL parcel 規約準拠が default 動作、escape hatch cvar も逆タグも入れない

### 6.2 既存の参考パターン (viewer 内)

- `LLViewerParcelMgr::canHearSound()` — 同一の判定ロジックは既に gesture / object sound / 一部 attached sound で使用されている
- 配信者主導タグでなく parcel flag 直読の前例として、`llvoiceclient` の voice cutoff も類似の sim-truth フォロー

### 6.3 構造調査メモ (2026-05-16 時点)

- `LLViewerParcelMgr::canHearSound(LLVector3d pos_global)` 既存 (`llviewerparcelmgr.cpp:843`)
- `LLParcel::getSoundLocal()` 既存 (`llparcel.h:477`)
- `LLViewerParcelOverlay::isSoundLocal(pos_region)` 既存
- 3D stream channel の position 管理は r10 で per-channel 化済み (`project_ayastorm_r10_5_1ch_placement.md` 参照)

→ **viewer 単独実装、sim 変更なし、新しい LSL タグ不要**

---

## 7. 受入基準

- [x] 配信 parcel が SOUND_LOCAL=on、隣接 parcel に居る listener → 3D stream が聞こえない
- [x] 配信 parcel が SOUND_LOCAL=on、同一 parcel 内の listener → 普通に聞こえる
- [x] listener parcel が SOUND_LOCAL=on、隣接 parcel の 3D stream → 聞こえない
- [x] どちらの parcel も SOUND_LOCAL=off → 従来通り聞こえる
- [x] parcel 境界を跨いで walk すると mute / unmute が滑らかに切り替わる (クリックノイズなし)
- [x] 5.1ch 配置で speaker が parcel boundary を跨いでいても、判定は配信タグ prim 1 点で完結し全 ch 一律
- [x] region 跨ぎ TP 直後に正しい mute 状態が反映される
- [x] venue reverb の tail が残っていても本体 mute と整合 (tail 単独で残り続けない)
- [x] 3 OS (Linux / Win / Mac) でビルド通過 + 動作確認

---

## 8. 残る未確定論点

すべて M1 で「追加機能なし」で確定。SL parcel 規約準拠を default 動作とし、cvar / 逆タグ / tail 制御は一切追加しない。

| 論点 | 優先 | 対処タイミング | 状況 |
|---|---|---|---|
| ~~escape hatch cvar (`FSParcelBoundStream3D` 等) を入れるか~~ | — | — | ✅ **なし**で確定。SL parcel 規約準拠を default 動作にする、cvar 追加しない |
| ~~配信者主導タグで「parcel フラグ無視して global broadcast」する逆 escape hatch~~ | — | — | ✅ **なし**で確定。global broadcast したい配信者は parcel SOUND_LOCAL を off にすれば足りる (SL 既存規約と一致) |
| ~~5.1ch 配置 で channel 単位 mute が「片寄せ感」で違和感を生まないか~~ | — | — | ✅ §3.2 で source 1 点判定に確定、論点消滅 |
| ~~venue reverb tail の打ち切りタイミング~~ | — | — | ✅ **追加対応なし**で確定。本体 mute で reverb 入力が止まり tail は自然減衰する、M2 実装中に挙動確認のみ |

---

## 9. リスク登録

| リスク | 影響 | 対策 |
|---|---|---|
| `canHearSound()` の per-frame コール頻度が高すぎる | 低 | `parcel_change` シグナルベース更新で抑制 |
| parcel overlay cache が region 跨ぎ直後に未到着の状態で判定 | 中 | overlay が無い時は "聞こえる" にフォールバック (`canHearSound()` の既存挙動) |
| 既存配信者が「俺の音が止まった」と困惑 | 中 | release note で「SL parcel 規約 (gesture/object sound と同等) に従うよう拡張、global broadcast したい場合は parcel の SOUND_LOCAL を off に」と明記 |

---

## 10. マイルストーン

| M | 内容 | ステータス |
|---|---|---|
| M1 | 仕様確定 + 構造調査 + escape hatch 方針確定 | ✅ 確定 (追加機能なし、SL parcel 規約準拠を default) |
| M2 | 実装 — `LLPositionalStreamMgr` に `canHearSound()` フック追加 | ✅ 完了 (commit `673ca4ae9c`、Linux ビルド PASS) |
| M3 | 受入テスト — 同一/隣接 parcel、装着 stream の parcel 跨ぎ移動、region 跨ぎ TP | ✅ 完了 (2026-05-16、全項目 PASS) |
| M4 | 3 OS ビルド (Linux → Win → Mac) | ✅ 完了 (2026-05-16、3 OS ビルド + 動作確認 PASS) |
| M5 | Release — spec doc 更新、release note 3 言語、tag | ✅ release notes 完成 (r14/r16-r20/r23 × en/ja/zh 計 21 + bundle release page × en/ja/zh 計 3)、tag 切りは AYA 手動 |
