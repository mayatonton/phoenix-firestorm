# AYAstorm r23 — リリース告知

**r23 は AYAstorm の 3D stream (positional / 5.1ch / distributed-stereo / binaural — r6 系列以降) を SL の `PARCEL_FLAG_SOUND_LOCAL` 境界に準拠させるリリース** — gesture / object sound と同じ規約。新規 cvar なし、逆向きタグなし、parcel フラグそのものが root truth。

> **配信形態**: r23 は r13〜r23 を一括配信するタグの主機能として出荷されます。同梱される他リリースの release note は GitHub Release ページから直接リンクされます。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r23-parcel-bound-3d-stream.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r23 — Parcel-bound 3D stream

### r23 の柱: 3D stream を SL の parcel 規約に乗せる

r22 までは AYAstorm の 3D stream (positional / distributed-stereo / 5.1ch placement / venue reverb / binaural — r6 系列以降) が **隣接 parcel まで無条件で漏れる** 状態でした。SL の vanilla flow では gesture / object sound / parcel music は送出 / 受信 parcel いずれかが `PARCEL_FLAG_SOUND_LOCAL` ("Restrict gestures and object sounds to this parcel") を立てている時、parcel boundary を越えて聞こえない規約があります。3D stream はこの規約から外れていたため、配信を仕込んだ parcel の外でも音が届いてしまい、近隣区画の体験を侵害していました。

r23 は 3D stream をこの SL 既存規約に **素直に乗せる** だけ。新規 cvar なし、配信者側の opt-in タグなし、escape hatch なし — parcel フラグそのものが root truth、gesture / object sound と同じ。

### 仕組み

3D stream channel の volume update hook に `LLViewerParcelMgr::canHearSound(pos_global)` を 1 行追加。挙動:

1. listener と source が **同一 parcel** → 聞こえる (現状維持)
2. 違う parcel で **agent parcel** が SOUND_LOCAL → mute
3. 違う parcel で **source parcel** が SOUND_LOCAL → mute
4. それ以外 → 聞こえる (現状維持)

これは SL が gesture / object sound / 一部の attached sound で既に使っている **同一ロジック** — 新しい規約を発明したわけではなく、3D stream を既存規約に compliance させただけ。

### per-channel position vs source position

5.1ch placement / distributed-stereo (r8〜r10) では各 FMOD channel に固有の spatial position がありますが、**parcel 判定は 1 点 = 3D stream root prim の position** で完結し、結果を全 channel に一律適用 (全鳴る or 全 mute)。

- 配信者 parcel 内の listener → 全 ch 聞こえる (5.1 FR speaker の音像が物理的に隣 parcel にはみ出ていても OK)
- 隣 parcel の listener → 全 ch mute (はみ出た FR の音像が物理的にすぐ近くにあっても mute)

per-channel parcel 判定は「片寄せ」な音像になり musically 不自然なので、意図的に採用しません (spec §3.2)。

### mute 方式: volume=0 (paused にしない)

mute は FMOD channel の `setVolume(0)` で実装。理由:
- volume=0 は channel position / spatial attenuation の計算を継続するので、parcel 復帰時に滑らかに戻る
- paused だと再開時のクリックノイズ / 再シーク必要のリスク
- `parcel_change` シグナル + idempotent guard (状態未変化時は FMOD コール抑制) で「無駄な API call」も防ぐ

### 2 tier 評価モデル

**Tier 1 (据置 stream、大多数)**: `LLAgent::addParcelChangedCallback` でだけ再評価。avatar が parcel を跨いだ瞬間に走る。per-frame コストはゼロ。

**Tier 2 (装着 stream、稀)**: binding 構築時に `LLViewerObject::isAttachment()` でフラグを立て、attachment 付き binding のみ per-frame で `canHearSound()` を再評価。装着 stream が無い場面では Tier 2 ループ自体が走らない。あっても「装着 binding 数 × 1 query × 60 fps」で十分に安い。

### 設定

**意図的にゼロ。** 新 cvar を追加していません。

- escape-hatch cvar (例: "FSParcelBoundStream3D") は配信者が規約を bypass する穴になるため、不採用
- 配信者側の逆向きタグ (例: "parcel boundary 無視") も同様の穴になり、関係者全員の認識が必要になるため不採用
- global broadcast したい配信者は parcel の SOUND_LOCAL を off にすれば足ります — これは SL 既存の native pathway そのもの

### 配信者向け移行ノート

AYAstorm 3D stream で配信していて、隣接 parcel の listener にも聞こえることを前提に運用している場合:

- parcel フラグの default は **off** — SOUND_LOCAL を立てていない配信者は **何も変わりません**
- parcel に SOUND_LOCAL を立てている配信者は、隣接 parcel の listener には聞こえなくなります — これは SL が gesture / object sound で常に行ってきた挙動と一致し、3D stream は単に意図せず例外になっていただけ

### 既知の制約

- **region 跨ぎ TP**: 新 region の parcel-changed signal 直後に正しい状態が再適用されます。極めて高速な跨ぎでは短い再生窓が出る可能性があるが有界
- **装着 stream が parcel を跨いで歩く**: 装着者自身は常に source と same parcel (常時聞こえる)。周囲は装着者と同 parcel の listener のみ聞こえる
- **Venue reverb tail**: 本体 mute で reverb 入力が止まり、tail は自然減衰。特別な対応は不要

### 実装概要

- `llpositionalstreammgr.cpp` / `.h` — volume update hook に `canHearSound()` 1 行追加
- 新 cvar なし、新 uniform なし、UI 変更なし
- 3 OS (Linux / macOS / Windows) ビルド + 動作確認 PASS

### ドキュメント

- r23 full spec / source position / 2 tier evaluation / リスク登録: `docs/ayastorm-r23-parcel-bound-3d-stream.md`
- 過去の 3D stream リリース (r6〜r12、listener binding 含む): in-tree spec doc および過去 release note (r10 5.1ch placement、r11 binaural + venue reverb 等) 参照
