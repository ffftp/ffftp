// encutf8.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	FILE* fpIn;
	FILE* fpOut;
	fpos_t Length;
	int InLength;
	char* pInBuffer;
	int UTF16Length;
	wchar_t* pUTF16Buffer;
	int OutLength;
	char* pOutBuffer;
	_tsetlocale(LC_ALL, _T(""));
	if(argc != 3)
	{
		_tprintf(_T("マルチバイト文字（コードページ932またはShift JIS）で書かれたテキストファイルをUTF-8にエンコードします。\n"));
		_tprintf(_T("コマンドライン\n"));
		_tprintf(_T("encutf8 [in] [out]\n"));
		_tprintf(_T("[in]    元のソースファイルのファイル名\n"));
		_tprintf(_T("[out]   保存先のファイル名\n"));
		return 0;
	}
	fpIn = _tfopen(argv[1], _T("rb"));
	if(!fpIn)
	{
		_tprintf(_T("ファイル\"%s\"が開けません。\n"), argv[1]);
		return 0;
	}
	fseek(fpIn, 0, SEEK_END);
	fgetpos(fpIn, &Length);
	fseek(fpIn, 0, SEEK_SET);
	InLength = Length / sizeof(char);
	pInBuffer = new char[InLength];
	UTF16Length = InLength;
	pUTF16Buffer = new wchar_t[InLength];
	OutLength = InLength * 4;
	pOutBuffer = new char[OutLength];
	if(!pInBuffer || !pUTF16Buffer || !pOutBuffer)
	{
		_tprintf(_T("メモリが確保できません。\n"));
		return 0;
	}
	fread(pInBuffer, 1, InLength, fpIn);
	fclose(fpIn);
	fpOut = _tfopen(argv[2], _T("wb"));
	if(!fpIn)
	{
		_tprintf(_T("ファイル\"%s\"が作成できません。\n"), argv[2]);
		return 0;
	}
	fwrite("\xEF\xBB\xBF", 1, 3, fpOut);
	UTF16Length = MultiByteToWideChar(CP_ACP, 0, pInBuffer, InLength / sizeof(char), pUTF16Buffer, UTF16Length);
	OutLength = WideCharToMultiByte(CP_UTF8, 0, pUTF16Buffer, UTF16Length, pOutBuffer, OutLength / sizeof(char), NULL, NULL);
	fwrite(pOutBuffer, sizeof(char), OutLength, fpOut);
	fclose(fpOut);
	return 0;
}

