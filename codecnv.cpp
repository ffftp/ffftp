// Copyright(C) 2019 Kurata Sayuri. All rights reserved.
#include "common.h"


// 改行コードをCRLFに変換
std::string ToCRLF(std::string_view source) {
	static std::regex re{ R"(\r\n|[\r\n])" };
	std::string result;
	std::regex_replace(back_inserter(result), begin(source), end(source), re, "\r\n");
	return result;
}


static auto normalize(NORM_FORM form, std::wstring_view src) {
	// NormalizeString returns estimated buffer length.
	auto estimated = NormalizeString(form, data(src), size_as<int>(src), nullptr, 0);
	std::wstring normalized(estimated, L'\0');
	auto len = NormalizeString(form, data(src), size_as<int>(src), data(normalized), estimated);
	normalized.resize(len);
	return normalized;
}


static auto convert(std::string_view input, DWORD incp, DWORD outcp, void (*update)(std::wstring&)) {
	static auto mlang = LoadLibraryW(L"mlang.dll");
	static auto convertINetMultiByteToUnicode = reinterpret_cast<decltype(&ConvertINetMultiByteToUnicode)>(GetProcAddress(mlang, "ConvertINetMultiByteToUnicode"));
	static auto convertINetUnicodeToMultiByte = reinterpret_cast<decltype(&ConvertINetUnicodeToMultiByte)>(GetProcAddress(mlang, "ConvertINetUnicodeToMultiByte"));
	static DWORD mb2u = 0, u2mb = 0;
	static INT scale = 2;
	std::wstring wstr;
	for (;; ++scale) {
		auto inlen = size_as<INT>(input), wlen = inlen * scale;
		wstr.resize(wlen);
		auto hr = convertINetMultiByteToUnicode(&mb2u, incp, data(input), &inlen, data(wstr), &wlen);
		assert(hr == S_OK);
		if (inlen == size_as<INT>(input)) {
			wstr.resize(wlen);
			break;
		}
	}
	update(wstr);
	for (std::string output;; ++scale) {
		auto wlen = size_as<INT>(wstr), outlen = wlen * scale;
		output.resize(outlen);
		auto hr = convertINetUnicodeToMultiByte(&u2mb, outcp, data(wstr), &wlen, data(output), &outlen);
		assert(hr == S_OK);
		if (wlen == size_as<INT>(wstr)) {
			output.resize(outlen);
			return output;
		}
	}
}


void CodeDetector::Test(std::string_view str) {
	static std::regex reJISor8BIT{ R"(\x1B(\$[B@]|\([BHIJ]|\$\([DOPQ])|[\x80-\xFF])" },
		// Shift-JIS
		// JIS X 0208  1～8、16～47、48～84  81-84,88-9F,E0-EA
		// NEC特殊文字                       87
		// NEC選定IBM拡張文字                ED-EE
		// IBM拡張文字                       FA-FC
		reSJIS{ R"((?:[\x00-\x7F\xA1-\xDF]|(?:[\x81-\x84\x87-\x9F\xE0-\xEA\xED\xEE\xFA-\xFC]|([\x81-\x9F\xE0-\xFC]))[\x40-\x7E\x80-\xFC])*)" },
		reEUC{ R"((?:[\x00-\x7F]|\x8E[\xA1-\xDF]|(?:[\xA1-\xA8\xB0-\xF4]|(\x8F?[\xA1-\xFE]))[\xA1-\xFE])*)" },
		// UTF-8
		// U+000000-U+00007F                   0b0000000-                 0b1111111  00-7F
		// U+000080-U+0007FF              0b00010'000000-            0b11111'111111  C2-DF             80-BF
		// U+000800-U+000FFF        0b0000'100000'000000-      0b0000'111111'111111  E0    A0-BF       80-BF
		// U+001000-U+00FFFF        0b0001'000000'000000-      0b1111'111111'111111  E1-EF       80-BF 80-BF
		// U+003099-U+00309A        0b0011'000010'011001-      0b0011'000010'011010  E3    82    99-9A       (COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK, COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK)
		// U+00D800-U+00DFFF        0b1101'100000'000000-      0b1101'111111'111111  ED    A0-BF       80-BF (High Surrogate Area, Low Surrogate Area)
		// U+00E000-U+00EFFF        0b1110'000000'000000-      0b1110'111111'111111  EE          80-BF 80-BF (Private Use Area)
		// U+00F000-U+00F8FF        0b1111'000000'000000-      0b1111'100011'111111  EF    80-A3       80-BF (Private Use Area)
		// U+010000-U+03FFFF  0b000'010000'000000'000000-0b000'111111'111111'111111  F0    90-BF 80-BF 80-BF
		// U+040000-U+0FFFFF  0b001'000000'000000'000000-0b011'111111'111111'111111  F1-F3 80-BF 80-BF 80-BF
		// U+0F0000-U+0FFFFF  0b011'110000'000000'000000-0b011'111111'111111'111111  F3    B0-BF 80-BF 80-BF (Supplementary Private Use Area-A)
		// U+100000-U+10FFFF  0b100'000000'000000'000000-0b100'001111'111111'111111  F4    80-8F 80-BF 80-BF (Supplementary Private Use Area-B)
		reUTF8{ R"((?:[\x00-\x7F]|(\xE3\x82[\x99\x9A])|(?:(\xED[\xA0-\xBF]|\xEF[\x80-\xA3])|[\xC2-\xDF]|\xE0[\xA0-\xBF]|(?:(\xEE|\xF3[\xB0-\xBF]|\xF4[\x80-\x8F])|[\xE1-\xEF]|\xF0[\x90-\xBF]|[\xF1-\xF3][\x80-\xBF])[\x80-\xBF])[\x80-\xBF])*)" };
	if (std::match_results<std::string_view::const_iterator> m; std::regex_search(begin(str), end(str), m, reJISor8BIT)) {
		if (m[1].matched)
			jis += 2;
		else {
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reSJIS))
				sjis += m[1].matched ? 1 : 2;
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reEUC))
				euc += m[1].matched ? 1 : 2;
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reUTF8)) {
				utf8 += m[2].matched || m[3].matched ? 1 : 2;
				if (m[1].matched)
					hfsx = true;
			}
		}
	}
}


static void fullwidth(std::wstring& str) {
	static std::wregex re{ LR"([\uFF61-\uFF9F]+)" };
	str = replace<wchar_t>(str, re, [](auto const& m) { return normalize(NormalizationKC, { m[0].first, (size_t)m.length() }); });
}


static auto tohex(std::cmatch const& m) {
	static char hex[] = "0123456789abcdef";
	std::string converted;
	for (auto it = m[0].first; it != m[0].second; ++it) {
		unsigned char ch = *it;
		converted += ':';
		converted += hex[ch >> 4];
		converted += hex[ch & 15];
	}
	return converted;
}


std::string CodeConverter::Convert(std::string_view input) {
	if (incode == KANJI_NOCNV)
		return std::string(input);
	rest += input;
	switch (incode) {
	case KANJI_SJIS: {
		static std::regex re{ R"(^(?:[\x00-\x7F\xA1-\xDF]|[\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]|[\x00-\xFF](?!$))*)" };
		std::smatch m;
		std::regex_search(rest, m, re);
		auto source = m.str();
		rest = m.suffix();
		switch (outcode) {
		case KANJI_SJIS:
			return kana ? convert(source, 932, 932, fullwidth) : source;
		case KANJI_JIS:
			return convert(source, 932, 50221, kana ? fullwidth : [](auto) {});
		case KANJI_EUC:
			return convert(source, 932, 51932, kana ? fullwidth : [](auto) {});
		case KANJI_UTF8BOM:
			if (first) {
				first = false;
				return "\xEF\xBB\xBF"sv + convert(source, 932, CP_UTF8, [](auto) {});
			}
			[[fallthrough]];
		case KANJI_UTF8N:
			return convert(source, 932, CP_UTF8, [](auto) {});
		}
		break;
	}
	case KANJI_JIS: {
		// 0 : entire, 1 : kanji range, 2 : latest escape sequence
		static std::regex re{ R"(^([\x00-\xFF]*(\x1B\([BHIJ]|\x1B\$(?:[@B]|\([DOPQ]))(?:[\x00-\xFF]{2})*))" };
		if (std::smatch m; std::regex_search(rest, m, re)) {
			auto isKanji = *(m[2].first + 1) == '$';
			auto source = isKanji ? m[1].str() : rest;
			rest = isKanji ? m[2].str() + m.suffix().str() : m[2];
			switch (outcode) {
			case KANJI_SJIS:
				return convert(source, 50221, 932, kana ? fullwidth : [](auto) {});
			case KANJI_JIS:
				return kana ? convert(source, 50221, 50221, fullwidth) : source;
			case KANJI_EUC:
				return convert(source, 50221, 51932, kana ? fullwidth : [](auto) {});
			case KANJI_UTF8BOM:
				if (first) {
					first = false;
					return "\xEF\xBB\xBF"sv + convert(source, 50221, CP_UTF8, [](auto) {});
				}
				[[fallthrough]];
			case KANJI_UTF8N:
				return convert(source, 50221, CP_UTF8, [](auto) {});
			}
		} else {
			auto source = rest;
			rest.clear();
			if (outcode == KANJI_UTF8BOM && first) {
				first = false;
				return "\xEF\xBB\xBF"sv + source;
			}
			return source;
		}
		break;
	}
	case KANJI_EUC: {
		static std::regex re{ R"(^(?:[\x00-\x7F]|\x8E[\xA1-\xDF]|\x8F?[\xA1-\xFE][\xA1-\xFE]|[\x00-\xFF](?!$))*)" };
		std::smatch m;
		std::regex_search(rest, m, re);
		auto source = m.str();
		rest = m.suffix();
		switch (outcode) {
		case KANJI_SJIS:
			return convert(source, 51932, 932, kana ? fullwidth : [](auto) {});
		case KANJI_JIS:
			return convert(source, 51932, 50221, kana ? fullwidth : [](auto) {});
		case KANJI_EUC:
			return kana ? convert(source, 51932, 51932, fullwidth) : source;
		case KANJI_UTF8BOM:
			if (first) {
				first = false;
				return "\xEF\xBB\xBF"sv + convert(source, 51932, CP_UTF8, [](auto) {});
			}
			[[fallthrough]];
		case KANJI_UTF8N:
			return convert(source, 51932, CP_UTF8, [](auto) {});
		}
		break;
	}
	case KANJI_UTF8BOM:
		if (first) {
			if (size(rest) < 3)
				return {};
			if (rest.compare(0, 3, "\xEF\xBB\xBF"sv) == 0)
				rest.erase(0, 3);
			if (outcode != KANJI_UTF8BOM)
				first = false;
		}
		[[fallthrough]];
	case KANJI_UTF8N: {
		static std::regex re{ R"(^[\x00-\xFF]*(?:[\x00-\x7F]|(?:[\xC0-\xDF]|(?:[\xE0-\xEF]|[\xF0-\xF4][\x80-\xBF])[\x80-\xBF])[\x80-\xBF]))" };
		std::smatch m;
		std::regex_search(rest, m, re);
		auto source = m.str();
		rest = m.suffix();
		switch (outcode) {
		case KANJI_SJIS:
			return convert(source, CP_UTF8, 932, kana ? fullwidth : [](auto) {});
		case KANJI_JIS:
			return convert(source, CP_UTF8, 50221, kana ? fullwidth : [](auto) {});
		case KANJI_EUC:
			return convert(source, CP_UTF8, 51932, kana ? fullwidth : [](auto) {});
		case KANJI_UTF8BOM:
			if (first) {
				first = false;
				return "\xEF\xBB\xBF"sv + source;
			}
			[[fallthrough]];
		case KANJI_UTF8N:
			return source;
		}
		break;
	}
	}
	return {};
}


std::string ConvertFrom(std::string_view str, int kanji) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, 932, CP_UTF8, [](auto) {});
	case KANJI_JIS:
		return convert(str, 50221, CP_UTF8, [](auto) {});
	case KANJI_EUC:
		return convert(str, 51932, CP_UTF8, [](auto) {});
	case KANJI_SMB_HEX:
	case KANJI_SMB_CAP: {
		static std::regex re{ R"(:([0-9A-Fa-f]{2}))" };
		auto decoded = replace(str, re, [](auto const& m) {
			char ch;
			std::from_chars(m[1].first, m[1].second, ch, 16);
			return ch;
		});
		return convert(decoded, 932, CP_UTF8, [](auto) {});
	}
	case KANJI_UTF8HFSX:
		return convert(str, CP_UTF8, CP_UTF8, [](auto& str) { str = normalize(NormalizationC, str); });
	case KANJI_UTF8N:
	default:
		return std::string(str);
	}
}


std::string ConvertTo(std::string_view str, int kanji, int kana) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, CP_UTF8, 932, [](auto) {});
	case KANJI_JIS:
		return convert(str, CP_UTF8, 50221, kana ? fullwidth : [](auto) {});
	case KANJI_EUC:
		return convert(str, CP_UTF8, 51932, kana ? fullwidth : [](auto) {});
	case KANJI_SMB_HEX: {
		auto sjis = convert(str, CP_UTF8, 932, [](auto) {});
		static std::regex re{ R"((?:[\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]|[\xA1-\xDF])+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_SMB_CAP: {
		auto sjis = convert(str, CP_UTF8, 932, [](auto) {});
		static std::regex re{ R"([\x80-\xFF]+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_UTF8HFSX:
		// TODO: fullwidth
		return convert(str, CP_UTF8, CP_UTF8, [](auto& str) {
			// HFS+のNFD対象外の範囲
			// U+02000–U+02FFF       2000-2FFF
			// U+0F900–U+0FAFF       F900-FAFF
			// U+2F800–U+2FAFF  D87E DC00-DEFF
			static std::wregex re{ LR"((.*?)((?:[\u2000-\u2FFF\uF900-\uFAFF]|\uD87E[\uDC00-\uDEFF])+|$))" };
			str = replace<wchar_t>(str, re, [](auto const& m) { return normalize(NormalizationD, { m[1].first, (size_t)m.length(1) }).append(m[2].first, m[2].second); });
		});
	case KANJI_UTF8N:
	default:
		return std::string(str);
	}
}
