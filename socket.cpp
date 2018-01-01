/*=============================================================================
*
*									ソケット
*
===============================================================================
/ Copyright (C) 1997-2007 Sota. All rights reserved.
/
/ Redistribution and use in source and binary forms, with or without 
/ modification, are permitted provided that the following conditions 
/ are met:
/
/  1. Redistributions of source code must retain the above copyright 
/     notice, this list of conditions and the following disclaimer.
/  2. Redistributions in binary form must reproduce the above copyright 
/     notice, this list of conditions and the following disclaimer in the 
/     documentation and/or other materials provided with the distribution.
/
/ THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
/ IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
/ OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
/ IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
/ INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
/ USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
/ ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
/ THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/============================================================================*/

#include "common.h"
#define SECURITY_WIN32
#include <natupnp.h>
#include <schannel.h>
#include <security.h>
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Secur32.lib")


#define USE_THIS	1
#define DBG_MSG		0




// Winsock2で定義される定数と名前が重複し値が異なるため使用不可
//#define FD_CONNECT_BIT		0x0001
//#define FD_CLOSE_BIT		0x0002
//#define FD_ACCEPT_BIT		0x0004
//#define FD_READ_BIT			0x0008
//#define FD_WRITE_BIT		0x0010





typedef struct {
	SOCKET Socket;
	int FdConnect;
	int FdClose;
	int FdAccept;
	int FdRead;
	int FdWrite;
	int Error;
	// ソケットにデータを付与
	struct sockaddr_in HostAddrIPv4;
	struct sockaddr_in SocksAddrIPv4;
	struct sockaddr_in6 HostAddrIPv6;
	struct sockaddr_in6 SocksAddrIPv6;
	int MapPort;
} ASYNCSIGNAL;


typedef struct {
	HANDLE Async;
	int Done;
	int ErrorDb;
} ASYNCSIGNALDATABASE;


// スレッド衝突のバグ修正
// 念のためテーブルを増量
//#define MAX_SIGNAL_ENTRY		10
//#define MAX_SIGNAL_ENTRY_DBASE	5
#define MAX_SIGNAL_ENTRY		16
#define MAX_SIGNAL_ENTRY_DBASE	16




/*===== プロトタイプ =====*/

static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int AskAsyncDone(SOCKET s, int *Error, int Mask);
static int AskAsyncDoneDbase(HANDLE Async, int *Error);
static int RegisterAsyncTable(SOCKET s);
static int RegisterAsyncTableDbase(HANDLE Async);
static int UnregisterAsyncTable(SOCKET s);
static int UnregisterAsyncTableDbase(HANDLE Async);


/*===== 外部参照 =====*/

extern int TimeOut;


/*===== ローカルなワーク =====*/

static const char SocketWndClass[] = "FFFTPSocketWnd";
static HWND hWndSocket;

static ASYNCSIGNAL Signal[MAX_SIGNAL_ENTRY];
static ASYNCSIGNALDATABASE SignalDbase[MAX_SIGNAL_ENTRY_DBASE];

//static HANDLE hAsyncTblAccMutex;
// スレッド衝突のバグ修正
static HANDLE hAsyncTblAccMutex;

// UPnP対応
IUPnPNAT* pUPnPNAT;
IStaticPortMappingCollection* pUPnPMap;


template<class T>
static T CreateInvalidateHandle() {
	T handle;
	SecInvalidateHandle(&handle);
	return std::move(handle);
}

struct Context {
	std::wstring host;
	CtxtHandle context;
	SecPkgContext_StreamSizes streamSizes;
	std::vector<char> readRaw;
	std::vector<char> readPlain;
	SECURITY_STATUS readStatus = SEC_E_OK;
	Context(std::wstring const& host, CtxtHandle context, SecPkgContext_StreamSizes streamSizes, std::vector<char> extra) : host{ host }, context{ context }, streamSizes{ streamSizes }, readRaw{ extra } {
		if (!empty(readRaw))
			Decypt();
	}
	Context(Context const&) = delete;
	~Context() {
		DeleteSecurityContext(&context);
	}
	void Decypt() {
		for (;;) {
			SecBuffer buffer[]{
				{ size_as<unsigned long>(readRaw), SECBUFFER_DATA, data(readRaw) },
				{ 0, SECBUFFER_EMPTY, nullptr },
				{ 0, SECBUFFER_EMPTY, nullptr },
				{ 0, SECBUFFER_EMPTY, nullptr },
			};
			SecBufferDesc desc{ SECBUFFER_VERSION, size_as<unsigned long>(buffer), buffer };
			if (readStatus = DecryptMessage(&context, &desc, 0, nullptr); readStatus != SEC_E_OK) {
				_RPTWN(_CRT_WARN, L"DecryptMessage error: %08X.\n", readStatus);
				break;
			}
			assert(buffer[0].BufferType == SECBUFFER_STREAM_HEADER && buffer[1].BufferType == SECBUFFER_DATA && buffer[2].BufferType == SECBUFFER_STREAM_TRAILER);
			readPlain.insert(end(readPlain), reinterpret_cast<const char*>(buffer[1].pvBuffer), reinterpret_cast<const char*>(buffer[1].pvBuffer) + buffer[1].cbBuffer);
			if (buffer[3].BufferType != SECBUFFER_EXTRA) {
				readRaw.clear();
				break;
			}
			readRaw = std::vector<char>(reinterpret_cast<const char*>(buffer[3].pvBuffer), reinterpret_cast<const char*>(buffer[3].pvBuffer) + buffer[3].cbBuffer);
		}
	}
	std::vector<char> Encrypt(std::string_view plain) {
		std::vector<char> result;
		while (!empty(plain)) {
			auto dataLength = std::min(size_as<unsigned long>(plain), streamSizes.cbMaximumMessage);
			auto offset = size_as<unsigned long>(result);
			result.resize(offset + streamSizes.cbHeader + dataLength + streamSizes.cbTrailer);
			std::copy_n(begin(plain), dataLength, begin(result) + offset + streamSizes.cbHeader);
			SecBuffer buffer[]{
				{ streamSizes.cbHeader,  SECBUFFER_STREAM_HEADER,  data(result) + offset },
				{ dataLength,            SECBUFFER_DATA,           data(result) + offset + streamSizes.cbHeader },
				{ streamSizes.cbTrailer, SECBUFFER_STREAM_TRAILER, data(result) + offset + streamSizes.cbHeader + dataLength },
				{ 0, SECBUFFER_EMPTY, nullptr },
			};
			SecBufferDesc desc{ SECBUFFER_VERSION, size_as<unsigned long>(buffer), buffer };
			if (auto ss = EncryptMessage(&context, 0, &desc, 0); ss != SEC_E_OK) {
				_RPTWN(_CRT_WARN, L"FTPS_send EncryptMessage error: %08x.\n", ss);
				return {};
			}
			assert(buffer[0].BufferType == SECBUFFER_STREAM_HEADER && buffer[0].cbBuffer == streamSizes.cbHeader);
			assert(buffer[1].BufferType == SECBUFFER_DATA && buffer[1].cbBuffer == dataLength);
			assert(buffer[2].BufferType == SECBUFFER_STREAM_TRAILER && buffer[2].cbBuffer <= streamSizes.cbTrailer);
			result.resize(offset + buffer[0].cbBuffer + buffer[1].cbBuffer + buffer[2].cbBuffer);
			plain = plain.substr(dataLength);
		}
		return std::move(result);
	}
};

static CredHandle credential = CreateInvalidateHandle<CredHandle>();
static std::mutex context_mutex;
static std::map<SOCKET, Context> contexts;

BOOL LoadSSL() {
	if (auto ss = AcquireCredentialsHandleW(nullptr, UNISP_NAME_W, SECPKG_CRED_OUTBOUND, nullptr, nullptr, nullptr, nullptr, &credential, nullptr); ss != SEC_E_OK) {
		_RPTWN(_CRT_WARN, L"AcquireCredentialsHandle error: %08X.\n", ss);
		return FALSE;
	}
	return TRUE;
}

void FreeSSL() {
	assert(SecIsValidHandle(&credential));
	std::lock_guard<std::mutex> lock_guard{ context_mutex };
	contexts.clear();
	FreeCredentialsHandle(&credential);
}

static Context* getContext(SOCKET s) {
	assert(SecIsValidHandle(&credential));
	std::lock_guard<std::mutex> lock_guard{ context_mutex };
	if (auto it = contexts.find(s); it != contexts.end())
		return &it->second;
	return nullptr;
}

namespace std {
	template<>
	struct default_delete<CERT_CONTEXT> {
		void operator()(CERT_CONTEXT* ptr) {
			CertFreeCertificateContext(ptr);
		}
	};
}

static BOOL ConfirmSSLCertificate(CtxtHandle& context, wchar_t* serverName, BOOL* pbAborted) {
	std::unique_ptr<CERT_CONTEXT> certContext{ [&context] {
		PCERT_CONTEXT certContext;
		if (auto ss = QueryContextAttributesW(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &certContext); ss != SEC_E_OK) {
			_RPTWN(_CRT_WARN, L"QueryContextAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT) error: %08X.\n", ss);
			nullptr;
		}
		return certContext;
	}() };
	if (!certContext)
		return FALSE;
	CERT_CHAIN_PARA chainPara{ sizeof CERT_CHAIN_PARA };
	PCCERT_CHAIN_CONTEXT chainContext;
	if (!CertGetCertificateChain(nullptr, certContext.get(), nullptr, nullptr, &chainPara, CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT, nullptr, &chainContext)) {
		_RPTWN(_CRT_WARN, L"CertGetCertificateChain error: %08X.\n", GetLastError());
		return FALSE;
	}
	SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslPolicy{ sizeof SSL_EXTRA_CERT_CHAIN_POLICY_PARA , AUTHTYPE_SERVER, 0, serverName };
	CERT_CHAIN_POLICY_PARA policyPara{ sizeof CERT_CHAIN_POLICY_PARA, 0, &sslPolicy };
	CERT_CHAIN_POLICY_STATUS policyStatus{ sizeof CERT_CHAIN_POLICY_STATUS };
	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chainContext, &policyPara, &policyStatus)) {
		_RPTW0(_CRT_WARN, L"CertVerifyCertificateChainPolicy: not able to check.\n");
		return FALSE;
	}
	if (policyStatus.dwError == 0)
		return TRUE;
	_RPTWN(_CRT_WARN, L"CertVerifyCertificateChainPolicy result: %08X.\n", policyStatus.dwError);
	return ConfirmCertificate(serverName, pbAborted);
}

// SSLセッションを終了
static BOOL DetachSSL(SOCKET s) {
	std::lock_guard<std::mutex> lock_guard{ context_mutex };
	contexts.erase(s);
	return true;
}

// SSLセッションを開始
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted, const char* ServerName) {
	assert(SecIsValidHandle(&credential));
	std::wstring wServerName;
	if (ServerName)
		wServerName = u8(ServerName);
	else if (parent != INVALID_SOCKET)
		if (auto context = getContext(parent))
			wServerName = context->host;
	auto node = empty(wServerName) ? nullptr : data(wServerName);

	CtxtHandle context;
	SecPkgContext_StreamSizes streamSizes;
	std::vector<char> extra;
	auto first = true;
	SECURITY_STATUS ss = SEC_I_CONTINUE_NEEDED;
	do {
		constexpr unsigned long contextReq = ISC_REQ_STREAM | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR | ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_PROMPT_FOR_CREDS /* | ISC_REQ_MANUAL_CRED_VALIDATION*/;
		SecBuffer inBuffer[]{ { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr } };
		SecBuffer outBuffer[]{ { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr } };
		SecBufferDesc inDesc{ SECBUFFER_VERSION, size_as<unsigned long>(inBuffer), inBuffer };
		SecBufferDesc outDesc{ SECBUFFER_VERSION, size_as<unsigned long>(outBuffer), outBuffer };
		unsigned long attr = 0;
		if (first) {
			first = false;
			ss = InitializeSecurityContextW(&credential, nullptr, node, contextReq, 0, 0, nullptr, 0, &context, &outDesc, &attr, nullptr);
		} else {
			for (;;) {
				char buffer[8192];
				if (auto read = recv(s, buffer, size_as<int>(buffer), 0); read == 0) {
					_RPTW0(_CRT_WARN, L"AttachSSL recv: connection closed.\n");
					return FALSE;
				} else if (0 < read) {
					_RPTWN(_CRT_WARN, L"AttachSSL recv: %d bytes.\n", read);
					extra.insert(end(extra), buffer, buffer + read);
					break;
				}
				if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
					_RPTWN(_CRT_WARN, L"AttachSSL recv error: %d.\n", lastError);
					return FALSE;
				}
				Sleep(0);
			}
			inBuffer[0] = { size_as<unsigned long>(extra), SECBUFFER_TOKEN, data(extra) };
			ss = InitializeSecurityContextW(&credential, &context, node, contextReq, 0, 0, &inDesc, 0, nullptr, &outDesc, &attr, nullptr);
		}
		if (FAILED(ss) && ss != SEC_E_INCOMPLETE_MESSAGE) {
			_RPTWN(_CRT_WARN, L"AttachSSL InitializeSecurityContext error: %08x.\n", ss);
			return FALSE;
		}
		_RPTWN(_CRT_WARN, L"AttachSSL InitializeSecurityContext result: %08x, inBuffer: %d/%d, %d/%d/%p, outBuffer: %d/%d, %d/%d, attr: %08x.\n",
			ss, inBuffer[0].BufferType, inBuffer[0].cbBuffer, inBuffer[1].BufferType, inBuffer[1].cbBuffer, inBuffer[1].pvBuffer, outBuffer[0].BufferType, outBuffer[0].cbBuffer, outBuffer[1].BufferType, outBuffer[1].cbBuffer, attr);
		if (outBuffer[0].BufferType == SECBUFFER_TOKEN && outBuffer[0].cbBuffer != 0) {
			auto written = send(s, reinterpret_cast<const char*>(outBuffer[0].pvBuffer), outBuffer[0].cbBuffer, 0);
			assert(written == outBuffer[0].cbBuffer);
			_RPTWN(_CRT_WARN, L"AttachSSL send: %d bytes.\n", written);
			FreeContextBuffer(outBuffer[0].pvBuffer);
		}
		if (ss == SEC_E_INCOMPLETE_MESSAGE)
			ss = SEC_I_CONTINUE_NEEDED;
		else if (inBuffer[1].BufferType == SECBUFFER_EXTRA)
			// inBuffer[1].pvBufferはnullptrの場合があるためinBuffer[1].cbBufferのみを使用する
			extra.erase(begin(extra), end(extra) - inBuffer[1].cbBuffer);
		else
			extra.clear();
	} while (ss == SEC_I_CONTINUE_NEEDED);

	//if (!ConfirmSSLCertificate(context, node, pbAborted))
	//	return FALSE;
	if (ss = QueryContextAttributesW(&context, SECPKG_ATTR_STREAM_SIZES, &streamSizes); ss != SEC_E_OK) {
		_RPTWN(_CRT_WARN, L"AttachSSL QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES) error: %08x.\n", ss);
		return FALSE;
	}
	std::lock_guard<std::mutex> lock_guard{ context_mutex };
	contexts.emplace(std::piecewise_construct, std::forward_as_tuple(s), std::forward_as_tuple(wServerName, context, streamSizes, extra));
	_RPTW0(_CRT_WARN, L"AttachSSL success.\n");
	return TRUE;
}

// SSLとしてマークされているか確認
// マークされていればTRUEを返す
BOOL IsSSLAttached(SOCKET s) {
	return getContext(s) != nullptr;
}

static int FTPS_recv(SOCKET s, char* buf, int len, int flags) {
	assert(flags == 0 || flags == MSG_PEEK);
	auto context = getContext(s);
	if (!context)
		return recv(s, buf, len, flags);

	if (empty(context->readPlain)) {
		auto offset = size_as<int>(context->readRaw);
		context->readRaw.resize(context->streamSizes.cbHeader + context->streamSizes.cbMaximumMessage + context->streamSizes.cbTrailer);
		auto read = recv(s, data(context->readRaw) + offset, size_as<int>(context->readRaw) - offset, 0);
		if (read <= 0) {
			context->readRaw.resize(offset);
			if (read == 0)
				_RPTW0(_CRT_WARN, L"FTPS_recv recv: connection closed.\n");
#ifdef _DEBUG
			else if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK)
				_RPTWN(_CRT_WARN, L"FTPS_recv recv error: %d.\n", lastError);
#endif
			return read;
		}
		_RPTWN(_CRT_WARN, L"FTPS_recv recv: %d bytes.\n", read);
		context->readRaw.resize(offset + read);
		context->Decypt();
	}

	if (empty(context->readPlain))
		// TODO: recv()の読み出し量が少ないとSEC_E_INCOMPLETE_MESSAGEが発生してしまう。
		return context->readStatus == SEC_I_CONTEXT_EXPIRED ? 0 : SOCKET_ERROR;
	len = std::min(len, size_as<int>(context->readPlain));
	std::copy_n(begin(context->readPlain), len, buf);
	if ((flags & MSG_PEEK) == 0)
		context->readPlain.erase(begin(context->readPlain), begin(context->readPlain) + len);
	_RPTWN(_CRT_WARN, L"FTPS_recv read: %d bytes.\n", len);
	return len;
}


/*----- 
*
*	Parameter
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeSocketWin(HWND hWnd, HINSTANCE hInst)
{
	int i;
	int Sts;
	WNDCLASSEX wClass;

	wClass.cbSize        = sizeof(WNDCLASSEX);
	wClass.style         = 0;
	wClass.lpfnWndProc   = SocketWndProc;
	wClass.cbClsExtra    = 0;
	wClass.cbWndExtra    = 0;
	wClass.hInstance     = hInst;
	wClass.hIcon         = NULL;
	wClass.hCursor       = NULL;
	wClass.hbrBackground = (HBRUSH)CreateSolidBrush(GetSysColor(COLOR_INFOBK));
	wClass.lpszMenuName  = NULL;
	wClass.lpszClassName = SocketWndClass;
	wClass.hIconSm       = NULL;
	RegisterClassEx(&wClass);

	Sts = FFFTP_FAIL;
	hWndSocket = CreateWindowEx(0, SocketWndClass, NULL,
			WS_BORDER | WS_POPUP,
			0, 0, 0, 0,
			hWnd, NULL, hInst, NULL);

	if(hWndSocket != NULL)
	{
//		hAsyncTblAccMutex = CreateMutex(NULL, FALSE, NULL);

		// スレッド衝突のバグ修正
//		for(i = 0; i < MAX_SIGNAL_ENTRY; i++)
//			Signal[i].Socket = INVALID_SOCKET;
//		for(i = 0; i < MAX_SIGNAL_ENTRY_DBASE; i++)
//			SignalDbase[i].Async = 0;
		if(hAsyncTblAccMutex = CreateMutex(NULL, FALSE, NULL))
		{
			for(i = 0; i < MAX_SIGNAL_ENTRY; i++)
				Signal[i].Socket = INVALID_SOCKET;
			for(i = 0; i < MAX_SIGNAL_ENTRY_DBASE; i++)
				SignalDbase[i].Async = 0;
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteSocketWin(void)
{
//	CloseHandle(hAsyncTblAccMutex);
	// スレッド衝突のバグ修正
	CloseHandle(hAsyncTblAccMutex);
	hAsyncTblAccMutex = NULL;

	if(hWndSocket != NULL)
		DestroyWindow(hWndSocket);
	return;
}


/*----- 
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		UINT message : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Pos;

	switch(message)
	{
		case WM_ASYNC_SOCKET :
			// スレッド衝突のバグ修正
			WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
			for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
			{
				if(Signal[Pos].Socket == (SOCKET)wParam)
				{
					Signal[Pos].Error = WSAGETSELECTERROR(lParam);
#if DBG_MSG
					if(WSAGETSELECTERROR(lParam) != 0)
						DoPrintf("####### Signal: error (%d)", WSAGETSELECTERROR(lParam));
#endif

					switch(WSAGETSELECTEVENT(lParam))
					{
						case FD_CONNECT :
							Signal[Pos].FdConnect = 1;
#if DBG_MSG
							DoPrintf("####### Signal: connect (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_CLOSE :
							Signal[Pos].FdClose = 1;
#if DBG_MSG
							DoPrintf("####### Signal: close (S=%x)", Signal[Pos].Socket);
#endif
//SetTaskMsg("####### Signal: close (%d) (S=%x)", Pos, Signal[Pos].Socket);
							break;

						case FD_ACCEPT :
							Signal[Pos].FdAccept = 1;
#if DBG_MSG
							DoPrintf("####### Signal: accept (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_READ :
							Signal[Pos].FdRead = 1;
#if DBG_MSG
							DoPrintf("####### Signal: read (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_WRITE :
							Signal[Pos].FdWrite = 1;
#if DBG_MSG
							DoPrintf("####### Signal: write (S=%x)", Signal[Pos].Socket);
#endif
							break;
					}
					break;
				}
			}
			// スレッド衝突のバグ修正
			ReleaseMutex(hAsyncTblAccMutex);
			break;

		case WM_ASYNC_DBASE :
			// APIの仕様上ハンドルが登録される前にウィンドウメッセージが呼び出される可能性あり
			RegisterAsyncTableDbase((HANDLE)wParam);
			// スレッド衝突のバグ修正
			WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
			for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
			{
				if(SignalDbase[Pos].Async == (HANDLE)wParam)
				{
					if(HIWORD(lParam) != 0)
					{
						SignalDbase[Pos].ErrorDb = 1;
#if DBG_MSG
						DoPrintf("##### SignalDatabase: error");
#endif
					}
					SignalDbase[Pos].Done = 1;
#if DBG_MSG
					DoPrintf("##### SignalDatabase: Done");
#endif
					break;
				}
			}
			// スレッド衝突のバグ修正
			ReleaseMutex(hAsyncTblAccMutex);
			break;

		default :
			return(DefWindowProc(hWnd, message, wParam, lParam));
	}
	return(0);
}




/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int AskAsyncDone(SOCKET s, int *Error, int Mask)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	*Error = 0;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			*Error = Signal[Pos].Error;
			if(Signal[Pos].Error != 0)
				Sts = YES;
			if((Mask & FD_CONNECT) && (Signal[Pos].FdConnect != 0))
			{
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: connect (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_CLOSE) && (Signal[Pos].FdClose != 0))
//			if(Mask & FD_CLOSE)
			{
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: close (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_ACCEPT) && (Signal[Pos].FdAccept != 0))
			{
				Signal[Pos].FdAccept = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: accept (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_READ) && (Signal[Pos].FdRead != 0))
			{
				Signal[Pos].FdRead = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: read (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_WRITE) && (Signal[Pos].FdWrite != 0))
			{
				Signal[Pos].FdWrite = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: write (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY)
	{
		if(Mask & FD_CLOSE)
		{
				Sts = YES;
		}
		else
		{
			MessageBox(GetMainHwnd(), "AskAsyncDone called with unregisterd socket.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}
	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int AskAsyncDoneDbase(HANDLE Async, int *Error)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	*Error = 0;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{
			if(SignalDbase[Pos].Done != 0)
			{
				*Error = SignalDbase[Pos].ErrorDb;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: Dbase (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY_DBASE)
	{
		MessageBox(GetMainHwnd(), "AskAsyncDoneDbase called with unregisterd handle.", "FFFTP inner error", MB_OK);
		exit(1);
	}
	return(Sts);
}



/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int RegisterAsyncTable(SOCKET s)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			// 強制的に閉じられたソケットがあると重複する可能性あり
//			MessageBox(GetMainHwnd(), "Async socket already registerd.", "FFFTP inner error", MB_OK);
//			break;
			Signal[Pos].Socket = INVALID_SOCKET;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY)
	{
		// スレッド衝突のバグ修正
		WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
		for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
		{
			if(Signal[Pos].Socket == INVALID_SOCKET)
			{

//SetTaskMsg("############### Regist socket (%d)", Pos);

				Signal[Pos].Socket = s;
				Signal[Pos].Error = 0;
				Signal[Pos].FdConnect = 0;
				Signal[Pos].FdClose = 0;
				Signal[Pos].FdAccept = 0;
				Signal[Pos].FdRead = 0;
				Signal[Pos].FdWrite = 0;
				// ソケットにデータを付与
				memset(&Signal[Pos].HostAddrIPv4, 0, sizeof(struct sockaddr_in));
				memset(&Signal[Pos].SocksAddrIPv4, 0, sizeof(struct sockaddr_in));
				memset(&Signal[Pos].HostAddrIPv6, 0, sizeof(struct sockaddr_in6));
				memset(&Signal[Pos].SocksAddrIPv6, 0, sizeof(struct sockaddr_in6));
				Signal[Pos].MapPort = 0;
				Sts = YES;
				break;
			}
		}
		// スレッド衝突のバグ修正
		ReleaseMutex(hAsyncTblAccMutex);

		if(Pos == MAX_SIGNAL_ENTRY)
		{
			MessageBox(GetMainHwnd(), "No more async regist space.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}

	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int RegisterAsyncTableDbase(HANDLE Async)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{
			// 強制的に閉じられたハンドルがあると重複する可能性あり
//			MessageBox(GetMainHwnd(), "Async handle already registerd.", "FFFTP inner error", MB_OK);
			// APIの仕様上ハンドルが登録される前にウィンドウメッセージが呼び出される可能性あり
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY_DBASE)
	{
		// スレッド衝突のバグ修正
		WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
		for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
		{
			if(SignalDbase[Pos].Async == 0)
			{

//SetTaskMsg("############### Regist dbase (%d)", Pos);

				SignalDbase[Pos].Async = Async;
				SignalDbase[Pos].Done = 0;
				SignalDbase[Pos].ErrorDb = 0;
				Sts = YES;
				break;
			}
		}
		// スレッド衝突のバグ修正
		ReleaseMutex(hAsyncTblAccMutex);

		if(Pos == MAX_SIGNAL_ENTRY_DBASE)
		{
			MessageBox(GetMainHwnd(), "No more async dbase regist space.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}

	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int UnregisterAsyncTable(SOCKET s)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{

//SetTaskMsg("############### UnRegist socket (%d)", Pos);

			Signal[Pos].Socket = INVALID_SOCKET;
			Sts = YES;
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);
	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int UnregisterAsyncTableDbase(HANDLE Async)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{

//SetTaskMsg("############### UnRegist dbase (%d)", Pos);

			SignalDbase[Pos].Async = 0;
			Sts = YES;
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);
	return(Sts);
}


// ソケットにデータを付与

int SetAsyncTableDataIPv4(SOCKET s, struct sockaddr_in* Host, struct sockaddr_in* Socks)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			if(Host != NULL)
				memcpy(&Signal[Pos].HostAddrIPv4, Host, sizeof(struct sockaddr_in));
			if(Socks != NULL)
				memcpy(&Signal[Pos].SocksAddrIPv4, Socks, sizeof(struct sockaddr_in));
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}

int SetAsyncTableDataIPv6(SOCKET s, struct sockaddr_in6* Host, struct sockaddr_in6* Socks)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			if(Host != NULL)
				memcpy(&Signal[Pos].HostAddrIPv6, Host, sizeof(struct sockaddr_in6));
			if(Socks != NULL)
				memcpy(&Signal[Pos].SocksAddrIPv6, Socks, sizeof(struct sockaddr_in6));
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}

int SetAsyncTableDataMapPort(SOCKET s, int Port)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			Signal[Pos].MapPort = Port;
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}

int GetAsyncTableDataIPv4(SOCKET s, struct sockaddr_in* Host, struct sockaddr_in* Socks)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			if(Host != NULL)
				memcpy(Host, &Signal[Pos].HostAddrIPv4, sizeof(struct sockaddr_in));
			if(Socks != NULL)
				memcpy(Socks, &Signal[Pos].SocksAddrIPv4, sizeof(struct sockaddr_in));
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}

int GetAsyncTableDataIPv6(SOCKET s, struct sockaddr_in6* Host, struct sockaddr_in6* Socks)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			if(Host != NULL)
				memcpy(Host, &Signal[Pos].HostAddrIPv6, sizeof(struct sockaddr_in6));
			if(Socks != NULL)
				memcpy(Socks, &Signal[Pos].SocksAddrIPv6, sizeof(struct sockaddr_in6));
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}

int GetAsyncTableDataMapPort(SOCKET s, int* Port)
{
	int Sts;
	int Pos;

	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			*Port = Signal[Pos].MapPort;
			Sts = YES;
			break;
		}
	}
	ReleaseMutex(hAsyncTblAccMutex);

	return(Sts);
}







// IPv6対応
//struct hostent *do_gethostbyname(const char *Name, char *Buf, int Len, int *CancelCheckWork)
struct hostent *do_gethostbynameIPv4(const char *Name, char *Buf, int Len, int *CancelCheckWork)
{
#if USE_THIS
	struct hostent *Ret;
	HANDLE hAsync;
	int Error;

#if DBG_MSG
	DoPrintf("# Start gethostbyname");
#endif
	Ret = NULL;
	// 同時接続対応
//	*CancelCheckWork = NO;

	// UTF-8対応
//	hAsync = WSAAsyncGetHostByName(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	hAsync = WSAAsyncGetHostByNameM(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	if(hAsync != NULL)
	{
		RegisterAsyncTableDbase(hAsync);
		while((*CancelCheckWork == NO) && (AskAsyncDoneDbase(hAsync, &Error) != YES))
		{
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
		}

		if(*CancelCheckWork == YES)
		{
			WSACancelAsyncRequest(hAsync);
		}
		else if(Error == 0)
		{
			Ret = (struct hostent *)Buf;
		}
		UnregisterAsyncTableDbase(hAsync);
	}
	return(Ret);
#else
	return(gethostbyname(Name));
#endif
}


struct hostent *do_gethostbynameIPv6(const char *Name, char *Buf, int Len, int *CancelCheckWork)
{
#if USE_THIS
	struct hostent *Ret;
	HANDLE hAsync;
	int Error;

#if DBG_MSG
	DoPrintf("# Start gethostbyname");
#endif
	Ret = NULL;
	// 同時接続対応
//	*CancelCheckWork = NO;

	// UTF-8対応
//	hAsync = WSAAsyncGetHostByName(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	hAsync = WSAAsyncGetHostByNameIPv6M(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len, AF_INET6);
	if(hAsync != NULL)
	{
		RegisterAsyncTableDbase(hAsync);
		while((*CancelCheckWork == NO) && (AskAsyncDoneDbase(hAsync, &Error) != YES))
		{
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
		}

		if(*CancelCheckWork == YES)
		{
			WSACancelAsyncRequestIPv6(hAsync);
		}
		else if(Error == 0)
		{
			Ret = (struct hostent *)Buf;
		}
		UnregisterAsyncTableDbase(hAsync);
	}
	return(Ret);
#else
	return(gethostbyname(Name));
#endif
}





SOCKET do_socket(int af, int type, int protocol)
{
	SOCKET Ret;

	Ret = socket(af, type, protocol);
	if(Ret != INVALID_SOCKET)
	{
		RegisterAsyncTable(Ret);
	}
#if DBG_MSG
	DoPrintf("# do_socket (S=%x)", Ret);
#endif
	return(Ret);
}


int do_closesocket(SOCKET s) {
	WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, 0);
	UnregisterAsyncTable(s);
	DetachSSL(s);
	if (int result = closesocket(s); result != SOCKET_ERROR)
		return result;
	for (;;) {
		if (int error = 0; AskAsyncDone(s, &error, FD_CLOSE) == YES)
			return error == 0 ? 0 : SOCKET_ERROR;
		Sleep(1);
		if (BackgrndMessageProc() == YES)
			return SOCKET_ERROR;
	}
}


int do_connect(SOCKET s, const struct sockaddr *name, int namelen, int *CancelCheckWork)
{
#if USE_THIS
	int Ret;
	int Error;

#if DBG_MSG
	DoPrintf("# Start connect (S=%x)", s);
#endif
	// 同時接続対応
//	*CancelCheckWork = NO;

#if DBG_MSG
	DoPrintf("## Async set: FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ|FD_WRITE");
#endif
	// 高速化のためFD_READとFD_WRITEを使用しない
//	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE);
	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT);
	if(Ret != SOCKET_ERROR)
	{
		Ret = connect(s, name, namelen);
		if(Ret == SOCKET_ERROR)
		{
			do
			{
				Error = 0;
				while((*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_CONNECT) != YES))
				{
					Sleep(1);
					if(BackgrndMessageProc() == YES)
						*CancelCheckWork = YES;
				}

				if(*CancelCheckWork == YES)
					break;
				if(Error == 0)
					Ret = 0;
				else
				{
//					Error = WSAGetLastError();
					DoPrintf("#### Connect: Error=%d", Error);
				}
			}
			while((Ret != 0) && (Error == WSAEWOULDBLOCK));
		}
	}
	else
		DoPrintf("#### Connect: AsyncSelect error (%d)", WSAGetLastError());

#if DBG_MSG
	DoPrintf("# Exit connect (%d)", Ret);
#endif
	return(Ret);
#else
	return(connect(s, name, namelen));
#endif
}





int do_listen(SOCKET s,	int backlog)
{
	int Ret;

	Ret = 1;
#if DBG_MSG
	DoPrintf("# Start listen (S=%x)", s);
	DoPrintf("## Async set: FD_CLOSE|FD_ACCEPT");
#endif

	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CLOSE | FD_ACCEPT);
	if(Ret != SOCKET_ERROR)
		Ret = listen(s, backlog);

#if DBG_MSG
	DoPrintf("# Exit listen (%d)", Ret);
#endif
	return(Ret);
}



SOCKET do_accept(SOCKET s, struct sockaddr *addr, int *addrlen)
{
#if USE_THIS
	SOCKET Ret2;
	int CancelCheckWork;
	int Error;

#if DBG_MSG
	DoPrintf("# Start accept (S=%x)", s);
#endif
	CancelCheckWork = NO;
	Ret2 = INVALID_SOCKET;
	Error = 0;

	while((CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_ACCEPT) != YES))
	{
		if(AskAsyncDone(s, &Error, FD_CLOSE) == YES)
		{
			Error = 1;
			break;
		}
		Sleep(1);
		if(BackgrndMessageProc() == YES)
			CancelCheckWork = YES;
	}

	if((CancelCheckWork == NO) && (Error == 0))
	{
		do
		{
			Ret2 = accept(s, addr, addrlen);
			if(Ret2 != INVALID_SOCKET)
			{
#if DBG_MSG
				DoPrintf("## do_sccept (S=%x)", Ret2);
				DoPrintf("## Async set: FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ|FD_WRITE");
#endif
				RegisterAsyncTable(Ret2);
				// 高速化のためFD_READとFD_WRITEを使用しない
//				if(WSAAsyncSelect(Ret2, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE) == SOCKET_ERROR)
				if(WSAAsyncSelect(Ret2, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT) == SOCKET_ERROR)
				{
					do_closesocket(Ret2);
					Ret2 = INVALID_SOCKET;
				}
				break;
			}
			Error = WSAGetLastError();
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				break;
		}
		while(Error == WSAEWOULDBLOCK);
	}

#if DBG_MSG
	DoPrintf("# Exit accept");
#endif
	return(Ret2);
#else
	return(accept(s, addr, addrlen));
#endif
}


int do_recv(SOCKET s, char *buf, int len, int flags, int *TimeOutErr, int *CancelCheckWork) {
	if (*CancelCheckWork != NO)
		return SOCKET_ERROR;
	auto endTime = TimeOut != 0 ? std::make_optional(std::chrono::steady_clock::now() + std::chrono::seconds(TimeOut)) : std::nullopt;
	*TimeOutErr = NO;
	for (;;) {
		if (auto read = FTPS_recv(s, buf, len, flags); read != SOCKET_ERROR)
			return read;
		if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK)
			return SOCKET_ERROR;
		Sleep(1);
		if (BackgrndMessageProc() == YES)
			return SOCKET_ERROR;
		if (endTime && *endTime < std::chrono::steady_clock::now()) {
			DoPrintf("do_recv timed out");
			*TimeOutErr = YES;
			*CancelCheckWork = YES;
			return SOCKET_ERROR;
		}
		if (*CancelCheckWork == YES)
			return SOCKET_ERROR;
	}
}


int do_send(SOCKET s, const char* buf, int len, int flags, int* CancelCheckWork) {
	// バッファの構築、SSLの場合には暗号化を行う
	std::vector<char> work;
	std::string_view buffer(buf, len);
	if (auto context = getContext(s); context) {
		if (work = context->Encrypt(buffer); empty(work))
			return WSAEFAULT;
		buffer = std::string_view{ data(work), size(work) };
	}

	// SSLの場合には暗号化されたバッファなため、全てのデータを送信するまで繰り返す必要がある（途中で中断しても再開しようがない）
	if (*CancelCheckWork != NO)
		return WSAECONNABORTED;
	auto endTime = TimeOut != 0 ? std::make_optional(std::chrono::steady_clock::now() + std::chrono::seconds(TimeOut)) : std::nullopt;
	do {
		auto sent = send(s, data(buffer), size_as<int>(buffer), flags);
		if (0 < sent)
			buffer = buffer.substr(sent);
		else if (sent == 0) {
			_RPTW0(_CRT_WARN, L"do_send: connection closed.\n");
			return WSAEDISCON;
		} else if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
			_RPTWN(_CRT_WARN, L"do_send error: %d.\n", lastError);
			return lastError;
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES || *CancelCheckWork == YES)
			return WSAECONNABORTED;
		if (endTime && *endTime < std::chrono::steady_clock::now()) {
			DoPrintf("do_recv timed out");
			*CancelCheckWork = YES;
			return WSAETIMEDOUT;
		}
	} while (!empty(buffer));
	return ERROR_SUCCESS;
}


// 同時接続対応
void RemoveReceivedData(SOCKET s)
{
	char buf[1024];
	int len;
//	int Error;
	while((len = FTPS_recv(s, buf, sizeof(buf), MSG_PEEK)) > 0)
	{
//		AskAsyncDone(s, &Error, FD_READ);
		FTPS_recv(s, buf, len, 0);
	}
}

// UPnP対応
int LoadUPnP()
{
	int Sts;
	Sts = FFFTP_FAIL;
	if(IsMainThread())
	{
		if(CoCreateInstance(CLSID_UPnPNAT, NULL, CLSCTX_ALL, IID_IUPnPNAT, (void**)&pUPnPNAT) == S_OK)
		{
			if(pUPnPNAT->lpVtbl->get_StaticPortMappingCollection(pUPnPNAT, &pUPnPMap) == S_OK)
				Sts = FFFTP_SUCCESS;
		}
	}
	return Sts;
}

void FreeUPnP()
{
	if(IsMainThread())
	{
		if(pUPnPMap != NULL)
			pUPnPMap->lpVtbl->Release(pUPnPMap);
		pUPnPMap = NULL;
		if(pUPnPNAT != NULL)
			pUPnPNAT->lpVtbl->Release(pUPnPNAT);
		pUPnPNAT = NULL;
	}
}

int IsUPnPLoaded()
{
	int Sts;
	Sts = NO;
	if(pUPnPNAT != NULL && pUPnPMap != NULL)
		Sts = YES;
	return Sts;
}

int AddPortMapping(char* Adrs, int Port, char* ExtAdrs)
{
	int Sts;
	WCHAR Tmp1[40];
	BSTR Tmp2;
	BSTR Tmp3;
	BSTR Tmp4;
	IStaticPortMapping* pPortMap;
	BSTR Tmp5;
	ADDPORTMAPPINGDATA Data;
	Sts = FFFTP_FAIL;
	if(IsMainThread())
	{
		MtoW(Tmp1, 40, Adrs, -1);
		if((Tmp2 = SysAllocString(Tmp1)) != NULL)
		{
			if((Tmp3 = SysAllocString(L"TCP")) != NULL)
			{
				if((Tmp4 = SysAllocString(L"FFFTP")) != NULL)
				{
					if(pUPnPMap->lpVtbl->Add(pUPnPMap, Port, Tmp3, Port, Tmp2, VARIANT_TRUE, Tmp4, &pPortMap) == S_OK)
					{
						if(pPortMap->lpVtbl->get_ExternalIPAddress(pPortMap, &Tmp5) == S_OK)
						{
							WtoM(ExtAdrs, 40, Tmp5, -1);
							Sts = FFFTP_SUCCESS;
							SysFreeString(Tmp5);
						}
						pPortMap->lpVtbl->Release(pPortMap);
					}
					SysFreeString(Tmp4);
				}
				SysFreeString(Tmp3);
			}
			SysFreeString(Tmp2);
		}
	}
	else
	{
		if(Data.h = CreateEvent(NULL, TRUE, FALSE, NULL))
		{
			Data.Adrs = Adrs;
			Data.Port = Port;
			Data.ExtAdrs = ExtAdrs;
			if(PostMessage(GetMainHwnd(), WM_ADDPORTMAPPING, 0, (LPARAM)&Data))
			{
				if(WaitForSingleObject(Data.h, INFINITE) == WAIT_OBJECT_0)
					Sts = Data.r;
			}
			CloseHandle(Data.h);
		}
	}
	return Sts;
}

int RemovePortMapping(int Port)
{
	int Sts;
	BSTR Tmp;
	REMOVEPORTMAPPINGDATA Data;
	Sts = FFFTP_FAIL;
	if(IsMainThread())
	{
		if((Tmp = SysAllocString(L"TCP")) != NULL)
		{
			if(pUPnPMap->lpVtbl->Remove(pUPnPMap, Port, Tmp) == S_OK)
				Sts = FFFTP_SUCCESS;
			SysFreeString(Tmp);
		}
	}
	else
	{
		if(Data.h = CreateEvent(NULL, TRUE, FALSE, NULL))
		{
			Data.Port = Port;
			if(PostMessage(GetMainHwnd(), WM_REMOVEPORTMAPPING, 0, (LPARAM)&Data))
			{
				if(WaitForSingleObject(Data.h, INFINITE) == WAIT_OBJECT_0)
					Sts = Data.r;
			}
			CloseHandle(Data.h);
		}
	}
	return Sts;
}


/*----- 
*
*	Parameter
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int CheckClosedAndReconnect(void)
{
	int Error;
	int Sts;

//SetTaskMsg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	Sts = FFFTP_SUCCESS;
	if(AskAsyncDone(AskCmdCtrlSkt(), &Error, FD_CLOSE) == YES)
	{
		Sts = ReConnectCmdSkt();
	}
	return(Sts);
}



// 同時接続対応
int CheckClosedAndReconnectTrnSkt(SOCKET *Skt, int *CancelCheckWork)
{
	int Error;
	int Sts;

//SetTaskMsg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	Sts = FFFTP_SUCCESS;
	if(AskAsyncDone(*Skt, &Error, FD_CLOSE) == YES)
	{
		Sts = ReConnectTrnSkt(Skt, CancelCheckWork);
	}
	return(Sts);
}

