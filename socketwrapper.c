// socketwrapper.c
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー
// socket関連関数をOpenSSL用に置換
// socket関連関数のIPv6対応
// コンパイルにはOpenSSLのヘッダーファイルが必要
// 実行にはOpenSSLのDLLが必要

#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
#include <openssl/ssl.h>

#include "socketwrapper.h"
#include "protectprocess.h"
#include "mbswrapper.h"
#include "punycode.h"

// FTPS対応

typedef void (__cdecl* _SSL_load_error_strings)();
typedef int (__cdecl* _SSL_library_init)();
typedef SSL_METHOD* (__cdecl* _SSLv23_method)();
typedef SSL* (__cdecl* _SSL_new)(SSL_CTX*);
typedef void (__cdecl* _SSL_free)(SSL*);
typedef long (__cdecl* _SSL_ctrl)(SSL*, int, long, void*);
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
typedef int (__cdecl* _SSL_set_cipher_list)(SSL*, const char*);
typedef SSL_CTX* (__cdecl* _SSL_CTX_new)(SSL_METHOD*);
typedef void (__cdecl* _SSL_CTX_free)(SSL_CTX*);
typedef X509_STORE* (__cdecl* _SSL_CTX_get_cert_store)(const SSL_CTX*);
typedef long (__cdecl* _SSL_CTX_ctrl)(SSL_CTX*, int, long, void*);
typedef BIO_METHOD* (__cdecl* _BIO_s_mem)();
typedef BIO* (__cdecl* _BIO_new)(BIO_METHOD*);
typedef int (__cdecl* _BIO_free)(BIO*);
typedef BIO* (__cdecl* _BIO_new_mem_buf)(void*, int);
typedef long (__cdecl* _BIO_ctrl)(BIO*, int, long, void*);
typedef void (__cdecl* _X509_free)(X509*);
typedef int (__cdecl* _X509_print_ex)(BIO*, X509*, unsigned long, unsigned long);
typedef X509_NAME* (__cdecl* _X509_get_subject_name)(X509*);
typedef int (__cdecl* _X509_NAME_print_ex)(BIO*, X509_NAME*, int, unsigned long);
typedef void (__cdecl* _X509_CRL_free)(X509_CRL*);
typedef EVP_PKEY* (__cdecl* _PEM_read_bio_PrivateKey)(BIO*, EVP_PKEY**, pem_password_cb*, void*);
typedef EVP_PKEY* (__cdecl* _PEM_read_bio_PUBKEY)(BIO*, EVP_PKEY**, pem_password_cb*, void*);
typedef X509* (__cdecl* _PEM_read_bio_X509)(BIO*, X509**, pem_password_cb*, void*);
typedef X509_CRL* (__cdecl* _PEM_read_bio_X509_CRL)(BIO*, X509_CRL**, pem_password_cb*, void*);
typedef int (__cdecl* _X509_STORE_add_cert)(X509_STORE*, X509*);
typedef int (__cdecl* _X509_STORE_add_crl)(X509_STORE*, X509_CRL*);
typedef void (__cdecl* _EVP_PKEY_free)(EVP_PKEY*);
typedef RSA* (__cdecl* _EVP_PKEY_get1_RSA)(EVP_PKEY*);
typedef void (__cdecl* _RSA_free)(RSA*);
typedef int (__cdecl* _RSA_size)(const RSA*);
typedef int (__cdecl* _RSA_private_encrypt)(int, const unsigned char*, unsigned char*, RSA*, int);
typedef int (__cdecl* _RSA_public_decrypt)(int, const unsigned char*, unsigned char*, RSA*, int);
typedef unsigned char* (__cdecl* _SHA1)(const unsigned char*, size_t, unsigned char*);
typedef unsigned char* (__cdecl* _SHA224)(const unsigned char*, size_t, unsigned char*);
typedef unsigned char* (__cdecl* _SHA256)(const unsigned char*, size_t, unsigned char*);
typedef unsigned char* (__cdecl* _SHA384)(const unsigned char*, size_t, unsigned char*);
typedef unsigned char* (__cdecl* _SHA512)(const unsigned char*, size_t, unsigned char*);

_SSL_load_error_strings p_SSL_load_error_strings;
_SSL_library_init p_SSL_library_init;
_SSLv23_method p_SSLv23_method;
_SSL_new p_SSL_new;
_SSL_free p_SSL_free;
_SSL_ctrl p_SSL_ctrl;
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
_SSL_set_cipher_list p_SSL_set_cipher_list;
_SSL_CTX_new p_SSL_CTX_new;
_SSL_CTX_free p_SSL_CTX_free;
_SSL_CTX_get_cert_store p_SSL_CTX_get_cert_store;
_SSL_CTX_ctrl p_SSL_CTX_ctrl;
_BIO_s_mem p_BIO_s_mem;
_BIO_new p_BIO_new;
_BIO_free p_BIO_free;
_BIO_new_mem_buf p_BIO_new_mem_buf;
_BIO_ctrl p_BIO_ctrl;
_X509_free p_X509_free;
_X509_print_ex p_X509_print_ex;
_X509_get_subject_name p_X509_get_subject_name;
_X509_NAME_print_ex p_X509_NAME_print_ex;
_X509_CRL_free p_X509_CRL_free;
_PEM_read_bio_PrivateKey p_PEM_read_bio_PrivateKey;
_PEM_read_bio_PUBKEY p_PEM_read_bio_PUBKEY;
_PEM_read_bio_X509 p_PEM_read_bio_X509;
_PEM_read_bio_X509_CRL p_PEM_read_bio_X509_CRL;
_X509_STORE_add_cert p_X509_STORE_add_cert;
_X509_STORE_add_crl p_X509_STORE_add_crl;
_EVP_PKEY_free p_EVP_PKEY_free;
_EVP_PKEY_get1_RSA p_EVP_PKEY_get1_RSA;
_RSA_free p_RSA_free;
_RSA_size p_RSA_size;
_RSA_private_encrypt p_RSA_private_encrypt;
_RSA_public_decrypt p_RSA_public_decrypt;
_SHA1 p_SHA1;
_SHA224 p_SHA224;
_SHA256 p_SHA256;
_SHA384 p_SHA384;
_SHA512 p_SHA512;

#define MAX_SSL_SOCKET 16

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

// OpenSSLを初期化
BOOL LoadOpenSSL()
{
	if(g_bOpenSSLLoaded)
		return FALSE;
#ifdef ENABLE_PROCESS_PROTECTION
	// 同梱するOpenSSLのバージョンに合わせてSHA1ハッシュ値を変更すること
#if defined(_M_IX86)
	// ssleay32.dll 1.0.2e
	RegisterTrustedModuleSHA1Hash("\xE8\xD5\xBE\x7A\xD7\xAC\x17\x7E\x1E\x60\xA7\x6A\xD3\xE6\x14\xC9\x7A\x79\x87\x7C");
	// libeay32.dll 1.0.2e
	RegisterTrustedModuleSHA1Hash("\x45\xEC\x0B\xCC\x1E\x5F\xC9\xF4\xDA\x03\xF5\xEE\xAB\x6C\x85\x3A\xD8\x49\x23\xD4");
#elif defined(_M_AMD64)
	// ssleay32.dll 1.0.2e
	RegisterTrustedModuleSHA1Hash("\xCB\x81\x60\x86\x1C\x27\xB8\x6D\x43\xA5\xBF\x34\x9F\x8E\xE0\x81\x2F\xFD\xC9\xA6");
	// libeay32.dll 1.0.2e
	RegisterTrustedModuleSHA1Hash("\x4A\xD3\x39\x10\x66\xA7\x89\x17\xCF\x5C\x65\x8C\xDE\x43\x9B\xF1\x64\xAE\x0E\x04");
#endif
#endif
	g_hOpenSSL = LoadLibrary("ssleay32.dll");
	// バージョン固定のためlibssl32.dllの読み込みは脆弱性の原因になり得るので廃止
//	if(!g_hOpenSSL)
//		g_hOpenSSL = LoadLibrary("libssl32.dll");
	if(!g_hOpenSSL
		|| !(p_SSL_load_error_strings = (_SSL_load_error_strings)GetProcAddress(g_hOpenSSL, "SSL_load_error_strings"))
		|| !(p_SSL_library_init = (_SSL_library_init)GetProcAddress(g_hOpenSSL, "SSL_library_init"))
		|| !(p_SSLv23_method = (_SSLv23_method)GetProcAddress(g_hOpenSSL, "SSLv23_method"))
		|| !(p_SSL_new = (_SSL_new)GetProcAddress(g_hOpenSSL, "SSL_new"))
		|| !(p_SSL_free = (_SSL_free)GetProcAddress(g_hOpenSSL, "SSL_free"))
		|| !(p_SSL_ctrl = (_SSL_ctrl)GetProcAddress(g_hOpenSSL, "SSL_ctrl"))
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
		|| !(p_SSL_set_session = (_SSL_set_session)GetProcAddress(g_hOpenSSL, "SSL_set_session"))
		|| !(p_SSL_set_cipher_list = (_SSL_set_cipher_list)GetProcAddress(g_hOpenSSL, "SSL_set_cipher_list"))
		|| !(p_SSL_CTX_new = (_SSL_CTX_new)GetProcAddress(g_hOpenSSL, "SSL_CTX_new"))
		|| !(p_SSL_CTX_free = (_SSL_CTX_free)GetProcAddress(g_hOpenSSL, "SSL_CTX_free"))
		|| !(p_SSL_CTX_get_cert_store = (_SSL_CTX_get_cert_store)GetProcAddress(g_hOpenSSL, "SSL_CTX_get_cert_store"))
		|| !(p_SSL_CTX_ctrl = (_SSL_CTX_ctrl)GetProcAddress(g_hOpenSSL, "SSL_CTX_ctrl")))
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
		|| !(p_BIO_new_mem_buf = (_BIO_new_mem_buf)GetProcAddress(g_hOpenSSLCommon, "BIO_new_mem_buf"))
		|| !(p_BIO_ctrl = (_BIO_ctrl)GetProcAddress(g_hOpenSSLCommon, "BIO_ctrl"))
		|| !(p_X509_free = (_X509_free)GetProcAddress(g_hOpenSSLCommon, "X509_free"))
		|| !(p_X509_print_ex = (_X509_print_ex)GetProcAddress(g_hOpenSSLCommon, "X509_print_ex"))
		|| !(p_X509_get_subject_name = (_X509_get_subject_name)GetProcAddress(g_hOpenSSLCommon, "X509_get_subject_name"))
		|| !(p_X509_NAME_print_ex = (_X509_NAME_print_ex)GetProcAddress(g_hOpenSSLCommon, "X509_NAME_print_ex"))
		|| !(p_X509_CRL_free = (_X509_CRL_free)GetProcAddress(g_hOpenSSLCommon, "X509_CRL_free"))
		|| !(p_PEM_read_bio_PrivateKey = (_PEM_read_bio_PrivateKey)GetProcAddress(g_hOpenSSLCommon, "PEM_read_bio_PrivateKey"))
		|| !(p_PEM_read_bio_PUBKEY = (_PEM_read_bio_PUBKEY)GetProcAddress(g_hOpenSSLCommon, "PEM_read_bio_PUBKEY"))
		|| !(p_PEM_read_bio_X509 = (_PEM_read_bio_X509)GetProcAddress(g_hOpenSSLCommon, "PEM_read_bio_X509"))
		|| !(p_PEM_read_bio_X509_CRL = (_PEM_read_bio_X509_CRL)GetProcAddress(g_hOpenSSLCommon, "PEM_read_bio_X509_CRL"))
		|| !(p_X509_STORE_add_cert = (_X509_STORE_add_cert)GetProcAddress(g_hOpenSSLCommon, "X509_STORE_add_cert"))
		|| !(p_X509_STORE_add_crl = (_X509_STORE_add_crl)GetProcAddress(g_hOpenSSLCommon, "X509_STORE_add_crl"))
		|| !(p_EVP_PKEY_free = (_EVP_PKEY_free)GetProcAddress(g_hOpenSSLCommon, "EVP_PKEY_free"))
		|| !(p_EVP_PKEY_get1_RSA = (_EVP_PKEY_get1_RSA)GetProcAddress(g_hOpenSSLCommon, "EVP_PKEY_get1_RSA"))
		|| !(p_RSA_free = (_RSA_free)GetProcAddress(g_hOpenSSLCommon, "RSA_free"))
		|| !(p_RSA_size = (_RSA_size)GetProcAddress(g_hOpenSSLCommon, "RSA_size"))
		|| !(p_RSA_private_encrypt = (_RSA_private_encrypt)GetProcAddress(g_hOpenSSLCommon, "RSA_private_encrypt"))
		|| !(p_RSA_public_decrypt = (_RSA_public_decrypt)GetProcAddress(g_hOpenSSLCommon, "RSA_public_decrypt"))
		|| !(p_SHA1 = (_SHA1)GetProcAddress(g_hOpenSSLCommon, "SHA1"))
		|| !(p_SHA224 = (_SHA224)GetProcAddress(g_hOpenSSLCommon, "SHA224"))
		|| !(p_SHA256 = (_SHA256)GetProcAddress(g_hOpenSSLCommon, "SHA256"))
		|| !(p_SHA384 = (_SHA384)GetProcAddress(g_hOpenSSLCommon, "SHA384"))
		|| !(p_SHA512 = (_SHA512)GetProcAddress(g_hOpenSSLCommon, "SHA512")))
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

// OpenSSLを解放
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

// OpenSSLが使用可能かどうか確認
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
	if(pX509 && p_SSL_get_verify_result(pSSL) == X509_V_OK)
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

// SSLルート証明書を設定
// PEM形式のみ指定可能
BOOL SetSSLRootCertificate(const void* pData, DWORD Length)
{
	BOOL r;
	X509_STORE* pStore;
	BYTE* p;
	BYTE* pBegin;
	BYTE* pEnd;
	DWORD Left;
	BIO* pBIO;
	X509* pX509;
	X509_CRL* pX509_CRL;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	r = FALSE;
	EnterCriticalSection(&g_OpenSSLLock);
	if(!g_pOpenSSLCTX)
	{
		g_pOpenSSLCTX = p_SSL_CTX_new(p_SSLv23_method());
		p_SSL_CTX_ctrl(g_pOpenSSLCTX, SSL_CTRL_MODE, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_AUTO_RETRY, NULL);
	}
	if(g_pOpenSSLCTX)
	{
		if(pStore = p_SSL_CTX_get_cert_store(g_pOpenSSLCTX))
		{
			p = (BYTE*)pData;
			pBegin = NULL;
			pEnd = NULL;
			Left = Length;
			while(Left > 0)
			{
				if(!pBegin)
				{
					if(Left < 27)
						break;
					if(memcmp(p, "-----BEGIN CERTIFICATE-----", 27) == 0)
						pBegin = p;
				}
				else if(!pEnd)
				{
					if(Left < 25)
						break;
					if(memcmp(p, "-----END CERTIFICATE-----", 25) == 0)
						pEnd = p + 25;
				}
				if(pBegin && pEnd)
				{
					if(pBIO = p_BIO_new_mem_buf(pBegin, (int)((size_t)pEnd - (size_t)pBegin)))
					{
						if(pX509 = p_PEM_read_bio_X509(pBIO, NULL, NULL, NULL))
						{
							if(p_X509_STORE_add_cert(pStore, pX509) == 1)
								r = TRUE;
							p_X509_free(pX509);
						}
						p_BIO_free(pBIO);
					}
					pBegin = NULL;
					pEnd = NULL;
				}
				p++;
				Left--;
			}
			p = (BYTE*)pData;
			pBegin = NULL;
			pEnd = NULL;
			Left = Length;
			while(Left > 0)
			{
				if(!pBegin)
				{
					if(Left < 24)
						break;
					if(memcmp(p, "-----BEGIN X509 CRL-----", 24) == 0)
						pBegin = p;
				}
				else if(!pEnd)
				{
					if(Left < 22)
						break;
					if(memcmp(p, "-----END X509 CRL-----", 22) == 0)
						pEnd = p + 22;
				}
				if(pBegin && pEnd)
				{
					if(pBIO = p_BIO_new_mem_buf(pBegin, (int)((size_t)pEnd - (size_t)pBegin)))
					{
						if(pX509_CRL = p_PEM_read_bio_X509_CRL(pBIO, NULL, NULL, NULL))
						{
							if(p_X509_STORE_add_crl(pStore, pX509_CRL) == 1)
								r = TRUE;
							p_X509_CRL_free(pX509_CRL);
						}
						p_BIO_free(pBIO);
					}
					pBegin = NULL;
					pEnd = NULL;
				}
				p++;
				Left--;
			}
		}
	}
	LeaveCriticalSection(&g_OpenSSLLock);
	return r;
}

// ワイルドカードの比較
// 主にSSL証明書のCN確認用
BOOL IsHostNameMatched(LPCSTR HostName, LPCSTR CommonName)
{
	BOOL bResult;
	const char* pAsterisk;
	size_t BeforeAsterisk;
	const char* pBeginAsterisk;
	const char* pEndAsterisk;
	const char* pDot;
	bResult = FALSE;
	if(HostName && CommonName)
	{
		if(pAsterisk = strchr(CommonName, '*'))
		{
			BeforeAsterisk = ((size_t)pAsterisk - (size_t)CommonName) / sizeof(char);
			pBeginAsterisk = HostName + BeforeAsterisk;
			while(*pAsterisk == '*')
			{
				pAsterisk++;
			}
			pEndAsterisk = HostName + strlen(HostName) - strlen(pAsterisk);
			// "*"より前は大文字小文字を無視して完全一致
			if(_strnicmp(HostName, CommonName, BeforeAsterisk) == 0)
			{
				// "*"より後は大文字小文字を無視して完全一致
				if(_stricmp(pEndAsterisk, pAsterisk) == 0)
				{
					// "*"と一致する範囲に"."が含まれてはならない
					pDot = strchr(pBeginAsterisk, '.');
					if(!pDot || pDot >= pEndAsterisk)
						bResult = TRUE;
				}
			}
		}
		else if(_stricmp(HostName, CommonName) == 0)
			bResult = TRUE;
	}
	return bResult;
}

#pragma warning(push)
#pragma warning(disable:4090)

// RSA暗号化
BOOL EncryptSignature(const char* PrivateKey, const char* Password, const void* pIn, DWORD InLength, void* pOut, DWORD OutLength, DWORD* pOutLength)
{
	BOOL bResult;
	BIO* pBIO;
	EVP_PKEY* pPKEY;
	RSA* pRSA;
	int i;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	bResult = FALSE;
	if(pBIO = p_BIO_new_mem_buf((void*)PrivateKey, sizeof(char) * strlen(PrivateKey)))
	{
		if(pPKEY = p_PEM_read_bio_PrivateKey(pBIO, NULL, NULL, (void*)Password))
		{
			if(pRSA = p_EVP_PKEY_get1_RSA(pPKEY))
			{
				if(p_RSA_size(pRSA) <= (int)OutLength)
				{
					i = p_RSA_private_encrypt((int)InLength, (const unsigned char*)pIn, (unsigned char*)pOut, pRSA, RSA_PKCS1_PADDING);
					if(i >= 0)
					{
						*pOutLength = (DWORD)i;
						bResult = TRUE;
					}
				}
				p_RSA_free(pRSA);
			}
			p_EVP_PKEY_free(pPKEY);
		}
		p_BIO_free(pBIO);
	}
	return bResult;
}

// RSA復号化
// 主に自動更新ファイルのハッシュの改竄確認
BOOL DecryptSignature(const char* PublicKey, const char* Password, const void* pIn, DWORD InLength, void* pOut, DWORD OutLength, DWORD* pOutLength)
{
	BOOL bResult;
	BIO* pBIO;
	EVP_PKEY* pPKEY;
	RSA* pRSA;
	int i;
	if(!g_bOpenSSLLoaded)
		return FALSE;
	bResult = FALSE;
	if(pBIO = p_BIO_new_mem_buf((void*)PublicKey, sizeof(char) * strlen(PublicKey)))
	{
		if(pPKEY = p_PEM_read_bio_PUBKEY(pBIO, NULL, NULL, Password))
		{
			if(pRSA = p_EVP_PKEY_get1_RSA(pPKEY))
			{
				if(p_RSA_size(pRSA) <= (int)OutLength)
				{
					i = p_RSA_public_decrypt((int)InLength, (const unsigned char*)pIn, (unsigned char*)pOut, pRSA, RSA_PKCS1_PADDING);
					if(i >= 0)
					{
						*pOutLength = (DWORD)i;
						bResult = TRUE;
					}
				}
				p_RSA_free(pRSA);
			}
			p_EVP_PKEY_free(pPKEY);
		}
		p_BIO_free(pBIO);
	}
	return bResult;
}

#pragma warning(pop)

// ハッシュ計算
// 他にも同等の関数はあるが主にマルウェア対策のための冗長化
BOOL GetHashSHA1(const void* pData, DWORD Size, void* pHash)
{
	if(!g_bOpenSSLLoaded)
		return FALSE;
	p_SHA1((const unsigned char*)pData, (size_t)Size, (unsigned char*)pHash);
	return TRUE;
}

BOOL GetHashSHA224(const void* pData, DWORD Size, void* pHash)
{
	if(!g_bOpenSSLLoaded)
		return FALSE;
	p_SHA224((const unsigned char*)pData, (size_t)Size, (unsigned char*)pHash);
	return TRUE;
}

BOOL GetHashSHA256(const void* pData, DWORD Size, void* pHash)
{
	if(!g_bOpenSSLLoaded)
		return FALSE;
	p_SHA256((const unsigned char*)pData, (size_t)Size, (unsigned char*)pHash);
	return TRUE;
}

BOOL GetHashSHA384(const void* pData, DWORD Size, void* pHash)
{
	if(!g_bOpenSSLLoaded)
		return FALSE;
	p_SHA384((const unsigned char*)pData, (size_t)Size, (unsigned char*)pHash);
	return TRUE;
}

BOOL GetHashSHA512(const void* pData, DWORD Size, void* pHash)
{
	if(!g_bOpenSSLLoaded)
		return FALSE;
	p_SHA512((const unsigned char*)pData, (size_t)Size, (unsigned char*)pHash);
	return TRUE;
}

// SSLセッションを開始
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted, BOOL bStrengthen)
{
	BOOL r;
	DWORD Time;
	SSL** ppSSL;
	BOOL bInherited;
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
	{
		g_pOpenSSLCTX = p_SSL_CTX_new(p_SSLv23_method());
		p_SSL_CTX_ctrl(g_pOpenSSLCTX, SSL_CTRL_MODE, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_AUTO_RETRY, NULL);
	}
	if(g_pOpenSSLCTX)
	{
		if(ppSSL = GetUnusedSSLPointer())
		{
			if(*ppSSL = p_SSL_new(g_pOpenSSLCTX))
			{
				if(p_SSL_set_fd(*ppSSL, s) != 0)
				{
					bInherited = FALSE;
					if(parent != INVALID_SOCKET)
					{
						if(ppSSLParent = FindSSLPointerFromSocket(parent))
						{
							if(pSession = p_SSL_get_session(*ppSSLParent))
							{
								if(p_SSL_set_session(*ppSSL, pSession) == 1)
									bInherited = TRUE;
							}
						}
					}
					if(!bInherited)
					{
						if(bStrengthen)
						{
							p_SSL_ctrl(*ppSSL, SSL_CTRL_OPTIONS, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3, NULL);
							p_SSL_set_cipher_list(*ppSSL, "HIGH");
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

// SSLセッションを終了
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

// SSLとしてマークされているか確認
// マークされていればTRUEを返す
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

SOCKET FTPS_socket(int af, int type, int protocol)
{
	return socket(af, type, protocol);
}

int FTPS_bind(SOCKET s, const struct sockaddr *addr, int namelen)
{
	return bind(s, addr, namelen);
}

int FTPS_listen(SOCKET s, int backlog)
{
	return listen(s, backlog);
}

// accept相当の関数
// ただし初めからSSLのネゴシエーションを行う
SOCKET FTPS_accept(SOCKET s, struct sockaddr *addr, int *addrlen)
{
	SOCKET r;
	BOOL bAborted;
	r = accept(s, addr, addrlen);
	bAborted = FALSE;
	if(!AttachSSL(r, INVALID_SOCKET, &bAborted, TRUE))
	{
		closesocket(r);
		return INVALID_SOCKET;
	}
	return r;
}

// connect相当の関数
// ただし初めからSSLのネゴシエーションを行う
int FTPS_connect(SOCKET s, const struct sockaddr *name, int namelen)
{
	int r;
	BOOL bAborted;
	r = connect(s, name, namelen);
	bAborted = FALSE;
	if(!AttachSSL(r, INVALID_SOCKET, &bAborted, TRUE))
		return SOCKET_ERROR;
	return r;
}

// closesocket相当の関数
int FTPS_closesocket(SOCKET s)
{
	DetachSSL(s);
	return closesocket(s);
}

// send相当の関数
int FTPS_send(SOCKET s, const char * buf, int len, int flags)
{
	int r;
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return send(s, buf, len, flags);
	EnterCriticalSection(&g_OpenSSLLock);
	ppSSL = FindSSLPointerFromSocket(s);
	LeaveCriticalSection(&g_OpenSSLLock);
	if(!ppSSL)
		return send(s, buf, len, flags);
	r = p_SSL_write(*ppSSL, buf, len);
	if(r < 0)
		return SOCKET_ERROR;
	return r;
}

// recv相当の関数
int FTPS_recv(SOCKET s, char * buf, int len, int flags)
{
	int r;
	SSL** ppSSL;
	if(!g_bOpenSSLLoaded)
		return recv(s, buf, len, flags);
	EnterCriticalSection(&g_OpenSSLLock);
	ppSSL = FindSSLPointerFromSocket(s);
	LeaveCriticalSection(&g_OpenSSLLock);
	if(!ppSSL)
		return recv(s, buf, len, flags);
	if(flags & MSG_PEEK)
		r = p_SSL_peek(*ppSSL, buf, len);
	else
		r = p_SSL_read(*ppSSL, buf, len);
	if(r < 0)
		return SOCKET_ERROR;
	return r;
}

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
	HANDLE hResult;
	GETHOSTBYNAMEDATA* pData;
	hResult = NULL;
	if(pData = malloc(sizeof(GETHOSTBYNAMEDATA)))
	{
		pData->hWnd = hWnd;
		pData->wMsg = wMsg;
		if(pData->name = malloc(sizeof(char) * (strlen(name) + 1)))
		{
			strcpy(pData->name, name);
			pData->buf = buf;
			pData->buflen = buflen;
			pData->Family = Family;
			if(pData->h = CreateThread(NULL, 0, WSAAsyncGetHostByNameIPv6ThreadProc, pData, CREATE_SUSPENDED, NULL))
			{
				ResumeThread(pData->h);
				hResult = pData->h;
			}
		}
	}
	if(!hResult)
	{
		if(pData)
		{
			if(pData->name)
				free(pData->name);
			free(pData);
		}
	}
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
				Result.u.Word[i] = (USHORT)strtol(cp, &p, 16);
				Result.u.Word[i] = ((Result.u.Word[i] & 0xff00) >> 8) | ((Result.u.Word[i] & 0x00ff) << 8);
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

BOOL ConvertDomainNameToPunycode(LPSTR Output, DWORD Count, LPCSTR Input)
{
	BOOL bResult;
	punycode_uint* pUnicode;
	punycode_uint* p;
	BOOL bNeeded;
	LPCSTR InputString;
	punycode_uint Length;
	punycode_uint OutputLength;
	bResult = FALSE;
	if(pUnicode = malloc(sizeof(punycode_uint) * strlen(Input)))
	{
		p = pUnicode;
		bNeeded = FALSE;
		InputString = Input;
		Length = 0;
		while(*InputString != '\0')
		{
			*p = (punycode_uint)GetNextCharM(InputString, NULL, &InputString);
			if(*p >= 0x80)
				bNeeded = TRUE;
			p++;
			Length++;
		}
		if(bNeeded)
		{
			if(Count >= strlen("xn--") + 1)
			{
				strcpy(Output, "xn--");
				OutputLength = Count - strlen("xn--");
				if(punycode_encode(Length, pUnicode, NULL, (punycode_uint*)&OutputLength, Output + strlen("xn--")) == punycode_success)
				{
					Output[strlen("xn--") + OutputLength] = '\0';
					bResult = TRUE;
				}
			}
		}
		free(pUnicode);
	}
	if(!bResult)
	{
		if(Count >= strlen(Input) + 1)
		{
			strcpy(Output, Input);
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input)
{
	BOOL bResult;
	DWORD Length;
	char* pm0;
	char* pm1;
	char* p;
	char* pNext;
	bResult = FALSE;
	Length = strlen(Input);
	if(pm0 = AllocateStringM(Length + 1))
	{
		if(pm1 = AllocateStringM(Length * 4 + 1))
		{
			strcpy(pm0, Input);
			p = pm0;
			while(p)
			{
				if(pNext = strchr(p, '.'))
				{
					*pNext = '\0';
					pNext++;
				}
				if(ConvertDomainNameToPunycode(pm1, Length * 4, p))
					strcat(Output, pm1);
				if(pNext)
					strcat(Output, ".");
				p = pNext;
			}
			bResult = TRUE;
			FreeDuplicatedString(pm1);
		}
		FreeDuplicatedString(pm0);
	}
	return bResult;
}

HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen)
{
	HANDLE r = NULL;
	char* pa0 = NULL;
	if(pa0 = AllocateStringA(strlen(name) * 4))
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
	if(pa0 = AllocateStringA(strlen(name) * 4))
	{
		if(ConvertNameToPunycode(pa0, name))
			r = WSAAsyncGetHostByNameIPv6(hWnd, wMsg, pa0, buf, buflen, Family);
	}
	FreeDuplicatedString(pa0);
	return r;
}

