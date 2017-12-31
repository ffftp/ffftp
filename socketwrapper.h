// socketwrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー

#ifndef __SOCKETWRAPPER_H__
#define __SOCKETWRAPPER_H__

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
BOOL GetHashSHA1(const void* pData, DWORD Size, void* pHash);
BOOL GetHashSHA224(const void* pData, DWORD Size, void* pHash);
BOOL GetHashSHA256(const void* pData, DWORD Size, void* pHash);
BOOL GetHashSHA384(const void* pData, DWORD Size, void* pHash);
BOOL GetHashSHA512(const void* pData, DWORD Size, void* pHash);
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted, BOOL bStrengthen, const char* ServerName);
BOOL DetachSSL(SOCKET s);
BOOL IsSSLAttached(SOCKET s);
SOCKET FTPS_socket(int af, int type, int protocol);
int FTPS_bind(SOCKET s, const struct sockaddr *addr, int namelen);
int FTPS_listen(SOCKET s, int backlog);
SOCKET FTPS_accept(SOCKET s, struct sockaddr *addr, int *addrlen);
int FTPS_connect(SOCKET s, const struct sockaddr *name, int namelen);
int FTPS_closesocket(SOCKET s);
int FTPS_send(SOCKET s, const char * buf, int len, int flags);
int FTPS_recv(SOCKET s, char * buf, int len, int flags);

HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle);
char* AddressToStringIPv4(char* str, void* in);
char* AddressToStringIPv6(char* str, void* in6);
char* inet6_ntoa(struct in6_addr in6);
struct in6_addr inet6_addr(const char* cp);
BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input);
HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);

extern const struct in6_addr IN6ADDR_NONE;

#endif

