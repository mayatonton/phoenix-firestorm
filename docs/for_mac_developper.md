# AYAstorm r42 — macOS 開発者向け handoff

Date: 2026-07-15 / Updated: 2026-07-16 / PR merge target: `dev/ayastorm-vk-premt@e7747fb267` / 実装source branch: `feat/macos-moltenvk-bootstrap@444612c9d0` / PR統合 branch: `feat/macos-moltenvk-premt`
原文: AYAstorm 設計チーム(Linux 側)。2026-07-15 のmacOS静的監査ではsource/configを実読し、build/runは **OPEN** だった。2026-07-16の実装・実機snapshotは§3.3を正とする。

実際の変更、build、実機gateの順序と停止条件は [`docs/specs/ayastorm-r42-macos-moltenvk-implementation-plan.md`](specs/ayastorm-r42-macos-moltenvk-implementation-plan.md) を実行計画の正本とする。このhandoffと実装計画が食い違う場合もHEADのコードを正とし、両文書を更新する。

macOSの製品対象architectureは **Apple Silicon arm64のみ** とする。Universal viewer、Rosetta上のx86_64 viewer、Intel Macはbuild/runtime/distribution gateの対象外であり、未実施でもmacOS arm64版の停止条件にしない。main executableはarm64-onlyとする。arm64 sliceを含むUniversal dependencyの余分なx86_64 sliceは除去要件にしないが、x86_64-only helper/libraryは機能影響とRosetta依存を明示してAYA判断を得る。共有コード変更に対するLinux/Windowsの回帰確認は別のOS gateとして維持する。

---

## 0. 最重要原則(この文書自体にも適用される)

1. **コードが唯一の真実**。この文書を含むあらゆる文書・コメント・過去の記録は「書かれた日の snapshot」であり、HEAD のソースと食い違ったらソースが正しい。判断は必ず実コードの file:line トレースで行うこと。
2. 報告は 3 分類で行う: **VERIFIED**(自分で実行/実読して確認した・file:line や実行ログを添える)/ **CLAIMED**(伝聞・文書由来・未検証)/ **OPEN**(未着手・不明)。「できているはず」を VERIFIED と書かない。
3. **機械的一括置換の禁止**。複数ファイルを script/regex で一括編集しない。編集前に対象ファイルの該当箇所を必ず読む。
4. 完了・done の宣言をしない。上記 3 分類で事実だけ報告する。

## 1. AYAstorm r42 とは(30 秒)

- Firestorm viewer の fork。**r42 の描画 backend は Vulkan 専用**。Linux では実 GL 呼びゼロ・実機動作検証済み(VERIFIED on Linux)。macOS では **MoltenVK**(Vulkan→Metal 変換層)経由で動かす設計だが、実機 gate 前の移行 bootstrap として NSOpenGLView/CGL を温存している。したがって「macOS でも OpenGL を完全削除済み」はまだ **OPEN**。
- 以後 OpenGL をメンテする計画はない。upstream(Firestorm)からの機能取り込みは継続する。
- 版体裁: 版番号 = **42.0.0**(実体 = `indra/newview/VIEWER_VERSION_AYA.txt`)/ channel = **`AYAstorm-release`** / based on Firestorm 7.2.4・SL 26.1.1。版の表示を新設するときは XML 直書き禁止・`LLVersionInfo` 経由。

## 2. あなた(macOS 担当)の任務

**2026-07-16時点でmac向けMoltenVK groundwork、Loader経由のdevice/surface作成、Retina 1x/2xの表示・左click・resize収束、ログインから正常終了までは旧baseのarm64 executableでVERIFIED。`dev/ayastorm-vk-premt`直上へのmacOS責任範囲の移植、arm64 full viewer build、app bundle staging、Loader/MoltenVK/ICD検査もVERIFIED。一方、移植後binaryの実機run、validation、pink flashの生成元、正式配布packageはOPENである。** 残る作業は以下の3段:

1. **Vulkan SDK / MoltenVK の固定と packaging**: local package、Darwin限定CMake入力、`libvulkan.dylib`・MoltenVK・ICD JSONのbundle配置、portability対応までは実装・arm64確認済み。正式package公開、tracked `autobuild.xml`、license、最終配布署名はOPEN。
2. **実機で presentation を検証**: 旧baseではRetina 1x/2xのログイン前表示、modal button hit、resize収束、ログイン、world接続、正常終了まで確認済み。移植後binaryでは製品相当run → fragment output / sampler上限の診断validation run → 2xで10回以上の連続resize → pink flashのframe trace → 左下チャットウィンドウの文字確認の順で進める。
3. **検証後に NSOpenGLView/CGL を退役**: 現状 CAMetalLayer は contentView の **sublayer 方式で NSOpenGLView と共存**している(意図的な移行設計)。VK 描画が実機で安定したら、Linux で実施済みの「GL context 生成退役」(r42 Phase1 B-② パターン、`indra/llwindow/llwindowsdl2.cpp` の履歴が参考例)を mac にも適用する。

## 3. macOS 側の現状(2026-07-16 実装・実機snapshot)

| 項目 | 状態 | 根拠(file:line は書時点) |
|---|---|---|
| Metal surface 生成 | **VERIFIED / IN_PROGRESS** | `vkCreateMetalSurfaceEXT` = `indra/llrender/llvkloader.cpp:8266-8271`。旧baseのarm64実機でsurface/swapchainからログイン画面まで到達。移植後runtimeはOPEN |
| CAMetalLayer 生成・resize 追従 | **VERIFIED / IN_PROGRESS** | `indra/llwindow/llwindowmacosx-objc.mm:237-276`、`indra/llwindow/llwindowmacosx.cpp:414-432`。旧baseの`RenderHiDPI=0/1`両runで収束。移植後runtimeはOPEN |
| native handles 配線(window→VK) | 実装・実読**VERIFIED** | `indra/llwindow/llwindowmacosx.cpp:2533-2551` |
| portability enumeration/subset | **VERIFIED / IN_PROGRESS** | instance = `indra/llrender/llvkloader.cpp:510-681`、device = `同:828-1104`。移植後compileはVERIFIED、runtime request/enableはOPEN |
| Loader / MoltenVK packaging | **VERIFIED / IN_PROGRESS** | `indra/cmake/Vulkan.cmake:22-46`、`indra/newview/viewer_manifest.py:1600-1601,1688-1713`。app stagingとbundle検査はVERIFIED。正式package URLとlicenseはOPEN |
| NSOpenGLView/CGL | **温存中(意図的)** | VK 実機検証まで退役しない設計 |
| mac arm64でのビルド | **VERIFIED / IN_PROGRESS** | `dev/ayastorm-vk-premt`直上でRelease `ayastorm-bin` full buildとapp stagingがexit 0。build log SHA-256=`caca76ae14a29b3f056d4f1bfadbef5492c5bfcbf60abf0dfd4a2ea2b4846fa7`。main executableはarm64-only、bundleはdeep/strict署名検証合格。`llpackage` / DMGはOPEN |
| 実機動作 | **VERIFIED / IN_PROGRESS** | bundle内Loader/MoltenVKで1x/2xとも全画面のログイン画面へ到達し、Web確認modalを左clickで閉じ、resize後も収束。同一arm64 executableの後続runはlogin success、initial simulator、movement complete、WebRTC terminate、Vulkan shutdown、`Goodbye!` / `status: stopped`へ到達。validation layer無効runのためvalidation gate合格には使わない |
| pipeline cache復旧 | **VERIFIED on old base / EXCLUDED** | 旧baseでは保存blob拒否時の隔離とempty cache retryを実機確認した。現在のPR target `indra/llrender/llvkloader.cpp:1638-1657`にはその処理を移植しておらず、本PRのscope外 |

- `docs/build/building_ayastorm_macos.md`は2026-07-16にVulkan/MoltenVK用へ更新し、arm64-only configure/build、local `vulkan_sdk_macos`登録、Loader/MoltenVK/ICDのbundle検査、overrideなし製品run、validation runを追加した。ただし正式package公開とtracked `autobuild.xml`反映はOPENで、clean checkout単独のrelease手順としては未合格。configureのchannelは`AYAstorm-release`を使い、食い違ったらコードとCMakeを正とする。
- この viewer は `VkApplicationInfo::apiVersion` と physical-device gate の両方で **Vulkan 1.3 以上を要求**する(`indra/llrender/llvkloader.cpp:626,728-732`)。MoltenVK は「Vulkan 1.2 相当」と決め打ちせず、Vulkan 1.3 以上を公開する release を固定する。viewer が使う機能で未対応のものが出た場合は、対処方針を独断で決めず報告すること(Linux 側の実装を変える判断があり得る)。
- Vulkan Loader + ICD 方式に必要な`VK_KHR_portability_enumeration`、`VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`、公開時だけ有効にする`VK_KHR_portability_subset`はMetal guard内へ実装済み。Windows/Linuxのextension listは変更しない設計だが、両OSのbuild/runtime gateはOPEN。
- portability の一次資料: [MoltenVK README](https://github.com/KhronosGroup/MoltenVK/blob/main/README.md)、[VK_KHR_portability_enumeration](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_enumeration.html)。

### 3.1 portability / packaging の具体的な実装点

以下は現在の実装点と残るOPEN。file:lineは2026-07-16の作業tree snapshotであり、HEADを必ず再読する。

1. **instance portability — `indra/llrender/llvkloader.cpp:createInstance()` (`510-681`)**
   - `VK_USE_PLATFORM_METAL_EXT`内でvalidationとは独立してextensionを列挙し、Loaderが公開した場合だけextensionとinstance flagを有効化する(`525-632`)。
   - `vkCreateInstance()`失敗時は`VkResult`とportability request状態を記録する(`665-681`)。
2. **device portability — `indra/llrender/llvkloader.cpp:createDevice()` (`828-1104`)**
   - Metal guard内の局所定数`"VK_KHR_portability_subset"`を使い、選択deviceが公開した場合だけdevice extensionへ追加する(`923-1077`)。required subset featureも同じfeature chainでquery/enableする。
   - `vkCreateDevice()`失敗時と成功時にsubset状態を記録する(`1089-1104`)。
3. **初期化 stage log — `indra/llrender/llvkloader.cpp:initVulkan()` (`2955-2982`)**
   - `selectPhysicalDevice() || selectQueueFamily() || createDevice()`は現在も連結判定(`2979-2982`)で、失敗stageの個別logは **OPEN**。
   - `vkEnumeratePhysicalDevices()` が 0 件の場合は、Loader path・ICD discovery・portability flag を確認対象として log に明示する。
   - `createInstance()`失敗時、および完全初期化前に`shutdownVulkan()`へ入る経路の`volkFinalize()`保証は **OPEN**。
4. **bundle packaging — macOS 配布系**
   - `indra/cmake/Vulkan.cmake:22-40`と`Glslang.cmake:30-39`はDarwinだけlocal packageを使い、他OS分岐を維持する。
   - `indra/newview/viewer_manifest.py:1600-1601`でLoader/MoltenVK、`1688-1713`でICD JSONを配置し、`library_path`だけをbundle相対pathへ書き換える。
   - `indra/newview/CMakeLists.txt:2869-2878`: 既存 `@executable_path/../Frameworks` runpath が採用した配置と一致するか確認し、必要な場合だけ変更する。
   - `indra/newview/licenses-mac.txt` に、固定した MoltenVK / Vulkan Loader 配布物のライセンス表記を追加する。
   - package後に `file` / `lipo -archs` / `otool -L` / `codesign --verify --deep --strict --verbose=2` で、全Mach-Oがarm64であること、参照先、署名を確認する。
5. **build 文書 — `docs/build/building_ayastorm_macos.md:157-185`**
   - configure 例と期待値を `AYAstorm-release`、固定した SDK/MoltenVK、packaging手順に更新する。ただし実装・実機 gateが固まる前に「VERIFIED手順」として書かない。

`initSurface()` の Metal branch(`indra/llrender/llvkloader.cpp:8216-8283`)は CAMetalLayer を受け取る。instance/device列挙とLoader/ICD packagingはbuild/stagingまで確認済みで、surfaceの移植後実機結果はOPEN。

### 3.2 Windows/Linux への影響境界(**実読 VERIFIED / build OPEN**)

`llvkloader.cpp` と `Vulkan.cmake` は 3 OS 共有なので、portability を「macOSでだけ有効にする」だけでは不十分。**他 OS ではコンパイル対象にも実行経路にも入れない境界**を次のように固定する。

| 変更箇所 | Windows/Linux で意図する結果 | 必須の分離 |
|---|---|---|
| instance extension / flag | extension list・`VkInstanceCreateInfo::flags`とも変更なし | `VK_USE_PLATFORM_METAL_EXT` 内で公開確認・追加・flag設定を完結 |
| device portability subset | device extension list変更なし | Metal guard内かつ、選択deviceが公開した場合だけ追加。beta定数を共有コードへ無条件に出さない |
| `Vulkan.cmake` | 既存SDK/header探索を維持 | macOS用header取得・固定versionだけを `if (DARWIN)` に限定 |
| bundle / runpath / license | package内容変更なし | 歴史的なclass名である`Darwin_x86_64_Manifest`と既存`if (DARWIN)`節だけを編集。class名はarm64出力を意味しない |
| 初期化stage log | 必要なら3 OS共通で診断改善 | 呼出順・short-circuit・失敗時cleanupを変えず、log追加だけにする |

- platform define は Windows=`VK_USE_PLATFORM_WIN32_KHR`、Linux=`VK_USE_PLATFORM_XLIB_KHR`、macOS=`VK_USE_PLATFORM_METAL_EXT`(`indra/cmake/00-Common.cmake:88-97,190-191,234-238`)。
- portability enumeration/flagを共有経路で無条件に有効化すると、非対応Loaderではinstance作成失敗、portability ICDが存在する環境では列挙device増加の可能性がある。現device選択はtype score中心(`indra/llrender/llvkloader.cpp:708-752`)なので、他OSで候補を増やさない。
- macOS packagingの編集先は、歴史的な名称を維持している`Darwin_x86_64_Manifest`(`indra/newview/viewer_manifest.py:1234-1236`)および`if (DARWIN)`(`indra/newview/CMakeLists.txt:2839`以降)に限定する。成果物architectureは別途`ARCHS=arm64`と`file`/`lipo -archs`で固定・検証する。
- 一次資料: [Khronos Vulkan `vulkan.h`](https://github.com/KhronosGroup/Vulkan-Headers/blob/main/include/vulkan/vulkan.h)、[`vulkan_beta.h`](https://github.com/KhronosGroup/Vulkan-Headers/blob/main/include/vulkan/vulkan_beta.h)。
- Windows/Linux の configure・compile・起動は未実施なので **OPEN**。少なくとも両OSでbuildし、従来deviceが選択され、有効化extension一覧が変わっていないことをgateにする。

### 3.3 2026-07-16 実装・実機snapshot

この節は2026-07-15の静的監査後に実行した範囲だけを追記する。phase全体の完了を意味しない。

| 項目 | 状態 | 根拠 |
|---|---|---|
| local SDK/package | **VERIFIED / IN_PROGRESS** | Vulkan SDK 1.4.350.1とcloneからbuildしたMoltenVK 1.4.2をdarwin64 packageへ固定。正式package公開とtracked `autobuild.xml`はOPEN |
| macOS dependency | **VERIFIED / IN_PROGRESS** | `indra/cmake/Vulkan.cmake:22-40`と`Glslang.cmake:30-39`でDarwinだけlocal packageを使用。他OS分岐は維持 |
| portability | **VERIFIED / IN_PROGRESS** | `llvkloader.cpp:510-681,828-1104`でMetal限定実装。移植後compileはVERIFIED、実機request/enableはOPEN |
| packaging | **VERIFIED / IN_PROGRESS** | `viewer_manifest.py:1600-1601,1688-1713`でLoader、MoltenVK、ICDをbundle化。移植後arm64 appのdeep/strict署名を確認。移植後起動とlicense追加はOPEN |
| arm64 build/startup | **VERIFIED / IN_PROGRESS** | `dev/ayastorm-vk-premt`直上の最新sourceでarm64 full viewer build、app staging、deep/strict署名検証まで成功。移植後binaryの起動・ログイン・終了はOPEN |
| validation | **VERIFIED / IN_PROGRESS** | portability swizzle/geometry VUIDは修正後runで0件、live `LLVertexBuffer` shutdown sweep後は`VUID-vkDestroyDevice-device-05137`も再出現しなかった。一方、ログイン後runでMoltenVK `invalid return type 'main0_out'` / `color attribute must be made explicit`を24件採取したため、validation/MVK ERROR 0件gateは未通過 |
| bundle architecture | **VERIFIED / IN_PROGRESS** | main executableはarm64-only。Loader/MoltenVKはarm64 sliceを含む。既存のVivox 3点とVLC SIMD plugin 6点はx86_64-onlyのため、完全native arm64配布での扱いはOPEN |

現在の起動ログはSDK不足を示していない。Retina drawable/swapchain/input、portability swizzle/geometry、`LLVertexBuffer` shutdown sweep、pipeline cache自己復旧は実機で個別に前進を確認した。再開時のP0はfragment output arrayをMoltenVKが受理できる明示location出力へ正規化し、cacheを再生成してログイン後描画を再検証すること。共有コードを修正するときはLinux/Windows gateをmerge-blocking OPENとして明示する。

### 3.4 2026-07-16 Retina / input P0 — failureと修正後snapshot

- **VERIFIED (修正前):** 5120x2830 pixelの実機スクリーンショットで、描画内容は概ね左下2560 pixel幅に留まった。resize後は`LLAppViewer::doFrame()`の`beginFrame return false`がcount=100まで増え続けた。
- **VERIFIED (source):** `RenderHiDPI`に応じてCocoa event/resizeと`CAMetalLayer.drawableSize`を同じ1xまたは2x単位へ揃える(`llopenglview-objc.mm:37-45,149-184,358-420`、`llwindowmacosx-objc.mm:237-276`)。macOSのviewer resizeはUI単位のまま扱い、Vulkanへはdrawable専用通知を送る(`llwindowmacosx.cpp:414-432,2533-2551`、`llvkloader.cpp:8619-8628`)。
- **VERIFIED (source):** swapchainはfixed extentまたは実測drawableを選択し、作成成功時にpending resizeを消費する(`llvkloader.cpp:2665-2851`)。
- **VERIFIED (runtime 1x):** `RenderHiDPI=0`で初期swapchainは2560x1387。Web確認modalを左clickで閉じ、10回の連続resizeで2302x1245 / 2179x1245へ各回一致した。`beginFrame return false`は各resize eventにつき1回だけ増え、操作停止後は増加しなかった。表示は全windowへ収束し、終了は`Goodbye!` / `status: stopped`へ到達した。
- **VERIFIED (runtime 2x):** `RenderHiDPI=1`で初期swapchainは4358x2490、MoltenVKのcontents scaleは2.0。Web確認modalを同じscreen位置の左clickで閉じ、1回のresize後はrequested/current/selectedが4096x2490で一致し、全window表示へ収束した。終了は`Goodbye!` / `status: stopped`へ到達し、このrunのlogにVUIDはなかった。
- **VERIFIED / OPEN:** 診断runの1回で保存済み状態から自動ログインし、認証成功、initial simulator、movement completeまで到達したが、同runでMoltenVK fragment shader生成ERRORが発生したためvalidation合格証拠にはしない。後続の製品相当runは同一UUIDで`STATE_LOGIN_WAIT`、`Attempting login`、`handleLoginSuccess`、initial simulator、movement complete、`Terminating WebRTC`、Vulkan shutdown、`Goodbye!`、`status: stopped`へ到達した。このrunには`[VK-ERROR]`、VUID、`mvk-error`、`main0_out`はなかったがvalidation layerを要求していないため、validation/MVK ERROR 0件gateはOPENのまま。
- **VERIFIED / OPEN:** 後続runは`createVkPipeline()`で`samplers=17 / sampler_limit=16`を8件記録し、該当pipelineを作成しなかった。ログインと正常終了は確認できても、Water/PBR Terrain/PBR Alpha/Deferred Softenの描画完全性はOPEN。macOSだけの解決で閉じられない場合は共有renderer判断としてAYAへ返す。
- **VERIFIED / OPEN:** 起動直後に全windowが一時的にmagentaとなるframeを旧base runで画像捕捉した。swapchain初回clearは黒(`llvkloader.cpp:4668-4672`)。world deferred pathにはmagenta clearがある(`llviewerdisplay.cpp:1005-1015,1326-1335`)が、ログイン前経路へ結び付けられていないため生成元はOPEN。

約1/2寸法表示、modal buttonのhit-test、resize再作成loopはログイン前P0の停止条件から外す。ただしPhase 6A全体はOPEN項目が残るため合格扱いにしない。magentaを黒へ変えて症状を隠さず、NSOpenGLView/CGL退役も引き続き着手禁止とする。

### 3.5 2026-07-16 `CODESIGNING Code 2 Invalid Page`の切り分け

- **VERIFIED:** 利用者添付のcrash reportはprocess launch直後、thread 0のdyld内で`SIGKILL (Code Signature Invalid)`、`Namespace CODESIGNING, Code 2 Invalid Page`により終了している。viewerの終了処理やVulkan shutdownへ到達したcrashではない。
- **VERIFIED:** 問題の診断runはbuild treeのlibrary directoryを広く`DYLD_LIBRARY_PATH`へ設定していた。そこにある`libfmod.dylib`はarm64 sliceで`codesign --verify --strict`が`invalid signature (code or signature have been modified)`になる一方、app bundle内の`Contents/Resources/libfmod.dylib`とapp全体はstrict/deep検証に合格する。main executableは`@executable_path/../Resources/libfmod.dylib`を要求するが、`DYLD_LIBRARY_PATH`が同名の外部dylibを先に解決した。
- **VERIFIED:** `--noaudio`でもdyldによるlinked dylibのloadは`main()`より先なので、この署名killは回避できない。後続の製品相当runは`DYLD_LIBRARY_PATH`をunsetして同じbundleを正常起動・終了できた。
- **運用規則:** 製品gateとvalidation gateの両方で`DYLD_LIBRARY_PATH` / `DYLD_FRAMEWORK_PATH`をunsetする。validation layerの場所を指定する必要がある場合は`VK_LAYER_PATH`だけを使い、起動前にappとbundle内dylibを`codesign --verify --deep --strict`で検査する。この事象を「AYAstorm終了時のVulkan error」と分類しない。

### 3.6 2026-07-16 一時中断snapshotと再開点

**VERIFIED**

- 旧baseではpipeline cache復旧とlive Vulkan buffer sweepを実機確認したが、現在のPR targetはそれらを含まない。現targetの`createPipelineCache()`は`llvkloader.cpp:1638-1657`、`LLVertexBuffer::cleanupClass()`は`llvertexbuffer.cpp:959-965`であり、共有Vulkan挙動を変えるため本PRから除外した。
- 旧baseの再開用appはmain executableがarm64-onlyで、ad-hocのdeep/strict署名検証に合格し、製品相当runでログイン画面を表示した。
- 11:40のrunはapp logに`[VK-ERROR]`、VUID、`mvk-error`、shader compile failure、login attemptを記録しないままshutdown時に別incidentの`SIGSEGV`となった。同じbinaryの後続runはlogin後に`Terminating WebRTC`、Vulkan shutdown、`Goodbye!`、`status: stopped`へ到達し、残留processもなかった。shutdown crashは再現性OPENであり、後続成功だけで根因解消とは扱わない。

**OPEN / 再開順**

1. `layout(location = 0) out vec4 frag_data[4];`を使うfragment shader群は、MoltenVK生成MSLで`frag_data_1..3`のcolor attributeが明示されず、ログイン後に`invalid return type 'main0_out'`となる。共通境界は`indra/llrender/llglslshader.cpp:vulkanizeStageSource()` (`709-920`)。fragment stage、対象宣言1件、定数index 0..3だけに限定し、4本のscalar output/location 0..3へ正規化してSPIR-V reflectionと実機logを検査する。中断時点では未実装。
2. 上記修正後にarm64 `llrender`、viewer、manifestを再buildする。生成済みXcode projectのmanifestが`llbase`を解決できない場合はrepo venvのsite-packagesをbuild environmentへ明示し、system Pythonへ場当たり的にinstallしない。
3. `dev/ayastorm-vk-premt@e7747fb267`へmacOS責任範囲だけを移植し、破損cacheの再隔離、ログイン画面、ログイン、左下チャット文字描画、validation/MVK ERROR 0件を同じbinaryで確認する。
4. pink frameの生成元をframe traceで確定する。clear色だけを黒へ変えて隠さない。
5. 旧baseの1 runは終了時に`EXC_BAD_ACCESS (SIGSEGV)`となった。faulting threadは`com.apple.audio.IOThread.client`で、先頭frameは`LLWebRTCLogSink::OnLogMessage()`。後続runは同じbinaryで正常終了したため、根因と再現条件はOPEN。
6. **OPEN root-cause boundary:** `LLVoiceClient::terminate()`はWebRTC terminate呼出し前も`LLVivoxVoiceClient::instanceExists()`を判定している(`indra/newview/llvoiceclient.cpp:306-315`)。当該runのlogに`Terminating WebRTC`はなく、`LLWebRTCVoiceClient`のdestructorは空、`cleanupSingleton()`も`llwebrtc::terminate()`を呼ばない(`llvoicewebrtc.cpp:248-272`)。音声thread停止前にlog sink/callback lifetimeが切れた可能性を、再現、ASan/thread trace、正しいinstance条件、`RemoveLogToStream()`とaudio device停止順で検証する。推定だけでparcel managerを原因扱いしない。
7. `recreateSwapchain()`の作成失敗時に旧swapchainを保持できるか、0x0/minimize、restore、fullscreen、display移動を確認する。共有修正ならLinux/Windows gateをmerge-blockingとする。

### 3.7 `dev/ayastorm-vk-premt`向けPR準備snapshot

**VERIFIED**

- PR merge targetは`dev/ayastorm-vk-premt@e7747fb267`。実装source `feat/macos-moltenvk-bootstrap@444612c9d0`とのmerge-baseは`e39389fe7d`で、target側には14件のVulkan変更がある。
- `e7747fb267`直上のPR統合branch `feat/macos-moltenvk-premt`へ、Darwin限定SDK/glslang、arm64既定値、Metal portability enumeration/subset、MoltenVK swizzle、Darwin manifest、Cocoa/CAMetalLayer/Retina、macOS文書を移植した。
- Release `ayastorm-bin`のarm64 full buildとapp stagingは成功。main executable SHA-256=`58840c6f509ff7dc0b3bbf10e937802f27d3f854272140dd30b0cc03ba8b6413`、Loader symlink、MoltenVK、ICD相対path、deep/strict署名を確認した。

**OPEN / merge-blocking**

- depth format、descriptor/geometry capability、image/vertex shutdown sweep、startup fail-closed、pipeline cache recoveryは現状のままでは共有Vulkan挙動を変える。Darwin runtime guardへ閉じるか、Linux/Windows gate付きの別commit/PRとしてAYA判断を得る。
- `dev/ayastorm-vk-premt`上のログイン画面、ログイン、normal shutdown、validation、描画確認は未実施。旧baseの実機証拠を移植後binaryの証拠として流用しない。
- current projectは`PACKAGE=OFF`で、`llpackage` / DMG / notarizationは未実施。正式artifactとtracked `autobuild.xml`、license stagingもmerge-blocking OPEN。

## 4. 既知の罠(Linux 側で実際に踏んだもの)

- **識別子に `Status` を使わない**。Linux の Xlib が `#define Status int` を漏らすため、共有コードに `Status` という型/変数名を入れると Linux ビルドが壊れる。3 OS 共有コードを編集するときは必ず順守。
- **新しい cvar(設定変数)を追加したら** `settings.xml` に登録し、persistent なものは `Comment` 必須(欠けると "Missing Files" crash)。
- **shader(`indra/newview/app_settings/shaders/`)**: `#ifdef LL_VULKAN_GLSL` 分岐が Vulkan 用。変更したら実行環境への配布を忘れない。
- **「GL だと問題ないが VK で発火する」族が主要な不具合パターン**。既知 3 族と規約:
  1. UBO 残留 — 共有/ring UBO は生成時ゼロ初期化されない。**write-before-read が規約**。
  2. push constant — offset 重複帯(64-128)があり、**毎 pass/毎 draw の再 push が規約**。set-once 禁止。
  3. sampler 状態 — texture の filter/address は**必ず正規 `LLTexUnit::bind()` 経由**。手動の状態差し替えは残留バグを作る(2026-07-14 に実例修正済)。
- レンダリングの不具合調査では cvar 切替を証拠にしない。**消費直前にコードで値を固定した diff でビルドして** A/B すること。
- MoltenVK 固有: 検証時は `AYASTORM_VK_VALIDATION=1` で viewer 側の validation 要求を有効化し、`MVK_CONFIG_LOG_LEVEL` 等の MoltenVK ログと Vulkan validation layer の両方を見ること(変換層起因か viewer 起因かの切り分けが最初の分岐)。使用する変数値は固定した MoltenVK release の文書に合わせ、実行ログに残す。
- macOS起動gateではbuild treeを指す広い`DYLD_LIBRARY_PATH`を設定しない。署名が壊れた同名dylibをbundleより先にloadし、viewerコードへ入る前に`CODESIGNING Code 2 Invalid Page`でkillされ得る。
- 生成済みXcode projectがHomebrew Pythonを使い、`llsd` / `llbase`を見つけられない場合がある。今回のarm64 full buildはrepo venvのPython 3.9 site-packagesを`PYTHONPATH`へ指定して通過した。恒久対応はconfigure時のPythonをrepo venvへ固定することであり、system Pythonへの個別installを前提にしない。

## 5. 開発の進め方(推奨)

1. `docs/build/building_ayastorm_macos.md` は Xcode・autobuild・DMG 作成の基礎として参照するが、そのまま実行しない。configure 前に full Xcode、Vulkan SDK(header / loader / validation layer)、Vulkan 1.3 以上を公開する MoltenVK release を準備し、version と入手元を記録する。configure の channel は `AYAstorm-release` を使う。
2. source を修正する前に baseline build を一度実行し、完全な configure/build log を保存する。「全エラー」を保証するのではなく、最初に現れる根因群を「環境・configure / compile・link / Loader・ICD / mac 固有(llwindowmacosx 系) / 共有コード / packaging・signing」に分類して報告する。共有コードの修正は Linux build を壊し得るので変更内容を明示(こちらで Linux gate をかける)。
3. 起動検証前に portability enumeration/subset、Loader + MoltenVK ICD の bundle 配置、runpath、arm64 architecture、license 同梱、code signing を実装・確認する。
4. 起動 gate は`DYLD_LIBRARY_PATH` / `DYLD_FRAMEWORK_PATH`をunsetし、一括判定せず、`volkInitialize` → `vkCreateInstance` → MoltenVK physical-device 列挙 → `vkCreateDevice` → Metal surface → swapchain → first frame → ログイン画面 → ログイン → 左下チャット文字描画の順に、各段階の log と VERIFIED/CLAIMED/OPEN を報告する。validation/MVK log も同時に保存する。
5. package gate では、Vulkan SDK を system install していないApple Silicon環境でも署名済み app/DMG が起動できることを確認する。`file` / `lipo -archs` でmain executableがarm64-only、Vulkan実行経路の依存がarm64 sliceを持つことを確認し、x86_64-only artifact一覧、`otool -L`、`codesign --verify --deep --strict --verbose=2` の結果を添付する。
6. NSOpenGLView/CGL 退役は**上記の実機 gate 通過後**に別変更として着手し、同じ gate を再実行する(先にやらない)。
7. 不明点・設計判断は AYA(mayatonton)へ。この文書と HEAD が食い違ったら、**コードを正としてその旨も報告**してほしい(文書を直す)。
