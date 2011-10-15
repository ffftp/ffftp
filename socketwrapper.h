// socketwrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー

#ifndef __SOCKETWRAPPER_H__
#define __SOCKETWRAPPER_H__

#define USE_OPENSSL

typedef BOOL (__stdcall* LPSSLTIMEOUTCALLBACK)();
typedef BOOL (__stdcall* LPSSLCONFIRMCALLBACK)(BOOL, LPCSTR, LPCSTR);

BOOL LoadOpenSSL();
void FreeOpenSSL();
BOOL IsOpenSSLLoaded();
void SetSSLTimeoutCallback(DWORD Timeout, LPSSLTIMEOUTCALLBACK pCallback);
void SetSSLConfirmCallback(LPSSLCONFIRMCALLBACK pCallback);
BOOL IsHostNameMatched(LPCSTR HostName, LPCSTR CommonName);
BOOL AttachSSL(SOCKET s, SOCKET parent);
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

#endif

