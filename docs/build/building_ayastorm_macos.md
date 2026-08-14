# AYAstorm Mac版ビルド手順

更新日: 2026-08-14

この文書は macOS arm64 向けのビルド手順である。用途を混同しないこと。

- **ローカル開発 app**: 「R42: ローカル arm64 / MoltenVK 開発 app」を使う。DMG は作らない。
- **通常版 / 配布用 DMG**: 「通常版 / 配布用 DMG」以降の配布手順を使う。
- **MoltenVK の通常起動仕様・失敗ログ**:
  [`docs/specs/ayastorm-r42-macos-moltenvk-runtime-bootstrap.md`](../specs/ayastorm-r42-macos-moltenvk-runtime-bootstrap.md)
- **R42 shadow multiview の実機検証**:
  [`docs/guides/ayastorm-r42-macos-arm64-moltenvk-validation.ja.md`](../guides/ayastorm-r42-macos-arm64-moltenvk-validation.ja.md)

出力例: `Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-80834.dmg`

この文書は AYAstorm の macOS arm64 におけるローカル開発 app と配布用 DMG の手順を扱う。Firestorm 本体の一般的な macOS ビルド要件は `doc/building_macos.md` も参照してください。

## 開発版と通常版の分離基準

macOS 実装はビルド時の `PACKAGE` 値ではなく、実行中の executable path に
`/build-darwin-` が含まれるかで開発版を判定する。従って、配布用に構成した app でも
build tree 内から直接起動すれば開発版の可変データを使う。通常版の動作確認は、DMG を
mount してその中の app を起動するか、`/Applications` など build tree 外へインストールした
app で行う。

| 用途 | build tree | Configure / target | 起動場所 | Application Support | cache root |
| --- | --- | --- | --- | --- | --- |
| ローカル開発 | `build-darwin-dev` | `PACKAGE=OFF` / `ayastorm-bin` | build tree 内 | `~/Library/Application Support/AYAstorm-dev/` | `~/Library/Caches/AYAstorm-devOS_x64/` |
| 通常版 / 配布 | `build-darwin-release` | `PACKAGE=ON` / `llpackage` | DMG 内またはインストール後 | `~/Library/Application Support/AYAstorm/` | `~/Library/Caches/AYAstormOS_x64/` |

開発用と配布用で同じ CMake cache、package staging、DerivedData を共有しない。以下の手順では
Autobuild 設定と build tree を用途別に分ける。なお `build-darwin-release` 内の staging app を
直接起動した場合は、表の通常版ではなく開発版として動作する。

## 前提

- macOS 15.x
- Xcode 16.x
- CMake
- Python 3.9 互換の venv
- `autobuild`
- `fs-build-variables`
- FMOD Studio API installer for macOS
- FMOD を有効化する場合は、FMOD package を登録した `my_autobuild.xml`

作業ディレクトリ例:

```bash
export WORK="$HOME/work_ayastorm"
export REPO="$WORK/phoenix-firestorm"
export FS_BUILD_VARIABLES="$WORK/fs-build-variables/variables"
export FMOD_REPO="$WORK/3p-fmodstudio"
export TARGET_REF="feature/macos-arm64-build-on-latest"
export AYA_BUILD_ID="80834"
export BASE_AUTOBUILD_CONFIG="$REPO/my_autobuild.xml"
export DEV_AUTOBUILD_CONFIG="$REPO/my_autobuild-dev.xml"
export RELEASE_AUTOBUILD_CONFIG="$REPO/my_autobuild-release.xml"
export DEV_BUILD_DIR="$REPO/build-darwin-dev"
export RELEASE_BUILD_DIR="$REPO/build-darwin-release"
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

configure 後は `$DEV_BUILD_DIR/CMakeCache.txt` と `$RELEASE_BUILD_DIR/CMakeCache.txt` で、
用途ごとの `LL_DULLAHAN_AUDIO_CALLBACK:BOOL=` の値を確認してください。

## Autobuild 設定と build tree の分離

FMOD、Vulkan SDK などのローカル installable を `my_autobuild.xml` にすべて登録してから、
開発用と配布用の設定を作る。`build_directory` は Autobuild 設定ファイルからの相対 path である。

```bash
cd "$REPO"

cp "$BASE_AUTOBUILD_CONFIG" "$DEV_AUTOBUILD_CONFIG"
cp "$BASE_AUTOBUILD_CONFIG" "$RELEASE_AUTOBUILD_CONFIG"

autobuild edit platform --config-file "$DEV_AUTOBUILD_CONFIG" \
  name=darwin64 build_directory=build-darwin-dev
autobuild edit platform --config-file "$RELEASE_AUTOBUILD_CONFIG" \
  name=darwin64 build_directory=build-darwin-release

autobuild print --config-file "$DEV_AUTOBUILD_CONFIG" --json | \
  rg '"build_directory": "build-darwin-dev"'
autobuild print --config-file "$RELEASE_AUTOBUILD_CONFIG" --json | \
  rg '"build_directory": "build-darwin-release"'
```

3 ファイルはいずれもローカル専用で git 管理しない。installable の URL / hash を変更した場合は
`my_autobuild.xml` を正本として更新し、用途別の 2 ファイルを再生成する。

## R42: ローカル arm64 / MoltenVK 開発 app（DMGなし）

Apple Silicon 用のローカル app は配布用 DMG の手順と分ける。これは Vulkan / MoltenVK の実行確認用であり、Release artifact や notarization の手順ではない。

開発 app は executable path に `/build-darwin-` を含むため、可変データを
`~/Library/Application Support/AYAstorm-dev/`、cache を
`~/Library/Caches/AYAstorm-devOS_x64/`、temp を system temp 配下の `AYAstorm-dev` に作る。
build tree 外の app は通常版の `AYAstorm` / `AYAstormOS_x64` を使う。既存の `Firestorm` profile
および通常版の `AYAstorm` profile / cache は移動・削除しない。

### 実行前提

この手順は、次の macOS runtime 実装が checkout に入っていることを前提とする。

- `indra/cmake/Variables.cmake`: arm64 をデフォルトにし、明示した `CMAKE_OSX_ARCHITECTURES` を尊重する
- `indra/cmake/Vulkan.cmake`: `vulkan_sdk_macos` prebuilt から headers と Loader を解決する
- `indra/cmake/Glslang.cmake`: 同 prebuilt から glslang / SPIRV-Tools を解決する
- `indra/newview/viewer_manifest.py`: Vulkan Loader、MoltenVK、ICD JSON を app bundle に格納する

これらを含まない checkout では、system Vulkan SDK と system glslang を別途用意しなければ clean configure は成功しない。`vulkan_sdk_macos` archive のローカル URL / hash は git-ignored の `my_autobuild.xml` にだけ登録し、tracked `autobuild.xml` へ開発者ローカルの絶対 path を書かない。

### Configure / Build

既存の `build-darwin-dev` が別のコミットやバージョンから作られた場合は再利用しない。以下では package target を作らず、`ayastorm-bin` だけを arm64 でビルドする。

```bash
cd "$REPO"
source .venv/bin/activate

export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
export AUTOBUILD_VARIABLES_FILE="$FS_BUILD_VARIABLES"
export AUTOBUILD_CONFIG_FILE="$DEV_AUTOBUILD_CONFIG"
export CLANG_MODULE_CACHE_PATH="$DEV_BUILD_DIR/ModuleCache"

rm -rf "$DEV_BUILD_DIR"

autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio \
  --openal \
  --chan AYAstorm-r42-phase2 \
  -DCMAKE_OSX_ARCHITECTURES:STRING=arm64 \
  -DPACKAGE:BOOL=OFF \
  -DLL_TESTS:BOOL=FALSE \
  -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE

rg -n 'CMAKE_BUILD_TYPE|CMAKE_OSX_ARCHITECTURES|VIEWER_CHANNEL|PACKAGE|USE_FMODSTUDIO|LL_DULLAHAN_AUDIO_CALLBACK' \
  "$DEV_BUILD_DIR/CMakeCache.txt"

DEVELOPER_DIR="$DEVELOPER_DIR" \
AUTOBUILD_VARIABLES_FILE="$AUTOBUILD_VARIABLES_FILE" \
AUTOBUILD_CONFIG_FILE="$AUTOBUILD_CONFIG_FILE" \
CLANG_MODULE_CACHE_PATH="$CLANG_MODULE_CACHE_PATH" \
xcodebuild \
  -project "$DEV_BUILD_DIR/Firestorm.xcodeproj" \
  -scheme ayastorm-bin \
  -configuration Release \
  -derivedDataPath "$DEV_BUILD_DIR/DerivedData" \
  ARCHS=arm64 \
  ONLY_ACTIVE_ARCH=YES \
  build
```

期待値は `CMAKE_OSX_ARCHITECTURES:STRING=arm64`、`PACKAGE:BOOL=OFF`、および `build-darwin-dev/newview/Release/AYAstorm.app` である。`llpackage` / DMG はこの手順の対象外とする。

### ローカル app の検証

```bash
export APP="$DEV_BUILD_DIR/newview/Release/AYAstorm.app"

lipo -archs "$APP/Contents/MacOS/AYAstorm"
codesign --verify --deep --strict --verbose=2 "$APP"
test -e "$APP/Contents/Frameworks/libvulkan.dylib"
test -e "$APP/Contents/Frameworks/libMoltenVK.dylib"
test -f "$APP/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"
```

製品相当の起動確認では `VK_ICD_FILENAMES`、`VK_LAYER_PATH`、`DYLD_LIBRARY_PATH`、`DYLD_INSERT_LIBRARIES`、`AYASTORM_VKCMD_MEMO` などの override を使わない。override を用いた診断 run は、通常起動とは別の証拠として扱う。

通常起動後は `~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log` を確認する。`initialized device=...`、`Vulkan presentation surface initialized`、`Initializing Login Screen` がこの順で記録され、ログイン画面が全面に描画されることを確認する。詳細な合格条件と cache 復旧ログは [r42 MoltenVK 実行時ブートストラップ仕様](../specs/ayastorm-r42-macos-moltenvk-runtime-bootstrap.md) に従う。

```bash
export DEV_LOG="$HOME/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log"

test -d "$HOME/Library/Caches/AYAstorm-devOS_x64"
rg 'setSoundCacheDir.*AYAstorm-devOS_x64' "$DEV_LOG"
```

ここで通常版の `~/Library/Application Support/AYAstorm/` や
`~/Library/Caches/AYAstormOS_x64/` に新しいログ・cache が作られた場合は、開発版の分離確認を
合格にしない。

## 通常版 / 配布用 DMG

ここからは開発用 build tree を使わず、配布専用の `build-darwin-release` を構成する。

### 環境変数

```bash
cd "$REPO"

export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
export AUTOBUILD_BUILD_ID="$AYA_BUILD_ID"
export AUTOBUILD_VARIABLES_FILE="$FS_BUILD_VARIABLES"
export AUTOBUILD_CONFIG_FILE="$RELEASE_AUTOBUILD_CONFIG"
export CLANG_MODULE_CACHE_PATH="$RELEASE_BUILD_DIR/ModuleCache"
```

### Configure

クリーンに作り直す場合:

```bash
cd "$REPO"
rm -rf "$RELEASE_BUILD_DIR"
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
  "$RELEASE_BUILD_DIR/CMakeCache.txt"
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
VIEWER_CHANNEL:STRING=Firestorm-AYAstorm-release
LL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE
```

### Build / Package

`llpackage` scheme を Release で実行します。

```bash
cd "$RELEASE_BUILD_DIR"

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
  -derivedDataPath "$RELEASE_BUILD_DIR/DerivedData" \
  CLANG_MODULE_CACHE_PATH="$CLANG_MODULE_CACHE_PATH" \
  build
```

DMG 作成時に `hdiutil create` が `装置が構成されていません` で失敗する場合は、サンドボックスや権限の制約でディスクイメージ操作が止まっています。同じコマンドを通常の Terminal から実行してください。

### 成果物

```bash
export DMG="$RELEASE_BUILD_DIR/newview/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-${AYA_BUILD_ID}.dmg"
export APP="$RELEASE_BUILD_DIR/newview/Release/AYAstorm.app"

ls -lh "$DMG"
```

成果物の例:

```text
build-darwin-release/newview/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-80834.dmg
```

### 検証

ローカル app / DMG の基本検証:

```bash
hdiutil verify "$DMG"
codesign --verify --deep --strict --verbose=2 "$APP"
lipo -archs "$APP/Contents/MacOS/AYAstorm"
/usr/libexec/PlistBuddy -c 'Print :CFBundleIconFile' "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP/Contents/Info.plist"
```

ここで `$APP` は bundle 構成の静的検証にだけ使う。`$APP` の executable path は
`/build-darwin-` を含むため、直接起動すると開発版 profile / cache を使う。通常版の runtime
分離は、以下の DMG 内 app または `/Applications` へインストールした app で確認する。

期待値:

```text
hdiutil verify: VALID
codesign: valid on disk / satisfies its Designated Requirement
lipo: arm64
CFBundleIconFile: ayastorm_icon.icns
CFBundleVersion: 7.2.4.80834
CFBundleShortVersionString: 7.2.4.80834
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
```

DMG 内の app を起動して通常版の runtime 分離を確認する。開発版のログや cache をこの確認の
代用にしない。

```bash
"$DMG_APP/Contents/MacOS/AYAstorm"

export NORMAL_LOG="$HOME/Library/Application Support/AYAstorm/logs/AYAstorm.log"
test -f "$NORMAL_LOG"
test -d "$HOME/Library/Caches/AYAstormOS_x64"
rg 'setSoundCacheDir.*AYAstormOS_x64' "$NORMAL_LOG"
```

確認後、`hdiutil attach` の出力で得た device 名を指定して detach する。

```bash
hdiutil detach -force /dev/diskX
```

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
