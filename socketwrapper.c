// socketwrapper.c
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー
// socket関連関数をOpenSSL用に置換
// コンパイルにはOpenSSLのヘッダーファイルが必要
// 実行にはOpenSSLのDLLが必要

#include <windows.h>
#include <mmsystem.h>
#include <openssl/ssl.h>

#include "socketwrapper.h"
#include "protectprocess.h"

typedef void (__cdecl* _SSL_load_error_strings)();
typedef int (__cdecl* _SSL_library_init)();
typedef SSL_METHOD* (__cdecl* _SSLv23_method)();
typedef SSL_CTX* (__cdecl* _SSL_CTX_new)(SSL_METHOD*);
typedef void (__cdecl* _SSL_CTX_free)(SSL_CTX*);
typedef SSL* (__cdecl* _SSL_new)(SSL_CTX*);
typedef void (__cdecl* _SSL_free)(SSL*);
typedef int (__cdecl* _SSL_shutdown)(SSL*);
typedef int (__cdecl* _SSL_get_fd)(SSL*);
typedef int (__cdecl* _SSL_set_fd)(SSL*, int);
typedef int (__cdecl* _SSL_accept)(SSL*);
typedef int (__cdecl* _SSL_connect)(SSL*);
typedef int (__cdecl* _SSL_write)(SSL*, const void*, int);
typedef int (__cdecl* _SSL_peek)(SSL*, void*, int);
typedef int (__cdecl* _SSL_read)(SSL*, void*, int);
typedef int (__cdecl* _SSL_get_error)(SSL*, int);

_SSL_load_error_strings pSSL_load_error_strings;
_SSL_library_init pSSL_library_init;
_SSLv23_method pSSLv23_method;
_SSL_CTX_new pSSL_CTX_new;
_SSL_CTX_free pSSL_CTX_free;
_SSL_new pSSL_new;
_SSL_free pSSL_free;
_SSL_shutdown pSSL_shutdown;
_SSL_get_fd pSSL_get_fd;
_SSL_set_fd pSSL_set_fd;
_SSL_accept pSSL_accept;
_SSL_connect pSSL_connect;
_SSL_write pSSL_write;
_SSL_peek pSSL_peek;
_SSL_read pSSL_read;
_SSL_get_error pSSL_get_error;

#define MAX_SSL_SOCKET 64

BOOL g_bOpenSSLLoaded;
HMODULE g_hOpenSSL;
CRITICAL_SECTION g_OpenSSLLock;
DWORD g_OpenSSLTimeout;
LPSSLTIMEOUTCALLBACK g_pOpenSSLTimeoutCallback;
SSL_CTX* g_pOpenSSLCTX;
SSL* g_pOpenSSLHandle[MAX_SSL_SOCKET];

BOOL __stdcall DefaultSSLTimeoutCallback()
{
	Sleep(100);
	return FALSE;
}

BOOL LoadOpenSSL()
{
	if(g_bOpenSSLLoaded)
		return FALSE;
#ifdef ENABLE_PROCESS_PROTECTION
	// 同梱するOpenSSLのバージョンに合わせてSHA1ハッシュ値を変更すること
	// ssleay32.dll 1.0.0e
	// libssl32.dll 1.0.0e
	RegisterTrustedModuleSHA1Hash("\x4E\xB7\xA0\x22\x14\x4B\x58\x6D\xBC\xF5\x21\x0D\x96\x78\x0D\x79\x7D\x66\xB2\xB0");
	// libeay32.dll 1.0.0e
	RegisterTrustedModuleSHA1Hash("\x01\x32\x7A\xAE\x69\x26\xE6\x58\xC7\x63\x22\x1E\x53\x5A\x78\xBC\x61\xC7\xB5\xC1");
#endif
	g_hOpenSSL = LoadLibrary("ssleay32.dll");
	if(!g_hOpenSSL)
		g_hOpenSSL = LoadLibrary("libssl32.dll");
	if(!g_hOpenSSL
		|| !(pSSL_load_error_strings = (_SSL_load_error_strings)GetProcAddress(g_hOpenSSL, "SSL_load_error_strings"))
		|| !(pSSL_library_init = (_SSL_library_init)GetProcAddress(g_hOpenSSL, "SSL_library_init"))
		|| !(pSSLv23_method = (_SSLv23_method)GetProcAddress(g_hOpenSSL, "SSLv23_method"))
		|| !(pSSL_CTX_new = (_SSL_CTX_new)GetProcAddress(g_hOpenSSL, "SSL_CTX_new"))
		|| !(pSSL_CTX_free = (_SSL_CTX_free)GetProcAddress(g_hOpenSSL, "SSL_CTX_free"))
		|| !(pSSL_new = (_SSL_new)GetProcAddress(g_hOpenSSL, "SSL_new"))
		|| !(pSSL_free = (_SSL_free)GetProcAddress(g_hOpenSSL, "SSL_free"))
		|| !(pSSL_shutdown = (_SSL_shutdown)GetProcAddress(g_hOpenSSL, "SSL_shutdown"))
		|| !(pSSL_get_fd = (_SSL_get_fd)GetProcAddress(g_hOpenSSL, "SSL_get_fd"))
		|| !(pSSL_set_fd = (_SSL_set_fd)GetProcAddress(g_hOpenSSL, "SSL_set_fd"))
		|| !(pSSL_accept = (_SSL_accept)GetProcAddress(g_hOpenSSL, "SSL_accept"))
		|| !(pSSL_connect = (_SSL_connect)GetProcAddress(g_hOpenSSL, "SSL_connect"))
		|| !(pSSL_write = (_SSL_write)GetProcAddress(g_hOpenSSL, "SSL_write"))
		|| !(pSSL_peek = (_SSL_peek)GetProcAddress(g_hOpenSSL, "SSL_peek"))
		|| !(pSSL_read = (_SSL_read)GetProcAddress(g_hOpenSSL, "SSL_read"))
		|| !(pSSL_get_error = (_SSL_get_error)GetProcAddress(g_hOpenSSL, "SSL_get_error")))
	{
		if(g_hOpenSSL)
			FreeLibrary(g_hOpenSSL);
		g_hOpenSSL = NULL;
		return FALSE;
	}
	InitializeCriticalSection(&g_OpenSSLLock);
	pSSL_load_error_strings();
	pSSL_library_init();
	SetSSLTimeoutCallback(60000, DefaultSSLTimeoutCallback);
	g_bOpenSSLLoaded = TRUE;
	return TRUE;
}

void FreeOpenSSL()
{
	int i;
	if(!g_bOpenSSLLoaded)
		return;
	EnterCriticalSection(&g_OpenSSLLock);
	for(i = 0; i < MAX_SSL_SOCKET; i++)
	{
		if(g_pOpenSSLHandle[i])
		{
			pSSL_shutdown(g_pOpenSSLHandle[i]);
			pSSL_free(g_pOpenSSLHandle[i]);
			g_pOpenSSLHandle[i] = NULL;
		}
	}
	if(g_pOpenSSLCTX)
		pSSL_CTX_free(g_pOpenSSLCTX);
	g_pOpenSSLCTX = NULL;
	FreeLibrary(g_hOpenSSL);
	g_hOpenSSL = NULL;
	LeaveCriticalSection(&g_OpenSSLLock);
	DeleteCriticalSection(&g_OpenSSLLock);
	g_bOpenSSLLoaded = FALSE;
}

BOOL IsOpenSSLLoaded()
{
	return g_bOpenSSLLoaded;
}

SSL** GetUnusedSSLPointer()
{
	int i;
	for(i = 0; i < MAX_SSL_SOCKET; i++)
	{
		if(!g_pOpenSSLHandle[i])
			return &g_pOpenSSLHandle[i];
	}
	return NULL;
}

SSL** FindSSLPointerFromSocket(SOCKET s)
{
	int i;
	for(i = 0; i < MAX_SSL_SOCKET; i++)
	{
		if(g_pOpenSSLHandle[i])
		{
			if(pSSL_get_fd(g_pOpenSSLHandle[i]) == s)
				return &g_pOpenSSLHandle[i];
		}
	}
	return NULL;
}

void SetSSLTimeoutCallback(DWORD Timeout, LPSSLTIMEOUTCALLBACK pCallback)
{
	if(!g_bOpenSSLLoaded)
		return;
	EnterCriticalSection(&g_OpenSSLLock);
	g_OpenSSLTimeout = Timeout;
	g_pOpenSSLTimeoutCallback = pCallback;
	LeaveCriticalSection(&g_OpenSSLLock);
}

BOOL AttachSSL(SOCKET s)
{
	BOOL r;
	DWORD Time;
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	r = FALSE;
	Time = timeGetTime();
	EnterCriticalSection(&g_OpenSSLLock);
	if(!g_pOpenSSLCTX)
		g_pOpenSSLCTX = pSSL_CTX_new(pSSLv23_method());
	if(g_pOpenSSLCTX)
	{
		if(ppSSL = GetUnusedSSLPointer())
		{
			if(*ppSSL = pSSL_new(g_pOpenSSLCTX))
			{
				if(pSSL_set_fd(*ppSSL, s) != 0)
				{
					r = TRUE;
					// SSLのネゴシエーションには時間がかかる場合がある
					while(pSSL_connect(*ppSSL) != 1)
					{
						LeaveCriticalSection(&g_OpenSSLLock);
						if(g_pOpenSSLTimeoutCallback() || timeGetTime() - Time >= g_OpenSSLTimeout)
						{
							DetachSSL(s);
							r = FALSE;
							EnterCriticalSection(&g_OpenSSLLock);
							break;
						}
						EnterCriticalSection(&g_OpenSSLLock);
					}
				}
				else
				{
					LeaveCriticalSection(&g_OpenSSLLock);
					DetachSSL(s);
					EnterCriticalSection(&g_OpenSSLLock);
				}
			}
		}
	}
	LeaveCriticalSection(&g_OpenSSLLock);
	return r;
}

BOOL DetachSSL(SOCKET s)
{
	BOOL r;
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	r = FALSE;
	EnterCriticalSection(&g_OpenSSLLock);
	if(ppSSL = FindSSLPointerFromSocket(s))
	{
		pSSL_shutdown(*ppSSL);
		pSSL_free(*ppSSL);
		*ppSSL = NULL;
		r = TRUE;
	}
	LeaveCriticalSection(&g_OpenSSLLock);
	return r;
}

BOOL IsSSLAttached(SOCKET s)
{
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	EnterCriticalSection(&g_OpenSSLLock);
	ppSSL = FindSSLPointerFromSocket(s);
	LeaveCriticalSection(&g_OpenSSLLock);
	if(!ppSSL)
		return TRUE;
	return TRUE;
}

SOCKET socketS(int af, int type, int protocol)
{
	return socket(af, type, protocol);
}

int bindS(SOCKET s, const struct sockaddr *addr, int namelen)
{
	return bind(s, addr, namelen);
}

int listenS(SOCKET s, int backlog)
{
	return listen(s, backlog);
}

SOCKET acceptS(SOCKET s, struct sockaddr *addr, int *addrlen)
{
	SOCKET r;
	r = accept(s, addr, addrlen);
	if(!AttachSSL(r))
	{
		closesocket(r);
		return INVALID_SOCKET;
	}
	return r;
}

int connectS(SOCKET s, const struct sockaddr *name, int namelen)
{
	int r;
	r = connect(s, name, namelen);
	if(!AttachSSL(r))
		return SOCKET_ERROR;
	return r;
}

int closesocketS(SOCKET s)
{
	DetachSSL(s);
	return closesocket(s);
}

int sendS(SOCKET s, const char * buf, int len, int flags)
{
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return send(s, buf, len, flags);
	EnterCriticalSection(&g_OpenSSLLock);
	ppSSL = FindSSLPointerFromSocket(s);
	LeaveCriticalSection(&g_OpenSSLLock);
	if(!ppSSL)
		return send(s, buf, len, flags);
	return pSSL_write(*ppSSL, buf, len);
}

int recvS(SOCKET s, char * buf, int len, int flags)
{
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return recv(s, buf, len, flags);
	EnterCriticalSection(&g_OpenSSLLock);
	ppSSL = FindSSLPointerFromSocket(s);
	LeaveCriticalSection(&g_OpenSSLLock);
	if(!ppSSL)
		return recv(s, buf, len, flags);
	if(flags & MSG_PEEK)
		return pSSL_peek(*ppSSL, buf, len);
	return pSSL_read(*ppSSL, buf, len);
}

