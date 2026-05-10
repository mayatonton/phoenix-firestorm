# AYAstorm r13: OBB タグベース遮蔽 (occlusion) 実装工程

**作成日**: 2026-05-10 (r12.1 完了直後の r13 計画策定時)
**対象**: AYAstorm `feature/aya-r13-obb-occlusion` (予定 / 着手前)
**仕様**: `doc/spec_obb_occlusion.md`
**前リリース**: r12 main (`feature/aya-r12-stereo-upmix`、PR #46) / r12.1 (`feature/r12.1-lfegain`、PR #52)
**同梱バグ修正**: `feature/ll-chat-livetune-font-plaintext` (commit 2689a35f8f、ChatFontSize / PlainTextChatHistory live-apply on LL-style chat)

> **本書の役割**: phase 単位の作業内容と依存関係、検証チェックリストの **テンプレート** を記録する。
> リスク (`spec §7`)、受入条件 (`spec §6`)、r14+ への持ち越し (`spec §9`) は仕様書側を canonical とし、本書では参照する。
> 本書は r13 着手前の **計画スナップショット** であり、実装進行に伴って commit hash / 実測値を埋めていく。

---

## 1. ゴール

r10 で完成した **Layer 1 (per-channel placement)** / r11 で完成した **Layer 2 (lite-HRTF + venue reverb)** / r12 で完成した **Layer 0 (stereo→5.1 upmix)** に対し、**SL 世界の物理ジオメトリによる遮蔽** を初めて持ち込むリリース。これまで「音源側の設定」だけで完結していた音響レンダリングに、「listener と音源の間の物理空間が音を遮る」という SL ユーザが期待する実体験を追加する。

設計の核は **会場運営主導モデルの新規導入** (r11 配信者主導モデルの対パターン): 建物プリム Desc に追加した 2 タグキー (`[ayastorm:occlude]` / `[ayastorm:door]`) が root truth、default は「タグ未指定 = 遮蔽なし」、material 表 (SL prim material flag → preset) で direct/reverb 値を決定。listener 側の Preferences UI 改修は一切行わない。実装/検証時の独立 toggle / global multiplier 用に debug settings 4 件 (sentinel 1 件 + 実値 default 3 件) + range 設定 1 件のみ提供。

形状近似は **OBB のみで決め打ち**。sphere/cylinder/torus/mesh も box 近似、回折 / 反射 / 共鳴は持たない直線 raycast モデル。Steam Audio 統合 / 形状特化近似 / mesh 実 triangle 利用 / SOFA per-source HRTF / 個人 HRTF / 公開 README / air absorption 客観 FFT は r14+ への保留 (spec §9)。

r13 同梱バグ修正として `feature/ll-chat-livetune-font-plaintext` ブランチの ChatFontSize / PlainTextChatHistory live-apply fix on LL-style chat (commit 2689a35f8f) を merge。occlusion とは別レイヤーだが「単独 release を切るほどではないバグ修正は次の planned release の train に乗せる」(memory `project_ayastorm_r13_obb_occlusion`) 方針による。

---

## 2. フェーズ分解

viewer-only の改修。配信側パイプラインは r9 / r10 / r11 / r12 流用、追加変更なし。検証材料は r12 流儀で `doc/r13/` に scene 構築手順 + audio 素材スクリプトを置く。

### P0: 仕様確定 + roadmap doc 同時更新 + 実装箇所調査

**目的**: 旧 r13+ basket (SOFA / Steam Audio / 個人 HRTF / 公開 README / air absorption 客観 FFT / VenueReverb CPU 最適化) を spec / roadmap 双方で「OBB タグベース遮蔽 + chat font 同梱」に置き換え、r14+ への降格を明文化。同時に FMOD geometry API (`System::createGeometry` / `Geometry::addPolygon` / `setRotation` / `setPosition` / `set3DOcclusion`) の挿入位置を実コードで判定。`LLPositionalStreamMgr` (r5-r12 系) と独立する新規 mgr クラス `LLOcclusionGeometryMgr` の責務境界を確立。

**ファイル**:
- `doc/spec_obb_occlusion.md` (新規、本書 §1 / §2 と同構成、初版)
- `docs/ayastorm-stream3d-roadmap.md` §3 r13 (旧 r13+ basket → 新仕様、r14+ 項目追加、ロードマップ題名 r12 → r13 に拡張)
- `doc/r13/fmod_geometry_survey.md` (新規、P0 調査記録、FMOD geometry API の本 viewer 内利用例ゼロを確認、`indra/llaudio/` の FMOD include 状況、`LLViewerObject::getPositionRegion/getRotationRegion/getScale` 取得経路を確認)
- 上記 spec / impl record / roadmap への新仕様反映 (P0 第 1 commit)

**完了条件**: 仕様書 AYA レビュー通過 / roadmap §3 r13 が新仕様で読める / Preferences UI 改修ゼロ + 会場運営主導モデル方針が明文化 / 役割分担 (建物オーナー = 物理 / 配信者 = 表現 / 両者直交) が明文化 / FMOD geometry API の挿入位置 (各 3D channel の `set3DOcclusion` を FMOD が geometry 経由で自動更新する形) が確定 / `LLOcclusionGeometryMgr` の責務境界 (parsing + OBB 抽出 + lifecycle + 動的 door 追従) が確立。

**commit**: (P0 = 初版 spec / impl record / roadmap / メモリ更新一括)

---

### P1: LLOcclusionGeometryMgr skeleton + FMOD geometry lifecycle

**目的**: 新規 mgr クラス `LLOcclusionGeometryMgr` の骨格を `indra/newview/llocclusiongeometrymgr.{h,cpp}` に作る。`LLSingleton` パターン (`LLPositionalStreamMgr` と同じ流儀)。最初は **空の geometry を生成 + 全プリムタグ scan も登録もしない**、ただ FMOD `System::createGeometry` の lifecycle (起動時生成 / 終了時 release) のみ確立する。viewer 起動 → 終了で leak ゼロを確認。

**ファイル**:
- `indra/newview/llocclusiongeometrymgr.{h,cpp}` (新規)
  - `class LLOcclusionGeometryMgr : public LLSingleton<LLOcclusionGeometryMgr>`
  - `void initInstance()` で `System::createGeometry(maxPolygons, maxVertices)` (`Stream3DOccluderMaxCount × 6` / `× 8`)
  - `void cleanupSingleton()` で `Geometry::release()`
  - `void update()` (空、P5 で実装)
  - `FMOD::Geometry* mGeometry` (per-region 1 個、region 跨ぎは P5/P11 で再評価)
- `indra/newview/llappviewer.cpp` (起動時 `LLOcclusionGeometryMgr::initParamSingleton()` 呼出、終了時 `deleteSingleton()`)

**完了条件**: viewer 起動 → 終了で `mGeometry` の create/release 呼出回数が一致 / FMOD assert なし / コンパイル通る / 既存 r5-r12 動作に影響ゼロ (geometry が空なので `set3DOcclusion` が動かない = 従来通り)。

**commit**: (TBD)

---

### P2: タグ parser (`[ayastorm:occlude]` / `[ayastorm:door]`) + 範囲内 prim scan

**目的**: spec §4.1 のタグ書式 (`[ayastorm:occlude]` / `[ayastorm:occlude:N]` / `[ayastorm:door]` / `[ayastorm:door:N]`) を parse し、listener 周辺 `Stream3DOccluderRange` (default 64m) 内の prim の Description フィールドから抽出。`LLPositionalStreamMgr` のタグ parser から **独立した parser** として実装 (役割分担: viewer 物理タグと配信者表現タグは別系統)。

**ファイル**:
- `indra/newview/llocclusiongeometrymgr.{h,cpp}` (parser 追加)
  - `struct OccluderTag { LLUUID prim_id; bool is_door; F32 override_value; bool has_override; };`
  - `bool parseOccluderTag(const std::string& desc, OccluderTag& out)` (regex `\[ayastorm:(occlude|door)(:[0-9.]+)?\]`)
  - `void scanRangeForTags()` (`LLViewerObjectList` の active object iter、距離フィルタ + Desc parse)
- `indra/newview/llpositionalstreammgr.cpp` (関与なし、completely 独立)

**完了条件**: regex で 4 形式すべて parse、不正値 (`[ayastorm:occlude:abc]` 等) は silent ignore + chat 通知 1 回 (r11 throttle 機構を流用) / Description が空 / 関係ないタグ / 範囲外プリムは silently skip / scan 1 回が 1ms 未満 (200 prim 想定)。

**commit**: (TBD)

---

### P3: OBB 抽出 helper (LLOcclusionGeometryHelper)

**目的**: spec §4.3 の OBB 抽出ロジック (`LLViewerObject` → 8 vertices + 6 quads) を static helper として実装。`LLOcclusionGeometryMgr` から呼ばれるピュア関数群。

**ファイル**:
- `indra/newview/llocclusiongeometryhelper.{h,cpp}` (新規)
  - `struct OccluderOBB { LLVector3 vertices[8]; LLVector3 quad_normals[6]; F32 direct_occlusion; F32 reverb_occlusion; };`
  - `static bool buildOBB(const LLViewerObject* obj, OccluderOBB& out)` (`getPositionRegion / getRotationRegion / getScale` から 8 vertices 計算、6 face 構成)
  - face indices は spec §4.3.2 の時計回り 6 quad

**完了条件**: 立方体 prim (1m × 1m × 1m) を入力 → 8 vertices が ±0.5m に正しく配置 / 回転 prim (45° 回転) を入力 → vertices が region 座標で正しく回転 / linkset の子 prim も `getPositionRegion` で linked 状態の region 座標が取れることを確認 / mesh prim も `getScale` の bounding box で OBB 化される (= 過剰遮蔽になるが仕様通り)。

**commit**: (TBD)

---

### P4: material 表実装 + タグ override 適用

**目的**: spec §4.4 の material → preset 表を helper に実装。`LLViewerObject::getMaterial()` の `LL_MCODE_*` 値から direct/reverb occlusion 値を引く。タグ引数 (`[ayastorm:occlude:0.5]`) 指定時は preset を完全無視。

**ファイル**:
- `indra/newview/llocclusiongeometryhelper.cpp` (material 表追加)
  - `static void resolveMaterial(U8 mcode, F32& out_direct, F32& out_reverb)` (8 entry table、spec §4.4)
  - `static void applyTagOverride(const OccluderTag& tag, OccluderOBB& obb)` (override 値 → direct=reverb=value)

**完了条件**: spec §4.4 表の 8 material 全行が正しい preset 値を返す / `LL_MCODE_NONE` (default) → concrete (0.7, 0.5) / タグ引数あり → material 表を無視して引数値を direct/reverb 両方に適用 / 引数値が範囲外 (例: `2.0`) は clamp [0.0, 1.0]。

**commit**: (TBD)

---

### P5: 静的 occluder lifecycle (rez/derez/move/Description 編集検知 + UUID→polygon index map)

**目的**: spec §4.5.1 の静的 occluder ライフサイクル。プリム rez で OBB 計算 + `Geometry::addPolygon` ×6、derez で `setPolygonAttributes(active=false)`、move (100ms 以上の遅延) / Description 編集 / material 変化で再評価。UUID → polygon index map で個別 prim を追跡。

**ファイル**:
- `indra/newview/llocclusiongeometrymgr.{h,cpp}` (lifecycle 追加)
  - `std::map<LLUUID, std::array<int, 6>> mUuidToPolygonIndices;`
  - `void onObjectAdded(LLViewerObject* obj)` (rez hook、タグあり → OBB → addPolygon ×6 → map 登録)
  - `void onObjectRemoved(LLViewerObject* obj)` (derez hook、map から index 引いて `setPolygonAttributes(active=false)` + map 削除)
  - `void onObjectUpdate(LLViewerObject* obj)` (move/edit hook、100ms throttle 後に re-add or update)
  - `void update()` (poll loop、`Stream3DPollInterval` 30s で `scanRangeForTags()` 実行 + 範囲外プリムを deactivate)
- `indra/newview/llviewerobjectlist.cpp` (rez/derez hook を `LLOcclusionGeometryMgr` に通知、既存 `LLPositionalStreamMgr` と同じ pattern で配線)

**完了条件**: prim rez × 10 で polygon 数が 60 増える (10 prim × 6 face) / prim derez × 5 で 30 個が `active=false` になり map から削除される / move 後 100ms 以内の連続移動はまとめて 1 回の re-add / Description で `[ayastorm:occlude]` を追加 → 即時登録、削除 → 即時 deactivate / material flag 変更で preset 値が再計算される / 60s 動作で leak なし (polygon 数が一意に推移)。

**commit**: (TBD)

---

### P6: 動的 door 追従 (毎フレーム setRotation/setPosition)

**目的**: spec §4.5.2 の動的 door。`door` タグ付きプリムを `mDoorPrims` set に登録、`update()` 毎フレームで全 door の `getPositionRegion/getRotationRegion` を取得し、変化があれば `Geometry::setRotation/setPosition` で polygon を再配置。しきい値ロジックは入れない (spec §4.5.2、現実的な扉数 = 1〜10 個前提で API call は無視できる)。

**ファイル**:
- `indra/newview/llocclusiongeometrymgr.{h,cpp}` (door 追従追加)
  - `std::set<LLUUID> mDoorPrims;`
  - `std::map<LLUUID, std::pair<LLVector3, LLQuaternion>> mDoorLastTransform;`
  - `void updateDoors()` (毎 `update()` call で `mDoorPrims` を反復、変化検出 + `setRotation/setPosition` 呼出)
  - `update()` 内で `updateDoors()` を call、頻度は viewer フレームレート (60Hz)

**完了条件**: door prim を LSL の `llSetRot()` で 90° 回転 → 1 frame 以内に geometry の polygon 配置が追従 / smooth animation (60Hz `llSetRot` 連続呼出) でも同期、聴感的にカクつかず / door 5 個同時動作で CPU 増分が +0.1pp 未満 / 静止時 (transform 変化なし) は API call ゼロ (変化検出による early return)。

**commit**: (TBD)

---

### P7: cap (Stream3DOccluderMaxCount) + range cap (Stream3DOccluderRange) + listener 距離順ソート

**目的**: spec §4.5.3 の cap 機構。範囲内 occluder + door 合計が `Stream3DOccluderMaxCount` (default 200) を超えたら listener 距離順で打切り。範囲外 (`Stream3DOccluderRange` default 64m 超) は登録対象外。`update()` poll 時に毎回再評価 (= listener 移動で近い順が変わる)。

**ファイル**:
- `indra/newview/llocclusiongeometrymgr.{h,cpp}` (cap 追加)
  - `void applyCap()` (距離計算 → ソート → 上位 N 個のみ active、下位は deactivate)
  - `update()` 末尾で `applyCap()` 呼出
- `indra/newview/app_settings/settings.xml` (`Stream3DOccluderMaxCount` int 200 / `Stream3DOccluderRange` F32 64.0 追加)

**完了条件**: `Stream3DOccluderMaxCount = 5` で 6 個目以降の壁が透過 (近い 5 個のみ active) / listener が 50m 移動すると近い順が更新され、新しい近距離 prim が active になる / `Stream3DOccluderRange = 32` で 32m 超の prim が登録されない / cap re-evaluate が `update()` 30s ごとに実行される。

**commit**: (TBD)

---

### P8: debug settings 4 件 (Stream3DOcclusion sentinel + global multiplier 3 件) 配線

**目的**: spec §4.7 の listener 側 override。`Stream3DOcclusion` (sentinel `-1` = タグ通り、`0` = 強制 OFF、`1` = 強制 ON) と global multiplier 3 件 (`Stream3DOcclusionDirectGain` / `Stream3DOcclusionReverbGain` / `Stream3DOccluderMaxCount`) を `settings.xml` に追加し、`LLOcclusionGeometryMgr` が main thread から読む経路を作る。

**ファイル**:
- `indra/newview/app_settings/settings.xml` (`Stream3DOcclusion` int -1 / `Stream3DOcclusionDirectGain` F32 1.0 / `Stream3DOcclusionReverbGain` F32 1.0、`Stream3DOccluderMaxCount` は P7 で追加済み)
- `indra/newview/llocclusiongeometrymgr.cpp` (debug settings 値を `update()` 内で取得、`Stream3DOcclusion == 0` → 全 polygon を `active=false`、`!= 0` → タグ通り、Direct/Reverb gain は polygon の direct/reverb 値の global multiplier として適用)

**完了条件**: `Stream3DOcclusion = 0` → 即座に全 polygon が deactivate (タグ全無視) / `Stream3DOcclusionDirectGain = 0.0` → direct 遮蔽消える、reverb は維持 / `Stream3DOcclusionReverbGain = 0.0` → reverb 遮蔽消える、direct は維持 / `Stream3DOcclusion = -1` → default 動作 (タグ通り) / `Stream3DOccluderMaxCount = 5` → P7 で確認済の cap 動作。

**commit**: (TBD)

---

### P9: 検証 scene 構築手順ドキュメント化 + 検証材料生成スクリプト

**目的**: spec §5 の 4 種 scene (静的壁のみ / 扉付き / linkset 壁 / mesh 壁) を再現可能に構築する手順を `doc/r13/` にドキュメント化。audio 素材は r12 のスクリプト (`doc/r12/gen_upmix_test_material.sh`) を流用。

**ファイル**:
- `doc/r13/build_test_scene.md` (新規、scene A/B/C/D 構築手順、prim サイズ / material flag / LSL door script 含む)
- `doc/r13/door_toggle.lsl` (新規、scene B 用の単純な扉開閉 LSL、`llSetRot` で 90° 回転 toggle)

**完了条件**: scene A/B/C/D が手順通りに in-world で再現可能 / LSL door script が touch で開閉 / r12 audio スクリプトで stereo voice / wide stereo music / 5.1 native の 3 素材が引き続き生成可能。

**commit**: (TBD)

---

### P10: spec §6.1 検証手順 全 14 ステップ実行 (O1〜O14)

**目的**: spec §6.1 の O1〜O14 を順次実行し受入判定。配置者主導モデルなので in-world での実機聴感が必須。memory `feedback_one_step_at_a_time` に従い 1 メッセージ 1 アクションで進める。

**ステップ概要** (spec §6.1 詳細):
1. O1: scene A 室外/室内で direct sound の muffled→clear 変化
2. O2: scene A + venue=hall_medium で reverb も同様の変化
3. O3: scene B 扉閉/開で listener 真正面の muffled→clear 切替
4. O4: scene B 扉開、listener 斜め前 30° で direct そこそこ抜ける
5. O5: scene B 扉開、建物真横 (壁越し直線) で muffled 維持 (FMOD 直線 raycast の仕様確認)
6. O6: material 別 (wood/glass/stone) で遮蔽強度差
7. O7: タグ override `[ayastorm:occlude:0.5]` が material 表より優先
8. O8: `Stream3DOcclusion = 0` で強制 OFF
9. O9: `Stream3DOcclusionDirectGain = 0.0` で direct 消失、reverb 維持
10. O10: `Stream3DOccluderMaxCount = 5` で cap 動作
11. O11: 範囲外 (64m 超) の occluder が登録されない
12. O12: door 移動 (回転/並進) で遮蔽位置が即追従
13. O13: 配信者 r11 venue タグと occlusion が独立動作 (狭箱で野外 venue 許容)
14. O14: `llPlaySound` (オブジェクト効果音) も occlusion 適用

**完了条件**: spec §6.1 受入表 14 行が全 PASS / 必要なら debug settings の default 値や material 表を P12 close-out 時に再調整 (R3/R6 縮退策)。

(in-conversation 実行、commit なし)

---

### P11: r10 / r11 / r12 回帰確認 + chat font live-apply fix の同梱確認

**目的**: occluder ゼロ状態 (= タグなしの default 環境) で r10/r11/r12 完全互換が成立することを実機確認。同時に chat font live-apply fix (commit 2689a35f8f) が r13 ブランチに正しく merge され、LL-style chat で ChatFontSize / PlainTextChatHistory が即時反映されることを確認。

**手順**:
- occluder タグなしの環境で r12 配置 (stereo upmix / 5.1 native / mono) を回し、r12 と完全同一動作を確認
- r11 §5.5 受入条件全行を再実行 (lite-HRTF / venue reverb / wetgain / 9 venue 段階差)
- r10 §5.3 受入条件全行を再実行 (placement / dropout / 互換マトリクス / routing 診断)
- r9 / r8 §5 全項目をスポット回帰
- chat font: LL-style chat 切替後に Preferences → Chat → font size を変更 → 即時反映 / Preferences → Chat → Plain Text History toggle → 即時反映 (タッチ不要)

**完了条件**: r12 と挙動完全一致 / 既存配置のユーザは r13 viewer でも occlusion タグを建物に貼らない限り何も変わらず動作 / chat font live-apply が LL-style chat 上で即時反映される (commit 2689a35f8f が正しく merge)。

(in-conversation 実行、commit なし)

---

### P12: CPU benchmark + spec close-out

**目的**: spec §6.3 の CPU 受入条件 (r12 baseline 比 +2pp 未満) を実測値で埋め、spec §10 変更履歴に close-out を追記。occluder 数 / door 数 / フルチェイン (occlusion + venue + binaural + upmix) で測定。

**手順**:
- `top -b -d 10 -n 7` で 60 秒サンプリング (初回 0% は捨て 6 サンプル平均)
- PID は `pgrep -f 'do-not-directly-run-ayastorm-bin' | head -n1`
- 4 構成を順次計測:
  - r12 baseline (occluder ゼロ + r12 default = upmix + binaural + venue dry)
  - occlusion only (occluder ×100 + door ×3 + r12 default)
  - occlusion + venue=hall_medium
  - occlusion + venue=hall_medium + binaural ON + upmix ON (フルチェイン)
- 5min dropout 0 / URL 切替 ×10 を 4 構成すべてで実施
- door prim 連続回転 (5min × 60Hz) で geometry update が遅延なく追従するか確認
- prim rez/derez ×20 で geometry leak (polygon 数推移) なし
- 結果を spec §6 / §10 に追記

**完了条件**: spec §6.3 全項目記入完了 / +2pp 目標との実測差を spec に明記 / リリース判断材料そろう / close-out commit で `feature/aya-r13-obb-occlusion` を merge 可能状態にする。

(in-conversation 実行、spec doc 編集のみ。commit 化は AYA 判断)

---

## 3. マイルストーン依存関係

```
P0 ─→ P1 ─→ P2 ─→ P3 ─→ P4 ─→ P5 ─→ P6 ─→ P7 ─→ P8 ─┬─→ P10 ─→ P11 ─→ P12
                                                       │
                                              P9 ────→ │ (P10 開始前まで)
```

P0 (仕様 + roadmap doc 同時更新 + 実装箇所調査) は本書策定と同時に実施。P1 (mgr skeleton + FMOD geometry lifecycle) は最小限の lifecycle 確立のみ、P2 (parser) と P3 (OBB 抽出) は P1 完了後に並行可だが、デバッグ容易性のため P2 → P3 を順次実装推奨。P4 (material 表 + タグ override) は P3 完了後。P5 (静的 lifecycle) は P2/P3/P4 全完了後 (parser から OBB を addPolygon に渡す経路がそろう)。P6 (動的 door) は P5 完了後 (静的経路を流用)。P7 (cap) は P5/P6 後、P8 (debug settings) は P7 後。P9 (検証 scene 構築) は P0 以降ならいつでも並行可。P10 (検証実行) は P1〜P9 全完了後。P11 で回帰、P12 でクローズ。

r12 と異なり、本リリースでは **追加 phase が想定外に発生する可能性** が以下の点で残る:

- **R1 (mesh prim OBB ズレ)** が P10 検証で「斜め屋根 / アーチ」等の体感ミスマッチを起こす場合 → P12.x で形状特化近似 (NG2 解禁) を r13 内で部分対応する判断あり、または r13.x として後追い
- **R3 (material 表の preset 値)** が P10 検証で強すぎ/弱すぎとなる場合 → P12 close-out 時に default 値を 0.1 単位で再 tune
- **R7 (rapid teleport)** が SL 実 sim で発火する場合 → P12.x として teleport hook 追加 (+0.5 日)
- **P10 検証中に新 material や新タグ要望が判明** したケース → 仕様書改定 + P13 として後追い phase を追加 (r11 P7c-C / r12 P12 と同じ流儀)

---

## 4. 動作確認チェックリスト (P10 / P11 / P12 結果の P 単位 trace)

`[x]` 実機実測で通過 / `[~]` コードレビューのみ / `[ ]` 未実施 (リリース後の運用観察に委ねる場合あり)。

安定性指標の実測値 (CPU / dropout / URL 切替成功率) は **spec §6.3** を、debug settings の最終 default 値 (R3 縮退で調整した場合) は **spec §4.7** を、material 表の最終値 (R3/R6 縮退で調整した場合) は **spec §4.4** を参照。本セクションは「どの P でどの観点を埋めたか」の trace。

### 4.1 r13 新規 (P1〜P9 で実装、P10/P11/P12 で検証)

- [ ] **O1: scene A 室外→室内で direct sound の muffled→clear 変化** — P10 step 1
- [ ] **O2: scene A + venue=hall_medium で reverb も muffled→clear** — P10 step 2
- [ ] **O3: scene B 扉閉/開で listener 真正面の muffled→clear 切替** — P10 step 3
- [ ] **O4: scene B 扉開、listener 斜め前 30° で direct そこそこ抜ける** — P10 step 4
- [ ] **O5: scene B 扉開、建物真横 (壁越し直線) で muffled 維持** — P10 step 5
- [ ] **O6: material 別 (wood/glass/stone) で遮蔽強度差** — P10 step 6
- [ ] **O7: タグ override `[ayastorm:occlude:0.5]` が material 表より優先** — P10 step 7
- [ ] **O8: `Stream3DOcclusion = 0` で強制 OFF** — P10 step 8
- [ ] **O9: `Stream3DOcclusionDirectGain = 0.0` で direct 消失、reverb 維持** — P10 step 9
- [ ] **O10: `Stream3DOccluderMaxCount = 5` で cap 動作** — P10 step 10
- [ ] **O11: 範囲外 (64m 超) の occluder が登録されない** — P10 step 11
- [ ] **O12: door 移動で遮蔽位置が即追従** — P10 step 12
- [ ] **O13: 配信者 r11 venue タグと occlusion が独立動作** — P10 step 13
- [ ] **O14: `llPlaySound` (オブジェクト効果音) も occlusion 適用** — P10 step 14
- [ ] **5min dropout 0** (occlusion + venue=hall_medium + binaural ON + upmix ON、occluder ×100 + door ×3) — P12
- [ ] **URL 切替 ×10** (occlusion 環境下で stereo upmix ↔ 5.1 native ↔ mono の組合せ) — P12
- [ ] **prim rez/derez ×20 で geometry leak なし** (polygon 数推移確認) — P12
- [ ] **door 連続回転 5min × 60Hz で geometry update 遅延なし** — P12
- [ ] **CPU r12 比 +2pp 未満** — P12

### 4.2 r10 / r11 / r12 互換 (回帰確認、P11)

- [ ] r12 §6 受入条件全行が回帰なし (occluder タグなしの環境で完全互換)
- [ ] r11 §5.5 受入条件全行が回帰なし (同上)
- [ ] r10 §5.3 受入条件全行が回帰なし (同上)
- [ ] r9 / r8 互換 (既存配置のタグ無改修動作) スポット回帰
- [~] codec 別: Vorbis のみ end-to-end 実機回し、Opus / FLAC は r9 確立経路の流用でコードレビューのみ

### 4.3 chat font live-apply fix 同梱確認 (P11)

- [ ] LL-style chat で ChatFontSize 変更が即時反映 (commit 2689a35f8f) — P11
- [ ] LL-style chat で PlainTextChatHistory toggle が即時反映 (commit 2689a35f8f) — P11
- [ ] FS-style chat 経路に regression なし (chat 描画系の従来動作維持) — P11

---

## 5. 参照リンク

| 項目 | 参照先 |
|---|---|
| 設計判断 (旧 r13+ basket 降格 / OBB 単独で shipping / 役割分担導入) | `doc/spec_obb_occlusion.md` §1 / §2.2 / §2.3 |
| リスク R1〜R9 (内容 + 縮退策) | `doc/spec_obb_occlusion.md` §7 |
| 受入条件表 (O1〜O14 + 互換 + 安定性) | `doc/spec_obb_occlusion.md` §6 |
| OBB 抽出 / material 表 / lifecycle / 動的 door 詳細 | `doc/spec_obb_occlusion.md` §4.3 / §4.4 / §4.5 |
| debug settings 4 件 (sentinel + multiplier/cap 3 件) + range 設定 1 件 | `doc/spec_obb_occlusion.md` §4.7 |
| r11 venue / r12 upmix との直交性 | `doc/spec_obb_occlusion.md` §4.6 |
| r14+ への持ち越し | `doc/spec_obb_occlusion.md` §9 |
| r12 impl record (本書のテンプレート) | `docs/ayastorm-r12-stereo-upmix.md` |
| r12 spec (前提) | `doc/spec_stereo_upmix.md` |
| r11 spec (前提) | `doc/spec_binaural_venue_reverb.md` |
| r10 spec (前提) | `doc/spec_5_1ch_placement.md` |
| r9 spec (前提) | `doc/spec_5_1ch_source.md` |
| 検証 scene 構築 | `doc/r13/build_test_scene.md` (P9 で作成予定) |
| 検証材料生成 (audio) | `doc/r12/gen_upmix_test_material.sh` (流用) |
| roadmap | `docs/ayastorm-stream3d-roadmap.md` §3 r13 |
| chat font live-apply 同梱 | commit 2689a35f8f (`feature/ll-chat-livetune-font-plaintext` ブランチ) |
| 役割分担 (会場運営 vs 配信者の直交性) | memory `project_venue_occlusion_orthogonal.md` |
| r13 フラグシップ + 同梱 fix 方針 | memory `project_ayastorm_r13_obb_occlusion.md` |
