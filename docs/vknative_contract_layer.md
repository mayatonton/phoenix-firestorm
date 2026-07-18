# VK 実行時契約層(LLVKContract)— 描画失敗の自己検知装置

- 目的: **システムが自分の描画失敗を検知・命名・位置特定して自己申告する常設装置**(AYA 発注 2026-07-18)。「描画がおかしい」の一次検知器が人間の視覚である状態を終わらせる。
- 常時 ON(release 含む)。kill switch なし(診断であり挙動を変えないため)。
- 実装 v1 = 本 doc の §2。module = `indra/llrender/llvkcontract.{h,cpp}`。

## 0. 破れていた不変条件(装置の対象)

> 描画の失敗(draw の不発火・fallback 置換・束縛の振動)は、**機構が原因コードと出自(provenance)付きで自己申告**しなければならない。人間に「どれが失敗したか」を聞き返した時点で装置の敗北。

v1 以前の実態: skip/未描画の警告は「shader 名 + 通算回数」のみ(原因 6+ 種の判別不能・object 不明)、once-per-shader dedup で 2 回目以降消滅、fallback 置換(白描画)は無音、alpha run の全滅は完全無音。

## 1. 設計原則

1. **チョークポイント検査**: 全 draw が必ず通る発火点(`drawRange`/`drawRangeFast`/`drawArrays`/`pushIndirectSpans`/`flushAlphaRun`)と、per-call set 解決の全失敗経路に hook。列挙漏れが構造的に起きない。
2. **原因コード必須**: 失敗は必ず `ECause` を持つ。resolver 入口で thread-local cause をクリア(`resolveBegin`)→ 失敗サイトが `cause()` → 発火点の `drawSkipped()` が回収。「NULL でした」で終わる報告を機構的に不可能に。
3. **provenance 必須**: pool が draw 直前に `DrawScope(&params, "pool名")` を積む(thread-local・RAII)。失敗時のみ describer(newview 登録・`LLDrawInfo` → obj LocalID / texture UUID / avatar 名 / material UUID)で解決 = 正常 path のコストほぼゼロ。
4. **料金は失敗時払い**: hot path 追加コストは relaxed atomic increment(fired)+ TLS store 2 個(scope)のみ。map/mutex は skip 発生時のみ。
5. **層違反しない**: llrender は `LLDrawInfo` を知らない。newview(lldrawpool.cpp)が describe/key 関数を `setResolvers` で注入。

## 2. v1 実装(2026-07-18)

### 2.1 原因コード(ECause)
| cause | 発生源(file) |
|---|---|
| vk_not_init / record_job_pull / no_shader_or_layout / no_sampler / ubo_collect_overflow / ensure_set_fail | `populateAndBindUniversalDescriptorSet`(llglslshader.cpp)の全 return 経路 |
| refresh_no_shader / refresh_perprogram_ubo / refresh_shared_ubo | `vkRefreshDynamicOffsetsForDraw` の set NULL 化 3 経路 |
| authored_empty | resolver: authored=true なのに set NULL(authority バグ) |
| cmd_null / pipeline_null | 発火点の cmd / pipeline 不成立 |
| fb_view_diffuse / fb_view_aux | fallback view 置換(populate + buildAndOverride 両建て。diffuse=unit0 は白描画の実犯) |
| fb_heap_default | bindless heap slot が default slot に落ちた(bindless 白) |
| flicker | 同一 draw(key = LocalID+texture+count)の fire/skip が 3 frame 以内で振動 |

### 2.2 出力(log 側の作り変え)
- **VKC 行**(WARN・"VKContract" tag): 原因別 2 冪 escalation で `VKC skip cause=<原因> shader='..' n=<累計> pool=<pool> obj=<LocalID> tex=<UUID> av='<名前>'`。
- **VKC-SUM 行**: 10 秒窓の集計を**失敗があった時だけ**常時出力: `VKC-SUM 10s skips=N fired=M cause{ensure_set_fail=.. fb_view_diffuse=..}`。ゼロなら無音。
- 旧 ad-hoc 警告(drawRange 565/658 系・once-per-shader dedup・"atomic 11" 文言)は撤去済(VKC に置換)。

### 2.3 並列安全性
- cause/provenance = thread_local(lane 内で完結・共有状態を読まない)。
- counters = relaxed atomic。flicker 表 = mutex だが skip 時のみ + fire 側は 64K bitmap プレフィルタ(atomic load 1 回)通過時のみ。
- 世代検査(async publish 契約)は v2(§3)。

## 3. 段階計画(v2 以降・未実装)

- **v2 束縛内容契約**: authority が「宣言」(意図した texture/UBO 世代)を draw と一緒に運び、発火点で実 bind と突合 → fire-but-wrong を直接検出。scene authority の宣言構造体設計が必要。
- **v3 世代契約**: 資源(heap slot / mega-buffer slice / UBO ring)に publish 世代を持たせ、使用点で世代照合 → async worker の差し替え/寿命バグを機械検出。
- **v4 presence census**: 「前 frame に居た draw が消えた・消失イベント無し」検出(消えた髪の直接検出)。v1 の flicker 検出はその skip 版部分集合。

## 4. 網羅の境界(正直申告)

- **網羅**: draw 不発火(全発火点・全原因)・fallback 置換・skip/fire 振動。
- **対象外(v1)**: fire-but-wrong(発火したが誤った束縛)= v2 の領分 / 画素内容の意味的正しさ(golden 比較の別装置)/ data race の瞬間(TSan の領分・契約層は結果のみ捕まえる)。
