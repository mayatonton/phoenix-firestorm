# AYAstorm r30 P5 — BD parity 構築 phase 事前 spec

**作成日**: 2026-05-19 (初稿) / 2026-05-19 (BD parity pivot 再定義)
**ステータス**: 起草中 (P4 ship 直後 / step 0a 完了)
**スコープ**: r30 章 §3 P5 の事前 trace。P2〜P4 で個別に borrow した BD 機能を、**「BD と同じ絵が作れる」レベル**まで詰めるための構築 phase 仕様。**単一 release ではなく複数 release (P5 / P5.1 / P5.2 …) に分割される可能性のある phase 全体の指針**。実装ステップは含む (作業者向け案内)。
**上流参照**: BD `995a1354d8` (Version to 5.6.2, 2026-04-19) — P2/P3/P4 と同じ参照点。本 phase は新規 borrow 候補がさらに増える前提 (pipeline 系統差分の audit 結果次第)。

---

## 0. 本 spec の再定義経緯 (2026-05-19)

### 0.1 当初 P5 想定 (drop)

当初 P5 は「Cinematic Controls floater への cvar 集約 + View Mode UI の正式モード昇格 + 第三者ブラインド A/B 判定」を 1 release で行い、判定通過したら Cinematic を正式モードに昇格させる単発 ship gate を想定していた。

### 0.2 pivot 契機 (AYA 判断)

P4 ship 直後の floater + UI 仕上げまで完了した時点で、AYA 自身が次のように確認:

> 「この時点の見た目では わたしが見ても BD に完敗だと思うので見せる価値がない」
> 「アルゴリズム的にすでに追いついているのかの判断は わたしにはちょっとわかってない」
> 「BD と完全に同じ絵を作れますか？」(現状: ノー)
> 「まず同等に追いついて はじめてその上を狙えます」

→ 「第三者に見せて評価」以前に、**AYA 自身の目で BD と区別がつかないレベルに到達していない**。よって P5 を以下に再定義:

### 0.3 新 P5 定義

**P5 = 「BD parity 構築 phase」**。Cinematic mode が BD と「同じ絵」を出せるところまで詰める phase。 phase 全体の出口条件は:

> **AYA 自身の目で、同一 location / 同一構図で撮影した BD vs AYAstorm Cinematic のスクリーンショットを並べて、どちらが BD でどちらが AYAstorm か区別がつかなくなる**

到達して初めて Cinematic を「正式モード」として README / release notes で告知する。それ以前は preview のまま (View Mode UI の `Cinematic (preview)` ラベル維持)。

「BD を超える」のは P6 以降 (AYA 独自の色作りで差別化)。**「超える」は「並ぶ」の後にしか存在しない**。

### 0.4 step 0a 完了 (View Mode UI 昇格の取り消し)

新定義に従い、当初 P5 想定で先行実施していた View Mode UI の preview ラベル除去 (combo / tool_tip / comment 全 3 箇所) を 1 commit で revert 済 (`6b1ef33377`)。preview ラベルは parity 到達まで維持。

---

## 1. 概要

### 1.1 P5 phase スコープ

- **in-scope**:
  - BD render pipeline と AYAstorm Cinematic mode の **系統的 diff audit** (どこで絵が乖離しているかの俯瞰)
  - **BD-compat preset** の実装 (Cinematic 内で BD と等価な default 値群を一発でロードできる UI / cvar セット)
  - **Tone mapping 同等化方針** (AYAstorm ACES tone をそのままに置く / BD tone を borrow する / preset 内だけで切替、の選択)
  - 未 borrow の BD 機能の **段階追加** (audit 結果次第で P5.1 / P5.2 / … で増える)
  - **A/B 検証** (AYA 自己判定で BD と区別がつかなくなったかを継続的に観測)

- **out-of-scope** (P6+ へ):
  - AYA 独自色作り (Cinematic 用 Kelvin / LUT / aerial 値の **新規探索**)
  - BD UI 翻訳 (Machinima Sidebar / Photo Tools panel)
  - floater 多言語対応 (英語維持 / 日本語/中国語 lproj は parity 到達後に検討)
  - Phototools floater との DoF 系重複解消 (§1.4 参照、parity 到達後に再設計)

### 1.2 章 spec との整合 (2026-05-19 章 spec realign 必要)

章 spec §3 P5 は当初 「Cinematic Controls floater 集約 + 第三者ブラインド A/B 判定」を想定していた。本 pivot に合わせて章 spec §3 P5 を「BD parity 構築 phase」に書き換える必要あり (step 0b の chapter spec realign で実施)。

### 1.3 P5 が複数 release に分割される前提

「BD parity 到達」は単一 release で達成できる保証がない。spec 上は P5 を **phase 全体の指針**として扱い、実 release 単位は P5 / P5.1 / P5.2 / … と分割する余地を残す。各 sub-release は本 spec の §4〜§9 の step 単位で切る (例: P5 = step 4 audit + step 5 BD-compat preset、P5.1 = step 6 tone 方針、P5.2 = step 7 追加 borrow …)。

### 1.4 意図的に P5 では触らない不整合 — Phototools floater との DoF 系重複

**現状**: 既存 Firestorm Phototools floater (汎用撮影 UI、6 tab) にも DoF / 関連項目があり、P4 で新設した Cinematic Controls floater との間で同 cvar に 2 つの動線がある。

**P5 判断 (2026-05-19 AYA confirm、pivot 後も維持)**: **意図して触らない**。理由:

- Phototools は Firestorm 由来の汎用撮影 UI で、Cinematic mode 外でも使用される。撮影 workflow の **互換性** を残すことが Firestorm fork として価値がある
- 重複解消は parity 構築自体とは直交する課題で、parity 到達が先
- 初期段階は release notes / README で「Cinematic 系設定は Cinematic Controls floater (Alt+C) を推奨、Phototools の同名項目もそのまま動く」と明示するだけで案内する

**P6+ 再検討余地**: parity 到達後、Cinematic が「BD と並ぶ絵」を安定して出せる状態に入ってから、Phototools との UI 動線を再設計する余地あり。

---

## 2. 受入 (phase 全体の出口条件)

### 2.1 主受入

**AYA 自身の目で、同一 location / 同一構図 / 同一時刻 / 同一 Windlight で撮影した BD vs AYAstorm Cinematic のスクリーンショットを並べて、どちらが BD でどちらが AYAstorm か区別がつかない**。最低 5 scene (屋外昼景 / 屋外夕景 / 屋内 / 浅景深ポートレート / 動きのある pan) で達成。

判定は **AYA 自身が行えば十分**。第三者ブラインド A/B は parity 到達後の「BD を超えたか」判定 (P6+) に retract する。理由: parity 未達の状態で第三者に見せても評価コストに対して得られる情報量が少ない (「BD のほうが良い」が確定済)。

### 2.2 副受入 (各 sub-release ごと)

- step 4 audit: BD vs AYAstorm Cinematic の pipeline 差分が **項目化された表** として spec §4 に記録されている
- step 5 BD-compat preset: floater 1 click で BD と等価な default 群がロードされる + 再起動なしで効くもの / 再起動必要なものが UI 上で区別されている
- step 6 tone 方針: ACES 維持 / BD tone borrow / preset 内切替 のいずれかが選択された経緯と根拠が spec §6 に残っている
- step 7 追加 borrow: 各 borrow が `<FS:AYA r30 P5.x>` tag + BD commit hash + 出処 file 列で spec §7 に記録されている

---

## 3. 範囲 / 除外 (詳細)

### 3.1 in-scope (parity 構築のために触る可能性のあるもの)

- AYAstorm Cinematic mode で gating されている全 shader / cvar (P2 velocity buffer + SMAA T2x / P3 Volumetric Lighting / P4 BD DoF chain + Chromatic Aberration、および pipeline 順序)
- AYAstorm 既存の r14+ visual realism (godrays / aerial / Kelvin / cloud volumetric / SSS) のうち、Cinematic mode で active になる経路 (これらは AYAstorm View でも使われるが、Cinematic で BD parity を測る都合上、差分要因として audit 対象に含める)
- AYAstorm tone mapping (ACES) と BD tone の差 (§6 で個別判断)
- BD 上流の未 borrow 機能 (audit 結果次第で追加 borrow 候補に上がるもの)

### 3.2 out-of-scope (parity 構築では触らない)

- **Firestorm View mode (AYAVisualRealismEnabled=0)** の挙動 — per-mode gating で保護されているので parity 作業による regression が乗らない。AYA 確認済 (「再起動すれば Firestorm View は生き残らせられる」)
- **AYAstorm View mode (AYAVisualRealismEnabled=1)** の挙動 — r14+ visual realism の独自 default 値はそのまま維持。BD-compat preset は Cinematic mode (mode=2) のときのみ意味を持つ
- **AYA 独自色作り** (Cinematic 用の新規 Kelvin / LUT / aerial 値探索) — parity 到達後の P6+

### 3.3 mode 別影響範囲表

| View Mode | parity 構築作業の影響 |
|---|---|
| Firestorm View (0) | なし (per-mode gate により保護) |
| AYAstorm View (1) | r14+ default 値は変更しない / 既存 release との連続性維持 |
| Cinematic (2) | 全面的に対象。BD parity に向けて default / pipeline / 機能群を整える |

---

## 4. step 4: BD render pipeline 系統 diff audit

### 4.1 目的

「どこで絵が乖離しているのか」を、shader / cvar / pipeline ステージ単位で表化する。audit せずに思いつきで追加 borrow を進めると、parity 到達の見通しが立たない。

### 4.2 audit 対象軸

1. **shader 単位**: BD と AYAstorm Cinematic で同名 shader の中身 diff (deferred / softenLightF / atmos 系を最優先)
2. **cvar default 単位**: BD default vs AYAstorm default の値ずれ (既知: `RenderChromaStrength` BD 0.0 vs AYAstorm 5.0、`RenderVolumetricLightingMultiplier` BD 1.0 vs AYAstorm 50.0、その他 audit で網羅)
3. **pipeline ステージ単位**: BD と AYAstorm で post-pass の順序 / 経由 buffer / blend mode が一致しているか (ACES tone を挟む位置 等)
4. **取り込まれていない BD 機能**: BD にあって AYAstorm Cinematic にない post-pass / feature の網羅

### 4.3 audit 成果物

本 spec §4 末尾に **「BD diff 表」** を追記する形で残す。列は [軸 / 項目名 / BD 値 / AYAstorm 値 / 寄与する絵の差 / 対応 step (5/6/7) のどれで詰めるか]。

### 4.4 audit 進め方の指針

- BD `995a1354d8` 時点のソースを baseline にする
- diff は階層 grep の棚卸しで満足せず、踏まない path / 分岐条件 / opaque/blend/deferred/forward を必ず追加検証 (`feedback_analysis_depth.md`)
- 想像で当てない、まず Frame Profile で hot path を取って実 trace (`feedback_render_full_trace_first.md`)

### 4.5 audit が見つけそうな乖離仮説 (事前)

- AYAstorm 側で additive (ONE/ONE) post-pass が scene alpha を破壊している箇所が他にも残っている可能性 (`project_aya_visual_realism_alpha_protect.md` に既出修正あり、別箇所の audit 必要)
- AYAstorm の Kelvin / aerial 補正は AYAstorm View 由来で常時 on のため、Cinematic でも BD には無い色味が乗っている
- BD は volumetric multiplier=1.0 を ACES 前提でない tone で乗せている。AYAstorm 50.0 + ACES での見え方が一致するとは限らない (tone 同等化の議題に直結 — §6)

(本節は audit 実施前の仮説。step 4 完了時に確定表で上書き)

---

## 5. step 5: BD-compat preset 実装

### 5.1 目的

Cinematic Controls floater に「BD-compat」ボタンを 1 つ追加し、押下で audit (§4) で同定された default ずれを一括で BD 等価値に設定できるようにする。「同等に揃えた上で差を見る」を成立させる UI 基盤。

### 5.2 実装方針

- preset の中身は **cvar default の塊** (LLControlGroup::setFromXML 風の一括反映)
- preset 反映は「Cinematic mode (mode=2) であること」を gate
- 再起動なしで効く cvar と、再起動必要な cvar (例: `RenderVolumetricLightingDirectional` の permutation) を UI で分けて表示
- preset を当てた後、AYAstorm 既存 default に戻す逆 preset 「AYAstorm-default」ボタンも対で実装する (戻せないと検証往復が回らない)

### 5.3 preset の範囲

audit (§4) で同定された cvar 群を対象。最低でも以下を初期セットに含める想定:

- `RenderChromaStrength`: BD 0.0 ↔ AYAstorm 5.0
- `RenderVolumetricLightingMultiplier`: BD 1.0 ↔ AYAstorm 50.0
- (audit で追加されるもの)

### 5.4 受入

- preset 押下で Cinematic Controls floater の全 slider / checkbox 表示が即座に BD 等価値を反映する
- 「AYAstorm-default」押下で AYAstorm 既存 default に戻る
- preset の各 cvar に「再起動必要」マーカーが UI 上で見える

---

## 6. step 6: Tone mapping 同等化方針

### 6.1 議題

AYAstorm は ACES tone を採用、BD は別 tone。BD と「同じ絵」を出すには、tone の差が支配的になる可能性が高い (volumetric multiplier の 50× も ACES 圧縮を前提とした補正)。

### 6.2 選択肢

| 案 | 内容 | parity 影響 | AYA 色作り (P6+) への影響 |
|---|---|---|---|
| A. ACES 維持 (現状) | AYAstorm 全 mode で ACES、BD parity は ACES 上で BD と「等価に見える」値を探す | parity 到達可否は audit 次第。ACES 上で BD と完全等価が作れる保証はない | P6+ で ACES 上の独自色作りに直接つながる |
| B. BD tone borrow (Cinematic mode のみ) | Cinematic mode のとき tone を BD tone に切替、AYAstorm View / Firestorm View は ACES のまま | parity 到達確度は高い (tone から揃う) | P6+ で「BD tone の上に AYA 色」になる、独自性が薄れる懸念 |
| C. BD-compat preset 内だけで tone 切替 | tone は cvar 化、BD-compat preset 押下時のみ BD tone へ、preset off で ACES に戻る | parity を「BD-compat preset 適用時」に限定して達成、ship 標準は ACES 維持 | P6+ で独自色作りは ACES 上で進められる、preset は比較用 |

### 6.3 推奨初期方針 (要 AYA confirm)

**案 C (preset 内だけで tone 切替)** を初期方針として提案。理由:

- ship 標準が ACES のまま維持されるので、AYAstorm の独自色作り (P6+) を ACES 上で続けられる
- BD-compat preset 適用時に限り tone まで BD 等価になるので、parity 判定 (§2.1) は preset on で実施できる
- 案 B のように常時 BD tone にしてしまうと、AYAstorm の独自性が「BD tone の上の色補正」に閉じ込められる

### 6.4 確定タイミング

step 4 audit 完了 (BD vs AYAstorm の tone 差が定量化されてから) で最終決定。本節は audit 結果を見て更新する。

---

## 7. step 7: 未 borrow BD 機能の段階追加

### 7.1 目的

audit (§4) で「BD にあって AYAstorm Cinematic にない」と判定された機能を、parity に必要な順で段階的に borrow する。各 borrow は P5.x として独立 release で切る (P5.1 / P5.2 / …)。

### 7.2 borrow 候補 (事前、audit で確定)

- BD 側 SSAO / SSR / SSGI のうち AYAstorm 未取り込み箇所
- BD 側 Bloom / Glow チェイン
- BD 側 Color Grading / 露出制御
- BD 側 Vignette / Film Grain (撮影系)
- (audit で増減)

### 7.3 各 borrow の指針

- 1 borrow = 1 P5.x release (commit 単位ではなく release 単位、parity 観測しながら追加)
- 各 borrow に `<FS:AYA r30 P5.x>` tag、BD commit hash、出処 file 列を spec §7 末尾に追記
- borrow 時点の AYAstorm 既存 default は意図して BD default に合わせて出す (`feedback_match_bd_defaults_on_borrow.md`)、AYA 独自値は P6+ で再探索

### 7.4 borrow 判断基準

- 「BD と差が出ている scene を直接補える機能」を優先 (例: 屋外昼景で差が大きいなら bloom / SSGI / Sun shadow を優先)
- 既存 AYAstorm 機能と直交しない機能 (例: 既存 godrays と被る BD 機能は §4 audit で乖離原因として扱い、別 borrow にはしない)

---

## 8. step 8: same-picture A/B 検証 (継続)

### 8.1 検証方法

step 5 / 6 / 7 の各 sub-release ごとに以下を実施:

1. 同一 SL location / 同一構図 / 同一時刻 / 同一 Windlight で BD と AYAstorm Cinematic (BD-compat preset on) を交互に撮影
2. 5 scene 最小 (屋外昼景 / 屋外夕景 / 屋内 / 浅景深ポートレート / 動きのある pan)
3. PNG / 無加工 / 同解像度で並べて目視比較
4. AYA 判定: scene ごとに「BD と区別がつかない / どちらが BD か特定できる / どちらが明らかに勝っている」の 3 段

### 8.2 進捗ログ

各 sub-release の検証結果を本 spec §8 末尾に時系列で残す。列は [日付 / sub-release / scene / 判定 / 主な乖離原因 / 次 sub-release で詰める項目]。

### 8.3 phase 完了条件

§2.1 の主受入を満たす (= 全 5 scene で「区別がつかない」)。

---

## 9. step 9: clean up + release notes + Cinematic formal ship

### 9.1 トリガー

§8 の phase 完了条件を満たした時点。

### 9.2 作業内容

- View Mode UI を再昇格 (combo `Cinematic (preview)` → `Cinematic`、tool_tip 改修、comment header 改修) — step 0a で revert したものの再適用
- README に Cinematic を 3 番目の View Mode として正式紹介
- ja/en/zh release notes 起草 (Cinematic 正式 ship、BD parity 到達を主柱)
- 本 spec §10 に最終 commit log を記録
- Phototools との UI 動線重複の再検討 chunk を P6+ spec に持ち上げ

### 9.3 ship 後の位置づけ

P5 ship をもって r30 章は「BD と並走できる Cinematic を持つ」状態に到達。P6+ は AYA 独自色作り (= BD を超える) phase に入る。

---

## 10. 受入観測 / commit log (実行時に追記)

### 10.1 step 0a (2026-05-19): View Mode UI promotion revert

| commit | 内容 |
|---|---|
| `6b1ef33377` | combo `Cinematic` → `Cinematic (preview)` / tool_tip preview 状態 + 現状 borrow 機能列に書き戻し / comment header に P5 pivot 趣旨追記 |

### 10.2 step 0b (2026-05-19): P5 spec rewrite + chapter spec realign

(commit hash は実施後追記)

### 10.3 以降の step (4〜9)

各 sub-release 実行時に [step / 日付 / commit hash / 概要] を追記する。

---

## 11. 未確定事項 (各 step 着手時に再確認)

### 11.1 step 4 audit のスコープ

shader 全件 / cvar 全件を 1 release で audit するか、軸ごとに分割するか。audit のサイズが大きい場合、step 4 自体を sub-release 化する可能性あり。

### 11.2 step 5 BD-compat preset の実装手段

LLControlGroup の preset 機構流用 / 独自 setControl 連打 / 別 cvar group 切替、のいずれを使うかは実装時に判断。

### 11.3 step 6 tone 方針の最終確定

§6.3 で案 C を初期推奨としているが、audit 結果次第で案 A/B に倒れる可能性あり。AYA 最終確認が必要。

### 11.4 P5.x 命名規約

P5.1 / P5.2 / … の番号付け基準 (step 単位 / 機能単位 / 検証完了タイミング単位) は実 release 時に確定。本 spec §10 に履歴として残す。

### 11.5 release notes の preview / formal 表現

P5.x の各 sub-release は preview のまま、§9 の最終 ship で初めて「formal」と表現する。途中 release の表現粒度 (「BD parity 構築 (進行中)」と書くか「P5 phase 進捗」と書くか) は AYA 判断。

---

## 12. 参考: 章 spec との対応 (step 0b realign 後)

| 章 spec 章節 | P5 spec 対応箇所 |
|---|---|
| §3 P5 (BD parity 構築 phase) | 本 spec 全体 |
| §3 P6+ (AYA 独自色作り) | 本 spec §1.1 out-of-scope / §2.1 主受入到達後 |
| §4.2 P5 行 (cvar 整備 / 追加 borrow) | 本 spec §5 (BD-compat preset) / §7 (追加 borrow) |
| §6.3 P5 same-picture A/B | 本 spec §8 (継続検証) |
| §8.1 P5 工数 (phase 全体) | 本 spec §1.3 (複数 release 分割) |

---

(本 spec は P5 phase 全体を 1 本でカバーする指針として運用する。P5.x の各 sub-release で本 spec を更新しながら parity 到達まで継続。)
