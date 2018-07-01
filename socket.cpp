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
#include <cryptuiapi.h>
#include <natupnp.h>
#include <schannel.h>
#include <security.h>
#ifndef SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3_CLIENT 0x00002000
#endif
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Cryptui.lib")
#pragma comment(lib, "Secur32.lib")


#define USE_THIS	1
#define DBG_MSG		0


struct AsyncSignal {
	int Event;
	int Error;
	std::variant<sockaddr_storage, std::tuple<std::string, int>> Target;
	int MapPort;
};


/*===== プロトタイプ =====*/

static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int AskAsyncDone(SOCKET s, int *Error, int Mask);
static void RegisterAsyncTable(SOCKET s);
static void UnregisterAsyncTable(SOCKET s);


/*===== 外部参照 =====*/

extern int TimeOut;


/*===== ローカルなワーク =====*/

static HWND hWndSocket;
static std::map<SOCKET, AsyncSignal> Signal;
static std::mutex SignalMutex;


template<class T>
static T CreateInvalidateHandle() {
	T handle;
	SecInvalidateHandle(&handle);
	return std::move(handle);
}

struct Context {
	std::wstring host;
	CtxtHandle context;
	const bool secure;
	SecPkgContext_StreamSizes streamSizes;
	std::vector<char> readRaw;
	std::vector<char> readPlain;
	SECURITY_STATUS readStatus = SEC_E_OK;
	Context(std::wstring const& host, CtxtHandle context, bool secure, SecPkgContext_StreamSizes streamSizes, std::vector<char> extra) : host{ host }, context{ context }, secure{ secure }, streamSizes { streamSizes }, readRaw{ extra } {
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
	// 目的：
	//   TLS 1.1以前を無効化する動きに対応しつつ、古いSSL 2.0にもできるだけ対応する
	// 前提：
	//   Windows 7以前はTLS 1.1、TLS 1.2が既定で無効化されている。 <https://docs.microsoft.com/en-us/windows/desktop/SecAuthN/protocols-in-tls-ssl--schannel-ssp->
	//   それとは別にTLS 1.2とSSL 2.0は排他となる。 <https://docs.microsoft.com/en-us/windows/desktop/api/schannel/ns-schannel-_schannel_cred>
	//   ドキュメントに記載されていないUNI; Multi-Protocol Unified Helloが存在し、Windows XPではUNIが既定で有効化されている。さらにTLS 1.2とUNIは排他となる。
	//   またドキュメントにはないがTLSとDTLSも排他となる。同じくドキュメントにないがTLS 1.3は現時点で未対応。
	// 手順：
	//   未指定でオープンすることで、レジストリ値に従った初期化をする。
	//   有効になっているプロトコルを調べ、SSL 2.0が無効かつTLS 1.2が無効な場合は開き直す。
	//   排他となるプロトコルがあるため、有効になっているプロトコルのうちSSL 3.0以降とTLS 1.2を指定してオープンする。
	static_assert((SP_PROT_TLS1_1PLUS_CLIENT & ~(SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT)) == 0, "new tls version detected.");
	if (auto ss = AcquireCredentialsHandleW(nullptr, UNISP_NAME_W, SECPKG_CRED_OUTBOUND, nullptr, nullptr, nullptr, nullptr, &credential, nullptr); ss != SEC_E_OK) {
		_RPTWN(_CRT_WARN, L"AcquireCredentialsHandle error: %08X.\n", ss);
		return FALSE;
	}
	SecPkgCred_SupportedProtocols sp;
	if (auto ss = QueryCredentialsAttributesW(&credential, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &sp); ss != SEC_E_OK) {
		_RPTWN(_CRT_WARN, L"QueryCredentialsAttributes error: %08X.\n", ss);
		return FALSE;
	}
	if ((sp.grbitProtocol & SP_PROT_SSL2_CLIENT) == 0 && (sp.grbitProtocol & SP_PROT_TLS1_2_CLIENT) == 0) {
		FreeCredentialsHandle(&credential);
		SCHANNEL_CRED sc{ SCHANNEL_CRED_VERSION };
		sc.grbitEnabledProtocols = sp.grbitProtocol & (SP_PROT_SSL3_CLIENT | SP_PROT_TLS1_0_CLIENT | SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT) | SP_PROT_TLS1_2_CLIENT;
		if (auto ss = AcquireCredentialsHandleW(nullptr, UNISP_NAME_W, SECPKG_CRED_OUTBOUND, nullptr, &sc, nullptr, nullptr, &credential, nullptr); ss != SEC_E_OK) {
			_RPTWN(_CRT_WARN, L"AcquireCredentialsHandle error: %08X.\n", ss);
			return FALSE;
		}
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

auto getCertContext(CtxtHandle& context) {
	PCERT_CONTEXT certContext = nullptr;
	[[maybe_unused]] auto ss = QueryContextAttributesW(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &certContext);
#ifdef _DEBUG
	if (ss != SEC_E_OK)
		_RPTWN(_CRT_WARN, L"QueryContextAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT) error: %08X.\n", ss);
#endif
	return std::unique_ptr<CERT_CONTEXT>{ certContext };
}

void ShowCertificate() {
	if (auto context = getContext(AskCmdCtrlSkt()))
		if (auto certContext = getCertContext(context->context)) {
			CRYPTUI_VIEWCERTIFICATE_STRUCT certViewInfo{ sizeof CRYPTUI_VIEWCERTIFICATE_STRUCT, 0, CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE, nullptr, certContext.get() };
			CryptUIDlgViewCertificate(&certViewInfo, nullptr);
		}
}

enum class CertResult {
	Secure,
	NotSecureAccepted,
	Declined,
	Failed = Declined,
};

struct CertDialog {
	using result_t = int;
	std::unique_ptr<CERT_CONTEXT> const& certContext;
	CertDialog(std::unique_ptr<CERT_CONTEXT> const& certContext) : certContext{ certContext } {}
	INT_PTR OnCommand(HWND hdlg, WORD commandId) {
		switch (commandId) {
		case IDYES:
		case IDNO:
			EndDialog(hdlg, commandId);
			break;
		case IDC_SHOWCERT:
			CRYPTUI_VIEWCERTIFICATE_STRUCTW certViewInfo{ sizeof CRYPTUI_VIEWCERTIFICATE_STRUCTW, hdlg, CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE, nullptr, certContext.get() };
			CryptUIDlgViewCertificateW(&certViewInfo, nullptr);
			break;
		}
		return 0;
	}
};

static CertResult ConfirmSSLCertificate(CtxtHandle& context, wchar_t* serverName, BOOL* pbAborted) {
	auto certContext = getCertContext(context);
	if (!certContext)
		return CertResult::Failed;

	CERT_CHAIN_PARA chainPara{ sizeof CERT_CHAIN_PARA };
	PCCERT_CHAIN_CONTEXT chainContext;
	if (!CertGetCertificateChain(nullptr, certContext.get(), nullptr, nullptr, &chainPara, CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT, nullptr, &chainContext)) {
		_RPTWN(_CRT_WARN, L"CertGetCertificateChain error: %08X.\n", GetLastError());
		return CertResult::Failed;
	}
	SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslPolicy{ sizeof SSL_EXTRA_CERT_CHAIN_POLICY_PARA , AUTHTYPE_SERVER, 0, serverName };
	CERT_CHAIN_POLICY_PARA policyPara{ sizeof CERT_CHAIN_POLICY_PARA, 0, &sslPolicy };
	CERT_CHAIN_POLICY_STATUS policyStatus{ sizeof CERT_CHAIN_POLICY_STATUS };
	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chainContext, &policyPara, &policyStatus)) {
		_RPTW0(_CRT_WARN, L"CertVerifyCertificateChainPolicy: not able to check.\n");
		return CertResult::Failed;
	}
	if (policyStatus.dwError == 0)
		return CertResult::Secure;
	_RPTWN(_CRT_WARN, L"CertVerifyCertificateChainPolicy result: %08X.\n", policyStatus.dwError);

	// thumbprint比較
	static std::vector<std::array<unsigned char, 20>> acceptedThumbprints;
	std::array<unsigned char, 20> thumbprint;
	if (auto size = size_as<DWORD>(thumbprint); !CertGetCertificateContextProperty(certContext.get(), CERT_HASH_PROP_ID, data(thumbprint), &size))
		return CertResult::Failed;
	if (std::find(begin(acceptedThumbprints), end(acceptedThumbprints), thumbprint) != end(acceptedThumbprints))
		return CertResult::NotSecureAccepted;

	if (Dialog(GetFtpInst(), certerr_dlg, GetMainHwnd(), CertDialog{ certContext }) == IDYES) {
		acceptedThumbprints.push_back(thumbprint);
		return CertResult::NotSecureAccepted;
	}
	*pbAborted = YES;
	return CertResult::Declined;
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
		wServerName = IdnToAscii(u8(ServerName));
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
		constexpr unsigned long contextReq = ISC_REQ_STREAM | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR | ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_MANUAL_CRED_VALIDATION;
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
					DoPrintf("AttachSSL recv: connection closed.");
					return FALSE;
				} else if (0 < read) {
					_RPTWN(_CRT_WARN, L"AttachSSL recv: %d bytes.\n", read);
					extra.insert(end(extra), buffer, buffer + read);
					break;
				}
				if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
					DoPrintf("AttachSSL recv error: %d.", lastError);
					return FALSE;
				}
				Sleep(0);
			}
			inBuffer[0] = { size_as<unsigned long>(extra), SECBUFFER_TOKEN, data(extra) };
			ss = InitializeSecurityContextW(&credential, &context, node, contextReq, 0, 0, &inDesc, 0, nullptr, &outDesc, &attr, nullptr);
		}
		if (FAILED(ss) && ss != SEC_E_INCOMPLETE_MESSAGE) {
			DoPrintf("AttachSSL InitializeSecurityContext error: %08x.", ss);
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

	if (ss = QueryContextAttributesW(&context, SECPKG_ATTR_STREAM_SIZES, &streamSizes); ss != SEC_E_OK) {
		DoPrintf("AttachSSL QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES) error: %08x.", ss);
		return FALSE;
	}

	bool secure;
	switch (ConfirmSSLCertificate(context, node, pbAborted)) {
	case CertResult::Secure:
		secure = true;
		break;
	case CertResult::NotSecureAccepted:
		secure = false;
		break;
	default:
		return FALSE;
	}
	std::lock_guard<std::mutex> lock_guard{ context_mutex };
	contexts.emplace(std::piecewise_construct, std::forward_as_tuple(s), std::forward_as_tuple(wServerName, context, secure, streamSizes, extra));
	_RPTW0(_CRT_WARN, L"AttachSSL success.\n");
	return TRUE;
}

bool IsSecureConnection() {
	auto context = getContext(AskCmdCtrlSkt());
	return context && context->secure;
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
#ifdef _DEBUG
			if (read == 0)
				_RPTW0(_CRT_WARN, L"FTPS_recv recv: connection closed.\n");
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
		switch (context->readStatus) {
		case SEC_I_CONTEXT_EXPIRED:
			return 0;
		case SEC_E_INCOMPLETE_MESSAGE:
			// recvできたデータが少なすぎてフレームの解析・デコードができず、復号データが得られないというエラー。
			// ブロッキングが発生するというエラーに書き換える。
			WSASetLastError(WSAEWOULDBLOCK);
			return SOCKET_ERROR;
		default:
			_RPTWN(_CRT_WARN, L"FTPS_recv readStatus: %08X.\n", context->readStatus);
			return SOCKET_ERROR;
		}
	len = std::min(len, size_as<int>(context->readPlain));
	std::copy_n(begin(context->readPlain), len, buf);
	if ((flags & MSG_PEEK) == 0)
		context->readPlain.erase(begin(context->readPlain), begin(context->readPlain) + len);
	_RPTWN(_CRT_WARN, L"FTPS_recv read: %d bytes.\n", len);
	return len;
}


int MakeSocketWin(HWND hWnd, HINSTANCE hInst) {
	auto const className = L"FFFTPSocketWnd";
	WNDCLASSEXW wcx{ sizeof(WNDCLASSEXW), 0, SocketWndProc, 0, 0, hInst, nullptr, nullptr, CreateSolidBrush(GetSysColor(COLOR_INFOBK)), nullptr, className, };
	RegisterClassExW(&wcx);
	if (hWndSocket = CreateWindowExW(0, className, nullptr, WS_BORDER | WS_POPUP, 0, 0, 0, 0, hWnd, nullptr, hInst, nullptr)) {
		Signal.clear();
		return FFFTP_SUCCESS;
	}
	return FFFTP_FAIL;
}


void DeleteSocketWin() {
	if (hWndSocket)
		DestroyWindow(hWndSocket);
}


static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_ASYNC_SOCKET) {
		std::lock_guard lock{ SignalMutex };
		if (auto it = Signal.find(wParam); it != end(Signal)) {
			it->second.Error = WSAGETSELECTERROR(lParam);
			it->second.Event |= WSAGETSELECTEVENT(lParam);
		}
		return 0;
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}


static int AskAsyncDone(SOCKET s, int *Error, int Mask) {
	*Error = 0;
	std::lock_guard lock{ SignalMutex };
	if (auto it = Signal.find(s); it != end(Signal)) {
		if (*Error = it->second.Error)
			return YES;
		if (it->second.Event & Mask) {
			if (Mask & FD_ACCEPT)
				it->second.Event &= ~FD_ACCEPT;
			else if (Mask & FD_READ)
				it->second.Event &= ~FD_READ;
			else if (Mask & FD_WRITE)
				it->second.Event &= ~FD_WRITE;
			return YES;
		}
		return NO;
	}
	if (Mask & FD_CLOSE)
		return YES;
	MessageBox(GetMainHwnd(), "AskAsyncDone called with unregisterd socket.", "FFFTP inner error", MB_OK);
	exit(1);
}


static void RegisterAsyncTable(SOCKET s) {
	std::lock_guard lock{ SignalMutex };
	Signal[s] = {};
}


static void UnregisterAsyncTable(SOCKET s) {
	std::lock_guard lock{ SignalMutex };
	Signal.erase(s);
}


void SetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::string, int>> const& target) {
	std::lock_guard lock{ SignalMutex };
	if (auto it = Signal.find(s); it != end(Signal))
		it->second.Target = target;
}

void SetAsyncTableDataMapPort(SOCKET s, int Port) {
	std::lock_guard lock{ SignalMutex };
	if (auto it = Signal.find(s); it != end(Signal))
		it->second.MapPort = Port;
}

int GetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::string, int>>& target) {
	std::lock_guard lock{ SignalMutex };
	if (auto it = Signal.find(s); it != end(Signal)) {
		target = it->second.Target;
		return YES;
	}
	return NO;
}

int GetAsyncTableDataMapPort(SOCKET s, int* Port) {
	std::lock_guard lock{ SignalMutex };
	if (auto it = Signal.find(s); it != end(Signal)) {
		*Port = it->second.MapPort;
		return YES;
	}
	return NO;
}


SOCKET do_socket(int af, int type, int protocol) {
	auto s = socket(af, type, protocol);
	if (s == INVALID_SOCKET) {
		DoPrintf("socket: socket failed: 0x%08X", WSAGetLastError());
		return INVALID_SOCKET;
	}
	RegisterAsyncTable(s);
	return s;
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


int do_connect(SOCKET s, const sockaddr* name, int namelen, int* CancelCheckWork) {
	if (WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT) != 0) {
		DoPrintf("connect: WSAAsyncSelect failed: 0x%08X", WSAGetLastError());
		return SOCKET_ERROR;
	}
	if (connect(s, name, namelen) == 0)
		return 0;
	if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
		DoPrintf("connect: connect failed: 0x%08X", WSAGetLastError());
		return SOCKET_ERROR;
	}
	while (*CancelCheckWork != YES) {
		if (int error = 0; AskAsyncDone(s, &error, FD_CONNECT) == YES && error != WSAEWOULDBLOCK) {
			if (error == 0)
				return 0;
			DoPrintf("connect: select error: 0x%08X", error);
			return SOCKET_ERROR;
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES)
			*CancelCheckWork = YES;
	}
	return SOCKET_ERROR;
}


int do_listen(SOCKET s, int backlog) {
	if (WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CLOSE | FD_ACCEPT) != 0) {
		DoPrintf("listen: WSAAsyncSelect failed: 0x%08X", WSAGetLastError());
		return SOCKET_ERROR;
	}
	if (listen(s, backlog) != 0) {
		DoPrintf("listen: listen failed: 0x%08X", WSAGetLastError());
		return SOCKET_ERROR;
	}
	return 0;
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


int SendData(SOCKET s, const char* buf, int len, int flags, int* CancelCheckWork) {
	if (s == INVALID_SOCKET)
		return FFFTP_FAIL;
	if (len <= 0)
		return FFFTP_SUCCESS;

	// バッファの構築、SSLの場合には暗号化を行う
	std::vector<char> work;
	std::string_view buffer{ buf, size_t(len) };
	if (auto context = getContext(s)) {
		if (work = context->Encrypt(buffer); empty(work)) {
			DoPrintf("send: EncryptMessage failed.");
			return FFFTP_FAIL;
		}
		buffer = { data(work), size(work) };
	}

	// SSLの場合には暗号化されたバッファなため、全てのデータを送信するまで繰り返す必要がある（途中で中断しても再開しようがない）
	if (*CancelCheckWork != NO)
		return FFFTP_FAIL;
	auto endTime = TimeOut != 0 ? std::optional{ std::chrono::steady_clock::now() + std::chrono::seconds(TimeOut) } : std::nullopt;
	do {
		auto sent = send(s, data(buffer), size_as<int>(buffer), flags);
		if (0 < sent)
			buffer = buffer.substr(sent);
		else if (sent == 0) {
			DoPrintf("send: connection closed.");
			return FFFTP_FAIL;
		} else if (auto lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
			DoPrintf("send: send failed: 0x%08X", lastError);
			return FFFTP_FAIL;
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES || *CancelCheckWork == YES)
			return FFFTP_FAIL;
		if (endTime && *endTime < std::chrono::steady_clock::now()) {
			SetTaskMsg(MSGJPN241);
			*CancelCheckWork = YES;
			return FFFTP_FAIL;
		}
	} while (!empty(buffer));
	return FFFTP_SUCCESS;
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
static ComPtr<IUPnPNAT> upnpNAT;
static ComPtr<IStaticPortMappingCollection> staticPortMappingCollection;

int LoadUPnP() {
	if (IsMainThread())
		if (CoCreateInstance(CLSID_UPnPNAT, NULL, CLSCTX_ALL, IID_PPV_ARGS(&upnpNAT)) == S_OK)
			if (upnpNAT->get_StaticPortMappingCollection(&staticPortMappingCollection) == S_OK)
				return FFFTP_SUCCESS;
	return FFFTP_FAIL;
}

void FreeUPnP() {
	if (IsMainThread()) {
		staticPortMappingCollection.Reset();
		upnpNAT.Reset();
	}
}

int IsUPnPLoaded() {
	return upnpNAT && staticPortMappingCollection ? YES : NO;
}

int AddPortMapping(const char* Adrs, int Port, char* ExtAdrs) {
	static _bstr_t TCP{ L"TCP" };
	static _bstr_t FFFTP{ L"FFFTP" };
	int result = FFFTP_FAIL;
	if (IsMainThread()) {
		if (ComPtr<IStaticPortMapping> staticPortMapping; staticPortMappingCollection->Add(Port, TCP, Port, _bstr_t{ Adrs }, VARIANT_TRUE, FFFTP, &staticPortMapping) == S_OK)
			if (_bstr_t buffer; staticPortMapping->get_ExternalIPAddress(buffer.GetAddress()) == S_OK) {
				strcpy(ExtAdrs, buffer);
				return FFFTP_SUCCESS;
			}
	} else {
		if (ADDPORTMAPPINGDATA Data; Data.h = CreateEvent(NULL, TRUE, FALSE, NULL)) {
			Data.Adrs = Adrs;
			Data.Port = Port;
			Data.ExtAdrs = ExtAdrs;
			if (PostMessage(GetMainHwnd(), WM_ADDPORTMAPPING, 0, (LPARAM)&Data))
				if (WaitForSingleObject(Data.h, INFINITE) == WAIT_OBJECT_0)
					result = Data.r;
			CloseHandle(Data.h);
		}
	}
	return result;
}

int RemovePortMapping(int Port) {
	static _bstr_t TCP{ L"TCP" };
	int result = FFFTP_FAIL;
	if (IsMainThread()) {
		if (staticPortMappingCollection->Remove(Port, TCP) == S_OK)
			return FFFTP_SUCCESS;
	} else {
		if (REMOVEPORTMAPPINGDATA Data; Data.h = CreateEvent(NULL, TRUE, FALSE, NULL)) {
			Data.Port = Port;
			if (PostMessage(GetMainHwnd(), WM_REMOVEPORTMAPPING, 0, (LPARAM)&Data))
				if (WaitForSingleObject(Data.h, INFINITE) == WAIT_OBJECT_0)
					result = Data.r;
			CloseHandle(Data.h);
		}
	}
	return result;
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

