# PDIC for Android 環境構築

## 動作環境

- Windows 10以上
- Android Studio

## NDKのビルド

- AndroidStudioでNDKをinstallする
  - Tools - SDK Managerにて、SDK ToolsタブでNDKを選択、右下の"Show Package Details"をチェックして使用するNDK versionを選択
    - 検証済みversion
      - 29.0.1420685
      - 21.0.6113669

- MSYS2, MinGWなど、make, cp, rmコマンドが実行できる環境を用意する
  - MSYS2を使用する場合
    - https://www.msys2.org/ より installerをdownload/install

- 環境変数
  - ndk-build.cmdへのpathを確認する
    - 通常 C:\Users\\`<user>`\AppData\Local\Android\Sdk\ndk\\`<installed version>`
  - 次のいずれかの設定を行う
    - ndk-build.cmdへのpathを環境変数PATHに設定
    - env.mkのNDK_PATHにndk-build.cmdへのpathを記述

- ビルドの実行
  - make

## 確認済みversion

- Android Studio Narwhal 4 Feature Drop|2025.1.4
- ndk-build.cmd 29.0.1420685, 21.0.6113669
- GNU Make 3.82.90, 4.4.1

# 参考情報

## git comment rules

- [A][B][C] 正式版での問題の修正
- [XA][XB][XC] 開発版での問題の修正（リリースされたものに限る）
- [D] 問題の修正だが、潜在的な問題
- [dbg] or [d] デバッグコード用の追加・修正
- [N] 新規追加機能 - 使用説明書に変更が必要なレベル
- [R] リファクタリング（実質的な変更を伴わない、コメント、シンボル名などの変更）
- [U!] 仕様変更で説明書に明記が必要な変更
- [U] 仕様変更に影響がない変更、上記いずれにも入らない修正全て
- [Release] リリース用コミット
- [?+] コミット漏れ(?は上記文字、それに+を追加) 
- [?*] エンバグ・副作用の修正


# リンク

## PDIC for Android Home Page

- <a href="https://pdic.sakura.ne.jp/android/">PDIC for Android公式ホームページ</a> ダウンロードはこちらから（野良アプリ）
- <a href="https://play.google.com/apps/testing/com.reliefoffice.pdic">Google Play</a> Googleの制限により非公開状態

