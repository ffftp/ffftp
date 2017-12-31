/*=============================================================================
*
*								ファイル一覧キャッシュ
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

#include "common.h"


static BOOL CALLBACK CountPrevFfftpWindowsProc(HWND hWnd, LPARAM lParam);



/*===== キャッシュデータのストラクチャ =====*/

typedef struct {
	time_t Time;						/* リストに登録した時間 */
	char HostAdrs[HOST_ADRS_LEN+1];		/* ホストのアドレス */
	char UserName[USER_NAME_LEN+1];		/* ユーザ名 */
	char Path[FMAX_PATH+1];				/* パス名 */
} CACHELIST;

/*===== ローカルなワーク =====*/

static CACHELIST *RemoteCache = NULL;	/* キャッシュデータ */
static int TmpCacheEntry = 0;			/* キャッシュされているデータの数 */
static int LastNum;						/* 現在表示しているデータの番号 */
static int ProgNum;						/* FFFTPの起動番号 */


/*----- ファイル一覧キャッシュを作成する --------------------------------------
*
*	Parameter
*		int Num : キャッシュ可能個数
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeCacheBuf(int Num)
{
	int Sts;
	int i;

	Sts = FFFTP_SUCCESS;
	if(Num > 0)
	{
		Sts = FFFTP_FAIL;
		if((RemoteCache = (CACHELIST*)malloc(sizeof(CACHELIST) * Num)) != NULL)
		{
			TmpCacheEntry = Num;
			for(i = 0; i < TmpCacheEntry; i++)
				ClearCache(i);
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- ファイル一覧キャッシュを削除する --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteCacheBuf(void)
{
	if(RemoteCache != NULL)
		free(RemoteCache);
	RemoteCache = NULL;
	TmpCacheEntry = 0;
	return;
}


/*----- パスがキャッシュされているかどうかを返す ------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int キャッシュデータの番号
*			-1 = キャッシュされていない
*----------------------------------------------------------------------------*/

int AskCached(char *Path)
{
	int Ret;
	int i;
	CACHELIST *Pos;

	Ret = -1;
	if(TmpCacheEntry > 0)
	{
		Pos = RemoteCache;
		for(i = 0; i < TmpCacheEntry; i++)
		{
			if((_stricmp(AskHostAdrs(), Pos->HostAdrs) == 0) &&
			   (strcmp(AskHostUserName(), Pos->UserName) == 0) &&
			   (strcmp(Path, Pos->Path) == 0))
			{
				time(&(Pos->Time));		/* Refresh */
				Ret = i;
//				DoPrintf("Filelist cache found. (%d)", Ret);
				break;
			}
			Pos++;
		}
	}
	return(Ret);
}


/*----- 未使用のキャッシュデータ番号を返す ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 未使用のキャッシュデータ番号
*
*	Note
*		未使用の物が無い時は、一番古いものに上書きする
*----------------------------------------------------------------------------*/

int AskFreeCache(void)
{
	int Ret;
	int i;
	time_t Oldest;
	CACHELIST *Pos;

	Ret = 0;
	if(TmpCacheEntry > 0)
	{
		Oldest = 0;
		Ret = -1;
		Pos = RemoteCache;
		for(i = 0; i < TmpCacheEntry; i++)
		{
			if(strlen(Pos->Path) == 0)
			{
				Ret = i;
				break;
			}
			else
			{
				if((Ret == -1) || (Pos->Time < Oldest))
				{
					Oldest = Pos->Time;
					Ret = i;
				}
			}
			Pos++;
		}
	}
//	DoPrintf("Set filelist cache. (%d)", Ret);
	return(Ret);
}


/*----- キャッシュデータをセットする ------------------------------------------
*
*	Parameter
*		int Num : キャッシュデータ番号
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		現在接続中のホスト名を使用する
*----------------------------------------------------------------------------*/

void SetCache(int Num, char *Path)
{
	if(TmpCacheEntry > 0)
	{
		strcpy((RemoteCache + Num)->HostAdrs, AskHostAdrs());
		strcpy((RemoteCache + Num)->UserName, AskHostUserName());
		strcpy((RemoteCache + Num)->Path, Path);
		time(&((RemoteCache + Num)->Time));
	}
	return;
}


/*----- キャッシュデータをクリアする ------------------------------------------
*
*	Parameter
*		int Num : キャッシュデータ番号
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ClearCache(int Num)
{
	if(TmpCacheEntry > 0)
	{
		strcpy((RemoteCache + Num)->HostAdrs, "");
		strcpy((RemoteCache + Num)->UserName, "");
		strcpy((RemoteCache + Num)->Path, "");
		(RemoteCache + Num)->Time = 0;
	}
	return;
}


/*----- 現在表示中のキャッシュデータ番号を返す --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int キャッシュデータ番号
*----------------------------------------------------------------------------*/

int AskCurrentFileListNum(void)
{
	return(LastNum);
}


/*----- 現在表示中のキャッシュデータ番号をセットする --------------------------
*
*	Parameter
*		int Num : キャッシュデータ番号
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetCurrentFileListNum(int Num)
{
	LastNum = Num;
}


/*----- キャッシュデータを保存する --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*
*	Note
*		キャッシュデータそのものはファイルになっているので、ここではインデック
*		スファイルを作成する。
*		また、不要なファイルは削除する。
*----------------------------------------------------------------------------*/

void SaveCache(void)
{
	char Buf[FMAX_PATH+1];
	FILE *fd;
	CACHELIST *Pos;
	int i;

	if(ProgNum == 0)	/* 最初のインスタンスだけがキャッシュを保存できる */
	{
		if(TmpCacheEntry > 0)
		{
			strcpy(Buf, AskTmpFilePath());
			SetYenTail(Buf);
			strcat(Buf, "_ffftp.idx");
			if((fd = fopen(Buf, "wt"))!=NULL)
			{
				Pos = RemoteCache;
				for(i = 0; i < TmpCacheEntry; i++)
				{
					if(strlen(Pos->Path) != 0)
						fprintf(fd, "%s %s %s %lld\n", Pos->HostAdrs, Pos->UserName, Pos->Path, Pos->Time);
					Pos++;
				}
				fclose(fd);
			}
		}
	}
	else
		DeleteCache();

	MakeCacheFileName(998, Buf);
	_unlink(Buf);

	MakeCacheFileName(999, Buf);
	_unlink(Buf);

	return;
}


/*----- キャッシュデータを読み込む --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void LoadCache(void)
{
	char Buf[FMAX_PATH+1];
	FILE *fd;
	CACHELIST *Pos;
	int Num;

	if(ProgNum == 0)	/* 最初のインスタンスだけがキャッシュを保存できる */
	{
		if(TmpCacheEntry > 0)
		{
			strcpy(Buf, AskTmpFilePath());
			SetYenTail(Buf);
			strcat(Buf, "_ffftp.idx");
			if((fd = fopen(Buf, "rt"))!=NULL)
			{
				Pos = RemoteCache;
				Num = 0;
				while(Num < TmpCacheEntry)
				{
					if(fgets(Buf, FMAX_PATH, fd) == NULL)
						break;

					if(sscanf(Buf, "%s %s %s %lld\n", Pos->HostAdrs, Pos->UserName, Pos->Path, &(Pos->Time)) == 4)
					{
						Pos++;
						Num++;
					}
					else
						ClearCache(Num);
				}
				fclose(fd);
			}
		}
	}
	return;
}


/*----- キャッシュデータを全て削除する ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteCache(void)
{
	char Buf[FMAX_PATH+1];
	int i;

	if(ProgNum == 0)
	{
		strcpy(Buf, AskTmpFilePath());
		SetYenTail(Buf);
		strcat(Buf, "_ffftp.idx");
		_unlink(Buf);
	}

	for(i = 0; i <= TmpCacheEntry; i++)
	{
		MakeCacheFileName(i, Buf);
		_unlink(Buf);
	}

	MakeCacheFileName(998, Buf);
	_unlink(Buf);

	MakeCacheFileName(999, Buf);
	_unlink(Buf);

	return;
}


/*----- キャッシュのファイル名を作成する --------------------------------------
*
*	Parameter
*		int Num : キャッシュデータ番号
*		char *Buf : ファイル名を格納するバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeCacheFileName(int Num, char *Buf)
{
	char Prog[10];
	char *Pos;

	strcpy(Buf, AskTmpFilePath());
	SetYenTail(Buf);
	Pos = strchr(Buf, NUL);

	strcpy(Prog, "");
	if(ProgNum > 0)
		sprintf(Prog, ".%d", ProgNum);

	sprintf(Pos, "_ffftp.%03d%s", Num, Prog);
	return;
}


/*----- 起動しているFFFTPの数を数える ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*
*	Note
*		個数はProgNumに格納する
*----------------------------------------------------------------------------*/

void CountPrevFfftpWindows(void)
{
	ProgNum = 0;
	EnumWindows(CountPrevFfftpWindowsProc, 0);
	return;
}


/*----- 起動しているFFFTPの数を数えるコールバック-------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		LPARAM lParam : パラメータ
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

static BOOL CALLBACK CountPrevFfftpWindowsProc(HWND hWnd, LPARAM lParam)
{
	char Buf[FMAX_PATH+1];

	if(GetClassName(hWnd, Buf, FMAX_PATH) > 0)
	{
		if(strcmp(Buf, "FFFTPWin") == 0)
			ProgNum++;
	}
	return(TRUE);
}



