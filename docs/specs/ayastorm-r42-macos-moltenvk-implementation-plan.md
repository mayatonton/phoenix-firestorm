# AYAstorm r42 — macOS MoltenVK implementation plan

Date: 2026-07-15 / Updated: 2026-07-16 / PR merge target: `dev/ayastorm-vk-premt@e7747fb267` / 実装source branch: `feat/macos-moltenvk-bootstrap@444612c9d0` / PR統合 branch: `feat/macos-moltenvk-premt`

この文書は `docs/for_mac_developper.md` を実際の変更、ビルド、実機検証へ落とすための実行計画である。実装済みの仕様書ではない。証拠状態と作業状態を分け、途中で文書と HEAD が食い違った場合は HEAD を正としてこの文書を更新する。

macOSの製品対象architectureは **Apple Silicon arm64のみ** とする。Universal binary、Rosetta x86_64、Intel Macは本計画のbuild/runtime/distribution gateから除外し、未実施でもarm64版の停止条件にしない。共有コードに対するLinux/Windows gateはarchitecture方針とは別に維持する。

## 0. 運用原則

1. 証拠は **VERIFIED / CLAIMED / OPEN**、作業状態は **NOT_STARTED / IN_PROGRESS / BLOCKED** で別々に報告し、コマンド、ログ、`file:line`を添える。
2. sourceを変更する前にbaseline configure/buildを実行し、停止した根因を分類する。
3. 複数ファイルの機械的一括置換を行わない。対象箇所を読んでから1ファイルずつ編集する。
4. macOS専用変更は`DARWIN`または`VK_USE_PLATFORM_METAL_EXT`で隔離し、Windows/Linuxのコンパイル・実行経路を変えない。
5. r42の描画経路は **Vulkan-only** とし、GL描画fallbackを復元しない。HEADでは`RenderBackend=1`と`2`に実行上の差がなく、主要draw pathはVulkan描画不成立時にdrawをskipする。この事実と異なる既存setting commentを正す。
6. NSOpenGLView/CGLはGL描画fallbackではない。window、input、IME、Retina座標変換などのbootstrap依存が残るため、退役は実機gate後の別変更とする。
7. Vulkan初期化、surface、present support、swapchainのいずれかが失敗した場合、GLへ戻らず、作成済みresourceをrollbackして起動を明示的に失敗させる。
8. gate未通過の項目について完了宣言を行わない。

## 1. HEAD code snapshot

| 項目 | 証拠 | 作業状態 | 根拠 |
|---|---|---|---|
| plan / integration base | **VERIFIED** | **IN_PROGRESS** | 実装sourceは`dev/ayastorm-vk-3os@444612c9d0`。PR targetは`dev/ayastorm-vk-premt@e7747fb267`で、target側14 commitと共有Vulkan変更が重複するため、`e7747fb267`直上の`feat/macos-moltenvk-premt`へhunk単位で移植する |
| renderer mode | **VERIFIED** | **NOT_STARTED** | `llvkloader.cpp:7409-7422`は`0`と非`0`だけを区別し、`llvertexbuffer.cpp:588-939`にはGL fallbackがない。既存setting commentはHEADと不一致 |
| portability enumeration/subset | **VERIFIED** | **IN_PROGRESS** | `llvkloader.cpp:471-512,577-583,862-923,985-1000`へMetal限定で実装し、Loader経由の実機runでinstance enumerationとdevice subsetの有効化を確認 |
| presentation readiness / present queue | **VERIFIED** | **IN_PROGRESS** | `llappviewer.cpp:initWindow()`はVulkan/surface/swapchain失敗を返し、`LLAppViewer::init()`もその戻り値を検査する。surface present support query、stage別log、partial-init rollbackはOPEN |
| macOS Vulkan dependency | **VERIFIED** | **IN_PROGRESS** | `Vulkan.cmake:22-40`と`Glslang.cmake:30-39`でlocal `vulkan_sdk_macos` packageをDarwin限定入力として使用。正式package公開とtracked `autobuild.xml`はOPEN |
| Loader / MoltenVK packaging | **VERIFIED** | **IN_PROGRESS** | `viewer_manifest.py:1594-1601,1687-1714`でLoader、MoltenVK、bundle相対ICDを配置し、arm64 appの署名と環境overrideなし起動を確認。license追加はOPEN |
| macOS build / runtime | **VERIFIED** | **IN_PROGRESS** | 旧baseではarm64 `ALL_BUILD`、1x/2x表示、modal hit、resize収束を確認。cache復旧差分を含む同一arm64 executableの後続runはlogin success、initial simulator、movement complete、WebRTC terminate、Vulkan shutdown、`Goodbye!` / `status: stopped`へ到達した。validation layer無効runで、`dev/ayastorm-vk-premt`移植後の再build/runはOPEN |

開発者固有のOS状態、toolの絶対path、空き容量、local checkout、作業logはtracked documentへ記載しない。再現可能な結論だけを、実行commandと必要なversion条件へ一般化して反映する。

### 1.1 2026-07-16 execution snapshot

以下は実行済み部分だけを記録した中間snapshotであり、Phase全体の完了を意味しない。

| 項目 | Evidence | Execution | 再現可能な結論 |
|---|---|---|---|
| SDK / MoltenVK入力 | **VERIFIED** | **IN_PROGRESS** | Vulkan SDK 1.4.350.1と、cloneからbuildしたMoltenVK 1.4.2をlocal darwin64 packageへ固定。arm64 Loader/MoltenVK/glslangを検査済み |
| configure / compile | **VERIFIED** | **IN_PROGRESS** | macOS configure成功。portability/Retina等を含む時点の`ARCHS=arm64 ONLY_ACTIVE_ARCH=YES`の`ALL_BUILD`はexit 0、build log SHA-256=`01f1ba294eb3d1d817e9cee96b28135277eb7bdcc687c50206d85155c4786e2f`。その後のpipeline cache recovery差分は`llrender` compileとviewer linkのみ成功し、最終manifestは`llbase`未解決で失敗。現source全体と`dev/ayastorm-vk-premt`移植後のfull buildはOPEN |
| arm64 artifact | **VERIFIED** | **IN_PROGRESS** | main executableはarm64-only。Loader/MoltenVKはarm64を含むUniversal入力で、ICD JSON parseとbundle相対path、deep/strict署名を確認。既存のVivox 3点とVLC SIMD plugin 6点はx86_64-onlyのため、完全native arm64配布として扱うかはOPEN |
| bundle discovery | **VERIFIED** | **IN_PROGRESS** | driver関連と`DYLD_*` overrideをunsetしたapp起動で、bundle内Loader、ICD、MoltenVKからApple GPUを作成 |
| portability | **VERIFIED** | **IN_PROGRESS** | Loader経由でenumeration extension/flagをrequestし、instance成功後にenabledを記録。selected deviceがsubsetを広告した場合だけdevice extensionをrequestし、device成功後にenabledを記録 |
| arm64 startup | **VERIFIED** | **IN_PROGRESS** | `RenderBackend=1`の製品相当runでMoltenVK 1.4.2、1x/2xの全windowログイン画面、modal button hit、resize収束を確認。拒否されたpersisted cacheを隔離した同一UUIDの後続runは`STATE_LOGIN_WAIT`からlogin success、world接続、正常終了まで到達 |
| validation | **VERIFIED** | **IN_PROGRESS** | 利用者指示により一時中断。portability swizzle/geometry VUIDとdevice destroy VUIDは修正後runで0件。一方、ログイン後runでMoltenVK `main0_out` / color attribute ERRORを24件採取したため、validation/MVK ERROR 0件gateは未通過 |

### 1.2 2026-07-16 Retina / input 実機failure snapshot

このsnapshotは、実機で観測した事実と、まだruntime値を採取していない推定を分離する。スクリーンショットやlocal logの絶対pathはtracked documentへ記載しない。

| 分類 | 状態 | 根拠 |
|---|---|---|
| 表示寸法 | **VERIFIED** | 実機スクリーンショットは5120x2830 pixelで、描画内容は概ね左下2560 pixel幅に収まり、残りが黒。起動logではscreen targetを2560x1387、resize後に2560x1421 / 2560x1368で確保 |
| resize後のframe開始 | **VERIFIED** | resize直後から`LLAppViewer::doFrame()`の`beginFrame return false`がcount=1、10、100まで継続。`llvkloader.cpp:3385-3409`はpending resizeとswapchain extentが一致しないとrecreateしてfalseを返す |
| Metal drawable | **VERIFIED (source)** | `llwindowmacosx-objc.mm:244-253,267-270`は`backingScaleFactor`と`convertSizeToBacking()`から`CAMetalLayer::drawableSize`を設定する |
| HiDPI setting境界 | **VERIFIED** | 最新runはcommand line overrideなし、user settingsにも`RenderHiDPI` overrideがなく、既定値は0(`settings.xml:11431-11440`)。`llappviewer.cpp:640-643`はこの値を`gHiDPISupport`へ反映する一方、Metal layer作成・更新は同flagを参照せず常にwindowのbacking scaleを使う |
| viewer resize / input | **VERIFIED (source)** | `llopenglview-objc.mm:168-171`はbacking変換したresizeを通知し、`339-425`はmouse位置をbacking変換してC++へ渡す。`llviewerwindow.cpp:1141-1148,1547-1553`は受取座標を`mDisplayScale`で割る |
| swapchain収束 | **VERIFIED (source)** | `createSwapchain()`は`caps.currentExtent`を優先し、未固定時は1280x720を使う(`llvkloader.cpp:2629-2644`)。`recreateSwapchain()`は成功後もpending width/heightを実extentへ同期せず(`2805-2841`)、次の`beginFrame()`で再比較する |
| 2倍不一致の直接原因 | **VERIFIED boundary / OPEN runtime tuple** | viewer側HiDPI設定が0でもMetal layerだけがbacking scaleを強制する境界不整合は実読確認済み。live runの`NSView bounds`、`CAMetalLayer.drawableSize`、`caps.currentExtent`、`sSwapchainExtent`、pending resize、最終viewportを同一frameで記録していないため、2560と5120が確定する最終runtime値は追加logで採取する |
| ログイン操作不能 | **CLAIMED / modal VERIFIED** | 実機利用者の操作不能報告は再現入力log未採取。添付画面中央にはWeb browser確認modalが表示され、logも同alertを記録している。このmodal表示中に背面のログインbuttonが操作不能なのは仕様どおり。modalのOK/Cancel自体が操作不能か、dismiss後もログインbuttonが外れるかを別々に再現する |
| 一瞬のピンク表示 | **VERIFIED / OPEN** | 中断直前runで起動直後の全window magenta frameを画像捕捉。swapchain初回clearは黒(`llvkloader.cpp:4454-4468`)で、world deferred pathのmagenta clear(`llviewerdisplay.cpp:1005-1015,1326-1335`)もログイン前経路へ直接結び付けられないため、生成元はOPEN |

**2026-07-16修正後snapshot:** `RenderHiDPI=0`では初期2560x1387、10回の連続resizeで2302x1245 / 2179x1245へ各回収束し、modal buttonを左clickで操作できた。`RenderHiDPI=1`では初期4358x2490、1回のresize後4096x2490へ収束し、MoltenVKはcontents scale 2.0を報告、同じscreen位置のmodal buttonを操作できた。両runとも全window表示と正常終了を確認した。修正点はCocoa event/viewer/CAMetalLayerの1x/2x単位統一、Vulkan drawable専用resize通知、実測extent選択、成功後のpending消費である。約1/2寸法表示、modal hit-test、再作成loopはログイン前P0の停止条件から外す。

**OPEN:** 2xで10回以上の連続resize、minimize/restore、fullscreen、display移動、最新validation全体のERROR 0件、pink flashのframe trace、左下チャット文字を含むログイン後描画完全性。製品相当の後続runはloginと正常終了へ到達したがvalidation layer無効で、sampler上限超過を8件記録した。Phase 6A全体は合格扱いにせず、NSOpenGLView/CGL退役と配布packageへ進まない。

### 1.3 2026-07-16 launch-time code-sign failure snapshot

| 項目 | 証拠状態 | 根拠 |
|---|---|---|
| crash分類 | **VERIFIED** | 利用者添付reportは2026-07-16 02:55、incident=`61CFE439-2CD0-4EA8-8801-36453BE1D3A4`、binary UUID=`3747E29E-C662-3177-9576-C98519DA1EA7`。launchから約2.19秒後、thread 0のdyld内で`SIGKILL (Code Signature Invalid)` / `Namespace CODESIGNING, Code 2 Invalid Page`。viewer shutdown/Vulkan shutdown crashではない |
| 読み込まれた候補 | **VERIFIED** | 診断runがbuild treeのlibrary directoryを`DYLD_LIBRARY_PATH`へ設定し、bundleが要求する`libfmod.dylib`と同名の外部dylibを優先させた |
| 署名状態 | **VERIFIED** | 外部`libfmod.dylib`のarm64 sliceはstrict codesign検証に失敗。bundle内`Contents/Resources/libfmod.dylib`とappのdeep/strict検証は成功 |
| 再現境界 | **VERIFIED** | `--noaudio`はdyld loadを止めない。`DYLD_LIBRARY_PATH`をunsetした後続runは同じbundleで起動・正常終了 |

**運用gate:** 製品runとvalidation runの両方で`DYLD_LIBRARY_PATH` / `DYLD_FRAMEWORK_PATH`をunsetする。validation layerは必要なら`VK_LAYER_PATH`だけで指定する。起動前にappとbundle内dylibのstrict署名を検査し、この事象を終了時Vulkan errorへ混在させない。

診断logはsource treeへ追加しない。PRへ証拠を添付する場合は、絶対pathとユーザー情報をredactし、完全command、exit code、SHA-256を別manifestへ記録する。

### 1.4 2026-07-16 pause / resume snapshot

ログインと正常終了へ到達できる再開用appを保持しつつ、PR target移植前の旧base snapshotとして記録する。作業状態は既定の`NOT_STARTED / IN_PROGRESS / BLOCKED`だけを使い、旧baseでの`VERIFIED`を`dev/ayastorm-vk-premt`移植後の証拠へ流用しない。

| 項目 | Evidence | Execution | 中断時点の事実 |
|---|---|---|---|
| pipeline cache recovery | **VERIFIED** | **IN_PROGRESS** | 中断中。`llvkloader.cpp:createPipelineCache()` (`1652-1733`)はpersisted blob失敗時だけ`.rejected`へ隔離し、empty cacheを1回再試行。実機で8,959,058 bytes、`result=-3`、`empty_retry result=0`、`STATE_LOGIN_WAIT`を確認 |
| shutdown buffer sweep | **VERIFIED** | **IN_PROGRESS** | 中断中。`llvertexbuffer.cpp`のlive registry (`325-333`)と`cleanupClass()` sweep (`971-1015`)で中断直前runは`registered=49 live=49`から`remaining=0` |
| resume app | **VERIFIED** | **IN_PROGRESS** | 中断中。main executableはarm64-only、UUID=`D67271A2-C346-35F9-9811-2E5AAF8F484C`、SHA-256=`2c9bae389aa993b35937206d345da9a197a9fc81268fa78054cacab1b0465446`。ad-hoc deep/strict署名合格、overrideなしでログイン画面へ到達 |
| latest product-like run | **VERIFIED** | **IN_PROGRESS** | 同一UUIDの後続runはlogin success、initial simulator、movement complete、`Terminating WebRTC`、Vulkan shutdown、`Goodbye!`、`status: stopped`へ到達し残留processなし。`[VK-ERROR]`、VUID、`mvk-error`、`main0_out`は0件だがvalidation layer無効。sampler上限超過は8件 |
| latest packaging | **VERIFIED** | **IN_PROGRESS** | 中断中。cache復旧差分のarm64 `llrender` compileとviewer linkは成功。Xcode targetの最終manifestはPythonが`llbase`を解決できず失敗。再開時はrepo venvのsite-packagesをbuild environmentへ指定する。link済みbundleをad-hoc再署名して上記runに使用 |
| fragment output | **VERIFIED** | **NOT_STARTED** | 中断中。ログイン後runの24件はMoltenVK MSL `main0_out`でcolor attributeが明示されないERROR。共通入力は`layout(location = 0) out vec4 frag_data[4];`、変換境界は`llglslshader.cpp:vulkanizeStageSource()` (`709-920`)。恒久修正は未実装 |
| pink frame | **VERIFIED** | **NOT_STARTED** | 中断中。起動直後の全window magenta frameを画像捕捉。生成render pass/attachment/clearは未特定 |
| shutdown crash | **VERIFIED** | **IN_PROGRESS** | 2026-07-16 11:40、incident=`4E57A208-FDDE-4291-B100-98246D098836`、UUID=`D67271A2-C346-35F9-9811-2E5AAF8F484C`で`SIGSEGV`。faulting audio thread先頭は`LLWebRTCLogSink::OnLogMessage()`。同一UUIDの後続runは正常終了したため、根因と再現条件はOPEN |
| PR integration | **VERIFIED** | **IN_PROGRESS** | `origin/dev/ayastorm-vk-premt@e7747fb267`を取得し、その直上に`feat/macos-moltenvk-premt`を用意。現在diffをそのままmergeすると共有Vulkan挙動とtarget側14 commitが混在するため、PR作成は移植・再gateまでBLOCKED |

**再開時の順序:** `dev/ayastorm-vk-premt@e7747fb267`へmacOS責任範囲をhunk単位で移植 → 共有resize依存を非Darwin無変更へ再構成 → repo venvを使ってarm64 viewer/manifestをbuild → cache再生成 → fragment outputとsampler上限をvalidation付きで診断 → ログインとチャット文字 → 正常終了/device destroy → shutdown crash再現監査 → pink frame trace → resize/minimize/fullscreen/display gate。`LLVoiceClient::terminate()`のinstance条件(`llvoiceclient.cpp:306-315`)は強い監査候補だが、後続正常終了だけで根因解消としない。共有変更のLinux/Windows gateはOPENのまま明示する。

## 2. 固定する設計境界

### 2.1 runtime構成

基準構成は **Vulkan Loader + MoltenVK ICD** とする。Apple product buildではbundle内Loaderを必須とし、volkの`libMoltenVK.dylib`直接fallbackを無効化する。Loaderをloadできない場合はVulkan初期化を失敗させ、GLへfallbackしない。

```text
AYAstorm executable
  -> Contents/Frameworks/libvulkan*.dylib
  -> Contents/Resources/vulkan/icd.d/MoltenVK_icd.json
  -> Contents/Frameworks/libMoltenVK.dylib
  -> Metal
```

ICD JSONは採用SDK付属版を基準とする。`library_path`はJSON位置を基準に次の相対pathとし、同一SDKのLoaderで`file_format_version`、`api_version`、`is_portability_driver`の解釈を実読・実行確認する。

```json
"library_path": "../../../Frameworks/libMoltenVK.dylib"
```

`VK_DRIVER_FILES`を主診断手段とし、deprecatedな`VK_ICD_FILENAMES`は旧Loader互換確認時にだけ使用する。製品gateでは`VK_DRIVER_FILES`、`VK_ICD_FILENAMES`、`VK_ADD_DRIVER_FILES`、`VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、関連`DYLD_*` overrideをunsetし、通常起動要件にしない。本計画の配布対象はDeveloper ID署名DMGによる直接配布であり、Mac App Storeは対象外とする。

### 2.2 dependency入力

開発時はLunarG公式macOS Vulkan SDKをversion/checksum固定で使用する。release buildが開発者固有の絶対pathに依存しないよう、次の内容を持つdarwin64 autobuild package(仮称`vulkan_sdk_macos`)を作成する。

- Vulkan headers
- Vulkan Loader dylibと必要なversioned name
- `libMoltenVK.dylib`
- `MoltenVK_icd.json`
- HEADが要求するglslang CMake config、`glslang::glslang`、`glslang::SPIRV`、`glslang::glslang-default-resource-limits`を解決する入力
- Loader / MoltenVK / Headersのライセンス

package install treeは少なくとも`include/vulkan/**`、`lib/release/**`、`share/vulkan/icd.d/MoltenVK_icd.json`、`lib/cmake/**`、`LICENSES/**`へ固定する。source URL、version、archive SHA-256、抽出component、各binary SHA-256、architecture、minimum OS、license/NOTICE、再配布可否をpackage manifestへ記録する。vendored Volk headerとのheader version整合も確認する。

最初はgit-ignoredの`my_autobuild.xml`へlocal packageを登録して検証する。正式packageのrepository名、配布先、固定versionはAYAの設計判断後に確定し、3p packageを公開してからviewer側の`autobuild.xml`を変更する。

### 2.3 他OSの不変条件

- Windowsの`VK_USE_PLATFORM_WIN32_KHR`経路を変えない。
- Linuxの`VK_USE_PLATFORM_XLIB_KHR`経路を変えない。
- Windows/Linuxのinstance/device extension一覧を変えない。
- Windows/LinuxではmacOS用SDK packageを取得・include・packageしない。
- shared codeの呼出順、short-circuit、cleanupを変える場合は変更内容を明示し、Linux/Windows gateをmerge-blockingとする。
- partial-init failure cleanupは監査対象外にせず、作成済みresourceとVolk moduleの逆順rollbackを3 OSで検証する。

## 3. 変更対象

| ファイル / 関数 | 予定する変更 | 変更しない範囲 |
|---|---|---|
| `indra/llrender/llvkloader.cpp:createInstance()` | Metal限定portability enumeration、instance flag、validation extension交渉、結果log | 他OSのextension list |
| `indra/llrender/llvkloader.cpp:createDevice()` | Metal限定portability subset検出・有効化、subset feature/property query、required capability gate | beta headerの全OS有効化 |
| `indra/llrender/llvkloader.cpp:initVulkan()`ほか初期化関数 | instance前段とsurface後段への初期化分割、stage別結果、単一rollback | 証拠なしのrenderer仕様変更 |
| `indra/llrender/llvkloader.cpp:createPipelineCache()` | 拒否されたpersisted blobの隔離、empty cache 1回retry、stage/result log | fragment shader ERRORの握り潰し、無限retry |
| `indra/llrender/llvertexbuffer.cpp:cleanupClass()` | device破棄前のlive Vulkan vertex/index allocation sweep | `LLVertexBuffer` instanceの所有権・寿命変更 |
| `indra/llrender/llglslshader.cpp:vulkanizeStageSource()` | OPEN: fragment `frag_data[4]`を4本の明示location scalar outputへ限定正規化 | shader個別ファイルの機械的一括置換、dynamic indexの推測変換 |
| `indra/llrender/llvkloader.cpp:selectQueueFamily()` / `initSurface()` / `createSwapchain()` | device作成前のgraphics+present queue選択、surface capability、swapchain結果検査 | 検査なしの固定値変更 |
| `indra/newview/llappviewer.cpp:initWindow()` | 各stageの戻り値を状態遷移へ反映し、失敗時は起動停止 | GL描画fallbackの追加 |
| `indra/llrender/volk.c` | Apple product buildのbundle Loader必須化、MoltenVK直接fallback無効化 | Windows/LinuxのLoader検索 |
| `indra/cmake/Vulkan.cmake` | `DARWIN`限定SDK/header入力 | Windows/Linuxの`find_package(Vulkan REQUIRED)` |
| `indra/cmake/Glslang.cmake` / `indra/llrender/CMakeLists.txt` | packageからglslang targetを解決するため実測上必要な最小変更 | 他OStarget構成 |
| `indra/cmake/Variables.cmake` | Darwinの未指定時architectureをarm64とし、明示的なCMake指定を尊重 | 他OSのarchitecture、明示指定したx86_64 / Universal build |
| `indra/newview/viewer_manifest.py` | Darwin manifestでLoader/MoltenVK/ICDを配置 | Windows/Linux manifest |
| `indra/newview/CMakeLists.txt` | staging/rpathが必要な場合だけDarwin節を変更 | 既存他OSlink設定 |
| `indra/llwindow/llwindowmacosx.cpp` / `llwindowmacosx-objc.mm` | presentation-ready gate、CAMetalLayer/drawable-size/delegateの実測修正 | NSOpenGLView/CGLの先行削除 |
| `indra/newview/app_settings/settings.xml` | GL fallback説明を削除し、r42のVulkan-only semanticsへ更新 | 未検証のGL-only動作主張 |
| `indra/newview/licenses-mac.txt` | 配布componentのlicense追加 | 他platform license |
| `docs/build/building_ayastorm_macos.md` | 実行して確認したSDK/configure/package手順 | 未検証手順のVERIFIED化 |

NSOpenGLView/CGLそのものの削除は最初の編集対象にしない。一方、presentation readiness、present queue、surface/swapchainの結果検査は既にHEAD実読で欠落を確認しているためbootstrap変更対象とする。format、extent、composite alpha、CAMetalLayer delegateなどの値はcapability logで不一致を確認してから変更する。

## 4. 実装phaseとgate

各phaseは証拠状態と作業状態を別々に記録する。以下の初期値はすべて`Evidence: OPEN / Execution: NOT_STARTED`である。

### Phase 0 — branch / evidence準備

1. この計画がbase branchへ反映された後、`feat/macos-moltenvk-bootstrap`を作成する。
2. `git status`、HEAD、OS/CPU、Xcode、CMake、Python、autobuildのversionを作業logへ保存する。
3. phaseごとにHEAD、完全command、開始終了時刻、exit code、log SHA-256をevidence manifestへ記録する。
4. build/configure/runtime logはsource treeへcommitせず、repo外へ保存し、PR添付前にusername、絶対path、token、Apple ID、署名情報をredactする。

**gate出力:** environment snapshot、evidence manifest、証拠状態と作業状態。

### Phase 1 — SDKなしsource未変更baseline

1. full Xcodeを`xcode-select -p`と`xcodebuild -version`で確認する。切替が必要なら`DEVELOPER_DIR`をcommand単位で指定する。
2. venv、`AUTOBUILD_VARIABLES_FILE`、必要な場合だけ`AUTOBUILD_CONFIG_FILE=my_autobuild.xml`を設定する。
3. channelを **`AYAstorm-release`** としてsource未変更のconfigureを実行し、`CMakeCache.txt`の`VIEWER_CHANNEL`も保存する。`AYAstorm-VK-release`は使用しない。
4. SDK未導入状態のconfigure logとexit codeを保存する。
5. environment/configure、compile、link、Loader/ICD、mac固有、共有コード、packaging/signingへ分類する。

**gate出力:** 再実行可能なcommand template、完全log、到達した最初の根因群。未到達エラーはOPENとする。

### Phase 2 — Vulkan SDK / darwin64 package

1. 公式SDK version、source URL、download checksum、license、再配布条件を再確認する。
2. SDKを導入し、Vulkan headers、Loader、validation layer、MoltenVK、glslangの実versionを記録する。
3. package install tree、CMake config target、各binaryのSHA-256、`file`、`lipo -archs`、`vtool -show-build`を記録する。
4. viewerのminimum macOSとLoader/MoltenVK各sliceの`minos`を照合する。
5. local darwin64 packageを作成し、`my_autobuild.xml`から取得できることを確認する。
6. arm64の`vulkaninfo`等でdevice名、API version、extensions、features、formatsを保存する。Apple GPUだけに限定しない。
7. 正式packageの公開先をAYAが決定し、packageを公開してからviewer側`autobuild.xml`を変更する。

**停止gate:** package provenance、再配布可否、glslang解決、header/minimum OS整合のいずれかが不明ならBLOCKED。

### Phase 3 — SDK準備後source未変更baseline

1. Phase 2のlocal packageを入力し、source未変更でconfigure/buildする。
2. source SHAを固定し、可能ならkeep-going相当と独立targetごとの実行で到達可能なエラーを最大化する。
3. コンパイラ停止後の未到達箇所を「全エラー」と呼ばずOPENに残す。
4. mac固有、共有コード、build/packageへ分類し、共有コード修正候補を明示する。

**gate出力:** command、完全log、exit code、分類表、最初の根因群。ここまでコード編集禁止。

### Phase 4 — portability / required capability / startup state

1. `createInstance()`でMetal buildだけが`VK_KHR_portability_enumeration`を列挙・追加し、追加できた場合だけ`VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`を設定する。
2. validation layer固有extensionを列挙し、`VK_EXT_validation_features`が存在する場合だけ有効化して`VkValidationFeaturesEXT`をpNextへ連結する。
3. instance作成とphysical device選択をwindow作成前段、surface作成、graphics+present queue選択とdevice作成を後段へ分ける。
4. surface作成後かつdevice作成前に`vkGetPhysicalDeviceSurfaceSupportKHR()`を実行し、graphics+present queue familyを確定する。
5. `createDevice()`でdeviceが公開する場合だけ`VK_KHR_portability_subset`を追加し、subset features/propertiesをqueryする。
6. API 1.3、dynamic rendering、`VK_KHR_swapchain`、必要format/usage/limitをrequired capability matrixとしてdevice作成前に検査する。
7. extension名だけが必要な段階ではMetal guard内の局所定数`"VK_KHR_portability_subset"`を使い、`VK_ENABLE_BETA_EXTENSIONS`を追加しない。subset feature/property structをqueryする段階でbeta headerが必要になった場合だけ、Darwin限定targetまたはtranslation unitに閉じ、3 OS共通定義にしない。
8. `sVulkanPresentationEnabled`は初期値falseとし、surface、present support、swapchain、acquire/present準備が成立した後だけtrueにする。
9. 各stage失敗時は単一rollbackでresourceとVolk moduleを逆順破棄し、GLへ戻らず起動を失敗させる。
10. `RenderBackend=1`は移行中のVulkan-only legacy alias、`2`はcanonical Vulkan-only値として扱う。既定値移行とpersisted value処理を実装し、`0`のGL-only動作をサポート済みと主張しない。
11. Apple product buildのvolkをbundle Loader必須へ切り替える変更までは、現存するMoltenVK直接fallbackを壊さない。Loader必須化を実装・failure-injection検証した同じ変更で直接fallbackを無効化する。

**static/compile gate:** macOS compile、`git diff --check`、guard実読、failure injectionによるrollback確認、diff監査。Linux/Windows gateはOPENのまま明示する。

### Phase 5 — bundle packaging / macOS arm64 staging

1. LoaderとMoltenVKを`Contents/Frameworks`、ICD JSONを`Contents/Resources/vulkan/icd.d`へ配置する。
2. SDK付属JSONを基準に`library_path`だけをbundle相対pathへ調整し、同一Loaderでschemaとportability driver認識を確認する。
3. `@executable_path/../Frameworks` runpath、Loader install name、dependency pathを確認する。
4. licensesを追加する。
5. fresh configureの`CMAKE_OSX_ARCHITECTURES=arm64`を確認し、`ARCHS=arm64 ONLY_ACTIVE_ARCH=YES`でviewerをbuildしてappをstagingする。
6. source変更ごとに同じtargetを再buildし、mac固有修正と共有コード修正をcommit単位で分ける。

**gate:** `file`、`lipo -archs`、`otool -D/-L`、`vtool -show-build`、JSON parse、relative path、main executableがarm64-onlyであること、Vulkan実行経路の全Mach-Oにarm64 sliceがあること、x86_64-only artifact一覧、ad-hoc staging署名、viewer binary、build log。配布署名はまだ要求しない。

### Phase 6 — arm64実機startup / rendering / lifecycle

製品gateはVulkan/Layer/DYLD overrideをunsetして実行する。別の診断runでは`DYLD_LIBRARY_PATH` / `DYLD_FRAMEWORK_PATH`を引き続きunsetし、`AYASTORM_VK_VALIDATION=1`、`VK_LOADER_DEBUG=all`、固定版に対応する`MVK_CONFIG_LOG_LEVEL=4`、必要な場合だけ`VK_LAYER_PATH`を使い、stdout/stderrを保存する。

#### Phase 6A — P0 Retina / drawable / input 収束

2026-07-16の実機failureにより、以下をログイン操作より先に通す。値を推定で補わず、同一frameまたは同一resize eventにcorrelation IDを付けて記録する。

1. `NSView.bounds`のpoint寸法、`backingScaleFactor`、`RenderHiDPI`、`mDisplayScale`。
2. `CAMetalLayer.contentsScale`と`drawableSize`。create時、resize前後、display移動時を記録する。
3. `VkSurfaceCapabilitiesKHR::{currentExtent,minImageExtent,maxImageExtent}`、swapchainへ要求したextent、作成後の`sSwapchainExtent`。
4. `notifyWindowResize()`へ渡したpending width/height、`gGLViewport`、swapchain rendering開始時のrender area。
5. pointer eventのCocoa point、backing変換後座標、`mDisplayScale`除算後座標、browser確認modalのbutton rect、dismiss後のログインbutton rectを各1操作だけ記録する。認証情報や入力文字列は記録しない。
6. `RenderHiDPI=0`と`1`を別runで検証し、いずれも`drawableSize == swapchain extent == presentation viewport`、UI layoutとpointer hit-testが同じ座標系へ収束することを確認する。
7. `caps.currentExtent == UINT32_MAX`の経路では1280x720固定値を使わず、実測drawable/pending sizeをmin/maxへclampする。固定extent経路ではsurfaceの値を正とする。
8. swapchain再作成成功後はpending値を実extentへ同期または消費し、失敗時はstageと`VkResult`を記録する。連続resize後に`beginFrame return false`が増え続けないことを確認する。
9. startup中にouter `beginFrame()`がfalseでも`display()`へ進む現行経路(`llappviewer.cpp:1759-1788`)と、`display_startup()`内の再度の`beginFrame()`(`llviewerdisplay.cpp:161-210`)を監査し、command bufferなしdrawや空presentを許容しない。
10. magenta clearを単に黒へ変更して症状を隠さない。pink flashが発生したframeのrender path、active attachment、clear値、present結果をtraceし、生成元を確定してから修正する。

**編集境界:** Cocoa/Metalの単位取得は`llwindowmacosx-objc.mm`へ閉じる。UI reshapeとVulkan drawable通知を分離する必要がある場合、macOS bridgeで両値を明示し、Windows/Linuxのwindow event単位を変更しない。`llvkloader.cpp`のswapchain収束を共有修正する場合はLinux/Windows gateをmerge-blocking OPENとする。

**Phase 6A gate:** 1x/2xの両runで全画面描画、ログインbutton hit、10回以上の連続resize後もswapchain recreateが収束、magenta flashなし。証拠は上記値の対応表、validation/MVK log、スクリーンショット、`beginFrame`/acquire/present件数とする。

#### Phase 6B — startup / rendering / lifecycle

1. bundle内Vulkan Loaderの絶対load path
2. bundle内ICD JSONとMoltenVKの絶対path、version、portability driver認識
3. validation layerと要求extensionの実ロード
4. `vkCreateInstance`、portability enumeration、physical device
5. portability subset feature/property matrixとrequired capability合格
6. Metal surface、graphics+present queue support
7. `vkCreateDevice`、swapchain、選択format/extent/composite alpha/depth format
8. `vkAcquireNextImageKHR`、submit、Vulkan draw count > 0、`vkQueuePresentKHR == VK_SUCCESS`
9. first frame、ログイン画面、ログイン、左下チャット文字描画
10. Retina drawable size、連続resize、minimize/restore、fullscreen、異なるscale/displayへの移動
11. sleep/wake、終了、再起動、一定時間soak

Loader/MoltenVK直接fallback、GL描画fallback、silent draw skipを合格経路にしない。validation ERROR/VUIDは0件を原則とし、warning例外はIDと理由を記録してAYA承認を得る。CPU/RSS、frame time、swapchain recreate回数、異常終了も記録する。

### Phase 7 — arm64 distribution / clean-machine

1. main executableをarm64-onlyとし、Loader、MoltenVK、実行するhelper appを含む全依存にarm64 sliceがあることを確認する。Universal dependencyの余分なx86_64 sliceは本計画では除去要件にしない。x86_64-only artifactは機能単位で一覧化し、Rosetta依存を許容するかAYA判断を得る。
2. Apple Silicon native arm64でPhase 6を再実行する。Rosetta x86_64とIntel Macは対象外とし、OPEN項目にも数えない。
3. `llpackage`とDMGを生成し、Developer ID、hardened runtime、secure timestamp、nested code署名、notarization、stapleを検査する。
4. `hdiutil verify`、`codesign --verify --deep --strict`、各Vulkan dylibのAuthority、`spctl --assess`、`stapler validate`を実行する。
5. Vulkan SDKをsystem installしていない別のApple Silicon Macで、quarantine付き最終DMGからコピーしたappへPhase 6を再実行する。

clean environmentの模擬試験は模擬試験としてVERIFIEDにできるが、別Mac gateはOPENのままとし、CLAIMEDへ置き換えない。

### Phase 8 — Windows/Linux gate / docs / PR

1. PR baseは`dev/ayastorm-vk-premt`、PR headは同base直上の`feat/macos-moltenvk-premt`とする。旧`dev/ayastorm-vk-3os`由来branchをそのままPR headにしない。
2. macOS責任範囲はDarwin限定SDK/glslang、arm64-only既定値、Metal portability enumeration/subset、MoltenVK swizzle emulation、Darwin manifest、Cocoa/CAMetalLayer/Retina、macOS文書とする。共有file内のhunkは`LL_DARWIN` / `VK_USE_PLATFORM_METAL_EXT`またはMoltenVK portability capabilityで非Darwinの実行結果を変えないことを実読する。
3. depth format、descriptor/geometry capability、image/vertex shutdown sweep、startup fail-closed、pipeline cache recoveryを共有挙動のまま同PRへ混ぜない。macOS限定へ再構成できない場合は別commit/PRとしてAYAへ返す。
4. Linux/Windowsでconfigure、compile、起動、device選択、instance/device extension一覧、正常終了を確認する。macOS専用PRとして非Darwin実行経路不変を静的に証明できても、両OS build未実施はmerge-blocking OPENと記録する。
5. shared fileの採用hunkと除外hunk、各OSへの影響をPR本文でfile/function単位に明示する。
6. `docs/build/building_ayastorm_macos.md`を実行済みcommandと期待値に更新する。
7. `docs/for_mac_developper.md`のsnapshot base、証拠状態、file:lineを更新する。
8. 全gate evidenceとdiff監査結果をPRへ添付する。

### Phase 9 — NSOpenGLView/CGL退役

**Phase 0-8の必要gate通過前は着手禁止。別branch・別PRとする。**

1. `LLOpenGLView`の`NSTextInputClient`、IME/preedit、mouse/drag、first responder、Retina変換、shared context、VRAM、vsync、`gGLManager`依存をfile/function単位で列挙する。
2. 代替NSView/window bootstrapを設計し、GL描画fallbackではなくwindow/input依存の退役として実装する。
3. 日本語IME、drag-and-drop、Retina/non-Retina、resize、minimize/restore、display移動を必須gateにする。
4. Phase 5-8と同じbuild/runtime/3 OS gateを再実行する。

`RenderBackend=2`の既定化をCGL退役条件にはしない。Vulkan-only semanticsとsetting migrationはPhase 4で確定させる。

## 5. commit / PR境界

bootstrap PR内でも次のcommit境界を維持する。

1. 3p recipe/package作成と正式package公開
2. macOS dependency/header/glslang wiring
3. portability、validation negotiation、required capability
4. presentation-ready state、present queue初期化順、failure rollback、RenderBackend semantics
5. bundle Loader必須化、Loader/MoltenVK/ICD packaging、license
6. 実機logで必要性を確認したmac固有修正(根因ごと)
7. 実行結果に基づくdocs更新

各code commitは対応するbuild/runtime証拠とdiff監査結果を明示する。NSOpenGLView/CGL退役をbootstrap PRへ混ぜない。共有renderer/lifecycle変更はmacOS限定commitと混ぜず、必要なら別PRへ分離する。

## 6. 停止してAYAへ確認する条件

- MoltenVK未対応機能のため共有renderer仕様を変える必要がある。
- Vulkan API 1.3要件を下げる必要がある。
- required capabilityまたはportability subset制限がrenderer要件を満たさない。
- graphics+present queueをdevice作成前に確定できず、追加の初期化設計判断が必要になる。
- Windows/Linuxのextension、device選択、build dependencyへ影響が出る。
- 新しい3p repository/package公開先を決める必要がある。
- SDK/packageのprovenance、再配布条件、minimum OS、header/glslang整合を確認できない。
- 実機gate前にNSOpenGLView/CGLを削除する必要が生じる。
- handoff、計画書、HEADの内容が食い違う。

## 7. 報告書式

各phaseの報告は次の形式に統一する。

```text
EVIDENCE: VERIFIED / CLAIMED / OPEN

VERIFIED
- command / file:line / log

CLAIMED
- 文書・upstream説明由来で未実行の内容

OPEN
- 未着手、未到達、追加判断が必要な内容

EXECUTION: NOT_STARTED / IN_PROGRESS / BLOCKED

NEXT
- 次に実行する1つのgate
```

## 8. 一次資料

- `docs/for_mac_developper.md`
- `docs/build/building_ayastorm_macos.md`
- [MoltenVK README](https://github.com/KhronosGroup/MoltenVK/blob/main/README.md)
- [MoltenVK Runtime User Guide](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md)
- [MoltenVK configuration parameters](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md)
- [VK_KHR_portability_enumeration](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_enumeration.html)
- [VK_KHR_portability_subset](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_subset.html)
- [Khronos Vulkan Loader driver discovery](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderDriverInterface.md)
- [LunarG macOS SDK version API](https://vulkan.lunarg.com/sdk/latest/mac.json)
- [Apple run-path dependent libraries](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/RunpathDependentLibraries.html)
- [Apple notarization](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
