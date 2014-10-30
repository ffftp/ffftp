開発者向けドキュメント：インストーラの作成方法

インストーラの作成にはEXEpress 6を使用します。EXEpress 6はフリーソフトウェア向けのものが無償でダウンロードできます。



自動的にインストーラを作成する手順

1. make_installer.batを実行



手動でインストーラを作成する手順

1. 準備
   1.1. make_installer_pre.batを実行

2. 日本語版インストーラを作成
   2.1. make_cab_file.batを実行（失敗する場合は手作業でEXEpress\jpn\ffftp\内のファイルをCAB形式で圧縮、EXEpress\jpn\ffftp.cabとして保存（Windows標準のiexpressツールが利用できます））
   2.2. make_exe_file.batを実行（失敗する場合は手作業でEXEpressを起動し、「設定読み込み」でEXEpress\jpn\ffftp.iniを指定、「作成」をクリック）

3. 英語版インストーラを作成（make_cab_file.batとmake_exe_file.batの実行に成功した場合はすでにインストーラが作成されていますので、何もする必要はありません）
   3.1. EXEpress\eng\ffftp\内のファイルをCAB形式で圧縮、EXEpress\eng\ffftp.cabとして保存
   3.2. EXEpressを起動し、「設定読み込み」でEXEpress\eng\ffftp.iniを指定、「作成」をクリックする

4. 日本語版ZIPファイルを作成
   4.1. zip\jpn\ffftpフォルダをZIP形式で圧縮（「送る」→「圧縮（ZIP形式）フォルダ」で可能）

5. 英語版ZIPファイルを作成
   4.1. zip\enf\ffftpフォルダをZIP形式で圧縮（「送る」→「圧縮（ZIP形式）フォルダ」で可能）

6. ファイルを収集
   6.1. make_installer_post.batを実行

