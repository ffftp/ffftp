// mbtoutf8.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	FILE* fpIn;
	FILE* fpOut;
	char InBuffer[16384];
	char OutBuffer[65536];
	int InPosition;
	int OutPosition;
	bool bEscape;
	bool bString;
	bool bEncoded;
	int Count;
	int UTF8Count;
	wchar_t UTF16Buffer[4];
	char UTF8Buffer[8];
	int i;
	_tsetlocale(LC_ALL, _T(""));
	if(argc != 3)
	{
		_tprintf(_T("マルチバイト文字（コードページ932またはShift JIS）で書かれたC言語ソースファイル内の文字列リテラルをUTF-8にエンコードします。\n"));
		_tprintf(_T("コマンドライン\n"));
		_tprintf(_T("mbtoutf8 [in] [out]\n"));
		_tprintf(_T("[in]    元のソースファイルのファイル名\n"));
		_tprintf(_T("[out]   保存先のファイル名\n"));
		return 0;
	}
	fpIn = _tfopen(argv[1], _T("rt"));
	if(!fpIn)
	{
		_tprintf(_T("ファイル\"%s\"が開けません。\n"), argv[1]);
		return 0;
	}
	fpOut = _tfopen(argv[2], _T("wt"));
	if(!fpIn)
	{
		_tprintf(_T("ファイル\"%s\"が作成できません。\n"), argv[2]);
		return 0;
	}
	while(fgets(InBuffer, sizeof(InBuffer) / sizeof(char), fpIn))
	{
		InPosition = 0;
		OutPosition = 0;
		bEscape = false;
		bString = false;
		bEncoded = false;
		while(InBuffer[InPosition])
		{
			Count = max(mblen(&InBuffer[InPosition], 4), 1);
			if(Count == 1)
			{
				switch(InBuffer[InPosition])
				{
				case '\\':
					bEscape = !bEscape;
					bEncoded = false;
					strncpy(&OutBuffer[OutPosition], &InBuffer[InPosition], Count);
					InPosition += Count;
					OutPosition += Count;
					break;
				case '\"':
					bEscape = false;
					bString = !bString;
					bEncoded = false;
					strncpy(&OutBuffer[OutPosition], &InBuffer[InPosition], Count);
					InPosition += Count;
					OutPosition += Count;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					bEscape = false;
					if(bEncoded)
					{
						for(i = 0; i < Count; i++)
						{
							sprintf(&OutBuffer[OutPosition], "\\x%02X", (unsigned char)InBuffer[InPosition]);
							InPosition++;
							OutPosition += 4;
						}
					}
					else
					{
						strncpy(&OutBuffer[OutPosition], &InBuffer[InPosition], Count);
						InPosition += Count;
						OutPosition += Count;
						break;
					}
					break;
				default:
					bEscape = false;
					bEncoded = false;
					strncpy(&OutBuffer[OutPosition], &InBuffer[InPosition], Count);
					InPosition += Count;
					OutPosition += Count;
					break;
				}
			}
			else
			{
				if(bString)
				{
					bEscape = false;
					bEncoded = true;
					UTF8Count = MultiByteToWideChar(CP_ACP, 0, &InBuffer[InPosition], Count, UTF16Buffer, sizeof(UTF16Buffer) / sizeof(wchar_t));
					UTF8Count = WideCharToMultiByte(CP_UTF8, 0, UTF16Buffer, UTF8Count, UTF8Buffer, sizeof(UTF8Buffer) / sizeof(char), NULL, NULL);
					InPosition += Count;
					for(i = 0; i < UTF8Count; i++)
					{
						sprintf(&OutBuffer[OutPosition], "\\x%02X", (unsigned char)UTF8Buffer[i]);
						OutPosition += 4;
					}
				}
				else
				{
					bEscape = false;
					bEncoded = false;
					strncpy(&OutBuffer[OutPosition], &InBuffer[InPosition], Count);
					InPosition += Count;
					OutPosition += Count;
				}
			}
		}
		OutBuffer[OutPosition] = '\0';
		fputs(OutBuffer, fpOut);
	}
	fclose(fpIn);
	fclose(fpOut);
	return 0;
}

