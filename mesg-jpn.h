#pragma once
extern std::map<int, std::string> msgs;
#define MSGJPN001 (std::data(msgs[10000+1])) /* 接続を中止しました. */
#define MSGJPN002 (std::data(msgs[10000+2])) /* 接続を中止しました. */
#define MSGJPN003 (std::data(msgs[10000+3])) /* \r\n再接続します.\r\n */
#define MSGJPN004 (std::data(msgs[10000+4])) /* 切断しました */
#define MSGJPN005 (std::data(msgs[10000+5])) /* 接続が切断されました. */
#define MSGJPN006 (std::data(msgs[10000+6])) /* FireWallにログインできません. */
#define MSGJPN007 (std::data(msgs[10000+7])) /* ホスト %s に接続できません. */
#define MSGJPN008 (std::data(msgs[10000+8])) /* ログインできません. */
#define MSGJPN009 (std::data(msgs[10000+9])) /* 接続できません. */
#define MSGJPN010 (std::data(msgs[10000+10])) /* FireWallのホスト名が設定されていません. */
#define MSGJPN011 (std::data(msgs[10000+11])) /* ホスト名がありません. */
#define MSGJPN012 (std::data(msgs[10000+12])) /* MD5を使用します. */
#define MSGJPN013 (std::data(msgs[10000+13])) /* SHA-1を使用します. */
#define MSGJPN014 (std::data(msgs[10000+14])) /* MD4(S/KEY)を使用します. */
#define MSGJPN015 (std::data(msgs[10000+15])) /* ワンタイムパスワードが処理できません */
#define MSGJPN017 (std::data(msgs[10000+17])) /* %sホスト %s (%s) に接続しています. */
#define MSGJPN019 (std::data(msgs[10000+19])) /* ホスト %s が見つかりません. */
#define MSGJPN021 (std::data(msgs[10000+21])) /* SOCKSサーバー %s が見つかりません. */
#define MSGJPN022 (std::data(msgs[10000+22])) /* SOCKSサーバー %s に接続しています. */
#define MSGJPN023 (std::data(msgs[10000+23])) /* SOCKSサーバーに接続できません. (Err=%d) */
#define MSGJPN025 (std::data(msgs[10000+25])) /* 接続しました. */
#define MSGJPN026 (std::data(msgs[10000+26])) /* 接続できません. */
#define MSGJPN027 (std::data(msgs[10000+27])) /* ソケットが作成できません. */
#define MSGJPN031 (std::data(msgs[10000+31])) /* %sコマンドが受け付けられません. */
#define MSGJPN033 (std::data(msgs[10000+33])) /* SOCKSのコマンドが送れませんでした (Cmd = %04X) */
#define MSGJPN034 (std::data(msgs[10000+34])) /* SOCKS5のコマンドに対するリプライが受信できませんでした */
#define MSGJPN035 (std::data(msgs[10000+35])) /* SOCKS4のコマンドに対するリプライが受信できませんでした */
#define MSGJPN036 (std::data(msgs[10000+36])) /* SOCKSサーバーの認証方式が異なります. */
#define MSGJPN037 (std::data(msgs[10000+37])) /* SOCKSサーバーに認証されませんでした. */
#define MSGJPN048 (std::data(msgs[10000+48])) /* テンポラリファイルが読み出せません. */
#define MSGJPN049 (std::data(msgs[10000+49])) /* ファイル一覧の取得に失敗しました. */
#define MSGJPN050 (std::data(msgs[10000+50])) /* 検索（ローカル） */
#define MSGJPN051 (std::data(msgs[10000+51])) /* 検索（ホスト） */
#define MSGJPN052 (std::data(msgs[10000+52])) /* 削除: %s */
#define MSGJPN053 (std::data(msgs[10000+53])) /* 作成: %s */
#define MSGJPN054 (std::data(msgs[10000+54])) /* 転送: %s */
#define MSGJPN055 (std::data(msgs[10000+55])) /* 削除: %s */
#define MSGJPN056 (std::data(msgs[10000+56])) /* 作成: %s */
#define MSGJPN057 (std::data(msgs[10000+57])) /* 転送: %s */
#define MSGJPN058 (std::data(msgs[10000+58])) /* %d個のファイルを転送します. */
#define MSGJPN059 (std::data(msgs[10000+59])) /* 転送するファイルはありません. */
#define MSGJPN060 (std::data(msgs[10000+60])) /* %d個のフォルダを作成します. */
#define MSGJPN061 (std::data(msgs[10000+61])) /* 作成するフォルダはありません. */
#define MSGJPN062 (std::data(msgs[10000+62])) /* %d個のファイル/フォルダを削除します. */
#define MSGJPN063 (std::data(msgs[10000+63])) /* 削除するファイル/フォルダはありません. */
#define MSGJPN070 (std::data(msgs[10000+70])) /* フォルダ作成（ローカル） */
#define MSGJPN071 (std::data(msgs[10000+71])) /* フォルダ作成（ホスト） */
#define MSGJPN072 (std::data(msgs[10000+72])) /* フォルダ変更（ローカル） */
#define MSGJPN073 (std::data(msgs[10000+73])) /* フォルダ変更（ホスト） */
#define MSGJPN078 (std::data(msgs[10000+78])) /* フォルダ作成 */
#define MSGJPN079 (std::data(msgs[10000+79])) /* フォルダ作成 */
#define MSGJPN080 (std::data(msgs[10000+80])) /* フォルダ削除 */
#define MSGJPN081 (std::data(msgs[10000+81])) /* ファイル削除 */
#define MSGJPN082 (std::data(msgs[10000+82])) /* フォルダ作成 */
#define MSGJPN083 (std::data(msgs[10000+83])) /* フォルダ削除 */
#define MSGJPN084 (std::data(msgs[10000+84])) /* ファイル削除 */
#define MSGJPN085 (std::data(msgs[10000+85])) /* %sという名前のファイルはダウンロードできません. */
#define MSGJPN086 (std::data(msgs[10000+86])) /* ダウンロード */
#define MSGJPN087 (std::data(msgs[10000+87])) /* ファイル一覧取得 */
#define MSGJPN088 (std::data(msgs[10000+88])) /* スキップ */
#define MSGJPN089 (std::data(msgs[10000+89])) /* ファイル %s はスキップします. */
#define MSGJPN090 (std::data(msgs[10000+90])) /* コマンドが受け付けられません. */
#define MSGJPN091 (std::data(msgs[10000+91])) /* ダウンロードのために */
#define MSGJPN092 (std::data(msgs[10000+92])) /* コマンドが受け付けられません. */
#define MSGJPN093 (std::data(msgs[10000+93])) /* アドレスが取得できません. */
#define MSGJPN094 (std::data(msgs[10000+94])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN095 (std::data(msgs[10000+95])) /* ファイル %s が作成できません. */
#define MSGJPN096 (std::data(msgs[10000+96])) /* ディスクがいっぱいで書き込めません. */
#define MSGJPN097 (std::data(msgs[10000+97])) /* ファイル一覧の取得を中止しました. */
#define MSGJPN098 (std::data(msgs[10000+98])) /* ファイル一覧 */
#define MSGJPN099 (std::data(msgs[10000+99])) /* ダウンロードを中止しました. (%lld Sec. %lld B/S). */
#define MSGJPN100 (std::data(msgs[10000+100])) /* ダウンロードを中止しました. */
#define MSGJPN101 (std::data(msgs[10000+101])) /* ファイル一覧の取得は正常終了しました. (%lld Bytes) */
#define MSGJPN102 (std::data(msgs[10000+102])) /* ダウンロードは正常終了しました. (%d Sec. %d B/S). */
#define MSGJPN103 (std::data(msgs[10000+103])) /* ダウンロードは正常終了しました. (%lld Bytes) */
#define MSGJPN104 (std::data(msgs[10000+104])) /* アップロード */
#define MSGJPN105 (std::data(msgs[10000+105])) /* ファイル %s が読み出せません. */
#define MSGJPN106 (std::data(msgs[10000+106])) /* スキップ */
#define MSGJPN107 (std::data(msgs[10000+107])) /* ファイル %s はスキップします. */
#define MSGJPN108 (std::data(msgs[10000+108])) /* コマンドが受け付けられません. */
#define MSGJPN109 (std::data(msgs[10000+109])) /* アップロードのために */
#define MSGJPN110 (std::data(msgs[10000+110])) /* コマンドが受け付けられません. */
#define MSGJPN111 (std::data(msgs[10000+111])) /* アドレスが取得できません. */
#define MSGJPN112 (std::data(msgs[10000+112])) /* ファイル %s がオープンできません. */
#define MSGJPN113 (std::data(msgs[10000+113])) /* アップロードを中止しました. (%lld Sec. %lld B/S). */
#define MSGJPN114 (std::data(msgs[10000+114])) /* アップロードを中止しました. */
#define MSGJPN115 (std::data(msgs[10000+115])) /* アップロードは正常終了しました. (%d Sec. %d B/S). */
#define MSGJPN116 (std::data(msgs[10000+116])) /* アップロードは正常終了しました. */
#define MSGJPN133 (std::data(msgs[10000+133])) /* GMT%+02d:00 (日本) */
#define MSGJPN145 (std::data(msgs[10000+145])) /* フォルダを変更できません. */
#define MSGJPN146 (std::data(msgs[10000+146])) /* フォルダを作成できません. */
#define MSGJPN147 (std::data(msgs[10000+147])) /* フォルダを削除できません. */
#define MSGJPN148 (std::data(msgs[10000+148])) /* フォルダを削除できません. */
#define MSGJPN149 (std::data(msgs[10000+149])) /* ファイルを削除できません. */
#define MSGJPN150 (std::data(msgs[10000+150])) /* ファイルを削除できません. */
#define MSGJPN151 (std::data(msgs[10000+151])) /* ファイル名変更ができません. */
#define MSGJPN177 (std::data(msgs[10000+177])) /* 設定名「%s」はありません. */
#define MSGJPN178 (std::data(msgs[10000+178])) /* 設定名が指定されていません. */
#define MSGJPN179 (std::data(msgs[10000+179])) /* ホスト名と設定名の両方が指定されています. */
#define MSGJPN180 (std::data(msgs[10000+180])) /* オプション「%s」が間違っています. */
#define MSGJPN182 (std::data(msgs[10000+182])) /* ビューアの起動に失敗しました. (ERROR=%d) */
#define MSGJPN185 (std::data(msgs[10000+185])) /* フォルダを選択してください */
#define MSGJPN199 (std::data(msgs[10000+199])) /* ファイル名 */
#define MSGJPN202 (std::data(msgs[10000+202])) /* ファイル名 */
#define MSGJPN203 (std::data(msgs[10000+203])) /* ファイル名 */
#define MSGJPN220 (std::data(msgs[10000+220])) /* ダイアルアップを切断しています. */
#define MSGJPN221 (std::data(msgs[10000+221])) /* ダイアルアップで接続しています. */
#define MSGJPN239 (std::data(msgs[10000+239])) /* # このファイルは編集しないでください.\n */
#define MSGJPN241 (std::data(msgs[10000+241])) /* 送信はタイムアウトで失敗しました. */
#define MSGJPN242 (std::data(msgs[10000+242])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN243 (std::data(msgs[10000+243])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN244 (std::data(msgs[10000+244])) /* 固定長の受信が失敗しました */
#define MSGJPN247 (std::data(msgs[10000+247])) /* 選択%d個（%s） */
#define MSGJPN248 (std::data(msgs[10000+248])) /* ローカル空 %s */
#define MSGJPN249 (std::data(msgs[10000+249])) /* 転送待ちファイル%d個 */
#define MSGJPN250 (std::data(msgs[10000+250])) /* 受信中 %s */
#define MSGJPN251 (std::data(msgs[10000+251])) /* Err: シード文字列 */
#define MSGJPN252 (std::data(msgs[10000+252])) /* Err: シード文字列 */
#define MSGJPN253 (std::data(msgs[10000+253])) /* Err: シーケンス番号 */
#define MSGJPN278 (std::data(msgs[10000+278])) /* 理由: %s */
#define MSGJPN279 (std::data(msgs[10000+279])) /* Listenソケットが取得できません */
#define MSGJPN280 (std::data(msgs[10000+280])) /* Dataソケットが取得できません */
#define MSGJPN281 (std::data(msgs[10000+281])) /* PASVモードで接続できません */
#define MSGJPN282 (std::data(msgs[10000+282])) /* INIファイル名が指定されていません */
#define MSGJPN283 (std::data(msgs[10000+283])) /* INIファイル指定:  */
#define MSGJPN299 (std::data(msgs[10000+299])) /* コマンドラインにマスターパスワードが指定されていません */
#define MSGJPN300 (std::data(msgs[10000+300])) /* デフォルトのマスターパスワードが使われます.\r\nマルウェアの攻撃を防ぐため,固有のマスターパスワードを設定することをおすすめします */
#define MSGJPN301 (std::data(msgs[10000+301])) /* マスターパスワードが設定と一致しません.安全のため設定の保存を行いません. */
#define MSGJPN302 (std::data(msgs[10000+302])) /* 確認用データが壊れているため,マスターパスワードの正当性を確認できませんでした. */
#define MSGJPN303 (std::data(msgs[10000+303])) /* マスターパスワードを変更しました */
#define MSGJPN314 (std::data(msgs[10000+314])) /* 通信は暗号化されていません.\r\n第三者にパスワードおよび内容を傍受される可能性があります. */
#define MSGJPN315 (std::data(msgs[10000+315])) /* FTP over Explicit SSL/TLS (FTPES)を使用します. */
#define MSGJPN316 (std::data(msgs[10000+316])) /* FTP over Implicit SSL/TLS (FTPIS)を使用します. */
#define MSGJPN317 (std::data(msgs[10000+317])) /* SSH FTP (SFTP)を使用します. */
#define MSGJPN321 (std::data(msgs[10000+321])) /* プロセスの保護に必要な関数を取得できませんでした. */
#define MSGJPN322 (std::data(msgs[10000+322])) /* デバッガが検出されました. */
#define MSGJPN323 (std::data(msgs[10000+323])) /* 信頼できないDLLをアンロードできませんでした. */
#define MSGJPN324 (std::data(msgs[10000+324])) /* プロセスの保護に必要な関数をフックできませんでした. */
#define MSGJPN358 (std::data(msgs[10000+358])) /* ソフトウェアの更新が完了しました.\n以前のバージョンのバックアップコピーは以下の場所にあります.\n%s */
#define MSGJPN359 (std::data(msgs[10000+359])) /* ソフトウェアの更新に失敗しました.\nWebサイトから最新版を入手して手動で更新してください. */
