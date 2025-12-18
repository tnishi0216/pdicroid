# 環境構築

## 動作環境

- Windows 10以上
- Android Studio

## NDKのビルド

- AndroidStudioでNDKをinstallする
  - Tools - SDK Managerにて、
    - SDK ToolsタブでNDKを選択
      - 右下の"Show Package Details"をチェックして使用するNDK versionを選択
      - 検証済みversion
        - 29.0.1420685
        - 21.0.6113669
      - 使用するNDK versionに応じてenv.mkを変更する

- MSYS2, MinGWなど、make, cp, rmコマンドが実行できる環境を用意する
  - MSYS2を使用する場合
    - https://www.msys2.org/ より installerをdownload/install

- 環境変数
  - ndk-build.cmdへのpathを確認する
  - 通常 C:\Users\<user>\AppData\Local\Android\Sdk\ndk\<installed version>
  - 次のいずれかの設定を行う
    - ndk-build.cmdへのpathを環境変数PATHに設定
    - NDK_PATH=ndk-build.cmdへのpathを記述

## 確認済みversion

- Android Studio Narwhal 4 Feature Drop|2025.1.4
- ndk-build.cmd 29.0.1420685, 21.0.6113669
- GNU Make 3.82.90, 4.4.1
