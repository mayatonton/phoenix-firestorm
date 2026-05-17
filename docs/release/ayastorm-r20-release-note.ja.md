# AYAstorm r20 — リリース告知

GitHub release ページ貼り付け用の文案。**r20 は視覚的リアリティ章 (r14〜r20) の第 7 弾、B 軸 (物質色) 完走リリース**。screen-space SSS (subsurface scattering) を **アバター肌** に適用し、自分にも他人にも、柔らかく内側から灯る肌の質感をもたらします。**gbuffer3 `.a` の per-pixel skin mask**、**世界座標スケール blur** で距離自動 fade、**右クリック学習** で新規 mesh body/head を瞬時に追加 (UUID を打ち込む必要なし)。

> **配信形態**: r20 は **r23 リリースに同梱配信** されます (r20 単独タグは発行しません)。r23 リリースページから本ノートと r20 spec doc にリンクする運用です。

実装・既知 limits・設定の詳細は永続資料 (`docs/specs/spec_avatar_skin_sss.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r20 — アバター肌 SSS (subsurface scattering)

### r20 の柱: 肌が生きている

A 軸 (r14〜r18) で空気を信頼に足るものに、r19 で薄物を太陽光で透かしました。r20 は **肌そのもの** — 「人」を写すときの中核を取りに行きます:

- **肌が適切なスケールで柔らかくなる** — separable 5-tap SSS blur に波長依存重み (赤が緑/青より遠くまで diffuse)
- **肌が subtle に光る** — glow restore (`pow(lit, 3) × glow_gain × warm_salmon`) で blur が眠くした hi-light を取り戻し、warm salmon tint で「血色のある肌」感を補強 (発光する陶器 ではなく)
- **自分にも他人にも効く** — 識別子は **mesh asset UUID**、既存の `ObjectUpdate` で viewer ローカルに来るため sim 往復ゼロ、装着者協力不要

SL/OpenSim viewer 史上初の viewer-side avatar skin SSS。主要 Mesh body/head 製品 (Maitreya、Legacy、Reborn、eBody、LeLutka Evolution heads、Genus 等) は安定した mesh UUID を使うため、いくつか覚えさせれば普通の SL 風景の 8〜9 割をカバーします。

### 仕組み

**識別子は mesh asset UUID** (単一軸):

`getVolume()->getParams().getSculptID()` を各 avatar の attachment から viewer ローカルで取得。UUID が whitelist cvar にあれば `mIsSSSTarget = true` をマーク。sim 往復なし、Description 編集不要、inventory item 名 lookup (他人に効かない) も不要。

**右クリック学習 UX**:

装着物を右クリック → **「Add to SSS whitelist」** → mesh UUID が whitelist cvar に追記 → cvar 変更シグナルが全 avatar 再評価をトリガ → 同じ body/head を使っている **他の全員にも瞬時に伝播**。ユーザーは UUID を一度も見ない。

フラット文脈メニュー (`menu_attachment_self/other.xml`) とパイメニュー (`More >` 配下) の自分/他人 4 経路すべてに登録。

**Per-pixel skin mask** (Phase C):

`gbuffer3` を `RGB16F` → `RGBA16F` に拡張。`.a` チャンネルに skin bit を持つ。SSS pass は (emissiveRect 経由で) per-pixel に bit を読み、肌 pixel だけに blur 適用 — 服 / 髪 / 眼鏡 / 目は無影響。

**世界座標スケール blur** (Jimenez "Separable SSS"、Phase D):

```
r_eff = aya_blur_radius / max(eye_dist_m, 1m)
```

- 1m 以内は `aya_blur_radius` 上限で頭打ち (近接の SSS を維持)
- 1m を超えると半径が距離に逆比例で減少
- 約 10m で半径 < 1px = 構造的に no-op

**これは以前の smoothstep 距離 fade (cvar 2 件) を置換** したもの。smoothstep は中距離で「ボケのボケ」問題が継続したため pivot。世界座標スケール式は距離 fade の判断ロジックを自動キャンセルする。

**Glow restore** (Phase D1):

```
glow_additive = pow(blurred_lit, 3) × glow_gain × glow_color
```

blurred 結果に additive で 1 pass、追加 RT なし。default の warm salmon (1.0, 0.65, 0.5) は「肌が発光する」感ではなく「血色のある肌」感を出すために選択。

### 設定 (Preferences → Graphics → SSS タブ)

| Key | Default | 役割 |
|---|---|---|
| `AYAR20AvatarSkinSSSEnabled` | `1` (ON) | SSS master switch |
| `AYAR20AvatarSkinSSSBlurRadius` | `1.0` | eye_dist=1m 基準の pixel 半径 (それ以降は世界座標スケール) |
| `AYAR20AvatarSkinSSSStrength` | `0.7` | Blur strength |
| `AYAR20AvatarSkinSSSGlowGain` | `3.0` (max 5.0) | Glow restore 強度 |
| `AYAR20AvatarSkinSSSGlowColor` | warm salmon (1.0, 0.65, 0.5, 1.0) | Glow tint |
| `AYAR20AvatarSkinSSSWhitelist` | — (ユーザーが育てる) | 改行区切りの mesh UUID 一覧 |

UI には各 cvar の **Default** ボタン、**Reset all to defaults** ボタン、whitelist text editor を read-only にする **Lock** チェックボックス (誤編集防止) も同梱。

### 既知の制約

- **シード UUID list は出荷しない** — 初回起動は「whitelist 空」状態。右クリック学習で育てる経路。主要 body/head UUID のシード同梱は community list 維持設計 (§6.1) と合わせて r21+ で検討
- **Transmittance は r21+** — 耳 / 指先の真の透過光 (rim glow 物理) は r20 範囲外、diffuse 側 SSS のみ
- **skin tone 別パラメータ無し** — 単一 global の blur / strength / glow 値、人種/肌色別調整は r21+
- **mesh でない attachment はメニュー grey out** — 古い sculpt prim や通常 prim は mesh UUID が取れないため
- **Pre-PBR / 古い hair shader** は non-PBR variant で `.a` 書込みが異なる可能性、SSS pass は「ここに skin pixel は無い」として safely fallback (blur されないだけ)

### 実装概要

- `llayaskinsss.{h,cpp}` (新規) — `SkinSSSMatcher` singleton、mesh UUID 抽出、whitelist パース、全 avatar 再評価
- `class1/deferred/skinSSSV.glsl` + `skinSSSF.glsl` (新規) — 2-pass separable SSS blur、波長依存重み、世界座標スケール半径、`emissiveRect.a` 経由 skin mask
- `pipeline.cpp` — `doSkinSSS()` 2-pass driver、gbuffer3 を RGBA16F に拡張
- `llvoavatar.cpp` — `attachObject` / `detachObject` で `SkinSSSMatcher` 連携
- `llviewermenu.cpp` — `SSS.Add` / `SSS.Remove` / `SSS.EnableAdd` / `SSS.EnableRemove` ハンドラ
- `panel_preferences_sss.xml` (新規) — Preferences → Graphics → SSS タブ
- `menu_attachment_self/other.xml` + `menu_pie_attachment_self/other.xml` — 右クリックエントリ
- `settings.xml` — 6 件の SSS cvar (Enabled / BlurRadius / Strength / GlowGain / GlowColor / Whitelist)
- Shader cache tag を bump (`AYASTORM_SHADER_CACHE_TAG = "AYAstorm r20"`) して古い compiled shader を自動無効化

### ドキュメント

- r20 full spec (Phase A–E ステータス / 識別子戦略比較 / debug settings 戻し案内): `docs/specs/spec_avatar_skin_sss.md`
- gbuffer3 storage 拡張リファレンス (RGBA16F skin bit): memory `reference_gbuffer3_storage.md`
- deferred shader routing リファレンス (writer → SSS mask path): `docs/ayastorm-deferred-shader-routing.md`
- 視覚的リアリティ章ロードマップ (B 軸完走 = r20): `docs/ayastorm-visual-realism-roadmap.md`
- sustained 検証で上書きした debug settings の戻し案内: memory `feedback_restore_debug_settings.md`
