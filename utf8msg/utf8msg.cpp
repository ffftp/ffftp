// utf8msg.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"


int EncodeText(char* pOut, const char* pIn, int Count)
{
	const char* pInEnd;
	char* pOutBegin;
	bool bEncoded;
	pInEnd = pIn + Count;
	pOutBegin = pOut;
	bEncoded = false;
	while(pIn < pInEnd)
	{
		switch(*pIn)
		{
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
			if(bEncoded)
			{
				sprintf(pOut, "\\x%02X", (unsigned char)*pIn);
				pOut += 4;
			}
			else
			{
				*pOut = *pIn;
				pOut++;
			}
			break;
		default:
			if(*pIn & 0x80)
			{
				bEncoded = true;
				sprintf(pOut, "\\x%02X", (unsigned char)*pIn);
				pOut += 4;
			}
			else
			{
				bEncoded = false;
				*pOut = *pIn;
				pOut++;
			}
			break;
		}
		pIn++;
	}
	return (int)((size_t)pOut - (size_t)pOutBegin);
}

char* FindQuote(const char* pIn, int Count)
{
	const char* pInEnd;
	bool bEscape;
	pInEnd = pIn + Count;
	bEscape = false;
	while(pIn < pInEnd)
	{
		switch(*pIn)
		{
		case '\\':
			bEscape = !bEscape;
			break;
		case '\"':
			if(!bEscape)
				return (char*)pIn;
			bEscape = false;
			break;
		default:
			bEscape = false;
			break;
		}
		pIn++;
	}
	return NULL;
}

char* FindEndOfLine(const char* pIn, int Count)
{
	const char* pInEnd;
	pInEnd = pIn + Count;
	while(pIn < pInEnd)
	{
		switch(*pIn)
		{
		case '\r':
		case '\n':
			if(pIn < pInEnd && *pIn == '\r')
				pIn++;
			if(pIn < pInEnd && *pIn == '\n')
				pIn++;
			return (char*)pIn;
			break;
		}
		pIn++;
	}
	return (char*)pIn;
}

int _tmain(int argc, _TCHAR* argv[])
{
	FILE* fpIn;
	FILE* fpOut;
	long InLength;
	long OutLength;
	char* pInBuffer;
	char* pOutBuffer;
	char* pIn;
	char* pOut;
	char* pInEnd;
	char* pOutEnd;
	int Count;
	bool bString;
	char* pInNext;
	char* pInQuote;
	char* pSrc;
	int SrcCount;
	_tsetlocale(LC_ALL, _T(""));
	if(argc != 3)
	{
		_tprintf(_T("Encode string literals in C language source files written in UTF-8.\n"));
		_tprintf(_T("The first string literal in each line will be retained.\n"));
		_tprintf(_T("The other string literals in each line will be encoded from the first one.\n"));
		_tprintf(_T("Command line\n"));
		_tprintf(_T("utf8msg.exe [in] [out]\n"));
		_tprintf(_T("[in]    Original source file name.\n"));
		_tprintf(_T("[out]   File name to save.\n"));
		return 0;
	}
	fpIn = _tfopen(argv[1], _T("rb"));
	if(!fpIn)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	if(fseek(fpIn, 0, SEEK_END) != 0)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	InLength = ftell(fpIn);
	if(fseek(fpIn, 0, SEEK_SET) != 0)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	pInBuffer = (char*)malloc(InLength);
	if(!pInBuffer)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	if((long)fread(pInBuffer, 1, InLength, fpIn) != InLength)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	OutLength = InLength * 4;
	pOutBuffer = (char*)malloc(OutLength);
	if(!pOutBuffer)
	{
		_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[1]);
		return 0;
	}
	pIn = pInBuffer;
	pOut = pOutBuffer;
	pInEnd = pInBuffer + InLength;
	pOutEnd = pOutBuffer + OutLength;
	while(pIn < pInEnd)
	{
		bString = false;
		pSrc = NULL;
		Count = (int)((size_t)pInEnd - (size_t)pIn);
		pInNext = FindEndOfLine(pIn, Count);
		while(pInQuote = FindQuote(pIn, (int)((size_t)pInNext - (size_t)pIn)))
		{
			if(bString)
			{
				bString = false;
				if(pSrc)
				{
					pIn = pInQuote + 1;
					pOut += EncodeText(pOut, pSrc, SrcCount);
				}
				else
				{
					pSrc = pIn;
					SrcCount = (int)((size_t)pInQuote - (size_t)pIn) + 1;
					Count = (int)((size_t)pInQuote - (size_t)pIn) + 1;
					memcpy(pOut, pIn, Count);
					pIn += Count;
					pOut += Count;
				}
			}
			else
			{
				bString = true;
				Count = (int)((size_t)pInQuote - (size_t)pIn) + 1;
				memcpy(pOut, pIn, Count);
				pIn += Count;
				pOut += Count;
			}
		}
		Count = (int)((size_t)pInNext - (size_t)pIn);
		memcpy(pOut, pIn, Count);
		pIn += Count;
		pOut += Count;
	}
	OutLength = (long)((size_t)pOut - (size_t)pOutBuffer);
	fpIn = _tfopen(argv[2], _T("rb"));
	if(fpIn)
	{
		if(fseek(fpIn, 0, SEEK_END) != 0)
		{
			_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[2]);
			return 0;
		}
		InLength = ftell(fpIn);
		if(fseek(fpIn, 0, SEEK_SET) != 0)
		{
			_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[2]);
			return 0;
		}
		pInBuffer = (char*)malloc(InLength);
		if(!pInBuffer)
		{
			_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[2]);
			return 0;
		}
		if((long)fread(pInBuffer, 1, InLength, fpIn) != InLength)
		{
			_tprintf(_T("File \"%s\" cannot be opened.\n"), argv[2]);
			return 0;
		}
		if(InLength == OutLength && memcmp(pInBuffer, pOutBuffer, InLength) == 0)
		{
			_tprintf(_T("There is no need to convert file \"%s\".\n"), argv[2]);
			return 0;
		}
	}
	fpOut = _tfopen(argv[2], _T("wb"));
	if(!fpOut)
	{
		_tprintf(_T("File \"%s\" cannot be created.\n"), argv[2]);
		return 0;
	}
	if((long)fwrite(pOutBuffer, 1, OutLength, fpOut) != OutLength)
	{
		_tprintf(_T("File \"%s\" cannot be created.\n"), argv[2]);
		return 0;
	}
	_tprintf(_T("File \"%s\" was successfully converted.\n"), argv[2]);
	return 0;
}

