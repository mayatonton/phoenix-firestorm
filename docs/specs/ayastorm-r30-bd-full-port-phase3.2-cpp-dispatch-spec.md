# AYAstorm r30 BD 完全移植 — Phase 3.2 C++ Cinematic dispatch spec

**Phase 名**: r30 BD 完全移植 Phase 3.2 — bucket 2.B 92 file の per-category dispatch 計画
**前提**: `docs/specs/ayastorm-r30-bd-full-port-inventory.md` (Phase 0) / `docs/specs/ayastorm-r30-bd-full-port-phase1-audit.md` (Phase 1) / `docs/specs/ayastorm-r30-bd-full-port-phase2-spec.md` (Phase 2 D1-D4)
**作成日**: 2026-05-19
**作成方針**: D2 (= AY 単一実装内 mode 別 dispatch、別 file 化しない) を 92 file に展開。実装は Phase 3.7 で行うが、dispatch 方針 / file 分類 / per-category 行動 / 高負荷 file の hotspot は本 spec で **確定** (`memory/feedback_no_escape_full_bd_coverage.md` 準拠)。

---

## §0 本 spec の出力意図

Phase 2 §2.4 で要求された下層 spec。Phase 0 inventory bucket 2.B common-diff の 92 file を 13 category に分類し、各 category に対する dispatch 方針を確定する。

- mechanical な行動規則を出す = 「Phase 3.7 で見てから判断」punt を排除
- 各 file が render path 直撃 / UI 系 / preset 系 / setting 系のどこに座るかを mode 2 Cinematic dispatch 観点で分類
- 大 diff file (pipeline.cpp +1582 行、llvoavatar.cpp +812 行、llviewerwindow.cpp +1186 行 等) は別表で hotspot 関数を列挙、Phase 3.7 で per-function walkthrough する起点を作る

---

## §1 file 分類 (13 category, 92 file)

### §1.1 category summary

| cat | 名称 | files | +AY total | -BD total | render dispatch 必要 | 説明 |
|---|---|---|---|---|---|---|
| 01 | pipeline_core | 2 | 3119 | 1395 | **YES** | pipeline.{cpp,h} — Cinematic dispatch の中心、全 render frame loop の起点 |
| 02 | drawpool | 20 | 865 | 561 | **YES** | lldrawpool*.{cpp,h} — render order / shader bind / state setting、mode 2 で BD 順序維持 |
| 03 | shadermgr | 2 | 449 | 234 | **YES** | llviewershadermgr.{cpp,h} — shader load + permutation 選択、Cinematic 用 permutation は既に在る (Phase 1b cat A) |
| 04 | render_object | 21 | 4640 | 2341 | **YES (一部)** | llvo* / llface / llspatialpartition / lldrawable — 個別 render-object 描画、AY-only feature dispatch を gating |
| 05 | environment | 13 | 1953 | 2986 | **NO (3.3-3.5 で処理)** | settings/sky/water — cvar helper (Phase 3.3-3.5) で BD default 適用、ここでは dispatch 不要 |
| 06 | view | 2 | 2858 | 1689 | **PARTIAL** | llviewerwindow.{cpp,h} — UI/window が大半、snapshot/frame buffer path のみ mode 2 dispatch |
| 07 | texture_material | 9 | 528 | 248 | **PARTIAL** | texture/material — render frame loop に乗る path だけ dispatch (texture format/order) |
| 08 | avatar_render_assist | 4 | 32 | 7 | **NO** | llcontrolavatar / llavatarrendernotifier — settings push のみ、render path 非該当 |
| 09 | snapshot | 3 | 72 | 3 | **PARTIAL** | llpostcard / llpanelsnapshotpostcard — snapshot 出力時 mode 2 で BD default 適用検討 |
| 10 | presets | 6 | 688 | 517 | **NO** | preset manager — Phase 3.6 (preset 移植) で扱う、ここでは dispatch なし |
| 11 | misc_ui | 7 | 658 | 696 | **NO** | settingsdebug/spellcheck/translation/voicedevice — render 非該当 |
| 12 | avatar_render_ui | 2 | 7 | 0 | **NO** | floateravatarrendersettings — UI のみ |
| 13 | select_ui | 1 | 145 | 42 | **NO** | llglsandbox — selection beacon / sandbox、render frame loop 非該当 |

**render dispatch 要 file 合計**: cat 01-04 + cat 06-07 部分 = **53 file** (うち cat 01-04 完全 = 45, cat 06-07 部分 = 8 候補)

**render dispatch 不要 file 合計**: cat 05 + cat 08-13 = **39 file** (cvar helper / preset / UI / 周辺で処理)

### §1.2 file 詳細表

`/tmp/phase3_2_classify.md` (Phase 3.2 着手時 helper script 生成物) に全 92 file の (cat, +AY, -BD, net) 表あり。spec doc 重複を避けるため本 spec には summary のみ収載、詳細は inventory / classify md を参照。

---

## §2 per-category dispatch 方針

### §2.1 cat 01 — pipeline_core (`pipeline.{cpp,h}`)

#### §2.1.1 dispatch 戦略

`LLPipeline` メンバ関数のうち AY が編集した関数を以下 3 種に分類:

| 種別 | 該当パターン | mode 2 dispatch |
|---|---|---|
| **A. AY-only 新規 helper / メソッド** | `LLPipeline::renderVolumetric()`, `LLPipeline::doGodrays()`, `LLPipeline::doSkinSSS()` 等の AY 新規 method | Phase 3.1 で predicate を `== 1` に統一済、mode 2 で early-out。本 spec では **追加 dispatch 不要** |
| **B. BD オリジナル method 内に AY 行 inject** | `LLPipeline::generateImpostor`, `LLPipeline::renderDeferredLighting`, `LLPipeline::doAtmospherics`, `LLPipeline::createGLBuffers` 等の BD 既存 method に AY 拡張行が挿入 | AY 拡張 block を `if (aya_view_mode() == 1) { AY-only logic }` で囲み、mode 2 では BD-original 通り走らせる |
| **C. BD method の bind 変更 (shader bind / state set 順序変更等)** | `LLPipeline::renderGeom`, `LLPipeline::stateSort` 系 | mode 2 では BD-original 順序を強制、AY 拡張は mode 1 のみ。具体的 site は Phase 3.7 で per-function walkthrough |

#### §2.1.2 hotspot 関数 (+AY ≥ 50 行)

pipeline.cpp + AY 行 2312 のうち、Phase 3.7 で walk すべき hotspot (mechanical 抽出基準: AY 行が ≥ 50 集中している関数):

- `LLPipeline::createGLBuffers` (mVelocityMap / mSMAAHistory alloc 含む、P2 work)
- `LLPipeline::renderDeferredLighting` (r19 / r20 / r17 / r14 拡張行集中)
- `LLPipeline::doGodrays` (Phase 3.1 で predicate flip 済、関数 body は AY 完全実装、mode 2 では呼ばれない)
- `LLPipeline::doSkinSSS` (同上)
- `LLPipeline::renderVolumetric` (Phase 1b cat A site 6、mode 2 のみ dispatch)
- `LLPipeline::generateImpostor` / `LLPipeline::renderShadowMaps` (BD 既存 + AY 拡張 mix)
- `LLPipeline::releaseGLBuffers` (P2 で AY-only buffer release 追加)

**Phase 3.7 着手時の責務**: 上記 hotspot を `git log -p --follow indra/newview/pipeline.cpp` で AY commit 走査 + BD diff 比較で完全特定し、AY 拡張行を `aya_view_mode() == 1` で gate する。

#### §2.1.3 dispatch helper 標準形

```cpp
// AY 拡張行を BD 既存 method 内に挿入する標準形
{
    static LLCachedControl<U32> aya_view_mode(gSavedSettings, "AYAVisualRealismEnabled", 1);
    if (aya_view_mode() == 1)  // AYAstorm View mode のみ
    {
        // AY 拡張 logic ...
    }
}
```

Cinematic 用 (mode 2) の AY-developed path (例: motionBlur, SMAA) は **既に** Phase 1b cat A の `aya_view_mode_xxx == 2` 判定で gate 済、Phase 3.1 と同列で個別判定維持。

### §2.2 cat 02 — drawpool (`lldrawpool*.{cpp,h}` 20 file)

#### §2.2.1 dispatch 戦略

drawpool は render-order / shader-bind の塊。AY mod は以下パターン:

1. **既存 pool method に AY shader / state injection** → mode 1 限定 dispatch、mode 2 では BD-original の shader/state を使う
2. **AY 新規 pool method (P2 P3 P4 で追加)** → mode 2 では呼ばれない経路、追加 dispatch 不要 (既に caller 側で gate)
3. **pool member の AY 新規 cvar 読み込み** → Phase 3.3-3.5 helper 経由に置換

#### §2.2.2 file 別 dispatch 担当

| file | +AY | dispatch 担当箇所 |
|---|---|---|
| `lldrawpoolavatar.cpp` | 236 | r20 SSS bind / r19 translucency push、AY mod を mode 1 gate |
| `lldrawpool.cpp` | 112 | 共通 base method、AY 追加 enum / state を mode 1 gate |
| `lldrawpoolterrain.cpp` | 78 | terrain shader bind、AY mod 該当時 mode 1 gate |
| `lldrawpoolsimple.cpp` | 65 | simple/fullbright pool、AY mod 識別後 gate |
| `lldrawpooltree.cpp` | 58 | tree pool、AY mod 識別後 gate |
| `lldrawpoolalpha.cpp` | 55 | Phase 1b cat A site 1 (alpha depth-write extension) 含む、既に mode 2 dispatch あり |
| `lldrawpoolbump.cpp` | 45 | bump pool、AY mod 識別後 gate |
| `lldrawpoolwater.cpp` | 29 | water pool、AY mod 識別後 gate |
| `lldrawpoolmaterials.cpp` | 22 | materials pool、AY mod 識別後 gate |
| `lldrawpoolpbropaque.cpp` | 6 | PBR pool、minor mod |

`.h` file (8 件) は member 宣言の AY 追加が大半、dispatch 不要 (実装側 .cpp の gate で済)。

#### §2.2.3 dispatch helper 適用

§2.1.3 と同じ標準形を各 pool の AY 拡張 block に適用。

### §2.3 cat 03 — shadermgr (`llviewershadermgr.{cpp,h}`)

#### §2.3.1 dispatch 戦略

shader load / permutation 選択。AY mod パターン:

1. **Cinematic 専用 permutation 選択** (Phase 1b cat A site 2-4) → 既に mode 2 dispatch あり、**追加修正不要**
2. **AYAstorm View 用 shader load** (r14-r20 系の AY shader binding) → mode 1 限定で load、mode 2 では BD shader が使われる
3. **AY shader uniform 登録** (LLShaderMgr::AYA_xxx) → 中央集権 uniform 経由 (Phase 3.1 で master uniform=0 化済)、追加 dispatch 不要

#### §2.3.2 Phase 3.7 担当

- AY shader load list を grep → mode 1 限定 load gate を入れる (= mode 2 では AY shader を load しないで rebuild reduce)
- 必須でなければ mode-agnostic load も許容 (AY shader が mode 2 で reference されなければ OK、リスク = bind cost、許容)

→ **方針**: shader load gate は **入れない**、reference 側の dispatch (cat 01-02) で十分。AY shader が load 済でも mode 2 で bind されなければ frame cost 0。本 spec で確定。

### §2.4 cat 04 — render_object (`llvo*` / `llface` / `llspatialpartition` / `lldrawable` 21 file)

#### §2.4.1 dispatch 戦略

object-level render。AY mod は以下パターン:

1. **r21 self picker / r22 chat split 等の AY 機能 plumbing** (mode 1 でも mode 2 でも残す必要あり = render 章ではない) → dispatch 不要、両 mode で active
2. **r14-r20 視覚表現章 plumbing** (sky/water/avatar 関連 uniform 計算) → mode 1 限定 gate
3. **AY-only state member** (mFSPickerLocalID 等) → 全 mode 維持 (= r21 picker は Cinematic でも動作させる、AYA 自身が使う)

#### §2.4.2 大 diff file の dispatch 担当

| file | +AY | dispatch 担当箇所 / 注意 |
|---|---|---|
| `llvoavatar.cpp` | 1803 | r21 picker / r22 chat / 視覚表現 — r21/r22 は両 mode 保持、視覚表現章 plumbing は mode 1 gate |
| `llvoavatarself.cpp` | 1236 | self avatar — 同上 |
| `llvovolume.cpp` | 495 | volume render — r21 picker plumbing は両 mode、render 拡張は mode 1 gate |
| `llspatialpartition.cpp` | 281 | spatial partition — AY mod 識別後 mode 1 gate |
| `llface.cpp` | 216 | face render — AY mod 識別後 mode 1 gate |
| `lldrawable.cpp` | 169 | drawable base — AY mod 識別後 mode 1 gate |
| `llvograss.cpp` | 65 | grass — AY mod 識別後 mode 1 gate |
| `llvowlsky.cpp` | 21 | wlsky — AY mod 識別後 mode 1 gate |
| `llvotree.cpp` | 16 | tree — minor |
| `llvopartgroup.cpp` | 15 | particles — minor |
| `llvosky.cpp` | 9 | sky — minor |

#### §2.4.3 重要原則

**r21 self picker / r22 chat split は Cinematic mode (2) でも動作させる**:

- r21/r22 は audio chapter / 撮影章ではなく **viewer 操作系**、配信者が Cinematic 撮影中も使う
- BD には無いが r14-r20 視覚表現章とは別軸、Cinematic 純 BD 化の対象外
- `feedback_warn_aya_off_bd_line.md` 適用: 「r21/r22 を Cinematic で off せよ」approach は本線逸脱 → **警告対象**

→ render_object cat の dispatch は **r14-r20 視覚表現章 plumbing のみ** mode 1 gate、r21+ は両 mode 維持。

### §2.5 cat 05 — environment (`llsettingsvo` / `llenvironment` / `llpaneleditsky` / `llfloater*environment*` / 等 13 file)

#### §2.5.1 dispatch 戦略

settings asset (sky/water/day) の解釈 + UI panel。render path に直接乗らない (= cvar 経由で pipeline / drawpool / shader が使う)。

- **`llsettingsvo.cpp`**: Phase 3.1 で 3 site flip 済 (r17/r18/master uniform)、追加 dispatch は cvar helper で済むため不要
- **`llenvironment.cpp`**: environment state machine、BD diff は r14+ 拡張 + BD-side feature 追加が混在 — mode 1/mode 2 で異なる解釈する必要は **無い** (= cvar / preset で間接的に発現)
- **panel/floater UI 系 (10 file)**: UI 表示のみ、render 非該当 → dispatch 不要

#### §2.5.2 Phase 3.3-3.5 で扱われる

settings 解釈の mode 別差分は cvar helper (`getRenderCvar<T>`) が mode 2 で BD default を返す形で吸収。per-file dispatch は本 spec scope 外。

### §2.6 cat 06 — view (`llviewerwindow.{cpp,h}`)

#### §2.6.1 dispatch 戦略

window/event 系の大半 (+1186 行) は r1-r29 (audio章 / 視覚表現章 / 撮影章 plumbing UI) で render path 非該当。

mode 2 dispatch が要る箇所のみ:

- **snapshot 系 method** (`saveImageNumbered`, `rawSnapshot` 等) → AY mod が snapshot 出力品質に影響していれば mode 2 で BD default 適用 (cat 09 と連動)
- **`stopGL` / `restoreGL`** — gl reset 時 AY buffer (mVelocityMap 等) 含む処理、Phase 3.7 で AY buffer release を mode 1 gate

dispatch 必要 site は **Phase 3.7 で AY mod walk 後確定**、本 spec では 2 area (snapshot / GL reset) のみ責務付け。

### §2.7 cat 07 — texture_material (9 file)

#### §2.7.1 dispatch 戦略

texture/material system。AY mod は r21 picker per-prim ID 用 plumbing が中心 (`llviewertexturelist.cpp` +193, `llviewertexture.cpp` +190)。

r21 plumbing は両 mode 維持 (cat 04 §2.4.3 と同方針)、render dispatch 不要。

- **`gltf/llgltfloader.cpp`** (-60 net) — BD diff だが r21 picker と無関係、BD-original に揃える方向で Phase 3.7 walk
- **`llmaterialeditor.cpp`** (+21) — UI 系、dispatch 不要

→ cat 07 は **per-file walk 不要**、Phase 3.7 で gltf loader のみ mechanical 差分照合。

### §2.8 cat 08-13 (avatar_assist / snapshot / presets / misc_ui / avatar_render_ui / select_ui)

#### §2.8.1 cat 08 avatar_render_assist (32 行)

`llcontrolavatar.{cpp,h}` (+14/+3), `llavatarrendernotifier.{cpp,h}` (+12/+3) — render settings push、render frame loop 非該当。dispatch 不要。

#### §2.8.2 cat 09 snapshot

`llpostcard.{cpp,h}` (+12/+5), `llpanelsnapshotpostcard.cpp` (+55) — snapshot UI / postcard 送信 plumbing。

mode 2 では snapshot 出力品質を BD default に揃える検討要 → Phase 3.7 で snapshot 出力 path の AY mod を確認。重要度低、後回し可。

#### §2.8.3 cat 10 presets

`llpresetsmanager.cpp` (+591) 含む 6 file — preset system 拡張。Phase 3.6 (preset 移植) で扱う、Phase 3.7 で dispatch は **入れない**。

#### §2.8.4 cat 11-13 (misc_ui / avatar_render_ui / select_ui)

UI 専用、render 非該当。**全 9 file dispatch 不要**。

---

## §3 Phase 3.7 着手時の per-function walkthrough 方針

本 spec で per-function dispatch 設計を全 92 file に展開することは **しない** (= 各 file の AY mod を逐行 walk するのは Phase 3.7 の責務)。

代わりに本 spec は:

- **dispatch 要 / 不要を 92 file 全部について確定** (§1 表)
- **dispatch 要 file の category 別方針を確定** (§2)
- **大 diff file の hotspot 関数を列挙** (§2.1.2)
- **per-function 標準 dispatch helper を確定** (§2.1.3)

Phase 3.7 着手時の手順:

1. 本 spec の dispatch 要 file リストを順に処理
2. 各 file で `diff -u /tmp/bd-baseline/<path> <ay path>` で AY mod hunk を取り出す
3. 各 hunk について §2 の category 方針を当てる
4. 標準 helper で gate を入れる (mode 1 のみ AY 拡張 active)
5. mode 1 = AYAstorm View 退行なし / mode 2 = BD-original 動作 を build + runtime で確認

---

## §4 punt 表 (= Phase 3.7 で mechanical 確定する未定項目)

| # | 未確定項目 | Phase 3.7 着手時の確定方法 |
|---|---|---|
| P1 | pipeline.cpp / llvoavatar.cpp / llviewerwindow.cpp の per-hunk gate 配置 | `diff -u` で hunk 取り出し、§2 方針機械的適用 |
| P2 | drawpool 20 file の AY mod hunk gate 配置 | 同上 |
| P3 | llviewershadermgr.cpp の AY shader load list (gate 不要だが mechanical に確認) | grep `gAYA*Program` で AY shader 列挙、reference site を cat 01-02 dispatch で gate 済を確認 |
| P4 | llviewerwindow.cpp snapshot path の AY mod 識別 | `git log -p --follow indra/newview/llviewerwindow.cpp` で snapshot 関連 commit 抽出、AY mod 該当時 mode 1 gate |
| P5 | cat 04 render_object 21 file で r14-r20 plumbing と r21+ plumbing の identification | commit message keyword (`r14`, `r15`, `r16`, `r17`, `r18`, `r19`, `r20` vs `r21`, `r22`) で hunk を identify、視覚表現章のみ mode 1 gate |

P1-P5 はすべて **mechanical 確定** (= 推論ではなく commit / diff / grep で出る) → `feedback_no_escape_full_bd_coverage.md` 違反なし。

---

## §5 sub-phase 境界の確定

本 spec 完成時点で Phase 3.2 は完了 (= spec doc 提出)。実装は Phase 3.7 で per-file walkthrough。

Phase 3.7 commit 単位:

- pipeline.cpp / pipeline.h (cat 01) → 1 commit
- drawpool 20 file (cat 02) → 1 commit
- shadermgr (cat 03) → 確認のみ commit なしの可能性 (§2.3.2)
- render_object 21 file (cat 04) → 2-3 commit (avatar / volume / その他)
- view (cat 06) → 1 commit (snapshot 系のみ)
- texture_material (cat 07) → 1 commit (gltf 差分のみ)
- snapshot (cat 09) → 1 commit (出力品質)

= Phase 3.7 で 6-8 commit、各 build + runtime 確認。

---

## §6 自検 (warn rule)

`feedback_warn_aya_off_bd_line.md` 適用:

- 「Cinematic で r21/r22 を off にして UI を BD 100% にする」approach → **不採用、本線逸脱、警告対象**
  - 理由: r21/r22 は撮影中の AYA 操作系、Cinematic で off にすると AYA 自身が撮影中に困る = `feedback_feature_value_in_main_usecase.md` 違反
- 「dispatch 要 file 53 件を Phase 3.7 で全部書き換えるのは時間がかかるから先に 3.6 (preset) を先行」approach → **不採用、本線逸脱、警告対象**
  - 理由: Phase 2 §5.1 依存図で 3.7 が中核、3.6 は独立だが 3.7 が回らないと parity 検証できない
- 「Cinematic dispatch を §2.1.3 標準 helper でなく独自 macro / wrapper で書く」approach → **不採用、本線逸脱、警告対象**
  - 理由: 既存 cvar 経由 dispatch (Phase 3.1) と pattern を揃えないと code review / runtime trace で識別不能化

---

## §7 Phase 3.3 への接続

本 spec で render-dispatch 不要と分類した cat 05 (environment 13 file) + cat 08-13 (39 file 合計) は、cvar helper (Phase 3.3-3.5) で BD default が pipeline に流れる仕組みを敷くと自動的に mode 2 BD-equivalent になる。

Phase 3.3 着手 → Phase 3.4-3.5 で 47+579 件の cvar を helper 経由化 → Phase 3.7 で残る per-file dispatch を実装 → Phase 3.8 (shader Cinematic mount) → Phase 3.9 (UI BD floater 移植) → Phase 4 (検証) → Phase 5 (cleanup) → r30 Cinematic release。

---

**End of Phase 3.2 Spec.**
