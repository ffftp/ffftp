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
#define MSGJPN016 (std::data(msgs[10000+16])) /* ホスト %s を探しています. (%s) */
#define MSGJPN017 (std::data(msgs[10000+17])) /* %sホスト %s (%s (%d)) に接続しています. (%s) */
#define MSGJPN018 (std::data(msgs[10000+18])) /* %sホスト %s (%d) に接続しています. (%s) */
#define MSGJPN019 (std::data(msgs[10000+19])) /* ホスト %s が見つかりません. (%s) */
#define MSGJPN020 (std::data(msgs[10000+20])) /* %sホスト %s (%d) に接続しています. (%s) */
#define MSGJPN021 (std::data(msgs[10000+21])) /* SOCKSサーバー %s が見つかりません. (%s) */
#define MSGJPN022 (std::data(msgs[10000+22])) /* SOCKSサーバー %s (%d) に接続しています. (%s) */
#define MSGJPN023 (std::data(msgs[10000+23])) /* SOCKSサーバーに接続できません. (Err=%d) (%s) */
#define MSGJPN024 (std::data(msgs[10000+24])) /* SOCKSサーバーに接続できません. (Err=%d) (%s) */
#define MSGJPN025 (std::data(msgs[10000+25])) /* 接続しました. (%s) */
#define MSGJPN026 (std::data(msgs[10000+26])) /* 接続できません. (%s) */
#define MSGJPN027 (std::data(msgs[10000+27])) /* ソケットが作成できません. (%s) */
#define MSGJPN028 (std::data(msgs[10000+28])) /* SOCKSサーバーに接続できません. (Err=%d) (%s) */
#define MSGJPN029 (std::data(msgs[10000+29])) /* SOCKSサーバーに接続できません. (Err=%d) (%s) */
#define MSGJPN030 (std::data(msgs[10000+30])) /* Listenソケットが作成できません. (%s) */
#define MSGJPN031 (std::data(msgs[10000+31])) /* PORTコマンドが受け付けられません. (%s) */
#define MSGJPN032 (std::data(msgs[10000+32])) /* 接続はユーザーによって中止されました. */
#define MSGJPN033 (std::data(msgs[10000+33])) /* SOCKSのコマンドが送れませんでした (Cmd = %04X) */
#define MSGJPN034 (std::data(msgs[10000+34])) /* SOCKS5のコマンドに対するリプライが受信できませんでした */
#define MSGJPN035 (std::data(msgs[10000+35])) /* SOCKS4のコマンドに対するリプライが受信できませんでした */
#define MSGJPN036 (std::data(msgs[10000+36])) /* SOCKSサーバーの認証方式が異なります. */
#define MSGJPN037 (std::data(msgs[10000+37])) /* SOCKSサーバーに認証されませんでした. */
#define MSGJPN038 (std::data(msgs[10000+38])) /* 名前 */
#define MSGJPN039 (std::data(msgs[10000+39])) /* 日付 */
#define MSGJPN040 (std::data(msgs[10000+40])) /* サイズ */
#define MSGJPN041 (std::data(msgs[10000+41])) /* 種類 */
#define MSGJPN042 (std::data(msgs[10000+42])) /* 名前 */
#define MSGJPN043 (std::data(msgs[10000+43])) /* 日付 */
#define MSGJPN044 (std::data(msgs[10000+44])) /* サイズ */
#define MSGJPN045 (std::data(msgs[10000+45])) /* 種類 */
#define MSGJPN046 (std::data(msgs[10000+46])) /* 属性 */
#define MSGJPN047 (std::data(msgs[10000+47])) /* 所有者 */
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
#define MSGJPN064 (std::data(msgs[10000+64])) /* 名前を変更してアップロード */
#define MSGJPN065 (std::data(msgs[10000+65])) /* 名前を変更してダウンロード */
#define MSGJPN066 (std::data(msgs[10000+66])) /* 削除（ローカル） */
#define MSGJPN067 (std::data(msgs[10000+67])) /* 削除（ホスト） */
#define MSGJPN068 (std::data(msgs[10000+68])) /* 名前変更（ローカル） */
#define MSGJPN069 (std::data(msgs[10000+69])) /* 名前変更（ホスト） */
#define MSGJPN070 (std::data(msgs[10000+70])) /* フォルダ作成（ローカル） */
#define MSGJPN071 (std::data(msgs[10000+71])) /* フォルダ作成（ホスト） */
#define MSGJPN072 (std::data(msgs[10000+72])) /* フォルダ変更（ローカル） */
#define MSGJPN073 (std::data(msgs[10000+73])) /* フォルダ変更（ホスト） */
#define MSGJPN074 (std::data(msgs[10000+74])) /* ローカル側のファイル容量を計算します. */
#define MSGJPN075 (std::data(msgs[10000+75])) /* ホスト側のファイル容量を計算します. */
#define MSGJPN076 (std::data(msgs[10000+76])) /* ローカル側のファイル容量 */
#define MSGJPN077 (std::data(msgs[10000+77])) /* ホスト側のファイル容量 */
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
#define MSGJPN117 (std::data(msgs[10000+117])) /* 完了 */
#define MSGJPN118 (std::data(msgs[10000+118])) /* 中止 */
#define MSGJPN119 (std::data(msgs[10000+119])) /* バイナリ */
#define MSGJPN120 (std::data(msgs[10000+120])) /* アスキー */
#define MSGJPN121 (std::data(msgs[10000+121])) /* 無変換 */
#define MSGJPN122 (std::data(msgs[10000+122])) /* JIS */
#define MSGJPN123 (std::data(msgs[10000+123])) /* EUC */
#define MSGJPN124 (std::data(msgs[10000+124])) /* 削除（ローカル） */
#define MSGJPN125 (std::data(msgs[10000+125])) /* 削除（ホスト） */
#define MSGJPN126 (std::data(msgs[10000+126])) /* WS_FTP.INIファイル */
#define MSGJPN127 (std::data(msgs[10000+127])) /* 基本 */
#define MSGJPN128 (std::data(msgs[10000+128])) /* 拡張 */
#define MSGJPN129 (std::data(msgs[10000+129])) /* 文字コード */
#define MSGJPN130 (std::data(msgs[10000+130])) /* ダイアルアップ */
#define MSGJPN131 (std::data(msgs[10000+131])) /* 高度 */
#define MSGJPN132 (std::data(msgs[10000+132])) /* ホストの設定 */
#define MSGJPN133 (std::data(msgs[10000+133])) /* GMT%+02d:00 (日本) */
#define MSGJPN134 (std::data(msgs[10000+134])) /* 使用しない */
#define MSGJPN135 (std::data(msgs[10000+135])) /* 自動認識 */
#define MSGJPN136 (std::data(msgs[10000+136])) /* OTP MD4, S/KEY */
#define MSGJPN137 (std::data(msgs[10000+137])) /* OTP MD5 */
#define MSGJPN138 (std::data(msgs[10000+138])) /* OTP SHA-1 */
#define MSGJPN139 (std::data(msgs[10000+139])) /* 自動認識 */
#define MSGJPN140 (std::data(msgs[10000+140])) /* ACOS */
#define MSGJPN141 (std::data(msgs[10000+141])) /* VAX VMS */
#define MSGJPN142 (std::data(msgs[10000+142])) /* IRMX */
#define MSGJPN143 (std::data(msgs[10000+143])) /* ACOS-4 */
#define MSGJPN144 (std::data(msgs[10000+144])) /* Stratus */
#define MSGJPN145 (std::data(msgs[10000+145])) /* フォルダを変更できません. */
#define MSGJPN146 (std::data(msgs[10000+146])) /* フォルダを作成できません. */
#define MSGJPN147 (std::data(msgs[10000+147])) /* フォルダを削除できません. */
#define MSGJPN148 (std::data(msgs[10000+148])) /* フォルダを削除できません. */
#define MSGJPN149 (std::data(msgs[10000+149])) /* ファイルを削除できません. */
#define MSGJPN150 (std::data(msgs[10000+150])) /* ファイルを削除できません. */
#define MSGJPN151 (std::data(msgs[10000+151])) /* ファイル名変更ができません. */
#define MSGJPN152 (std::data(msgs[10000+152])) /* テンポラリフォルダ「%s」がありません */
#define MSGJPN153 (std::data(msgs[10000+153])) /* フォルダ「%s」を使用します */
#define MSGJPN154 (std::data(msgs[10000+154])) /* 接続 */
#define MSGJPN155 (std::data(msgs[10000+155])) /* クイック接続 */
#define MSGJPN156 (std::data(msgs[10000+156])) /* 切断 */
#define MSGJPN157 (std::data(msgs[10000+157])) /* ダウンロード */
#define MSGJPN158 (std::data(msgs[10000+158])) /* アップロード */
#define MSGJPN159 (std::data(msgs[10000+159])) /* ミラーリングアップロード */
#define MSGJPN160 (std::data(msgs[10000+160])) /* 削除 */
#define MSGJPN161 (std::data(msgs[10000+161])) /* 名前変更 */
#define MSGJPN162 (std::data(msgs[10000+162])) /* フォルダ作成 */
#define MSGJPN163 (std::data(msgs[10000+163])) /* 一つ上のフォルダへ */
#define MSGJPN164 (std::data(msgs[10000+164])) /* フォルダの移動 */
#define MSGJPN165 (std::data(msgs[10000+165])) /* アスキー転送モード */
#define MSGJPN166 (std::data(msgs[10000+166])) /* バイナリ転送モード */
#define MSGJPN167 (std::data(msgs[10000+167])) /* ファイル名で転送モード切替え */
#define MSGJPN168 (std::data(msgs[10000+168])) /* 表示を更新 */
#define MSGJPN169 (std::data(msgs[10000+169])) /* 一覧 */
#define MSGJPN170 (std::data(msgs[10000+170])) /* 詳細 */
#define MSGJPN171 (std::data(msgs[10000+171])) /* ホストの漢字コードはEUC */
#define MSGJPN172 (std::data(msgs[10000+172])) /* ホストの漢字コードはJIS */
#define MSGJPN173 (std::data(msgs[10000+173])) /* 漢字コードの変換なし */
#define MSGJPN174 (std::data(msgs[10000+174])) /* 半角カナを全角に変換 */
#define MSGJPN175 (std::data(msgs[10000+175])) /* フォルダ同時移動 */
#define MSGJPN176 (std::data(msgs[10000+176])) /* 受信中止 */
#define MSGJPN177 (std::data(msgs[10000+177])) /* 設定名「%s」はありません. */
#define MSGJPN178 (std::data(msgs[10000+178])) /* 設定名が指定されていません. */
#define MSGJPN179 (std::data(msgs[10000+179])) /* ホスト名と設定名の両方が指定されています. */
#define MSGJPN180 (std::data(msgs[10000+180])) /* オプション「%s」が間違っています. */
#define MSGJPN181 (std::data(msgs[10000+181])) /* ホスト名と設定名の両方が指定されています. */
#define MSGJPN182 (std::data(msgs[10000+182])) /* ビューアの起動に失敗しました. (ERROR=%d) */
#define MSGJPN185 (std::data(msgs[10000+185])) /* フォルダを選択してください */
#define MSGJPN186 (std::data(msgs[10000+186])) /* ユーザー */
#define MSGJPN187 (std::data(msgs[10000+187])) /* 転送1 */
#define MSGJPN188 (std::data(msgs[10000+188])) /* 転送2 */
#define MSGJPN189 (std::data(msgs[10000+189])) /* 転送3 */
#define MSGJPN190 (std::data(msgs[10000+190])) /* ミラーリング */
#define MSGJPN191 (std::data(msgs[10000+191])) /* 操作 */
#define MSGJPN192 (std::data(msgs[10000+192])) /* 表示1 */
#define MSGJPN193 (std::data(msgs[10000+193])) /* 接続/切断 */
#define MSGJPN194 (std::data(msgs[10000+194])) /* FireWall */
#define MSGJPN195 (std::data(msgs[10000+195])) /* ツール */
#define MSGJPN196 (std::data(msgs[10000+196])) /* サウンド */
#define MSGJPN197 (std::data(msgs[10000+197])) /* その他 */
#define MSGJPN198 (std::data(msgs[10000+198])) /* オプション */
#define MSGJPN199 (std::data(msgs[10000+199])) /* ファイル名 */
#define MSGJPN200 (std::data(msgs[10000+200])) /* ファイル名 */
#define MSGJPN201 (std::data(msgs[10000+201])) /* 属性 */
#define MSGJPN202 (std::data(msgs[10000+202])) /* ファイル名 */
#define MSGJPN203 (std::data(msgs[10000+203])) /* ファイル名 */
#define MSGJPN204 (std::data(msgs[10000+204])) /* FWユーザー名→ FWパスワード→ SITE ホスト名 */
#define MSGJPN205 (std::data(msgs[10000+205])) /* FWユーザー名→ FWパスワード→ USER ユーザー名@ホスト名 */
#define MSGJPN206 (std::data(msgs[10000+206])) /* FWユーザー名→ FWパスワード */
#define MSGJPN207 (std::data(msgs[10000+207])) /* USER ユーザー名@ホスト名 */
#define MSGJPN208 (std::data(msgs[10000+208])) /* OPEN ホスト名 */
#define MSGJPN209 (std::data(msgs[10000+209])) /* SOCKS4 */
#define MSGJPN210 (std::data(msgs[10000+210])) /* SOCKS5 (認証なし) */
#define MSGJPN211 (std::data(msgs[10000+211])) /* SOCKS5 (ユーザー名,パスワード認証) */
#define MSGJPN212 (std::data(msgs[10000+212])) /* 使用しない */
#define MSGJPN213 (std::data(msgs[10000+213])) /* 自動認識 */
#define MSGJPN214 (std::data(msgs[10000+214])) /* OTP MD4,S/KEY */
#define MSGJPN215 (std::data(msgs[10000+215])) /* OTP MD5 */
#define MSGJPN216 (std::data(msgs[10000+216])) /* OTP SHA-1 */
#define MSGJPN217 (std::data(msgs[10000+217])) /* ビューアの選択 */
#define MSGJPN218 (std::data(msgs[10000+218])) /* 実行ファイル\0*.exe;*.com;*.bat\0全てのファイル\0*\0 */
#define MSGJPN219 (std::data(msgs[10000+219])) /* Waveファイル */
#define MSGJPN220 (std::data(msgs[10000+220])) /* ダイアルアップを切断しています. */
#define MSGJPN221 (std::data(msgs[10000+221])) /* ダイアルアップで接続しています. */
#define MSGJPN226 (std::data(msgs[10000+226])) /* ポートを開いています... */
#define MSGJPN227 (std::data(msgs[10000+227])) /* ポートが開かれました */
#define MSGJPN228 (std::data(msgs[10000+228])) /* ダイアル中... */
#define MSGJPN229 (std::data(msgs[10000+229])) /* ダイアル完了 */
#define MSGJPN230 (std::data(msgs[10000+230])) /* 全デバイスが接続されました */
#define MSGJPN231 (std::data(msgs[10000+231])) /* ユーザー名とパスワードを検証中... */
#define MSGJPN232 (std::data(msgs[10000+232])) /* 再検証中... */
#define MSGJPN233 (std::data(msgs[10000+233])) /* パスワードを変更して下さい */
#define MSGJPN234 (std::data(msgs[10000+234])) /* 検証が終了しました */
#define MSGJPN235 (std::data(msgs[10000+235])) /* 接続しました */
#define MSGJPN236 (std::data(msgs[10000+236])) /* 切断しました */
#define MSGJPN237 (std::data(msgs[10000+237])) /* 接続処理中... */
#define MSGJPN239 (std::data(msgs[10000+239])) /* # このファイルは編集しないでください.\n */
#define MSGJPN240 (std::data(msgs[10000+240])) /* INIファイルに設定が保存できません */
#define MSGJPN241 (std::data(msgs[10000+241])) /* 送信はタイムアウトで失敗しました. */
#define MSGJPN242 (std::data(msgs[10000+242])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN243 (std::data(msgs[10000+243])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN244 (std::data(msgs[10000+244])) /* 固定長の受信が失敗しました */
#define MSGJPN245 (std::data(msgs[10000+245])) /* ローカル */
#define MSGJPN246 (std::data(msgs[10000+246])) /* ホスト */
#define MSGJPN247 (std::data(msgs[10000+247])) /* 選択%d個（%s） */
#define MSGJPN248 (std::data(msgs[10000+248])) /* ローカル空 %s */
#define MSGJPN249 (std::data(msgs[10000+249])) /* 転送待ちファイル%d個 */
#define MSGJPN250 (std::data(msgs[10000+250])) /* 受信中 %s */
#define MSGJPN251 (std::data(msgs[10000+251])) /* Err: シード文字列 */
#define MSGJPN252 (std::data(msgs[10000+252])) /* Err: シード文字列 */
#define MSGJPN253 (std::data(msgs[10000+253])) /* Err: シーケンス番号 */
#define MSGJPN254 (std::data(msgs[10000+254])) /* MS Shell Dlg */
#define MSGJPN255 (std::data(msgs[10000+255])) /* アップロード(&U) */
#define MSGJPN256 (std::data(msgs[10000+256])) /* 名前を変えてアップロード(&P)... */
#define MSGJPN257 (std::data(msgs[10000+257])) /* 全てをアップロード */
#define MSGJPN258 (std::data(msgs[10000+258])) /* 削除(&R) */
#define MSGJPN259 (std::data(msgs[10000+259])) /* 名前変更(&N)... */
#define MSGJPN260 (std::data(msgs[10000+260])) /* フォルダ作成(&K)... */
#define MSGJPN261 (std::data(msgs[10000+261])) /* ファイル容量計算(&Z) */
#define MSGJPN262 (std::data(msgs[10000+262])) /* 最新の情報に更新(&F) */
#define MSGJPN263 (std::data(msgs[10000+263])) /* ダウンロード(&D) */
#define MSGJPN264 (std::data(msgs[10000+264])) /* 名前を変えてダウンロード(&W)... */
#define MSGJPN265 (std::data(msgs[10000+265])) /* ファイルとしてダウンロード(&I) */
#define MSGJPN266 (std::data(msgs[10000+266])) /* 全てをダウンロード */
#define MSGJPN267 (std::data(msgs[10000+267])) /* 削除(&R) */
#define MSGJPN268 (std::data(msgs[10000+268])) /* 名前変更(&N)... */
#define MSGJPN269 (std::data(msgs[10000+269])) /* 属性変更(&A)... */
#define MSGJPN270 (std::data(msgs[10000+270])) /* フォルダ作成(&K)... */
#define MSGJPN271 (std::data(msgs[10000+271])) /* URLをクリップボードへコピー(&C) */
#define MSGJPN272 (std::data(msgs[10000+272])) /* ファイル容量計算(&Z) */
#define MSGJPN273 (std::data(msgs[10000+273])) /* 最新の情報に更新(&F) */
#define MSGJPN274 (std::data(msgs[10000+274])) /* 開く(&O) */
#define MSGJPN275 (std::data(msgs[10000+275])) /* %sで開く(&%d) */
#define MSGJPN276 (std::data(msgs[10000+276])) /* WS_FTP.INI\0ws_ftp.ini\0全てのファイル\0*\0 */
#define MSGJPN277 (std::data(msgs[10000+277])) /* Waveファイル\0*.wav\0全てのファイル\0*\0 */
#define MSGJPN278 (std::data(msgs[10000+278])) /* 理由: %s */
#define MSGJPN279 (std::data(msgs[10000+279])) /* Listenソケットが取得できません */
#define MSGJPN280 (std::data(msgs[10000+280])) /* Dataソケットが取得できません */
#define MSGJPN281 (std::data(msgs[10000+281])) /* PASVモードで接続できません */
#define MSGJPN282 (std::data(msgs[10000+282])) /* INIファイル名が指定されていません */
#define MSGJPN283 (std::data(msgs[10000+283])) /* INIファイル指定:  */
#define MSGJPN284 (std::data(msgs[10000+284])) /* https://github.com/sayurin/ffftp */
#define MSGJPN285 (std::data(msgs[10000+285])) /* レジストリエディタが起動できませんでした */
#define MSGJPN286 (std::data(msgs[10000+286])) /* 設定ファイルの保存 */
#define MSGJPN287 (std::data(msgs[10000+287])) /* Regファイル\0*.reg\0全てのファイル\0*\0 */
#define MSGJPN288 (std::data(msgs[10000+288])) /* INIファイル\0*.ini\0全てのファイル\0*\0 */
#define MSGJPN289 (std::data(msgs[10000+289])) /* Agilent Logic analyzer */
#define MSGJPN290 (std::data(msgs[10000+290])) /* Regファイル\0*.reg\0INIファイル\0*.ini\0全てのファイル\0*\0 */
#define MSGJPN291 (std::data(msgs[10000+291])) /* 設定をファイルから復元 */
#define MSGJPN292 (std::data(msgs[10000+292])) /* 設定をファイルから復元するためには,FFFTPを再起動してください. */
#define MSGJPN293 (std::data(msgs[10000+293])) /* 設定ファイルは拡張子が.regか.iniでなければなりません. */
#define MSGJPN294 (std::data(msgs[10000+294])) /* USER FWユーザー名:FWパスワード@ホスト名 */
#define MSGJPN295 (std::data(msgs[10000+295])) /* シバソク WL */
#define MSGJPN296 (std::data(msgs[10000+296])) /* 読み取り専用ファイルです.読み取り専用属性を解除しますか？ */
#define MSGJPN297 (std::data(msgs[10000+297])) /* %s は不正なファイル名です.\r\nこのファイルはダウンロードされません. */
#define MSGJPN298 (std::data(msgs[10000+298])) /* OLEの初期化に失敗しました. */
#define MSGJPN299 (std::data(msgs[10000+299])) /* コマンドラインにマスターパスワードが指定されていません */
#define MSGJPN300 (std::data(msgs[10000+300])) /* デフォルトのマスターパスワードが使われます.\r\nマルウェアの攻撃を防ぐため,固有のマスターパスワードを設定することをおすすめします */
#define MSGJPN301 (std::data(msgs[10000+301])) /* マスターパスワードが設定と一致しません.安全のため設定の保存を行いません. */
#define MSGJPN302 (std::data(msgs[10000+302])) /* 確認用データが壊れているため,マスターパスワードの正当性を確認できませんでした. */
#define MSGJPN303 (std::data(msgs[10000+303])) /* マスターパスワードを変更しました */
#define MSGJPN304 (std::data(msgs[10000+304])) /* 指定されたマスターパスワードが登録されたものと一致しません.\r\n再度入力しますか？\r\n「いいえ」を選ぶと記憶されたFTPパスワードは利用できません. */
#define MSGJPN305 (std::data(msgs[10000+305])) /* Shift_JIS */
#define MSGJPN306 (std::data(msgs[10000+306])) /* UTF-8 */
#define MSGJPN307 (std::data(msgs[10000+307])) /* ホストの漢字コードはShift_JIS */
#define MSGJPN308 (std::data(msgs[10000+308])) /* ホストの漢字コードはUTF-8 */
#define MSGJPN309 (std::data(msgs[10000+309])) /* ローカルの漢字コードはShift_JIS */
#define MSGJPN310 (std::data(msgs[10000+310])) /* ローカルの漢字コードはEUC */
#define MSGJPN311 (std::data(msgs[10000+311])) /* ローカルの漢字コードはJIS */
#define MSGJPN312 (std::data(msgs[10000+312])) /* ローカルの漢字コードはUTF-8 */
#define MSGJPN313 (std::data(msgs[10000+313])) /* 暗号化 */
#define MSGJPN314 (std::data(msgs[10000+314])) /* 通信は暗号化されていません.\r\n第三者にパスワードおよび内容を傍受される可能性があります. */
#define MSGJPN315 (std::data(msgs[10000+315])) /* FTP over Explicit SSL/TLS (FTPES)を使用します. */
#define MSGJPN316 (std::data(msgs[10000+316])) /* FTP over Implicit SSL/TLS (FTPIS)を使用します. */
#define MSGJPN317 (std::data(msgs[10000+317])) /* SSH FTP (SFTP)を使用します. */
#define MSGJPN320 (std::data(msgs[10000+320])) /* 特殊機能 */
#define MSGJPN321 (std::data(msgs[10000+321])) /* プロセスの保護に必要な関数を取得できませんでした. */
#define MSGJPN322 (std::data(msgs[10000+322])) /* デバッガが検出されました. */
#define MSGJPN323 (std::data(msgs[10000+323])) /* 信頼できないDLLをアンロードできませんでした. */
#define MSGJPN324 (std::data(msgs[10000+324])) /* プロセスの保護に必要な関数をフックできませんでした. */
#define MSGJPN325 (std::data(msgs[10000+325])) /* 新しいマスターパスワードが一致しません. */
#define MSGJPN329 (std::data(msgs[10000+329])) /* UTF-8 BOM */
#define MSGJPN330 (std::data(msgs[10000+330])) /* ホストの漢字コードはUTF-8 BOM */
#define MSGJPN331 (std::data(msgs[10000+331])) /* ローカルの漢字コードはUTF-8 BOM */
#define MSGJPN332 (std::data(msgs[10000+332])) /* 自動 */
#define MSGJPN333 (std::data(msgs[10000+333])) /* TCP/IPv4 */
#define MSGJPN334 (std::data(msgs[10000+334])) /* TCP/IPv6 */
#define MSGJPN335 (std::data(msgs[10000+335])) /* 毎回尋ねる */
#define MSGJPN336 (std::data(msgs[10000+336])) /* 全て後で上書き */
#define MSGJPN337 (std::data(msgs[10000+337])) /* 全て後でリジューム */
#define MSGJPN338 (std::data(msgs[10000+338])) /* 全てスキップ */
#define MSGJPN339 (std::data(msgs[10000+339])) /* 転送4 */
#define MSGJPN340 (std::data(msgs[10000+340])) /* 表示2 */
#define MSGJPN341 (std::data(msgs[10000+341])) /* WindowsファイアウォールのステートフルFTPフィルタの有効無効を設定します.\nこれはWindows Vista以降でのみ動作します.\n有効化または無効化することで通信状態が改善されることがあります.\n有効化するには「はい」,無効化するには「いいえ」を選択してください. */
#define MSGJPN342 (std::data(msgs[10000+342])) /* ステートフルFTPフィルタを設定できませんでした. */
#define MSGJPN343 (std::data(msgs[10000+343])) /* ファイル名の漢字コードの判別結果は%sです. */
#define MSGJPN344 (std::data(msgs[10000+344])) /* ファイル名の漢字コードを判別できませんでした. */
#define MSGJPN345 (std::data(msgs[10000+345])) /* Shift_JIS */
#define MSGJPN346 (std::data(msgs[10000+346])) /* JIS */
#define MSGJPN347 (std::data(msgs[10000+347])) /* EUC */
#define MSGJPN348 (std::data(msgs[10000+348])) /* UTF-8 */
#define MSGJPN349 (std::data(msgs[10000+349])) /* UTF-8 HFS+ */
#define MSGJPN350 (std::data(msgs[10000+350])) /* 新しいバージョンの設定が検出されました.\nこのバージョンでは一部の設定が正しく読み込まれない,またはこのバージョンで設定を上書きすると設定が変化する可能性があります.\nこのバージョン用に設定を上書きして保存するには「はい」を選択してください.\n設定をレジストリではなくINIファイルに保存するには「いいえ」を選択してください.\n読み取り専用で設定を読み込むには「キャンセル」を選択してください. */
#define MSGJPN355 (std::data(msgs[10000+355])) /* 一つ上のフォルダへ移動(&P)... */
#define MSGJPN356 (std::data(msgs[10000+356])) /* XMLファイル\0*.xml\0全てのファイル\0*\0 */
#define MSGJPN357 (std::data(msgs[10000+357])) /* 設定のエクスポートに失敗しました.\n保存する場所や形式を変更してください. */
#define MSGJPN358 (std::data(msgs[10000+358])) /* ソフトウェアの更新が完了しました.\n以前のバージョンのバックアップコピーは以下の場所にあります.\n%s */
#define MSGJPN359 (std::data(msgs[10000+359])) /* ソフトウェアの更新に失敗しました.\nWebサイトから最新版を入手して手動で更新してください. */
#define MSGJPN365 (std::data(msgs[10000+365])) /* この機能で新規作成したINIファイルをWinSCPで読み込むと全ての設定が失われます.\nすでにWinSCPをお使いでありホストの設定のみ移行したい場合は既存のWinSCP.iniを選択してください. */
#if defined(HAVE_TANDEM)
#define MSGJPN2000 (std::data(msgs[10000+2000])) /* NonStop Server */
#define MSGJPN2001 (std::data(msgs[10000+2001])) /* OSS<->GUARDIAN 切り替え(&O) */
#endif
