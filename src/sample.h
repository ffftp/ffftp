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

/* サンプルホスト定義ファイル */

typedef struct {
	int Level;							/* 設定のレベル */
	char HostName[HOST_NAME_LEN+1];		/* 設定名 */
	char HostAdrs[HOST_ADRS_LEN+1];		/* ホスト名 */
} SAMPLEHOSTDATA;


#define SAMPLE_HOSTS	9


static const SAMPLEHOSTDATA Sample[SAMPLE_HOSTS] = {
	{ 32768,"anonymous FTP site",				"" },
	{ 1,	"Vector",					"ftp.vector.co.jp" },
	{ 1,	"窓の杜(Forest)",			"ftp.forest.impress.co.jp" },
	{ 1,	"Ring server",				"ftp.ring.gr.jp" },
	{ 1,	"BIGLOBE",					"ftp.biglobe.ne.jp" },
	{ 1,	"IIJ",						"ftp.iij.ad.jp" },
	{ 1,	"SUNSITE",					"sunsite.sut.ac.jp" },
	{ 1,	"WIN Internet Service",		"ftp.win.ne.jp" },
	{ 1,	"Microsoft",				"ftp.microsoft.com" }
};




