# AYAstorm r21: self rigged attachment GPU picker

**作成日**: 2026-05-14 (cleanup commit 直前に資料起票)
**対象**: AYAstorm `feature/aya-r21.1-self-rigged-picker`
**位置づけ**: r21.0 (picker initial REPLACE) と r21.1 (GPU 化 + 掃除) を **合体 1 リリース**として出荷。視覚表現章 (r14-r20) とは独立した **UX 修正系**

> **本書の役割**: r21 picker の **動機 / アーキテクチャ / 実装記録 / 設定 reference / 既知 limits** の永続資料。Release Note (`docs/ayastorm-r21-release-note.{en,ja,zh}.md`) は本書へのリンク + 差分ハイライトで構成 (memory `feedback_release_notes_link_only.md` 方針)。

---

## 1. 動機 — 上流 worldray の不一致

Firestorm / Linden 標準の右クリック picker は `LLPipeline::lineSegmentIntersectInWorld()` で **CPU bind-pose mesh × world ray** を交差判定する。これは static prim では正確だが、自分の rigged attachment (Mesh body / 服 / アクセサリ) に対して以下の症状を起こす:

- **closeup zoom** で攻撃クリック (例: 顔のすぐ手前) → 服を選んでいるつもりが「自分のアバター本体」に解決される (= 上流が hit 出来ず fall-through)
- **アルファ抜き髪** 越しに顔を選ぼうとすると、ray が髪 prim の透明 triangle を pierce してしまい「髪」が選ばれる (= 視覚と不一致)
- **idle animation 中** に rig が微動する frame で、CPU bind-pose と GPU skin の位置が ~4-5cm ズレる → ray が外れて「ragdoll aim」感

これらは CPU mesh ray の構造的限界で、tolerance を広げて誤魔化すと「漠然と当たるが何を選んでるか不明」になる (= memory `feedback_root_cause_not_dump.md` の「半分動く誤魔化し」)。

### 1.1 解決方針

「**画面に出ているピクセルそのもの**」が真実。であれば GPU で 1 枚 RGBA8 buffer を作り、`gAgentAvatarp` の rigged attachment を **同じ skinning matrix で再描画**して、各 prim の **LocalID を 4 byte に pack** して書き込む。右クリック時はマウス pixel の byte quad を読み戻して LocalID を復元 → 該当 prim を resolve。

CPU/GPU drift も alpha-discard 不一致も idle skin lag も **構造的に発生し得ない** (visible scene と同じ skinning 結果を使う)。

---

## 2. アーキテクチャ

```
[deferred gbuffer pass (visible scene)]
        │
        │ depth finalised
        ▼
[renderSelfRiggedObjectIDBuffer()]            ←── pipeline.cpp:9923
  ├ FSSelfRiggedPickerGPU で gate
  ├ mObjectIDBuffer.bindTarget()               ←── RGBA8 / WorldViewRectRaw
  ├ glColorMask(GL_TRUE×4) + clear (0,0,0,0)
  ├ depth share + LEQUAL test (write off)
  ├ glCullFace(GL_BACK)
  ├ gFSObjectIDShader.bind()                   ←── deferred/fsObjectID{V,F}.glsl
  └ for each PASS_*_RIGGED in kFSRiggedPasses:
       for each LLDrawInfo info:
         if info->mAvatar != gAgentAvatarp:      skip
         if info->mFSPickerLocalID == 0:        skip   ←── ※M4.17 per-prim id
         uniform object_id_packed = pack(id)
         uploadMatrixPalette(...)               ←── 同じ GPU skinning
         drawRange(...)
        │
        ▼
[mObjectIDBuffer: 各 attachment pixel に LocalID]
        │
        │ 右クリック発火
        ▼
[FSSelfRiggedPicker::findClosestAttachment]   ←── fsselfriggedpicker.cpp
  ├ mouse_x/y → buffer-local coord (HiDPI + UI chrome 補正)
  ├ glReadPixels(1x1, RGBA, UNSIGNED_BYTE)
  ├ U32 local_id = R | G<<8 | B<<16 | A<<24
  └ findSelfAttachmentByLocalID(gAgentAvatarp tree)
        │
        ▼
[lltoolpie.cpp]
  ├ picked          → object 差し替え + handleObjectSelection
  ├ id==0 && rigged → force-to-body (= avatar 本体)   ←── M4.18 ガード
  └ id==0 && non-rigged → 上流 worldray の object を尊重
```

### 2.1 構成ファイル

| ファイル | 役割 |
|---|---|
| `indra/newview/fsselfriggedpicker.{h,cpp}` | 右クリック呼出側の GPU buffer 読み出し / LocalID 復元 / attachment tree walk |
| `indra/newview/lltoolpie.cpp` | 右クリック handler。picker を呼ぶか / 上流結果を採るかの判定 (M4.18 rigged gate 含む) |
| `indra/newview/pipeline.{h,cpp}` | `mObjectIDBuffer` 確保 / `renderSelfRiggedObjectIDBuffer()` 本体 |
| `indra/newview/llspatialpartition.h` | `LLDrawInfo::mFSPickerLocalID` 新規 field (M4.17 per-prim identity) |
| `indra/newview/llvovolume.cpp` | DrawInfo 構築時に `mFSPickerLocalID` 書込 + batching merge 条件に LocalID 一致を追加 |
| `indra/newview/llviewershadermgr.{h,cpp}` | `gFSObjectIDShader` 登録 (vertex に `hasObjectSkinning=true`) |
| `indra/newview/app_settings/shaders/class1/deferred/fsObjectID{V,F}.glsl` | vertex: rig skinning で clip pos のみ生成 / fragment: 4-byte LocalID を pack して書き込み |
| `indra/newview/app_settings/settings.xml` | `FSSelfRiggedPickerEnable` (master) / `FSSelfRiggedPickerGPU` (kill-switch) |

### 2.2 shader

**`fsObjectIDV.glsl`** (vertex):
```glsl
mat4 mat = getObjectSkinnedTransform();           // 視覚パスと同じ
mat = modelview_matrix * mat;
gl_Position = projection_matrix * vec4(mat * vec4(position, 1.0)).xyz;
```
varying なし。normal なし。clip pos だけ生成して fragment へ。

**`fsObjectIDF.glsl`** (fragment):
```glsl
uniform vec4 object_id_packed;   // host が R=byte0 .. A=byte3 で詰める
out vec4 frag_color;
void main() { frag_color = object_id_packed; }
```

depth は `mRT->deferredScreen` と共有、LEQUAL test (write off)。alpha-discard された pixel は元から depth に居ないので picker buffer にも到達しない → **画面で見えない triangle は picker からも見えない** が構造的に成立。

### 2.3 buffer 配置と coordinate conversion

`mObjectIDBuffer` は **`WorldViewRectRaw` 寸法**で確保される (`pipeline.cpp::resizeScreenTexture()`)。つまり buffer の `(0,0)` は world view rect の bottom-left を raw pixel で示す。一方マウス座標 `LLCoordGL` は window-relative の **scaled (logical) pixel**。HiDPI と UI chrome の二段補正が必要:

```cpp
sx = WindowWidthRaw / WindowWidthScaled;        // DisplayScale
mx_win_raw = round(mouse_x * sx);
mx_buf = mx_win_raw - WorldViewRectRaw.mLeft;   // UI chrome offset
```

どちらかを忘れると `glReadPixels` が誤った pixel を読んで「id=0 (no hit)」を返す。 (M4.1 で初期実装、M4.2 でちらつき対策完成)

### 2.4 M4.17: per-DrawInfo LocalID (BoM body 衝突解決)

**問題**: 初期版は `unordered_map<skin->mHash, LocalID>` を pipeline.cpp で構築し、render 時に hash 経由で LocalID を引いていた。しかし `LLMeshSkinInfo::mHash` は **rig binding のハッシュ** (頂点データではない) で、BoM body のように **複数の rigged child prim が 1 つの avatar rig を共有**するケースで全て同じ hash になる。最後に iterate された child の LocalID が map slot を独占し、**体の全 pixel が 1 個の child prim に解決される** (例: 腕クリックが足 prim に化ける)。

**観測**: 2026-05-14 検証で「腕の disjoint 2 点クリックが両方とも foot prim の LocalID を返す」を log で確定。

**解決**: hash 介在を完全に除去し、`LLDrawInfo` 自体に `mFSPickerLocalID` を持たせる (`llspatialpartition.h:155`)。DrawInfo 構築時 (`llvovolume.cpp:5823`) に `vobj->getLocalID()` を直接 stamp。

**副次対策**: batching merge 条件 (`llvovolume.cpp:5767`) に LocalID 一致を追加。これがないと「同 rig / 同 material / 同 VB の 2 prim」が 1 つの DrawInfo にマージされて identity が再び消える。

詳細経緯: memory `project_skin_hash_collision_bom_body.md`。

### 2.5 M4.18: non-rigged attachment は upstream に委譲

GPU buffer は `PASS_*_RIGGED` のみ dispatch されるので、**rigged ではない self attachment (ピアス / 単独 jewelry prim) は構造的に id=0 を返す**。掃除前の lltoolpie.cpp は `gpu_authoritative && upstream_picked_self_attachment` だけで force-to-body していたので、ピアスクリック → 全部 body に取られて「ピアス選択不能」になった (2026-05-14 観測)。

**修正**: force-to-body 条件に `object->isRiggedMesh()` を追加 (`lltoolpie.cpp:2389`)。非 rigged は GPU buffer の id=0 を「picker の管轄外」と解釈し、upstream worldray の object を尊重する。

---

## 3. 設定

### 3.1 `FSSelfRiggedPickerEnable` (Boolean, default 1)

GPU picker 全体の **master switch**。OFF にすると lltoolpie の picker 呼び出し自体がスキップされ、上流 worldray の結果が無加工で使われる (= 完全に r20 以前の挙動)。

### 3.2 `FSSelfRiggedPickerGPU` (Boolean, default 0 → M5 で 1 予定)

GPU buffer pass (`renderSelfRiggedObjectIDBuffer`) と GPU readback の **kill-switch**。OFF の場合:

- `renderSelfRiggedObjectIDBuffer` は early-return (buffer 描画なし、コスト 0)
- `findClosestAttachment` も early-return `nullptr` + `out_gpu_authoritative=false`
- lltoolpie は force-to-body 条件にも入らず、上流 object をそのまま採用 → **r20 以前と同一挙動**

> **CPU fallback は廃止** (M4.7 で legacy stage を削除)。OFF = picker 無効化スイッチ、CPU 経路で picker が動くわけではない。これは memory `feedback_root_cause_not_dump.md` (半分動く誤魔化しを残さない) と `feedback_feature_value_in_main_usecase.md` (主流ユースケースで判定) に基づく設計判断。Mac 等で問題が出た場合のフォールバックは「**上流 worldray のみ**」になる。

### 3.3 検証後の戻し方表 (memory `feedback_restore_debug_settings.md` 方針)

| キー | 一時値 | 戻すべき default | 備考 |
|---|---|---|---|
| `FSSelfRiggedPickerEnable` | 0 (kill) / 1 (normal) | **1** | master、検証時に切るシナリオあり |
| `FSSelfRiggedPickerGPU` | 1 (検証 ON) | **0** (M5 までは default OFF / M5 で 1) | M5 リリース以降は default 1 |

---

## 4. 既知の limits

| ID | 限界 | 原因 / 回避 |
|---|---|---|
| L1 | **アルファ抜き髪越しの顔選択は完全には改善しない** | GPU buffer は深度共有なので alpha-discarded triangle は到達しないが、髪が **alpha-blend** の場合は depth に書かれない場面があり、これは GPU buffer も「貫通して顔を選ぶ」。AYA 評価で「仕方ない」と確認済み (2026-05-14) |
| L2 | **HUD attachment は picker 対象外** | HUD は別 camera (screen-space) で render され `mObjectIDBuffer` には書かれない。lltoolpie は HUD 判定で早期 bypass (M4.9) |
| L3 | **非 rigged self attachment は upstream worldray 依存** | ピアス等 — M4.18 で「id=0 でも force-to-body しない」処理を入れて upstream 結果を尊重 |
| L4 | **CPU mode (`FSSelfRiggedPickerGPU=0`) では picker が動作しない** | 構造的判断 (memory `feedback_feature_value_in_main_usecase.md`)。Mac の software OpenGL renderer 等で問題が出た場合はこの kill-switch で OFF にして上流動作に戻す |
| L5 | **他者 avatar の rigged attachment は対象外** | r21 スコープは self picker のみ。他者 picker は r22+ で別途検討 |
| L6 | **closeup zoom で id=0 を引くケースが残存可能性あり** | M4.8 として task 残置 (#72) — M4.17 hash collision 解決でほぼ抑え込めた感触だが、M5 検証時に再現したら根本究明 |

---

## 5. 受け入れ条件 (M5 検証で確認)

- [x] **rigged self attachment** (Mesh body の腕 / 頭 / 胴 / 服 / 髪): 右クリック → 正しい prim が pie menu に出る (M4.17 PASS)
- [x] **非 rigged self attachment** (ピアス / 単独 jewelry prim): 右クリック → 該当 prim が pie menu に出る (M4.18 PASS)
- [x] **他者 avatar / land / HUD**: picker bypass で上流挙動が壊れない (M4.9 で HUD 明示 bypass)
- [ ] **3 OS ビルド + 起動 + 動作確認** (Linux PASS、Win / macOS は M5 で実施 — memory `project_ayastorm_three_platforms.md`)
- [ ] **M5: `FSSelfRiggedPickerGPU` default を 1 に flip** (cleanup commit 後の別 commit)

---

## 6. リスク

| ID | リスク | 対策 / 現ステータス |
|---|---|---|
| R1 | M4.17 hash collision の再発 (新たな batching merge 経路や別 DrawInfo 構築点で identity が消える) | LLDrawInfo merge 条件と construction 両方にコメント明示 (`llvovolume.cpp:5762, 5817`)、memory `project_skin_hash_collision_bom_body.md` に検出 signal 記述 (skins=N vs drawn=M 不一致) |
| R2 | M4.18 ガードで rigged 判定が `isRiggedMesh()` virtual に依存。サブクラスで誤実装があると force-to-body が暴発 | LLViewerObject base 実装は false、LLVOVolume override が唯一の true 経路。Mesh body 以外の rigged は viewer 内に存在しないため実害は無いはず |
| R3 | Mac software OpenGL fallback で `mObjectIDBuffer.allocate()` は成功するが draw が遅い / 不正色 | `isComplete()` check で「allocate 失敗 → 自動 no-op」は保証済み。draw 異常は事前検証不能、release 後フィードバック頼り (memory `feedback_release_with_user_feedback.md`)、kill-switch cvar が保険 |
| R4 | `gFSObjectIDShader` の hasObjectSkinning permutation が他 platform で link 失敗 | `add_common_permutations(&gFSObjectIDShader)` で標準処理、`objectSkinV.glsl` 参照は他 shader と同形式 |
| R5 | closeup zoom shirt の id=0 (task #72 M4.8) が M5 検証で再現 | task 残置中。再現時は LL_PROFILE 取って深く trace、当てずっぽう shader 書き換えはしない (memory `feedback_render_full_trace_first.md`) |

---

## 7. 関連 memory / docs

- `project_skin_hash_collision_bom_body.md` — M4.17 の核心 (BoM body 複数 prim が rig hash 共有)
- `reference_deferred_shader_routing.md` / `reference_gbuffer3_storage.md` — 描画系を触る前の必読 reference
- `feedback_root_cause_not_dump.md` — CPU fallback 廃止の判断根拠
- `feedback_feature_value_in_main_usecase.md` — Mac CPU mode 切り捨て判断
- `feedback_release_with_user_feedback.md` — Mac 事前検証不能を release-feedback で補う方針
- `feedback_render_full_trace_first.md` — 描画系 trace 優先
- `docs/ayastorm-deferred-shader-routing.md` — deferred 経路 reference
- `docs/ayastorm-gbuffer3-trace.md` — gbuffer3 仕様
- `docs/ayastorm-visual-realism-roadmap.md` — 章全体 (r21 は視覚表現章の外、UX 修正系)

---

## 8. 更新履歴

- 2026-05-08 (`940b989ca5`) **r21 起票 + initial picker REPLACE**: 上流 worldray を独立 fallback で差し替え。CPU bind-pose ray を 3 stage (depth-assist / pierce / near-miss) で組んで Linux PASS。memory `feedback_upstream_bug_replace_over_fix.md` 適用例 (上流バグ修正でなく差し替え)
- 2026-05-14 **r21.1 起票**: closeup zoom で「肌が選べない」「服が選べない」症状残存、idle skin drift で誤選択 1/5。CPU bind-pose の構造的限界を認識し **GPU buffer 方式** に転換 (M4)
- 2026-05-14 M4.1〜M4.6: GPU pass 初期実装 (mObjectIDBuffer 確保、shader 登録、draw pass、coord conversion、alpha-mask fix、back-face cull)
- 2026-05-14 M4.7: legacy CPU stage 廃止判断 (GPU 一本化)
- 2026-05-14 M4.8〜M4.13 Diag: depth 棄却 / hash collision / 画面 dump で根本原因切り分け。`skin->mHash` map collapse を確定
- 2026-05-14 M4.16: M4.15 で入れた `root_id` 伝播が誤り → revert (child prim 本人 LocalID 書込)
- 2026-05-14 (`f3c0829ea8`) **M4.17 Linux PASS**: `LLDrawInfo::mFSPickerLocalID` 導入で hash collision 完全解決。腕/頭/胴/服 全て正解
- 2026-05-14 **M4.18 PASS**: non-rigged ピアス選択不能を `isRiggedMesh()` ガードで解決
- 2026-05-14 **掃除 commit (この commit)**: CPU stage 0/1 ヘルパ ~400 行、CPU stage 関連 cvar 4 件 (`Tolerance` / `DepthAssist` / `DepthTolerance` / `DumpBuffer`)、per-click diag log、Diag-C/D PNG dump を全削除。net **-746 行**。掃除後の `findClosestAttachment` は 60 行台、GPU 1 path のみ
- (M5 予定) cvar `FSSelfRiggedPickerGPU` default = 1 に flip、3 OS 検証完走、`v7.2.x-ayastorm-r21` tag/release
