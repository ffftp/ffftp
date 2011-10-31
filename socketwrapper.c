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
typedef X509* (__cdecl* _SSL_get_peer_certificate)(const SSL*);
typedef long (__cdecl* _SSL_get_verify_result)(const SSL*);
typedef SSL_SESSION* (__cdecl* _SSL_get_session)(SSL*);
typedef int (__cdecl* _SSL_set_session)(SSL*, SSL_SESSION*);
typedef BIO_METHOD* (__cdecl* _BIO_s_mem)();
typedef BIO* (__cdecl* _BIO_new)(BIO_METHOD*);
typedef int (__cdecl* _BIO_free)(BIO*);
typedef long (__cdecl* _BIO_ctrl)(BIO*, int, long, void*);
typedef void (__cdecl* _X509_free)(X509*);
typedef int (__cdecl* _X509_print_ex)(BIO*, X509*, unsigned long, unsigned long);
typedef X509_NAME* (__cdecl* _X509_get_subject_name)(X509*);
typedef int (__cdecl* _X509_NAME_print_ex)(BIO*, X509_NAME*, int, unsigned long);

_SSL_load_error_strings p_SSL_load_error_strings;
_SSL_library_init p_SSL_library_init;
_SSLv23_method p_SSLv23_method;
_SSL_CTX_new p_SSL_CTX_new;
_SSL_CTX_free p_SSL_CTX_free;
_SSL_new p_SSL_new;
_SSL_free p_SSL_free;
_SSL_shutdown p_SSL_shutdown;
_SSL_get_fd p_SSL_get_fd;
_SSL_set_fd p_SSL_set_fd;
_SSL_accept p_SSL_accept;
_SSL_connect p_SSL_connect;
_SSL_write p_SSL_write;
_SSL_peek p_SSL_peek;
_SSL_read p_SSL_read;
_SSL_get_error p_SSL_get_error;
_SSL_get_peer_certificate p_SSL_get_peer_certificate;
_SSL_get_verify_result p_SSL_get_verify_result;
_SSL_get_session p_SSL_get_session;
_SSL_set_session p_SSL_set_session;
_BIO_s_mem p_BIO_s_mem;
_BIO_new p_BIO_new;
_BIO_free p_BIO_free;
_BIO_ctrl p_BIO_ctrl;
_X509_free p_X509_free;
_X509_print_ex p_X509_print_ex;
_X509_get_subject_name p_X509_get_subject_name;
_X509_NAME_print_ex p_X509_NAME_print_ex;

#define MAX_SSL_SOCKET 64

BOOL g_bOpenSSLLoaded;
HMODULE g_hOpenSSL;
HMODULE g_hOpenSSLCommon;
CRITICAL_SECTION g_OpenSSLLock;
DWORD g_OpenSSLTimeout;
LPSSLTIMEOUTCALLBACK g_pOpenSSLTimeoutCallback;
LPSSLCONFIRMCALLBACK g_pOpenSSLConfirmCallback;
SSL_CTX* g_pOpenSSLCTX;
SSL* g_pOpenSSLHandle[MAX_SSL_SOCKET];

BOOL __stdcall DefaultSSLTimeoutCallback(BOOL* pbAborted)
{
	Sleep(100);
	return *pbAborted;
}

BOOL __stdcall DefaultSSLConfirmCallback(BOOL* pbAborted, BOOL bVerified, LPCSTR Certificate, LPCSTR CommonName)
{
	return bVerified;
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
		|| !(p_SSL_load_error_strings = (_SSL_load_error_strings)GetProcAddress(g_hOpenSSL, "SSL_load_error_strings"))
		|| !(p_SSL_library_init = (_SSL_library_init)GetProcAddress(g_hOpenSSL, "SSL_library_init"))
		|| !(p_SSLv23_method = (_SSLv23_method)GetProcAddress(g_hOpenSSL, "SSLv23_method"))
		|| !(p_SSL_CTX_new = (_SSL_CTX_new)GetProcAddress(g_hOpenSSL, "SSL_CTX_new"))
		|| !(p_SSL_CTX_free = (_SSL_CTX_free)GetProcAddress(g_hOpenSSL, "SSL_CTX_free"))
		|| !(p_SSL_new = (_SSL_new)GetProcAddress(g_hOpenSSL, "SSL_new"))
		|| !(p_SSL_free = (_SSL_free)GetProcAddress(g_hOpenSSL, "SSL_free"))
		|| !(p_SSL_shutdown = (_SSL_shutdown)GetProcAddress(g_hOpenSSL, "SSL_shutdown"))
		|| !(p_SSL_get_fd = (_SSL_get_fd)GetProcAddress(g_hOpenSSL, "SSL_get_fd"))
		|| !(p_SSL_set_fd = (_SSL_set_fd)GetProcAddress(g_hOpenSSL, "SSL_set_fd"))
		|| !(p_SSL_accept = (_SSL_accept)GetProcAddress(g_hOpenSSL, "SSL_accept"))
		|| !(p_SSL_connect = (_SSL_connect)GetProcAddress(g_hOpenSSL, "SSL_connect"))
		|| !(p_SSL_write = (_SSL_write)GetProcAddress(g_hOpenSSL, "SSL_write"))
		|| !(p_SSL_peek = (_SSL_peek)GetProcAddress(g_hOpenSSL, "SSL_peek"))
		|| !(p_SSL_read = (_SSL_read)GetProcAddress(g_hOpenSSL, "SSL_read"))
		|| !(p_SSL_get_error = (_SSL_get_error)GetProcAddress(g_hOpenSSL, "SSL_get_error"))
		|| !(p_SSL_get_peer_certificate = (_SSL_get_peer_certificate)GetProcAddress(g_hOpenSSL, "SSL_get_peer_certificate"))
		|| !(p_SSL_get_verify_result = (_SSL_get_verify_result)GetProcAddress(g_hOpenSSL, "SSL_get_verify_result"))
		|| !(p_SSL_get_session = (_SSL_get_session)GetProcAddress(g_hOpenSSL, "SSL_get_session"))
		|| !(p_SSL_set_session = (_SSL_set_session)GetProcAddress(g_hOpenSSL, "SSL_set_session")))
	{
		if(g_hOpenSSL)
			FreeLibrary(g_hOpenSSL);
		g_hOpenSSL = NULL;
		return FALSE;
	}
	g_hOpenSSLCommon = LoadLibrary("libeay32.dll");
	if(!g_hOpenSSLCommon
		|| !(p_BIO_s_mem = (_BIO_s_mem)GetProcAddress(g_hOpenSSLCommon, "BIO_s_mem"))
		|| !(p_BIO_new = (_BIO_new)GetProcAddress(g_hOpenSSLCommon, "BIO_new"))
		|| !(p_BIO_free = (_BIO_free)GetProcAddress(g_hOpenSSLCommon, "BIO_free"))
		|| !(p_BIO_ctrl = (_BIO_ctrl)GetProcAddress(g_hOpenSSLCommon, "BIO_ctrl"))
		|| !(p_X509_free = (_X509_free)GetProcAddress(g_hOpenSSLCommon, "X509_free"))
		|| !(p_X509_print_ex = (_X509_print_ex)GetProcAddress(g_hOpenSSLCommon, "X509_print_ex"))
		|| !(p_X509_get_subject_name = (_X509_get_subject_name)GetProcAddress(g_hOpenSSLCommon, "X509_get_subject_name"))
		|| !(p_X509_NAME_print_ex = (_X509_NAME_print_ex)GetProcAddress(g_hOpenSSLCommon, "X509_NAME_print_ex")))
	{
		if(g_hOpenSSL)
			FreeLibrary(g_hOpenSSL);
		g_hOpenSSL = NULL;
		if(g_hOpenSSLCommon)
			FreeLibrary(g_hOpenSSLCommon);
		g_hOpenSSLCommon = NULL;
		return FALSE;
	}
	InitializeCriticalSection(&g_OpenSSLLock);
	p_SSL_load_error_strings();
	p_SSL_library_init();
	SetSSLTimeoutCallback(60000, DefaultSSLTimeoutCallback);
	SetSSLConfirmCallback(DefaultSSLConfirmCallback);
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
			p_SSL_shutdown(g_pOpenSSLHandle[i]);
			p_SSL_free(g_pOpenSSLHandle[i]);
			g_pOpenSSLHandle[i] = NULL;
		}
	}
	if(g_pOpenSSLCTX)
		p_SSL_CTX_free(g_pOpenSSLCTX);
	g_pOpenSSLCTX = NULL;
	FreeLibrary(g_hOpenSSL);
	g_hOpenSSL = NULL;
	FreeLibrary(g_hOpenSSLCommon);
	g_hOpenSSLCommon = NULL;
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
			if(p_SSL_get_fd(g_pOpenSSLHandle[i]) == s)
				return &g_pOpenSSLHandle[i];
		}
	}
	return NULL;
}

BOOL ConfirmSSLCertificate(SSL* pSSL, BOOL* pbAborted)
{
	BOOL bResult;
	BOOL bVerified;
	char* pData;
	char* pSubject;
	X509* pX509;
	BIO* pBIO;
	long Length;
	char* pBuffer;
	char* pCN;
	char* p;
	bResult = FALSE;
	bVerified = FALSE;
	pData = NULL;
	pSubject = NULL;
	if(pX509 = p_SSL_get_peer_certificate(pSSL))
	{
		if(pBIO = p_BIO_new(p_BIO_s_mem()))
		{
			p_X509_print_ex(pBIO, pX509, 0, XN_FLAG_RFC2253);
			if((Length = p_BIO_ctrl(pBIO, BIO_CTRL_INFO, 0, &pBuffer)) > 0)
			{
				if(pData = (char*)malloc(Length + sizeof(char)))
				{
					memcpy(pData, pBuffer, Length);
					*(char*)((size_t)pData + Length) = '\0';
				}
			}
			p_BIO_free(pBIO);
		}
		if(pBIO = p_BIO_new(p_BIO_s_mem()))
		{
			p_X509_NAME_print_ex(pBIO, p_X509_get_subject_name(pX509), 0, XN_FLAG_RFC2253);
			if((Length = p_BIO_ctrl(pBIO, BIO_CTRL_INFO, 0, &pBuffer)) > 0)
			{
				if(pSubject = (char*)malloc(Length + sizeof(char)))
				{
					memcpy(pSubject, pBuffer, Length);
					*(char*)((size_t)pSubject + Length) = '\0';
				}
			}
			p_BIO_free(pBIO);
		}
		p_X509_free(pX509);
	}
	if(p_SSL_get_verify_result(pSSL) == X509_V_OK)
		bVerified = TRUE;
	pCN = pSubject;
	while(pCN)
	{
		if(strncmp(pCN, "CN=", strlen("CN=")) == 0)
		{
			pCN += strlen("CN=");
			if(p = strchr(pCN, ','))
				*p = '\0';
			break;
		}
		if(pCN = strchr(pCN, ','))
			pCN++;
	}
	bResult = g_pOpenSSLConfirmCallback(pbAborted, bVerified, pData, pCN);
	if(pData)
		free(pData);
	if(pSubject)
		free(pSubject);
	return bResult;
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

void SetSSLConfirmCallback(LPSSLCONFIRMCALLBACK pCallback)
{
	if(!g_bOpenSSLLoaded)
		return;
	EnterCriticalSection(&g_OpenSSLLock);
	g_pOpenSSLConfirmCallback = pCallback;
	LeaveCriticalSection(&g_OpenSSLLock);
}

BOOL IsHostNameMatched(LPCSTR HostName, LPCSTR CommonName)
{
	BOOL bResult;
	char* pAsterisk;
	bResult = FALSE;
	if(HostName && CommonName)
	{
		if(pAsterisk = strchr(CommonName, '*'))
		{
			if(_strnicmp(HostName, CommonName, ((size_t)pAsterisk - (size_t)CommonName) / sizeof(char)) == 0)
			{
				while(*pAsterisk == '*')
				{
					pAsterisk++;
				}
				if(_stricmp(HostName + strlen(HostName) - strlen(pAsterisk), pAsterisk) == 0)
					bResult = TRUE;
			}
		}
		else if(_stricmp(HostName, CommonName) == 0)
			bResult = TRUE;
	}
	return bResult;
}

BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted)
{
	BOOL r;
	DWORD Time;
	SSL** ppSSL;
	SSL** ppSSLParent;
	SSL_SESSION* pSession;
	int Return;
	int Error;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	r = FALSE;
	Time = timeGetTime();
	EnterCriticalSection(&g_OpenSSLLock);
	if(!g_pOpenSSLCTX)
		g_pOpenSSLCTX = p_SSL_CTX_new(p_SSLv23_method());
	if(g_pOpenSSLCTX)
	{
		if(ppSSL = GetUnusedSSLPointer())
		{
			if(*ppSSL = p_SSL_new(g_pOpenSSLCTX))
			{
				if(p_SSL_set_fd(*ppSSL, s) != 0)
				{
					if(parent != INVALID_SOCKET)
					{
						if(ppSSLParent = FindSSLPointerFromSocket(parent))
						{
							if(pSession = p_SSL_get_session(*ppSSLParent))
							{
								if(p_SSL_set_session(*ppSSL, pSession) == 1)
								{
								}
							}
						}
					}
					// SSLのネゴシエーションには時間がかかる場合がある
					r = TRUE;
					while(r)
					{
						Return = p_SSL_connect(*ppSSL);
						if(Return == 1)
							break;
						Error = p_SSL_get_error(*ppSSL, Return);
						if(Error == SSL_ERROR_WANT_READ || Error == SSL_ERROR_WANT_WRITE)
						{
							LeaveCriticalSection(&g_OpenSSLLock);
							if(g_pOpenSSLTimeoutCallback(pbAborted) || (g_OpenSSLTimeout > 0 && timeGetTime() - Time >= g_OpenSSLTimeout))
								r = FALSE;
							EnterCriticalSection(&g_OpenSSLLock);
						}
						else
							r = FALSE;
					}
					if(r)
					{
						if(ConfirmSSLCertificate(*ppSSL, pbAborted))
						{
						}
						else
						{
							LeaveCriticalSection(&g_OpenSSLLock);
							DetachSSL(s);
							r = FALSE;
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
		p_SSL_shutdown(*ppSSL);
		p_SSL_free(*ppSSL);
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
		return FALSE;
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
	BOOL bAborted;
	r = accept(s, addr, addrlen);
	bAborted = FALSE;
	if(!AttachSSL(r, INVALID_SOCKET, &bAborted))
	{
		closesocket(r);
		return INVALID_SOCKET;
	}
	return r;
}

int connectS(SOCKET s, const struct sockaddr *name, int namelen)
{
	int r;
	BOOL bAborted;
	r = connect(s, name, namelen);
	bAborted = FALSE;
	if(!AttachSSL(r, INVALID_SOCKET, &bAborted))
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
	return p_SSL_write(*ppSSL, buf, len);
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
		return p_SSL_peek(*ppSSL, buf, len);
	return p_SSL_read(*ppSSL, buf, len);
}

