#pragma once
extern std::map<int, std::string> msgs;
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
#define MSGJPN078 (std::data(msgs[10000+78])) /* フォルダ作成 */
#define MSGJPN079 (std::data(msgs[10000+79])) /* フォルダ作成 */
#define MSGJPN080 (std::data(msgs[10000+80])) /* フォルダ削除 */
#define MSGJPN081 (std::data(msgs[10000+81])) /* ファイル削除 */
#define MSGJPN082 (std::data(msgs[10000+82])) /* フォルダ作成 */
#define MSGJPN083 (std::data(msgs[10000+83])) /* フォルダ削除 */
#define MSGJPN084 (std::data(msgs[10000+84])) /* ファイル削除 */
#define MSGJPN086 (std::data(msgs[10000+86])) /* ダウンロード */
#define MSGJPN087 (std::data(msgs[10000+87])) /* ファイル一覧取得 */
#define MSGJPN088 (std::data(msgs[10000+88])) /* スキップ */
#define MSGJPN091 (std::data(msgs[10000+91])) /* ダウンロードのために */
#define MSGJPN093 (std::data(msgs[10000+93])) /* アドレスが取得できません. */
#define MSGJPN094 (std::data(msgs[10000+94])) /* 受信はタイムアウトで失敗しました. */
#define MSGJPN095 (std::data(msgs[10000+95])) /* ファイル %s が作成できません. */
#define MSGJPN096 (std::data(msgs[10000+96])) /* ディスクがいっぱいで書き込めません. */
#define MSGJPN098 (std::data(msgs[10000+98])) /* ファイル一覧 */
#define MSGJPN104 (std::data(msgs[10000+104])) /* アップロード */
#define MSGJPN105 (std::data(msgs[10000+105])) /* ファイル %s が読み出せません. */
#define MSGJPN106 (std::data(msgs[10000+106])) /* スキップ */
#define MSGJPN109 (std::data(msgs[10000+109])) /* アップロードのために */
#define MSGJPN112 (std::data(msgs[10000+112])) /* ファイル %s がオープンできません. */
#define MSGJPN133 (std::data(msgs[10000+133])) /* GMT%+02d:00 (日本) */
#define MSGJPN147 (std::data(msgs[10000+147])) /* フォルダを削除できません. */
#define MSGJPN149 (std::data(msgs[10000+149])) /* ファイルを削除できません. */
#define MSGJPN185 (std::data(msgs[10000+185])) /* フォルダを選択してください */
#define MSGJPN239 (std::data(msgs[10000+239])) /* # このファイルは編集しないでください.\n */
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
#define MSGJPN317 (std::data(msgs[10000+317])) /* SSH FTP (SFTP)を使用します. */
#define MSGJPN321 (std::data(msgs[10000+321])) /* プロセスの保護に必要な関数を取得できませんでした. */
#define MSGJPN322 (std::data(msgs[10000+322])) /* デバッガが検出されました. */
#define MSGJPN323 (std::data(msgs[10000+323])) /* 信頼できないDLLをアンロードできませんでした. */
#define MSGJPN324 (std::data(msgs[10000+324])) /* プロセスの保護に必要な関数をフックできませんでした. */
#define MSGJPN358 (std::data(msgs[10000+358])) /* ソフトウェアの更新が完了しました.\n以前のバージョンのバックアップコピーは以下の場所にあります.\n%s */
#define MSGJPN359 (std::data(msgs[10000+359])) /* ソフトウェアの更新に失敗しました.\nWebサイトから最新版を入手して手動で更新してください. */
