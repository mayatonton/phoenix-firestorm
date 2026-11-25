# AYAstorm r31-bugfix-2 メニュー全階層

AYAviewer UI 設計用の現状資料。対象はリリースタグ `v7.2.4-ayastorm-r31-bugfix-2`（`214054a5a71619c83a9802d37a185f6bc44c6908`）の `skins/default/xui/en/menu_*.xml`。日本語表示名は同タグの `xui/ja` に対応名がある場合だけ採用し、対応がない項目は英語を残した。

この階層は XML 定義を列挙した静的資料。実画面ではログイン状態、権限、グリッド、RLVa、選択対象、設定、実行時追加・削除によって表示と操作可否が変わる。「初期非表示」はそのノードの XML 属性、「表示条件あり」「有効条件あり」はコールバックの存在を示す。親が非表示なら子も表示されない。セパレーター、コールバック要素、汎用ウィジェット定義 6 件は省いた。

設計上の機能整理は [作業台](ayaviewer-ui-ia-working-map-r31.ja.md)、階層の検索・展開は [探索図](ayaviewer-ui-menu-explorer-r31.html)、経路と処理の機械検索は [CSV](ayaviewer-ui-menu-items-r31.csv) を参照。

## 通常画面のメニューバー

### `menu_viewer.xml`

定義: `indra/newview/skins/default/xui/en/menu_viewer.xml`

- Main Menu
  - アバター / Avatar
    - マイアカウント / Account [表示条件あり]
    - マーケティングプレイスの出品リスト… / Marketplace listings...
    - L$ の購入 / Buy L$
    - [Membership] [表示条件あり]
    - インベントリ / Inventory [切替]
    - 新着のインベントリのウィンドウ / New Inventory Window [初期非表示]
    - 保護フォルダ / Protected Folders [切替]
    - お気に入りの着用物やＨＵＤ / Favorite Wearables [切替]
    - ピック… / Picks
    - 体験… / Experiences
    - プロフィール / Profile
    - 着用中… / Now wearing... [有効条件あり]
    - アウトフィット / Outfits [切替・有効条件あり]
    - 脱ぐ / Take off
      - 衣装 / Clothes
        - シャツ / Shirt [有効条件あり]
        - ズボン / Pants [有効条件あり]
        - スカート / Skirt [有効条件あり]
        - 靴 / Shoes [有効条件あり]
        - 靴下 / Socks [有効条件あり]
        - ジャケット / Jacket [有効条件あり]
        - 手袋 / Gloves [有効条件あり]
        - 下着（上半身） / Undershirt [有効条件あり]
        - 下着（下半身） / Underpants [有効条件あり]
        - タトゥ / Tattoo [有効条件あり]
        - 物理作用 / Physics [有効条件あり]
        - アルファ / Alpha [有効条件あり]
        - すべての衣装 / All Clothes
      - ＨＵＤ / HUD
      - 取り外す / Detach
      - すべてを取り外す / Detach All [有効条件あり]
    - 選択した装着物を削除 / Remove selected attachments [有効条件あり]
    - アバターウェルカムパック… / Avatar Welcome Pack... [表示条件あり]
    - ホバーする高さ / Hover Height [初期非表示・有効条件あり]
    - 移動 / Movement
      - 座る / Sit Down [表示条件あり・有効条件あり]
      - 立つ / Stand Up [表示条件あり・有効条件あり]
      - 飛ぶ / Fly [切替・有効条件あり]
      - 着地する / Stop flying [有効条件あり]
      - 常に走る / Always Run [切替]
      - 地面に座る / Force Ground Sit [切替・有効条件あり]
      - 移動ロック / Movelock [切替・有効条件あり]
      - アバターを身軽に / Quickjump [切替]
      - Face nearest avatar
      - Avatar alignment [切替]
    - 移動操作 / Move Controls [切替]
    - カメラ操作 / Camera Controls [切替]
    - アバターの状態 / Avatar Health
      - アバターのアニメーションを停止する / Stop Avatar Animations
      - アバターのアニメーションを停止＆権限を取り消す / Stop Avatar Animations & Revoke Permissions
      - アバターの変形を解除 / Undeform Avatar
      - スケルトンをリセット / Reset skeleton
      - スケルトンとアニメーションのリセット / Reset skeleton and animations
      - テクスチャをリベークする / Force Appearance Update (Rebake)
      - 装着物をリフレッシュ / Refresh Attachments
      - アバターを男性のデフォルトにリセット / Reset Default Male Avatar (Character Test)
      - アバターを女性のデフォルトにリセット / Reset Default Female Avatar (Character Test)
      - アバターの複雑度を表示 / Show Avatar Complexity Information [切替]
      - 装着されたスクリプト… / Scripts
      - 遅延メーター / Lag Meter
      - ＬＳＬブリッジを再作成 / Recreate LSL Bridge
    - スナップショット / Snapshot
    - ３６０度スナップショット / 360° Snapshot
    - 取引ログ / Money Tracker
    - ポーズスタンド… / Pose Stand... [有効条件あり]
    - ポーザー / Poser [切替]
    - 初期設定 / Preferences
    - ツールバーボタン / Toolbar Buttons
    - ＨＵＤを表示 / Show HUD Attachments [切替・有効条件あり]
    - インターフェースを表示 / Show User Interface [切替]
    - 管理者権限のリクエスト / Request Admin Status [初期非表示]
    - 管理者ステータスを解除する / Leave Admin Status [初期非表示]
    - [APP_NAME]を終了 / Exit [APP_NAME]
  - コミュニケーション / Comm
    - ログイン状態 / Online Status
      - 一時退席中 / Away [切替]
      - 取り込み中 / Unavailable [切替]
      - 自動応答を有効化 / Autorespond [切替]
      - 非フレンドへの自動応答を有効化 / Autorespond to non-friends [切替]
      - テレポートのオファーとリクエストを拒否する / Reject teleport offers and requests [切替]
      - すべてのグループの招待を拒否する / Reject all group invites [切替]
      - フレンドシップのリクエストを拒否する / Reject all friendship requests [切替]
      - 会議（アドホック）チャット / Ad-hoc conferences
        - すべてを自動的に無視して退出する / Automatically ignore and leave all [切替]
        - 近くのチャットで無視を報告する / Report ignored in nearby chat [切替・有効条件あり]
        - フレンドからは無視しない / Don't ignore from my friends [切替・有効条件あり]
    - フレンド / Friends
    - コンタクト / Contacts [切替]
    - コンタクトセット / Contact Sets
    - グループ / Groups
    - チャット… / Chat... [切替]
    - 人物 / People [切替]
    - 会話 / Conversations [切替]
    - 包括フィルタ / Omnifilter [切替]
    - ジェスチャー / Gestures [切替]
    - Flickr… / Flickr...
    - Primfeed… / Primfeed...
    - Discord… / Discord...
    - 会話ログ… / Conversation Log... [切替・有効条件あり]
    - 近くのボイス / Nearby Voice [切替]
    - ブロックリスト / Block List
    - オンスクリーンチャットコンソールを表示 / Show On-screen Chat Console [切替]
  - ワールド / World
    - アニメーションの再同期 / Resync Animations
    - 近くにいる人物 / Nearby Avatars
    - レーダー / Radar [切替]
    - テレポート履歴 / Teleport History
    - 場所 / Places [切替]
    - 行き先… / Destinations [表示条件あり]
    - イベント / Events [表示条件あり]
    - ミニマップ / Mini-Map [切替]
    - ワールドマップ / World Map [切替]
    - リージョントラッカー / Region Tracker [切替]
    - ストリームタイトル / Stream Title [切替]
    - 現在地のランドマークを作成 / Landmark this Place [有効条件あり]
    - 場所のプロフィール / Location Profile [有効条件あり]
    - 土地情報 / Parcel Details [有効条件あり]
    - リージョン／不動産 / Region Details [有効条件あり]
    - 現在地をホームとして設定 / Set Home to Here [有効条件あり]
    - この土地を購入する / Buy this Land [有効条件あり]
    - 自分の土地 / Show Owned Land
    - リンデンホーム… / My Linden Home... [表示条件あり]
    - 表示 / Show More
      - バンラインを隠す / Hide Ban Lines [切替]
      - バンラインにぶつかったときに表示 / Show Ban Lines On Collision [切替]
      - バンラインに近づいたときに表示 / Show Ban Lines On Proximity [切替]
      - ビーコン / Beacons [切替]
      - 物件境界線 / Property Lines [切替]
      - 土地所有者 / Land Owners [切替]
      - 座標 / Coordinates [切替]
      - 区画のプロパティ / Parcel Permissions [切替]
      - 高度な操作メニューを表示する / Advanced Menu [切替]
    - グラフィック性能の最適化… / Improve graphics speed... [切替]
    - ホームにテレポート / Teleport Home [有効条件あり]
    - 自然環境 / Environment
      - 朝方 / Sunrise [切替・有効条件あり]
      - 昼間 / Midday [切替・有効条件あり]
      - 昼間（レガシー） / Midday (Legacy) [切替・有効条件あり]
      - 夕方 / Sunset [切替・有効条件あり]
      - 夜中 / Midnight [切替・有効条件あり]
      - 共有された環境を使用 / Use Shared Environment [切替・有効条件あり]
      - 自分の環境… / My Environments... [有効条件あり]
      - 環境の個人設定… / Personal Lighting... [有効条件あり]
      - 雲の動きを一時停止 / Pause Clouds [切替・有効条件あり]
      - 一括インポート / Bulk Import
        - デイサイクル… / Days... [有効条件あり]
        - 空の設定… / Skies... [有効条件あり]
        - 水面の設定… / Water... [有効条件あり]
    - フォトとビデオ / Photo and Video
      - フォトツール / Phototools
      - カメラツール / Cameratools
      - ＤｏＦ焦点をロック / Depth of Field Focus lock [切替]
      - ＤｏＦ焦点にクロスヘアーを描画 / Draw DoF Focus crosshair [切替]
    - エリアサーチ / Area Search
    - サウンドエクスプローラー / Sound Explorer
    - アニメーションエクスプローラー / Animation Explorer
    - 非表示のアイテム一覧 / Asset Blacklist
    - アバターの表示設定 / Avatar Render Settings
    - アバター表示 / Avatar Display
      - 複雑度で制限 / Limit by Complexity [切替]
      - フレンドは常に表示 / Always show Friends [切替]
      - フレンドのみ表示 / Only show Friends [切替]
    - フレンドのみ表示 / Show Friends only [切替]
  - ビルド / Build
    - ビルドツールを表示 / Build [切替・有効条件あり]
    - ビルドツールを選択する / Select Build Tool
      - フォーカスツール / Focus Tool
      - 移動ツール / Move Tool
      - 編集ツール / Edit Tool
      - 作成ツール / Create Tool
      - 土地ツール / Land Tool
    - リンク / Link [有効条件あり]
    - リンク解除 / Unlink [有効条件あり]
    - リンクされた部分を編集 / Edit Linked Parts [切替・有効条件あり]
    - 要素を選択 / Select Elements
      - 次のパーツまたは面を選択 / Select Next Part or Face [有効条件あり]
      - 前のパーツまたは面を選択 / Select Previous Part or Face [有効条件あり]
      - 次のパーツまたは面を含める / Include Next Part or Face [有効条件あり]
      - 前のパーツまたは面を含める / Include Previous Part or Face [有効条件あり]
    - 選択をフォーカス / Focus on Selection [有効条件あり]
    - 選択をズーム / Zoom to Selection [有効条件あり]
    - オブジェクト / Object
      - 購入 / Buy [表示条件あり・有効条件あり]
      - 取得 / Take [表示条件あり・有効条件あり]
      - コピーを取得 / Take Copy [有効条件あり]
      - 複製 / Duplicate
      - パーティクルの編集 / Edit Particles [有効条件あり]
      - オブジェクトの中身に保存 / Save Back to Object Contents [有効条件あり]
      - オブジェクトを返却 / Return Object [有効条件あり]
      - 別名で保存 / Save as
        - バックアップを作成 / Backup [有効条件あり]
        - メッシュとしてエクスポート / Collada [有効条件あり]
    - スクリプト / Scripts
      - スクリプトの警告／エラーを表示 / Show Script Warnings/Errors
      - スクリプト情報（カウンター） / Script Info (Counter) [有効条件あり]
      - スクリプトの再コンパイル（Ｍｏｎｏ） / Recompile Scripts (Mono) [有効条件あり]
      - スクリプトの再コンパイル（ＬＳＬ） / Recompile Scripts (LSL) [有効条件あり]
      - スクリプトのリセット / Reset Scripts [有効条件あり]
      - スクリプトを実行中にする / Set Scripts to Running [有効条件あり]
      - スクリプトを実行停止にする / Set Scripts to Not Running [有効条件あり]
      - 選択からスクリプトを削除する / Remove Scripts From Selection [有効条件あり]
    - 経路探索 / Pathfinding
      - リージョンのオブジェクト / Region Objects [有効条件あり]
      - キャラクター… / Characters... [有効条件あり]
      - 表示／テスト… / View / test... [有効条件あり]
      - リージョンの再構築 / Rebake region [有効条件あり]
    - オプション / Options
      - 権限の詳細を表示する / Show Advanced Permissions [切替]
      - 自分のオブジェクトだけを選択する / Select Only My Objects [切替]
      - 動的オブジェクトだけを選択する / Select Only Movable Objects [切替]
      - ロックされたオブジェクトのみ選択する / Select Only Locked Objects [切替]
      - コピー可能なオブジェクトだけを選択する / Select Only Copyable Objects [切替]
      - 非表示のオブジェクトを選択する / Select Invisible Objects [切替]
      - 反射プローブを選択する / Select Reflection Probes [切替]
      - 範囲内を選択する / Select By Surrounding [切替]
      - グループ所有のオブジェクトを含む / Include Group-Owned Objects [切替]
      - 編集時に物理的形状を表示する / Show Physics Shape When Editing [切替]
      - 選択のアウトラインを表示する / Show Selection Outlines [切替]
      - 隠れた位置の選択も表示する / Show Hidden Selection [切替]
      - 選択した光の効果範囲を表示する / Show Light Radius for Selection [切替]
      - 反射プローブの影響範囲を表示する / Show Reflection Probe Volumes [切替]
      - 選択ビームを表示する / Show Selection Beam [切替]
      - 透過をハイライトする / Highlight Transparent [切替]
      - －透過リグを含む / - include Rigged Transparent [切替]
      - Highlight Transparent Probes [切替・有効条件あり]
      - 3D ストリームの遮蔽プリムを表示する (AYAstorm) / Show 3D Stream Occluders (AYAstorm) [切替]
      - ポストプロセッシングを無効化 / No Post-processing [切替]
      - ＬｏＤ（詳細レベル）の選択 / Selection level of detail
        - デフォルト / Default [切替]
        - 高 / High [切替]
        - 中 / Medium [切替]
        - 低 / Low [切替]
        - 最低 / Lowest [切替]
      - グリッド吸着 / Snap to Grid [切替・有効条件あり]
      - オブジェクトのＸＹ軸をグリッドに吸着させる / Snap Object XY to Grid [有効条件あり]
      - 選択をグリッドに使用する / Use Selection for Grid [有効条件あり]
      - グリッドオプション… / Grid Options... [有効条件あり]
      - デフォルトの権限設定… / Set Default Permissions...
    - アップロード / Upload
      - 画像（L$ [COST]）… / Image... [表示条件あり・有効条件あり]
      - サウンド（L$ [COST]）… / Sound (L$[COST])... [表示条件あり・有効条件あり]
      - アニメーション（L$ [COST]）… / Animation (L$[COST])... [表示条件あり・有効条件あり]
      - モデル… / Mesh Model... [表示条件あり・有効条件あり]
      - マテリアル… / Material... [有効条件あり]
      - 一括… / Bulk... [表示条件あり]
      - リンクセットをインポート / Import Linkset...
    - ローカルメッシュ / Local Mesh
    - 取り消し / Undo [有効条件あり]
    - やり直し / Redo [有効条件あり]
  - コンテンツ / Content
    - 検索 / Search [切替]
    - ＳＬマーケットプレイス / SL Marketplace [表示条件あり]
    - L$マーケットデータ / L$ Market Data [表示条件あり]
    - スクリプトライブラリ / Script Library
    - ＳＬコミュニティ（ブログ、フォーラム、ニュース） / SL Community (blogs, forums, news)
    - Firestormブログ / Firestorm Blog
    - Firestorm Primfeed
    - Firestorm Flickr
    - Firestorm YouTube
    - Firestorm X / Firestorm Twitter
    - Firestorm Plurk
    - 本日のメッセージ / Message of the day
  - AYAstorm
    - AYAstorm Controls...
  - ヘルプ / Help
    - ヒントを有効にする / Enable Viewer UI Hints [切替]
    - Firestorm Wiki
    - 問題の解決 / Troubleshooting
    - Firestormサポートグループに参加 / Join Firestorm Support Group [表示条件あり]
    - Firestorm勉強会スケジュール / Firestorm Classes Schedule
    - Firestormイベントカレンダー / Firestorm Events Calendar
    - Guidebook [切替]
    - [CURRENT_GRID]ヘルプ / [CURRENT_GRID] Help
    - [CURRENT_GRID]について / About [CURRENT_GRID]
    - ホワイトリストアドバイザー / Whitelist adviser
    - Check Grid status [切替]
    - 嫌がらせの報告 / Report Abuse
    - バグの報告 / Report Problem
    - 衝突・プッシュ＆打撃 / Bumps, Pushes & Hits
    - システム情報ボタンを有効化 / Enable Sysinfo Button [切替]
    - [APP_NAME]について / About [APP_NAME]
  - ＲＬＶａ / RLVa
    - デバッグ / Debug
      - トップレベルのＲＬＶａメニューを表示 / Show Top-level RLVa Menu [切替]
      - デバッグメッセージを表示 / Show Debug Messages [切替]
      - 未設定または重複メッセージを隠す / Hide Unset or Duplicate Messages [切替]
      - アサーションの失敗を表示 / Show Assertion Failures [切替]
      - ロックされたレイヤーを隠す / Hide Locked Layers [切替]
      - ロックされた装着物を隠す / Hide Locked Attachments [切替]
      - 古いネーミングを有効化 / Enable Legacy Naming [切替]
      - 共有された衣装を有効化 / Enable Shared Wear [切替]
      - 共有された着用中のアイテムの名前変更 / Rename Shared Items on Wear [切替]
      - ロック… / Locks... [切替]
    - ＯＯＣ（その人らしくない）チャットの許可 / Allow OOC Chat [切替]
    - フィルタリングされたチャットを表示 / Show Filtered Chat [切替]
    - リダイレクトされたチャット入力を表示 / Show Redirected Chat Typing [切替]
    - リダイレクトされた長文チャット分割 / Split Long Redirected Chat [切替]
    - 一時的な装着物を許可 / Allow Temporary Attachments [切替]
    - #RLVへの受け渡しを禁止 / Forbid Give to #RLV [切替]
    - 着用物の交換をアンロック / Wear Replaces Unlocked [切替]
    - コンソール… / Console... [切替]
    - 制限… / Restrictions... [切替]
    - 文章… / Strings... [切替]
  - 高度な操作 / Advanced [初期非表示]
    - テクスチャをリベークする / Rebake Textures
    - 装着物をリフレッシュ / Refresh Attachments
    - ＵＩサイズをデフォルトにする / Set UI Size to Default
    - ウィンドウサイズ設定… / Set Window Size...
    - オブジェクトの選択距離の制限 / Limit Select Distance [切替]
    - カメラの距離制限を解除 / Disable Camera Constraints [切替]
    - 高解像度スナップショット / High-res Snapshot [切替]
    - シャッター音を出さずにスナップショットをディスクに保存 / Quiet Snapshots [切替]
    - パフォーマンスツール / Performance Tools
      - 遅延メーター / Lag Meter
      - 統計バー / Statistics Bar [切替]
      - Scene Load Statistics [切替]
      - グラフィック性能の最適化… / Improve graphics speed... [切替]
      - アバターの複雑度を表示 / Show avatar complexity information [切替]
    - ハイライトと表示 / Highlighting and Visibility
      - ビーコンを簡素化 / Cheesy Beacon [切替]
      - パーティクルを非表示 / Hide Particles [切替]
      - 選択を非表示 / Hide Selected [切替]
      - マウスルックのときにクロスヘアーを表示 / Show Mouselook Crosshairs [切替]
      - マウスホバー時のツールチップ / Hover Tips
        - ツールチップを表示 / Show Tips [切替]
        - 土地のツールチップを表示 / Show Land Tooltips [切替・有効条件あり]
        - すべてのオブジェクトのツールチップを表示 / Show Tips On All Objects [切替・有効条件あり]
    - レンダリング種別 / Rendering Types
      - シンプル / Simple [切替]
      - マテリアル / Materials [切替]
      - アルファマスク / Alpha Mask [切替]
      - 明るさ全開のアルファマスク / Fullbright Alpha Mask [切替]
      - グロー / Glow [切替]
      - 明るさ全開 / Fullbright [切替]
      - アルファ / Alpha [切替]
      - 樹木 / Tree [切替]
      - アバター / Avatars [切替]
      - アニメッシュ / Animeshes [切替]
      - サーフェスパッチ / Surface Patch [切替]
      - 空 / Sky [切替]
      - 水面 / Water [切替]
      - ボリューム / Volume [切替]
      - 草 / Grass [切替]
      - 雲 / Clouds [切替]
      - パーティクル / Particles [切替]
      - 衝突 / Bump [切替]
      - ＰＢＲ / PBR [切替]
    - レンダリング機能 / Rendering Features
      - ＵＩ / UI [切替]
      - 選択済 / Selected [切替]
      - ハイライト / Highlighted [切替]
      - 動的テクスチャ / Dynamic Textures [切替]
      - 足の影 / Foot Shadows [切替]
      - フォグ / Fog [切替]
      - FRInfoのテスト / Test FRInfo [切替]
      - フレキシブルオブジェクト / Flexible Objects [切替]
    - ＲＬＶａ / RLVa
    - メディアストリームのバックアップ / Media Streams Backup
      - ストリームリストのＸＭＬをインポート… / Import Stream List XML...
      - ストリームリストのＸＭＬをエクスポート… / Export Stream List XML...
    - プラグイン読み取りスレッドの使用 / Use Plugin Read Thread [切替]
    - グループキャッシュのクリア / Clear Group Cache
    - マウスの平滑化 / Mouse Smoothing [切替]
    - リリースキー / Release Keys [有効条件あり]
    - ショートカット / Shortcuts [初期非表示]
      - 検索 / Search [切替]
      - ダブルクリックでテレポート / DoubleClick Teleport [切替]
      - 常に走る / Always Run [切替]
      - 飛ぶ / Fly [切替・有効条件あり]
      - ウィンドウを閉じる / Close Window [有効条件あり]
      - ウィンドウグループを閉じる / Close Window Group [有効条件あり]
      - 全てのウィンドウを閉じる / Close All Windows [有効条件あり]
      - スナップショットをディスクに保存する / Snapshot to Disk
      - マウスルック / Mouselook [有効条件あり]
      - ジョイスティック・フライカム / Joystick Flycam [切替・有効条件あり]
      - 視界をリセット / Reset View
      - カメラアングルをリセット / Reset Camera Angles
      - 最後の発言者を見る / Look at Last Chatter [有効条件あり]
      - ズーム・イン / Zoom In
      - ズーム（デフォルト） / Zoom Default
      - ズーム・アウト / Zoom Out
    - 飛行オーバーライド / Fly Override [切替]
    - RestrainedLove API（ＲＬＶａ）を有効化 / RestrainedLove API [切替・表示条件あり]
    - デバッグ設定を表示 / Show Debug Settings
    - 開発メニューを表示 / Show Developer Menu [切替]
  - 開発 / Developer [初期非表示]
    - CheckTexture [表示条件あり・有効条件あり]
    - CheckTexture (glTF) [表示条件あり・有効条件あり]
    - コンソール / Consoles
      - テクスチャのコンソール / Texture Console [切替]
      - デバッグコンソール / Debug Console [切替]
      - 通知コンソール / Notifications Console
      - ファーストタイマー / Fast Timers [切替]
      - メモリ / Memory [切替]
      - シーンの統計 / Scene Statistics [切替]
      - シーンの読み込みモニター / Scene Loading Monitor [切替]
      - リージョンのデバッグコンソール / Region Debug Console [切替]
      - リージョン情報をデバッグコンソールへ / Region Info to Debug Console
      - グループ情報をデバッグコンソールへ / Group Info to Debug Console
      - 性能情報をデバッグコンソールへ / Capabilities Info to Debug Console
      - カメラ / Camera [切替]
      - 風 / Wind [切替]
      - 視野角（ＦｏＶ） / FOV [切替]
      - バッジ / Badge [切替]
      - クッキー / Cookies [切替・初期非表示]
    - 情報を表示 / Show Info
      - 時間を表示する / Show Time [切替]
      - アップロードトランザクションを表示 / Show Upload Transaction [切替]
      - テクスチャ情報を表示 / Show Texture Info [切替]
      - オブジェクトごとのＶＲＡＭ使用率 / VRAM usage per object
      - アバターの描画情報を表示 / Show Avatar Render Info [切替]
      - 描画情報を表示する / Show Render Info [切替]
      - マトリックスを表示する / Show Matrices [切替]
      - カーソルを乗せた場所の色を表示する / Show Color Under Cursor [切替]
      - メモリを表示 / Show Memory [切替]
      - オブジェクトの更新を表示する / Show Updates to Objects [切替]
    - プロファイリング／テレメトリ / Profiling/Telemetry
      - プロファイリング / Profiling [切替・有効条件あり]
      - 接続時に開始 / Start when connected [切替]
    - エラー実行 / Force an Error
      - ブレークポイント / Force Breakpoint
      - LLErrorとクラッシュを実行する / Force LLError And Crash
      - LLErrorとメッセージとクラッシュを実行する / Force LLError, Message And Crash
      - 不正なメモリアクセスを実行する / Force Bad Memory Access
      - コルーチン内で不正なメモリアクセスを発生させる / Force Bad Memory Access in Coroutine
      - 無限ループを発生させる / Force Infinite Loop
      - ドライバのクラッシュを発生させる / Force Driver Crash
      - ソフトウェア例外エラーの発生させる / Force Software Exception
      - ＯＳの例外を発生させる / Force OS Exception [表示条件あり]
      - コルーチンでソフトウェア例外を強制する / Force Software Exception in Coroutine
      - 並列処理でクラッシュを発生させる / Force a Crash in a Coroprocedure
      - ワークキューでクラッシュを発生させる / Force a Crash in a Work Queue
      - スレッド内でLLErrorクラッシュを発生させる / Force an LLError Crash in a Thread
      - スレッド内で例外クラッシュを発生させる / Force an Exception Crash in a Thread
      - ビューアの接続遮断を実行する / Force Disconnect Viewer
      - メモリリークをシミュレートする / Simulate a Memory Leak
    - ＧＬＴＦ / GLTF
      - 開く… / Open...
      - 名前をつけて保存… / Save As... [有効条件あり]
      - アップロード… / Upload... [有効条件あり]
      - 編集… / Edit... [有効条件あり]
    - ビデオ / Video
      - 1080✕1920 / 1080x1920
      - 1920✕1080 / 1920x1080
      - 1280✕720 / 1280x720
      - 720✕1280 / 720x1280
    - 描画テスト / Render Tests
      - カメラのオフセット / Camera Offset [切替]
      - フレームレートをランダム化 / Randomize Framerate [切替]
      - 周期的な遅いフレーム / Periodic Slow Frame [切替]
      - フレームのプロフィール / Frame Profile
      - ベンチマーク / Benchmark
      - ＨＤＲＩプレビュー / HDRI Preview
    - メタデータの描画 / Render Metadata
      - バウンディングボックス / Bounding Boxes [切替]
      - アバターのヒットボックス / Avatar Hitboxes [切替]
      - 普通 / Normals [切替]
      - 八分木 / Octree [切替]
      - ＧＬＴＦノード / GLTF Nodes [切替]
      - シャドウ円錐 / Shadow Frusta [切替]
      - 物理的形状 / Physics Shapes [切替]
      - オクルージョン / Occlusion [切替]
      - バッチの描画 / Render Batches [切替]
      - タイプを更新 / Update Type [切替]
      - テクスチャのアニメ / Texture Anim [切替]
      - テクスチャ優先度 / Texture Priority [切替]
      - テクスチャの範囲 / Texture Area [切替]
      - 側面 / Face Area [切替]
      - ＬｏＤ情報 / LOD Info [切替]
      - 三角形の数 / Triangle Count [切替]
      - 光源 / Lights [切替]
      - 反射プローブ / Reflection Probes [切替]
      - プローブの更新 / Probe Updates [切替]
      - パーティクル / Particles [切替]
      - スケルトンの衝突判定 / Collision Skeleton [切替]
      - ジョイント / Joints [切替]
      - レイキャスト / Raycast [切替]
      - 風のベクトル / Wind Vectors [切替]
      - スカルプト / Sculpt [切替]
      - テクスチャサイズ / Texture Size [切替]
      - テクスチャの密度 / Texture Density
        - 無し / None [切替]
        - 現行 / Current [切替]
        - 望ましい / Desired [切替]
        - 全て / Full [切替]
    - レンダリング / Rendering
      - 軸 / Axes [切替]
      - 接線基準 / Tangent Basis [切替]
      - 選択されたテクスチャ情報基準 / Selected Texture Info Basis
      - 選択したマテリアルの情報 / Selected Material Info
      - ワイヤーフレーム / Wireframe [切替]
      - オブジェクト間のオクルージョン / Object-Object Occlusion [切替]
      - 太陽・月・プロジェクタからの影 / Shadows from Sun/Moon/Projectors [切替・有効条件あり]
      - ＳＳＡＯと影のスムージング / SSAO and Shadow Smoothing [切替・有効条件あり]
      - 次回起動時にＧＬをデバッグ / Start Debug GL on next run [切替]
      - パイプラインをデバッグ / Debug Pipeline [切替]
      - 自動アルファマスク（遅延） / Automatic Alpha Masks [切替]
      - アニメーションテクスチャ / Animation Textures [切替]
      - テクスチャを無効にする / Disable Textures [切替]
      - 全てのアニメッシュの描画解除 / Derender all animesh
      - フル解像度のテクスチャ（危険） / Full Res Textures (dangerous) [切替]
      - 装着された光源を描画する / Render Attached Lights [切替]
      - 取り付けられたパーティクルを描画する / Render Attached Particles [切替]
      - Render Animated Avatar System Cloth [切替]
      - ＶＲＡＭに余裕がないときに描画距離を抑制する / Reduce Draw Distance when VRAM is full [切替]
      - フォント頂点バッファを収集する / Collect Font Vertex Buffers [切替]
      - シェーダーキャッシュを有効化 / Enable Shader Cache [切替]
      - シェーダーキャッシュをクリア / Clear Shader Cache
    - ネットワーク / Network
      - エージェントを一時停止する / Pause Agent [切替]
      - メッセージログを有効にする / Enable Message Log
      - メッセージログを使用不可にする / Disable Message Log
      - 速度補間オブジェクト / Velocity Interpolate Objects [切替]
      - 速度補間オブジェクトへPing / Ping Interpolate Object Positions [切替]
      - パケットをドロップ / Drop a Packet
    - キャッシュ / Cache
      - ディスクキャッシュをパージ / Purge Disk Cache
    - スクリプト付きカメラをダンプ / Dump Scripted Camera
    - レコーダー / Recorder
      - イベントの記録開始 / Start event recording [初期非表示・表示条件あり]
      - イベントの記録停止 / Stop event recording [初期非表示・表示条件あり]
      - イベントの記録のプレイバック / Playback event recording [初期非表示・表示条件あり]
      - 再生開始 / Start Playback
      - 再生停止 / Stop Playback
      - 再生をループ / Loop Playback [切替]
      - 記録開始 / Start Record
      - 記録停止 / Stop Record
    - ワールド / World
      - シムの太陽の設定を無視する / Sim Sun Override [切替]
      - 固定された天気 / Fixed Weather [切替]
      - リージョンオブジェクトのキャッシュをダンプ / Dump Region Object Cache
      - レコード状況をファイルに保存 / Record Stats to File [切替]
      - 関心リスト：３６０モード / Interest Lists 360 Mode [切替]
      - 関心リストをリセット / Reset Interest Lists
      - シミュレーターの特徴を近くのチャットにダンプ / Dump Simulator Features to Nearby Chat
    - 地形 / Terrain
      - 地形をリビルド / Rebuild Terrain
      - ローカルペイントマップを作成 / Create Local Paintmap [有効条件あり]
      - ローカルペイントマップを削除 / Delete Local Paintmap
    - ＵＩ / UI
      - メディアブラウザのテスト / Media Browser Test
      - リージョン再起動テスト… / Region Restart Test... [切替]
      - Webコンテンツブラウザ / Web Content Browser
      - インベントリのサムネイルヘルパ / Inventory Thumbnails Helper
      - フォントをダンプ / Dump Fonts
      - フォントをテクスチャにダンプ / Dump Font Textures
      - SelectMgrをダンプ / Dump SelectMgr
      - インベントリの出力 / Dump Inventory
      - タイマーをダンプ / Dump Timers
      - フォーカスホルダーをダンプ / Dump Focus Holder
      - 選択したオブジェクト情報をプリント / Print Selected Object Info
      - エージェント情報をプリント / Print Agent Info
      - ダブルクリックでオートパイロット / Double-Click Auto-Pilot [切替]
      - ダブルクリックでテレポート / Double-Click Teleport [切替]
      - SelectMgrのデバッグ / Debug SelectMgr [切替]
      - ダブルクリック / Debug Clicks [切替]
      - デバッグ表示 / Debug Views [切替]
      - Unicodeのデバッグ / Debug Unicode [切替]
      - カメラ操作のデバッグ / Debug Camera Controls [切替]
      - デバッグ名ツールチップ / Debug Name Tooltips [切替]
      - マウスイベントのデバッグ / Debug Mouse Events [切替]
      - デバッグキー / Debug Keys [切替]
      - WindowProcのデバッグ / Debug Window Process [切替]
    - ＸＵＩ／色 / XUI
      - 色の設定 / Show Color settings
      - 色の設定をリロード / Reload Color Settings
      - フォントテストを表示 / Show Font Test
      - SLappsテストを表示 / Show SLapps Test
      - ＸＭＬから読み込む / Load from XML
      - ＸＭＬに保存 / Save to XML
      - ＸＵＩ名を表示 / Show XUI Names [切替]
      - デバッグ情報をビューに表示 / Show debugging info for views [切替]
      - ＸＵＩプレビューツール / XUI Preview Tool
      - テストＩＭを送信する / Send Test IMs
      - 名前キャッシュをフラッシュ / Flush Names Caches
    - アバター / Avatar
      - ベークドテクスチャを取得する / Grab Baked Texture
        - 瞳 / Iris [有効条件あり]
        - 頭 / Head [有効条件あり]
        - 上半身 / Upper Body [有効条件あり]
        - 下半身 / Lower Body [有効条件あり]
        - スカート / Skirt [有効条件あり]
      - キャラクターテスト / Character Tests
        - 容姿をＸＭＬとして保存する / Appearance To XML
        - キャラクタージオメトリの切り替え / Toggle Character Geometry [有効条件あり]
        - 男性アバターのテスト / Test Male
        - 女性アバターのテスト / Test Female
        - アバター選択を許可 / Allow Select Avatar [切替]
        - 視線アニメーションを無効化 / Disable Look At Animation [切替]
        - フレンドのみ描画 / Render Only Friends [切替]
      - アニメーションの速度 / Animation Speed
        - すべてのアニメーションを１０％速く / All Animations 10% Faster
        - すべてのアニメーションを１０％遅く / All Animations 10% Slower
        - すべてのアニメーションの速度をリセット / Reset All Animation Speed
        - アニメーションをスローモーションにする / Slow Motion Animations [切替]
      - パラメータを強制的にデフォルトにする / Force Params to Default
      - アニメーション情報 / Animation Info [切替]
      - 見えているものを表示する / Show Look At [切替]
      - クリックした場所を表示する / Show Point At [切替]
      - ジョイント部分の更新をデバッグ / Debug Joint Updates [切替]
      - ＬｏＤを無効にする / Disable LOD [切替]
      - キャラクターのVisをデバッグ / Debug Character Vis [切替]
      - キャラクターのRezステートをデバッグ / Debug Character Rez State [切替]
      - スケルトンの衝突判定を表示する / Show Collision Skeleton [切替]
      - ボーンを表示 / Show Bones [切替]
      - エージェントの視線を表示する / Display Agent Target [切替]
      - インポスターの範囲を表示 / Show Impostor Extents [切替]
      - 装着物のダンプ / Dump Attachments
      - アバターのテクスチャをデバッグ / Debug Avatar Textures
      - ローカルテクスチャをダンプ / Dump Local Textures
      - アバターのクラウドパーティクルをリロード / Reload Avatar Cloud Particle
    - ＨＴＴＰテクスチャ / HTTP Textures [切替]
    - ＨＴＴＰインベントリ / HTTP Inventory [切替]
    - 圧縮画像 / Compress Images
    - 圧縮ファイルテスト / Compress File Test
    - Primfeed認証テスト / Primfeed Auth Test
    - Primfeed認証をリセット / Primfeed Auth Reset
    - Visual Leak Detectorを有効にする / Enable Visual Leak Detector
    - デバッグ用のミニダンプを出力する / Output Debug Minidump [切替]
    - 次回の起動時にコンソールウィンドウを表示する / Console Window on next Run [切替]
    - ログレベルを設定 / Set Logging Level
      - デバッグ / Debug [切替]
      - 情報 / Info [切替]
      - 警告 / Warning [切替]
      - エラー / Error [切替]
      - なし / None [切替]
    - 管理者ステータスの呼び出し / Request Admin Status
    - 管理者ステータス解除 / Leave Admin Status
    - 管理者メニューを表示する / Show Admin Menu [切替・有効条件あり]
  - 管理 / Admin [初期非表示]
    - オブジェクト / Object
      - コピーを取る / Take Copy [有効条件あり]
      - 自分を所有者にする / Force Owner To Me [有効条件あり]
      - 所有者権限の実行 / Force Owner Permissive [有効条件あり]
      - 削除 / Delete [有効条件あり]
      - ロック / Lock [有効条件あり]
      - アセットのＩＤを取得 / Get Assets IDs [有効条件あり]
    - 区画 / Parcel
      - 自分を所有者にする / Force Owner To Me [有効条件あり]
      - Lindenのコンテンツに設定する / Set to Linden Content [有効条件あり]
      - パブリックの土地を取得する / Claim Public Land [有効条件あり]
    - リージョン / Region
      - 一時アセットデータのダンプ / Dump Temp Asset Data [有効条件あり]
      - リージョンの状態を保存する / Save Region State [有効条件あり]
    - ゴッド・ツール / God Tools [有効条件あり]
  - 管理 / Admin（旧定義・通常は非表示） [初期非表示]
    - オブジェクトを取り付ける / Attach Object
    - オブジェクトを取り外す / Detach Object
    - 衣装を脱ぐ / Take Off Clothing
      - シャツ / Shirt [有効条件あり]
      - ズボン / Pants [有効条件あり]
      - 靴 / Shoes [有効条件あり]
      - 靴下 / Socks [有効条件あり]
      - ジャケット / Jacket [有効条件あり]
      - 手袋 / Gloves [有効条件あり]
      - 下着（上半身） / Undershirt [有効条件あり]
      - 下着（下半身） / Underpants [有効条件あり]
      - スカート / Skirt [有効条件あり]
      - アルファ / Alpha [有効条件あり]
      - タトゥ / Tattoo [有効条件あり]
      - ユニバーサル / Universal [有効条件あり]
      - 物理作用 / Physics [有効条件あり]
      - すべての衣装 / All Clothes
    - ヘルプ / Help
      - Linden公式ブログ / Official Linden Blog
      - スクリプトポータル / Scripting Portal
      - バグ報告 / Bug Reporting
        - パブリック問題トラッカー / Public Issue Tracker
        - パブリック問題トラッカーヘルプ / Public Issue Tracker Help
        - バグ報告の基礎知識 / Bug Reporting 101
        - セキュリティ問題 / Security Issues
        - 品質保証ポータル / QA Wiki

## ログイン画面のメニューバー

### `menu_login.xml`

定義: `indra/newview/skins/default/xui/en/menu_login.xml`

- Login Menu
  - ファイル / Viewer
    - 初期設定… / Preferences...
    - ウィンドウを閉じる / Close Window [初期非表示・有効条件あり]
    - グリッドピッカーを表示する / Show Grid Picker [切替・初期非表示]
    - [APP_NAME]を終了 / Exit [APP_NAME]
  - ヘルプ / Help
    - Firestorm Wiki
    - トラブルシューティング / Troubleshooting
    - ガイドブック / Guidebook
    - ホワイトリストアドバイザー / Whitelist adviser
    - Second Lifeのバグ報告 / Report Problem
    - [APP_NAME]について / About [APP_NAME]
    - [CURRENT_GRID]ヘルプ / [CURRENT_GRID] Help
    - [CURRENT_GRID]について / About [CURRENT_GRID]
  - デバッグメニューを表示する / Show Debug Menu [切替・初期非表示]
  - デバッグ / Debug [初期非表示]
    - デバッグ設定を表示 / Show Debug Settings
    - 色の設定 / Show Color settings
    - ＸＵＩプレビューツール / XUI Preview Tool
    - フォント / Fonts
      - フォントテストを表示 / Show Font Test
      - フォントをダンプ / Dump Fonts
      - フォントをテクスチャにダンプ / Dump Font Textures
    - ＵＩテスト / UI Tests
      - テキストボックス / Textbox
      - テキストエディタ / Text Editor
      - ウィジェット / Widgets
      - インスペクタ / Inspectors
    - ウィンドウのサイズを設定… / Set Window Size...
    - 利用規約を表示 / Show TOS
    - クリティカルメッセージを表示 / Show Critical Message
    - メディアブラウザ / Media Browser
    - ログレベルを設定 / Set Logging Level
      - デバッグ / Debug [切替]
      - 情報 / Info [切替]
      - 警告 / Warning [切替]
      - エラー / Error [切替]
      - なし / None [切替]
    - 通知コンソールを表示する / Show Notifications Console [初期非表示]

## その他のクリック／右クリックメニュー

以下の入口名はファイル名からの推定であり、個々の起動箇所を実機確認したものではない。

### `menu_add_wearable_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_add_wearable_gear.xml`

- Add Wearable Gear Menu
  - 新着順で並べ替え / Sort by Most Recent [切替]
  - 名前で並べ替え / Sort by Name [切替]
  - 種別で並べ替え / Sort by Type [切替・表示条件あり]

### `menu_attachment_other.xml`

定義: `indra/newview/skins/default/xui/en/menu_attachment_other.xml`

- Avatar Pie
  - プロフィールを見る / View Profile [有効条件あり]
  - フレンドに追加 / Add Friend [有効条件あり]
  - コンタクトセットに追加 / Add to Set [有効条件あり]
  - ＩＭ / IM [有効条件あり]
  - コーリングカードを渡す / Give Calling Card [有効条件あり]
  - コール / Call [有効条件あり]
  - グループに招待… / Invite to Group [有効条件あり]
  - Face towards avatar [有効条件あり]
  - スケルトンのリセット / Reset Skeleton
  - スケルトンとアニメーションのリセット / Reset Skeleton And Animations
  - メッシュのＬＯＤをリセット / Reset Mesh LOD
  - 迷惑行為 / Annoyance
    - ブロック / Block [表示条件あり]
    - ブロック解除 / Unblock [表示条件あり]
    - 嫌がらせの報告 / Report
    - フリーズ / Freeze [表示条件あり]
    - 追放 / Eject [表示条件あり]
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - ズームイン / Zoom In
  - 支払う… / Pay [有効条件あり]
  - オブジェクトの調査 / Object Profile [有効条件あり]
  - 常に詳細まで表示 / Always Display Full Detail [切替]
  - 詳細まで表示しない / Never Display Full Detail [切替]
  - 表示の例外から削除する / Remove From Display Exceptions [切替・表示条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [表示条件あり]
  - 調査 / Inspect
  - スクリプト情報 / Script Info [有効条件あり]
  - 描画解除 / Derender
  - 描画解除＋ブラックリスト / Derender + Blacklist
  - テクスチャを表示 / Show Textures
  - テクスチャをリフレッシュ / Texture Refresh
  - 名前をつけて保存 / Save as
    - バックアップ / Backup [有効条件あり]
    - Collada形式 / Collada [有効条件あり]
  - Add to SSS whitelist [有効条件あり]
  - Remove from SSS whitelist [有効条件あり]
  - Add entire linkset to SSS whitelist [有効条件あり]
  - Remove entire linkset from SSS whitelist [有効条件あり]

### `menu_attachment_self.xml`

定義: `indra/newview/skins/default/xui/en/menu_attachment_self.xml`

- Attachment Pie
  - アイテムを触る / Touch [有効条件あり]
  - インベントリに表示 / Show in inventory [有効条件あり]
  - 編集… / Edit [有効条件あり]
  - ＰＢＲマテリアルを編集 / Edit PBR Material [有効条件あり]
  - 取り外す / Detach [有効条件あり]
  - 名前をつけて保存 / Save as
    - バックアップ / Backup [有効条件あり]
    - Collada [有効条件あり]
  - 座る / Sit Down [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - 容姿 / Appearance
    - 着用中… / Now wearing... [有効条件あり]
    - アウトフィット… / Change Outfit [有効条件あり]
    - シェイプを編集 / Edit Shape [有効条件あり]
    - アウトフィットを編集 / Edit Outfit [有効条件あり]
    - ホバーする高さ… / Hover Height [有効条件あり]
    - スケルトンをリセット / Reset Skeleton
    - スケルトンとアニメをリセット / Reset Skeleton And Animations
    - メッシュのＬＯＤをリセット / Reset Mesh LOD
  - 脱ぐ / Take off
    - 衣装 / Clothes
      - シャツ / Shirt [有効条件あり]
      - ズボン / Pants [有効条件あり]
      - スカート / Skirt [有効条件あり]
      - 靴 / Shoes [有効条件あり]
      - 靴下 / Socks [有効条件あり]
      - ジャケット / Jacket [有効条件あり]
      - グローブ / Gloves [有効条件あり]
      - 下着（上半身） / Undershirt [有効条件あり]
      - 下着（下半身） / Underpants [有効条件あり]
      - タトゥー / Tattoo [有効条件あり]
      - 物理効果 / Physics [有効条件あり]
      - アルファ / Alpha [有効条件あり]
      - すべての衣装 / All Clothes
    - ＨＵＤ / HUD
    - 取り外す / Detach
    - 全て取り外し / Detach All [有効条件あり]
  - 座る／立つ / Sit / stand [有効条件あり]
  - 飛ぶ／降りる / Fly / land [有効条件あり]
  - アニメーション停止 / Stop animations
  - コミュニティ / Community
    - フレンド / Friends
    - グループ / Groups
    - プロフィール / Profile
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - オブジェクトのプロフィール / Object Profile [有効条件あり]
  - 調査 / Inspect
  - スクリプト情報 / Script Info [有効条件あり]
  - 描画解除 / Derender [有効条件あり]
  - 描画解除＋ブラックリスト / Derender + Blacklist [有効条件あり]
  - テクスチャを表示 / Show Textures
  - テクスチャのリフレッシュ / Texture Refresh
  - 下に置く / Drop [有効条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [有効条件あり]
  - Add to SSS whitelist [有効条件あり]
  - Remove from SSS whitelist [有効条件あり]
  - Add entire linkset to SSS whitelist [有効条件あり]
  - Remove entire linkset from SSS whitelist [有効条件あり]

### `menu_avatar_icon.xml`

定義: `indra/newview/skins/default/xui/en/menu_avatar_icon.xml`

- Avatar Icon Menu [初期非表示]
  - プロフィールの表示 / View Profile
  - ＩＭ / IM
  - 住民の場所にテレポート / Teleport to resident
  - テレポートのオファー / Offer teleport
  - テレポートをリクエスト / Request Teleport
  - 音声通話 / Voice call
  - チャット履歴… / Chat history...
  - フレンドに追加… / Add friend
  - フレンドから削除… / Remove friend
  - コンタクトセットに追加 / Add to set
  - グループに勧誘… / Invite to group...
  - ズームイン / Zoom In
  - マップ / Map
  - 追跡 / Track
  - シェア / Share
  - 支払う / Pay [有効条件あり]
  - 名前をコピー / Copy Name to clipboard
  - ＵＲＬをコピー / Copy Url to clipboard
  - メンション先のＵＲＩをクリップボードにコピー / Copy Mention URI to clipboard
  - 嫌がらせの報告 / Report Abuse [有効条件あり]
  - ボイスをブロック / Block Voice [切替]
  - テキストをブロック / Block Text [切替]
  - モデレーターのオプション / Moderator Options
    - Allow text chat [有効条件あり]
    - テキストチャットを禁止 / Forbid text chat [有効条件あり]
    - この参加者をミュートする / Mute this participant [表示条件あり・有効条件あり]
    - この参加者のミュートを解除する / Unmute this participant [表示条件あり・有効条件あり]
  - メンバーをバン / Ban member [有効条件あり]
  - メンション先のユーザをチャットに / Mention User in Chat

### `menu_avatar_other.xml`

定義: `indra/newview/skins/default/xui/en/menu_avatar_other.xml`

- Avatar Pie
  - プロフィールを見る / View Profile [有効条件あり]
  - フレンドに追加 / Add Friend [有効条件あり]
  - コーリングカードを渡す / Give Calling Card [有効条件あり]
  - コンタクトセットに追加 / Add to Set [有効条件あり]
  - ＩＭ / IM [有効条件あり]
  - コール / Call [有効条件あり]
  - グループに招待 / Invite to Group [有効条件あり]
  - Face towards avatar [有効条件あり]
  - スケルトンのリセット / Reset Skeleton
  - スケルトンとアニメーションのリセット / Reset Skeleton And Animations
  - メッシュのＬＯＤをリセット / Reset Mesh LOD
  - 迷惑行為 / Annoyance
    - ブロック / Block [表示条件あり]
    - ブロック解除 / Unblock [表示条件あり]
    - 嫌がらせの報告 / Report
    - フリーズ / Freeze [表示条件あり]
    - 追放 / Eject [表示条件あり]
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - ズームイン / Zoom In
  - 支払う / Pay [有効条件あり]
  - 描画解除 / Derender
  - 描画解除＋ブラックリスト / Derender + Blacklist
  - スクリプト情報 / Script Info [有効条件あり]
  - テクスチャを表示 / Show Textures
  - テクスチャのリフレッシュ / Texture Refresh
  - 常に詳細まで表示 / Always Display Full Detail [切替]
  - 詳細まで表示しない / Never Display Full Detail [切替]
  - 表示の例外から削除する / Remove From Display Exceptions [切替・表示条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [表示条件あり]

### `menu_avatar_rendering_settings.xml`

定義: `indra/newview/skins/default/xui/en/menu_avatar_rendering_settings.xml`

- Settings
  - 常に描画 / Always full detail [切替]
  - 描画しない / Never full detail [切替]
  - 例外から削除 / Remove from exceptions [切替・表示条件あり]

### `menu_avatar_rendering_settings_add.xml`

定義: `indra/newview/skins/default/xui/en/menu_avatar_rendering_settings_add.xml`

- menu_settings_add.xml [初期非表示]
  - 常に詳細まで全て描画… / Always full detail...
  - 詳細まで描画しない… / Never full detail...

### `menu_avatar_self.xml`

定義: `indra/newview/skins/default/xui/en/menu_avatar_self.xml`

- Self Pie
  - 座る / Sit Down [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - 脱ぐ / Take Off
    - 衣装 / Clothes
      - シャツ / Shirt [有効条件あり]
      - ズボン / Pants [有効条件あり]
      - スカート / Skirt [有効条件あり]
      - 靴 / Shoes [有効条件あり]
      - 靴下 / Socks [有効条件あり]
      - ジャケット / Jacket [有効条件あり]
      - 手袋 / Gloves [有効条件あり]
      - 下着（上半身） / Undershirt [有効条件あり]
      - 下着（下半身） / Underpants [有効条件あり]
      - タトゥ / Tattoo [有効条件あり]
      - 物理作用 / Physics [有効条件あり]
      - アルファ / Alpha [有効条件あり]
      - すべての衣装 / All Clothes
    - ＨＵＤ / HUD
    - 取り外す / Detach
    - すべて取り外す / Detach All [有効条件あり]
  - 容姿 / Appearance
    - 着用中… / Now wearing... [有効条件あり]
    - アウトフィットを変更 / Outfits [有効条件あり]
    - シェイプを編集 / Edit Shape [有効条件あり]
    - アウトフィットを編集 / Edit Outfit [有効条件あり]
    - ホバーする高さ… / Hover Height [有効条件あり]
    - スケルトンをリセット / Reset Skeleton
    - スケルトンとアニメーションをリセット / Reset Skeleton And Animations
    - メッシュのＬＯＤをリセット / Reset Mesh LOD
  - コミュニティ / Community
    - フレンド / Friends
    - グループ / Groups
    - プロフィール / Profile
  - スクリプト情報 / Script Info [有効条件あり]
  - テクスチャのデバッグ… / Show Textures
  - テクスチャのリフレッシュ / Texture Refresh
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [有効条件あり]

### `menu_classifieds.xml`

定義: `indra/newview/skins/default/xui/en/menu_classifieds.xml`

- Classifieds
  - 情報 / Info [有効条件あり]
  - 編集 / Edit [初期非表示・有効条件あり]
  - テレポート / Teleport [有効条件あり]
  - マップ / Map [有効条件あり]
  - 削除 / Delete [初期非表示・有効条件あり]

### `menu_cof_attachment.xml`

定義: `indra/newview/skins/default/xui/en/menu_cof_attachment.xml`

- COF Attachment
  - 触る / Touch [有効条件あり]
  - 編集 / Edit [有効条件あり]
  - 取り外す / Detach

### `menu_cof_body_part.xml`

定義: `indra/newview/skins/default/xui/en/menu_cof_body_part.xml`

- COF Body
  - 置き換え / Replace
  - 編集 / Edit [有効条件あり]
  - 新規作成 / Create New [有効条件あり]

### `menu_cof_clothing.xml`

定義: `indra/newview/skins/default/xui/en/menu_cof_clothing.xml`

- COF Clothing
  - 脱ぐ / Take Off [有効条件あり]
  - 編集 / Edit [有効条件あり]
  - 着替える / Replace [有効条件あり]
  - 新規作成 / Create New [有効条件あり]

### `menu_cof_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_cof_gear.xml`

- Gear COF
  - 新しい衣装 / New Clothes
  - 新しい身体部位 / New Body Parts

### `menu_conversation.xml`

定義: `indra/newview/skins/default/xui/en/menu_conversation.xml`

- menu_conversation_participant [初期非表示]
  - 会話を終える / Close conversation
  - ボイスチャットを始める / Open voice conversation [有効条件あり]
  - ボイスチャットを切断する / Disconnect from voice
  - 選択した項目を閉じる / Close Selected
  - プロフィールを表示 / View Profile [有効条件あり]
  - ＩＭ / IM [有効条件あり]
  - テレポートのオファー / Offer teleport [有効条件あり]
  - テレポートのリクエスト / Request teleport [有効条件あり]
  - ボイスコール / Voice call [有効条件あり]
  - チャットの履歴… / Chat transcript... [有効条件あり]
  - フレンドに追加 / Add friend [有効条件あり]
  - フレンドから削除 / Remove friend [有効条件あり]
  - フレンドから削除 / Remove friends [有効条件あり]
  - グループに招待… / Invite to group... [有効条件あり]
  - ズームイン / Zoom In [有効条件あり]
  - マップ / Map [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - 嫌がらせの報告 / Report Abuse [有効条件あり]
  - ボイスをブロック / Block Voice [切替・有効条件あり]
  - テキストをブロックする / Block Text [切替・有効条件あり]
  - グループのプロフィール / Group Profile [有効条件あり]
  - このグループをアクティブ化 / Activate Group [有効条件あり]
  - このグループから脱退する / Leave Group [有効条件あり]
  - モデレーターのオプション / Moderator Options
    - テキストチャットを許可 / Allow text chat [切替・有効条件あり]
    - この参加者をミュートする / Mute this participant [表示条件あり・有効条件あり]
    - この参加者のミュートを解除する / Unmute this participant [表示条件あり・有効条件あり]
    - 全員のボイスをミュートする / Mute everyone [有効条件あり]
    - 全員のミュートを解除する / Unmute everyone [有効条件あり]
  - メンバーをバンする / Ban member [有効条件あり]

### `menu_conversation_log_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_conversation_log_gear.xml`

- Conversation Context Menu
  - ＩＭ… / IM... [有効条件あり]
  - ボイスコール… / Voice call... [有効条件あり]
  - チャットの履歴を開く… / Open chat transcript... [有効条件あり]
  - チャットの履歴を別ウィンドウで開く… / Open chat transcript externally... [有効条件あり]
  - プロフィールを表示 / View Profile [有効条件あり]
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - フレンドに追加 / Add Friend [表示条件あり]
  - フレンドから削除 / Remove Friend [表示条件あり]
  - グループに招待…. / Invite to group... [有効条件あり]
  - マップ / Map [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - ブロック／ブロック解除 / Block/Unblock [切替・有効条件あり]

### `menu_conversation_log_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_conversation_log_view.xml`

- menu_conversation_view [初期非表示]
  - 名前で並べ替え / Sort by name [切替]
  - 日付で並べ替え / Sort by date [切替]
  - 上位のフレンドで並べ替え / Sort friends on top [切替]
  - 近くのチャットの履歴を表示… / View Nearby chat transcript...

### `menu_copy_paste_color.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_color.xml`

- Copy Paste Color Menu
  - コピー / Copy
  - ペースト / Paste [有効条件あり]

### `menu_copy_paste_features.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_features.xml`

- Copy Paste Features Menu
  - コピー / Copy
  - ペースト / Paste [有効条件あり]

### `menu_copy_paste_light.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_light.xml`

- Copy Paste Light Menu
  - コピー / Copy
  - ペースト / Paste [有効条件あり]

### `menu_copy_paste_object.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_object.xml`

- Copy Paste Object Menu
  - コピー / Copy
  - ペースト / Paste [有効条件あり]

### `menu_copy_paste_pos.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_pos.xml`

- Copy Paste Position Menu
  - すべてをコピー / Copy all [有効条件あり]
  - 位置をコピー / Copy position
  - すべてペースト / Paste all [有効条件あり]
  - 位置をペースト / Paste position [有効条件あり]

### `menu_copy_paste_rot.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_rot.xml`

- Copy Paste Rotation Menu
  - すべてをコピー / Copy all [有効条件あり]
  - 回転をコピー / Copy rotation
  - すべてをペースト / Paste all [有効条件あり]
  - 回転をペースト / Paste rotation [有効条件あり]

### `menu_copy_paste_size.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_size.xml`

- Copy Paste Size Menu
  - すべてをコピー / Copy all [有効条件あり]
  - サイズをコピー / Copy size
  - すべてをペースト / Paste all [有効条件あり]
  - サイズをペースト / Paste size [有効条件あり]

### `menu_copy_paste_texture.xml`

定義: `indra/newview/skins/default/xui/en/menu_copy_paste_texture.xml`

- Copy Paste Texture Menu
  - コピー / Copy
  - ペースト / Paste [有効条件あり]

### `menu_edit.xml`

定義: `indra/newview/skins/default/xui/en/menu_edit.xml`

- 編集 / Edit [初期非表示]
  - 取り消し / Undo [有効条件あり]
  - やり直し / Redo [有効条件あり]
  - カット / Cut [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - 削除 / Delete [有効条件あり]
  - すべて選択 / Select All [有効条件あり]
  - 選択解除 / Deselect [有効条件あり]

### `menu_embedded_item.xml`

定義: `indra/newview/skins/default/xui/en/menu_embedded_item.xml`

- Embedded item context menu
  - 開く / Open [有効条件あり]
  - インベントリにコピー / Copy to Inventory [有効条件あり]

### `menu_favorites.xml`

定義: `indra/newview/skins/default/xui/en/menu_favorites.xml`

- Popup [初期非表示]
  - テレポート / Teleport
  - マップに表示 / Show on Map [有効条件あり]
  - ランドマークの表示／編集 / View/Edit Landmark
  - ランドマークに移動 / Move to Landmarks
  - SLurlをコピー / Copy SLurl [有効条件あり]
  - ピックを作成 / Create Pick [有効条件あり]
  - コピー / Copy
  - ペースト / Paste [有効条件あり]
  - 名前変更 / Rename
  - 削除 / Delete

### `menu_fs_area_search.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_area_search.xml`

- Object List popup
  - スクリプト情報 / Script Info [表示条件あり・有効条件あり]
  - 触る / Touch [表示条件あり]
  - 座る / Sit [表示条件あり・有効条件あり]
  - オブジェクトの場所にテレポート / Teleport to [表示条件あり・有効条件あり]
  - ズーム / Zoom [有効条件あり]
  - 調査 / Inspect [表示条件あり]
  - 編集 / Edit [表示条件あり]
  - 返却 / Return [表示条件あり]
  - 削除 / Delete [表示条件あり]
  - ブラックリストに登録 / Blacklist [表示条件あり]
  - オブジェクトを購入 / Buy object [表示条件あり・有効条件あり]
  - 自分のオブジェクトを検索 / Filter My Objects
  - すべてを選択 / Select All
  - 選択をクリア / Clear Selection

### `menu_fs_asset_blacklist.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_asset_blacklist.xml`

- Asset Blacklist Menu
  - 削除 / Remove

### `menu_fs_avatar_render_setting.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_avatar_render_setting.xml`

- Avatar Render Settings Menu
  - 常に描画 / Always Full Detail
  - 描画しない / Never Full Detail
  - 例外から削除 / Remove From Exceptions

### `menu_fs_avatar_search.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_avatar_search.xml`

- Avatar Context Menu
  - プロフィールを表示 / View Profile
  - フレンドに追加 / Add Friend [有効条件あり]
  - コンタクトセットに追加 / Add to Set
  - フレンドから削除 / Remove Friend [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - グループに招待 / Invite To Group
  - ブロック／ブロック解除 / Block/Unblock [切替・有効条件あり]

### `menu_fs_avatar_search_multiselect.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_avatar_search_multiselect.xml`

- Multi-Selected People Context Menu
  - コンタクトセットに追加 / Add to Set
  - フレンドから削除 / Remove Friends [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - テレポートのオファー / Offer Teleport [有効条件あり]

### `menu_fs_block_list.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_block_list.xml`

- menu_blocked_gear [初期非表示]
  - ブロック解除 / Unblock [有効条件あり]
  - ボイスをブロック / Block Voice [切替・表示条件あり・有効条件あり]
  - テキストをブロック / Block Text [切替・表示条件あり・有効条件あり]
  - パーティクルをブロック / Block Particles [切替・表示条件あり・有効条件あり]
  - オブジェクトのサウンドをブロック / Block Object Sounds [切替・表示条件あり・有効条件あり]
  - プロフィール… / Profile... [有効条件あり]

### `menu_fs_bump_list.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_bump_list.xml`

- Bump List Context Menu
  - プロフィールを表示 / View Profile [有効条件あり]
  - ＩＭを送信… / Send IM... [有効条件あり]
  - 住民をズーム / Zoom at Resident [有効条件あり]
  - 住民のところにテレポート / Teleport to Resident [有効条件あり]
  - ブロック / Block [切替・有効条件あり]
  - 嫌がらせの報告 / Report [有効条件あり]
  - コピー / Copy [有効条件あり]

### `menu_fs_chat_options.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_chat_options.xml`

- Chat Options Menu [初期非表示]
  - レーダーを表示… / Show Radar... [切替]
  - 近くのボイスを表示… / Show Nearby Voice... [切替]
  - ブロックリストを表示… / Show Block List... [切替]
  - Ｖ１形式のチャットヘッダを使用 / Use V1 style chat headers [切替]
  - プロフィールアイコンをチャットヘッダーに表示 / Show profile icons in chat headers [切替・表示条件あり]
  - フォントサイズ… / Font size...
    - 小 / Small [切替]
    - 中 / Medium [切替]
    - 大 / Large [切替]
    - 特大 / Huge [切替]
  - 相手が入力中であることを示す角かっこ（＞）を追加 / Additional chevron (>) as typing indicator [切替・表示条件あり・有効条件あり]
  - 新規メッセージ通知を表示 / Show new message notification [切替]
  - チャットバーを表示 / Show chat bar [切替・表示条件あり]
  - チャンネル選択を表示 / Show channel selection [切替・表示条件あり・有効条件あり]
  - 絵文字ボタンを表示 / Show emoji button [切替]
  - チャットの入力／送信ボタンを表示 / Show chat type / send button [切替・表示条件あり・有効条件あり]
  - ＩＭに送信ボタンを表示 / Show send button in IM sessions [切替・表示条件あり]

### `menu_fs_contacts_friends.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_contacts_friends.xml`

- Friends Popup
  - プロフィール / View Profile
  - ＩＭ… / Send IM...
  - 会話履歴を見る… / View chat transcripts... [有効条件あり]
  - コンタクトセットに追加… / Add to Set...
  - ズームイン / Zoom at Friend [有効条件あり]
  - フレンドの場所にテレポート / Teleport to Friend [有効条件あり]
  - テレポートを送る / Offer Teleport [有効条件あり]
  - テレポートを要求する / Request Teleport [有効条件あり]
  - 支払う / Pay
  - フレンドを追跡 / Track Friend [有効条件あり]
  - フレンドを削除… / Remove Friend... [有効条件あり]
  - 名前をコピー / Copy Name to clipboard
  - ＵＲＬをコピー / Copy Url to clipboard
  - メンション先のＵＲＩをクリップボードにコピー / Copy Mention URI to clipboard
  - オプション… / Options...
    - ユーザ名列を表示 / Show username column [切替・有効条件あり]
    - 表示名列を表示 / Show display name column [切替・有効条件あり]
    - Show full name column [切替・有効条件あり]
    - 権限列を表示 / Show permission columns [切替]
    - ユーザ名で並べ替え / Sort by username [切替]
    - 表示名で並べ替え / Sort by display name [切替]
    - フルネーム形式をユーザ名（表示名）に / Full name format Username (Display name) [切替・有効条件あり]
    - フルネーム形式を表示名（ユーザ名）に / Full name format Display name (Username) [切替・有効条件あり]
    - 検索フィルタを表示 / Show search filter [切替]
  - フレンドのログイン状況 / Online status for friends visible [切替]

### `menu_fs_contacts_friends_multiselect.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_contacts_friends_multiselect.xml`

- Friends Popup
  - ＩＭ… / Send IM...
  - コンタクトセットに追加… / Add to Set...
  - テレポートを送る / Offer Teleport [有効条件あり]
  - フレンドの削除… / Remove Friends... [有効条件あり]
  - オプション… / Options...
    - ユーザ名列を表示 / Show username column [切替・有効条件あり]
    - 表示名列を表示 / Show display name column [切替・有効条件あり]
    - フルネーム列を表示 / Show full name column [切替・有効条件あり]
    - 権限列を表示 / Show permission columns [切替]
    - ユーザ名で並べ替え / Sort by username [切替]
    - 表示名で並べ替え / Sort by display name [切替]
    - フルネーム形式をユーザ名（表示名）に / Full name format Username (Display name) [切替・有効条件あり]
    - フルネーム形式を表示名（ユーザ名）に / Full name format Display name (Username) [切替・有効条件あり]
    - 検索フィルタを表示 / Show search filter [切替]
  - フレンドのログイン状況 / Online status for friends visible [切替]

### `menu_fs_im_teleport.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_im_teleport.xml`

- menu_teleport [初期非表示]
  - テレポートを送る / Offer teleport [有効条件あり]
  - テレポートのリクエスト / Request teleport

### `menu_fs_im_well_button.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_im_well_button.xml`

- IM Well Button Context Menu
  - すべてを閉じる / Close All [有効条件あり]

### `menu_fs_imchiclet_adhoc.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_imchiclet_adhoc.xml`

- IMChiclet AdHoc Menu [初期非表示]
  - セッション終了 / End Session

### `menu_fs_imchiclet_group.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_imchiclet_group.xml`

- IMChiclet Group Menu [初期非表示]
  - グループ情報 / Group Info
  - セッションを表示 / Show Session
  - セッションをスヌーズ / Snooze Session
  - セッション終了 / End Session

### `menu_fs_imchiclet_p2p.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_imchiclet_p2p.xml`

- IMChiclet P2P Menu [初期非表示]
  - プロフィールを表示 / View Profile
  - フレンドに追加 / Add Friend
  - セッションを表示 / Show Session
  - セッション終了 / End Session

### `menu_fs_inspect_options.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_inspect_options.xml`

- menu_group_plus
  - 説明 / Description [切替]
  - 所有者名 / Owner Name [切替]
  - 制作者名 / Creator Name [切替]
  - 面数 / Faces [切替]
  - 頂点数 / Vertices [切替]
  - 三角形数 / Triangles [切替]
  - ＴＲＡＭ / TMem [切替]
  - ＶＲＡＭ / VRAM [切替]
  - 作成日 / Creation Date [切替]

### `menu_fs_moneytracker_list.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_moneytracker_list.xml`

- Money Tracker List Context Menu
  - ＣＳＶ形式でコピー / Copy as CSV [有効条件あり]
  - 削除 / Delete [有効条件あり]

### `menu_fs_namelist_avatar.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_namelist_avatar.xml`

- Namelist Popup
  - プロフィールを表示 / View Profile
  - ＩＭを送信… / Send IM... [有効条件あり]
  - 会話ログを表示 / View chat transcripts... [有効条件あり]
  - フレンドに追加 / Add Friend [有効条件あり]
  - セットに追加… / Add to Set...
  - 住民をズーム / Zoom at Resident [有効条件あり]
  - 住民のところにテレポート / Teleport to Resident [有効条件あり]
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - 住民を追跡 / Track Resident [有効条件あり]
  - フレンドから削除 / Remove Friend... [有効条件あり]
  - ブロック / Block [切替・有効条件あり]
  - 名前をコピー / Copy Name to clipboard
  - ＵＲＬをコピー / Copy Url to clipboard

### `menu_fs_namelist_avatar_multiselect.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_namelist_avatar_multiselect.xml`

- Namelist Popup
  - ＩＭを送信… / Send IM...
  - セットに追加… / Add to Set...
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - フレンド削除… / Remove Friends... [有効条件あり]

### `menu_fs_poser_poses_btn.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_poser_poses_btn.xml`

- Settings
  - 選択的に読み込む / Load Selective
  - 選択的に読み込む－回転のみ / Load Selective - Rotations Only
  - 読み込む－回転のみ / Load - Rotations Only
  - 読み込む / Load

### `menu_fs_profile_image_actions.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_profile_image_actions.xml`

- Avatar Profile Image Actions
  - 画像をアップロード / Upload Photo [有効条件あり]
  - 画像を変更 / Change Photo [有効条件あり]
  - 画像を削除 / Remove Photo [有効条件あり]

### `menu_fs_profile_name_field.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_profile_name_field.xml`

- CopyMenu
  - 表示名をコピー / Copy Display Name
  - エージェント名をコピー / Copy Agent Name
  - ＵＵＩＤをコピー / Copy UUID
  - ＵＲＩをコピー / Copy URI

### `menu_fs_profile_overflow.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_profile_overflow.xml`

- profile_overflow_menu
  - 共有 / Share
  - コール / Call [有効条件あり]
  - コンタクトセットに追加 / Add to Contact Set
  - 名前をコーピー / Copy Name
  - ＵＲＩをコピー / Copy URI
  - ＵＵＩＤをコピー / Copy UUID
  - キック / Kick [表示条件あり・有効条件あり]
  - フリーズ / Freeze [表示条件あり・有効条件あり]
  - フリーズ解除 / Unfreeze [表示条件あり・有効条件あり]
  - ＣＳＲ / CSR [表示条件あり・有効条件あり]
  - 嫌がらせの報告 / Report

### `menu_fs_radar.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_radar.xml`

- Avatar Context Menu
  - プロフィールの表示 / View Profile
  - フレンド登録 / Add Friend [有効条件あり]
  - 連絡先セットに追加 / Add to Set
  - フレンドを削除 / Remove Friend [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - ワールドマップ / Map [有効条件あり]
  - 追跡する / Track
  - Face towards avatar [有効条件あり]
  - マーク… / Mark...
    - 赤 / Red
    - 緑 / Green
    - 青 / Blue
    - 紫 / Purple
    - 黄 / Yellow
    - マークをクリア / Clear Mark(s)
    - 全てのマークをクリア / Clear all Marks
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - テレポートを送る / Offer Teleport [有効条件あり]
  - テレポートを要求 / Request Teleport [有効条件あり]
  - テレポートする / Teleport To
  - ズームイン / Zoom In [表示条件あり]
  - グループに招待 / Invite To Group
  - スクリプト情報取得 / Get Script Info [有効条件あり]
  - チャットの履歴… / View chat transcripts... [有効条件あり]
  - ブロック／ブロック解除 / Block/Unblock [切替・有効条件あり]
  - 報告する / Report
  - フリーズする / Freeze [表示条件あり]
  - 区画から追放 / Parcel Eject [表示条件あり]
  - リージョンから追放 / Estate Kick [表示条件あり]
  - ホームへ強制送還 / Estate Teleport Home [表示条件あり]
  - リージョンへの立入禁止 / Estate Ban [表示条件あり]
  - 描画解除 / Derender
  - 描画解除リストに追加 / Derender & Blacklist
  - 描画設定 / Render Settings
    - 通常表示 / Render Normally [切替]
    - 描画しない / Do Not Render [切替]
    - 常に完全描画 / Render Fully [切替]

### `menu_fs_radar_multiselect.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_radar_multiselect.xml`

- Multi-Selected People Context Menu
  - フレンド登録 / Add Friends [有効条件あり]
  - 連絡先セットに追加 / Add to Set
  - フレンドを削除 / Remove Friends [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - マーク… / Mark...
    - 赤 / Red
    - 緑 / Green
    - 青 / Blue
    - 紫 / Purple
    - 黄 / Yellow
    - マークをクリア / Clear Mark(s)
    - マークを全てクリア / Clear all Marks
  - 共有 / Share
  - 支払う / Pay [有効条件あり]
  - テレポートを送る / Offer Teleport [有効条件あり]
  - フリーズ / Freeze [表示条件あり]
  - 追放 / Parcel Eject [表示条件あり]
  - キック / Estate Kick [表示条件あり]
  - ホームへテレポート / Estate Teleport Home [表示条件あり]
  - 不動産からバン / Estate Ban [表示条件あり]
  - 描画解除 / Derender
  - 非表示にして無視リストに追加 / Derender & Blacklist
  - 描画設定 / Render Settings
    - 通常描画 / Render Normally
    - 描画しない / Do Not Render
    - 常に描画 / Render Fully

### `menu_fs_radar_options.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_radar_options.xml`

- menu_group_plus
  - ミニマップをレーダーの上に表示 / Show Minimap above Radar [切替・有効条件あり]
  - 設定した範囲内の結果を表示 / Limit Results to specific Radius [切替]
  - ミュート／ブラックリストにしたアバターも表示 / Show muted and blacklisted Avatars [切替]
  - 名前の表示形式 / Radar Name Format
    - 表示名 / Display Name [切替]
    - ユーザー名 / Username [切替]
    - 表示名（ユーザ名） / Display Name (Username) [切替]
    - ユーザー名（表示名） / Username (Display Name) [切替]
  - 表示する項目 / Displayed Columns
    - ボイス / Voice [切替]
    - 同一リージョン / Same Region [切替]
    - 入力状況 / Typing Status [切替]
    - 着席状況 / Sitting Status [切替]
    - 支払情報 / Flags [切替]
    - ノート / Notes [切替]
    - 年齢 / Age [切替]
    - 表示時間 / Seen [切替]
  - 距離で名前を色分け / Color names based on distance [切替]
  - 通知方法 / Report To
    - 近くのチャットで通知 / Nearby Chat [切替]
    - トーストで通知 / Notification Toasts [切替]
  - ミニマップにチャット範囲を表示 / Nearby chat distance ring in Minimap [切替・表示条件あり]
  - 描画範囲に入ってきたら通知 / Report Entering Draw Range [切替]
  - 描画範囲から去っていったら通知 / Report Leaving Draw Range [切替]
  - チャット範囲に入ってきたら通知 / Report Entering Chat Range [切替]
  - チャット範囲から去っていったら通知 / Report Leaving Chat Range [切替]
  - 同じＳＩＭに入ってきたら通知 / Report Entering your Region [切替]
  - 同じＳＩＭから去っていったら通知 / Report Leaving your Region [切替]
  - アバターの年齢に問題があったら通知 / Report Avatar Age Alert [切替]
  - 入ってきた通知をスクリプトに表示 / Report Enter Alerts to Scripts [切替]
  - 去っていった通知をスクリプトに表示 / Report Exit Alerts to Scripts [切替・有効条件あり]
  - ＬＳＬクライアントブリッジでレーダーを強化 / Enhance Radar with LSL-Client Bridge [切替]
  - ブロックした人物とオブジェクトを表示 / Open Blocked Residents & Objects

### `menu_fs_wearable_favorites.xml`

定義: `indra/newview/skins/default/xui/en/menu_fs_wearable_favorites.xml`

- menu_wearable_favorites
  - 名前で並べ替え… / Sort by name... [切替]
  - 新しい順に並べ替え… / Sort by most recent... [切替]
  - 種類で並べ替え… / Sort by type name... [切替]

### `menu_gallery_inventory.xml`

定義: `indra/newview/skins/default/xui/en/menu_gallery_inventory.xml`

- Gallery
  - 共有 / Share
  - ゴミ箱を空に / Empty Trash
  - 遺失物を空に / Empty Lost And Found
  - テレポート / Teleport
  - 現在のアウトフィットと置き換え / Replace Current Outfit
  - フォルダのアイテムを追加 / Add To Current Outfit
  - フォルダのアイテムを脱ぐ / Remove From Current Outfit
  - アウトフィット一覧をクリップボードにコピー / Copy outfit list to clipboard
  - オリジナルを検索 / Find Original
  - アイテムをパージ / Purge Item
  - アイテムを復元 / Restore Item
  - 開く / Open
  - オリジナルを開く / Open Original
  - プロパティ / Properties
  - 画像… / Image...
  - アセットのＵＵＩＤをコピー / Copy Asset UUID
  - 開く / Open
  - 新しいウィンドウで開く / Open in new window
  - 名前変更 / Rename
  - カット / Cut
  - コピー / Copy
  - ペースト / Paste
  - リンクとしてペースト / Paste As Link
  - リンクを置き換え / Replace Links
  - 削除 / Delete
  - システムフォルダを削除 / Delete System Folder
  - 再生 / Play
  - SLurlをコピー / Copy SLurl
  - ランドマークについて / About Landmark
  - マップ上に表示 / Show on Map
  - インワールドで再生 / Play Inworld
  - ローカルで再生 / Play Locally
  - インスタントメッセージを送信 / Send Instant Message
  - テレポートのオファー… / Offer Teleport...
  - テレポートのリクエスト… / Request Teleport...
  - 有効化 / Activate
  - 無効化 / Deactivate
  - 別名で保存 / Save As
  - 選択を別名で保存 / Save Selected As
  - 着る / Wear
  - 装着先 / Attach To
  - ＨＵＤ装着先 / Attach To HUD
  - 触る / Touch
  - 編集 / Edit
  - 追加 / Add
  - 自身から取り外す / Detach From Yourself
  - 脱ぐ / Take Off
  - 自分のみに反映 / Apply Only To Myself
  - 区画に反映 / Apply To Parcel
  - 新規作成 / Create new
    - 新しいフォルダ / New Folder
    - 新しいスクリプト / New Script
    - 新しいノートカード / New Notecard
    - 新しいジェスチャー / New Gesture
    - 新しいマテリアル / New Material [有効条件あり]
    - 新しい衣装 / New Clothes
      - 新しいシャツ / New Shirt
      - 新しいズボン / New Pants
      - 新しい靴 / New Shoes
      - 新しい靴下 / New Socks
      - 新しいジャケット / New Jacket
      - 新しいスカート / New Skirt
      - 新しい手袋 / New Gloves
      - 新しい下着（上半身） / New Undershirt
      - 新しい下着（下半身） / New Underpants
      - 新しいアルファ / New Alpha Mask
      - 新しいタトゥ / New Tattoo
      - 新しいユニバーサル / New Universal
      - 新しい物理作用 / New Physics
    - 新しい身体部位 / New Body Parts
      - 新しいシェイプ（体型） / New Shape
      - 新しいスキン / New Skin
      - 新しい髪 / New Hair
      - 新しい目 / New Eyes
    - 新しい自然環境の設定 / New Settings
      - 新しい空 / New Sky [有効条件あり]
      - 新しい水面 / New Water [有効条件あり]
      - 新しいデイサイクル / New Day Cycle [有効条件あり]
  - 新しいフォルダを選択から作成 / Create folder from selected
  - フォルダのアイテムのグループ解除 / Ungroup folder items
  - お気に入りに追加 / Add to Favorites
  - お気に入りから削除 / Remove from Favorites
  - フォルダにアップロード / Upload to folder
    - 画像… / Image... [表示条件あり・有効条件あり]
    - サウンド（L$ [COST]）… / Sound (L$[COST])... [表示条件あり・有効条件あり]
    - アニメーション（L$ [COST]）… / Animation (L$[COST])... [表示条件あり・有効条件あり]
    - モデル… / Model... [表示条件あり・有効条件あり]
    - マテリアル… / Material... [有効条件あり]
    - 一括… / Bulk... [表示条件あり]
  - デフォルトのアップロード先 / Use as default for
    - アップロードした画像 / Image uploads [切替・表示条件あり]
    - アップロードしたサウンド / Sound uploads [切替・表示条件あり]
    - アップロードしたアニメーション / Animation uploads [切替・表示条件あり]
    - アップロードしたモデル / Model uploads [切替・表示条件あり]
    - アップロードしたＰＢＲマテリアル / PBR material uploads [切替]
  - マーケットプレイスの出品リストにコピー / Copy to Marketplace Listings
  - マーケットプレイスの出品リストに移動 / Move to Marketplace Listings
  - ‐オプション無し‐ / --no options--

### `menu_gallery_outfit_tab.xml`

定義: `indra/newview/skins/default/xui/en/menu_gallery_outfit_tab.xml`

- Outfit
  - アウトフィットを置き換え / Replace Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットにアイテムを追加 / Add to Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットのアイテムを脱ぐ / Remove from Current Outfit [表示条件あり・有効条件あり]
  - 画像… / Image...
  - お気に入りに追加 / Add to Favorites [表示条件あり]
  - お気に入りから削除 / Remove from Favorites [表示条件あり]
  - アウトフィットを編集 / Edit Outfit [表示条件あり]
  - アウトフィットの名前変更 / Rename Outfit [有効条件あり]
  - このアウトフィットを保存 / Save to this Outfit
  - アウトフィットを削除 / Delete Outfit [表示条件あり]
  - 新しい衣装 / New Clothes
    - 新しいシャツ / New Shirt
    - 新しいズボン / New Pants
    - 新しい靴 / New Shoes
    - 新しい靴下 / New Socks
    - 新しいジャケット / New Jacket
    - 新しいスカート / New Skirt
    - 新しい手袋 / New Gloves
    - 新しい下着（上半身） / New Undershirt
    - 新しい下着（下半身） / New Underpants
    - 新しいアルファ / New Alpha
    - 新しい物理作用 / New Physics
    - 新しいタトゥ / New Tattoo
  - 新しい身体部位 / New Body Parts
    - 新しいシェイプ / New Shape
    - 新しい肌 / New Skin
    - 新しい髪型 / New Hair
    - 新しい目 / New Eyes

### `menu_gesture_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_gesture_gear.xml`

- menu_gesture_gear [初期非表示]
  - 選択したジェスチャーの有効化／無効化 / Activate/Deactivate selected gesture
  - 名前変更 / Rename [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - アセットのＵＵＩＤをコピー / Copy UUID [有効条件あり]
  - 現在のアウトフィットに保存 / Save to current outfit
  - 編集 / Edit [有効条件あり]

### `menu_group_plus.xml`

定義: `indra/newview/skins/default/xui/en/menu_group_plus.xml`

- menu_group_plus [初期非表示]
  - グループに参加… / Join Group...
  - 新しいグループ… / New Group...

### `menu_hide_navbar.xml`

定義: `indra/newview/skins/default/xui/en/menu_hide_navbar.xml`

- hide_navbar_menu [初期非表示]
  - ナビゲーションバーを表示 / Show Navigation and Search Bar [切替]
  - 検索バーを表示 / Show Search Bar [切替・有効条件あり]
  - お気に入りバーを表示 / Show Favorites Bar [切替]
  - 現在地を表示 / Show Location [切替]
  - シミュレーターバージョンを表示 / Show Simulator Version [切替・有効条件あり]
  - メニュー検索を表示 / Show Menu Search [切替]
  - 所持金を表示 / Show Currency Balance [切替]
  - メディア操作を表示 / Show Media Controls [切替]
  - ＦＰＳを表示 / Show Frames per Second [切替]
  - ＦＰＳのカラーインジケーターを表示 / Show Color Indicator for Frames per Second [切替・有効条件あり]
  - トラフィックインジケーターを表示 / Show Traffic Indicator [切替]
  - 座標を表示 / Show Coordinates [切替]
  - 区画の権限を表示 / Show Parcel Permissions [切替]

### `menu_im_conversation.xml`

定義: `indra/newview/skins/default/xui/en/menu_im_conversation.xml`

- Conversation Gear Menu
  - プロフィールを表示 / View Profile [有効条件あり]
  - フレンドに追加 / Add Friend [有効条件あり]
  - フレンドから削除 / Remove friend [有効条件あり]
  - テレポートをオファー / Offer teleport [有効条件あり]
  - グループに招待… / Invite to group... [有効条件あり]
  - チャットの履歴… / Chat transcript... [有効条件あり]
  - ズームイン / Zoom In [有効条件あり]
  - マップ / Map [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - 嫌がらせの報告 / Report Abuse [有効条件あり]
  - ボイスのブロック／解除 / Block Voice [切替・有効条件あり]
  - テキストをブロックする / Block Text [切替・有効条件あり]

### `menu_im_session_showmodes.xml`

定義: `indra/newview/skins/default/xui/en/menu_im_session_showmodes.xml`

- menu_modes [初期非表示]
  - 簡易表示 / Compact view [切替]
  - 詳細表示 / Expanded view [切替]
  - 発言時刻を表示 / Show time [切替・有効条件あり]
  - １対１の会話で名前を表示 / Show names in one-to-one conversations [切替・有効条件あり]

### `menu_imchiclet_adhoc.xml`

定義: `indra/newview/skins/default/xui/en/menu_imchiclet_adhoc.xml`

- IMChiclet AdHoc Menu [初期非表示]
  - セッション終了 / End Session

### `menu_imchiclet_group.xml`

定義: `indra/newview/skins/default/xui/en/menu_imchiclet_group.xml`

- IMChiclet Group Menu [初期非表示]
  - プロフィールを表示 / Group Info
  - セッションを表示 / Show Session
  - セッション終了 / End Session

### `menu_imchiclet_p2p.xml`

定義: `indra/newview/skins/default/xui/en/menu_imchiclet_p2p.xml`

- IMChiclet P2P Menu [初期非表示]
  - プロフィールの表示 / View Profile
  - フレンドに追加 / Add Friend
  - セッションを表示 / Show Session
  - セッション終了 / End Session

### `menu_inspect_avatar_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_inspect_avatar_gear.xml`

- Gear Menu [初期非表示]
  - プロフィールの表示 / View Profile
  - フレンド登録 / Add Friend [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - テレポート / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - グループに招待 / Invite to Group
  - ブロック / Block [表示条件あり]
  - ブロック解除 / Unblock [表示条件あり]
  - 嫌がらせの報告 / Report
  - フリーズ / Freeze [表示条件あり]
  - 追放 / Eject [表示条件あり]
  - キック / Kick [表示条件あり]
  - ＣＳＲ / CSR [表示条件あり]
  - テクスチャのデバッグ / Debug Textures [表示条件あり]
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - マップ上で探す / Find On Map [表示条件あり]
  - ズームイン / Zoom In [表示条件あり]
  - 支払う / Pay [有効条件あり]
  - 共有 / Share

### `menu_inspect_object_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_inspect_object_gear.xml`

- Gear Menu [初期非表示]
  - 触る / Touch [表示条件あり]
  - 座る / Sit [表示条件あり]
  - 支払う / Pay [表示条件あり]
  - 買う / Buy [表示条件あり]
  - 取る / Take [有効条件あり]
  - コピーを取る / Take Copy [表示条件あり]
  - 開く / Open [表示条件あり]
  - 編集 / Edit [有効条件あり]
  - 装着 / Wear [表示条件あり]
  - 追加 / Add [表示条件あり]
  - 嫌がらせの報告 / Report
  - ブロック / Block [表示条件あり]
  - ブロック解除 / Unblock [表示条件あり]
  - ズームイン / Zoom In
  - 削除 / Remove [表示条件あり]
  - 詳細 / More Info

### `menu_inspect_self_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_inspect_self_gear.xml`

- Gear Menu
  - 座る / Sit Down [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - 取り外す / Take Off
    - 衣装 / Clothes
      - シャツ / Shirt [有効条件あり]
      - ズボン / Pants [有効条件あり]
      - スカート / Skirt [有効条件あり]
      - 靴 / Shoes [有効条件あり]
      - 靴下 / Socks [有効条件あり]
      - ジャケット / Jacket [有効条件あり]
      - 手袋 / Gloves [有効条件あり]
      - 下着（下半身） / Undershirt [有効条件あり]
      - 下着（上半身） / Underpants [有効条件あり]
      - タトゥ / Tattoo [有効条件あり]
      - アルファ / Alpha [有効条件あり]
      - すべての衣装 / All Clothes
    - ＨＵＤ / HUD
    - 取り外す / Detach
    - すべて取り外す / Detach All [有効条件あり]
  - アウトフィットの変更 / Change Outfit [有効条件あり]
  - アウトフィットの編集 / Edit Outfit [有効条件あり]
  - シェイプの編集 / Edit Shape [有効条件あり]
  - フレンド / Friends
  - グループ / Groups
  - プロフィール / Profile
  - テクスチャのデバッグ / Debug Textures [表示条件あり]
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]

### `menu_inv_offer_chiclet.xml`

定義: `indra/newview/skins/default/xui/en/menu_inv_offer_chiclet.xml`

- InvOfferChiclet Menu [初期非表示]
  - 閉じる / Close

### `menu_inventory.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory.xml`

- Popup [初期非表示]
  - 出品リストを作成 / Create listing
  - 出品リストを関連付け / Associate listing
  - 出品リストを取得 (更新) / Get (Refresh) listing
  - エラーを確認 / Check for errors
  - 出品リストを編集 / Edit listing
  - 出品リスト / List
  - 出品リストから取り除く / Unlist
  - 有効化 / Activate
  - 無効化 / Deactivate
  - 共有 / Share
  - 開く / Open
  - 再生 / Play
  - プロパティ / Properties
  - 名前の変更 / Rename
  - 削除 / Delete
  - ごみ箱を空に / Empty Trash
  - 遺失物フォルダを空に / Empty Lost And Found
  - 新しいフォルダ / New Folder
  - 新しいフォルダ / New Folder
  - 新しいフォルダ / New Folder
  - 新しいアウトフィット / New Outfit
  - 新しいスクリプト / New Script
  - 新しいノートカード / New Notecard
  - 新しいジェスチャー / New Gesture
  - 新しいマテリアル / New Material [有効条件あり]
  - 新しい衣装 / New Clothes
    - 新しいシャツ / New Shirt
    - 新しいズボン / New Pants
    - 新しい靴 / New Shoes
    - 新しい靴下 / New Socks
    - 新しいジャケット / New Jacket
    - 新しいスカート / New Skirt
    - 新しい手袋 / New Gloves
    - 新しい下着（上半身） / New Undershirt
    - 新しい下着（下半身） / New Underpants
    - 新しいアルファ / New Alpha Mask
    - 新しいタトゥ / New Tattoo
    - 新しいユニバーサル / New Universal
    - 新しい物理作用 / New Physics
  - 新しい身体部位 / New Body Parts
    - 新しいシェイプ（体型） / New Shape
    - 新しいスキン / New Skin
    - 新しい髪 / New Hair
    - 新しい目 / New Eyes
  - 新しい自然環境の設定 / New Settings
    - 新しい空 / New Sky [有効条件あり]
    - 新しい水面 / New Water [有効条件あり]
    - 新しいデイサイクル / New Day Cycle [有効条件あり]
  - 以下のデフォルトとして使用 / Use as default for
    - 画像のアップロード / Image uploads [切替]
    - サウンドのアップロード / Sound uploads [切替]
    - アニメーションのアップロード / Animation uploads [切替]
    - モデルのアップロード / Model uploads [切替]
    - ＰＢＲマテリアルのアップロード / PBR material uploads [切替]
  - 種類を変更 / Change Type
    - デフォルト / Default
    - 手袋 / Gloves
    - ジャケット / Jacket
    - ズボン / Pants
    - シェイプ / Shape
    - 靴 / Shoes
    - シャツ / Shirt
    - スカート / Skirt
    - 下着（上半身） / Underpants
    - 下着（下半身） / Undershirt
  - テレポート / Teleport
  - 開く / Open
  - 開く / Open
  - 現在のアウトフィットと置き換え / Replace Current Outfit
  - アイテムを追加 / Add To Current Outfit
  - アイテムを着用 / Wear Items
  - アイテムを脱ぐ / Remove From Current Outfit
  - アウトフィットリストをクリップボードにコピー / Copy outfit list to clipboard
  - オリジナルを探す / Find Original
  - アイテムを削除 / Purge Item
  - アイテムを復元 / Restore Item
  - 開く / Open
  - オリジナルを開く / Open Original
  - プロパティ / Properties
  - 名前を変更 / Rename
  - 画像… / Image...
  - 保護 / Protect
  - 保護解除 / Unprotect
  - フォルダのリロード / Reload Folder
  - アセットのＵＵＩＤをコピー / Copy Asset UUID
  - ＵＵＩＤをコピー / Copy UUID
  - 最後の位置に復元 / Restore to Last Position
  - メインパネルで表示 / Show in Main View
  - 新しいウィンドウで表示 / Show in new Window
  - 開く / Open
  - 新しいウィンドウで開く / Open in new window
  - コピー / Copy
  - カット / Cut
  - ペースト / Paste
  - リンクとしてペースト / Paste As Link
  - すべてのリンクを探す / Find All Links
  - リンクを置き換える / Replace Links
  - 削除 / Delete
  - 破損したリンクをクリーンアップ / Cleanup broken Links
  - デフォルトのフォルダに移動 / Move to Default Folder
  - 遺失物に移動 / Move to Lost And Found
  - システムフォルダを削除 / Delete System Folder
  - 選択したもので新しいフォルダを作る / Create Folder From Selected
  - フォルダの項目のグループ解除 / Ungroup Folder Items
  - お気に入りに追加 / Add to Favorites
  - お気に入りから削除 / Remove from Favorites
  - 会話チャットを開始 / Start Conference Chat
  - 再生 / Play
  - SLurlをコピー / Copy SLurl
  - ランドマークの情報 / About Landmark
  - マップに表示 / Show on Map
  - インワールドで再生 / Play Inworld
  - ローカルで再生 / Play Locally
  - インスタントメッセージを送信 / Send Instant Message
  - テレポートをオファー… / Offer Teleport...
  - テレポートをリクエスト… / Request Teleport...
  - 会話チャットを開始 / Start Conference Chat
  - 再生 / Play
  - 有効化 / Activate
  - 無効化 / Deactivate
  - 別名で保存 / Save As
  - 選択を別名で保存 / Save to
  - 装着 / Wear
  - 追加 / Add
  - 装着先 / Attach To
  - ＨＵＤ装着先 / Attach To HUD
  - 触る / Touch
  - 編集 / Edit
  - テクスチャをリフレッシュ / Texture Refresh
  - 取り外す / Detach From Yourself
  - 脱ぐ / Take Off
  - 自分にのみ適用 / Apply Only To Myself
  - 区画に適用 / Apply To Parcel
  - マーケットプレイスの出品リストにコピー / Copy to Marketplace Listings
  - マーケットプレイスの出品リストに移動 / Move to Marketplace Listings
  - －オプション無し－ / --no options--

### `menu_inventory_add.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory_add.xml`

- menu_inventory_add [初期非表示]
  - アップロード / Upload
    - 画像 （L$ [COST]）… / Image... [有効条件あり]
    - サウンド （L$ [COST]）… / Sound (L$[COST])... [有効条件あり]
    - アニメーション （L$ [COST]）… / Animation (L$[COST])... [有効条件あり]
    - モデル… / Model... [表示条件あり・有効条件あり]
    - マテリアル… / Material... [有効条件あり]
    - 一括… / Bulk...
    - リンクセットをインポート… / Import Linkset...
  - 新しいフォルダ / New Folder
  - 新しいスクリプト / New Script
  - 新しいノートカード / New Notecard
  - 新しいジェスチャー / New Gesture
  - 新しいマテリアル / New Material [有効条件あり]
  - 新しい衣装 / New Clothes
    - 新しいシャツ / New Shirt
    - 新しいズボン / New Pants
    - 新しい靴 / New Shoes
    - 新しい靴下 / New Socks
    - 新しいジャケット / New Jacket
    - 新しいスカート / New Skirt
    - 新しい手袋 / New Gloves
    - 新しい下着（上半身） / New Undershirt
    - 新しい下着（下半身） / New Underpants
    - 新しいアルファ / New Alpha
    - 新しいタトゥ / New Tattoo
    - 新しいユニバーサル / New Universal
    - 新しい物理作用 / New Physics
  - 新しい身体部位 / New Body Parts
    - 新しいシェイプ / New Shape
    - 新しいスキン / New Skin
    - 新しい髪 / New Hair
    - 新しい目 / New Eyes
  - 新しい自然環境の設定 / New Settings
    - 新しい空 / New Sky [有効条件あり]
    - 新しい水面 / New Water [有効条件あり]
    - 新しいデイサイクル / New Day Cycle [有効条件あり]
  - ショップ… / Shop...

### `menu_inventory_custom_tab.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory_custom_tab.xml`

- menu_inventory_custom_tab [初期非表示]
  - 名前を変更... / Rename...
  - 複製 / Clone
  - 閉じる / Close

### `menu_inventory_gear_default.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory_gear_default.xml`

- menu_gear_default [初期非表示]
  - 新しいインベントリウィンドウを開く / New Inventory Window
  - 保護フォルダ / Protected Folders [切替]
  - インベントリ設定… / Inventory settings... [切替]
  - 一覧表示 / List view [切替・表示条件あり]
  - ギャラリー表示 / Gallery view [切替・表示条件あり]
  - 複合表示 / Combination view [切替・表示条件あり]
  - 名前で並べ替え / Sort by Name [切替]
  - 新着で並べ替え / Sort by Most Recent [切替]
  - フォルダは常に名前で並べ替え / Sort Folders Always by Name [切替]
  - システムフォルダは上に並べ替え / Sort System Folders to Top [切替・表示条件あり]
  - 検索… / Search By...
    - 名前 / Name [切替]
    - 制作者 / Creator [切替]
    - 説明 / Description [切替]
    - ＵＵＩＤ / UUID [切替]
    - 全て / All [切替]
  - オブジェクトをダブルクリックで追加 / Add objects on double click [切替]
  - 衣装をダブルクリックで追加 / Add clothes on double click [切替]
  - フィルタを表示… / Show Filters...
  - フィルタをリセット… / Reset Filters
  - すべてのフォルダを折り畳む / Collapse All Folders [表示条件あり]
  - すべてのウィンドウを閉じる / Close All Windows
  - 遺失物を空にする / Empty Lost and Found [表示条件あり・有効条件あり]
  - 名前をつけてテクスチャを保存 / Save Texture As [有効条件あり]
  - 共有 / Share [有効条件あり]
  - オリジナルを表示 / Find Original [有効条件あり]
  - すべてのリンクを表示 / Find All Links [有効条件あり]
  - リンクを置き換える / Replace Links
  - リンクを表示 / Show Links [切替]
  - リンクのみを表示 / Show Only Links [切替]
  - リンクを隠す / Hide Links [切替]
  - 修正可能なものだけ表示 / Only Modifiable [切替]
  - コピー可能なものだけ表示 / Only Copyable [切替]
  - 譲渡可能なものだけ表示 / Show Transferable [切替]
  - 複合オブジェクトのみ表示 / Only Coalesced [切替]
  - ごみ箱を空にする / Empty Trash [表示条件あり・有効条件あり]

### `menu_inventory_search_visibility.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory_search_visibility.xml`

- menu_search_visibility [初期非表示]
  - アウトフィットフォルダを検索 / Search outfit folders [切替]
  - ゴミ箱を検索 / Search Trash [切替]
  - ライブラリを検索 / Search Library [切替]
  - リンクを含む / Include links [切替]

### `menu_inventory_view_default.xml`

定義: `indra/newview/skins/default/xui/en/menu_inventory_view_default.xml`

- menu_view_default [初期非表示]
  - 名前順で並べ替える / Sort by Name [切替]
  - 新着順で並べ替える / Sort by Most Recent [切替]
  - フォルダは常に名前順にして並べ替える / Sort Folders Always by Name [切替]
  - システムフォルダを上にして並べ替える / Sort System Folders to Top [切替・表示条件あり]
  - リスト表示 / List view [切替・表示条件あり]
  - ギャラリー表示 / Gallery view [切替・表示条件あり]
  - 複合表示 / Combination view [切替・表示条件あり]

### `menu_land.xml`

定義: `indra/newview/skins/default/xui/en/menu_land.xml`

- Land Pie
  - 土地情報… / About Land [有効条件あり]
  - ここに座る / Sit Here [有効条件あり]
  - この土地を購入 / Buy This Land [有効条件あり]
  - 入場許可を購入 / Buy Pass [有効条件あり]
  - 作成 / Build [有効条件あり]
  - 地形を編集 / Edit Terrain [有効条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [有効条件あり]

### `menu_landmark.xml`

定義: `indra/newview/skins/default/xui/en/menu_landmark.xml`

- landmark_overflow_menu
  - SLurlをコピー / Copy SLurl
  - 削除 / Delete
  - ピックを作成 / Create Pick [有効条件あり]
  - お気に入りバーに追加 / Add to Favorites Bar

### `menu_lsl_font_size.xml`

定義: `indra/newview/skins/default/xui/en/menu_lsl_font_size.xml`

- menu_font_size [初期非表示]
  - 小 / Small [切替]
  - デフォルト / Default [切替]
  - 中 / Medium [切替]
  - 大 / Large [切替]
  - 最大 / Huge [切替]

### `menu_marketplace_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_marketplace_view.xml`

- menu_marketplace_sort [初期非表示]
  - 名前順で並べ替え / Sort by name [切替]
  - 新着順で並べ替え / Sort by most recent [切替]
  - 在庫数順で並べ替え（少から多へ） / Sort by stock amount (low to high) [切替]
  - 出品リストフォルダのみ表示 / Show only listing folders [切替]

### `menu_media_ctrl.xml`

定義: `indra/newview/skins/default/xui/en/menu_media_ctrl.xml`

- media ctrl context menu
  - カット / Cut [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - Webインスペクタを開く / Open Web Inspector [初期非表示]
  - ソースを表示 / Show Source [初期非表示]

### `menu_mini_map.xml`

定義: `indra/newview/skins/default/xui/en/menu_mini_map.xml`

- ミニマップ / Mini-Map [初期非表示]
  - プロフィールを閲覧 / View Profile
  - プロフィールを閲覧 / View Profile
  - コンタクトセットに追加 / Add to Set
  - カメラ / Cam
  - Face towards avatar [有効条件あり]
  - マーク… / Mark...
    - 赤 / Red
    - 緑 / Green
    - 青 / Blue
    - 紫 / Purple
    - 黄 / Yellow
    - マークをクリア / Clear Mark(s)
    - 全てのマークをクリア / Clear all Marks
  - その他のオプション… / More Options...
    - フレンドに追加 / Add Friend [有効条件あり]
    - コンタクトセットに追加 / Add to Set
    - フレンド削除 / Remove Friend [有効条件あり]
    - ＩＭ / IM
    - コール / Call [有効条件あり]
    - マップ / Map [有効条件あり]
    - 共有 / Share [有効条件あり]
    - 支払う / Pay
    - テレポートのオファー / Offer Teleport [有効条件あり]
    - テレポートのリクエスト / Request Teleport [有効条件あり]
    - テレポート先 / Teleport To
    - グループに招待 / Invite To Group
    - スクリプト情報を取得 / Get Script Info [有効条件あり]
    - ブロック／ブロック解除 / Block/Unblock [切替・有効条件あり]
    - 嫌がらせの報告 / Report
    - フリーズ / Freeze [表示条件あり]
    - 区画から追放 / Parcel Eject [表示条件あり]
    - 不動産からキック / Estate Kick [表示条件あり]
    - 不動産からホームにテレポート / Estate Teleport Home [表示条件あり]
    - 不動産からバン / Estate Ban [表示条件あり]
    - 描画解除 / Derender
    - 描画解除＋ブラックリスト / Derender + Blacklist
  - 追跡開始 / Start Tracking
  - 追跡停止 / Stop Tracking
  - ズーム / Zoom
    - かなり近く / Very close [切替]
    - 近く / Close [切替]
    - 中（デフォルト） / Medium (Default) [切替]
    - 遠く / Far [切替]
  - 表示 / Show
    - オブジェクト / Objects [切替]
    - 物理的なオブジェクト / Physical Objects [切替・有効条件あり]
    - スクリプト化されたオブジェクト / Scripted Objects [切替・有効条件あり]
    - 一時的にRezされたオブジェクト / Temp On Rez Objects [切替・有効条件あり]
    - 物件のライン / Property Lines [切替]
    - 販売中の区画 / Parcels for Sale [切替・有効条件あり]
  - 北を上に / North at top [切替]
  - カメラの向きを上に / Camera at top [切替]
  - マップの自動センター / Auto-center map [切替]
  - マップの再センター / Re-center map
  - チャットの距離のリング / Chat Distance Rings
    - チャットの届く範囲のリングを表示 / Show Chat Distance Rings [切替]
    - ささやきが届く距離のリングを表示 / Show Whisper Distance Ring [切替]
    - 通常チャットが届く距離のリングを表示 / Show Chat Distance Ring [切替]
    - シャウトが届く距離のリングを表示 / Show Shout Distance Ring [切替]
  - この土地について / About Land
  - 場所のプロフィール / Place Profile
  - ワールドマップ / World Map

### `menu_model_import_gear_default.xml`

定義: `indra/newview/skins/default/xui/en/menu_model_import_gear_default.xml`

- model_menu_gear_default [初期非表示]
  - 縁を表示 / Show edges [切替・有効条件あり]
  - 物理効果を表示 / Show physics [切替・有効条件あり]
  - テクスチャを表示 / Show textures [切替・有効条件あり]
  - スキンのウエイトを表示 / Show skin weight [切替・有効条件あり]
  - ジョイントポジションを表示 / Show joint positions [切替・有効条件あり]

### `menu_mute_particle.xml`

定義: `indra/newview/skins/default/xui/en/menu_mute_particle.xml`

- Mute Particle Pie
  - パーティクル所有者をブロック / Block Particle Owner [有効条件あり]

### `menu_navbar.xml`

定義: `indra/newview/skins/default/xui/en/menu_navbar.xml`

- Navbar Menu [初期非表示]
  - 座標を表示 / Show Coordinates [切替]
  - 区画のプロパティを表示 / Show Parcel Properties [切替]
  - ランドマーク / Landmark
  - カット / Cut [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - 削除 / Delete [有効条件あり]
  - すべて選択 / Select All [有効条件あり]

### `menu_nearby_chat.xml`

定義: `indra/newview/skins/default/xui/en/menu_nearby_chat.xml`

- NearBy Chat Menu [初期非表示]
  - 近くの人物を表示する… / Show Nearby People...
  - ブロックされた文字を表示 / Show Blocked Text [切替]
  - バディアイコンを表示 / Show Buddy Icons [切替]
  - 名前を表示 / Show Names [切替]
  - アイコンと名前を表示 / Show Icons and Names [切替]
  - 文字の大きさ / Font Size

### `menu_nearby_media.xml`

定義: `indra/newview/skins/default/xui/en/menu_nearby_media.xml`

- menu_nearby_media [初期非表示]
  - ＵＲＬをコピー / Copy Url
  - データをコピー / Copy Data [表示条件あり]

### `menu_notification_well_button.xml`

定義: `indra/newview/skins/default/xui/en/menu_notification_well_button.xml`

- Notification Well Button Context Menu
  - すべて閉じる / Close All [有効条件あり]

### `menu_object.xml`

定義: `indra/newview/skins/default/xui/en/menu_object.xml`

- Object Pie
  - 触る / Touch [有効条件あり]
  - 編集… / Edit [有効条件あり]
  - ＰＢＲマテリアルを編集 / Edit PBR Material [有効条件あり]
  - ビルド / Build [有効条件あり]
  - 開く / Open [有効条件あり]
  - ここに座る / Sit here [有効条件あり]
  - 立ち上がる / Stand up [有効条件あり]
  - オブジェクト / Object
    - プロフィール / Profile [有効条件あり]
    - 調査 / Inspect
    - スクリプト情報 / Script Info [有効条件あり]
    - リンク / Link [有効条件あり]
    - リンク解除 / Unlink [有効条件あり]
    - リンクされたパーツを編集 / Edit Linked Parts [切替・有効条件あり]
    - コンパイル（Ｍｏｎｏ） / Compile (Mono) [有効条件あり]
    - コンパイル（ＬＳＬ） / Compile (LSL) [有効条件あり]
    - スクリプトのリセット / Reset Scripts [有効条件あり]
    - スクリプトの実行 / Run Scripts [有効条件あり]
    - スクリプトの停止 / Stop Scripts [有効条件あり]
    - スクリプトの削除 / Remove Scripts [有効条件あり]
  - ズームイン / Zoom in
  - リンクセットで表示 / Show in Region Objects [表示条件あり・有効条件あり]
  - キャラクターで表示 / Show in characters [表示条件あり・有効条件あり]
  - 取り付ける / Put on
    - 着用 / Wear [有効条件あり]
    - 追加 / Add [有効条件あり]
    - 装着 / Attach
    - ＨＵＤに装着 / Attach HUD
  - 管理 / Annoyance
    - 嫌がらせの報告… / Report Abuse [有効条件あり]
    - ブロック / Block [表示条件あり]
    - ブロック解除 / Unblock [表示条件あり]
  - テクスチャのリフレッシュ / Texture Refresh
  - 描画解除 / Derender
  - 描画解除＋ブラックリスト / Derender + Blacklist
  - 返却… / Return [有効条件あり]
  - 取る / Take [表示条件あり・有効条件あり]
  - コピーを取る / Take copy [表示条件あり・有効条件あり]
  - 複数取る / Take multiple
    - 複合アイテムとして取る / Take as combined item [表示条件あり・有効条件あり]
    - 複合アイテムとしてコピー / Take copy as combined item [表示条件あり・有効条件あり]
    - 別個のアイテムとして取る / Take as separate items [表示条件あり・有効条件あり]
    - 別個のアイテムとしてコピー / Take copies as separate items [表示条件あり・有効条件あり]
  - 支払う… / Pay [有効条件あり]
  - 購入… / Buy [有効条件あり]
  - 名前をつけて保存 / Save as
    - バックアップ / Backup [有効条件あり]
    - Collada [有効条件あり]
  - 削除 / Delete [有効条件あり]
  - パーティクル所有者をブロック / Block Particle Owner [有効条件あり]
  - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
  - スケルトンをリセット / Reset Skeleton [表示条件あり]

### `menu_object_icon.xml`

定義: `indra/newview/skins/default/xui/en/menu_object_icon.xml`

- Object Icon Menu [初期非表示]
  - オブジェクトのプロフィール… / Object Profile...
  - ブロック… / Block... [表示条件あり]
  - ブロック解除 / Unblock [表示条件あり]
  - ズーム / Zoom in [有効条件あり]
  - マップに表示 / Show on Map
  - オブジェクトの場所にテレポート / Teleport to Object Location

### `menu_outfit_gallery_sort.xml`

定義: `indra/newview/skins/default/xui/en/menu_outfit_gallery_sort.xml`

- Sort Outfit [初期非表示]
  - お気に入りを上で並べ替え / Sort favorites to top [切替]
  - 画像を上で並べ替え / Sort images to top [切替]
  - 名前で並べ替え / Sort by name [切替]

### `menu_outfit_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_outfit_gear.xml`

- Gear Outfit [初期非表示]
  - 現在アウトフィットと置き換え / Replace Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットにアイテムを追加 / Add to Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットのアイテムを脱ぐ / Remove from Current Outfit [表示条件あり・有効条件あり]
  - 画像… / Image...
  - お気に入りのアウトフィットに追加 / Add to favorite outfits [表示条件あり]
  - お気に入りのアウトフィットから削除 / Remove from favorite outfits [表示条件あり]
  - アウトフィットの名前変更 / Rename outfit [表示条件あり・有効条件あり]
  - このアウトフィットを保存 / Save to this outfit [表示条件あり]
  - アウトフィットを削除 / Delete outfit [表示条件あり・有効条件あり]
  - New Clothes
    - New Shirt
    - New Pants
    - New Shoes
    - New Socks
    - New Jacket
    - New Skirt
    - New Gloves
    - New Undershirt
    - New Underpants
    - New Alpha
    - New Physics
    - New Tattoo
    - New Universal
  - 新しい身体部位 / New Body Parts
    - 新しいシェイプ / New Shape
    - 新しいスキン / New Skin
    - 新しい髪型 / New Hair
    - 新しい目 / New Eyes
  - インベントリ設定… / Inventory settings... [切替]

### `menu_outfit_list_sort.xml`

定義: `indra/newview/skins/default/xui/en/menu_outfit_list_sort.xml`

- Sort Outfit [初期非表示]
  - すべてのフォルダを拡げる / Expand all folders
  - すべてのフォルダを折り畳む / Collapse all folders
  - お気に入りを上に並べ替える / Sort favorites to top [切替]
  - 検索でアウトフィット全体を表示する / Show entire outfit in search [切替]

### `menu_outfit_tab.xml`

定義: `indra/newview/skins/default/xui/en/menu_outfit_tab.xml`

- Outfit
  - 現在のアウトフィットと置き換え / Replace Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットにアイテムを追加 / Add to Current Outfit [表示条件あり・有効条件あり]
  - アウトフィットのアイテムを脱ぐ / Remove from Current Outfit [表示条件あり・有効条件あり]
  - 画像… / Image...
  - お気に入りに追加 / Add to Favorites [表示条件あり]
  - お気に入りから削除 / Remove from Favorites [表示条件あり]
  - アウトフィットを編集 / Edit Outfit [表示条件あり]
  - アウトフィットの名前変更 / Rename Outfit [有効条件あり]
  - 現在のアウトフィットを保存 / Save to this Outfit
  - アウトフィットを削除 / Delete Outfit [表示条件あり]

### `menu_participant_list.xml`

定義: `indra/newview/skins/default/xui/en/menu_participant_list.xml`

- Participant List Context Menu
  - プロフィールの表示 / View Profile [有効条件あり]
  - フレンド登録 / Add Friend [有効条件あり]
  - コンタクトセットに追加 / Add to Set
  - ＩＭ / IM [有効条件あり]
  - コール / Call [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - ズームイン / Zoom In [有効条件あり]
  - 名前で並べ替え / Sort by Name [切替]
  - 最近の発言者で並べ替え / Sort by Recent Speakers [切替]
  - 人物のアイコン表示 / View People Icons [切替]
  - ボイスをブロック / Block Voice [切替・有効条件あり]
  - テキストをブロック / Block Text [切替・有効条件あり]
  - モデレーターのオプション / Moderator Options
    - グループから追放 / Eject from Group [有効条件あり]
    - Allow text chat [有効条件あり]
    - テキストチャットを禁止 / Forbid text chat [有効条件あり]
    - この参加者をミュートする / Mute this participant [有効条件あり]
    - この参加者のミュートを解除する / Unmute this participant [有効条件あり]
    - 全員をミュート / Mute everyone [有効条件あり]
    - 全員のミュート解除 / Unmute everyone [有効条件あり]
  - Ban member [有効条件あり]
  - メンション先のＵＲＩをクリップボードにコピー / Copy Mention URI to clipboard
  - メンション先のユーザをチャットに / Mention User in Chat

### `menu_participant_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_participant_view.xml`

- participant_manu_view
  - 会話をタイプで並べ替え / Sort conversations by type [切替]
  - 会話を名前で並べ替え / Sort conversations by name [切替]
  - 会話を最新アクティビティで並べ替え / Sort conversations by recent activity [切替]
  - 参加者を名前で並べ替え / Sort participants by name [切替]
  - 参加者を最新アクティビティで並べ替え / Sort participants by recent activity [切替]
  - チャットの初期設定… / Chat preferences...
  - プライバシーの初期設定… / Privacy preferences...
  - 会話ログ… / Conversation log... [切替・有効条件あり]
  - チャットを翻訳 / Translate chat [切替・有効条件あり]
  - 翻訳の設定… / Translation settings... [切替]

### `menu_people_blocked_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_blocked_gear.xml`

- menu_blocked_gear [初期非表示]
  - ブロック解除 / Unblock [有効条件あり]
  - ボイスをブロック / Block Voice [切替・表示条件あり・有効条件あり]
  - テキストをブロック / Block Text [切替・表示条件あり・有効条件あり]
  - パーティクルをブロック / Block Particles [切替・表示条件あり・有効条件あり]
  - オブジェクトのサウンドをブロック / Block Object Sounds [切替・表示条件あり・有効条件あり]
  - プロフィール… / Profile... [有効条件あり]

### `menu_people_blocked_plus.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_blocked_plus.xml`

- menu_blocked_plus [初期非表示]
  - 名前で住人をブロック… / Block Resident by name...
  - 名前でオブジェクトをブロック… / Block object by name

### `menu_people_blocked_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_blocked_view.xml`

- menu_blocked_view [初期非表示]
  - 名前で並べ替え / Sort by name [切替]
  - 名前で並べ替え（降順） / Sort by name (descending) [切替]
  - 種類で並べ替え / Sort by type [切替]
  - 種類で並べ替え（降順） / Sort by type (descending) [切替]

### `menu_people_contact_sets_add.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_contact_sets_add.xml`

- menu_people_contact_sets_add [初期非表示]
  - コンタクトセットに追加… / Add Set...
  - 連絡先に追加… / Add Contact... [有効条件あり]

### `menu_people_contact_sets_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_contact_sets_gear.xml`

- menu_people_contact_sets_gear [初期非表示]
  - プロフィールを表示 / View Profile [有効条件あり]
  - ＩＭ… / IM... [有効条件あり]
  - テレポート… / Teleport... [有効条件あり]
  - エイリアスをセット… / Set alias... [有効条件あり]
  - エイリアスを削除… / Remove alias... [表示条件あり]
  - 表示名を削除… / Remove display name... [表示条件あり]
  - コンタクトセット設定… / Configure Contact Set... [有効条件あり]

### `menu_people_contact_sets_remove.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_contact_sets_remove.xml`

- menu_people_contact_sets_remove [初期非表示]
  - コンタクトセットを削除… / Remove Set... [有効条件あり]
  - コンタクトを削除… / Remove Contact... [有効条件あり]
  - エイリアスを削除… / Remove Alias... [表示条件あり]
  - 表示名を削除… / Remove Display Name... [表示条件あり]
  - コンタクトを移動… / Move Contact... [有効条件あり]

### `menu_people_friends_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_friends_view.xml`

- menu_group_plus [初期非表示]
  - 名前で並べ替え / Sort by Name [切替]
  - ユーザ名で並べ替え / Sort by Username [切替]
  - オンライン状況で並べ替え / Sort by Status [切替]
  - 人物のアイコン表示 / View People Icons [切替]
  - 付与された権限の表示 / View Permissions Granted [切替]
  - 会話ログを表示… / View Conversation Log... [切替・有効条件あり]

### `menu_people_groups.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_groups.xml`

- menu_group_plus [初期非表示]
  - アクティブ / Activate [有効条件あり]
  - 情報を表示 / View Info [有効条件あり]
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard [有効条件あり]
  - チャット / Chat [有効条件あり]
  - ボイスコール / Voice call [有効条件あり]
  - グループをピン / Pin group [表示条件あり・有効条件あり]
  - グループのピン解除 / Unpin group [表示条件あり・有効条件あり]
  - 脱退 / Leave [有効条件あり]

### `menu_people_groups_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_groups_view.xml`

- menu_group_plus [初期非表示]
  - グループアイコンを表示 / Display Group Icons [切替]

### `menu_people_nearby.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_nearby.xml`

- Nearby People Context Menu
  - プロフィールを表示 / View Profile
  - ＩＭ / IM [有効条件あり]
  - テレポートを送る / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - ボイスコール / Voice call [有効条件あり]
  - チャットの履歴を表示… / View chat transcripts... [有効条件あり]
  - フレンドに追加 / Add Friend [表示条件あり]
  - コンタクトセットに追加… / Add to Set...
  - フレンドから削除 / Remove Friend [表示条件あり]
  - グループに招待… / Invite to group... [有効条件あり]
  - ズームイン / Zoom In [有効条件あり]
  - マップ / Map [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - ブロック／ブロック解除 / Block/Unblock [切替・有効条件あり]
  - フリーズ / Freeze [表示条件あり]
  - 追放 / Eject [表示条件あり]

### `menu_people_nearby_multiselect.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_nearby_multiselect.xml`

- Multi-Selected People Context Menu
  - フレンドに追加 / Add Friends [有効条件あり]
  - コンタクトセットに追加… / Add to Set...
  - フレンドから削除 / Remove Friends [有効条件あり]
  - ＩＭ / IM
  - コール / Call [有効条件あり]
  - 共有 / Share [有効条件あり]
  - 支払う / Pay [有効条件あり]
  - テレポートをオファー / Offer Teleport [有効条件あり]

### `menu_people_nearby_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_nearby_view.xml`

- menu_group_plus [初期非表示]
  - 最近の発言者で並べ替え / Sort by Recent Speakers [切替]
  - 名前で並べ替え / Sort by Name [切替]
  - 距離で並べ替え / Sort by Distance [切替]
  - 到着順で並べ替え / Sort by Recent arrival [切替]
  - 人物のアイコン表示 / View People Icons [切替]
  - マップを表示 / View Map [切替]

### `menu_people_recent_view.xml`

定義: `indra/newview/skins/default/xui/en/menu_people_recent_view.xml`

- menu_group_plus [初期非表示]
  - 新着順で並べ替え / Sort by Most Recent [切替]
  - 名前で並べ替え / Sort by Name [切替]
  - 人物アイコンを表示 / View People Icons [切替]

### `menu_perf_avatar_rendering_settings.xml`

定義: `indra/newview/skins/default/xui/en/menu_perf_avatar_rendering_settings.xml`

- Settings
  - 常に描画 / Always full detail [切替]
  - 描画しない / Never full detail [切替]
  - 例外から削除 / Remove from exceptions [切替・表示条件あり]
  - アバターの調査 / Inspect Avatar
  - アバターをズーム / Zoom to Avatar

### `menu_picks.xml`

定義: `indra/newview/skins/default/xui/en/menu_picks.xml`

- Picks
  - テレポート / Teleport [有効条件あり]
  - ピックの情報 / View Pick [有効条件あり]
  - ピックを編集 / Edit Pick [初期非表示・有効条件あり]
  - マップに表示 / Show on map [有効条件あり]
  - 削除 / Delete [初期非表示・有効条件あり]

### `menu_picks_plus.xml`

定義: `indra/newview/skins/default/xui/en/menu_picks_plus.xml`

- picks_plus_menu
  - 新しいピック / New Pick [有効条件あり]
  - 新しいクラシファイド広告 / New Classified

### `menu_place.xml`

定義: `indra/newview/skins/default/xui/en/menu_place.xml`

- place_overflow_menu
  - SLurlをコピー / Copy SLurl
  - ランドマークを作成 / Make a Landmark
  - ピックを作成 / Create Pick [有効条件あり]

### `menu_place_add_button.xml`

定義: `indra/newview/skins/default/xui/en/menu_place_add_button.xml`

- menu_create [初期非表示]
  - 現在の場所にランドマーク作成 / Landmark current location [有効条件あり]
  - フォルダ作成 / Create folder [有効条件あり]

### `menu_places_gear_folder.xml`

定義: `indra/newview/skins/default/xui/en/menu_places_gear_folder.xml`

- menu_folder_gear [初期非表示]
  - ランドマークを追加 / Landmark current location [有効条件あり]
  - フォルダを追加 / Create subfolder [表示条件あり・有効条件あり]
  - アイテムを復元 / Restore Item
  - カット / Cut [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - 名前の変更 / Rename [有効条件あり]
  - 削除 / Delete [有効条件あり]
  - 拡げる / Expand [表示条件あり・有効条件あり]
  - 折りたたむ / Collapse [表示条件あり・有効条件あり]

### `menu_places_gear_landmark.xml`

定義: `indra/newview/skins/default/xui/en/menu_places_gear_landmark.xml`

- menu_ladmark_gear [初期非表示]
  - テレポート / Teleport [有効条件あり]
  - マップ上に表示 / Show on Map [有効条件あり]
  - 共有 / Share [有効条件あり]
  - ランドマークの表示／編集 / View/Edit Landmark [有効条件あり]
  - ランドマークの場所に移動 / Move to Landmarks [表示条件あり]
  - お気に入りへ移動する / Move to Favorites [表示条件あり]
  - SLurlをコピー / Copy SLurl [有効条件あり]
  - ピックを作成 / Create Pick [有効条件あり]
  - アイテムを復元 / Restore Item
  - カット / Cut [有効条件あり]
  - コピー / Copy
  - ペースト / Paste [有効条件あり]
  - 名前の変更 / Rename [有効条件あり]
  - 削除 / Delete [有効条件あり]

### `menu_places_gear_sorting.xml`

定義: `indra/newview/skins/default/xui/en/menu_places_gear_sorting.xml`

- menu_sorter_gear [初期非表示]
  - 全てのフォルダを開く / Expand all folders [有効条件あり]
  - 全てのフォルダを閉じる / Collapse all folders [有効条件あり]
  - 日付順で並べ替える / Sort by Date [切替・有効条件あり]

### `menu_profile_other.xml`

定義: `indra/newview/skins/default/xui/en/menu_profile_other.xml`

- Avatar Profile Menu
  - ＩＭ / IM
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - ボイスコール / Voice call [有効条件あり]
  - チャット履歴を表示… / View chat history... [有効条件あり]
  - フレンドに追加 / Add Friend [表示条件あり]
  - フレンドから削除 / Remove Friend [有効条件あり]
  - グループに招待… / Invite to group...
  - 権限 / Permissions [表示条件あり]
  - マップ / Map [有効条件あり]
  - 共有 / Share
  - 支払う / Pay
  - ブロック／解除 / Block/Unblock [切替・有効条件あり]
  - 表示名をコピー / Copy Display Name [有効条件あり]
  - エージェント名をコピー / Copy Agent Name [有効条件あり]
  - ＵＵＩＤをコピー / Copy Agent Id

### `menu_profile_self.xml`

定義: `indra/newview/skins/default/xui/en/menu_profile_self.xml`

- Avatar Profile Menu Self
  - 表示名を編集 / Edit Display Name
  - パートナーを編集 / Edit Partner
  - 画像をアップロード / Upload Photo [有効条件あり]
  - 画像を変更 / Change Photo [有効条件あり]
  - 画像を削除 / Remove Photo [有効条件あり]
  - 表示名をコピー / Copy Display Name [有効条件あり]
  - エージェント名をコピー / Copy Agent Name [有効条件あり]
  - ＵＵＩＤをコピー / Copy Agent Id

### `menu_save_outfit.xml`

定義: `indra/newview/skins/default/xui/en/menu_save_outfit.xml`

- save_outfit_menu
  - 保存 / Save
  - 別名で保存 / Save As

### `menu_save_settings.xml`

定義: `indra/newview/skins/default/xui/en/menu_save_settings.xml`

- save_settings_menu
  - 保存 / Save [切替]
  - 別名で保存 / Save As [切替]
  - 変更をコミット / Commit [切替]
  - 自分のみに適用 / Apply Only To Myself [切替]
  - 区画に適用 / Apply To Parcel [切替]
  - リージョンに適用 / Apply To Region [切替]

### `menu_script_chiclet.xml`

定義: `indra/newview/skins/default/xui/en/menu_script_chiclet.xml`

- ScriptChiclet Menu [初期非表示]
  - 閉じる / Close
  - 全てのダイアログを閉じる / Close All Dialogs

### `menu_settings_add.xml`

定義: `indra/newview/skins/default/xui/en/menu_settings_add.xml`

- menu_settings_add [初期非表示]
  - 新しい空 / New Sky [有効条件あり]
  - 新しい水面 / New Water [有効条件あり]
  - 新しいデイサイクル / New Day Cycle [有効条件あり]

### `menu_settings_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_settings_gear.xml`

- menu_settings_gear [初期非表示]
  - 編集 / Edit [有効条件あり]
  - 自分にのみ適用 / Apply Only To Myself [有効条件あり]
  - 区画に適用 / Apply To Parcel [有効条件あり]
  - リージョンに適用 / Apply To Region [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - アセットのＵＵＩＤをコピー / Copy UUID [有効条件あり]

### `menu_slurl.xml`

定義: `indra/newview/skins/default/xui/en/menu_slurl.xml`

- Popup
  - ＵＲＬについて / About URL
  - ＵＲＬにテレポートする / Teleport to URL
  - マップ / Map

### `menu_teleport_history_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_teleport_history_gear.xml`

- Teleport History Gear Context Menu [初期非表示]
  - すべてのフォルダを拡げる / Expand all folders [有効条件あり]
  - すべてのフォルダを折りたたむ / Collapse all folders [有効条件あり]
  - タイムゾーンを選択… / Select Timezone...
    - ＵＴＣ / UTC [切替]
    - ＳＬＴ / SLT [切替]
    - ローカル / Local [切替]
  - 表示… / Show...
    - 位置 / Position [切替]
    - 日付 / Date [切替]
  - テレポート履歴をクリア / Clear Teleport History [有効条件あり]

### `menu_teleport_history_item.xml`

定義: `indra/newview/skins/default/xui/en/menu_teleport_history_item.xml`

- Teleport History Item Menu [初期非表示]
  - テレポート / Teleport [有効条件あり]
  - より詳細な情報 / View [有効条件あり]
  - マップに表示 / Show on map [有効条件あり]
  - SLurlをコピー / Copy SLurl [有効条件あり]
  - 履歴から削除 / Remove from history [有効条件あり]

### `menu_teleport_history_tab.xml`

定義: `indra/newview/skins/default/xui/en/menu_teleport_history_tab.xml`

- Teleport History Item Context Menu
  - 開く / Open
  - 閉じる / Close

### `menu_teleport_history_timezone.xml`

定義: `indra/newview/skins/default/xui/en/menu_teleport_history_timezone.xml`

- menu_teleport_history_timezone [初期非表示]
  - ＵＴＣ / UTC [切替]
  - ＳＬＴ / SLT [切替]
  - ローカル / Local [切替]

### `menu_text_editor.xml`

定義: `indra/newview/skins/default/xui/en/menu_text_editor.xml`

- Text editor context menu
  - （不明） / (unknown) [表示条件あり]
  - （不明） / (unknown) [表示条件あり]
  - （不明） / (unknown) [表示条件あり]
  - （不明） / (unknown) [表示条件あり]
  - （不明） / (unknown) [表示条件あり]
  - 辞書に追加 / Add to Dictionary [有効条件あり]
  - 無視に追加 / Add to Ignore [有効条件あり]
  - カット / Cut [有効条件あり]
  - コピー / Copy [有効条件あり]
  - ペースト / Paste [有効条件あり]
  - 削除 / Delete [有効条件あり]
  - すべて選択 / Select All [有効条件あり]

### `menu_toolbars.xml`

定義: `indra/newview/skins/default/xui/en/menu_toolbars.xml`

- Toolbars Popup [初期非表示]
  - このボタンを削除 / Remove this button
  - ツールバーのボタン… / Toolbar buttons...
  - アイコンとラベル / Icons and labels [切替]
  - アイコンのみ / Icons only [切替]
  - ロック / Locked [切替]

### `menu_toolbars_horizontal.xml`

定義: `indra/newview/skins/default/xui/en/menu_toolbars_horizontal.xml`

- ツールバー / Toolbar [初期非表示]
  - このボタンを削除 / Remove this button
  - ツールバーのボタン… / Toolbar buttons...
  - 左に揃える / Left Aligned [切替]
  - 中央に揃える / Center Aligned [切替]
  - 右に揃える / Right Aligned [切替]
  - アイコンとラベル / Icons and labels [切替]
  - アイコンのみ / Icons only [切替]
  - ラベルのみ / Labels only [切替]
  - ボタンサイズ自動 / Autosize Buttons [切替]
  - ボタンサイズ固定 / Fixed Size Buttons [切替]
  - バー全体にボタンを配置 / Buttons Fill Bar [切替]
  - ロック / Locked [切替]

### `menu_toolbars_vertical.xml`

定義: `indra/newview/skins/default/xui/en/menu_toolbars_vertical.xml`

- ツールバー / Toolbar [初期非表示]
  - このボタンを削除 / Remove this button
  - ツールバーのボタン… / Toolbar buttons...
  - 上に揃える / Top Aligned [切替]
  - 中央に揃える / Center Aligned [切替]
  - 下に揃える / Bottom Aligned [切替]
  - アイコンとラベル / Icons and labels [切替]
  - アイコンのみ / Icons only [切替]
  - ラベルのみ / Labels only [切替]
  - ロック / Locked [切替]

### `menu_topinfobar.xml`

定義: `indra/newview/skins/default/xui/en/menu_topinfobar.xml`

- menu_topinfobar [初期非表示]
  - 座標を表示 / Show Coordinates [切替]
  - 区画のプロパティを表示 / Show Parcel Properties [切替]
  - ランドマーク / Landmark
  - コピー / Copy

### `menu_url_agent.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_agent.xml`

- Url Popup
  - プロフィールを表示 / View Profile [有効条件あり]
  - ＩＭを送信… / Send IM... [有効条件あり]
  - チャット履歴を表示… / View chat transcripts... [有効条件あり]
  - フレンドに追加… / Add Friend... [有効条件あり]
  - コンタクトセットに追加… / Add to Set...
  - 住民をズーム / Zoom at Resident [有効条件あり]
  - 住民のところにテレポート / Teleport to Resident [有効条件あり]
  - テレポートのオファー / Offer Teleport [有効条件あり]
  - テレポートのリクエスト / Request Teleport [有効条件あり]
  - 住民を追跡 / Track Resident [有効条件あり]
  - フレンドから削除… / Remove Friend... [有効条件あり]
  - 嫌がらせの報告 / Report Abuse
  - ブロック / Block [切替・有効条件あり]
  - モデレーターオプション / Moderator Options
    - テキストチャットを許可 / Allow text chat
    - テキストチャットを禁止 / Forbid text chat
    - メンバーをバン / Ban member
    - メンバーを追放 / Eject member
  - 名前をクリップボードにコピー / Copy Name to clipboard
  - SLurlをクリップボードにコピー / Copy Url to clipboard
  - メンション先のＵＲＩをクリップボードにコピー / Copy Mention URI to clipboard
  - メンション先のユーザをチャットに / Mention User in Chat

### `menu_url_email.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_email.xml`

- Email Popup
  - 外部クライアントで電子メールを作成 / Compose Email in an External client
  - 電子メールをクリップボードにコピー / Copy Email to clipboard

### `menu_url_experience.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_experience.xml`

- Url Popup
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_group.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_group.xml`

- Url Popup
  - グループ情報を表示 / Show Group Information
  - グループデータを取得しています。もう一度お試しください！ / Fetching Group Data, please try again! [表示条件あり]
  - グループをアクティベート / Activate Group [初期非表示・表示条件あり・有効条件あり]
  - グループに加入 / Join Group [初期非表示・表示条件あり・有効条件あり]
  - グループを脱退 / Leave Group [初期非表示・表示条件あり・有効条件あり]
  - グループをクリップボードにコピー / Copy Group to clipboard
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_http.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_http.xml`

- Url Popup
  - Webページを開く / Open Web Page
  - 内蔵ブラウザで開く / Open in Internal Browser
  - 外部ブラウザで開く / Open in External Browser
  - ＵＲＬをクリップボードにコピー / Copy URL to clipboard

### `menu_url_inventory.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_inventory.xml`

- Url Popup
  - インベントリのアイテムを表示 / Show Inventory Item
  - 名前をクリップボードにコピー / Copy Name to clipboard
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_map.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_map.xml`

- Url Popup
  - マップに表示 / Show on Map
  - 現地にテレポート / Teleport to Location
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_objectim.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_objectim.xml`

- Url Popup
  - オブジェクトのプロフィール… / Object Profile...
  - ブロック… / Block...
  - ブロック解除 / Unblock
  - ズーム / Zoom in
  - マップに表示 / Show on Map
  - オブジェクトの場所にテレポート / Teleport to Object Location
  - オブジェクト名をクリップボードにコピー / Copy Object Name to clipboard
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_parcel.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_parcel.xml`

- Url Popup
  - 区画情報を表示 / Show Parcel Information
  - マップに表示 / Show on Map
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_slapp.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_slapp.xml`

- Url Popup
  - このコマンドを実行 / Run This Command
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_slurl.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_slurl.xml`

- Url Popup
  - 場所の情報を表示 / Show Place Information
  - マップに表示 / Show on Map
  - 現地にテレポート / Teleport to Location
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_url_teleport.xml`

定義: `indra/newview/skins/default/xui/en/menu_url_teleport.xml`

- Url Popup
  - この場所にテレポート / Teleport to this Location
  - マップに表示 / Show on Map
  - SLurlをクリップボードにコピー / Copy SLurl to clipboard

### `menu_wearable_list_item.xml`

定義: `indra/newview/skins/default/xui/en/menu_wearable_list_item.xml`

- Outfit Wearable Context Menu
  - 着替える / Replace [初期非表示]
  - 着る / Wear [初期非表示]
  - 追加 / Add [初期非表示]
  - 装着先 / Attach to
  - ＨＵＤに装着 / Attach to HUD
  - 触る / Touch [初期非表示]
  - 編集 / Edit [初期非表示]
  - 脱ぐ／取り外す / Take Off / Detach [初期非表示]
  - 取り外す / Detach [初期非表示]
  - 取り外す / Take Off [初期非表示]
  - アイテムのプロフィール / Item Profile [初期非表示]
  - お気に入りに追加 / Add to favorites
  - お気に入りから削除 / Remove from favorites
  - オリジナルを表示 / Show Original [初期非表示・有効条件あり]
  - アウトフィットから削除 / Delete from outfit [初期非表示]
  - 新規作成 / Create New [初期非表示]
  - －オプションなし－ / --no options-- [初期非表示]

### `menu_wearing_gear.xml`

定義: `indra/newview/skins/default/xui/en/menu_wearing_gear.xml`

- Gear Wearing [初期非表示]
  - 触る / Touch [有効条件あり]
  - 編集 / Edit [有効条件あり]
  - 脱ぐ / Take Off [有効条件あり]
  - アウトフィットの編集 / Edit Outfit
  - アウトフィットのリストをクリップボードにコピー / Copy outfit list to clipboard

### `menu_wearing_tab.xml`

定義: `indra/newview/skins/default/xui/en/menu_wearing_tab.xml`

- Wearing
  - 触る / Touch
  - 編集 / Edit
  - 脱ぐ / Take Off
  - 取り外す / Detach
  - 脱ぐ／取り外す / Take Off / Detach
  - お気に入りに追加 / Add to Favorites
  - お気に入りから削除 / Remove from Favorites
  - オリジナルを表示 / Show Original [有効条件あり]
  - アウトフィットの編集 / Edit Outfit

## 円形コンテキストメニュー

### `menu_pie_attachment_other.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_attachment_other.xml`

- Avatar Pie
  - プロフィール… / Profile... [有効条件あり]
  - ミュート　＞ / Mute >
    - ミュート / Mute
    - パーティクル / Part. Owner [有効条件あり]
  - 会いに行く / Go to
  - 嫌がらせの報告… / Report...
  - 追加　＞ / Add >
    - フレンドに追加… / As Friend... [有効条件あり]
    - コンタクトセットに追加 / To Set... [有効条件あり]
  - 支払う… / Pay... [有効条件あり]
  - 詳細　＞ / More >
    - フリーズ… / Freeze [有効条件あり]
    - コーリングカードを渡す / Give Card [有効条件あり]
    - グループに招待… / Invite to Group [有効条件あり]
    - Face towards [有効条件あり]
    - 追放… / Eject... [有効条件あり]
    - 詳細　＞ / More >
      - Debug >
        - Textures
        - Script Info [有効条件あり]
      - コール / Call [有効条件あり]
      - ズームイン / Zoom In
      - テクスチャをリフレッシュ / Tex Refresh
      - リセット　＞ / Reset >
        - スケルトンとアニメ / Skel. & Anim.
        - スケルトン / Skeleton
        - メッシュのＬＯＤ / Mesh LOD
      - 名前をつけて保存　＞ / Save as >
        - バックアップ / Backup [有効条件あり]
        - Collada [有効条件あり]
        - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
      - 描画　＞ / Display >
        - 通常描画 / Normally [表示条件あり]
        - 描画しない / Never
        - 常に描画 / Fully
      - SSS >
        - Add [有効条件あり]
        - Remove [有効条件あり]
        - Add Linkset [有効条件あり]
        - Remove Linkset [有効条件あり]
    - 調査 / Inspect [有効条件あり]
    - 描画解除　＞ / Derender >
      - ブラックリスト / Blacklist
      - 描画解除 / Temporary
  - ＩＭを送信… / IM [有効条件あり]

### `menu_pie_attachment_self.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_attachment_self.xml`

- Attachment Pie
  - プロフィール… / Profile...
  - 下に置く / Drop [有効条件あり]
  - 詳細　＞ / More >
    - テクスチャをリフレッシュ / Tex Refresh
    - スクリプト情報 / Script Info [有効条件あり]
    - 触る / Touch [有効条件あり]
    - 表示しない　＞ / Derender >
      - 描画解除 / Temporary [有効条件あり]
      - 描画解除リストに追加 / Blacklist [有効条件あり]
    - 名前をつけて保存　＞ / Save as >
      - バックアップ / Backup [有効条件あり]
      - Collada [有効条件あり]
    - オリジナルを表示 / Show in Inv. [有効条件あり]
    - Debug >
      - Inspect [有効条件あり]
      - Textures
    - SSS >
      - Add [有効条件あり]
      - Remove [有効条件あり]
      - Add Linkset [有効条件あり]
      - Remove Linkset [有効条件あり]
  - ここに座る / Sit Here [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - 取り外す / Detach [有効条件あり]
  - ジェスチャー… / Gestures...
  - 容姿　＞ / Appearance >
    - シェイプを編集 / Edit Shape [有効条件あり]
    - リセット　＞ / Reset >
      - スケルトン＋アニメ / Skel. & Anim.
      - スケルトン / Skeleton
      - メッシュのＬＯＤ / Mesh LOD
    - アウトフィットを編集 / Edit Outfit [有効条件あり]
    - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
    - ホバーする高さ / Hover Height [有効条件あり]
  - 編集… / Edit [有効条件あり]

### `menu_pie_avatar_other.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_avatar_other.xml`

- Avatar Pie
  - プロフィール… / Profile... [有効条件あり]
  - ミュート　＞ / Mute >
    - ミュート / Mute
    - パーティクル / Part. Owner [有効条件あり]
  - 会いに行く / Go to
  - 嫌がらせの報告… / Report...
  - 追加　＞ / Add >
    - フレンドに追加… / As Friend... [有効条件あり]
    - コンタクトセットに追加 / To Set... [有効条件あり]
  - 支払う… / Pay... [有効条件あり]
  - 詳細　＞ / More >
    - 動けなくする / Freeze [有効条件あり]
    - コーリングカードを渡す / Give Card [有効条件あり]
    - グループに招待 / Invite to Group [有効条件あり]
    - Face towards [有効条件あり]
    - 追放する… / Eject... [有効条件あり]
    - 詳細　＞ / More >
      - テクスチャ / Textures
      - スクリプト情報 / Script Info [有効条件あり]
      - コール / Call [有効条件あり]
      - ズームイン / Zoom In
      - テクスチャ更新 / Tex Refresh
      - リセット　＞ / Reset >
        - スケルトンとアニメ / Skel. & Anim.
        - スケルトン / Skeleton
        - メッシュのＬＯＤ / Mesh LOD
      - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
      - 描画　＞ / Display >
        - 通常描画 / Normally [表示条件あり]
        - 描画しない / Never
        - 常に描画 / Fully
    - 描画解除　＞ / Derender >
      - 描画解除リストに追加 / Blacklist
      - 描画解除 / Temporary
  - ＩＭを送信… / IM [有効条件あり]

### `menu_pie_avatar_self.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_avatar_self.xml`

- Self Pie
  - プロフィール… / Profile
  - グループ… / Groups
  - 取り外す　＞ / Take Off >
    - 衣装　＞ / Clothes >
      - シャツ / Shirt [有効条件あり]
      - ズボン / Pants [有効条件あり]
      - 靴 / Shoes [有効条件あり]
      - 靴下 / Socks [有効条件あり]
      - ジャケット / Jacket [有効条件あり]
      - 手袋 / Gloves [有効条件あり]
      - 詳細　＞ / More >
        - 下着（上半身） / Undershirt [有効条件あり]
        - すべての服 / All Clothes
        - 下着（下半身） / Underpants [有効条件あり]
        - タトゥー / Tattoo [有効条件あり]
        - 物理作用 / Physics [有効条件あり]
        - アルファ / Alpha [有効条件あり]
      - スカート / Skirt [有効条件あり]
    - ＨＵＤ　＞ / HUD >
    - 取り外す / Detach >
      - Ext. Skeleton >
        - Right Wing [有効条件あり]
        - Groin [有効条件あり]
        - Tail Base [有効条件あり]
        - Tail Tip [有効条件あり]
        - Left Wing [有効条件あり]
        - L Hind Foot [有効条件あり]
        - More >
          - Alt R Ear [有効条件あり]
          - Alt R Eye [有効条件あり]
          - Tongue [有効条件あり]
          - Alt L Eye [有効条件あり]
          - Alt L Ear [有効条件あり]
          - L Ring Finger [有効条件あり]
          - Jaw [有効条件あり]
          - R Ring Finger [有効条件あり]
        - R Hind Foot [有効条件あり]
    - すべて取り外す / Detach All [有効条件あり]
  - ここに座る / Sit Down [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - スクリプト情報 / Script Info [有効条件あり]
  - ジェスチャー… / Gestures...
  - 容姿　＞ / Appearance >
    - シェイプを編集 / Edit Shape [有効条件あり]
    - リセット > / Reset >
      - スケルトンとアニメ / Skel. & Anim.
      - スケルトン / Skeleton
      - メッシュのＬＯＤ / Mesh LOD
    - テクスチャ更新 / Tex Refresh
    - 衣装を編集 / Edit Outfit [有効条件あり]
    - ＸＭＬとしてダンプ / Dump XML [表示条件あり]
    - ホバーする高さ / Hover Height [有効条件あり]
  - テクスチャ / Textures

### `menu_pie_land.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_land.xml`

- Land Pie
  - 土地情報… / About Land...
  - 作成 / Create [有効条件あり]
  - ここに来る / Go Here
  - ここに座る / Sit Here
  - パーティクルを無視 / Mute Part. Own. [有効条件あり]
  - 入場許可購入 / Buy Pass [有効条件あり]
  - 地形を編集 / Edit Terrain [有効条件あり]
  - 土地を購入 / Buy This Land [有効条件あり]

### `menu_pie_mute_particle.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_mute_particle.xml`

- Mute Particle Pie
  - パーティクル所有者を無視 / Mute Part. Own. [有効条件あり]

### `menu_pie_object.xml`

定義: `indra/newview/skins/default/xui/en/menu_pie_object.xml`

- Object Pie
  - 開く / Open [有効条件あり]
  - 作成 / Create [有効条件あり]
  - 触る / Touch [有効条件あり]
  - ここに座る / Sit Here [有効条件あり]
  - 立つ / Stand Up [有効条件あり]
  - 購入する / Buy [有効条件あり]
  - 取る　＞ / Take >
    - コピー：分割 / Copies: Separately [表示条件あり・有効条件あり]
    - コピーを取る / Take copy [表示条件あり・有効条件あり]
    - 取る / Take [表示条件あり・有効条件あり]
    - 取る：複合 / Take: Combined [表示条件あり・有効条件あり]
    - コピー：複合 / Copy: Combined [表示条件あり・有効条件あり]
    - 取る：分割 / Take: Separately [表示条件あり・有効条件あり]
  - 支払う / Pay [有効条件あり]
  - 詳細　＞ / More >
    - 削除 / Delete [有効条件あり]
    - 装着 / Wear [有効条件あり]
    - コピーを取る / Take Copy [有効条件あり]
    - ＨＵＤの装着先　＞ / Attach HUD >
    - 装着先　＞ / Attach >
      - Ext. Skeleton >
        - Right Wing [有効条件あり]
        - Groin [有効条件あり]
        - Tail Base [有効条件あり]
        - Tail Tip [有効条件あり]
        - Left Wing [有効条件あり]
        - L Hind Foot [有効条件あり]
        - More >
          - Alt R Ear [有効条件あり]
          - Alt R Eye [有効条件あり]
          - Tongue [有効条件あり]
          - Alt L Eye [有効条件あり]
          - Alt L Ear [有効条件あり]
          - L Ring Finger [有効条件あり]
          - Jaw [有効条件あり]
          - R Ring Finger [有効条件あり]
        - R Hind Foot [有効条件あり]
    - 返却 / Return [有効条件あり]
    - 詳細　＞ / More >
      - リセット　＞ / Reset >
        - テクスチャのリセット / Tex Refresh
        - スケルトン / Skeleton [表示条件あり]
      - 描画解除　＞ / Derender >
        - 描画解除リストに追加 / Blacklist
        - 描画解除 / Temporary
      - 嫌がらせの報告… / Report... [有効条件あり]
      - 名前をつけて保存　＞ / Save as >
        - バックアップ / Backup [有効条件あり]
        - Collada [有効条件あり]
        - Dump XML [表示条件あり]
      - 経路探索　＞ / Pathfinding >
        - リンクセットで表示 / Show in linksets [有効条件あり]
        - キャラクターで表示 / Show in characters [有効条件あり]
      - ミュート　＞ / Mute >
        - ミュート / Mute
        - パーティクル / Part. Owner [有効条件あり]
      - スクリプト　＞ / Scripts >
        - コンパイル（Ｍｏｎｏ） / Compile (Mono) [有効条件あり]
        - コンパイル（ＬＳＬ） / Compile (LSL) [有効条件あり]
        - スクリプトリセット / Reset Scripts [有効条件あり]
        - スクリプトを実行 / Run Scripts [有効条件あり]
        - スクリプトを停止 / Stop Scripts [有効条件あり]
        - スクリプトを削除 / Remove Scripts [有効条件あり]
        - スクリプト情報 / Script Info [有効条件あり]
      - 調査 / Inspect
    - 購入 / Buy [有効条件あり]
  - 編集 / Edit [有効条件あり]
