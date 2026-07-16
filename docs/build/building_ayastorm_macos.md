# AYAstorm Mac版ビルド手順

Date: 2026-07-16
Target example: `dev/ayastorm-vk-premt`
Output example: `Phoenix-FirestormOS-AYAstorm-release_arm64-<version>-<build-id>.dmg`

この文書は AYAstorm の macOS arm64 配布用 DMG を作成するための手順です。r42の描画backendはVulkan専用であり、macOSではbundle内Vulkan Loader + MoltenVK ICDを使用します。Firestorm 本体の一般的な macOS ビルド要件は `doc/building_macos.md` も参照してください。

2026-07-16時点で`vulkan_sdk_macos`はtracked `autobuild.xml`へ未登録です。そのためclean checkout単独ではconfigureできず、正式artifactの公開とtracked metadata反映までmerge / release gateは **OPEN** です。local検証では以下の固定archiveを開発チームから取得し、git-ignoredの`my_autobuild.xml`へ登録してください。開発者固有の絶対pathをtrackedファイルへ追加しないでください。

## 前提

- macOS 15.x
- Xcode 16.x
- CMake
- Python 3.9 互換の venv
- `autobuild`
- `fs-build-variables`
- FMOD Studio API installer for macOS
- FMOD を有効化する場合は、FMOD package を登録した `my_autobuild.xml`
- `vulkan_sdk_macos` 1.4.350.1 package、SHA-256=`a6b0a46d6c5c9e99bf50e5354d7ade5025ec5edaae4e112ebc8b8feb5bcea3b7`

作業ディレクトリ例:

```bash
export WORK="$HOME/work_ayastorm"
export REPO="$WORK/phoenix-firestorm"
export FS_BUILD_VARIABLES="$WORK/fs-build-variables/variables"
export FMOD_REPO="$WORK/3p-fmodstudio"
export TARGET_REF="dev/ayastorm-vk-premt"
export AYA_BUILD_ID="<autobuild build id>"
```

`AYA_BUILD_ID` は Release ページの成果物名に含める autobuild build id です。Release 配布物では、git commit count ではなく CI / autobuild 側の build id を明示して揃えます。

## ソース取得

```bash
mkdir -p "$WORK"
cd "$WORK"

git clone https://github.com/mayatonton/phoenix-firestorm.git "$REPO"
git clone https://github.com/FirestormViewer/fs-build-variables.git "$WORK/fs-build-variables"

cd "$REPO"
git checkout "$TARGET_REF"
git status --short
```

既存 worktree を使う場合は `REPO` にその path を指定し、ビルド前に `git status --short` で差分を確認します。不要な差分を含めたまま Release ビルドしないようにしてください。

## Python / autobuild

```bash
cd "$REPO"

python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
autobuild --version
```

`xcodebuild` から起動される Python が venv の package を参照できるよう、site-packages を `PYTHONPATH` に渡します。

```bash
export PYTHON_SITE="$(python -c 'import site; print(site.getsitepackages()[0])')"
export PYTHONPATH="$PYTHON_SITE"
```

## FMOD Studio API

AYAstorm の Release ビルドでは `--fmodstudio` を付けて configure します。そのため、事前に FMOD Studio API を autobuild package 化し、`my_autobuild.xml` に登録しておく必要があります。

FMOD Studio API は FMOD 公式サイトから macOS 版を取得します。FMOD Studio Tool ではなく、FMOD Studio API installer を使ってください。

この作業環境では FMOD package 作成に既存の local clone を使います。

```bash
export FMOD_REPO="$WORK/3p-fmodstudio"
test -d "$FMOD_REPO/.git"
git -C "$FMOD_REPO" remote -v
```

ダウンロードした macOS 版 FMOD Studio API installer の `.dmg` を `$FMOD_REPO` に置きます。

```bash
cd "$FMOD_REPO"
ls -lh *.dmg

autobuild build -A 64 --all
autobuild package -A 64 --results-file result.txt
cat result.txt
```

`result.txt` に package path と md5 hash が出力されます。作成された `fmodstudio-*-darwin64-*.tar.bz2` を viewer 側の `my_autobuild.xml` に登録します。

```bash
cd "$REPO"
cp -n autobuild.xml my_autobuild.xml
export AUTOBUILD_CONFIG_FILE="my_autobuild.xml"

export FMOD_PACKAGE="$(find "$FMOD_REPO" -maxdepth 1 -name 'fmodstudio-*-darwin64-*.tar.bz2' -print -quit)"
export FMOD_HASH="<result.txt の md5 hash>"

autobuild installables edit fmodstudio platform=darwin64 \
  hash="$FMOD_HASH" \
  url="file://$FMOD_PACKAGE"
```

登録後、`my_autobuild.xml` の `fmodstudio` / `darwin64` が作成した package を指していることを確認します。

```bash
rg -n 'fmodstudio|darwin64|file://' my_autobuild.xml
```

FMOD を使わないビルドにする場合は、以降の configure から `--fmodstudio` を外してください。

## Vulkan SDK / MoltenVK / glslang

macOSだけは`indra/cmake/Vulkan.cmake`と`indra/cmake/Glslang.cmake`が`vulkan_sdk_macos`を要求します。現在の固定packageはLunarG Vulkan SDK 1.4.350.1のheaders、Loader、validation layer、glslang / SPIRV-Toolsと、MoltenVK 1.4.2のlocal buildを含みます。viewer本体はarm64-onlyでbuildしますが、package内dependencyにarm64 sliceと余分なx86_64 sliceがあることは停止条件にしません。

archiveとSHA-256を確認し、FMODと同じ`my_autobuild.xml`へ登録します。

```bash
cd "$REPO"
cp -n autobuild.xml my_autobuild.xml
export AUTOBUILD_CONFIG_FILE="my_autobuild.xml"

export VULKAN_PACKAGE="<vulkan_sdk_macos-1.4.350.1-darwin64-261970001.tar.bz2 の path>"
export VULKAN_HASH="a6b0a46d6c5c9e99bf50e5354d7ade5025ec5edaae4e112ebc8b8feb5bcea3b7"

test "$(shasum -a 256 "$VULKAN_PACKAGE" | awk '{print $1}')" = "$VULKAN_HASH"

if rg -q '<key>vulkan_sdk_macos</key>' my_autobuild.xml; then
  VULKAN_ACTION=edit
else
  VULKAN_ACTION=add
fi

autobuild installables "$VULKAN_ACTION" vulkan_sdk_macos platform=darwin64 \
  hash="$VULKAN_HASH" \
  hash_algorithm=sha256 \
  url="file://$VULKAN_PACKAGE"

rg -n 'vulkan_sdk_macos|1\.4\.350\.1|sha256' my_autobuild.xml
```

`url`のlocal pathは`my_autobuild.xml`のみに保持します。正式package公開後は、レビュ済みのURL、hash、license metadataをtracked `autobuild.xml`へ反映し、このlocal登録を外します。

## Dullahan audio callback

AYAstorm Mac ビルド手順のデフォルトは Dullahan audio callback 経路を有効にします。

`autobuild.xml` または `my_autobuild.xml` に `t-noami/dullahan` fork の installable (`dullahan_aya_audio`) が存在している場合:

```bash
-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE
```

`t-noami/dullahan` fork が存在しておらず、upstream の `secondlife/dullahan` installable (`dullahan`) が存在している場合:

```bash
-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=FALSE
```

configure 後は `build-darwin-universal/CMakeCache.txt` で `LL_DULLAHAN_AUDIO_CALLBACK:BOOL=` の値を確認してください。

## 環境変数

```bash
cd "$REPO"

export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
export AUTOBUILD_BUILD_ID="$AYA_BUILD_ID"
export AUTOBUILD_VARIABLES_FILE="$FS_BUILD_VARIABLES"
export AUTOBUILD_CONFIG_FILE="my_autobuild.xml"
export CLANG_MODULE_CACHE_PATH="$REPO/build-darwin-universal/ModuleCache"
```

## Configure

クリーンに作り直す場合:

```bash
cd "$REPO"
rm -rf build-darwin-universal
```

configure:

```bash
autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio \
  --openal \
  --package \
  --chan AYAstorm-release \
  -DCMAKE_OSX_ARCHITECTURES:STRING=arm64 \
  -DLL_TESTS:BOOL=FALSE \
  -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE
```

configure 後に主要な設定を確認します。

```bash
rg -n 'CMAKE_BUILD_TYPE|ADDRESS_SIZE|CMAKE_OSX_ARCHITECTURES|VIEWER_CHANNEL|USE_FMODSTUDIO|USE_OPENAL|OPENSIM|PACKAGE|VIEWER_BINARY_NAME|LL_DULLAHAN_AUDIO_CALLBACK' \
  build-darwin-universal/CMakeCache.txt
```

期待値の例:

```text
ADDRESS_SIZE:STRING=64
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_OSX_ARCHITECTURES:STRING=arm64
OPENSIM:BOOL=ON
PACKAGE:BOOL=ON
USE_FMODSTUDIO:BOOL=ON
USE_OPENAL:BOOL=ON
VIEWER_BINARY_NAME:STRING=ayastorm-bin
VIEWER_CHANNEL:STRING=AYAstorm-release
LL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE
```

## Build / Package

`llpackage` scheme を Release で実行します。

```bash
cd "$REPO/build-darwin-universal"

DEVELOPER_DIR="$DEVELOPER_DIR" \
AUTOBUILD_BUILD_ID="$AYA_BUILD_ID" \
PYTHONPATH="$PYTHONPATH" \
AUTOBUILD_VARIABLES_FILE="$FS_BUILD_VARIABLES" \
AUTOBUILD_CONFIG_FILE="$AUTOBUILD_CONFIG_FILE" \
CLANG_MODULE_CACHE_PATH="$CLANG_MODULE_CACHE_PATH" \
xcodebuild \
  -project Firestorm.xcodeproj \
  -scheme llpackage \
  -configuration Release \
  -derivedDataPath "$REPO/build-darwin-universal/DerivedData" \
  CLANG_MODULE_CACHE_PATH="$CLANG_MODULE_CACHE_PATH" \
  ARCHS=arm64 \
  ONLY_ACTIVE_ARCH=YES \
  build
```

DMG 作成時に `hdiutil create` が `装置が構成されていません` で失敗する場合は、サンドボックスや権限の制約でディスクイメージ操作が止まっています。同じコマンドを通常の Terminal から実行してください。

## 成果物

```bash
export AYA_VERSION="$(tr -d '\r\n' < "$REPO/indra/newview/VIEWER_VERSION_AYA.txt")"
export AYA_VERSION_DASHED="${AYA_VERSION//./-}"
export DMG="$REPO/build-darwin-universal/newview/Phoenix-FirestormOS-AYAstorm-release_arm64-${AYA_VERSION_DASHED}-${AYA_BUILD_ID}.dmg"
export APP="$REPO/build-darwin-universal/newview/Release/AYAstorm.app"

ls -lh "$DMG"
```

成果物の例:

```text
build-darwin-universal/newview/Phoenix-FirestormOS-AYAstorm-release_arm64-42-0-0-<build-id>.dmg
```

## 検証

ローカル app / DMG の基本検証:

```bash
hdiutil verify "$DMG"
codesign --verify --deep --strict --verbose=2 "$APP"
lipo -archs "$APP/Contents/MacOS/AYAstorm"
test -e "$APP/Contents/Frameworks/libvulkan.dylib"
test -e "$APP/Contents/Frameworks/libMoltenVK.dylib"
test -f "$APP/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"
test -L "$APP/Contents/Frameworks/libvulkan.dylib"
test "$(readlink "$APP/Contents/Frameworks/libvulkan.dylib")" = "libvulkan.1.dylib"
test -L "$APP/Contents/Frameworks/libvulkan.1.dylib"
test "$(readlink "$APP/Contents/Frameworks/libvulkan.1.dylib")" = "libvulkan.1.4.350.dylib"
lipo -verify_arch arm64 "$APP/Contents/Frameworks/libvulkan.1.4.350.dylib"
lipo -verify_arch arm64 "$APP/Contents/Frameworks/libMoltenVK.dylib"
otool -L "$APP/Contents/MacOS/AYAstorm"
otool -D "$APP/Contents/Frameworks/libvulkan.dylib"
otool -D "$APP/Contents/Frameworks/libMoltenVK.dylib"
/usr/libexec/PlistBuddy -c 'Print :CFBundleIconFile' "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP/Contents/Info.plist"

python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); assert d["ICD"]["library_path"] == "../../../Frameworks/libMoltenVK.dylib"; assert d["ICD"]["is_portability_driver"] is True' \
  "$APP/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"
```

期待値:

```text
hdiutil verify: VALID
codesign: valid on disk / satisfies its Designated Requirement
main executable: arm64
libvulkan.dylib -> libvulkan.1.dylib -> libvulkan.1.4.350.dylib: present, arm64 slice present
libMoltenVK.dylib / MoltenVK_icd.json: present, arm64 slice present
ICD.library_path: ../../../Frameworks/libMoltenVK.dylib
ICD.is_portability_driver: true
CFBundleIconFile: ayastorm_icon.icns
CFBundleVersion: 42.0.0.<build-id>
CFBundleShortVersionString: 42.0.0.<build-id>
```

ログイン情報やローカル環境情報が混入していないか確認します。いずれも出力なしであることを確認してください。

```bash
export LOCAL_USER="$(id -un)"

rg -a -l "$LOCAL_USER|/Users/$LOCAL_USER" "$APP/Contents"

find "$APP" \
  -iname 'account_history*' \
  -o -iname 'bin_conf.dat' \
  -o -iname 'cookies*' \
  -o -iname '*credentials*' \
  -o -iname '*saved_password*'
```

DMG 内の app も確認します。

```bash
hdiutil attach -nobrowse -readonly "$DMG"
```

`hdiutil attach` の出力から device と mount point を確認し、以下の `diskX` を置き換えます。

```bash
export DMG_MOUNT="/Volumes/AYAstorm Installer"
export DMG_APP="$DMG_MOUNT/FirestormOS-AYAstorm-release.app"

codesign --verify --deep --strict --verbose=2 "$DMG_APP"
rg -a -l "$LOCAL_USER|/Users/$LOCAL_USER" "$DMG_APP/Contents"
find "$DMG_APP" \
  -iname 'account_history*' \
  -o -iname 'bin_conf.dat' \
  -o -iname 'cookies*' \
  -o -iname '*credentials*' \
  -o -iname '*saved_password*'

md5 "$DMG_MOUNT/.VolumeIcon.icns" \
  "$REPO/indra/newview/icons/ayastorm/ayastorm_icon.icns"

hdiutil detach -force /dev/diskX
```

## arm64実機起動gate

製品相当runではVulkan、validation layer、dylib探索のoverrideに依存していないことを確認します。`DYLD_LIBRARY_PATH`や`DYLD_FRAMEWORK_PATH`を付けた起動はcode signature無効のdylibをbundle外から読み込む危険があるため、合格証拠に使いません。

```bash
env \
  -u VK_DRIVER_FILES \
  -u VK_ICD_FILENAMES \
  -u VK_ADD_DRIVER_FILES \
  -u VK_LAYER_PATH \
  -u VK_ADD_LAYER_PATH \
  -u DYLD_LIBRARY_PATH \
  -u DYLD_FRAMEWORK_PATH \
  -u DYLD_FALLBACK_LIBRARY_PATH \
  -u DYLD_INSERT_LIBRARIES \
  -u DYLD_IMAGE_SUFFIX \
  -u DYLD_ROOT_PATH \
  "$APP/Contents/MacOS/AYAstorm" &
export VIEWER_PID=$!
```

ログイン画面が表示されたら、起動中processが実際にmapしたLoader / MoltenVKのpathを保存します。bare-name loadの成否を`AYAstorm.log`だけから推定しません。

```bash
vmmap "$VIEWER_PID" | tee "$WORK/ayastorm-vmmap.txt"
lsof -p "$VIEWER_PID" | tee "$WORK/ayastorm-lsof.txt"

rg -F "$APP/Contents/Frameworks/libvulkan" "$WORK/ayastorm-vmmap.txt" "$WORK/ayastorm-lsof.txt"
rg -F "$APP/Contents/Frameworks/libMoltenVK.dylib" "$WORK/ayastorm-vmmap.txt" "$WORK/ayastorm-lsof.txt"
```

次を同一binaryで確認し、`VERIFIED / CLAIMED / OPEN`で報告します。

- 全windowにログイン画面が表示され、modalとログインbuttonの左clickが反応する。
- `RenderHiDPI=0 / 1`でUI寸法、mouse hit-test、CAMetalLayer drawable寸法、swapchain extentがそれぞれ収束する。
- 10回以上の連続resize、minimize / restore、fullscreen、異なるscaleのdisplay間移動でrecreate loopにならない。
- ログイン、initial simulator、movement complete、チャット文字描画、正常終了へ到達する。
- `~/Library/Application Support/Firestorm/logs/AYAstorm.log`でportability enumeration/subset、surface / swapchainを確認し、`[VK-ERROR]`、VUID、MoltenVK ERROR、shader compile failure、異常終了を分類する。Loader / MoltenVKの実load pathは上記`vmmap` / `lsof`を証拠にする。

validation診断runは製品相当runと分け、dylib系overrideは引き続き外したまま実行します。

```bash
env \
  -u VK_DRIVER_FILES \
  -u VK_ICD_FILENAMES \
  -u VK_ADD_DRIVER_FILES \
  -u VK_LAYER_PATH \
  -u VK_ADD_LAYER_PATH \
  -u DYLD_LIBRARY_PATH \
  -u DYLD_FRAMEWORK_PATH \
  -u DYLD_FALLBACK_LIBRARY_PATH \
  -u DYLD_INSERT_LIBRARIES \
  -u DYLD_IMAGE_SUFFIX \
  -u DYLD_ROOT_PATH \
  AYASTORM_VK_VALIDATION=1 \
  VK_LOADER_DEBUG=all \
  MVK_CONFIG_LOG_LEVEL=4 \
  "$APP/Contents/MacOS/AYAstorm"
```

上記runでvalidation layerが見つからない場合は合格扱いにせず、固定packageのmanifestだけを明示した別runを実行します。

```bash
export FIXED_LAYER_PATH="$REPO/build-darwin-universal/packages/share/vulkan/explicit_layer.d"
test -f "$FIXED_LAYER_PATH/VkLayer_khronos_validation.json"

env \
  -u VK_DRIVER_FILES \
  -u VK_ICD_FILENAMES \
  -u VK_ADD_DRIVER_FILES \
  -u VK_ADD_LAYER_PATH \
  -u DYLD_LIBRARY_PATH \
  -u DYLD_FRAMEWORK_PATH \
  -u DYLD_FALLBACK_LIBRARY_PATH \
  -u DYLD_INSERT_LIBRARIES \
  -u DYLD_IMAGE_SUFFIX \
  -u DYLD_ROOT_PATH \
  VK_LAYER_PATH="$FIXED_LAYER_PATH" \
  AYASTORM_VK_VALIDATION=1 \
  VK_LOADER_DEBUG=all \
  MVK_CONFIG_LOG_LEVEL=4 \
  "$APP/Contents/MacOS/AYAstorm"
```

`FIXED_LAYER_PATH`、package version、validation layerの有効化logを証拠に残します。validation/MoltenVK ERROR 0件を確認できないrunは合格扱いにしません。

## 署名とローカルパス対策

`Code Signature Invalid` やローカルビルドパス混入を避けるため、Release package では Mach-O のローカルシンボルを落としてから再署名します。

- `indra/cmake/00-Common.cmake`
  - Darwin / Clang で `-ffile-prefix-map`, `-fmacro-prefix-map`, `-fdebug-prefix-map` を指定
- `indra/newview/viewer_manifest.py`
  - package 時に bundle 内 Mach-O へ `strip -S -x` を実行
  - その後に nested app / dylib を再署名

`strip` の途中で code signature が無効になる警告が出ることがありますが、その直後に再署名するため想定内です。配布前には必ず `codesign --verify --deep --strict` を通してください。

## 注意

- DMG ファイル名は Release 向けに `arm64` を含めます。
- アプリ名は `AYAstorm.app` のままです。
- DMG 内の app 名は `FirestormOS-AYAstorm-release.app` です。
- 配布前に notarization を行う場合は、Developer ID と notary 設定を別途確認します。
- 生成済み app を手作業で編集した場合は、必ず再署名と `codesign --verify --deep --strict` を実行します。
