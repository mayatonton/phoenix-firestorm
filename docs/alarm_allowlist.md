# alarm allow-list(fail-closed の唯一の逃がし穴・AYA 権限専用)

CLAUDE.md 冒頭「🔒 憲法」の default-deny gate が参照する唯一の allow-list。
**ここに明示されたシグナルだけが PASS を妨げない。それ以外の全 log alarm は自動でブロック(fail-closed)。**

## 運用ルール(不変)
- **追記できるのは AYA(人間)のみ。** agent はこのファイルに追記・編集してはならない(憲法 §4 = 凍結対象)。
- 1 エントリに **理由**必須。理由なき登録は無効。
- 登録は「バグを消した」ではなく「このシグナルは当面 PASS を妨げないと AYA が判断した」の記録。可能なら TICKET/期限を併記。
- alarm が根治したら該当行を削除(allow-list は最小を保つ)。

## 書式
```
| signal (grep パターン/一意キー) | 分類: ACCEPTED / KNOWN-BUG(#ref) | 理由 | 登録者 | 日付 |
```

## 登録済み

> r43 upstream 統合(feature/ayastorm-r43-upstream-sync)の validation 起動(`AYASTORM_VK_VALIDATION=1`)で観測された pre-existing alarm を AYA 裁定で登録(2026-07-24)。いずれも「r43 merge が触れていない VK-infra/skinning/RT/network 経路」で、merge 導入の新規 alarm ではないことを確認済(UBOReg/BindReg violation=0・merge-touched code の error/warning=0)。

| signal | 分類 | 理由 | 登録者 | 日付 |
|---|---|---|---|---|
| `abort: fail harder`(lltexturefetch.cpp doWork) | ACCEPTED | upstream 由来の texture-fetch noise・非自 bug(既 disposition) | AYA | 2026-07-24 |
| `#VKContract#` の `cause{...fb_view_diffuse` / `fb_view_aux` / `fb_heap_default...}` | ACCEPTED | RT texture-sample の benign note(失敗 verdict でない・既 disposition) | AYA | 2026-07-24 |
| `#GLTF# ... Error getting material asset data: Asset request: failed (-1)`(llgltfmateriallist.cpp) | ACCEPTED | material asset の fetch miss(network/content・code 起因でない) | AYA | 2026-07-24 |
| `LLSDXMLParser::Impl::parse: XML_STATUS_ERROR parsing:Not [Ff]ound`(llsdserialize_xml.cpp) | ACCEPTED | asset 応答 "Not Found" を LLSD parse した miss(network・自 xml 起因でない) | AYA | 2026-07-24 |
| `#VKNaN# ... non-finite skin palette -> sanitized to identity`(llviewershadermgr.cpp writeObjectSkinUBO) | KNOWN-BUG(Class 2 joint torn-read) | off-main skeleton の joint 行列 torn-read(upstream 由来・[[handoff_skeleton_offmain_race_class1_class2_resolved]])。sanitizer が identity 是正で視覚正常。r43 は skin 経路未 touch。⚠️従来 closed 状態は VKNaN=0 だったが active/crowd scene で顕在化(200/走行・単一 avatar joints=28)= 根治でなく緩和で受容 | AYA | 2026-07-24 |
| `UNASSIGNED-Threading-MultipleThreads-Read` / `-Write`(VK_OBJECT_TYPE_FENCE の vkGetFenceStatus/vkResetFences) | ACCEPTED | 我々の VK fence 同期設計(main+worker が fence status を並行 poll)を validation layer が指摘。設計上の並行 access で joint/merge とは無関係 | AYA | 2026-07-24 |
| `UNASSIGNED-CoreValidation-DrawState-InvalidImageLayout` | ACCEPTED | VK infra の RT/texture image layout 遷移(validation-layer のみ・視覚正常)・merge 未 touch | AYA | 2026-07-24 |
| `VUID-vkAcquireNextImageKHR-surface-07783` | ACCEPTED | swapchain acquire の forward-progress 警告(present-mode/FRAMES_IN_FLIGHT 由来)・merge 未 touch | AYA | 2026-07-24 |
| `VUID-vkCmdDrawIndexedIndirect-renderpass` | ACCEPTED | shutdown/teardown 時の draw(saveSnapshot→pool closing 近傍)・clean 終了経路・merge 未 touch | AYA | 2026-07-24 |
