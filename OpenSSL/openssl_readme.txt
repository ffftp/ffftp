OpenSSLビルド用ファイル

1. 準備
   1.1. 任意のバージョンのVisual Studio（以下、Visual Studio 2008を例に説明）をインストール
   1.2. 最新のActivePerl（5.20.2 Build 2001で動作確認）をインストール

2. OpenSSLソースコードの入手
   2.1. OpenSSLのWebサイト（https://www.openssl.org/source/）より1.1.0以降のバージョンのソースコードの書庫ファイルをダウンロード
   2.2. 書庫ファイルをディレクトリ構造を保ったままこのディレクトリ内に解凍

3. OpenSSLのビルド（32ビット版）
   3.1. Visual Studio 2008 コマンド プロンプトを起動
   3.2. コマンド プロンプト内でbuildx86.batを実行
   3.3. 正常にビルドされた場合distディレクトリ内にlibeay32.dllおよびssleay32.dllが作成される

3. OpenSSLのビルド（64ビット版）
   3.1. Visual Studio 2008 x64 Win64 コマンド プロンプトを起動
   3.2. コマンド プロンプト内でbuildx64.batを実行
   3.3. 正常にビルドされた場合dist\amd64ディレクトリ内にlibeay32.dllおよびssleay32.dllが作成される

