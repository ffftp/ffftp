/*=============================================================================
*							ＦＦＦＴＰ共通定義ファイル
*
===============================================================================
/ Copyright (C) 1997-2007 Sota. All rights reserved.
/
/ Redistribution and use in source and binary forms, with or without 
/ modification, are permitted provided that the following conditions 
/ are met:
/
/  1. Redistributions of source code must retain the above copyright 
/     notice, this list of conditions and the following disclaimer.
/  2. Redistributions in binary form must reproduce the above copyright 
/     notice, this list of conditions and the following disclaimer in the 
/     documentation and/or other materials provided with the distribution.
/
/ THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
/ IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
/ OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
/ IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
/ INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
/ USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
/ ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
/ THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/============================================================================*/

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
#include <cassert>
#include <crtdbg.h>
#include <mbstring.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <windowsx.h>
#include <winsock2.h>
#include <commdlg.h>
#include <HtmlHelp.h>
#include <MMSystem.h>
#include <mstcpip.h>
#include <shellapi.h>
#include <WinCrypt.h>
#include <WS2tcpip.h>
#include "config.h"
#include "mbswrapper.h"
#include "socketwrapper.h"
#include "Resource/resource.ja-JP.h"
#include "mesg-jpn.h"
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "HtmlHelp.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Ws2_32.lib")
namespace fs = std::experimental::filesystem;
using namespace std::literals;
template<class...>
constexpr bool false_v = false;


#define NUL				'\0'

#define LOW8(x)			((x) & 0xFF)
#define HIGH8(x)		(((x) >> 8) & 0xFF)
#define LOW32(x)		((x) & 0xFFFFFFFF)
#define HIGH32(x)		(((x) >> 32) & 0xFFFFFFFF)
#define N2INT(h,l)		((int)(((uchar)(h) << 4) + (uchar)(l)))

#define IsDigit(n)		(isascii(n) && isdigit(n))
#define IsAlpha(n)		(isascii(n) && isalpha(n))

#define uchar			unsigned char
#define ushort			unsigned short
#define ulong			unsigned long

// 警告の回避
//#define FAIL			0
//#define SUCCESS			1
#define FFFTP_FAIL			0
#define FFFTP_SUCCESS			1

#define NO				0
#define YES				1
#define NO_ALL			2
#define YES_ALL			3
#define YES_LIST		4

/*===== バージョン ======*/

// SourceForge.JPによるフォーク
//#define VER_STR					"1.97b"
//#define VER_NUM					1921		/* 設定バージョン */
//#define PROGRAM_VERSION_NUM		1972		/* バージョン */
// 64ビット対応
#ifdef _WIN64
#define VER_STR					"1.99a-20171104 64bit"
#else
#define VER_STR					"1.99a-20171104"
#endif
#define VER_NUM					1990		/* 設定バージョン */
#define PROGRAM_VERSION_NUM		1990		/* バージョン */
// ソフトウェア自動更新
// リリースバージョンはリリース予定年（10進数4桁）+月（2桁）+日（2桁）+通し番号（0スタート2桁）とする
// 2014年7月31日中の30個目のリリースは2014073129
#define RELEASE_VERSION_NUM		2017110400	/* リリースバージョン */


// SourceForge.JPによるフォーク
//#define MYWEB_URL	"http://www2.biglobe.ne.jp/~sota/ffftp-qa.html"
#define MYWEB_URL	"https://osdn.jp/projects/ffftp/forums/"



/*===== 通信関係 ======*/

#define TCP_PORT		6

/*===== ウインドウサイズ ======*/

#define TOOLWIN_HEIGHT	28		/* ツールバーの高さ */

/*===== 特殊なキャッシュデータ番号 =====*/
/* （ファイル一覧取得で使用するローカルファイル名 _ffftp.??? の番号部分） */

#define CACHE_FILE_TMP1	999		/* ホストのファイルツリー取得用 */
#define CACHE_FILE_TMP2	998		/* アップロード中のホストのファイル一覧取得用 */

/*===== ユーザ定義コマンド =====*/

#define WM_CHANGE_COND	(WM_USER+1)	/* ファイル一覧を変更するコマンド */
#define WM_SET_PACKET	(WM_USER+2)	/* 現在使用している転送パケットのアドレスを通知 */
#define WM_SELECT_HOST	(WM_USER+3)	/* ホストをダブルクリックで選択した */
#define WM_DIAL_MSG		(WM_USER+4)	/* ダイアル中のステータス通知 */

#define WM_ASYNC_SOCKET	(WM_USER+5)
#define WM_ASYNC_DBASE	(WM_USER+6)

#define WM_REFRESH_LOCAL_FLG	(WM_USER+7)
#define WM_REFRESH_REMOTE_FLG	(WM_USER+8)

// UPnP対応
#define WM_ADDPORTMAPPING	(WM_USER+9)
#define WM_REMOVEPORTMAPPING	(WM_USER+10)

// 同時接続対応
#define WM_RECONNECTSOCKET	(WM_USER+11)

/*===== ホスト番号 =====*/
/* ホスト番号は 0～ の値を取る */

#define HOSTNUM_NOENTRY	(-1)	/* ホスト一覧に無いときのホスト番号 */

/*===== バッファサイズ =====*/

#define BUFSIZE			4096	/* ファイル転送バッファのサイズ(4k以上) */

#define HOST_NAME_LEN	40		/* 一覧に表示するホストの名前 */
#define HOST_ADRS_LEN	80		/* ホスト名 */
#define USER_NAME_LEN	80		/* ユーザ名 */
#define PASSWORD_LEN	80		/* パスワード */
#define ACCOUNT_LEN		80		/* アカウント */
#define HOST_TYPE_LEN	1		/* ホストの種類 */
#define INIT_DIR_LEN	(FMAX_PATH-40)	/* 初期ディレクトリ */
#define USER_MAIL_LEN	80		/* ユーザのメールアドレス */
								/*   PASSWORD_LEN と同じにすること */
#define ASCII_EXT_LEN	400		/* アスキーモード転送のファイル名列 */
#define FILTER_EXT_LEN	400		/* フィルタのファイル名列 */
#define BOOKMARK_SIZE	2048	/* ブックマーク */
#define CHMOD_CMD_LEN	40		/* 属性変更コマンド */
#define MIRROR_LEN		400		/* ミラーリングの設定用 */
#define NLST_NAME_LEN	40		/* NLSTに付けるファイル名／オプション */
#define DEFATTRLIST_LEN	800		/* 属性リストの長さ */
#define INITCMD_LEN		256		/* 初期化コマンド */
#define OWNER_NAME_LEN	40		/* オーナ名 */
#define RAS_NAME_LEN	256		/* RASのエントリ名の長さ */

#define FMAX_PATH		1024

#define ONELINE_BUF_SIZE	(10*1024)

// 暗号化通信対応
#define PRIVATE_KEY_LEN 4096

/*===== 初期値 =====*/

#define SAMBA_HEX_TAG	':'				/* Samba-HEX の区切り文字 */
#define CHMOD_CMD_NOR	"SITE CHMOD"	/* 属性変更コマンド */
#define PORT_NOR		21				/* ポート番号 */
#define LS_FNAME		"-alL"			/* NLSTに付けるもの */
#if defined(HAVE_TANDEM)
#define DEF_PRIEXT		4				/* Primary Extents の初期値 */
#define DEF_SECEXT		28				/* Secondary Extents の初期値 */
#define DEF_MAXEXT		978				/* Max Extents の初期値 */
#endif

/*===== 同じ名前のファイルがあった時の処理 =====*/

#define EXIST_OVW		0		/* 上書き */
#define EXIST_NEW		1		/* 新しければ上書き */
#define EXIST_RESUME	2		/* レジューム */
#define EXIST_IGNORE	3		/* 無視 */
#define EXIST_UNIQUE	4		/* ホストが名前を付ける */
#define EXIST_ABORT		5		/* 全て中止 */
// 同じ名前のファイルの処理方法追加
#define EXIST_LARGE		6		/* 大きければ上書き */

/*===== ファイル名の比較モード =====*/

#define COMP_IGNORE		0		/* 大文字/小文字は区別しない */
#define COMP_STRICT		1		/* 大文字/小文字を区別する */
#define COMP_LOWERMATCH	2		/* 大文字/小文字を区別しない（片側は全て小文字） */

/*===== FTPの応答コードの頭１桁 =====*/

#define FTP_PRELIM		1		/* */
#define FTP_COMPLETE	2		/* */
#define FTP_CONTINUE	3		/* */
#define FTP_RETRY		4		/* */
#define FTP_ERROR		5		/* */

/*===== ファイルリストのノード属性 =====*/

#define NODE_DIR		0		/* ディレクトリ */
#define NODE_FILE		1		/* ファイル */
#define NODE_DRIVE		2		/* ドライブ */
#define NODE_NONE		(-1)	/* なし */

/*===== 上書き確認方法の設定値 =====*/

#define TRANS_OVW		0		/* 上書き */
#define TRANS_DLG		1		/* ダイアログを出す */

/*===== ホスト内ファイル移動確認方法の設定値 =====*/

#define MOVE_NODLG		0		/* ダイアログを出さない */
#define MOVE_DLG		1		/* ダイアログを出す */
#define MOVE_DISABLE	2		/* 機能使用禁止 */

/*===== 転送方法 =====*/

#define TYPE_I			'I'		/* バイナリモード */
#define TYPE_A			'A'		/* アスキーモード */
#define TYPE_X			'X'		/* 自動判別 */

#define TYPE_DEFAULT	NUL		/* 設定しない DirectConnectProc()の引数として有効 */

/*===== ソート方法 =====*/
/* 番号はListViewのカラム番号と合わせてある */

#define SORT_NAME		0		/* 名前順 */
#define SORT_DATE		1		/* 日付順 */
#define SORT_SIZE		2		/* サイズ順 */
#define SORT_EXT		3		/* 拡張子順 */

#define SORT_ASCENT		0x00	/* 昇順 */
#define SORT_DESCENT	0x80	/* 降順 */

#define SORT_MASK_ORD	0x7F	/* ｘｘ順を取り出すマスク */
#define SORT_GET_ORD	0x80	/* 昇順／降順を取り出すマスク */

#define SORT_NOTSAVED	((ulong)0xFFFFFFFF)	/* ホスト毎のセーブ方法を保存していない時の値 */

/*===== ソートする場所 =====*/

#define ITEM_LFILE		0		/* ローカルの名前 */
#define ITEM_LDIR		1		/* ローカルのディレクトリ */
#define ITEM_RFILE		2		/* ホストの名前 */
#define ITEM_RDIR		3		/* ホストのディレクトリ */

/*===== ウインドウ番号 =====*/

#define WIN_LOCAL		0		/* ローカル */
#define WIN_REMOTE		1		/* ホスト */
#define WIN_BOTH		2		/* 両方 */

/*===== ファイル選択方法 =====*/

#define SELECT_ALL		0		/* 全選択 */
#define SELECT_REGEXP	1		/* 検索式を入力して選択 */
// ローカル側自動更新
#define SELECT_LIST		2		/* リストに含まれるファイルを選択 */

/*===== 検索方法 =====*/

#define FIND_FIRST		0		/* 最初の検索 */
#define FIND_NEXT		1		/* 次を検索 */

/*===== ファイル名の大文字／小文字変換 =====*/

#define FNAME_NOCNV		0		/* 変換しない */
#define FNAME_LOWER		1		/* 小文字に変換 */
#define FNAME_UPPER		2		/* 大文字に変換 */

/*===== ファイル一覧の形式 =====*/

#define LIST_UNKNOWN	(-1)	/* 不明 */

#define LIST_UNIX_10	0		/* UNIX 10 */
#define LIST_UNIX_11	1		/* UNIX 11 */
#define LIST_UNIX_12	2		/* UNIX 12 */
#define LIST_UNIX_13	3		/* UNIX 13 */
#define LIST_UNIX_14	4		/* UNIX 14 */
#define LIST_UNIX_15	5		/* UNIX 15 */
#define LIST_UNIX_20	6		/* UNIX 20 */
#define LIST_UNIX_21	7		/* UNIX 21 */
#define LIST_UNIX_22	8		/* UNIX 22 */
#define LIST_UNIX_23	9		/* UNIX 23 */
#define LIST_UNIX_24	10		/* UNIX 24 */
#define LIST_UNIX_25	11		/* UNIX 25 */
#define LIST_UNIX_50	12		/* UNIX 50 */
#define LIST_UNIX_51	13		/* UNIX 51 */
#define LIST_UNIX_54	14		/* UNIX 54 */
#define LIST_UNIX_60	15		/* UNIX 60 */
#define LIST_UNIX_61	16		/* UNIX 61 */
#define LIST_UNIX_62	17		/* UNIX 62 */
#define LIST_UNIX_63	18		/* UNIX 63 */
#define LIST_UNIX_64	19		/* UNIX 64 */
#define LIST_UNIX_65	20		/* UNIX 65 */
#define LIST_DOS_1		21		/* MS-DOS 1 */
#define LIST_DOS_2		22		/* MS-DOS 2 */
#define LIST_DOS_3		23		/* MS-DOS 3 */
#define LIST_DOS_4		24		/* MS-DOS 4 */
#define LIST_ACOS		25		/* ACOS */
#define LIST_AS400		26		/* AS/400 */
#define LIST_M1800		27		/* Fujitu M1800 (OS IV/MSP E20) */
#define LIST_CHAMELEON	28		/* Win3.1用 Chameleon FTP server */
#define LIST_GP6000		29		/* Fujitu GP6000 Model 900 */
#define LIST_OS2		30		/* OS/2 */
#define LIST_VMS		31		/* VAX VMS */
#define LIST_OS7_1		32		/* Toshiba OS7 */
#define LIST_OS7_2		33		/* Toshiba OS7 */
#define LIST_IRMX		34		/* IRMX */
#define LIST_ACOS_4		35		/* ACOS-4 */
#define LIST_STRATUS	36		/* Stratus */
#define LIST_ALLIED		37		/* allied telesis (DOS) */
#define LIST_OS9		38		/* OS/9 */
#define LIST_IBM		39		/* IBM host */
#define LIST_AGILENT	40		/* Agilent logic analyzer */
#define LIST_SHIBASOKU	41		/* Shibasoku LSI test system */
#define LIST_UNIX_70	42		/* UNIX 70 */
#define LIST_UNIX_71	43		/* UNIX 71 */
#define LIST_UNIX_72	44		/* UNIX 72 */
#define LIST_UNIX_73	45		/* UNIX 73 */
#define LIST_UNIX_74	46		/* UNIX 74 */
#define LIST_UNIX_75	47		/* UNIX 75 */
// linux-ftpd
#define LIST_UNIX_16	48		/* UNIX 16 */
// MLSD対応
#define LIST_MLSD		49
#if defined(HAVE_TANDEM)
#define LIST_TANDEM		50		/* HP NonStop Server */
#endif
// uClinux
#define LIST_UNIX_17	51		/* UNIX 17 */
// Windows Server 2008 R2
#define LIST_DOS_5		52		/* MS-DOS 5 */

#define LIST_MELCOM		0x100	/* MELCOM80 */

#define LIST_MASKFLG	0xFF

// UTF-8対応
#define LIST_RAW_NAME	0x80000000

/* ファイル一覧情報例 ---------------

*LIST_UNIX_10
	0          1   2     3      4    5    6   7         8
	-------------------------------------------------------
	drwxr-xr-x 15  owner group  1024 Nov  6   14:21     Linux/
	-rwxrwx---  5  owner group    12 Nov  6   1996      test.txt
	drwxr-xr-x 15  owner group  1024 11月 6日 14:21     Linux/
	drwxr-xr-x 15  owner group  1024 11月 6日 14時21分  Linux/
	-rwxrwx---  5  owner group    12 11月 6日 1996年    test.txt
	drwxrwxr-x 6   root  sys     512  1月 26  03:10     adm		(月はGBコードで0xD4C2)

*LIST_UNIX_11
	0          1   2     3      4    5        6         7
	-------------------------------------------------------
	drwxr-xr-x 15  owner group  1024 11月12日 14時21分  Linux/
	-rwxrwx---  5  owner group    12 11月12日 1996年    test.txt

*LIST_UNIX_12
	0              1     2      3    4    5   6         7
	-------------------------------------------------------
	drwxr-xr-x123  owner group  1024 Nov  6   14:21     Linux/
	-rwxrwx---132  owner group    12 Nov  6   1996      test.txt
	drwxr-xr-x123  owner group  1024 11月 6日 14:21     Linux/
	drwxr-xr-x123  owner group  1024 11月 6日 14時21分  Linux/
	-rwxrwx---132  owner group    12 11月 6日 1996年    test.txt

*LIST_UNIX_13
	0              1     2      3    4        5         6
	-------------------------------------------------------
	drwxr-xr-x123  owner group  1024 11月12日 14時21分  Linux/
	-rwxrwx---132  owner group    12 11月12日 1996年    test.txt

*LIST_UNIX_14
	0          1   2     3      4    5    6   7         8
	-------------------------------------------------------
	drwxr-xr-x 15  owner group  512  2001 6月 18        audit	(月はGBコードで0xD4C2)

*LIST_UNIX_15
	0              1     2      3    4    5   6         7
	-------------------------------------------------------
	drwxr-xr-x15   owner group  512  2001 6月 18        audit	(月はGBコードで0xD4C2)





*LIST_UNIX_20
	0          1   2            3    4    5   6         7
	-------------------------------------------------------
	drwxr-xr-x 15  owner        1024 Nov  6   14:21     Linux/
	-rwxrwx---  5  owner          12 Nov  6   1996      test.txt
	drwxr-xr-x 15  owner        1024 11月 6日 14:21     Linux/
	drwxr-xr-x 15  owner        1024 11月 6日 14時21分  Linux/
	-rwxrwx---  5  owner          12 11月 6日 1996年    test.txt

*LIST_UNIX_21
	0          1   2            3    4        5         6
	-------------------------------------------------------
	drwxr-xr-x 15  owner        1024 11月12日 14時21分  Linux/
	-rwxrwx---  5  owner          12 11月12日 1996年    test.txt

*LIST_UNIX_22
	0              1            2    3    4   5         6
	-------------------------------------------------------
	drwxr-xr-x123  owner        1024 Nov  6   14:21     Linux/
	-rwxrwx---132  owner          12 Nov  6   1996      test.txt
	drwxr-xr-x123  owner        1024 11月 6日 14:21     Linux/
	drwxr-xr-x123  owner        1024 11月 6日 14時21分  Linux/
	-rwxrwx---132  owner          12 11月 6日 1996年    test.txt

*LIST_UNIX_23
	0              1            2    3        4         5
	-------------------------------------------------------
	drwxr-xr-x123  owner        1024 11月12日 14時21分  Linux/
	-rwxrwx---132  owner          12 11月12日 1996年    test.txt

*LIST_UNIX_24
	0          1   2            3    4    5   6         7
	-------------------------------------------------------
	drwxr-xr-x 15  owner        512  2001 6月 18        audit	(月はGBコードで0xD4C2)

*LIST_UNIX_25
	0              1            2    3    4   5         6
	-------------------------------------------------------
	drwxr-xr-x15   owner        512  2001 6月 18        audit	(月はGBコードで0xD4C2)







*LIST_UNIX_50
	0              1            2    3    4   5         6
	-------------------------------------------------------
	drwxr-xr-x     owner        1024 Nov  6   14:21     Linux/
	-rwxrwx---     owner          12 Nov  6   1996      test.txt
	drwxr-xr-x     owner        1024 11月 6日 14:21     Linux/
	drwxr-xr-x     owner        1024 11月 6日 14時21分  Linux/
	-rwxrwx---     owner          12 11月 6日 1996年    test.txt

*LIST_UNIX_51
	0              1            2    3        4         5
	-------------------------------------------------------
	drwxr-xr-x     owner        1024 11月12日 14時21分  Linux/
	-rwxrwx---     owner          12 11月12日 1996年    test.txt

	0          1   2        3        4        5
	-------------------------------------------------------
	-rwxrwxrwx SEQ 36203776 01/07/07 12:38:28 ADRS001                         
	-rwxrwxrwx SEQ 70172160 01/07/07 13:59:58 ADRS002                         

*LIST_UNIX_54
	0              1            2    3    4   5         6
	-------------------------------------------------------
	drwxr-xr-x     owner        512  2001 6月 18        audit	(月はGBコードで0xD4C2)







*LIST_UNIX_60
	0          1    2     3 4     5 6    7    8  9     10
	-------------------------------------------------------
	drwxr-xr-x 123  owner m group g 1024 Nov  6  14:21 Linux/
	-rwxrwx--- 132  owner m group g   12 Nov  6  1996  test.txt

*LIST_UNIX_61
	0          1    2     3 4     5 6    7         8     9
	-------------------------------------------------------
	drwxr-xr-x 123  owner m group g 1024 11月12日  14:21 Linux/
	-rwxrwx--- 132  owner m group g   12 11月12日  1996  test.txt

*LIST_UNIX_62
	0              1     2 3     4 5    6    7  8     9
	-------------------------------------------------------
	drwxr-xr-x123  owner m group g 1024 Nov  6  14:21 Linux/
	-rwxrwx---132  owner m group g   12 Nov  6  1996  test.txt

*LIST_UNIX_63
	0              1     2 3     4 5    6         7     8
	-------------------------------------------------------
	drwxr-xr-x123  owner m group g 1024 11月12日  14:21 Linux/
	-rwxrwx---132  owner m group g   12 11月12日  1996  test.txt

*LIST_UNIX_64
	0          1   2     3 4     5  6    7    8   9    10
	-------------------------------------------------------
	drwxr-xr-x 15  owner m group g  512  2001 6月 18   audit	(月はGBコードで0xD4C2)

*LIST_UNIX_65
	0              1     2 3     4  5    6    7   8    9
	-------------------------------------------------------
	drwxr-xr-x15   owner m group g  512  2001 6月 18   audit	(月はGBコードで0xD4C2)




LIST_UNIX_70
	0          1    2       3     4 5    6    7  8     9
	-------------------------------------------------------
	drwxr-xr-x 123  owner   group g 1024 Nov  6  14:21 Linux/
	-rwxrwx--- 132  owner   group g   12 Nov  6  1996  test.txt

*LIST_UNIX_71
	0          1    2       3     4 5    6         7     8
	-------------------------------------------------------
	drwxr-xr-x 123  owner   group g 1024 11月12日  14:21 Linux/
	-rwxrwx--- 132  owner   group g   12 11月12日  1996  test.txt

*LIST_UNIX_72
	0              1       2     3 4    5    6  7     8
	-------------------------------------------------------
	drwxr-xr-x123  owner   group g 1024 Nov  6  14:21 Linux/
	-rwxrwx---132  owner   group g   12 Nov  6  1996  test.txt

*LIST_UNIX_73
	0              1       2     3 4    5         6     7
	-------------------------------------------------------
	drwxr-xr-x123  owner   group g 1024 11月12日  14:21 Linux/
	-rwxrwx---132  owner   group g   12 11月12日  1996  test.txt

*LIST_UNIX_74
	0          1   2       3     4  5    6    7   8    9
	-------------------------------------------------------
	drwxr-xr-x 15  owner   group g  512  2001 6月 18   audit	(月はGBコードで0xD4C2)

*LIST_UNIX_75
	0              1       2     3  4    5    6   7    8
	-------------------------------------------------------
	drwxr-xr-x15   owner   group g  512  2001 6月 18   audit	(月はGBコードで0xD4C2)






*unix系で以下のような日付
	0              1            2    3   4    5         6
	-------------------------------------------------------
	drwxr-xr-x123  owner        1024 11/ 6    14:21     Linux/
	-rwxrwx---132  owner          12 11/13    1996      test.txt


















*LIST_DOS_1
	0         1          2       3
	-------------------------------------------------------
	97-10-14  03:34p     <DIR>   Linux
	97-10-14  03:34p        12   test.txt
	100-10-14 03:34p        12   test.txt

*LIST_DOS_2
	0         1          2       3
	-------------------------------------------------------
	10-14-97  03:34p     <DIR>   Linux
	10-14-97  03:34p        12   test.txt
	10-14-100 03:34p        12   test.txt

*LIST_DOS_3
	0             1      2         3       4
	-------------------------------------------------------
	Linux         <DIR>  10-14-97  03:34    
	test.txt         12  10-14-97  14:34   A
	test.txt         12  10-14-100 14:34   A

*LIST_DOS_4
	0          1            2        3
	-------------------------------------------------------
	1998/07/30 15:39:02     <DIR>    Linux
	1998/07/30 15:42:19     11623    test.txt

*LIST_ACOS
	0
	-------------------------------------------------------
	test.txt
	ディレクトリなし、

*LIST_AS400
	0           1     2        3        4        5
	-------------------------------------------------------
	QSYS        18944 96/09/20 00:35:10 *DIR     QOpenSys/
	QDOC        26624 70/01/01 00:00:00 *FLR     QDLS/
	QSYS            0 98/09/27 10:00:04 *LIB     QSYS.LIB/
	QSECOFR         0 98/05/15 16:01:15 *STMF    WWWTEST.BAK

*LIST_M1800
	0     1     2       3       4     5         6 (ファイル名の後ろにスペースあり）
	-------------------------------------------------------
	drwx  F        400     400  PO    93.10.27  COMMON.PDL.EXCEL/       
	-rw-  F      10000   10000  DA    97.03.04  DTSLOG1.FNA             
	-rw-  F      10000  ******  DA    97.03.04  DTSBRB.FNA              
	drwx  U     ******    6144  PO    96.12.15  IS01.TISPLOAD/          
	-rw-  ****  ******  ******  VSAM  **.**.**  HICS.CMDSEQ             

*LIST_CHAMELEON
	0            1        2    3 4    5     6
	-------------------------------------------------------
	Linux        <DIR>    Nov  6 1997 14:21 drw-
	test.txt           12 Nov  6 1886 14:21 -rwa

*LIST_GP6000
	0          1        2        3        4        5    6
	-------------------------------------------------------
	drwxrwxrwx 98.10.21 14:38:46 SYSG03   XSYSOPR  2048 atlib
	-rwxrwxrwx 97.10.30 11:06:04 XSYSMNGR XSYSOPR  2048 blib

*LIST_OS2
	   0        1          2          3      4
	-------------------------------------------------------
	   345      A          12-02-98   10:59  VirtualDevice.java
	     0           DIR   12-09-98   09:43  ディレクトリ
	     0           DIR   12-09-100  09:43  ディレクトリ

*LIST_MELCOM
	0 1           2   3          4  5    6  7    8
	---------------------------------------------------------------
	- RW-RW-RW-   1   TERA       50 DEC  1  1997 AAAJ          B(B)
	- RW-RW-RW-   1   TERA        1 AUG  7  1998 12345678901234B(B)
	d RWXRWXRWX   2   TERA       64 NOV 13  1997 Q2000         -

*LIST_VMS
	0                  1         2           3         4
	---------------------------------------------------------------
	CIM_ALL.MEM;5        2/4     21-APR-1998 11:01:17  [CIM,MIZOTE]
	(RWED,RWED,RE,)
	MAIL.DIR;1         104/248   18-SEP-2001 16:19:39  [CIM,MIZOTE]
	(RWE,RWE,,)
		※VMSの場合一覧が複数行に別れる場合がある

*LIST_OS7_1
	0                       1        2        3
	---------------------------------------------------------------
	drwxrwxrwx              99/05/13 11:38:34 APL
*LIST_OS7_2
	0          1      2     3        4        5
	---------------------------------------------------------------
	-rwxrwxrwx SEQ    17408 96/12/06 10:11:27 INIT_CONFIG

*LIST_IRMX
	0          1   2     3  4       5       6 7 8         9  10  11
	---------------------------------------------------------------
	world      DR  DLAC  1    416   1,024   1 WORLD       05 FEB 98
	world      DR        1    416   1,024   1 WORLD       05 FEB 98
	name.f38       DRAU  5  4,692   1,024   1 # 0         24 MAR 99
	name.f38             5  4,692   1,024   1 # 0         24 MAR 99

*LIST_STRATUS
	 0      1  2         3        4         5
	---------------------------------------------------------------
	Files: 15  Blocks: 29
	 w      1  seq       99-06-15 13:11:39  member_srv.error
	Dirs: 74
	 m      3  98-12-25 16:14:58  amano

*LIST_ALLIED
	 0             1        2   3   4  5        6
	---------------------------------------------------------------
	     41622     IO.SYS   Tue Dec 20 06:20:00 1994
	<dir>             DOS   Wed Nov 24 09:35:48 1999

*LIST_OS9
	 0       1        2     3            4      5      6
	---------------------------------------------------------------
	 0.0     01/02/13 0945  d-----wr     3C0    148724 W_017
	 0.0     01/02/13 0945  ------wr     C20     48828 W_017.CLG

*LIST_IBM
	 0      1      2           3  4    5      6   7      8   9
	---------------------------------------------------------------
	 JXSIB1 3390   2000/12/27  1  810  FB     240 24000  PO  DIRNAME
	 JXSW01 3390   2000/12/27  1    5  VBA    240  3120  PS  FILENAME

*LIST_AGILENT
	 0             1    2    3      4     5
	---------------------------------------------------------------
	 drwxrwxrwx    1    1    1      1024  system
	 -rw-rw-rw-    1    1    1      1792  abc.aaa

*LIST_SHIBASOKU
	 0        1            2          3                 4
	---------------------------------------------------------------
	   512    Jan-30-2002  14:52:04   DIRNAME           <DIR>
	 61191    Aug-30-2002  17:30:38   FILENAME.C        


// linux-ftpd
*LIST_UNIX_16
	0          1   2     3      4    5          6     7
	-------------------------------------------------------
	合計 12345
	drwxr-x--- 2 root root      4096 2011-12-06 23:39 .
	drwxr-x--- 3 root root      4096 2011-12-06 23:39 ..
	-rw-r----- 1 root root       251 2011-12-06 23:39 .hoge

// uClinux
*LIST_UNIX_17
	0          1 2 3 4   5
	-------------------------------------------------------
	-rw-r--r-- 1 0 0 100 services
	lrwxrwxrwx 1 0 0 20 resolv.conf -> /var/run/resolv.conf
	drwxr-sr-x 1 0 0 0 rc.d
	-rw-r--r-- 1 0 0 290 rc
	-rw-r--r-- 1 0 0 34 passwd
	lrwxrwxrwx 1 0 0 18 inittab -> ../var/tmp/inittab

// Windows Server 2008 R2
*LIST_DOS_5
	0          1       2     3
	-------------------------------------------------------
	02-05-2013 09:45AM <DIR> TEST
	01-28-2013 03:54PM 2847 DATA.TXT

*LIST_TANDEM
	 0             1               2    3         4        5       6
	---------------------------------------------------------------
	File         Code             EOF  Last Modification    Owner  RWEP
	EMSACSTM      101             146  18-Sep-00 09:03:37 170,175 "nunu"
	TACLCSTM   O  101             101  4-Mar-01  23:50:06 255,255 "oooo"

------------------------------------*/

/*===== 接続ウインドウの形式 =====*/

#define DLG_TYPE_CON	0		/* 簡易（ホスト編集なし） */
#define DLG_TYPE_SET	1		/* ホスト編集あり */

/*===== ファイル一覧取得モード =====*/

#define CACHE_NORMAL	0		/* 通常（キャッシュにあれば使用、なければ読み込み） */
#define CACHE_REFRESH	1		/* 常に読み込み */
#define CACHE_LASTREAD	2		/* 最後に読み込んだものを使用 */

/*===== 漢字コード変換 =====*/

#define KANJI_SJIS		0		/* SJIS */
#define KANJI_JIS		1		/* JIS */
#define KANJI_EUC		2		/* EUC */
#define KANJI_SMB_HEX	3		/* Samba-HEX */
#define KANJI_SMB_CAP	4		/* Samba-CAP */
#define KANJI_UTF8N		5		/* UTF-8 */
// UTF-8対応
#define KANJI_UTF8BOM		6		/* UTF-8 BOM */
// UTF-8 HFS+対応
#define KANJI_UTF8HFSX		7		/* UTF-8 HFS+ */

#define KANJI_NOCNV		-1		/* 漢字コード変換なし */

// UTF-8対応
#define KANJI_AUTO		-1

/*===== サウンド =====*/

#define SND_CONNECT		0		/* 接続時のサウンド */
#define SND_TRANS		1		/* 転送終了時のサウンド */
#define SND_ERROR		2		/* エラー時のサウンド */

#define SOUND_TYPES		3		/* サウンドの種類 */

/*===== ビューワ =====*/

#define VIEWERS			3		/* ビューワの数 */

/*===== ブックマーク =====*/

#define BMARK_SUB_MENU		2		/* ブックマークメニューのサブメニュー番号 */
#define DEFAULT_BMARK_ITEM	5		/* ブックマークメニューにある固定部分の数 */
#define MENU_BMARK_TOP		30000	/* 3000以降(3100くらいまで)は予約する */
									/* resource.h の定義と重ならないように */

#define BMARK_TYPE_NONE		0		/* ブックマーク無し */
#define BMARK_TYPE_LOCAL	1		/* ローカル側のブックマーク */
#define BMARK_TYPE_REMOTE	2		/* ホスト側のブックマーク */
#define BMARK_TYPE_BOTH		3		/* 両方のブックマーク */

#define BMARK_MARK_LOCAL	"L "	/* ローカル側の印 */
#define BMARK_MARK_REMOTE	"H "	/* ホスト側の印 */
#define BMARK_MARK_BOTH		"W "	/* 両方の印 */
#define BMARK_MARK_LEN		2		/* 印の文字数 */

#define BMARK_SEP			" <> "	/* ローカル側とホスト側の区切り */
#define BMARK_SEP_LEN		4		/* 区切りの文字数 */

/*===== レジストリのタイプ =====*/

#define REGTYPE_REG		0		/* レジストリ */
#define REGTYPE_INI		1		/* INIファイル */

// UTF-8対応
//#define REG_SECT_MAX	(16*1024)	/* レジストリの１セクションの最大データサイズ */
#define REG_SECT_MAX	(64*1024)	/* レジストリの１セクションの最大データサイズ */

/*===== ホスト設定で明示的に指定するホストのタイプ =====*/

#define	HTYPE_AUTO		0		/* 自動 */
#define	HTYPE_ACOS		1		/* ACOS (待機結合ファイルの指定が必要) */
#define	HTYPE_VMS		2		/* VAX VMS */
#define	HTYPE_IRMX		3		/* IRMX */
#define	HTYPE_ACOS_4	4		/* ACOS ファイル名を('')で括らない */
#define	HTYPE_STRATUS	5		/* Stratus */
#define	HTYPE_AGILENT	6		/* Agilent Logic analyzer */
#define	HTYPE_SHIBASOKU	7		/* Shibasoku LSI test system */
#if defined(HAVE_TANDEM)
#define HTYPE_TANDEM	8		/* HP NonStop Server */
#endif

/*===== コマンドラインオプション =====*/

//#define OPT_MIRROR		0x0001	/* ミラーリングアップロードを行う */
//#define OPT_FORCE		0x0002	/* ミラーリング開始の確認をしない */
//#define OPT_QUIT		0x0004	/* 終了後プログラム終了 */
//#define OPT_EUC			0x0008	/* 漢字コードはEUC */
//#define OPT_JIS			0x0010	/* 漢字コードはJIS */
//#define OPT_ASCII		0x0020	/* アスキー転送モード */
//#define OPT_BINARY		0x0040	/* バイナリ転送モード */
//#define OPT_AUTO		0x0080	/* 自動判別 */
//#define OPT_KANA		0x0100	/* 半角かなをそのまま通す */
//#define OPT_EUC_NAME	0x0200	/* ファイル名はEUC */
//#define OPT_JIS_NAME	0x0400	/* ファイル名はJIS */
//#define OPT_MIRRORDOWN	0x0800	/* ミラーリングダウンロードを行う */
//#define OPT_SAVEOFF		0x1000	/* 設定の保存を中止する */
//#define OPT_SAVEON		0x2000	/* 設定の保存を再開する */
#define OPT_MIRROR		0x00000001	/* ミラーリングアップロードを行う */
#define OPT_FORCE		0x00000002	/* ミラーリング開始の確認をしない */
#define OPT_QUIT		0x00000004	/* 終了後プログラム終了 */
#define OPT_EUC			0x00000008	/* 漢字コードはEUC */
#define OPT_JIS			0x00000010	/* 漢字コードはJIS */
#define OPT_ASCII		0x00000020	/* アスキー転送モード */
#define OPT_BINARY		0x00000040	/* バイナリ転送モード */
#define OPT_AUTO		0x00000080	/* 自動判別 */
#define OPT_KANA		0x00000100	/* 半角かなをそのまま通す */
#define OPT_EUC_NAME	0x00000200	/* ファイル名はEUC */
#define OPT_JIS_NAME	0x00000400	/* ファイル名はJIS */
#define OPT_MIRRORDOWN	0x00000800	/* ミラーリングダウンロードを行う */
#define OPT_SAVEOFF		0x00001000	/* 設定の保存を中止する */
#define OPT_SAVEON		0x00002000	/* 設定の保存を再開する */
// UTF-8対応
#define OPT_SJIS		0x00004000	/* 漢字コードはShift_JIS */
#define OPT_UTF8N		0x00008000	/* 漢字コードはUTF-8 */
#define OPT_UTF8BOM		0x00010000	/* 漢字コードはUTF-8 BOM */
#define OPT_SJIS_NAME	0x00020000	/* ファイル名はShift_JIS */
#define OPT_UTF8N_NAME	0x00040000	/* ファイル名はUTF-8 */

/*===== ホストのヒストリ =====*/

#define	HISTORY_MAX		20		/* ファイルのヒストリの最大個数 */
#define DEF_FMENU_ITEMS	8		/* Fileメニューにある項目数の初期値 */

/*===== SOCKS4 =====*/

#define SOCKS4_VER			4	/* SOCKSのバージョン */

#define SOCKS4_CMD_CONNECT	1	/* CONNECTコマンド */
#define SOCKS4_CMD_BIND		2	/* BINDコマンド */

/* リザルトコード */
#define SOCKS4_RES_OK		90	/* 要求は許可された */
	/* その他のコードはチェックしないので定義しない */

/*===== SOCKS5 =====*/

#define SOCKS5_VER			5	/* SOCKSのバージョン */

#define SOCKS5_CMD_CONNECT	1	/* CONNECTコマンド */
#define SOCKS5_CMD_BIND		2	/* BINDコマンド */

#define SOCKS5_AUTH_NONE	0	/* 認証無し */
#define SOCKS5_AUTH_GSSAPI	1	/* GSS-API */
#define SOCKS5_AUTH_USER	2	/* Username/Password */

#define SOCKS5_ADRS_IPV4	1	/* IP V4 address */
#define SOCKS5_ADRS_NAME	3	/* Domain name */
#define SOCKS5_ADRS_IPV6	4	/* IP V6 address */

#define SOCKS5_USERAUTH_VER	1	/* Username\Password認証のバージョン */

/* リザルトコード */
#define SOCKS5_RES_OK		0x00	/* succeeded */
	/* その他のコードはチェックしないので定義しない */

/*===== 中断コード =====*/

#define ABORT_NONE			0		/* 転送中断なし */
#define ABORT_USER			1		/* ユーザによる中断 */
#define ABORT_ERROR			2		/* エラーによる中断 */
#define ABORT_DISKFULL		3		/* ディスクフルよる中断 */

/*===== FireWallの種類 =====*/

#define FWALL_NONE			0		/* なし */
#define FWALL_FU_FP_SITE	1		/* FW user → FW pass → SITE host */
#define FWALL_FU_FP_USER	2		/* FW user → FW pass → USER user@host */
#define FWALL_USER			3		/* USER user@host */
#define FWALL_OPEN			4		/* OPEN host */
#define FWALL_SOCKS4		5		/* SOCKS4 */
#define FWALL_SOCKS5_NOAUTH	6		/* SOCKS5 (認証なし) */
#define FWALL_SOCKS5_USER	7		/* SOCKS5 (Username/Password認証) */
#define FWALL_FU_FP			8		/* FW user → FW pass */
#define FWALL_SIDEWINDER	9		/* USER FWuser:FWpass@host */

/*===== ワンタイムパスワード =====*/

/* コードの種類 */
#define SECURITY_DISABLE	0		/* 使用しない */
#define SECURITY_AUTO		1		/* 自動認識 */
#define MD4					2		/* MD4 */
#define MD5					3		/* MD5 */
#define SHA1				4		/* SHA-1 */

#define MAX_SEED_LEN		16		/* Seedの文字数 */

/*===== 再帰的なファイル検索の方法 =====*/

#define RDIR_NONE		0		/* 再帰検索なし */
#define RDIR_NLST		1		/* NLST -R */
#define RDIR_CWD		2		/* CWDで移動 */

/*===== VAX VMS 関係 =====*/

#define BLOCK_SIZE		512		/* 1ブロックのバイト数 */

/*===== 設定のレベル =====*/

#define SET_LEVEL_GROUP		0x8000		/* グループのフラグ */
#define SET_LEVEL_MASK		0x7FFF
#define SET_LEVEL_SAME		0x7FFF

/*===== 日付、数値の表示形式 =====*/

#define DISPFORM_LEGACY		0		/* 固定書式 */
#define DISPFORM_LOCALE		1		/* コントロールパネルに従う */

/*===== ファイル一覧に存在した情報 =====*/

#define FINFO_DATE			0x01	/* 日付 */
#define FINFO_TIME			0x02	/* 時間 */
#define FINFO_SIZE			0x04	/* サイズ */
#define FINFO_ATTR			0x08	/* 属性 */
#define FINFO_ALL			0xFF	/* 全て */

/*===== GetMasterPasswordStatusで使うコード =====*/
#define PASSWORD_OK 0
#define PASSWORD_UNMATCH 2
#define BAD_PASSWORD_HASH 3

/*===== 暗号化パスワード設定 =====*/
#define DEFAULT_PASSWORD	"DefaultPassword"
#define MAX_PASSWORD_LEN	128

// 暗号化通信対応
#define CRYPT_NONE			0
#define CRYPT_FTPES			1
#define CRYPT_FTPIS			2
#define CRYPT_SFTP			3

// FEAT対応
// UTF-8対応
#define FEATURE_UTF8		0x00000001
// MLSD対応
#define FEATURE_MLSD		0x00000002
// IPv6対応
#define FEATURE_EPRT		0x00000004
#define FEATURE_EPSV		0x00000008
// ホスト側の日時取得
#define FEATURE_MDTM		0x00000010
// ホスト側の日時設定
#define FEATURE_MFMT		0x00000020

// IPv6対応
#define NTYPE_AUTO			0		/* 自動 */
#define NTYPE_IPV4			1		/* TCP/IPv4 */
#define NTYPE_IPV6			2		/* TCP/IPv6 */

// 暗号化通信対応
// REG_SECT_MAXの値を加味する必要がある
#define MAX_CERT_CACHE_HASH 256


/*=================================================
*		ストラクチャ
*=================================================*/

/*===== ホスト設定データ =====*/

typedef struct {
	int Level;							/* 設定のレベル */
										/* 通常はグループのフラグのみが有効 */
										/* レベル数は設定の登録／呼出時のみで使用 */
	char HostName[HOST_NAME_LEN+1];		/* 設定名 */
	char HostAdrs[HOST_ADRS_LEN+1];		/* ホスト名 */
	char UserName[USER_NAME_LEN+1];		/* ユーザ名 */
	char PassWord[PASSWORD_LEN+1];		/* パスワード */
	char Account[ACCOUNT_LEN+1];		/* アカウント */
	char LocalInitDir[INIT_DIR_LEN+1];	/* ローカルの開始ディレクトリ */
	char RemoteInitDir[INIT_DIR_LEN+1];	/* ホストの開始ディレクトリ */
	char BookMark[BOOKMARK_SIZE];		/* ブックマーク */
	char ChmodCmd[CHMOD_CMD_LEN+1];		/* 属性変更コマンド */
	char LsName[NLST_NAME_LEN+1];		/* NLSTに付けるファイル名/オプション*/
	char InitCmd[INITCMD_LEN+1];		/* ホストの初期化コマンド */
	int Port;							/* ポート番号 */
	int Anonymous;						/* Anonymousフラグ */
	int KanjiCode;						/* ホストの漢字コード (KANJI_xxx) */
	int KanaCnv;						/* 半角カナを全角に変換(YES/NO) */
	int NameKanjiCode;					/* ファイル名の漢字コード (KANJI_xxx) */
	// UTF-8対応
	int CurNameKanjiCode;				/* 自動判別後のファイル名の漢字コード (KANJI_xxx) */
	int NameKanaCnv;					/* ファイル名の半角カナを全角に変換(YES/NO) */
	int Pasv;							/* PASVモード (YES/NO) */
	int FireWall;						/* FireWallを使う (YES/NO) */
	int ListCmdOnly;					/* "LIST"コマンドのみ使用する */
	int UseNLST_R;						/* "NLST -R"コマンドを使用する */
	int LastDir;						/* 最後にアクセスしたフォルダを保存 */
	int TimeZone;						/* タイムゾーン (-12～12) */
	int HostType;						/* ホストのタイプ (HTYPE_xxx) */
	int SyncMove;						/* フォルダ同時移動 (YES/NO) */
	int NoFullPath;						/* フルパスでファイルアクセスしない (YES/NO) */
	ulong Sort;							/* ソート方法 (0x11223344 : 11=LFsort 22=LDsort 33=RFsort 44=RFsort) */
	int Security;						/* セキュリティ (SECURITY_xxx , MDx) */
	int Dialup;							/* ダイアルアップ接続するかどうか (YES/NO) */
	int DialupAlways;					/* 常にこのエントリへ接続するかどうか (YES/NO) */
	int DialupNotify;					/* 再接続の際に確認する (YES/NO) */
	char DialEntry[RAS_NAME_LEN+1];		/* ダイアルアップエントリ */
	// 暗号化通信対応
	int CryptMode;						/* 暗号化通信モード (CRYPT_xxx) */
	int UseNoEncryption;				/* 暗号化なしで接続する (YES/NO) */
	int UseFTPES;						/* FTPESで接続する (YES/NO) */
	int UseFTPIS;						/* FTPISで接続する (YES/NO) */
	int UseSFTP;						/* SFTPで接続する (YES/NO) */
	char PrivateKey[PRIVATE_KEY_LEN+1];	/* テキスト形式の秘密鍵 */
	int NoWeakEncryption;				/* 弱い暗号を拒否 (YES/NO) */
	// 同時接続対応
	int MaxThreadCount;					/* 同時接続数 */
	int ReuseCmdSkt;					/* メインウィンドウのソケットを再利用する (YES/NO) */
	int NoDisplayUI;					/* UIを表示しない (YES/NO) */
	// FEAT対応
	int Feature;						/* 利用可能な機能のフラグ (FEATURE_xxx) */
	// MLSD対応
	int UseMLSD;						/* "MLSD"コマンドを使用する */
	// IPv6対応
	int NetType;						/* ネットワークの種類 (NTYPE_xxx) */
	int CurNetType;						/* 接続中のネットワークの種類 (NTYPE_xxx) */
	// 自動切断対策
	int NoopInterval;					/* 無意味なコマンドを送信する間隔（秒数、0で無効）*/
	// 再転送対応
	int TransferErrorMode;				/* 転送エラー時の処理 (EXIST_xxx) */
	int TransferErrorNotify;			/* 転送エラー時に確認ダイアログを出すかどうか (YES/NO) */
	// セッションあたりの転送量制限対策
	int TransferErrorReconnect;			/* 転送エラー時に再接続する (YES/NO) */
} HOSTDATA;


/*===== ホスト設定リスト =====*/

typedef struct hostlistdata {
	HOSTDATA Set;					/* ホスト設定データ */
	struct hostlistdata *Next;
	struct hostlistdata *Prev;
	struct hostlistdata *Child;
	struct hostlistdata *Parent;
} HOSTLISTDATA;


/*===== 接続ヒストリリスト =====*/

typedef struct historydata {
	char HostAdrs[HOST_ADRS_LEN+1];		/* ホスト名 */
	char UserName[USER_NAME_LEN+1];		/* ユーザ名 */
	char PassWord[PASSWORD_LEN+1];		/* パスワード */
	char Account[ACCOUNT_LEN+1];		/* アカウント */
	char LocalInitDir[INIT_DIR_LEN+1];	/* ディレクトリ */
	char RemoteInitDir[INIT_DIR_LEN+1];	/* ディレクトリ */
	char ChmodCmd[CHMOD_CMD_LEN+1];		/* 属性変更コマンド */
	char LsName[NLST_NAME_LEN+1];		/* NLSTに付けるファイル名/オプション*/
	char InitCmd[INITCMD_LEN+1];		/* ホストの初期化コマンド */
	int Port;							/* ポート番号 */
	int KanjiCode;						/* ホストの漢字コード (KANJI_xxx) */
	int KanaCnv;						/* 半角カナを全角に変換(YES/NO) */
	int NameKanjiCode;					/* ファイル名の漢字コード (KANJI_xxx) */
	int NameKanaCnv;					/* ファイル名の半角カナを全角に変換(YES/NO) */
	int Pasv;							/* PASVモード (YES/NO) */
	int FireWall;						/* FireWallを使う (YES/NO) */
	int ListCmdOnly;					/* "LIST"コマンドのみ使用する */
	int UseNLST_R;						/* "NLST -R"コマンドを使用する */
	int TimeZone;						/* タイムゾーン (-12～12) */
	int HostType;						/* ホストのタイプ (HTYPE_xxx) */
	int SyncMove;						/* フォルダ同時移動 (YES/NO) */
	int NoFullPath;						/* フルパスでファイルアクセスしない (YES/NO) */
	ulong Sort;							/* ソート方法 (0x11223344 : 11=LFsort 22=LDsort 33=RFsort 44=RFsort) */
	int Security;						/* セキュリティ (OTP_xxx , MDx) */
	int Type;							/* 転送方法 (TYPE_xx) */
	int Dialup;							/* ダイアルアップ接続するかどうか (YES/NO) */
	int DialupAlways;					/* 常にこのエントリへ接続するかどうか (YES/NO) */
	int DialupNotify;					/* 再接続の際に確認する (YES/NO) */
	char DialEntry[RAS_NAME_LEN+1];		/* ダイアルアップエントリ */
	// 暗号化通信対応
	int UseNoEncryption;				/* 暗号化なしで接続する (YES/NO) */
	int UseFTPES;						/* FTPESで接続する (YES/NO) */
	int UseFTPIS;						/* FTPISで接続する (YES/NO) */
	int UseSFTP;						/* SFTPで接続する (YES/NO) */
	char PrivateKey[PRIVATE_KEY_LEN+1];	/* テキスト形式の秘密鍵 */
	int NoWeakEncryption;				/* 弱い暗号を拒否 (YES/NO) */
	// 同時接続対応
	int MaxThreadCount;					/* 同時接続数 */
	int ReuseCmdSkt;					/* メインウィンドウのソケットを再利用する (YES/NO) */
	// MLSD対応
	int UseMLSD;						/* "MLSD"コマンドを使用する */
	// IPv6対応
	int NetType;						/* ネットワークの種類 (NTYPE_xxx) */
	// 自動切断対策
	int NoopInterval;					/* NOOPコマンドを送信する間隔（秒数、0で無効）*/
	// 再転送対応
	int TransferErrorMode;				/* 転送エラー時の処理 (EXIST_xxx) */
	int TransferErrorNotify;			/* 転送エラー時に確認ダイアログを出すかどうか (YES/NO) */
	// セッションあたりの転送量制限対策
	int TransferErrorReconnect;			/* 転送エラー時に再接続する (YES/NO) */
	struct historydata *Next;
} HISTORYDATA;


/*===== 転送ファイルリスト =====*/

typedef struct transpacket {
	SOCKET ctrl_skt;				/* Socket */
	char Cmd[40];					/* STOR/RETR/MKD */
	char RemoteFile[FMAX_PATH+1];	/* ホスト側のファイル名（フルパス） */
									/* VMSの時は ddd[xxx.yyy]/yyy/zzz のように */
									/* なってるので注意 */
	char LocalFile[FMAX_PATH+1];	/* ローカル側のファイル名（フルパス） */
	int Type;						/* 転送方法 (TYPE_xx) */
	LONGLONG Size;					/* ファイルのサイズ */
	LONGLONG ExistSize;				/* すでに存在するファイルのサイズ */
									/* 転送中は、転送したファイルのサイズを格納する */
	FILETIME Time;					/* ファイルの時間(UTC) */
	int Attr;						/* ファイルの属性 */
	int KanjiCode;					/* 漢字コード (KANJI_xxx) */
	// UTF-8対応
	int KanjiCodeDesired;			/* ローカルの漢字コード (KANJI_xxx) */
	int KanaCnv;					/* 半角カナを全角に変換(YES/NO) */
	int Mode;						/* 転送モード (EXIST_xxx) */
#if defined(HAVE_TANDEM)
	int FileCode;					/* ファイルコード */
	int PriExt;						/* Primary Extents */
	int SecExt;						/* Secondary Extents */
	int MaxExt;						/* Max Extents */
#endif
	HWND hWndTrans;					/* 転送中ダイアログのウインドウハンドル */
	int Abort;						/* 転送中止フラグ (ABORT_xxx) */
	// ミラーリング設定追加
	int NoTransfer;
	// 同時接続対応
	int ThreadCount;
	struct transpacket *Next;
} TRANSPACKET;


/*===== ファイルリスト =====*/

typedef struct filelist {
	char File[FMAX_PATH+1];			/* ファイル名 */
	char Node;						/* 種類 (NODE_xxx) */
	char Link;						/* リンクファイルかどうか (YES/NO) */
	LONGLONG Size;					/* ファイルサイズ */
	int Attr;						/* 属性 */
	FILETIME Time;					/* 時間(UTC) */
	char Owner[OWNER_NAME_LEN+1];	/* オーナ名 */
	char InfoExist;					/* ファイル一覧に存在した情報のフラグ (FINFO_xxx) */
	// ファイルアイコン表示対応
	int ImageId;					/* アイコン画像番号 */
	struct filelist *Next;
} FILELIST;


/*===== コード変換情報パケット =====*/

typedef char * (*funcptr)(struct codeconvinfo *, char , char *);
// UTF-8対応
typedef int (*convptr)(struct codeconvinfo *);

typedef struct codeconvinfo {
	char *Str;			/* 文字列 */
	int StrLen;			/* 文字列の長さ */
	int KanaCnv;		/* 半角カタカナを全角に変換するかどうか (YES/NO) */
	char *Buf;			/* 変換後の文字列を格納するバッファ */
	int BufSize;		/* 変換後の文字列を格納するバッファのサイズ */
	int OutLen;			/* 変換後の文字列のサイズ */
	int KanjiMode;		/* 漢字モードフラグ(YES/NO) (内部処理用ワーク) */
	int EscProc;		/* エスケープシーケンス文字数 (0～) (内部処理用ワーク) */
	char EscCode[2];	/* エスケープシーケンス文字保存用 (内部処理用ワーク) */
	char KanjiFst;		/* 漢字コード１バイト目保存用 (内部処理用ワーク) */
	char KanaPrev;		/* 半角カタカナ保存用 (内部処理用ワーク) */
	funcptr KanaProc;	/* 半角カタカナ処理ルーチン (内部処理用ワーク) */
	// UTF-8対応
	char EscUTF8[16];	/* エスケープシーケンス文字数 (0～) (内部処理用ワーク) */
	int EscUTF8Len;		/* エスケープシーケンス文字保存用 (内部処理用ワーク) */
	int EscFlush;		/* 残り情報を出力 (YES/NO) */
	convptr FlushProc;	/* 残り情報処理ルーチン (内部処理用ワーク) */
} CODECONVINFO;


/*===== 改行コード変換情報パケット =====*/

typedef struct termcodeconvinfo {
	char *Str;			/* 文字列 */
	int StrLen;			/* 文字列の長さ */
	char *Buf;			/* 変換後の文字列を格納するバッファ */
	int BufSize;		/* 変換後の文字列を格納するバッファのサイズ */
	int OutLen;			/* 変換後の文字列のサイズ */
	char Term;			/* 改行コード１バイト目保存用 (内部処理用ワーク) */
} TERMCODECONVINFO;


/*===== テンポラリファイルリスト =====*/

typedef struct tempfilelist {
	char *Fname;				/* ファイル名 */
	struct tempfilelist *Next;
} TEMPFILELIST;


/*===== サウンドファイル =====*/

typedef struct {
	int On;						/* ON/OFFスイッチ */
	char Fname[FMAX_PATH+1];		/* ファイル名 */
} SOUNDFILE;


/*===== ラジオボタンの設定 =====*/

typedef struct {
	int ButID;			/* ボタンのID */
	int Value;			/* 値 */
} RADIOBUTTON;


/*===== SOCKS4 =====*/

/* コマンドパケット */
typedef struct {
	char Ver;						/* バージョン (SOCKS4_VER) */
	char Cmd;						/* コマンド (SOCKS4_CMD_xxx) */
	ushort Port;					/* ポート */
	ulong AdrsInt;					/* アドレス */
	char UserID[USER_NAME_LEN+1];	/* ユーザID */
} SOCKS4CMD;


/* 返信パケット */
typedef struct {
	char Ver;				/* バージョン */
	char Result;			/* リザルトコード (SOCKS4_RES_xxx) */
	ushort Port;			/* ポート */
	ulong AdrsInt;			/* アドレス */
} SOCKS4REPLY;

#define SOCKS4REPLY_SIZE	8


/*===== SOCKS5 =====*/

/* Method requestパケット */
typedef struct {
	char Ver;				/* バージョン (SOCKS5_VER) */
	char Num;				/* メソッドの数 */
	uchar Methods[1];		/* メソッド */
} SOCKS5METHODREQUEST;

#define SOCKS5METHODREQUEST_SIZE	3


/* Method replyパケット */
typedef struct {
	char Ver;				/* バージョン (SOCKS5_VER) */
	uchar Method;			/* メソッド */
} SOCKS5METHODREPLY;

#define SOCKS5METHODREPLY_SIZE	2


/* Requestパケット */
typedef struct {
	char Ver;				/* バージョン (SOCKS5_VER) */
	char Cmd;				/* コマンド (SOCKS5_CMD_xxx) */
	char Rsv;				/* （予約） */
	char Type;				/* アドレスのタイプ */
							/* 以後（可変長部分） */
	char _dummy[255+1+2];	/* アドレス、ポート */
} SOCKS5REQUEST;

#define SOCKS5REQUEST_SIZE 4	/* 最初の固定部分のサイズ */


/* Replyパケット */
typedef struct {
	char Ver;				/* バージョン */
	char Result;			/* リザルトコード (SOCKS4_RES_xxx) */
	char Rsv;				/* （予約） */
	char Type;				/* アドレスのタイプ */
							/* 以後（可変長部分） */
	// IPv6対応
//	ulong AdrsInt;			/* アドレス */
//	ushort Port;			/* ポート */
//	char _dummy[2];			/* dummy */
	char _dummy[255+1+2];	/* dummy */
} SOCKS5REPLY;

#define SOCKS5REPLY_SIZE 4	/* 最初の固定部分のサイズ */


/* Username/Password認証statusパケット */
typedef struct {
	char Ver;				/* バージョン */
	uchar Status;			/* ステータス */
} SOCKS5USERPASSSTATUS;

#define SOCKS5USERPASSSTATUS_SIZE	2



/*===== ダイアログボックス変更処理用 =====*/

typedef struct {
	// ホスト共通設定機能
//	int HorMoveList[10];	/* 水平に動かす部品のリスト */
//	int VarMoveList[10];	/* 垂直に動かす部品のリスト */
//	int ResizeList[10];		/* サイズ変更する部品のリスト */
	int HorMoveList[16];	/* 水平に動かす部品のリスト */
	int VarMoveList[16];	/* 垂直に動かす部品のリスト */
	int ResizeList[16];		/* サイズ変更する部品のリスト */
	SIZE MinSize;			/* 最少サイズ */
	SIZE CurSize;			/* 現在のサイズ */
} DIALOGSIZE;


/*===== 数値変換用 =====*/

typedef struct {
	int Num1;
	int Num2;
} INTCONVTBL;


// UPnP対応
typedef struct
{
	int r;
	HANDLE h;
	char* Adrs;
	int Port;
	char* ExtAdrs;
} ADDPORTMAPPINGDATA;

typedef struct
{
	int r;
	HANDLE h;
	int Port;
} REMOVEPORTMAPPINGDATA;

/*=================================================
*		プロトタイプ
*=================================================*/

/*===== main.c =====*/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow);
void DispWindowTitle(void);
HWND GetMainHwnd(void);
HWND GetFocusHwnd(void);
void SetFocusHwnd(HWND hWnd);
HINSTANCE GetFtpInst(void);
void DoubleClickProc(int Win, int Mode, int App);
void ExecViewer(char *Fname, int App);
void ExecViewer2(char *Fname1, char *Fname2, int App);
void AddTempFileList(char *Fname);
void SoundPlay(int Num);
char *AskHelpFilePath(void);
char *AskTmpFilePath(void);
char *AskIniFilePath(void);
int AskForceIni(void);
int BackgrndMessageProc(void);
void ResetAutoExitFlg(void);
int AskAutoExit(void);
// 暗号化通信対応
BOOL __stdcall SSLTimeoutCallback(BOOL* pbAborted);
BOOL __stdcall SSLConfirmCallback(BOOL* pbAborted, BOOL bVerified, LPCSTR Certificate, LPCSTR CommonName);
BOOL LoadSSLRootCAFile();
// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
BOOL IsMainThread();
// ポータブル版判定
void CheckPortableVersion();
int AskPortableVersion(void);
// 全設定暗号化対応
int Restart();
void Terminate();
// タスクバー進捗表示
int LoadTaskbarList3();
void FreeTaskbarList3();
int IsTaskbarList3Loaded();
void UpdateTaskbarProgress();
// 高DPI対応
int AskToolWinHeight(void);

/*===== filelist.c =====*/

int MakeListWin(HWND hWnd, HINSTANCE hInst);
void DeleteListWin(void);
HWND GetLocalHwnd(void);
HWND GetRemoteHwnd(void);
void GetListTabWidth(void);
void SetListViewType(void);
void GetRemoteDirForWnd(int Mode, int *CancelCheckWork);
void GetLocalDirForWnd(void);
void ReSortDispList(int Win, int *CancelCheckWork);
// ローカル側自動更新
//void SelectFileInList(HWND hWnd, int Type);
void SelectFileInList(HWND hWnd, int Type, FILELIST *Base);
void FindFileInList(HWND hWnd, int Type);
// void WildCard2RegExp(char *Str);
int GetCurrentItem(int Win);
int GetItemCount(int Win);
int GetSelectedCount(int Win);
int GetFirstSelected(int Win, int All);
int GetNextSelected(int Win, int Pos, int All);
// ローカル側自動更新
int GetHotSelected(int Win, char *Fname);
int SetHotSelected(int Win, char *Fname);
int FindNameNode(int Win, char *Name);
void GetNodeName(int Win, int Pos, char *Buf, int Max);
int GetNodeTime(int Win, int Pos, FILETIME *Buf);
int GetNodeSize(int Win, int Pos, LONGLONG *Buf);
int GetNodeAttr(int Win, int Pos, int *Buf);
int GetNodeType(int Win, int Pos);
void GetNodeOwner(int Win, int Pos, char *Buf, int Max);
void EraseRemoteDirForWnd(void);
double GetSelectedTotalSize(int Win);
// ファイル一覧バグ修正
//void MakeSelectedFileList(int Win, int Expand, int All, FILELIST **Base, int *CancelCheckWork);
int MakeSelectedFileList(int Win, int Expand, int All, FILELIST **Base, int *CancelCheckWork);
void MakeDroppedFileList(WPARAM wParam, char *Cur, FILELIST **Base);
void MakeDroppedDir(WPARAM wParam, char *Cur);
void AddRemoteTreeToFileList(int Num, char *Path, int IncDir, FILELIST **Base);
void DeleteFileList(FILELIST **Base);
FILELIST *SearchFileList(char *Fname, FILELIST *Base, int Caps);
int Assume1900or2000(int Year);
void SetFilter(int *CancelCheckWork);
void doDeleteRemoteFile(void);
// UTF-8対応
int AnalyzeNameKanjiCode(int Num);


/*===== toolmenu.c =====*/

int MakeToolBarWindow(HWND hWnd, HINSTANCE hInst);
void DeleteToolBarWindow(void);
HWND GetMainTbarWnd(void);
HWND GetLocalHistHwnd(void);
HWND GetRemoteHistHwnd(void);
HWND GetLocalHistEditHwnd(void);
HWND GetRemoteHistEditHwnd(void);
HWND GetLocalTbarWnd(void);
HWND GetRemoteTbarWnd(void);
int GetHideUI(void);
void MakeButtonsFocus(void);
void DisableUserOpe(void);
void EnableUserOpe(void);
int AskUserOpeDisabled(void);
void SetTransferTypeImm(int Mode);
void SetTransferType(int Type);
void DispTransferType(void);
int AskTransferType(void);
int AskTransferTypeAssoc(char *Fname, int Type);
void SaveTransferType(void);
void SetHostKanjiCodeImm(int Mode);
void SetHostKanjiCode(int Type);
void DispHostKanjiCode(void);
int AskHostKanjiCode(void);
void HideHostKanjiButton(void);
// UTF-8対応
void SetLocalKanjiCodeImm(int Mode);
void SetLocalKanjiCode(int Type);
void DispLocalKanjiCode(void);
int AskLocalKanjiCode(void);
void HideLocalKanjiButton(void);
void SaveLocalKanjiCode(void);
void SetHostKanaCnvImm(int Mode);
void SetHostKanaCnv(void);
void DispHostKanaCnv(void);
int AskHostKanaCnv(void);
void SetSortTypeImm(int LFsort, int LDsort, int RFsort, int RDsort);
void SetSortTypeByColumn(int Win, int Tab);
int AskSortType(int Name);
void SetSaveSortToHost(int Sw);
int AskSaveSortToHost(void);
void DispListType(void);
void SetSyncMoveMode(int Mode);
void ToggleSyncMoveMode(void);
void DispSyncMoveMode(void);
int AskSyncMoveMode(void);
void SetRemoteDirHist(char *Path);
void SetLocalDirHist(char *Path);
void AskLocalCurDir(char *Buf, int Max);
void AskRemoteCurDir(char *Buf, int Max);
void SetCurrentDirAsDirHist(void);
void DispDotFileMode(void);
void LocalRbuttonMenu(int Pos);
void RemoteRbuttonMenu(int Pos);

/*===== statuswin.c =====*/

int MakeStatusBarWindow(HWND hWnd, HINSTANCE hInst);
void DeleteStatusBarWindow(void);
HWND GetSbarWnd(void);
void DispCurrentWindow(int Win);
void DispSelectedSpace(void);
void DispLocalFreeSpace(char *Path);
void DispTransferFiles(void);
void DispDownloadSize(LONGLONG Size);

/*===== taskwin.c =====*/

int MakeTaskWindow(HWND hWnd, HINSTANCE hInst);
void DeleteTaskWindow(void);
HWND GetTaskWnd(void);
void _SetTaskMsg(const char* format, ...);
#ifdef _DEBUG
#define SetTaskMsg(...) do { char buffer[10240 + 3]; sprintf(buffer, __VA_ARGS__); _SetTaskMsg(buffer); } while(0)
#else
#define SetTaskMsg(...) _SetTaskMsg(__VA_ARGS__)
#endif
int SaveTaskMsg(char *Fname);
void DispTaskMsg(void);
void _DoPrintf(const char* format, ...);
#ifdef _DEBUG
#define DoPrintf(...) do { char buffer[10240]; sprintf(buffer, __VA_ARGS__); _DoPrintf(buffer); } while(0)
#else
#define DoPrintf(...) _DoPrintf(__VA_ARGS__)
#endif

/*===== hostman.c =====*/

int SelectHost(int Type);
int AddHostToList(HOSTDATA *Set, int Pos, int Level);
int CopyHostFromList(int Num, HOSTDATA *Set);
int CopyHostFromListInConnect(int Num, HOSTDATA *Set);
int SetHostBookMark(int Num, char *Bmask, int Len);
char *AskHostBookMark(int Num);
int SetHostDir(int Num, char *LocDir, char *HostDir);
int SetHostPassword(int Num, char *Pass);
int SetHostSort(int Num, int LFSort, int LDSort, int RFSort, int RDSort);
void DecomposeSortType(ulong Sort, int *LFSort, int *LDSort, int *RFSort, int *RDSort);
int AskCurrentHost(void);
void SetCurrentHost(int Num);
void CopyDefaultHost(HOSTDATA *Set);
// ホスト共通設定機能
void ResetDefaultHost(void);
void SetDefaultHost(HOSTDATA *Set);
void CopyDefaultDefaultHost(HOSTDATA *Set);
int SearchHostName(char *Name);
void ImportFromWSFTP(void);
// 暗号化通信対応
int SetHostEncryption(int Num, int UseNoEncryption, int UseFTPES, int UseFTPIS, int UseSFTP);

/*===== connect.c =====*/

void ConnectProc(int Type, int Num);
void QuickConnectProc(void);
void DirectConnectProc(char *unc, int Kanji, int Kana, int Fkanji, int TrMode);
void HistoryConnectProc(int MenuCmd);
char *AskHostAdrs(void);
int AskHostPort(void);
int AskHostNameKanji(void);
int AskHostNameKana(void);
int AskListCmdMode(void);
int AskUseNLST_R(void);
char *AskHostChmodCmd(void);
int AskHostTimeZone(void);
int AskPasvMode(void);
char *AskHostLsName(void);
int AskHostType(void);
int AskHostFireWall(void);
int AskNoFullPathMode(void);
char *AskHostUserName(void);
void SaveCurrentSetToHost(void);
int ReConnectCmdSkt(void);
// int ReConnectTrnSkt(void);
// 同時接続対応
int ReConnectTrnSkt(SOCKET *Skt, int *CancelCheckWork);
SOCKET AskCmdCtrlSkt(void);
SOCKET AskTrnCtrlSkt(void);
void SktShareProh(void);
int AskShareProh(void);
void DisconnectProc(void);
void DisconnectSet(void);
int AskConnecting(void);
#if defined(HAVE_TANDEM)
int AskRealHostType(void);
int SetOSS(int wkOss);
int AskOSS(void);
#endif
SOCKET connectsock(char *host, int port, char *PreMsg, int *CancelCheckWork);
// IPv6対応
SOCKET connectsockIPv4(char *host, int port, char *PreMsg, int *CancelCheckWork);
SOCKET connectsockIPv6(char *host, int port, char *PreMsg, int *CancelCheckWork);
SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork);
// IPv6対応
SOCKET GetFTPListenSocketIPv4(SOCKET ctrl_skt, int *CancelCheckWork);
SOCKET GetFTPListenSocketIPv6(SOCKET ctrl_skt, int *CancelCheckWork);
int AskTryingConnect(void);
// 同時接続対応
//int SocksGet2ndBindReply(SOCKET Socket, SOCKET *Data);
int SocksGet2ndBindReply(SOCKET Socket, SOCKET *Data, int *CancelCheckWork);
// 暗号化通信対応
int AskCryptMode(void);
int AskUseNoEncryption(void);
int AskUseFTPES(void);
int AskUseFTPIS(void);
int AskUseSFTP(void);
char *AskPrivateKey(void);
// 同時接続対応
int AskMaxThreadCount(void);
int AskReuseCmdSkt(void);
// FEAT対応
int AskHostFeature(void);
// MLSD対応
int AskUseMLSD(void);
// IPv6対応
int AskCurNetType(void);
// 自動切断対策
int AskNoopInterval(void);
// 再転送対応
int AskTransferErrorMode(void);
int AskTransferErrorNotify(void);
// セッションあたりの転送量制限対策
int AskErrorReconnect(void);

/*===== cache.c =====*/

int MakeCacheBuf(int Num);
void DeleteCacheBuf(void);
int AskCached(char *Path);
int AskFreeCache(void);
void SetCache(int Num, char *Path);
void ClearCache(int Num);
int AskCurrentFileListNum(void);
void SetCurrentFileListNum(int Num);
void SaveCache(void);
void LoadCache(void);
void DeleteCache(void);
void MakeCacheFileName(int Num, char *Buf);
void CountPrevFfftpWindows(void);

/*===== ftpproc.c =====*/

void DownloadProc(int ChName, int ForceFile, int All);
void DirectDownloadProc(char *Fname);
void InputDownloadProc(void);
void MirrorDownloadProc(int Notify);
void UploadListProc(int ChName, int All);
void UploadDragProc(WPARAM wParam);
void MirrorUploadProc(int Notify);
void DeleteProc(void);
void RenameProc(void);
void MoveRemoteFileProc(int);
void MkdirProc(void);
void ChangeDirComboProc(HWND hWnd);
void ChangeDirBmarkProc(int MarkID);
void ChangeDirDirectProc(int Win);
void ChangeDirDropFileProc(WPARAM wParam);
void ChmodProc(void);
// 64ビット対応
//BOOL CALLBACK ChmodDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChmodDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
void SomeCmdProc(void);
void CalcFileSizeProc(void);
void DispCWDerror(HWND hWnd);
void CopyURLtoClipBoard(void);
// 同時接続対応
//int ProcForNonFullpath(char *Path, char *CurDir, HWND hWnd, int Type);
int ProcForNonFullpath(SOCKET cSkt, char *Path, char *CurDir, HWND hWnd, int *CancelCheckWork);
void ReformToVMSstyleDirName(char *Path);
void ReformToVMSstylePathName(char *Path);
#if defined(HAVE_OPENVMS)
void ReformVMSDirName(char *DirName, int Flg);
#endif
// 自動切断対策
void NoopProc(int Force);
// 同時接続対応
void AbortRecoveryProc(void);
void ReconnectProc(void);

/*===== local.c =====*/

int DoLocalCWD(char *Path);
void DoLocalMKD(char *Path);
void DoLocalPWD(char *Buf);
void DoLocalRMD(char *Path);
void DoLocalDELE(char *Path);
void DoLocalRENAME(char *Src, char *Dst);
void DispFileProperty(char *Fname);
HANDLE FindFirstFileAttr(char *Fname, WIN32_FIND_DATA *FindData, int IgnHide);
BOOL FindNextFileAttr(HANDLE hFind, WIN32_FIND_DATA *FindData, int IgnHide);

/*===== remote.c =====*/

int DoCWD(char *Path, int Disp, int ForceGet, int ErrorBell);
int DoCWDStepByStep(char *Path, char *Cur);
int DoMKD(char *Path);
void InitPWDcommand();
int DoRMD(char *Path);
int DoDELE(char *Path);
int DoRENAME(char *Src, char *Dst);
int DoCHMOD(char *Path, char *Mode);
// 同時接続対応
//int DoSIZE(char *Path, LONGLONG *Size);
int DoSIZE(SOCKET cSkt, char *Path, LONGLONG *Size, int *CancelCheckWork);
// 同時接続対応
//int DoMDTM(char *Path, FILETIME *Time);
int DoMDTM(SOCKET cSkt, char *Path, FILETIME *Time, int *CancelCheckWork);
// ホスト側の日時設定
int DoMFMT(SOCKET cSkt, char *Path, FILETIME *Time, int *CancelCheckWork);
// 同時接続対応
//int DoQUOTE(char *CmdStr);
int DoQUOTE(SOCKET cSkt, char *CmdStr, int *CancelCheckWork);
SOCKET DoClose(SOCKET Sock);
// 同時接続対応
//int DoQUIT(SOCKET ctrl_skt);
int DoQUIT(SOCKET ctrl_skt, int *CancelCheckWork);
int DoDirListCmdSkt(char *AddOpt, char *Path, int Num, int *CancelCheckWork);
#if defined(HAVE_TANDEM)
void SwitchOSSProc(void);
#endif
#define CommandProcTrn(CSKT, REPLY, CANCELCHECKWORK, ...) (command(CSKT, REPLY, CANCELCHECKWORK, __VA_ARGS__))
int _command(SOCKET cSkt, char* Reply, int* CancelCheckWork, const char* fmt, ...);
#define command(CSKT, REPLY, CANCELCHECKWORK, ...) (_scprintf(__VA_ARGS__), _command(CSKT, REPLY, CANCELCHECKWORK, __VA_ARGS__))
int SendData(SOCKET Skt, char *Data, int Size, int Mode, int *CancelCheckWork);
int ReadReplyMessage(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork, char *Tmp);
int ReadNchar(SOCKET cSkt, char *Buf, int Size, int *CancelCheckWork);
char *ReturnWSError(UINT Error);
void ReportWSError(char *Msg, UINT Error);
int ChangeFnameRemote2Local(char *Fname, int Max);
int ChangeFnameLocal2Remote(char *Fname, int Max);

/*===== getput.c =====*/

int MakeTransferThread(void);
void CloseTransferThread(void);
// 同時接続対応
void AbortAllTransfer();
int AddTmpTransFileList(TRANSPACKET *Pkt, TRANSPACKET **Base);
void EraseTmpTransFileList(TRANSPACKET **Base);
int RemoveTmpTransFileListItem(TRANSPACKET **Base, int Num);

void AddTransFileList(TRANSPACKET *Pkt);
// バグ対策
void AddNullTransFileList();
void AppendTransFileList(TRANSPACKET *Pkt);
void KeepTransferDialog(int Sw);
int AskTransferNow(void);
int AskTransferFileNum(void);
void GoForwardTransWindow(void);
void InitTransCurDir(void);
int DoDownload(SOCKET cSkt, TRANSPACKET *Pkt, int DirList, int *CancelCheckWork);
int CheckPathViolation(TRANSPACKET *packet);
// タスクバー進捗表示
LONGLONG AskTransferSizeLeft(void);
LONGLONG AskTransferSizeTotal(void);
int AskTransferErrorDisplay(void);

/*===== codecnv.c =====*/

void InitTermCodeConvInfo(TERMCODECONVINFO *cInfo);
int FlushRestTermCodeConvData(TERMCODECONVINFO *cInfo);
int ConvTermCodeToCRLF(TERMCODECONVINFO *cInfo);

void InitCodeConvInfo(CODECONVINFO *cInfo);
int FlushRestData(CODECONVINFO *cInfo);
// UTF-8対応
int ConvNoConv(CODECONVINFO *cInfo);
int ConvEUCtoSJIS(CODECONVINFO *cInfo);
int ConvJIStoSJIS(CODECONVINFO *cInfo);
int ConvSMBtoSJIS(CODECONVINFO *cInfo);
int ConvUTF8NtoSJIS(CODECONVINFO *cInfo); // UTF-8対応
int ConvSJIStoEUC(CODECONVINFO *cInfo);
int ConvSJIStoJIS(CODECONVINFO *cInfo);
int ConvSJIStoSMB_HEX(CODECONVINFO *cInfo);
int ConvSJIStoSMB_CAP(CODECONVINFO *cInfo);
int ConvSJIStoUTF8N(CODECONVINFO *cInfo); // UTF-8対応
// UTF-8 HFS+対応
int ConvUTF8NtoUTF8HFSX(CODECONVINFO *cInfo);
int ConvUTF8HFSXtoUTF8N(CODECONVINFO *cInfo);
void ConvAutoToSJIS(char *Text, int Pref);
int CheckKanjiCode(char *Text, int Size, int Pref);
// UTF-8対応
int LoadUnicodeNormalizationDll();
void FreeUnicodeNormalizationDll();
int IsUnicodeNormalizationDllLoaded();

/*===== option.c =====*/

void SetOption(int Start);
int SortSetting(void);
// hostman.cで使用
int GetDecimalText(HWND hDlg, int Ctrl);
void SetDecimalText(HWND hDlg, int Ctrl, int Num);
void CheckRange2(int *Cur, int Max, int Min);
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);

/*===== bookmark.c =====*/

void ClearBookMark(void);
void AddCurDirToBookMark(int Win);
int AskBookMarkText(int MarkID, char *Local, char *Remote, int Max);
void SaveBookMark(void);
void LoadBookMark(void);
int EditBookMark(void);

/*===== regexp.c =====*/

int LoadJre(void);
void ReleaseJre(void);
int AskRasUsable(void);
int AskJreUsable(void);
int GetJreVersion(void);
int JreCompileStr(char *Str);
char *JreGetStrMatchInfo(char *Str, UINT nStart);

/*===== wildcard.c =====*/

int CheckFname(char *str, char *regexp);

/*===== registry.c =====*/

void SaveRegistry(void);
int LoadRegistry(void);
void ClearRegistry(void);
// ポータブル版判定
void ClearIni(void);
void SetMasterPassword( const char* );
// セキュリティ強化
void GetMasterPassword(char*);
int GetMasterPasswordStatus(void);
int ValidateMasterPassword(void);
DWORD LoadHideDriveListRegistry(void);
void SaveSettingsToFile(void);
int LoadSettingsFromFile(void);
// ポータブル版判定
int IsRegAvailable();
int IsIniAvailable();
// バージョン確認
int ReadSettingsVersion();
// FileZilla XML形式エクスポート対応
void SaveSettingsToFileZillaXml();
// WinSCP INI形式エクスポート対応
void SaveSettingsToWinSCPIni();

/*===== lvtips.c =====*/

int InitListViewTips(HWND hWnd, HINSTANCE hInst);
void DeleteListViewTips(void);
void EraseListViewTips(void);
HWND GetListViewTipsHwnd(void);
void CheckTipsDisplay(HWND hWnd, LPARAM lParam);

/*===== ras.c =====*/

void LoadRasLib(void);
void ReleaseRasLib(void);
void DisconnectRas(int Notify);
int SetRasEntryToComboBox(HWND hDlg, int Item, char *CurName);
int ConnectRas(int Dialup, int UseThis, int Notify, char *Name);

/*===== misc.c =====*/

int InputDialogBox(int Res, HWND hWnd, char *Title, char *Buf, int Max, int *Flg, int Help);
// 64ビット対応
//BOOL CALLBACK ExeEscDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ExeEscDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
// 64ビット対応
//BOOL CALLBACK ExeEscTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ExeEscTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
// 全設定暗号化対応
INT_PTR CALLBACK AnyButtonDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void SetYenTail(char *Str);
void RemoveYenTail(char *Str);
void SetSlashTail(char *Str);
void RemoveReturnCode(char *Str);
void ReplaceAll(char *Str, char Src, char Dst);
int IsDigitSym(int Ch, int Sym);
int StrAllSameChar(char *Str, char Ch);
void RemoveTailingSpaces(char *Str);
char *stristr(char *s1, char *s2);
char *GetNextField(char *Str);
int GetOneField(char *Str, char *Buf, int Max);
void RemoveComma(char *Str);
char *GetFileName(char *Path);
char *GetFileExt(char *Path);
char *GetToolName(char *Path);
void RemoveFileName(char *Path, char *Buf);
void GetUpperDir(char *Path);
void GetUpperDirEraseTopSlash(char *Path);
int AskDirLevel(char *Path);
void MakeSizeString(double Size, char *Buf);
void DispStaticText(HWND hWnd, char *Str);
int StrMultiLen(char *Str);
void RectClientToScreen(HWND hWnd, RECT *Rect);
int hex2bin(char Ch);
int SplitUNCpath(char *unc, char *Host, char *Path, char *File, char *User, char *Pass, int *Port);
int TimeString2FileTime(char *Time, FILETIME *Buf);
// タイムスタンプのバグ修正
//void FileTime2TimeString(FILETIME *Time, char *Buf, int Mode, int InfoExist);
void FileTime2TimeString(FILETIME *Time, char *Buf, int Mode, int InfoExist, int ShowSeconds);
void SpecificLocalFileTime2FileTime(FILETIME *Time, int TimeZone);
int AttrString2Value(char *Str);
// ファイルの属性を数字で表示
//void AttrValue2String(int Attr, char *Buf);
void AttrValue2String(int Attr, char *Buf, int ShowNumber);
void FormatIniString(char *Str);
int SelectFile(HWND hWnd, char *Fname, char *Title, char *Filters, char *Ext, int Flags, int Save);
int SelectDir(HWND hWnd, char *Buf, int MaxLen);
void SetRadioButtonByValue(HWND hDlg, int Value, const RADIOBUTTON *Buttons, int Num);
int AskRadioButtonValue(HWND hDlg, const RADIOBUTTON *Buttons, int Num);
int xtoi(char *Str);
int CheckFileReadable(char *Fname);
int max1(int n, int m);
int min1(int n, int m);
void ExcEndianDWORD(DWORD *x);
void SwapInt(int *Num1, int *Num2);
int IsFolderExist(char *Path);
int ConvertNum(int x, int Dir, const INTCONVTBL *Tbl, int Num);
int MoveFileToTrashCan(char *Path);
LONGLONG MakeLongLong(DWORD High, DWORD Low);
char *MakeNumString(LONGLONG Num, char *Buf, BOOL Comma);
// 異なるファイルが表示されるバグ修正
char* MakeDistinguishableFileName(char* Out, char* In);
// 環境依存の不具合対策
char* GetAppTempPath(char* Buf);
#if defined(HAVE_TANDEM)
void CalcExtentSize(TRANSPACKET *Pkt, LONGLONG Size);
#endif
// 高DPI対応
void QueryDisplayDPI();
int CalcPixelX(int x);
int CalcPixelY(int y);
HBITMAP ResizeBitmap(HBITMAP hBitmap, int UnitSizeX, int UnitSizeY, int ScaleNumerator, int ScaleDenominator);

/*===== dlgsize.c =====*/

void DlgSizeInit(HWND hDlg, DIALOGSIZE *Dt, SIZE *Size);
void AskDlgSize(HWND hDlg, DIALOGSIZE *Dt, SIZE *Size);
void DlgSizeChange(HWND hDlg, DIALOGSIZE *Dt, RECT *New, int Flg);

/*===== opie.c =====*/

int Make6WordPass(int seq, char *seed, char *pass, int type, char *buf);

/*===== tool.c =====*/

void OtpCalcTool(void);
// FTPS対応
void TurnStatefulFTPFilter();

/*===== history.c =====*/

void AddHostToHistory(HOSTDATA *Host, int TrMode);
void AddHistoryToHistory(HISTORYDATA *Hist);
int AskHistoryNum(void);
void CheckHistoryNum(int Space);
void CopyHistoryToHost(HISTORYDATA *Hist, HOSTDATA *Host);
void CopyDefaultHistory(HISTORYDATA *Set);
void SetAllHistoryToMenu(void);
int GetHistoryByCmd(int MenuCmd, HISTORYDATA *Buf);
int GetHistoryByNum(int Num, HISTORYDATA *Buf);

/*===== clipboard.c =====*/

int CopyStrToClipBoard(char *Str);

/*===== diskfree.c =====*/

void LoadKernelLib(void);
void ReleaseKernelLib(void);
char *AskLocalFreeSpace(char *Path);

/*===== socket.c =====*/

int MakeSocketWin(HWND hWnd, HINSTANCE hInst);
void DeleteSocketWin(void);
// ソケットにデータを付与
int SetAsyncTableDataIPv4(SOCKET s, struct sockaddr_in* Host, struct sockaddr_in* Socks);
int SetAsyncTableDataIPv6(SOCKET s, struct sockaddr_in6* Host, struct sockaddr_in6* Socks);
int SetAsyncTableDataMapPort(SOCKET s, int Port);
int GetAsyncTableDataIPv4(SOCKET s, struct sockaddr_in* Host, struct sockaddr_in* Socks);
int GetAsyncTableDataIPv6(SOCKET s, struct sockaddr_in6* Host, struct sockaddr_in6* Socks);
int GetAsyncTableDataMapPort(SOCKET s, int* Port);
// IPv6対応
//struct hostent *do_gethostbyname(const char *Name, char *Buf, int Len, int *CancelCheckWork);
struct hostent *do_gethostbynameIPv4(const char *Name, char *Buf, int Len, int *CancelCheckWork);
struct hostent *do_gethostbynameIPv6(const char *Name, char *Buf, int Len, int *CancelCheckWork);
SOCKET do_socket(int af, int type, int protocol);
int do_connect(SOCKET s, const struct sockaddr *name, int namelen, int *CancelCheckWork);
int do_closesocket(SOCKET s);
int do_listen(SOCKET s,	int backlog);
SOCKET do_accept(SOCKET s, struct sockaddr *addr, int *addrlen);
int do_recv(SOCKET s, char *buf, int len, int flags, int *TimeOut, int *CancelCheckWork);
int do_send(SOCKET s, const char *buf, int len, int flags, int *TimeOutErr, int *CancelCheckWork);
// 同時接続対応
void RemoveReceivedData(SOCKET s);
// UPnP対応
int LoadUPnP();
void FreeUPnP();
int IsUPnPLoaded();
int AddPortMapping(char* Adrs, int Port, char* ExtAdrs);
int RemovePortMapping(int Port);
int CheckClosedAndReconnect(void);
// 同時接続対応
int CheckClosedAndReconnectTrnSkt(SOCKET *Skt, int *CancelCheckWork);


extern HCRYPTPROV HCryptProv;

void sha_memory(const char* mem, DWORD length, uint32_t* buffer);

template<class Target, class Source>
constexpr auto data_as(Source& source) {
	return reinterpret_cast<Target*>(std::data(source));
}
template<class Target, class Source>
constexpr auto data_as(Source const& source) {
	return reinterpret_cast<const Target*>(std::data(source));
}
template<class Size, class Source>
constexpr auto size_as(Source const& source) {
	return static_cast<Size>(std::size(source));
}
std::wstring u8(std::string_view const& utf8);
std::string u8(std::wstring_view const& wide);
template<class Char>
static inline auto u8(const Char* str) {
	return u8(std::basic_string_view<Char>{ str });
}
template<class Char>
static inline auto u8(const Char* str, size_t len) {
	return u8(std::basic_string_view<Char>{ str, len });
}
template<class Char, class Traits, class Allocator>
static inline auto u8(std::basic_string<Char, Traits, Allocator> const& str) {
	return u8(std::basic_string_view<Char>{ data(str), size(str) });
}
static inline auto Message(HWND owner, HINSTANCE instance, int textId, int captionId, DWORD style) {
	MSGBOXPARAMSW msgBoxParams{ sizeof MSGBOXPARAMSW, owner, instance, MAKEINTRESOURCEW(textId), MAKEINTRESOURCEW(captionId), style, nullptr, 0, nullptr, LANG_NEUTRAL };
	return MessageBoxIndirectW(&msgBoxParams);
}
