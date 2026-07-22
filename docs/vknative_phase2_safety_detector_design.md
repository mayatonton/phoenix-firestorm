# Phase 2 入場ゲート = 並列安全性検出器 設計

> **⚠️ 訂正(2026-07-22・skeleton off-main M3 の gate で実走判明)= Layer A(TSan・§3/§6 D3)は本コードベースで実行不能。** viewer は `boost::fibers`(フル fiber scheduler・llcoros.h:113)使用 → TSan が fiber context switch を追えず内部 `CHECK failed: thr->slot != 0` で自死(window init で死・world 未到達)。annotation(`__tsan_switch_to_fiber`)には boost.fiber scheduler 内部の全 switch 点への hook = 非現実的。**D3「✅ 構成済」は config のみで未走行だったため未発覚**(cmake `USE_TSAN` + suppressions は在るが、ビルドに `-Wno-error=tsan` 追加が必要 = commit `42d5fb9531`)。jemalloc は TSan と衝突ゆえ走行時 `~/ayastorm/lib/libjemalloc.so` 退避要。**代替 = Layer B(bespoke)+ jemalloc heap-corruption crash を正の race オラクル**(元 crash が race を検出・source 不問)。詳細 = memory `finding_tsan_layerA_blocked_boost_fiber`。実例 = skeleton M3 の lifecycle race は controller single-owner mutex + jemalloc オラクルで根治(TSan 抜き)。**Layer A 記述は AYA が入場ゲート再設計する際の訂正材料。**

**status**: 設計(AYA gate 待ち)。真の Phase 2 の入場ゲート(AYA 2026-07-21 Open)。**HEAD を file:line 実トレースして作成。**
**位置づけ**: E 系 closed + C(teardown 統一)決着後、分散化(Phase 2)を開始する**前提装置**。CLAUDE.md「安全性検出器が入場ゲート」の実体。doctrine =「装置が犯人を名指しするまで fix を書かない・伸ばすのは装置」。

---

## 0. なぜ検出器が先か(readiness で確定した論理)
- Phase 2 の本命 = **aChar(`LLVOAvatar::updateCharacter` @llvoavatar.cpp:5342)の per-avatar 並列**(2.05ms/f・avatar の 66%)。
- **updateCharacter 本体は ~100 行だが 2.05ms は深い呼出木**(LLMotionController/LLJoint/skeleton)にある = **触る共有 mutable の全面を手動監査しきれない**。MT-2 監査(memory `project_mt2_shared_state_audit`)が示した通り、並列化の危険は「散在 mutable への非同期アクセス」で、1 つ見落とせば 1 race。
- ∴ **手動 owner 割当だけに依存しない = 見落としを runtime で捕まえる装置**が要る。T 系で UAF を繰り返し踏んだ(lleventpoll UAF / T2 geometry UAF)= 並列は必ず穴を生む前提。

## 1. 脅威モデル(並列化が破るもの)
1. **data race**: 2 スレッドが同一 mutable を write/read 競合。
2. **owner 違反**: worker が「main 専有」state を write(gGLLastMatrix・per-call statics・deferred free queue・VK 資源生成 等 = MT-2 監査の「worker 禁止」群)。
3. **UAF**: dispatch 中に avatar/参照物が解体され worker が dead を触る。
4. **VK object の外部同期違反**: worker が DescriptorPool/queue 等を触る(spec 違反)。既に VVL Threading + `VkcRaceProbe`(mega/drawdata)が一部捕捉。

## 2. 既存資産(伸ばす種)
- **`VkcRaceProbe`**(llvkloader.cpp:232-253)= atomic owner に CAS(0→self)、失敗=2 スレッド同時侵入 → `LLVKContract::cause(c)` 発火。mega(`sVkcMegaOwner`)/ drawdata(`sVkcSlotOwner`/`sVkcScratchOwner`)で稼働中。**= scoped concurrent-entry 検出の完成品**。
- **`LLVKContract`**(llvkcontract.h)= alarm チャネル(cause/note/frameBegin)+ 既存 race cause(`C_MEGA_RACE`/`C_DRAWDATA_RACE`)。**検出器の出力先が既にある**。
- **`AYASTORM_MT_THREADS=1`** = 全 MT 直列化の既存 kill switch。並列 epoch を殺せる。

## 3. 設計 = 2 層検出器(cheap 常時 + exhaustive 発見)

### Layer A — TSan build flavor(自動・網羅・発見用)
- `-fsanitize=thread` の専用ビルド構成。**exercised path 上の data race / UAF を全部**、2 スレッド stack 付きで検出。**aChar の深い呼出木の未知 race はこれでしか自動発見できない**(手動監査の限界を埋める本命)。
- dev/CI 専用(5-15x 遅・release 非搭載)。**crowd stress で aChar 並列経路を走らせて ZERO** が発見側 gate。
- 設定コスト = suppression file(webrtc/driver スレッドの既知 benign を抑制)= 初期整備項目。

### Layer B — ownership guard(bespoke・常時・cheap・regression 用)
`VkcRaceProbe` を Phase 2 用に一般化。**MT-2 監査で owner を割り当てた既知 state を runtime で強制**(監査が「文書」から「機械強制」へ)。release でも稼働・violation は `LLVKContract` alarm へ。

**芯 = 並列 epoch 概念**:
```
tIsWorkerThread            // thread_local: この thread は並列 worker か
sParallelEpochActive       // atomic: aChar dispatch..join の間だけ true
```
epoch 非活性(通常時)= **全 guard は no-op = ゼロコスト**(doctrine「通常起動は無音」)。epoch 活性中のみ判定。

**guard primitive(3 種)**:
| primitive | 置く場所 | 発火条件 | 新 cause |
|---|---|---|---|
| `MainOnlyGuard` | main 専有 state の write 点(gGLLastMatrix・per-call statics・free queue enqueue 等) | epoch 活性中に **worker が** or **epoch 活性中に write が起きたら** | `C_PAR_MAIN_ONLY_WRITE` |
| `WorkerForbiddenGuard` | worker 禁止操作(VK 資源生成/destroy・queue submit・pool alloc) | `tIsWorkerThread` で到達 | `C_PAR_WORKER_FORBIDDEN` |
| `ConcurrentEntryGuard`(= VkcRaceProbe 一般化) | single-writer を要求する region | 2 スレッド同時侵入(CAS 失敗) | `C_PAR_CONCURRENT` |

**UAF guard**: epoch は dispatch する avatar を **LLPointer pin**(MT-2 の `sShadowRecordPins` と同型・join で解放)。`DeadObjectGuard(obj)` = worker が dead avatar を触れば発火(`C_PAR_DEAD_ACCESS`)。

## 4. gate 基準(fail-closed・憲法準拠)
Phase 2 の各並列段は、以下**全部の沈黙**でのみ受入(PASS は自称不可):
1. **TSan build で crowd stress → race/UAF ZERO**(発見側)。
2. **Layer B guard が通常 + stress で沈黙**(regression 側)。
3. 既存 gate = 視覚同一 + VVL 0 + VKC 全層沈黙。
4. 実効設定確認欄(`AYASTORM_MT_THREADS` 値・avatar 数)。
- 「装置が沈黙するのに症状」= 装置のギャップ → **伸ばすのは装置**(fix 仮説に行かない)。

## 5. kill switch / ゼロコスト
- 工事中 = `AYASTORM_MT_THREADS=1` で epoch 非活性 = 並列退化 + guard no-op。
- Layer B は epoch flag の 1 branch で gate = release でも実質ゼロ。gate PASS 後も **Layer B は恒久常設**(regression 網・VKC と同格の検出装置 = 撤去は AYA 承認要 = 憲法の検出器凍結条項)。TSan は build flavor ゆえ release に痕跡なし。

## 6. 実装段取り(検出器が先・aChar は後)
- **D1**: 並列 epoch 基盤(`tIsWorkerThread`/`sParallelEpochActive`/pin 機構)+ 新 cause 4 種を `LLVKContract` へ(検出器の凍結対象 = AYA 承認)。
- **D2**: guard primitive 3 種 + `DeadObjectGuard` 実装。MT-2 監査の「worker 禁止」「main 専有」既知点に設置(まず aChar が触る範囲)。
- **D3 ✅ 構成済**: TSan build flavor + suppression。
  - **cmake option `USE_TSAN`**(`indra/cmake/00-Common.cmake`・既定 OFF = 既存 build 不変)= ON で `-fsanitize=thread -fno-omit-frame-pointer -g`(compile+link・LINUX/DARWIN)。
  - **suppression = `indra/newview/tsan_suppressions.txt`**(TSan 版 allow-list = 憲法上 alarm_allowlist と同格・追加は AYA 承認)。初期 = `called_from_lib:libllwebrtc.so` のみ。
  - **走らせ方(AYA cold-launch)**: `USE_TSAN=ON` で別 build dir を config(例 `build-linux-x86_64-tsan`)→ ビルド → `TSAN_OPTIONS="suppressions=<repo>/indra/newview/tsan_suppressions.txt:halt_on_error=0"` で起動 → crowd で aChar 並列経路 stress → 残 report = 我々の race。⚠️ prebuilt lib(webrtc/driver/VMA/volk)は非計装 = そこ由来 noise は suppression で最小 allow(AYA 承認)。
- **D4(検出器の稼働確認)**: **わざと 1 つ違反を仕込んで全 primitive が名指しするか**(装置の自己検証 = AYA 起動 2 種の②)。これが通って初めて aChar 並列に着手可。
- 各 D: JIT 詳細設計(触る file・罠・申告)→ 承認 → 実装 → 突合。

## 7. 申告(縮小・省略・解釈)
- **解釈**: 「安全性検出器」を **2 層(TSan 発見 + bespoke 常設)** と解釈。TSan だけ(手動監査不要になるが release 非搭載・遅い)でも bespoke だけ(cheap だが監査漏れは捕まえられない)でも不足 = 両方必要と判断。AYA が「片方でよい」なら縮小可。
- **省略(今回の設計に含めない)**: net(idleNetwork)並列・LLVertexBuffer CPU 副本除去は Phase 2 の別段。本設計は aChar 並列の入場ゲートに限定(検出器自体は net 並列にも流用可)。
- **未決(D 着手時に確定)**: ①aChar 呼出木の「main 専有」既知点の全列挙(MT-2 監査は shadow record 対象 = aChar 用に再監査要)②TSan の webrtc/driver スレッド suppression の実際の量 ③pin する参照物の範囲(avatar 本体 + 参照 joint/motion の寿命)。
- **product 分岐なし**(内部装置)。ただし**新 cause の追加 = 検出器 diff = AYA 承認必須**(憲法 4 = 装置凍結)。
