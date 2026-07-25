# AYAstorm r42 macOS MoltenVK 実行時ブートストラップ仕様

## 目的と対象

Apple Silicon（arm64）の macOS 開発 app を、Vulkan Loader と MoltenVK を
app bundle 内の成果物だけで起動するための実装・検証仕様である。

- 対象: `build-darwin-universal/newview/Release/AYAstorm.app`
- 描画経路: Vulkan / MoltenVK
- 対象外: Intel Mac、OpenGL へのフォールバック、DMG 配布・notarization

通常のローカル app ビルド手順は
[`docs/build/building_ayastorm_macos.md`](../build/building_ayastorm_macos.md) を参照する。

## 既知の別件（このブランチでは修正しない）

次の不具合は確認済みだが、MoltenVK runtime bootstrap の修正対象ではない。
このブランチでコード変更・原因調査・回帰修正を行わず、それぞれ専用の別ブランチで扱う。

| 事象 | 状態 | 対応方針 |
| --- | --- | --- |
| `r32-bugfix-2` で実装した上部ステータスバーの 3D Stream ボタンなどの UI が表示されない | 確認済み | UI 回帰として別ブランチで再現・修正する |
| SIM を複数越えると crash する | 確認済み、原因未検証 | 別ブランチで log・process 状態・crash trace を採取してから原因を特定し、修正する |

## 実行時の成立条件

app bundle には次の 3 点がそろっていなければならない。

- `Contents/Frameworks/libvulkan.dylib`
- `Contents/Frameworks/libMoltenVK.dylib`
- `Contents/Resources/vulkan/icd.d/MoltenVK_icd.json`

`volk.c` は macOS で最初に
`@executable_path/../Frameworks/libvulkan.dylib` を開く。続いて
`LLVKLoader::initVulkan()` が bundle 内の ICD manifest を `VK_DRIVER_FILES`
へプロセス内で設定する。利用者が `VK_ICD_FILENAMES`、`VK_LAYER_PATH`、
`DYLD_LIBRARY_PATH` を指定する必要はない。

MoltenVK の検出では、`createInstance()` が
`VK_KHR_portability_enumeration` と
`VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` を必須として有効化する。

## 実装済みの復旧処理

### 互換性を失った pipeline cache

`pipeline_cache.bin` は Vulkan 実装・GPU ドライバに依存する。MoltenVK または
Metal 更新後にキャッシュを `vkCreatePipelineCache` が拒否した場合、同じ Vulkan
device で空の初期データを使って一度だけ再試行する。

- 拒否されたキャッシュはその実行では使用しない。
- 起動完了後は通常どおり新しいキャッシュを書き出す。
- キャッシュを手作業で削除することや OpenGL に切り替えることを通常の復旧手段にしない。

### 画面表示までの初期化

`LLAppViewer::initWindow()` は Vulkan 初期化成功後に `initSurface()`、
`initSwapchain()` を順に実行する。いずれかが失敗した場合は失敗箇所をログへ記録し、
Vulkan presentation を成功したものとして扱わない。

macOS では swapchain の extent に native window の backing-pixel size を使う。
これにより CAMetalLayer、レンダー解像度、ポインタ座標を Retina display 上で一致させる。

## 通常起動での確認

arm64 の `ayastorm-bin` をビルド後、環境変数を追加せず app を起動する。
ログイン画面が全面に表示され、操作できることを最終受け入れ条件とする。

起動ログでは、ログイン画面作成前に少なくとも次を確認する。

```text
Vulkan: macOS Loader driver manifest: .../Resources/vulkan/icd.d/MoltenVK_icd.json
Vulkan: VK_KHR_portability_enumeration enabled
Vulkan: initialized device=...
Vulkan: Vulkan presentation surface initialized
Vulkan: macOS swapchain drawable extent=...
RenderInit: Vulkan/MoltenVK startup: bypassing legacy GPU benchmark; using GPU class 3
AppInit: Initializing Login Screen
```

以前の cache が非互換だった場合は、次の警告の後も `initialized device=...` が続くことを確認する。

```text
Vulkan: vkCreatePipelineCache rejected cached data result=...; retrying with an empty cache
```

bundle の検証では、3 つの runtime artifact、`lipo -archs` の `arm64`、
および `codesign --verify --deep --strict` が成功することも確認する。

## 診断の読み方

| ログ | 意味 | 対応箇所 |
| --- | --- | --- |
| `missing bundled driver manifest` | bundle に ICD manifest がない | `viewer_manifest.py` と bundle staging |
| `volkInitialize failed` | Loader を開けない | `volk.c` と `Contents/Frameworks` |
| `VK_KHR_portability_enumeration unavailable` | MoltenVK を portability driver として列挙できない | Loader / ICD / instance extension |
| `no suitable physical device` | 必要な Vulkan device を選べない | MoltenVK / device capability |
| `vkCreatePipelineCache rejected cached data` | 旧 driver の cache が非互換 | 空 cache 再試行後の初期化継続を確認 |

## 実装境界

- 関連実装は `indra/llrender/volk.c`、`indra/llrender/llvkloader.cpp`、
  `indra/newview/llfeaturemanager.cpp`、`indra/newview/llappviewer.cpp`、
  `indra/newview/viewer_manifest.py` にある。
- MoltenVK 固有の処理は Darwin / Metal の条件分岐内に閉じる。Linux と Windows の
  Loader 発見・instance extension の振る舞いは変更しない。
- `NSOpenGLView` / CGL の廃止は別タスクであり、この仕様で OpenGL fallback は追加しない。
