# AYAstorm r30 — リリース告知

**r30 は撮影品質を狙って r30 章で構築してきた新しい描画エンジンを「新しい AYAstorm View」として出荷するリリース**。ビューモード picker は 2 つだけ — `Firestorm View` と `AYAstorm View` — に整理され、新エンジン (velocity buffer + SMAA T2x + Volumetric Light + BD クラス DoF chain + Motion Blur + Chromatic Aberration + 35 cvar の AYAstorm Controls floater) がそのまま「AYAstorm View」となります。

> **配信形態**: r30 は r25〜r30 を一括配信するタグの 1 機能として出荷されます。同梱される他リリースの release note は GitHub Release ページから直接リンクされます。

実装の系譜 (P1 再起動切替インフラ → P2 velocity buffer → P3 Volumetric Light → P4 BD DoF chain → P5 BD parity gate → P6 live BD cvar port → Phase 6 Controls Cleanup → r30 release picker reshuffle) と migration / 残置 code 設計は `docs/specs/` 配下に歴史記録として残しています。r30 release 判断の **単一の真実** は `docs/specs/ayastorm-r30-view-mode-reshuffle.md` です。本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r30 — View Mode picker reshuffle: Cinematic を AYAstorm View に promote

### r30 の柱: AYAstorm View が新しい描画エンジンになる

r14〜r20 にかけて AYAstorm は Firestorm のベースラインの上に視覚的リアリズム拡張層を積み上げ、r24 までの picker で「AYAstorm View」と呼ばれていました。r30 章 (P1〜P6) では、その横に並行して別経路で描画エンジン本体を移植・構築し、内部では「Cinematic」と呼ばれる 3 番目のモードとして BD クラスの撮影品質を目指していました。

r30 release で編集判断を下しました:

- 新エンジンは章の目標に到達した
- ユーザーに「Firestorm View / AYAstorm View / Cinematic」の 3 モードを覚えさせるのは、要するに「エンジンが良くなった」という事実に対して説明過多

そこで **新エンジンを新しい AYAstorm View として出荷** します。picker は 2 つに整理:

| Picker | 描画 pipeline | 用途 |
|---|---|---|
| Firestorm View | Linden / Firestorm ベースライン | 配信視聴、作業用、低リソース運用 |
| **AYAstorm View** (default) | 新しい描画エンジン (velocity buffer / SMAA T2x / Volumetric Light / BD クラス DoF / Motion Blur / Chromatic Aberration / r14〜r20 拡張は opt-in) | 撮影、machinima、日常の没入用途 |

### upgrade 後の初回起動で何が起きるか

r24 までの旧 AYAstorm View (永続化されていた `AYAVisualRealismEnabled = 1`) は、起動時の冪等 1-shot migration で `2` (新しい AYAstorm View) に書き換えられ、`AYAViewModeMigrationVersion = 1` がスタンプされます。ログには次のような行が出ます:

```
View mode migration v0->v1: AYAVisualRealismEnabled 1 (legacy AYAstorm View) -> 2 (new AYAstorm View)
```

ユーザー向けプロンプトは出しません。あなたの視点では「エンジンが新しくなった」だけです。

### モード切替は再起動必須

Firestorm View と AYAstorm View の切替は AYAstorm を再起動して反映されます。pipeline は起動時に 1 回構築され、稼働中のセッションは常に唯一のモードを反映します (r30 章の設計どおり — フレーム途中の pipeline 再構成に起因する種類のバグを避けるため、ランタイム gate は意図的に採用していません)。

### AYAstorm Controls (Alt+C)

章中で導入していた Cinematic Controls floater は、**`AYAstorm → AYAstorm Controls...`** (`Alt+C`) として開けます。新エンジンの調整用 cvar — BD live cvar (shadow / DoF / fullbright / lights / global light / post FX)、per-channel shadow tuning、r14〜r20 の AYA 視覚リアリズム拡張を新エンジンの上に個別 opt-in する 6 件の cvar 等 — を提供します。

> **内部命名は据え置き**。内部 mode index `AYAVisualRealismEnabled == 2`、`AYACinematicModeActive` helper cvar、`AYASTORM_CINEMATIC` shader `#define`、`LLCinematicOverlay` namespace、`floater_aya_cinematic.xml` ファイル、`settings_cinematic_bd.xml` overlay 等はそのまま残しています。rename はユーザー視点で benefit ゼロの churn-only refactor になり、commit / comment 中の BD upstream lineage の可読性も損なわれるため、promote は UI 上の rename + 1-shot migration だけに留めています。詳細: `docs/specs/ayastorm-r30-view-mode-reshuffle.md` §2.2。

### 旧 AYAstorm View はどうなる?

r14〜r20 の旧 AYAstorm View (mode `1`) は **picker から削除** されますが、コードパス自体は残置されています。何らかの理由で再訪したい場合、Debug Settings で `AYAViewModeMigrationVersion = 0` **と** `AYAVisualRealismEnabled = 1` の両方を書き戻して再起動すれば、一時的に旧モードを覗けます — ただし次回起動時 migration が再走して mode `2` に書き戻されます。これは意図通り (章の編集判断としては「エンジンが差し替わった」のであって「2 つのエンジンから選べる」状態にはしていません)。

旧 AYAstorm View の個別 r14〜r20 効果 (大気遠近、godray、aerial perspective、色温度、cloud volumetric、translucency、avatar SSS) は、新エンジンの上に **個別 opt-in する追加機能** として AYAstorm Controls floater から到達可能です。新しい AYAstorm View ではこれらは default OFF で出荷、必要に応じて ON にすれば新 pipeline の上に r14〜r20 層を載せられます。

### 設定

通常運用ではユーザー操作不要 (AYAstorm View が default)。

| Cvar | Default | 役割 |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0` = Firestorm View / `2` = AYAstorm View (新エンジン)。**再起動必須**。`1` は Debug Settings + `AYAViewModeMigrationVersion=0` 経由でのみ書き戻し可能、次回起動の migration で再書換 |
| `AYAViewModeMigrationVersion` | `0` → migration 後 `1` | 1-shot migration の冪等 gate。意図して migration 再走させたい場合以外は触らない |

新エンジンのパラメータ調整は **AYAstorm Controls** floater (`Alt+C`) に集約。

### 既知の制約

- **モード切替は再起動必須**: セッション中の live 切替は意図的に採用していません。次回起動から反映
- **旧 AYAstorm View は unsupported**: 上述の Debug Settings 経路で覗けるが、r30 release で表向きにはサポートしない方針 — migration は通常手段では不可逆を意図設計
- **システム body (Ruth/Roth) は motion blur 除外**: 新エンジンのベースラインに合わせた挙動 (`LLDrawPoolAvatar::renderMotionBlur` が完全コメントアウト)。modern rigged mesh アバターは他の pool 経由で motion blur が乗ります
- **macOS OpenGL deprecation 監視**: 章全体と同様、新エンジンの shader chain は将来の macOS toolchain で fallback が必要になる可能性があります。r30 出荷時点では既知の regression なし

### 実装サマリ

- picker reshuffle commit (`6a6b657441`) の変更: 10 ファイル — `settings.xml`、`llcinematicoverlay.{h,cpp}` (migration helper)、`llappviewer.cpp` (起動順序)、`panel_preferences_graphics1.xml` (en/ja)、`menu_viewer.xml` (en)、`floater_aya_cinematic.xml` (en/ja)、`floater_about.xml` (en)
- Migration 起動順序: `applyAYAViewModeMigrationIfNeeded()` は `applyCinematicOverlayIfNeeded()` の **前** に走らせ、upgrade ユーザーが同一起動で新エンジン overlay も適用されるよう ordering を確保
- 完全な系譜 (P1 → P6 + Controls Cleanup + view-mode reshuffle) は `docs/specs/` の r30 spec 群に分散

### Credits

新エンジンの pipeline は外部の撮影志向描画エンジン (NiranV Dean 氏の Black Dragon viewer、LGPL-2.1、viewerlgpl と互換) から大量に拝借しています。クレジットは `floater_about.xml` に明記、port spec の header に BD repository commit ref を保持しています。

### 関連資料

- r30 release 判断の単一の真実: `docs/specs/ayastorm-r30-view-mode-reshuffle.md`
- 章 status block (歴史記録、reshuffle へ forward 参照): `docs/specs/ayastorm-r30-cinematic-chapter.md`
- P1 再起動切替インフラ (歴史記録): `docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md`
- Phase 別 spec (P2 velocity buffer / P3 Volumetric Light / P4 DoF chain / P5 BD parity / P6 live cvar port / Cinematic Controls Cleanup): `docs/specs/ayastorm-r30-*.md`
- BD live cvar port reference: `docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`
- Cinematic Controls floater audit: `docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`
