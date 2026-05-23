# r30 Cinematic Godrays 引き継ぎ資料 (2026-05-24)

context clear 直前の作業状態 snapshot。次セッションはこの doc を最初に読んで、
working tree の uncommitted 変更と AYA さん検証待ちの内容を pick up すること。

## branch / 親

- 作業 branch: `fix/r30-cinematic-godrays-sun-shadow-permutation`
- 親: `ayastorm-release`
- **commit 0 件** (全変更が uncommitted、working tree のみ)
- AYA さん指示があるまで commit しない (`feedback_no_auto_commit`)

## このセッションの本題

Cinematic mode (`AYAVisualRealismEnabled==2`) で **r15 godrays** を
ちゃんと魅せるための live cvar 整備と default 調整。

経緯のキーポイント:

1. **SUN_SHADOW permutation 抜け**: BD-port `gVolumetricLightProgram` に
   `SUN_SHADOW` permutation を立て忘れていて、`shadowUtil.glsl` の
   `nonpcfShadowAtPos` が一様 `1.0` fallback を返し godray が「光のシャフト」
   ではなく scene 全体への一様 sunlight 加算になっていた → fix 済。
2. **godrays の cvar 化**: 旧 `const float strength = 0.10` / `pow(cos, 8.0)`
   を live cvar (`AYAR15GodraysStrength` / `AYAR15GodraysPhaseExponent`) に置換。
3. **白飛び対策**: 当初 Reinhard soft-clip (`AYAR15GodraysSoftClip`) を入れたが、
   「白飛びを抑えると結局 beam 強度も落ちるだけ」と AYA さんが指摘し、
   **物理的に等価 (Reinhard は最も明るい sun center を最も削るため強度低下と等価)** と
   判明 → 全削除。
4. ~~MAX blend 案~~ → AYA さん検証で「あまりよくなかった」と却下、全削除済
   (`AYAR15GodraysUseMaxBlend` cvar / pipeline.cpp glBlendEquation / xui checkbox 全消)。
5. **default チューニング** (AYA 体感確定):
   - `AYAR15GodraysPhaseExponent`: 16.0 → **30.0** (集中ビーム)
   - `AYAR15GodraysStrength`: 0.15 → **0.10** (控えめ)
6. **mesh blow-out (sun-facing 面の白濁)**: godrays が原因ではなく
   mesh material × tonemap/exposure が支配的、と AYA さん診断。**Task #12 で
   parking lot に保留** (godrays 整備完了後に別 phase で着手)。
7. **Cinematic 新 default 3 件追加** (overlay + parityTable 同期):
   - `RenderShadowResolutionScale = 3.0` (sunset 影の cascade 解像度確保)
   - `RenderFarClip = 400.0` (遠景描画)
   - `RenderVolumetricLightingMultiplier = 4.0` (BD-port renderVolumetric 強度、
     50 → 4.0 に大幅 down。AYA さん「4.0 でも OK」と承認済)
8. **Godrays Cinematic default flip migration**: `AYAR15GodraysInCinematicEnabled`
   default `false` → `true` に切替。既存ユーザーの persisted false を一度だけ
   強制 true 上書きする one-shot migration (`AYAR15GodraysCinematicMigrationVersion`
   sentinel) を追加。
9. **UI label 統一**: floater section / checkbox / tooltip 全部
   "AYAstorm r15 ゴッドレイ" → "ゴッドレイ" (en は "Godrays") に簡略化。
   Cinematic 有効化 checkbox label は "ゴッドレイ有効化" / "Enable Godrays"。
10. **floater 右余白縮小** (AYA 指摘): floater 600→500、tab 590→490、
    panel 588→488、header text 572→472、checkbox label 506→406 で 100px 削減。
    Glow & Volumetric panel height は MAX blend 行削除分 457→433 (-24)。

## working tree 変更ファイル (14 件)

| ファイル | 役割 |
| --- | --- |
| `indra/newview/llviewershadermgr.cpp` | `gVolumetricLightProgram` に `SUN_SHADOW=1` permutation 追加 (godrays shaftify fix の本丸) |
| `indra/newview/app_settings/settings.xml` | godrays cvar 4 件追加/更新 (`InCinematicEnabled` default→1, `PhaseExponent` default 30.0, `Strength` default 0.10, `CinematicMigrationVersion`)。MAX blend cvar は削除 |
| `indra/newview/app_settings/settings_cinematic_bd.xml` | Cinematic overlay に `RenderShadowResolutionScale=3.0` / `RenderFarClip=400.0` / `RenderVolumetricLightingMultiplier=4.0` を追加 |
| `indra/newview/llviewermenu.cpp` | `AYAResetCinematic::parityTable` に上記 3 件を同期 (D ボタン reset 値) |
| `indra/newview/llappviewer.cpp` | `LLCinematicOverlay::applyR15GodraysCinematicMigrationIfNeeded()` 呼出追加 |
| `indra/newview/llcinematicoverlay.cpp` / `.h` | 同 migration 関数の実装/宣言 |
| `indra/newview/llsettingsvo.cpp` | godrays 用 `phase_exponent` / `strength` uniform binding (LLCachedControl + uniform1f) |
| `indra/llrender/llshadermgr.h` | `AYA_R15_GODRAYS_PHASE_EXPONENT` / `AYA_R15_GODRAYS_STRENGTH` enum 追加 |
| `indra/llrender/llshadermgr.cpp` | 同 uniform name の `mReservedUniforms.push_back` |
| `indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl` | 旧 const を `uniform float aya_r15_godrays_phase_exponent` / `aya_r15_godrays_strength` 化 |
| `indra/newview/pipeline.cpp::doGodrays` | mode==1 早期 return 削除 (唯一の caller 側に gate 集約済)。MAX blend opt-in は削除 |
| `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` | floater 490 / tab_container 461 / `tab_glow_godrays` panel 433、`PhaseExp` slider min 4→1、section/checkbox label を "Godrays" に統一、Cinematic 有効化 checkbox label を "Enable Godrays"。floater 右余白 100px 削減 (各 width -100)。MAX blend 行 / SoftClip 行は完全削除 |
| `indra/newview/skins/default/xui/ja/floater_aya_cinematic.xml` | 同 ja 翻訳追加。section "ゴッドレイ"、Cinematic 有効化 checkbox label "ゴッドレイ有効化"。MAX blend / SoftClip 行削除 |

> SoftClip 関連 + MAX blend 関連は **全 6 ファイルから完全 revert 済**
> (`AYAR15GodraysSoftClip` / `aya_r15_godrays_soft_clip` / `AYA_R15_GODRAYS_SOFT_CLIP` /
> `AYAR15GodraysUseMaxBlend` / `aya_r15_max_blend` / `MAX_BLEND` 全文字列 grep して
> 残存 0 を確認済)。

## 新規 cvar 一覧

| cvar | Type | default (settings.xml) | Cinematic overlay | parityTable (D 値) | 役割 |
| --- | --- | --- | --- | --- | --- |
| `AYAR15GodraysInCinematicEnabled` | Boolean | **1** (flip 済) | — | — | godrays の Cinematic mode opt-in。default ON 化 |
| `AYAR15GodraysCinematicMigrationVersion` | S32 | 0 | — | — | flip migration の sentinel (0=未適用 → 起動時に強制 true 上書き、1=適用済) |
| `AYAR15GodraysPhaseExponent` | F32 | **30.0** | — | LL default (30.0) | Mie phase exponent。1=全画面 halo / 8=wide / 16=moderate / 30=tight beam / 32=thin shaft |
| `AYAR15GodraysStrength` | F32 | **0.10** | — | LL default (0.10) | 加算強度 |
| `RenderShadowResolutionScale` | F32 | 1.0 | **3.0** | 3.0 | Cinematic で sunset の cascade 影確保 |
| `RenderFarClip` | F32 | 256.0 | **400.0** | 400.0 | Cinematic 遠景 |
| `RenderVolumetricLightingMultiplier` | F32 | 50.0 | **4.0** | 4.0 | BD-port renderVolumetric 強度 (godrays とは別 path) |

## build / install 状態

- **build**: 完了 (autobuild ReleaseFS_open + fmodstudio + opensim + LL_DULLAHAN_AUDIO_CALLBACK=ON)
- **install**: `~/ayastorm/` に deploy 済 (rm -rf → cp -r build-linux-x86_64/newview/packaged/.)
- **shader cache clear**: `~/.ayastorm_x64/cache/shader_cache/` 削除済
- AYA さん起動して live 検証待ち

## AYA さんに依頼中の検証

```
1. Cinematic mode で起動 (mode 0/1 → 2 切替は再起動必要)
2. AYAstorm Controls → "Glow & Volumetric" tab → "ゴッドレイ (太陽方向ビーム)" section
   - "ゴッドレイ有効化" checkbox が default ON
   - "ビーム集中度" default 30.0、"ビーム強度" default 0.10
3. 新 3 default が初起動時に焼かれているか確認:
   - RenderShadowResolutionScale = 3.00
   - RenderFarClip = 400
   - RenderVolumetricLightingMultiplier = 4.0
4. PhaseExp slider min が 1 まで下がるか (旧 4 制限の撤廃)
5. floater 右余白が縮小 (幅 500px) され、コンパクトに収まっているか
```

**sentinel 既適用ユーザーの注意点**:

すでに mode 2 で起動済みなら `AYACinematicOverlayApplied=1` が立っていて
新 default は再焼きされない。次のいずれかが必要:

- (a) Debug Settings から `AYACinematicOverlayApplied=0` に手動 reset → 再起動
- (b) mode 0/1 → 2 に切り替え (sentinel auto reset される)→ 再起動

## 検証後の debug settings 戻し案内 (release note 用)

`feedback_restore_debug_settings` 準拠。検証で持ち上げた値があれば下記表で戻す:

| cvar | 検証で持ち上げる可能性 | 戻す値 |
| --- | --- | --- |
| `AYAR15GodraysPhaseExponent` | スライダーで A/B 比較 | 30.0 (default) |
| `AYAR15GodraysStrength` | スライダーで A/B 比較 | 0.10 (default) |
| `AYACinematicOverlayApplied` | 0 に reset した場合 | 1 (再焼き完了後に勝手に戻る) |
| `AYAR15GodraysCinematicMigrationVersion` | デバッグで 0 に戻す可能性 | 1 (migration 完了後) |

## 残タスク

| # | 状態 | 内容 |
| --- | --- | --- |
| #11 | pending | SSS controls を Preferences → AYAstorm Controls に full move (godrays 受入後着手) |
| #12 | pending (parking lot) | sun-facing mesh blow-out (tonemap/exposure 軸) 調査。godrays とは独立、別 phase |
| #14 | completed | Cinematic 3 defaults 追加 (本セッション分) |
| #15 | completed | build + install (本セッション分) |

## 次に何をすべきか (Resume チェックリスト)

1. AYA さんから検証結果を聞く (MAX blend の見た目、新 3 default の有効化確認、min 1 動作)。
2. 検証 OK なら **commit してよいか AYA さんに確認** (`feedback_no_auto_commit`)。
   commit する場合の単位推奨:
   - C1: SUN_SHADOW permutation fix (llviewershadermgr.cpp) — 単独 bug fix として独立
   - C2: godrays cvar 化 + UI label 統一 + 右余白縮小 (settings.xml + llshadermgr.h/cpp + llsettingsvo.cpp + godraysF.glsl + pipeline.cpp + xui en/ja)
   - C3: Cinematic default flip migration (settings.xml の `InCinematicEnabled=1` + `CinematicMigrationVersion` + llappviewer.cpp + llcinematicoverlay.cpp/h)
   - C4: Cinematic 3 defaults (settings_cinematic_bd.xml + llviewermenu.cpp)
3. push / PR は AYA さん側 (`feedback_release_flow`)。
4. Task #11 (SSS migration) に着手するか AYA さんに確認 (godrays 確定後)。
5. Task #12 (mesh blow-out) は別 phase。今 phase では触らない。

## 注意点

- 「壊さない」原則: BD-port `renderVolumetric` (50→4 default 変更) と r15 `doGodrays` は
  **完全に別 path**。両者を混同して片方の挙動で他方を診断しない。
- `RenderVolumetricLightingMultiplier=4.0` は **Cinematic overlay でのみ 4.0**、
  Firestorm View / AYAstorm View では LL default 50 のまま (BD改善 phase 思想:
  Cinematic default は変えるが他 mode は触らない)。
- `AYAR15GodraysInCinematicEnabled` の default flip は **flag のみ反転**、
  migration は既存ユーザー救済用の sentinel one-shot。逆戻ししたいユーザーは
  Debug Settings から false に再設定可能。
