﻿/*=============================================================================
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
#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WINNLS_									// avoid include winnls.h
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <future>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
#include <concurrent_queue.h>
#include <boost/regex.hpp>
#include <cassert>
#include <cwctype>
#include <crtdbg.h>
#include <mbstring.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#pragma push_macro("WINVER")
#undef _WINNLS_
#if WINVER < 0x0600
#undef WINVER
#define WINVER 0x0600			// fake WINVER to load Windows API Normalization Functions.
#endif
#include <WinNls.h>
#pragma pop_macro("WINVER")
#include <ObjBase.h>			// for COM interface, define `interface` macro.
#include <windowsx.h>
#include <winsock2.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <MLang.h>
#include <MMSystem.h>
#include <mstcpip.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <ShObjIdl.h>
#include <WinCrypt.h>
#include <WS2tcpip.h>
#include "wrl/client.h"
#include "wrl/implements.h"
#include <comutil.h>
#include "config.h"
#include "dialog.h"
#include "helpid.h"
#include "Resource/resource.ja-JP.h"
#include "mesg-jpn.h"
// IdnToAscii()、NormalizeString()共にVistaからNormaliz.dllからKERNEL32.dllに移されている。
// この影響かのためかNormalizeString()だけはkernel32.libにも登録されている（多分バグ）。
// このため、XP向けに正しくリンクするためにはリンカー引数でkernel32.libよりも前にnormaliz.libを置く必要がある。
// #pragma comment(lib, "normaliz.lib") ではこれを実現できないため、リンクオプションで設定する。
// 逆にIdnToAscii()はkernel32.libに登録されていないため、Vista以降をターゲットとする場合でもnormaliz.libは必要となる。
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "comsuppw.lib")
namespace fs = std::filesystem;
using namespace std::literals;
template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
template<class...>
constexpr bool false_v = false;

enum class FileType : UINT {
	All = IDS_FILETYPE_ALL,
	Executable = IDS_FILETYPE_EXECUTABLE,
	Audio = IDS_FILETYPE_AUDIO,
	Reg = IDS_FILETYPE_REG,
	Ini = IDS_FILETYPE_INI,
	Xml = IDS_FILETYPE_XML,
};

constexpr FileType AllFileTyes[]{ FileType::All, FileType::Executable, FileType::Audio, FileType::Reg, FileType::Ini, FileType::Xml, };


#define NUL				'\0'

#define IsDigit(n)		(isascii(n) && isdigit(n))
#define IsAlpha(n)		(isascii(n) && isalpha(n))

#define FFFTP_FAIL			0
#define FFFTP_SUCCESS			1

#define NO				0
#define YES				1
#define NO_ALL			2
#define YES_ALL			3
#define YES_LIST		4

/*===== バージョン ======*/

#ifdef _WIN64
#define VER_STR					"4.7 64bit"
#else
#define VER_STR					"4.7 32bit"
#endif
#define VER_NUM					2000		/* 設定バージョン */

/*===== ユーザ定義コマンド =====*/

#define WM_CHANGE_COND	(WM_USER+1)	/* ファイル一覧を変更するコマンド */
#define WM_SET_PACKET	(WM_USER+2)	/* 現在使用している転送パケットのアドレスを通知 */

#define WM_ASYNC_SOCKET	(WM_USER+5)

#define WM_REFRESH_LOCAL_FLG	(WM_USER+7)
#define WM_REFRESH_REMOTE_FLG	(WM_USER+8)

// UPnP対応
#define WM_ADDPORTMAPPING	(WM_USER+9)
#define WM_REMOVEPORTMAPPING	(WM_USER+10)

// 同時接続対応
#define WM_RECONNECTSOCKET	(WM_USER+11)

// ゾーンID設定追加
#define WM_MARKFILEASDOWNLOADEDFROMINTERNET	(WM_USER+12)

/*===== ホスト番号 =====*/
/* ホスト番号は 0～ の値を取る */

#define HOSTNUM_NOENTRY	(-1)	/* ホスト一覧に無いときのホスト番号 */

/*===== バッファサイズ =====*/

#define HOST_NAME_LEN	40		/* 一覧に表示するホストの名前 */
#define HOST_ADRS_LEN	80		/* ホスト名 */
#define USER_NAME_LEN	80		/* ユーザ名 */
#define PASSWORD_LEN	80		/* パスワード */
#define ACCOUNT_LEN		80		/* アカウント */
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

#define CHMOD_CMD_NOR	"SITE CHMOD"	/* 属性変更コマンド */
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

#define SORT_NOTSAVED	0xFFFF'FFFFu	/* ホスト毎のセーブ方法を保存していない時の値 */

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

/*===== レジストリのタイプ =====*/

#define REGTYPE_REG		0		/* レジストリ */
#define REGTYPE_INI		1		/* INIファイル */

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

/*===== ホストのヒストリ =====*/

#define	HISTORY_MAX		20		/* ファイルのヒストリの最大個数 */
#define DEF_FMENU_ITEMS	8		/* Fileメニューにある項目数の初期値 */

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


struct HostExeptPassword {
	static inline int DefaultTimeZone = [] {
		TIME_ZONE_INFORMATION tzi;
		GetTimeZoneInformation(&tzi);
		return tzi.Bias / -60;
	}();
	char HostAdrs[HOST_ADRS_LEN + 1] = "";				/* ホスト名 */
	char UserName[USER_NAME_LEN + 1] = "";				/* ユーザ名 */
	char Account[ACCOUNT_LEN + 1] = "";					/* アカウント */
	char LocalInitDir[INIT_DIR_LEN + 1];				/* ローカルの開始ディレクトリ */
	char RemoteInitDir[INIT_DIR_LEN + 1] = "";			/* ホストの開始ディレクトリ */
	char ChmodCmd[CHMOD_CMD_LEN + 1] = CHMOD_CMD_NOR;	/* 属性変更コマンド */
	char LsName[NLST_NAME_LEN + 1] = LS_FNAME;			/* NLSTに付けるファイル名/オプション*/
	char InitCmd[INITCMD_LEN + 1] = "";					/* ホストの初期化コマンド */
	int Port = IPPORT_FTP;								/* ポート番号 */
	int KanjiCode = KANJI_NOCNV;						/* ホストの漢字コード (KANJI_xxx) */
	int KanaCnv = YES;									/* 半角カナを全角に変換(YES/NO) */
	int NameKanjiCode = KANJI_NOCNV;					/* ファイル名の漢字コード (KANJI_xxx) */
	int NameKanaCnv = NO;								/* ファイル名の半角カナを全角に変換(YES/NO) */
	int Pasv = YES;										/* PASVモード (YES/NO) */
	int FireWall = NO;									/* FireWallを使う (YES/NO) */
	int ListCmdOnly = YES;								/* "LIST"コマンドのみ使用する */
	int UseNLST_R = YES;								/* "NLST -R"コマンドを使用する */
	int TimeZone = DefaultTimeZone;						/* タイムゾーン (-12～12) */
	int HostType = HTYPE_AUTO;							/* ホストのタイプ (HTYPE_xxx) */
	int SyncMove = NO;									/* フォルダ同時移動 (YES/NO) */
	int NoFullPath = NO;								/* フルパスでファイルアクセスしない (YES/NO) */
	uint32_t Sort = SORT_NOTSAVED;						/* ソート方法 (0x11223344 : 11=LFsort 22=LDsort 33=RFsort 44=RFsort) */
	int Security = SECURITY_AUTO;						/* セキュリティ (SECURITY_xxx , MDx) */
	int Dialup = NO;									/* ダイアルアップ接続するかどうか (YES/NO) */
	int DialupAlways = NO;								/* 常にこのエントリへ接続するかどうか (YES/NO) */
	int DialupNotify = YES;								/* 再接続の際に確認する (YES/NO) */
	char DialEntry[RAS_NAME_LEN + 1] = "";				/* ダイアルアップエントリ */
	int UseNoEncryption = YES;							/* 暗号化なしで接続する (YES/NO) */
	int UseFTPES = YES;									/* FTPESで接続する (YES/NO) */
	int UseFTPIS = YES;									/* FTPISで接続する (YES/NO) */
	int UseSFTP = YES;									/* SFTPで接続する (YES/NO) */
	char PrivateKey[PRIVATE_KEY_LEN + 1] = "";			/* テキスト形式の秘密鍵 */
	int MaxThreadCount = 1;								/* 同時接続数 */
	int ReuseCmdSkt = YES;								/* メインウィンドウのソケットを再利用する (YES/NO) */
	int UseMLSD = YES;									/* "MLSD"コマンドを使用する */
	int NoopInterval = 60;								/* 無意味なコマンドを送信する間隔（秒数、0で無効）*/
	int TransferErrorMode = EXIST_OVW;					/* 転送エラー時の処理 (EXIST_xxx) */
	int TransferErrorNotify = YES;						/* 転送エラー時に確認ダイアログを出すかどうか (YES/NO) */
	int TransferErrorReconnect = YES;					/* 転送エラー時に再接続する (YES/NO) */
	int NoPasvAdrs = NO;								/* PASVで返されるアドレスを無視する (YES/NO) */
	HostExeptPassword();
};

struct Host : HostExeptPassword {
	char PassWord[PASSWORD_LEN+1] = "";		/* パスワード */
	Host() = default;
	Host(Host const&) = default;
	Host(Host const& that, bool includePassword) : HostExeptPassword{ that } {
		if (includePassword)
			strcpy(PassWord, that.PassWord);
	}
};


struct HOSTDATA : Host {
	int Level = 0;							/* 設定のレベル */
											/* 通常はグループのフラグのみが有効 */
											/* レベル数は設定の登録／呼出時のみで使用 */
	char HostName[HOST_NAME_LEN+1] = "";	/* 設定名 */
	char BookMark[BOOKMARK_SIZE] = "";		/* ブックマーク */
	int Anonymous = NO;						/* Anonymousフラグ */
	int CurNameKanjiCode = KANJI_NOCNV;		/* 自動判別後のファイル名の漢字コード (KANJI_xxx) */
	int LastDir = NO;						/* 最後にアクセスしたフォルダを保存 */
	int CryptMode = CRYPT_NONE;				/* 暗号化通信モード (CRYPT_xxx) */
	int NoDisplayUI = NO;					/* UIを表示しない (YES/NO) */
	int Feature = 0;						/* 利用可能な機能のフラグ (FEATURE_xxx) */
	int CurNetType = NTYPE_AUTO;			/* 接続中のネットワークの種類 (NTYPE_xxx) */
	HOSTDATA() = default;
	HOSTDATA(struct HISTORYDATA const& history);
};


struct HISTORYDATA : Host {
	int Type;							/* 転送方法 (TYPE_xx) */
	HISTORYDATA() : Type{ 0 } {}
	HISTORYDATA(HISTORYDATA const&) = default;
	HISTORYDATA(Host const& that, bool includePassword, int type) : Host{ that, includePassword }, Type{ type } {}
};


struct TRANSPACKET {
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
	int NoTransfer;
	int ThreadCount;
};


/*===== ファイルリスト =====*/

typedef struct filelist {
	char File[FMAX_PATH + 1] = {};			/* ファイル名 */
	char Node = 0;							/* 種類 (NODE_xxx) */
	char Link = 0;							/* リンクファイルかどうか (YES/NO) */
	LONGLONG Size = 0;						/* ファイルサイズ */
	int Attr = 0;							/* 属性 */
	FILETIME Time = {};						/* 時間(UTC) */
	char Owner[OWNER_NAME_LEN + 1] = {};	/* オーナ名 */
	char InfoExist = 0;						/* ファイル一覧に存在した情報のフラグ (FINFO_xxx) */
	int ImageId = 0;						/* アイコン画像番号 */
	filelist() = default;
	filelist(const char* file, char node) : Node{ node } {
		strcpy(File, file);
	}
	filelist(const char* file, char node, char link, LONGLONG size, int attr, FILETIME time, char infoExist) : Node{ node }, Link{ link }, Size{ size }, Attr{ attr }, Time{ time }, InfoExist{ infoExist } {
		strcpy(File, file);
	}
	filelist(const char* file, char node, char link, LONGLONG size, int attr, FILETIME time, const char* owner, char infoExist) : Node{ node }, Link{ link }, Size{ size }, Attr{ attr }, Time{ time }, InfoExist{ infoExist } {
		strcpy(File, file);
		strcpy(Owner, owner);
	}
} FILELIST;


/*===== サウンドファイル =====*/

typedef struct {
	int On;						/* ON/OFFスイッチ */
	char Fname[FMAX_PATH+1];		/* ファイル名 */
} SOUNDFILE;


// UPnP対応
typedef struct
{
	int r;
	HANDLE h;
	const char* Adrs;
	int Port;
	char* ExtAdrs;
} ADDPORTMAPPINGDATA;

typedef struct
{
	int r;
	HANDLE h;
	int Port;
} REMOVEPORTMAPPINGDATA;

// ゾーンID設定追加
typedef struct
{
	int r;
	HANDLE h;
	char* Fname;
} MARKFILEASDOWNLOADEDFROMINTERNETDATA;

/*=================================================
*		プロトタイプ
*=================================================*/

/*===== main.c =====*/

fs::path systemDirectory();
fs::path const& tempDirectory();
void DispWindowTitle();
HWND GetMainHwnd(void);
HWND GetFocusHwnd(void);
void SetFocusHwnd(HWND hWnd);
HINSTANCE GetFtpInst(void);
void DoubleClickProc(int Win, int Mode, int App);
void ExecViewer(char *Fname, int App);
void ExecViewer2(char *Fname1, char *Fname2, int App);
void AddTempFileList(fs::path const& file);
void SoundPlay(int Num);
void ShowHelp(DWORD_PTR helpTopicId);
fs::path const& AskIniFilePath();
int AskForceIni(void);
int BackgrndMessageProc(void);
void ResetAutoExitFlg(void);
int AskAutoExit(void);
// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
BOOL IsMainThread();
void Restart();
void Terminate();
// タスクバー進捗表示
int LoadTaskbarList3();
void FreeTaskbarList3();
int IsTaskbarList3Loaded();
void UpdateTaskbarProgress();
// 高DPI対応
int AskToolWinHeight(void);

/*===== filelist.c =====*/

int MakeListWin();
void DeleteListWin(void);
HWND GetLocalHwnd(void);
HWND GetRemoteHwnd(void);
void GetListTabWidth(void);
void SetListViewType(void);
void GetRemoteDirForWnd(int Mode, int *CancelCheckWork);
void GetLocalDirForWnd(void);
void ReSortDispList(int Win, int *CancelCheckWork);
bool CheckFname(std::wstring str, std::wstring const& regexp);
void SelectFileInList(HWND hWnd, int Type, std::vector<FILELIST> const& Base);
void FindFileInList(HWND hWnd, int Type);
int GetCurrentItem(int Win);
int GetItemCount(int Win);
int GetSelectedCount(int Win);
int GetFirstSelected(int Win, int All);
int GetNextSelected(int Win, int Pos, int All);
// ローカル側自動更新
int GetHotSelected(int Win, char *Fname);
int SetHotSelected(int Win, char *Fname);
int FindNameNode(int Win, char *Name);
std::wstring GetNodeName(int Win, int Pos);
void GetNodeName(int Win, int Pos, char *Buf, int Max);
int GetNodeTime(int Win, int Pos, FILETIME *Buf);
int GetNodeSize(int Win, int Pos, LONGLONG *Buf);
int GetNodeAttr(int Win, int Pos, int *Buf);
int GetNodeType(int Win, int Pos);
void GetNodeOwner(int Win, int Pos, char *Buf, int Max);
void EraseRemoteDirForWnd(void);
double GetSelectedTotalSize(int Win);
int MakeSelectedFileList(int Win, int Expand, int All, std::vector<FILELIST>& Base, int *CancelCheckWork);
void MakeDroppedFileList(WPARAM wParam, char *Cur, std::vector<FILELIST>& Base);
void MakeDroppedDir(WPARAM wParam, char *Cur);
void AddRemoteTreeToFileList(int Num, char *Path, int IncDir, std::vector<FILELIST>& Base);
const FILELIST* SearchFileList(const char* Fname, std::vector<FILELIST> const& Base, int Caps);
static inline FILELIST* SearchFileList(const char* Fname, std::vector<FILELIST>& Base, int Caps) {
	return const_cast<FILELIST*>(SearchFileList(Fname, static_cast<std::vector<FILELIST> const&>(Base), Caps));
}
int Assume1900or2000(int Year);
void SetFilter(int *CancelCheckWork);
void doDeleteRemoteFile(void);


/*===== toolmenu.c =====*/

bool MakeToolBarWindow();
void DeleteToolBarWindow(void);
HWND GetMainTbarWnd(void);
HWND GetLocalHistHwnd(void);
HWND GetRemoteHistHwnd(void);
HWND GetLocalHistEditHwnd(void);
HWND GetRemoteHistEditHwnd(void);
HWND GetLocalTbarWnd(void);
HWND GetRemoteTbarWnd(void);
void MakeButtonsFocus(void);
void DisableUserOpe(void);
void EnableUserOpe(void);
bool AskUserOpeDisabled();
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
void SetRemoteDirHist(std::wstring const&  path);
void SetLocalDirHist(fs::path const& path);
fs::path const& AskLocalCurDir();
std::wstring const& AskRemoteCurDir();
void SetCurrentDirAsDirHist();
void DispDotFileMode(void);
void ShowPopupMenu(int Win, int Pos);

/*===== statuswin.c =====*/

int MakeStatusBarWindow();
void DeleteStatusBarWindow(void);
HWND GetSbarWnd(void);
void UpdateStatusBar();
void DispCurrentWindow(int Win);
void DispSelectedSpace(void);
void DispLocalFreeSpace(char *Path);
void DispTransferFiles(void);
void DispDownloadSize(LONGLONG Size);
bool NotifyStatusBar(const NMHDR* hdr);

/*===== taskwin.c =====*/

int MakeTaskWindow();
void DeleteTaskWindow(void);
HWND GetTaskWnd(void);
void SetTaskMsg(_In_z_ _Printf_format_string_ const char* format, ...);
void DispTaskMsg(void);
void DoPrintf(_In_z_ _Printf_format_string_ const char* format, ...);

/*===== hostman.c =====*/

int SelectHost(int Type);
int AddHostToList(HOSTDATA *Set, int Pos, int Level);
int CopyHostFromList(int Num, HOSTDATA *Set);
int CopyHostFromListInConnect(int Num, HOSTDATA *Set);
int SetHostBookMark(int Num, char *Bmask, int Len);
char *AskHostBookMark(int Num);
int SetHostDir(int Num, const char* LocDir, const char* HostDir);
int SetHostPassword(int Num, char *Pass);
int SetHostSort(int Num, int LFSort, int LDSort, int RFSort, int RDSort);
void DecomposeSortType(uint32_t Sort, int *LFSort, int *LDSort, int *RFSort, int *RDSort);
int AskCurrentHost(void);
void SetCurrentHost(int Num);
void CopyDefaultHost(HOSTDATA *Set);
// ホスト共通設定機能
void ResetDefaultHost(void);
void SetDefaultHost(HOSTDATA *Set);
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
std::string AskHostChmodCmd();
int AskHostTimeZone(void);
int AskPasvMode(void);
std::string AskHostLsName();
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
std::optional<sockaddr_storage> SocksReceiveReply(SOCKET s, int* CancelCheckWork);
SOCKET connectsock(char *host, int port, char *PreMsg, int *CancelCheckWork);
SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork);
int AskTryingConnect(void);
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
// ホスト側の設定ミス対策
int AskNoPasvAdrs(void);

/*===== cache.c =====*/

fs::path MakeCacheFileName(int Num);

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
std::optional<std::wstring> ChmodDialog(std::wstring const& text);
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

int DoLocalCWD(const char *Path);
void DoLocalMKD(const char* Path);
void DoLocalPWD(char *Buf);
void DoLocalRMD(const char* Path);
void DoLocalDELE(const char* Path);
void DoLocalRENAME(const char *Src, const char *Dst);
void DispFileProperty(const char* Fname);

/*===== remote.c =====*/

int DoCWD(const char *Path, int Disp, int ForceGet, int ErrorBell);
int DoCWDStepByStep(const char* Path, const char* Cur);
int DoMKD(const char* Path);
void InitPWDcommand();
int DoRMD(const char* Path);
int DoDELE(const char* Path);
int DoRENAME(const char *Src, const char *Dst);
int DoCHMOD(const char *Path, const char *Mode);
int DoSIZE(SOCKET cSkt, const char* Path, LONGLONG *Size, int *CancelCheckWork);
int DoMDTM(SOCKET cSkt, const char* Path, FILETIME *Time, int *CancelCheckWork);
// ホスト側の日時設定
int DoMFMT(SOCKET cSkt, const char* Path, FILETIME *Time, int *CancelCheckWork);
int DoQUOTE(SOCKET cSkt, const char* CmdStr, int *CancelCheckWork);
SOCKET DoClose(SOCKET Sock);
// 同時接続対応
//int DoQUIT(SOCKET ctrl_skt);
int DoQUIT(SOCKET ctrl_skt, int *CancelCheckWork);
int DoDirListCmdSkt(const char* AddOpt, const char* Path, int Num, int *CancelCheckWork);
#if defined(HAVE_TANDEM)
void SwitchOSSProc(void);
#endif
#define CommandProcTrn(CSKT, REPLY, CANCELCHECKWORK, ...) (command(CSKT, REPLY, CANCELCHECKWORK, __VA_ARGS__))
int command(SOCKET cSkt, char* Reply, int* CancelCheckWork, _In_z_ _Printf_format_string_ const char* fmt, ...);
int ReadReplyMessage(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork, char *Tmp);
int ReadNchar(SOCKET cSkt, char *Buf, int Size, int *CancelCheckWork);
void ReportWSError(const char* Msg, UINT Error);

/*===== getput.c =====*/

int MakeTransferThread(void);
void CloseTransferThread(void);
// 同時接続対応
void AbortAllTransfer();
int AddTmpTransFileList(TRANSPACKET const& item, std::forward_list<TRANSPACKET>& list);
int RemoveTmpTransFileListItem(std::forward_list<TRANSPACKET>& list, int Num);

void AddTransFileList(TRANSPACKET *Pkt);
// バグ対策
void AddNullTransFileList();
void AppendTransFileList(std::forward_list<TRANSPACKET>&& list);
void KeepTransferDialog(int Sw);
int AskTransferNow(void);
int AskTransferFileNum(void);
void GoForwardTransWindow(void);
void InitTransCurDir(void);
int DoDownload(SOCKET cSkt, TRANSPACKET& item, int DirList, int *CancelCheckWork);
int CheckPathViolation(TRANSPACKET const& item);
// タスクバー進捗表示
LONGLONG AskTransferSizeLeft(void);
LONGLONG AskTransferSizeTotal(void);
int AskTransferErrorDisplay(void);
// ゾーンID設定追加
int LoadZoneID();
void FreeZoneID();
int IsZoneIDLoaded();
int MarkFileAsDownloadedFromInternet(char* Fname);

/*===== codecnv.c =====*/

class CodeDetector {
	int utf8 = 0;
	int sjis = 0;
	int euc = 0;
	int jis = 0;
	bool nfc = false;
	bool nfd = false;
public:
	void Test(std::string_view str);
	int result() const {
		DoPrintf("CodeDetector::result(): utf8 %d, sjis %d, euc %d, jis %d, nfc %d, nfd %d", utf8, sjis, euc, jis, int(nfc), int(nfd));
		auto& [_, id] = std::max<std::tuple<int, int>>({
			{ utf8, KANJI_UTF8N },
			{ sjis, KANJI_SJIS },
			{ euc, KANJI_EUC },
			{ jis, KANJI_JIS },
			}, [](auto const& l, auto const& r) { return std::get<0>(l) < std::get<0>(r); });
		return id == KANJI_UTF8N && nfd ? KANJI_UTF8HFSX : id;
	}
};

class CodeConverter {
	const int incode;
	const int outcode;
	const bool kana;
	bool first = true;
	std::string rest;
public:
	CodeConverter(int incode, int outcode, bool kana) : incode{ outcode == KANJI_NOCNV || incode == outcode && !kana ? KANJI_NOCNV : incode }, outcode{ outcode }, kana{ kana } {}
	std::string Convert(std::string_view input);
};

std::string ToCRLF(std::string_view source);
std::string ConvertFrom(std::string_view str, int kanji);
std::string ConvertTo(std::string_view str, int kanji, int kana);

/*===== option.c =====*/

void SetOption();
int SortSetting(void);
// hostman.cで使用
int GetDecimalText(HWND hDlg, int Ctrl);
void SetDecimalText(HWND hDlg, int Ctrl, int Num);
void CheckRange2(int *Cur, int Max, int Min);
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);

/*===== bookmark.c =====*/

void ClearBookMark();
void AddCurDirToBookMark(int Win);
std::tuple<std::wstring, std::wstring> AskBookMarkText(size_t MarkID);
void SaveBookMark();
void LoadBookMark();
void EditBookMark();

/*===== registry.c =====*/

void SaveRegistry(void);
int LoadRegistry(void);
void ClearRegistry();
// ポータブル版判定
void ClearIni(void);
void SetMasterPassword( const char* );
// セキュリティ強化
void GetMasterPassword(char*);
int GetMasterPasswordStatus(void);
int ValidateMasterPassword(void);
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

int InitListViewTips();
void DeleteListViewTips(void);
void EraseListViewTips(void);
HWND GetListViewTipsHwnd(void);
void CheckTipsDisplay(HWND hWnd, LPARAM lParam);

/*===== ras.c =====*/

void DisconnectRas(bool confirm);
void SetRasEntryToComboBox(HWND hdlg, int item, std::wstring const& selected);
bool ConnectRas(bool dialup, bool explicitly, bool confirm, std::wstring const& name);

/*===== misc.c =====*/

void SetYenTail(char *Str);
void RemoveYenTail(char *Str);
void SetSlashTail(char *Str);
void ReplaceAll(char *Str, char Src, char Dst);
int IsDigitSym(int Ch, int Sym);
int StrAllSameChar(const char* Str, char Ch);
void RemoveTailingSpaces(char *Str);
const char* stristr(const char* s1, const char* s2);
static inline char* stristr(char* s1, const char* s2) { return const_cast<char*>(stristr(static_cast<const char*>(s1), s2)); }
const char* GetNextField(const char* Str);
int GetOneField(const char* Str, char *Buf, int Max);
void RemoveComma(char *Str);
const char* GetFileName(const char* Path);
const char* GetFileExt(const char* Path);
void RemoveFileName(const char* Path, char *Buf);
void GetUpperDir(char *Path);
void GetUpperDirEraseTopSlash(char *Path);
int AskDirLevel(const char* Path);
void MakeSizeString(double Size, char *Buf);
void DispStaticText(HWND hWnd, const char* Str);
int StrMultiLen(const char *Str);
void RectClientToScreen(HWND hWnd, RECT *Rect);
int SplitUNCpath(char *unc, char *Host, char *Path, char *File, char *User, char *Pass, int *Port);
int TimeString2FileTime(const char *Time, FILETIME *Buf);
void FileTime2TimeString(const FILETIME *Time, char *Buf, int Mode, int InfoExist, int ShowSeconds);
void SpecificLocalFileTime2FileTime(FILETIME *Time, int TimeZone);
int AttrString2Value(const char *Str);
// ファイルの属性を数字で表示
//void AttrValue2String(int Attr, char *Buf);
void AttrValue2String(int Attr, char *Buf, int ShowNumber);
void FormatIniString(char *Str);
fs::path SelectFile(bool open, HWND hWnd, UINT titleId, const wchar_t* initialFileName, const wchar_t* extension, std::initializer_list<FileType> fileTypes);
int SelectDir(HWND hWnd, char *Buf, size_t MaxLen);
int MoveFileToTrashCan(const char *Path);
std::string MakeNumString(LONGLONG Num);
// 異なるファイルが表示されるバグ修正
char* MakeDistinguishableFileName(char* Out, const char* In);
#if defined(HAVE_TANDEM)
void CalcExtentSize(TRANSPACKET *Pkt, LONGLONG Size);
#endif
int CalcPixelX(int x);
int CalcPixelY(int y);

/*===== opie.c =====*/

int Make6WordPass(int seq, char *seed, char *pass, int type, char *buf);

/*===== tool.c =====*/

void OtpCalcTool(void);
// FTPS対応
void TurnStatefulFTPFilter();

/*===== history.c =====*/

void AddHostToHistory(Host const& host);
void AddHistoryToHistory(HISTORYDATA const& history);
void SetAllHistoryToMenu();
std::optional<HISTORYDATA> GetHistoryByCmd(int menuId);
std::vector<HISTORYDATA> const& GetHistories();

/*===== socket.c =====*/

BOOL LoadSSL();
void FreeSSL();
void ShowCertificate();
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted, const char* ServerName);
bool IsSecureConnection();
BOOL IsSSLAttached(SOCKET s);
int MakeSocketWin();
void DeleteSocketWin(void);
void SetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::string, int>> const& target);
void SetAsyncTableDataMapPort(SOCKET s, int Port);
int GetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::string, int>>& target);
int GetAsyncTableDataMapPort(SOCKET s, int* Port);
SOCKET do_socket(int af, int type, int protocol);
int do_connect(SOCKET s, const struct sockaddr *name, int namelen, int *CancelCheckWork);
int do_closesocket(SOCKET s);
int do_listen(SOCKET s,	int backlog);
SOCKET do_accept(SOCKET s, struct sockaddr *addr, int *addrlen);
int do_recv(SOCKET s, char *buf, int len, int flags, int *TimeOut, int *CancelCheckWork);
int SendData(SOCKET s, const char* buf, int len, int flags, int* CancelCheckWork);
// 同時接続対応
void RemoveReceivedData(SOCKET s);
// UPnP対応
int LoadUPnP();
void FreeUPnP();
int IsUPnPLoaded();
int AddPortMapping(const char* Adrs, int Port, char* ExtAdrs);
int RemovePortMapping(int Port);
int CheckClosedAndReconnect(void);
// 同時接続対応
int CheckClosedAndReconnectTrnSkt(SOCKET *Skt, int *CancelCheckWork);


extern int DispIgnoreHide;
extern HCRYPTPROV HCryptProv;

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
template<class T, class Allocator>
constexpr auto before_end(std::forward_list<T, Allocator>& list) {
	for (auto it = list.before_begin();;)
		if (auto prev = it++; it == end(list))
			return prev;
}
template<class DstChar, class SrcChar, class Fn>
static inline auto convert(Fn&& fn, std::basic_string_view<SrcChar> src) {
	auto len1 = fn(data(src), size_as<int>(src), nullptr, 0);
	std::basic_string<DstChar> dst(len1, 0);
	auto len2 = fn(data(src), size_as<int>(src), data(dst), len1);
	dst.resize(len2);
	return dst;
}
template<class DstChar, class Fn>
static inline auto convert(Fn&& fn, std::string_view src) {
	return convert<DstChar, char>(std::forward<Fn>(fn), src);
}
template<class DstChar, class Fn>
static inline auto convert(Fn&& fn, std::wstring_view src) {
	return convert<DstChar, wchar_t>(std::forward<Fn>(fn), src);
}
template<class Char, class Traits, class Alloc>
static inline auto operator+(std::basic_string<Char, Traits, Alloc> const& left, std::basic_string_view<Char, Traits> const& right) {
	std::basic_string<Char, Traits, Alloc> result(size(left) + size(right), Char(0));
	auto it = std::copy(begin(left), end(left), begin(result));
	std::copy(begin(right), end(right), it);
	return result;
}
template<class Char, class Traits, class Alloc>
static inline auto operator+(std::basic_string_view<Char, Traits> const& left, std::basic_string<Char, Traits, Alloc> const& right) {
	std::basic_string<Char, Traits, Alloc> result(size(left) + size(right), Char(0));
	auto it = std::copy(begin(left), end(left), begin(result));
	std::copy(begin(right), end(right), it);
	return result;
}
static inline auto u8(std::string_view utf8) {
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, srclen, dst, dstlen); }, utf8);
}
static inline auto u8(std::wstring_view wide) {
	return convert<char>([](auto src, auto srclen, auto dst, auto dstlen) { return WideCharToMultiByte(CP_UTF8, 0, src, srclen, dst, dstlen, nullptr, nullptr); }, wide);
}
template<class Char>
static inline auto u8(const Char* str, size_t len) {
	return u8(std::basic_string_view<Char>{ str, len });
}
static auto ieq(std::wstring const& left, std::wstring const& right) {
	return std::equal(begin(left), end(left), begin(right), end(right), [](auto const l, auto const r) { return std::towupper(l) == std::towupper(r); });
}
static auto ieq(std::wstring_view left, std::wstring_view right) {
	return std::equal(begin(left), end(left), begin(right), end(right), [](auto const l, auto const r) { return std::towupper(l) == std::towupper(r); });
}
static auto lc(std::wstring_view source) {
	std::wstring result{ source };
	_wcslwr(data(result));
	return result;
}
template<class Char, class Evaluator>
static inline auto replace(std::basic_string_view<Char> input, boost::basic_regex<Char> const& pattern, Evaluator&& evaluator) {
	std::basic_string<Char> replaced;
	auto last = data(input);
	for (boost::regex_iterator<const Char*> it{ data(input), data(input) + size(input), pattern }, end; it != end; ++it) {
		replaced.append(last, (*it)[0].first);
		replaced += evaluator(*it);
		last = (*it)[0].second;
	}
	replaced.append(last, data(input) + size(input));
	return replaced;
}
template<int captionId = IDS_APP>
static inline auto Message(HWND owner, int textId, DWORD style) {
	MSGBOXPARAMSW msgBoxParams{ sizeof MSGBOXPARAMSW, owner, GetFtpInst(), MAKEINTRESOURCEW(textId), MAKEINTRESOURCEW(captionId), style, nullptr, 0, nullptr, LANG_NEUTRAL };
	return MessageBoxIndirectW(&msgBoxParams);
}
template<int captionId = IDS_APP>
static inline auto Message(int textId, DWORD style) {
	return Message<captionId>(GetMainHwnd(), textId, style);
}
static auto GetString(UINT id) {
	wchar_t buffer[1024];
	auto length = LoadStringW(GetFtpInst(), id, buffer, size_as<int>(buffer));
	return std::wstring(buffer, length);
}
static auto GetText(HWND hwnd) {
	std::wstring text;
	if (auto const length = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0); 0 < length) {
		text.resize(length);
		auto const result = SendMessageW(hwnd, WM_GETTEXT, length + 1, (LPARAM)text.data());
		text.resize(result);
	}
	return text;
}
static inline auto GetText(HWND hdlg, int id) {
	return GetText(GetDlgItem(hdlg, id));
}
static inline void SetText(HWND hwnd, const wchar_t* text) {
	SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)text);
}
static inline void SetText(HWND hdlg, int id, const wchar_t* text) {
	SendDlgItemMessageW(hdlg, id, WM_SETTEXT, 0, (LPARAM)text);
}
static inline void SetText(HWND hwnd, const std::wstring& text) {
	SetText(hwnd, text.c_str());
}
static inline void SetText(HWND hdlg, int id, const std::wstring& text) {
	SetText(hdlg, id, text.c_str());
}
static inline auto AddressPortToString(const SOCKADDR* sa, size_t salen) {
	std::wstring string(sizeof "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff%4294967295]:65535" - 1, L'\0');
	auto length = size_as<DWORD>(string) + 1;
	auto result = WSAAddressToStringW(const_cast<SOCKADDR*>(sa), static_cast<DWORD>(salen), nullptr, data(string), &length);
	assert(result == 0);
	string.resize(length - 1);
	return string;
}
template<class SockAddr>
static inline auto AddressPortToString(const SockAddr* sa, size_t salen = sizeof(SockAddr)) {
	static_assert(std::is_same_v<SockAddr, sockaddr_in> || std::is_same_v<SockAddr, sockaddr_in6> || std::is_same_v<SockAddr, sockaddr_storage>);
	return AddressPortToString(reinterpret_cast<const SOCKADDR*>(sa), salen);
}
static inline auto AddressToString(sockaddr_storage const& sa) {
	if (sa.ss_family == AF_INET) {
		auto local = reinterpret_cast<sockaddr_in const&>(sa);
		local.sin_port = 0;
		return AddressPortToString(&local);
	} else {
		auto local = reinterpret_cast<sockaddr_in6 const&>(sa);
		local.sin6_port = 0;
		local.sin6_scope_id = 0;
		return AddressPortToString(&local);
	}
}
extern bool SupportIdn;
static inline auto IdnToAscii(std::wstring const& unicode) {
	if (!SupportIdn || empty(unicode))
		return unicode;
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return IdnToAscii(0, src, srclen, dst, dstlen); }, unicode);
}
static inline auto NormalizeString(NORM_FORM form, std::wstring_view src) {
	if (!SupportIdn || empty(src))
		return std::wstring{ src };
	return convert<wchar_t>([form](auto src, auto srclen, auto dst, auto dstlen) { return NormalizeString(form, src, srclen, dst, dstlen); }, src);
}
static inline auto InputDialog(int dialogId, HWND parent, char *Title, char *Buf, size_t maxlength = 0, int* flag = nullptr, int helpTopicId = IDH_HELP_TOPIC_0000001) {
	struct Data {
		using result_t = bool;
		char* Title;
		char* Buf;
		size_t maxlength;
		int* flag;
		int helpTopicId;
		Data(char* Title, char* Buf, size_t maxlength, int* flag, int helpTopicId) : Title{ Title }, Buf{ Buf }, maxlength{ maxlength }, flag{ flag }, helpTopicId{ helpTopicId } {}
		INT_PTR OnInit(HWND hDlg) {
			if (Title)
				SetText(hDlg, u8(Title));
			SendDlgItemMessageW(hDlg, INP_INPSTR, EM_LIMITTEXT, maxlength - 1, 0);
			SetText(hDlg, INP_INPSTR, u8(Buf));
			if (flag)
				SendDlgItemMessageW(hDlg, INP_ANONYMOUS, BM_SETCHECK, *flag, 0);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				strncpy(Buf, u8(GetText(hDlg, INP_INPSTR)).c_str(), maxlength - 1);
				if (flag)
					*flag = (int)SendDlgItemMessageW(hDlg, INP_ANONYMOUS, BM_GETCHECK, 0, 0);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case IDHELP:
				ShowHelp(helpTopicId);
				break;
			case INP_BROWSE:
				if (char Tmp[FMAX_PATH + 1]; SelectDir(hDlg, Tmp, FMAX_PATH) == TRUE)
					SetText(hDlg, INP_INPSTR, u8(Tmp));
				break;
			}
		}
	};
	return Dialog(GetFtpInst(), dialogId, parent, Data{ Title, Buf, maxlength, flag, helpTopicId });
}
struct ProcessInformation : PROCESS_INFORMATION {
	ProcessInformation() : PROCESS_INFORMATION{ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE } {}
	~ProcessInformation() {
		if (hThread != INVALID_HANDLE_VALUE)
			CloseHandle(hThread);
		if (hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(hProcess);
	}
};
template<class Fn>
static inline void GetDrives(Fn&& fn) {
	auto drives = GetLogicalDrives();
	DWORD nodrives = 0;
	DWORD size = sizeof(DWORD);
	SHRegGetUSValueW(LR"(Software\Microsoft\Windows\CurrentVersion\Policies\Explorer)", L"NoDrives", nullptr, &nodrives, &size, false, nullptr, 0);
	for (int i = 0; i < sizeof(DWORD) * 8; i++)
		if ((drives & 1 << i) != 0 && (nodrives & 1 << i) == 0) {
			wchar_t drive[] = { wchar_t(L'A' + i), L':', L'\\', 0 };
			fn(drive);
		}
}
static auto GetErrorMessage(int lastError) {
	wchar_t* buffer;
	auto length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	std::wstring message(buffer, buffer + length);
	LocalFree(buffer);
	return std::move(message);
}
