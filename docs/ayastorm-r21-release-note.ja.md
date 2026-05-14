# AYAstorm r21 — リリース告知文案

GitHub release ページ貼り付け用の文案。**r21 は視覚的リアリティ章 (r14-r20) の外側にある UX 修正系の単機能リリース**で、自分の rigged attachment に対する右クリック picker を「上流 CPU bind-pose mesh ray」から「GPU 専用 object-ID buffer」に作り直したものを 1 本で提供します。

実装・既知 limits・設定の詳細は永続資料 (`docs/ayastorm-r21-self-rigged-picker.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r21 — GPU self-rigged picker

### r21 の柱: 自分の rigged attachment を右クリックすると「画面で見えているもの」が選べる

r20 までは自分の rigged attachment (Mesh body / 服 / 髪) を右クリックした時、Firestorm / Linden 標準の picker が動いて world ray と **CPU bind-pose mesh** の交差を取っていました。これは構造的に以下 3 症状を出していました:

- **closeup zoom** で服をクリック → 服でなく自分のアバター本体に解決される
- **アルファ抜き髪** が顔の前にあると、ray が透明 triangle を pierce して髪が取られる
- **idle animation で rig が微動する frame** → CPU bind-pose と GPU skinning が ~4-5cm ズレて ray が外れる

r21 では self attachment の picker を **専用 GPU object-ID buffer** (`mObjectIDBuffer`) に切り出し、`gAgentAvatarp` の rigged attachment を **視覚パスと同じ skinning matrix** で再描画して、各 prim の LocalID を 4 byte channel に pack して書き込みます。右クリック時はマウス pixel の byte quad を読み戻して LocalID を復元 → 該当 prim を attachment tree から解決します。CPU/GPU drift も alpha-discard 不一致も idle skin lag も **構造的に発生し得ません** — picker が見ているのは画面に出ているピクセルそのものだからです。

詳細 → spec `docs/ayastorm-r21-self-rigged-picker.md`

### 設定

| キー | 既定 | 用途 |
|---|---|---|
| `FSSelfRiggedPickerEnable` | `1` | picker の master switch。`0` で r20 以前の上流挙動に完全に戻る |
| `FSSelfRiggedPickerGPU` | `1` | GPU buffer pass の kill-switch。`0` = picker 無効化 (上流 worldray を素通し)。Mac software OpenGL 等で GPU pass が破綻した場合の逃げ道として用意 |

> **CPU fallback は意図的に提供しません。** GPU pass が動く (`GPU=1`) か、picker そのものが無効化される (`GPU=0`、= r20 以前と同じ) か、のいずれかです。これは memory `feedback_root_cause_not_dump.md` (半分動く workaround を残さない) と `feedback_feature_value_in_main_usecase.md` (主流ユースケースで機能の存在価値を判定する) に基づく設計判断です。

### 既知の制約

- **alpha-blend 髪越しの顔選択**: GPU buffer は深度共有なので alpha-discard 三角形は picker に届きませんが、本当に alpha-blend で描かれて depth に書かない髪は依然として貫通します。AYA 評価で「仕方ない」と確認済 (2026-05-14)。
- **HUD attachment**: picker bypass (HUD は別 camera / screen-space で `mObjectIDBuffer` に書かれない)。
- **非 rigged self attachment** (ピアス / 単独 jewelry prim 等): GPU buffer は `PASS_*_RIGGED` のみ走るので、非 rigged は GPU buffer の id=0 を「picker の管轄外」と扱い、上流 worldray の object をそのまま採用します (`lltoolpie` の `isRiggedMesh()` ガードで自動処理)。
- **他者 avatar の rigged attachment**: r21 スコープ外 (self-only)。r22+ で検討予定。

### ドキュメント

- r21 spec / アーキテクチャ / 既知 limits / risk register: `docs/ayastorm-r21-self-rigged-picker.md`
- BoM body の rig hash collision 解決の経緯 (M4.17): memory `project_skin_hash_collision_bom_body.md`
- deferred shader routing reference: `docs/ayastorm-deferred-shader-routing.md`
