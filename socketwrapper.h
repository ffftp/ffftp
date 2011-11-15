// socketwrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー

#ifndef __SOCKETWRAPPER_H__
#define __SOCKETWRAPPER_H__

#include <ws2tcpip.h>
#include <windows.h>

#define USE_OPENSSL

typedef BOOL (__stdcall* LPSSLTIMEOUTCALLBACK)(BOOL*);
typedef BOOL (__stdcall* LPSSLCONFIRMCALLBACK)(BOOL*, BOOL, LPCSTR, LPCSTR);

BOOL LoadOpenSSL();
void FreeOpenSSL();
BOOL IsOpenSSLLoaded();
void SetSSLTimeoutCallback(DWORD Timeout, LPSSLTIMEOUTCALLBACK pCallback);
void SetSSLConfirmCallback(LPSSLCONFIRMCALLBACK pCallback);
BOOL SetSSLRootCertificate(const void* pData, DWORD Length);
BOOL IsHostNameMatched(LPCSTR HostName, LPCSTR CommonName);
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted);
BOOL DetachSSL(SOCKET s);
BOOL IsSSLAttached(SOCKET s);
SOCKET socketS(int af, int type, int protocol);
int bindS(SOCKET s, const struct sockaddr *addr, int namelen);
int listenS(SOCKET s, int backlog);
SOCKET acceptS(SOCKET s, struct sockaddr *addr, int *addrlen);
int connectS(SOCKET s, const struct sockaddr *name, int namelen);
int closesocketS(SOCKET s);
int sendS(SOCKET s, const char * buf, int len, int flags);
int recvS(SOCKET s, char * buf, int len, int flags);

HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle);
char* AddressToStringIPv6(char* str, void* in6);
char* inet6_ntoa(struct in6_addr in6);
struct in6_addr inet6_addr(const char* cp);
HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);

#endif

