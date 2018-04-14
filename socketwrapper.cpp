// socketwrapper.c
// Copyright (C) 2011 Suguru Kawamoto

#include "common.h"

extern bool SupportIdn;


// IPv6対応

const struct in6_addr IN6ADDR_NONE = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

typedef struct
{
	HANDLE h;
	HWND hWnd;
	u_int wMsg;
	char * name;
	char * buf;
	int buflen;
	short Family;
} GETHOSTBYNAMEDATA;

DWORD WINAPI WSAAsyncGetHostByNameIPv6ThreadProc(LPVOID lpParameter)
{
	GETHOSTBYNAMEDATA* pData;
	struct hostent* pHost;
	struct addrinfo* pAddr;
	struct addrinfo* p;
	pHost = NULL;
	pData = (GETHOSTBYNAMEDATA*)lpParameter;
	if(getaddrinfo(pData->name, NULL, NULL, &pAddr) == 0)
	{
		p = pAddr;
		while(p)
		{
			if(p->ai_family == pData->Family)
			{
				switch(p->ai_family)
				{
				case AF_INET:
					pHost = (struct hostent*)pData->buf;
					if((size_t)pData->buflen >= sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in_addr)
						&& p->ai_addrlen >= sizeof(struct sockaddr_in))
					{
						pHost->h_name = NULL;
						pHost->h_aliases = NULL;
						pHost->h_addrtype = p->ai_family;
						pHost->h_length = sizeof(struct in_addr);
						pHost->h_addr_list = (char**)(&pHost[1]);
						pHost->h_addr_list[0] = (char*)(&pHost->h_addr_list[2]);
						pHost->h_addr_list[1] = NULL;
						memcpy(pHost->h_addr_list[0], &((struct sockaddr_in*)p->ai_addr)->sin_addr, sizeof(struct in_addr));
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in_addr)));
					}
					else
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(WSAENOBUFS << 16));
					break;
				case AF_INET6:
					pHost = (struct hostent*)pData->buf;
					if((size_t)pData->buflen >= sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in6_addr)
						&& p->ai_addrlen >= sizeof(struct sockaddr_in6))
					{
						pHost->h_name = NULL;
						pHost->h_aliases = NULL;
						pHost->h_addrtype = p->ai_family;
						pHost->h_length = sizeof(struct in6_addr);
						pHost->h_addr_list = (char**)(&pHost[1]);
						pHost->h_addr_list[0] = (char*)(&pHost->h_addr_list[2]);
						pHost->h_addr_list[1] = NULL;
						memcpy(pHost->h_addr_list[0], &((struct sockaddr_in6*)p->ai_addr)->sin6_addr, sizeof(struct in6_addr));
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in6_addr)));
					}
					else
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(WSAENOBUFS << 16));
					break;
				}
			}
			if(pHost)
				break;
			p = p->ai_next;
		}
		if(!p)
			PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(ERROR_INVALID_FUNCTION << 16));
		freeaddrinfo(pAddr);
	}
	else
		PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(ERROR_INVALID_FUNCTION << 16));
	// CreateThreadが返すハンドルが重複するのを回避
	Sleep(10000);
	CloseHandle(pData->h);
	free(pData->name);
	free(pData);
	return 0;
}

// IPv6対応のWSAAsyncGetHostByName相当の関数
// FamilyにはAF_INETまたはAF_INET6を指定可能
// ただしANSI用
HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family)
{
	HANDLE hResult = nullptr;
	GETHOSTBYNAMEDATA Data;
	Data.hWnd = hWnd;
	Data.wMsg = wMsg;
	if(Data.name = (char*)malloc(sizeof(char) * (strlen(name) + 1)))
	{
		strcpy(Data.name, name);
		Data.buf = buf;
		Data.buflen = buflen;
		Data.Family = Family;
		if(Data.h = CreateThread(NULL, 0, WSAAsyncGetHostByNameIPv6ThreadProc, &Data, CREATE_SUSPENDED, NULL))
		{
			ResumeThread(Data.h);
			hResult = Data.h;
		}
	}
	if(!hResult) free(Data.name);
	return hResult;
}

// WSAAsyncGetHostByNameIPv6用のWSACancelAsyncRequest相当の関数
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle)
{
	int Result;
	Result = SOCKET_ERROR;
	if(TerminateThread(hAsyncTaskHandle, 0))
	{
		CloseHandle(hAsyncTaskHandle);
		Result = 0;
	}
	return Result;
}

char* AddressToStringIPv4(char* str, void* in)
{
	char* pResult;
	unsigned char* p;
	pResult = str;
	p = (unsigned char*)in;
	sprintf(str, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
	return pResult;
}

char* AddressToStringIPv6(char* str, void* in6)
{
	char* pResult;
	unsigned char* p;
	int MaxZero;
	int MaxZeroLen;
	int i;
	int j;
	char Tmp[5];
	pResult = str;
	p = (unsigned char*)in6;
	MaxZero = 8;
	MaxZeroLen = 1;
	for(i = 0; i < 8; i++)
	{
		for(j = i; j < 8; j++)
		{
			if(p[j * 2] != 0 || p[j * 2 + 1] != 0)
				break;
		}
		if(j - i > MaxZeroLen)
		{
			MaxZero = i;
			MaxZeroLen = j - i;
		}
	}
	strcpy(str, "");
	for(i = 0; i < 8; i++)
	{
		if(i == MaxZero)
		{
			if(i == 0)
				strcat(str, ":");
			strcat(str, ":");
		}
		else if(i < MaxZero || i >= MaxZero + MaxZeroLen)
		{
			sprintf(Tmp, "%x", (((int)p[i * 2] & 0xff) << 8) | ((int)p[i * 2 + 1] & 0xff));
			strcat(str, Tmp);
			if(i < 7)
				strcat(str, ":");
		}
	}
	return pResult;
}

// IPv6対応のinet_ntoa相当の関数
// ただしANSI用
char* inet6_ntoa(struct in6_addr in6)
{
	char* pResult;
	static char Adrs[40];
	pResult = NULL;
	memset(Adrs, 0, sizeof(Adrs));
	pResult = AddressToStringIPv6(Adrs, &in6);
	return pResult;
}

// IPv6対応のinet_addr相当の関数
// ただしANSI用
struct in6_addr inet6_addr(const char* cp)
{
	struct in6_addr Result;
	int AfterZero;
	int i;
	char* p;
	memset(&Result, 0, sizeof(Result));
	AfterZero = 0;
	for(i = 0; i < 8; i++)
	{
		if(!cp)
		{
			memcpy(&Result, &IN6ADDR_NONE, sizeof(struct in6_addr));
			break;
		}
		if(i >= AfterZero)
		{
			if(strncmp(cp, ":", 1) == 0)
			{
				cp = cp + 1;
				if(i == 0 && strncmp(cp, ":", 1) == 0)
					cp = cp + 1;
				p = (char*)cp;
				AfterZero = 7;
				while(p = strstr(p, ":"))
				{
					p = p + 1;
					AfterZero--;
				}
			}
			else
			{
				Result.u.Word[i] = _byteswap_ushort((USHORT)strtol(cp, &p, 16));
				if(strncmp(p, ":", 1) != 0 && strlen(p) > 0)
				{
					memcpy(&Result, &IN6ADDR_NONE, sizeof(struct in6_addr));
					break;
				}
				if(cp = strstr(cp, ":"))
					cp = cp + 1;
			}
		}
	}
	return Result;
}

BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input) {
	static std::regex re{ R"(^(?:[A-Za-z0-9\-.])+$)" };
	if (std::regex_match(Input, re)) {
		strcpy(Output, Input);
		return TRUE;
	}
	if (!SupportIdn)
		return FALSE;
	auto const winput = u8(Input);
	auto length = IdnToAscii(0, data(winput), size_as<int>(winput), nullptr, 0);
	if (length == 0)
		return FALSE;
	std::wstring wbuffer(length, L'\0');
	auto result = IdnToAscii(0, data(winput), size_as<int>(winput), data(wbuffer), length);
	assert(result == length);
	strcpy(Output, u8(wbuffer).c_str());
	return TRUE;
}

HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen)
{
	HANDLE r = NULL;
	char* pa0 = NULL;
	if(pa0 = AllocateStringA((int)strlen(name) * 4))
	{
		if(ConvertNameToPunycode(pa0, name))
			r = WSAAsyncGetHostByName(hWnd, wMsg, pa0, buf, buflen);
	}
	FreeDuplicatedString(pa0);
	return r;
}

HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family)
{
	HANDLE r = NULL;
	char* pa0 = NULL;
	if(pa0 = AllocateStringA((int)strlen(name) * 4))
	{
		if(ConvertNameToPunycode(pa0, name))
			r = WSAAsyncGetHostByNameIPv6(hWnd, wMsg, pa0, buf, buflen, Family);
	}
	FreeDuplicatedString(pa0);
	return r;
}

