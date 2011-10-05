開発者向けドキュメント：インストーラの作成方法

インストーラの作成にはEXEpress CX 5を使用します。EXEpress CX 5はフリーソフトウェア向けのものが無償でダウンロードできます。

1. 準備
  1.1. make_installer_pre.batを実行

2. 日本語版インストーラを作成
  2.1. EXEpress\jpn\ffftp\内のファイルをCAB形式で圧縮、EXEpress\jpn\ffftp.cabとして保存（Windows標準のiexpressツールが利用できます）
  2.2. EXEpressを起動し、「設定読み込み」でEXEpress\jpn\ffftp.iniを指定、「作成」をクリックする

3. 英語版インストーラを作成
  3.1. EXEpress\eng\ffftp\内のファイルをCAB形式で圧縮、EXEpress\eng\ffftp.cabとして保存
  3.2. EXEpressを起動し、「設定読み込み」でEXEpress\eng\ffftp.iniを指定、「作成」をクリックする

4. 日本語版ZIPファイルを作成
  4.1. zip\jpn\ffftpフォルダをZIP形式で圧縮（「送る」→「圧縮（ZIP形式）フォルダ」で可能）

5. 英語版ZIPファイルを作成
  4.1. zip\enf\ffftpフォルダをZIP形式で圧縮（「送る」→「圧縮（ZIP形式）フォルダ」で可能）

6. ファイルを収集
  6.1. make_installer_post.batを実行
