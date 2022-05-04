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
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Cryptui.lib")
#pragma comment(lib, "Secur32.lib")

static constexpr unsigned long contextReq = ISC_REQ_STREAM | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR | ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_MANUAL_CRED_VALIDATION;
static CredHandle credential = CreateInvalidateHandle<CredHandle>();


std::vector<char> SocketContext::Encrypt(std::string_view plain) {
	std::vector<char> result;
	while (!empty(plain)) {
		auto dataLength = std::min(size_as<unsigned long>(plain), sslStreamSizes.cbMaximumMessage);
		auto offset = size(result);
		result.resize(offset + sslStreamSizes.cbHeader + dataLength + sslStreamSizes.cbTrailer);
		std::copy_n(begin(plain), dataLength, begin(result) + offset + sslStreamSizes.cbHeader);
		SecBuffer buffer[]{
			{ sslStreamSizes.cbHeader,  SECBUFFER_STREAM_HEADER,  data(result) + offset                                        },
			{ dataLength,               SECBUFFER_DATA,           data(result) + offset + sslStreamSizes.cbHeader              },
			{ sslStreamSizes.cbTrailer, SECBUFFER_STREAM_TRAILER, data(result) + offset + sslStreamSizes.cbHeader + dataLength },
			{ 0,                        SECBUFFER_EMPTY,          nullptr                                                      },
		};
		SecBufferDesc desc{ SECBUFFER_VERSION, size_as<unsigned long>(buffer), buffer };
		if (auto const ss = EncryptMessage(&sslContext, 0, &desc, 0); ss != SEC_E_OK) {
			_RPTWN(_CRT_WARN, L"FTPS_send EncryptMessage error: %08x.\n", ss);
			return {};
		}
		assert(buffer[0].BufferType == SECBUFFER_STREAM_HEADER && buffer[0].cbBuffer == sslStreamSizes.cbHeader);
		assert(buffer[1].BufferType == SECBUFFER_DATA && buffer[1].cbBuffer == dataLength);
		assert(buffer[2].BufferType == SECBUFFER_STREAM_TRAILER && buffer[2].cbBuffer <= sslStreamSizes.cbTrailer);
		result.resize(offset + buffer[0].cbBuffer + buffer[1].cbBuffer + buffer[2].cbBuffer);
		plain = plain.substr(dataLength);
	}
	return result;
}

BOOL LoadSSL() {
	// 目的：
	//   TLS 1.1以前を無効化する動きに対応しつつ、古いSSL 2.0にもできるだけ対応する
	// 前提：
	//   Windows 7以前はTLS 1.1、TLS 1.2が既定で無効化されている。 <https://docs.microsoft.com/en-us/windows/desktop/SecAuthN/protocols-in-tls-ssl--schannel-ssp->
	//   それとは別にTLS 1.2とSSL 2.0は排他となる。
	//   ドキュメントに記載されていないUNI; Multi-Protocol Unified Helloが存在し、Windows XPではUNIが既定で有効化されている。さらにTLS 1.2とUNIは排他となる。
	//   TLS 1.3はWindows 10で実験的搭載されている。TLS 1.3を有効化するとTLS 1.3が使われなくてもセッション再開（TLS Resumption）が無効化される模様？
	// 手順：
	//   未指定でオープンすることで、レジストリ値に従った初期化をする。
	//   有効になっているプロトコルを調べ、SSL 2.0が無効かつTLS 1.2が無効な場合は開き直す。
	//   排他となるプロトコルがあるため、有効になっているプロトコルのうちSSL 3.0以降とTLS 1.2を指定してオープンする。
	//   セッション再開が必要とされるため、TLS 1.3は明示的には有効化せず、レジストリ指定に従う。
	static_assert(SP_PROT_TLS1_3PLUS_CLIENT == SP_PROT_TLS1_3_CLIENT, "new tls version detected.");
	if (auto const ss = AcquireCredentialsHandleW(nullptr, __pragma(warning(suppress:26465)) const_cast<wchar_t*>(UNISP_NAME_W), SECPKG_CRED_OUTBOUND, nullptr, nullptr, nullptr, nullptr, &credential, nullptr); ss != SEC_E_OK) {
		Error(L"AcquireCredentialsHandle()"sv, ss);
		return FALSE;
	}
	SecPkgCred_SupportedProtocols sp;
	if (__pragma(warning(suppress:6001)) auto const ss = QueryCredentialsAttributesW(&credential, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &sp); ss != SEC_E_OK) {
		Error(L"QueryCredentialsAttributes(SECPKG_ATTR_SUPPORTED_PROTOCOLS)"sv, ss);
		return FALSE;
	}
	if ((sp.grbitProtocol & SP_PROT_SSL2_CLIENT) == 0 && (sp.grbitProtocol & SP_PROT_TLS1_2_CLIENT) == 0) {
		FreeCredentialsHandle(&credential);
		// pAuthDataはSCHANNEL_CREDからSCH_CREDENTIALSに変更されたが現状維持する。
		// https://github.com/MicrosoftDocs/win32/commit/e9f333c14bad8fd65d89ccc64d42882bc5fa7d9c
		SCHANNEL_CRED sc{ .dwVersion = SCHANNEL_CRED_VERSION, .grbitEnabledProtocols = sp.grbitProtocol & SP_PROT_SSL3TLS1_X_CLIENTS | SP_PROT_TLS1_2_CLIENT };
		if (auto const ss = AcquireCredentialsHandleW(nullptr, __pragma(warning(suppress:26465)) const_cast<wchar_t*>(UNISP_NAME_W), SECPKG_CRED_OUTBOUND, nullptr, &sc, nullptr, nullptr, &credential, nullptr); ss != SEC_E_OK) {
			Error(L"AcquireCredentialsHandle()"sv, ss);
			return FALSE;
		}
	}
	return TRUE;
}

void FreeSSL() noexcept {
	assert(SecIsValidHandle(&credential));
	FreeCredentialsHandle(&credential);
}

namespace std {
	template<>
	struct default_delete<CERT_CONTEXT> {
		void operator()(CERT_CONTEXT* ptr) noexcept {
			CertFreeCertificateContext(ptr);
		}
	};
	template<>
	struct default_delete<const CERT_CHAIN_CONTEXT> {
		void operator()(const CERT_CHAIN_CONTEXT* ptr) noexcept {
			CertFreeCertificateChain(ptr);
		}
	};
}

auto getCertContext(CtxtHandle& context) noexcept {
	PCERT_CONTEXT certContext = nullptr;
	[[maybe_unused]] auto const ss = QueryContextAttributesW(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, &certContext);
#ifdef _DEBUG
	if (ss != SEC_E_OK)
		_RPTWN(_CRT_WARN, L"QueryContextAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT) error: %08X.\n", ss);
#endif
	return std::unique_ptr<CERT_CONTEXT>{ certContext };
}

void ShowCertificate() {
	if (auto const& sc = AskCmdCtrlSkt(); sc && sc->IsSSLAttached())
		if (auto certContext = getCertContext(sc->sslContext)) {
			CRYPTUI_VIEWCERTIFICATE_STRUCTW certViewInfo{ sizeof CRYPTUI_VIEWCERTIFICATE_STRUCTW, 0, CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE, nullptr, certContext.get() };
			__pragma(warning(suppress:6387)) CryptUIDlgViewCertificateW(&certViewInfo, nullptr);
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
	CERT_CONTEXT* const certContext;
	void OnCommand(HWND hdlg, WORD commandId) noexcept {
		switch (commandId) {
		case IDYES:
		case IDNO:
			EndDialog(hdlg, commandId);
			break;
		case IDC_SHOWCERT:
			CRYPTUI_VIEWCERTIFICATE_STRUCTW certViewInfo{ sizeof CRYPTUI_VIEWCERTIFICATE_STRUCTW, hdlg, CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE, nullptr, certContext };
			__pragma(warning(suppress:6387)) CryptUIDlgViewCertificateW(&certViewInfo, nullptr);
			break;
		}
	}
};

static CertResult ConfirmSSLCertificate(CtxtHandle& context, wchar_t* serverName, BOOL* pbAborted) {
	auto certContext = getCertContext(context);
	if (!certContext)
		return CertResult::Failed;

	auto chainContext = [&certContext]() noexcept {
		CERT_CHAIN_PARA chainPara{ sizeof(CERT_CHAIN_PARA) };
		PCCERT_CHAIN_CONTEXT chainContext;
		auto const result = CertGetCertificateChain(nullptr, certContext.get(), nullptr, nullptr, &chainPara, CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT, nullptr, &chainContext);
		return std::unique_ptr<const CERT_CHAIN_CONTEXT>{ result ? chainContext : nullptr };
	}();
	if (!chainContext)
		return CertResult::Failed;
	SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslPolicy{ sizeof(SSL_EXTRA_CERT_CHAIN_POLICY_PARA), AUTHTYPE_SERVER, 0, serverName };
	CERT_CHAIN_POLICY_PARA policyPara{ sizeof(CERT_CHAIN_POLICY_PARA), 0, &sslPolicy };
	CERT_CHAIN_POLICY_STATUS policyStatus{ sizeof(CERT_CHAIN_POLICY_STATUS) };
	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chainContext.get(), &policyPara, &policyStatus))
		return CertResult::Failed;
	if (policyStatus.dwError == 0)
		return CertResult::Secure;
	Notice(IDS_CERTERROR, policyStatus.dwError, GetErrorMessage(policyStatus.dwError));

	// thumbprint比較
	static std::vector<std::array<unsigned char, 20>> acceptedThumbprints;
	std::array<unsigned char, 20> thumbprint;
	if (auto size = size_as<DWORD>(thumbprint); !CertGetCertificateContextProperty(certContext.get(), CERT_HASH_PROP_ID, data(thumbprint), &size))
		return CertResult::Failed;
	if (std::find(begin(acceptedThumbprints), end(acceptedThumbprints), thumbprint) != end(acceptedThumbprints))
		return CertResult::NotSecureAccepted;

	if (Dialog(GetFtpInst(), certerr_dlg, GetMainHwnd(), CertDialog{ certContext.get() }) == IDYES) {
		acceptedThumbprints.push_back(thumbprint);
		return CertResult::NotSecureAccepted;
	}
	*pbAborted = YES;
	return CertResult::Declined;
}


template<class Test>
static inline std::invoke_result_t<Test> Wait(SocketContext& sc, int* CancelCheckWork, Test test) {
	for (auto f1 = gsl::finally([&sc]() noexcept { CancelIo((HANDLE)sc.handle); });;) {
		if (auto const result = test())
			return result;
		if (auto const result = sc.AsyncFetch(); result != 0 && result != WSA_IO_PENDING)
			return {};
		for (auto const expiredAt = std::chrono::steady_clock::now() + std::chrono::seconds{ TimeOut }; SleepEx(0, true) != WAIT_IO_COMPLETION;) {
			if (TimeOut != 0 && expiredAt < std::chrono::steady_clock::now()) {
				Notice(IDS_MSGJPN242);
				sc.recvStatus = WSAETIMEDOUT;
				return {};
			}
			if (BackgrndMessageProc() == YES || *CancelCheckWork == YES) {
				sc.recvStatus = ERROR_OPERATION_ABORTED;
				return {};
			}
		}
	}
}


// SSLセッションを開始
BOOL SocketContext::AttachSSL(BOOL* pbAborted) {
	assert(SecIsValidHandle(&credential));

	SecBuffer outBuffer[]{ { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr } };
	SecBufferDesc outDesc{ SECBUFFER_VERSION, size_as<unsigned long>(outBuffer), outBuffer };
	unsigned long attr = 0;
	sslReadStatus = InitializeSecurityContextW(&credential, nullptr, const_cast<SEC_WCHAR*>(punyTarget.c_str()), contextReq, 0, 0, nullptr, 0, &sslContext, &outDesc, &attr, nullptr);
	_RPTWN(_CRT_WARN, L"SC{%zu}: InitializeSecurityContextW(%d): %08X, out=%d/%d/%p, %d/%d/%p, attr=%X.\n", handle, size_as<int>(readRaw), sslReadStatus,
		outBuffer[0].BufferType, outBuffer[0].cbBuffer, outBuffer[0].pvBuffer, outBuffer[1].BufferType, outBuffer[1].cbBuffer, outBuffer[1].pvBuffer,
		attr
	);
	if (sslReadStatus != SEC_I_CONTINUE_NEEDED)
		return FALSE;
	assert(outBuffer[0].BufferType == SECBUFFER_TOKEN && outBuffer[0].cbBuffer != 0 && outBuffer[0].pvBuffer != nullptr);
	auto const written = send(handle, static_cast<const char*>(outBuffer[0].pvBuffer), outBuffer[0].cbBuffer, 0);
	_RPTWN(_CRT_WARN, L"SC{%zu}: send(): %d.\n", handle, written);
	assert(written == outBuffer[0].cbBuffer);
	__pragma(warning(suppress:6387)) FreeContextBuffer(outBuffer[0].pvBuffer);

	sslNeedRenegotiate = true;

	auto result = Wait(*this, pbAborted, [this]() -> std::optional<bool> {
		switch (sslReadStatus) {
		case SEC_E_OK:
			return true;
		case SEC_E_INCOMPLETE_MESSAGE:
		case SEC_I_CONTINUE_NEEDED:
			return {};
		default:
			return false;
		}
	});
	if (!result || !*result)
		return FALSE;

	if ((sslReadStatus = QueryContextAttributesW(&sslContext, SECPKG_ATTR_STREAM_SIZES, &sslStreamSizes)) != SEC_E_OK) {
		Error(L"QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES)"sv, sslReadStatus);
		return FALSE;
	}

	switch (ConfirmSSLCertificate(sslContext, const_cast<wchar_t*>(punyTarget.c_str()), pbAborted)) {
	case CertResult::Secure:
		sslSecure = true;
		break;
	case CertResult::NotSecureAccepted:
		sslSecure = false;
		break;
	default:
		return FALSE;
	}
	_RPTWN(_CRT_WARN, L"SC{%zu}: ssl attached.\n", handle);
	return TRUE;
}

bool IsSecureConnection() {
	auto const& sc = AskCmdCtrlSkt();
	return sc && sc->IsSSLAttached() && sc->sslSecure;
}


std::shared_ptr<SocketContext> SocketContext::Create(int af, std::variant<std::wstring_view, std::reference_wrapper<const SocketContext>> originalTarget) {
	auto s = socket(af, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		WSAError(L"socket()"sv);
		return {};
	}
	if (originalTarget.index() == 0) {
		auto target = std::get<0>(originalTarget);
		return std::make_shared<SocketContext>(s, std::wstring{ target }, IdnToAscii(target));
	} else {
		SocketContext const& target = std::get<1>(originalTarget);
		return std::make_shared<SocketContext>(s, target.originalTarget, target.punyTarget);
	}
}


SocketContext::SocketContext(SOCKET s, std::wstring originalTarget, std::wstring punyTarget) : WSAOVERLAPPED{}, handle { s }, originalTarget{ originalTarget }, punyTarget{ punyTarget } {}


SocketContext::~SocketContext() {
	if (int const result = closesocket(handle); result == SOCKET_ERROR)
		WSAError(L"closesocket()"sv);
	if (SecIsValidHandle(&sslContext))
		DeleteSecurityContext(&sslContext);
	Debug(L"Skt={} : Socket closed."sv, handle);
}


int SocketContext::Connect(const sockaddr* name, int namelen, int* CancelCheckWork) {
	if ((hEvent = WSACreateEvent()) == WSA_INVALID_EVENT) {
		WSAError(L"WSACreateEvent()"sv);
		return SOCKET_ERROR;
	}
	auto f1 = gsl::finally([this]() noexcept { WSACloseEvent(hEvent); });
	if (WSAEventSelect(handle, hEvent, FD_CONNECT | FD_CLOSE) != 0) {
		WSAError(L"WSAEventSelect()"sv);
		return SOCKET_ERROR;
	}
	auto f2 = gsl::finally([this]() noexcept { WSAEventSelect(handle, hEvent, 0); });
	if (connect(handle, name, namelen) == 0)
		return 0;
	if (auto const lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
		WSAError(L"connect()"sv, lastError);
		return SOCKET_ERROR;
	}
	for (;;) {
		auto const result = WSAWaitForMultipleEvents(1, &hEvent, false, 0, false);
		if (result == WSA_WAIT_EVENT_0)
			break;
		if (result != WSA_WAIT_TIMEOUT) {
			WSAError(L"WSAWaitForMultipleEvents()"sv);
			return SOCKET_ERROR;
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES)
			*CancelCheckWork = YES;
		if (*CancelCheckWork == YES)
			return SOCKET_ERROR;
	}
	if (WSANETWORKEVENTS networkEvents; WSAEnumNetworkEvents(handle, hEvent, &networkEvents) != 0) {
		WSAError(L"WSAEnumNetworkEvents()"sv);
		return SOCKET_ERROR;
	} else if ((networkEvents.lNetworkEvents & FD_CONNECT) == 0 || networkEvents.iErrorCode[FD_CONNECT_BIT] != 0) {
		Debug(L"networkEvents: {:08X}, connect: {}, close: {}."sv, (unsigned long)networkEvents.lNetworkEvents, networkEvents.iErrorCode[FD_CONNECT_BIT], networkEvents.iErrorCode[FD_CLOSE_BIT]);
		return SOCKET_ERROR;
	}
	return 0;
}


int SocketContext::Listen(int backlog) {
	if ((hEvent = WSACreateEvent()) == WSA_INVALID_EVENT) {
		WSAError(L"WSACreateEvent()"sv);
		return {};
	}
	if (WSAEventSelect(handle, hEvent, FD_ACCEPT | FD_CLOSE) != 0) {
		WSAError(L"WSAEventSelect()"sv);
		WSACloseEvent(hEvent);
		return SOCKET_ERROR;
	}
	if (listen(handle, backlog) != 0) {
		WSAError(L"listen()"sv);
		WSAEventSelect(handle, hEvent, 0);
		WSACloseEvent(hEvent);
		return SOCKET_ERROR;
	}
	return 0;
}


std::shared_ptr<SocketContext> SocketContext::Accept(_Out_writes_bytes_opt_(*addrlen) struct sockaddr* addr, _Inout_opt_ int* addrlen) {
	for (auto f1 = gsl::finally([this]() noexcept { WSAEventSelect(handle, hEvent, 0); WSACloseEvent(hEvent); });;) {
		auto const result = WSAWaitForMultipleEvents(1, &hEvent, false, 0, false);
		if (result == WSA_WAIT_EVENT_0) {
			if (WSANETWORKEVENTS networkEvents; WSAEnumNetworkEvents(handle, hEvent, &networkEvents) != 0) {
				WSAError(L"WSAEnumNetworkEvents()"sv);
				return {};
			} else if ((networkEvents.lNetworkEvents & FD_ACCEPT) == 0 || networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
				Debug(L"networkEvents: {:08X}, accept: {}, close: {}."sv, (unsigned long)networkEvents.lNetworkEvents, networkEvents.iErrorCode[FD_ACCEPT_BIT], networkEvents.iErrorCode[FD_CLOSE_BIT]);
				return {};
			}
			break;
		}
		if (result != WSA_WAIT_TIMEOUT) {
			WSAError(L"WSAWaitForMultipleEvents()"sv);
			return {};
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES)
			return {};
	}
	auto s = accept(handle, addr, addrlen);
	if (s == INVALID_SOCKET) {
		WSAError(L"accept()"sv);
		return {};
	}
	return std::make_shared<SocketContext>(s, originalTarget, punyTarget);
}


// AsyncFetchで使用されるコールバック。
void SocketContext::OnComplete(DWORD error, DWORD transferred, DWORD flags) {
	_RPTWN(_CRT_WARN, L"SC{%zu}: OnComplete(): error=%u, transferred=%u, flags=%u.\n", handle, error, transferred, flags);
	readRaw.resize((size_t)readRawSize + transferred);
	recvStatus = error;
	if (transferred == 0) {
		if (error == 0)
			recvStatus = ERROR_HANDLE_EOF;
		return;
	}
	if (!IsSSLAttached()) {
		readPlain.insert(end(readPlain), begin(readRaw), end(readRaw));
		readRaw.clear();
		return;
	}
	while (!empty(readRaw)) {
		if (sslNeedRenegotiate) {
			SecBuffer inBuffer[]{ { size_as<unsigned long>(readRaw), SECBUFFER_TOKEN, data(readRaw) }, { 0, SECBUFFER_EMPTY, nullptr } };
			SecBuffer outBuffer[]{ { 0, SECBUFFER_EMPTY, nullptr }, { 0, SECBUFFER_EMPTY, nullptr } };
			SecBufferDesc inDesc{ SECBUFFER_VERSION, size_as<unsigned long>(inBuffer), inBuffer };
			SecBufferDesc outDesc{ SECBUFFER_VERSION, size_as<unsigned long>(outBuffer), outBuffer };
			unsigned long attr = 0;
			sslReadStatus = InitializeSecurityContextW(&credential, &sslContext, const_cast<SEC_WCHAR*>(punyTarget.c_str()), contextReq, 0, 0, &inDesc, 0, nullptr, &outDesc, &attr, nullptr);
			_RPTWN(_CRT_WARN, L"SC{%zu}: InitializeSecurityContextW(%d): %08X, in=%d/%d/%p, %d/%d/%p, out=%d/%d/%p, %d/%d/%p, attr=%X.\n", handle, size_as<int>(readRaw), sslReadStatus,
				inBuffer[0].BufferType, inBuffer[0].cbBuffer, inBuffer[0].pvBuffer, inBuffer[1].BufferType, inBuffer[1].cbBuffer, inBuffer[1].pvBuffer,
				outBuffer[0].BufferType, outBuffer[0].cbBuffer, outBuffer[0].pvBuffer, outBuffer[1].BufferType, outBuffer[1].cbBuffer, outBuffer[1].pvBuffer,
				attr
			);
			if (sslReadStatus == SEC_E_OK || sslReadStatus == SEC_I_CONTINUE_NEEDED) {
				if (outBuffer[0].BufferType == SECBUFFER_TOKEN && outBuffer[0].cbBuffer != 0) {
					// TODO: 送信バッファが埋まっている場合に失敗する
					auto const written = send(handle, static_cast<const char*>(outBuffer[0].pvBuffer), outBuffer[0].cbBuffer, 0);
					_RPTWN(_CRT_WARN, L"SC{%zu}: send(): %d.\n", handle, written);
					assert(written == outBuffer[0].cbBuffer);
					FreeContextBuffer(outBuffer[0].pvBuffer);
				}
				readRaw.erase(begin(readRaw), end(readRaw) - (inBuffer[1].BufferType == SECBUFFER_EXTRA ? inBuffer[1].cbBuffer : 0));
				if (sslReadStatus == SEC_E_OK)
					sslNeedRenegotiate = false;
			} else if (sslReadStatus == SEC_E_INCOMPLETE_MESSAGE) {
				break;
			} else {
				Error(L"InitializeSecurityContextW()"sv, sslReadStatus);
				return;
			}
		} else {
			SecBuffer buffer[]{
				{ size_as<unsigned long>(readRaw), SECBUFFER_DATA, data(readRaw) },
				{ 0, SECBUFFER_EMPTY, nullptr },
				{ 0, SECBUFFER_EMPTY, nullptr },
				{ 0, SECBUFFER_EMPTY, nullptr },
			};
			SecBufferDesc desc{ SECBUFFER_VERSION, size_as<unsigned long>(buffer), buffer };
			sslReadStatus = DecryptMessage(&sslContext, &desc, 0, nullptr);
			_RPTWN(_CRT_WARN, L"SC{%zu}: DecryptMessage(%d): %08X, buf=%d/%d/%p, %d/%d/%p, %d/%d/%p, %d/%d/%p.\n", handle, size_as<int>(readRaw), sslReadStatus,
				buffer[0].BufferType, buffer[0].cbBuffer, buffer[0].pvBuffer, buffer[1].BufferType, buffer[1].cbBuffer, buffer[1].pvBuffer,
				buffer[2].BufferType, buffer[2].cbBuffer, buffer[2].pvBuffer, buffer[3].BufferType, buffer[3].cbBuffer, buffer[3].pvBuffer
			);
			if (sslReadStatus == SEC_E_OK) {
				assert(buffer[0].BufferType == SECBUFFER_STREAM_HEADER && buffer[1].BufferType == SECBUFFER_DATA && buffer[2].BufferType == SECBUFFER_STREAM_TRAILER);
				readPlain.insert(end(readPlain), static_cast<const char*>(buffer[1].pvBuffer), static_cast<const char*>(buffer[1].pvBuffer) + buffer[1].cbBuffer);
				readRaw.erase(begin(readRaw), end(readRaw) - (buffer[3].BufferType == SECBUFFER_EXTRA ? buffer[3].cbBuffer : 0));
			} else if (sslReadStatus == SEC_I_CONTEXT_EXPIRED) {
				assert(buffer[0].BufferType == SECBUFFER_DATA && buffer[0].cbBuffer == size_as<unsigned long>(readRaw));
				readRaw.clear();
				return;
			} else if (sslReadStatus == SEC_E_INCOMPLETE_MESSAGE) {
				break;
			} else if (sslReadStatus == SEC_I_RENEGOTIATE) {
				assert(buffer[0].BufferType == SECBUFFER_STREAM_HEADER && buffer[1].BufferType == SECBUFFER_DATA && buffer[1].cbBuffer == 0 && buffer[2].BufferType == SECBUFFER_STREAM_TRAILER && buffer[3].BufferType == SECBUFFER_EXTRA);
				readRaw.erase(begin(readRaw), end(readRaw) - buffer[3].cbBuffer);
				sslNeedRenegotiate = true;
			} else {
				Error(L"DecryptMessage()"sv, sslReadStatus);
				return;
			}
		}
	}
}


// 非同期読み込みを開始する。Alertable I/Oを使用しているので、呼び出し元はAPC queueを実行すること。
//   0 .................. 成功。既に完了している。
//   WSA_IO_PENDING ..... 成功。非同期実行が開始された。
//   ERROR_HANDLE_EOF ... 失敗。終端に達したため、これ以上読めない。
//   other .............. 失敗。
int SocketContext::AsyncFetch() {
	if (auto const status = GetReadStatus(); status != 0)
		return status;
	if (recvStatus == 0) {
		readRawSize = size_as<ULONG>(readRaw);
		readRaw.resize((size_t)readRawSize + recvlen);
		readRaw.resize(readRaw.capacity());
		WSABUF buf{ size_as<ULONG>(readRaw) - readRawSize, data(readRaw) + readRawSize };
		DWORD flag = 0;
		auto const result = WSARecv(handle, &buf, 1, nullptr, &flag, this, [](DWORD error, DWORD transferred, LPWSAOVERLAPPED overlapped, DWORD flags) { static_cast<SocketContext*>(overlapped)->OnComplete(error, transferred, flags); });
		recvStatus = result == 0 ? 0 : WSAGetLastError();
		_RPTWN(_CRT_WARN, L"SC{%zu}: WSARecv(): %d, %d.\n", handle, result, recvStatus);
		if (recvStatus != 0 && recvStatus != WSA_IO_PENDING)
			readRaw.resize(readRawSize);
	}
	return recvStatus;
}


// 現在の読み込みステータス。SSLとrecvを踏まえて決定される。
//   0 .................. まだ読めそう。
//   ERROR_HANDLE_EOF ... 終端に達したため、これ以上読めない。
//   other .............. SSLステータスもしくはrecvステータス。
int SocketContext::GetReadStatus() noexcept {
	switch (sslReadStatus) {
	case SEC_I_CONTEXT_EXPIRED:
		return ERROR_HANDLE_EOF;
	case SEC_E_OK:
	case SEC_E_INCOMPLETE_MESSAGE:
	case SEC_I_RENEGOTIATE:
	case SEC_I_CONTINUE_NEEDED:
		return recvStatus == WSA_IO_PENDING ? 0 : recvStatus;
	default:
		return sslReadStatus;
	}
}


std::tuple<int, std::wstring> SocketContext::ReadReply(int* CancelCheckWork) {
	static boost::regex re1{ R"(^[0-9]{3}[- ])" }, re2{ R"(^([0-9]{3})(?:-(?:[^\n]*\n)+?\1)? [^\n]*\n)" };
	static boost::wregex re3{ LR"((.*?)[ \r]*\n)" }, re4{ LR"([ \r]*\n[0-9]*)" };
	auto result = Wait(*this, CancelCheckWork, [this]() -> std::optional<std::tuple<int, std::wstring>> {
		if (4 <= size(readPlain) && !boost::regex_search(begin(readPlain), end(readPlain), re1))
			return { { 429, {} } };
		if (boost::match_results<decltype(readPlain)::iterator> m; boost::regex_search(begin(readPlain), end(readPlain), m, re2)) {
			auto code = std::stoi(m[1]);
			auto text = ConvertFrom(sv(m[0]), GetCurHost().CurNameKanjiCode);
			readPlain.erase(m[0].first, m[0].second);
			for (boost::wsregex_iterator it{ begin(text), end(text), re3 }, end; it != end; ++it)
				Notice(IDS_REPLY, sv((*it)[1]));
			text = boost::regex_replace(text, re4, L" ");
			return { { code, std::move(text) } };
		}
		return {};
	});
	return result.value_or(std::tuple{ 429, L""s });
}


bool SocketContext::ReadSpan(std::span<char>& span, int* CancelCheckWork) {
	auto const result = Wait(*this, CancelCheckWork, [this, &span] {
		if (size(readPlain) < size(span))
			return false;
		std::copy(begin(readPlain), begin(readPlain) + size(span), begin(span));
		readPlain.erase(begin(readPlain), begin(readPlain) + size(span));
		return true;
	});
	if (!result)
		Notice(IDS_MSGJPN244);
	return result;
}


int SocketContext::ReadAll(int* CancelCheckWork, std::function<bool(std::vector<char> const&)> callback) {
	Wait(*this, CancelCheckWork, [this, &callback] {
		auto result = callback(readPlain);
		readPlain.clear();
		return result;
	});
	return GetReadStatus();
}


void SocketContext::ClearReadBuffer() noexcept {
	assert(empty(readRaw));
	readPlain.clear();
}


int SocketContext::Send(const char* buf, int len, int flags, int* CancelCheckWork) {
	if (len <= 0)
		return FFFTP_SUCCESS;

	// バッファの構築、SSLの場合には暗号化を行う
	std::vector<char> work;
	std::string_view buffer{ buf, size_t(len) };
	if (IsSSLAttached()) {
		if (work = Encrypt(buffer); empty(work)) {
			Debug(L"SendData: EncryptMessage failed."sv);
			return FFFTP_FAIL;
		}
		buffer = { data(work), size(work) };
	}

	// SSLの場合には暗号化されたバッファなため、全てのデータを送信するまで繰り返す必要がある（途中で中断しても再開しようがない）
	if (*CancelCheckWork != NO)
		return FFFTP_FAIL;
	auto endTime = TimeOut != 0 ? std::optional{ std::chrono::steady_clock::now() + std::chrono::seconds(TimeOut) } : std::nullopt;
	do {
		auto const sent = send(handle, data(buffer), size_as<int>(buffer), flags);
		if (0 < sent)
			buffer = buffer.substr(sent);
		else if (sent == 0) {
			Debug(L"SendData: send(): connection closed."sv);
			return FFFTP_FAIL;
		} else if (auto const lastError = WSAGetLastError(); lastError != WSAEWOULDBLOCK) {
			Error(L"SendData: send()"sv, lastError);
			return FFFTP_FAIL;
		}
		Sleep(1);
		if (BackgrndMessageProc() == YES || *CancelCheckWork == YES)
			return FFFTP_FAIL;
		if (endTime && *endTime < std::chrono::steady_clock::now()) {
			Notice(IDS_MSGJPN241);
			*CancelCheckWork = YES;
			return FFFTP_FAIL;
		}
	} while (!empty(buffer));
	return FFFTP_SUCCESS;
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

int IsUPnPLoaded() noexcept {
	return upnpNAT && staticPortMappingCollection ? YES : NO;
}


std::optional<std::wstring> AddPortMapping(std::wstring const& internalAddress, int port) {
	static _bstr_t TCP{ L"TCP" };
	static _bstr_t FFFTP{ L"FFFTP" };
	struct Data : public MainThreadRunner {
		long port;
		std::wstring const& internalAddress;
		_bstr_t externalAddress;
		Data(std::wstring const& internalAddress, long port) noexcept : port{ port }, internalAddress{ internalAddress } {}
		int DoWork() override {
			ComPtr<IStaticPortMapping> staticPortMapping;
			auto result = staticPortMappingCollection->Add(port, TCP, port, _bstr_t{ internalAddress.c_str() }, VARIANT_TRUE, FFFTP, &staticPortMapping);
			if (result == S_OK)
				result = staticPortMapping->get_ExternalIPAddress(externalAddress.GetAddress());
			return result;
		}
	} data{ internalAddress, port };
	if (auto const result = (HRESULT)data.Run(); result == S_OK)
		return { { (const wchar_t*)data.externalAddress, data.externalAddress.length() } };
	return {};
}

bool RemovePortMapping(int port) {
	static _bstr_t TCP{ L"TCP" };
	struct Data : public MainThreadRunner {
		long port;
		Data(long port) noexcept : port{ port } {}
		int DoWork() override {
			return staticPortMappingCollection->Remove(port, TCP);
		}
	} data{ port };
	auto const result = (HRESULT)data.Run();
	return result == S_OK;
}


int CheckClosedAndReconnect() {
	if (!AskCmdCtrlSkt())
		return ReConnectCmdSkt();
	return FFFTP_SUCCESS;
}


int CheckClosedAndReconnectTrnSkt(std::shared_ptr<SocketContext>& Skt, int* CancelCheckWork) {
	if (!Skt)
		return ReConnectTrnSkt(Skt, CancelCheckWork);
	return FFFTP_SUCCESS;
}
