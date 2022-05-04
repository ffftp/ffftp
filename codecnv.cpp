// Copyright(C) Kurata Sayuri. All rights reserved.
#include "common.h"


// 改行コードをCRLFに変換
std::string ToCRLF(std::string_view source) {
	static boost::regex re{ R"(\r\n|[\r\n])" };
	std::string result;
	boost::regex_replace(back_inserter(result), begin(source), end(source), re, "\r\n");
	return result;
}


static auto mlang = LoadLibraryW(L"mlang.dll");
static auto convert(std::string_view input, DWORD incp) {
	static auto convertINetMultiByteToUnicode = reinterpret_cast<decltype(&ConvertINetMultiByteToUnicode)>(GetProcAddress(mlang, "ConvertINetMultiByteToUnicode"));
	static DWORD mode = 0;
	static INT scale = 1;
	for (std::wstring output;; ++scale) {
		auto inlen = size_as<INT>(input), outlen = inlen * scale;
		output.resize(outlen);
		if (auto const hr = convertINetMultiByteToUnicode(&mode, incp, data(input), &inlen, data(output), &outlen); hr == S_OK && inlen == size_as<INT>(input)) {
			output.resize(outlen);
			return output;
		}
	}
}
static auto convert(std::wstring_view input, DWORD outcp) {
	static auto convertINetUnicodeToMultiByte = reinterpret_cast<decltype(&ConvertINetUnicodeToMultiByte)>(GetProcAddress(mlang, "ConvertINetUnicodeToMultiByte"));
	static DWORD mode = 0;
	static INT scale = 2;
	for (std::string output;; ++scale) {
		auto inlen = size_as<INT>(input), outlen = inlen * scale;
		output.resize(outlen);
		if (auto const hr = convertINetUnicodeToMultiByte(&mode, outcp, data(input), &inlen, data(output), &outlen); hr == S_OK && inlen == size_as<INT>(input)) {
			output.resize(outlen);
			return output;
		}
	}
}
static auto convert(std::string_view input, DWORD incp, DWORD outcp, void (*update)(std::wstring&)) {
	auto wstr = convert(input, incp);
	update(wstr);
	return convert(wstr, outcp);
}


void CodeDetector::Test(std::string_view str) {
	static boost::regex reJISor8BIT{ R"(\x1B(\$[B@]|\([BHIJ]|\$\([DOPQ])|[\x80-\xFF])" },
		// Shift-JIS
		// JIS X 0208  1～8、16～47、48～84  81-84,88-9F,E0-EA
		// NEC特殊文字                       87
		// NEC選定IBM拡張文字                ED-EE
		// IBM拡張文字                       FA-FC
		reSJIS{ R"((?:[\x00-\x7F\xA1-\xDF]|(?:[\x81-\x84\x87-\x9F\xE0-\xEA\xED\xEE\xFA-\xFC]|([\x85\x86\xEB\xEC\xEF-\xF9]))[\x40-\x7E\x80-\xFC])*)" },
		reEUC{ R"((?:[\x00-\x7F]|\x8E[\xA1-\xDF]|(?:[\xA1-\xA8\xB0-\xF4]|([\xA9-\xAF\xF5-\xFE]|\x8F[\xA1-\xFE]))[\xA1-\xFE])*)" },
		// UTF-8
		// U+000000-U+00007F                   0b0000000-                 0b1111111  00-7F
		// U+000080-U+0007FF              0b00010'000000-            0b11111'111111  C2-DF             80-BF
		// U+000800-U+000FFF        0b0000'100000'000000-      0b0000'111111'111111  E0    A0-BF       80-BF
		// U+001000-U+00CFFF        0b0001'000000'000000-      0b1100'111111'111111  E1-EC       80-BF 80-BF
		// U+00D000-U+00D7FF        0b1101'000000'000000-      0b1101'011111'111111  ED    80-9F       80-BF
		// U+00D800-U+00DFFF        0b1101'100000'000000-      0b1101'111111'111111  ED    A0-BF       80-BF (High Surrogate Area, Low Surrogate Area)
		// U+00E000-U+00EFFF        0b1110'000000'000000-      0b1110'111111'111111  EE          80-BF 80-BF (Private Use Area)
		// U+00F000-U+00F8FF        0b1111'000000'000000-      0b1111'100011'111111  EF    80-A3       80-BF (Private Use Area)
		// U+00F900-U+00FFFF        0b1111'100100'000000-      0b1111'111111'111111  EF    A4-BF       80-BF
		// U+010000-U+03FFFF  0b000'010000'000000'000000-0b000'111111'111111'111111  F0    90-BF 80-BF 80-BF
		// U+040000-U+0BFFFF  0b001'000000'000000'000000-0b010'111111'111111'111111  F1-F2 80-BF 80-BF 80-BF
		// U+0C0000-U+0EFFFF  0b011'000000'000000'000000-0b011'101111'111111'111111  F3    80-AF 80-BF 80-BF
		// U+0F0000-U+0FFFFF  0b011'110000'000000'000000-0b011'111111'111111'111111  F3    B0-BF 80-BF 80-BF (Supplementary Private Use Area-A)
		// U+100000-U+10FFFF  0b100'000000'000000'000000-0b100'001111'111111'111111  F4    80-8F 80-BF 80-BF (Supplementary Private Use Area-B)
		reUTF8{ R"((?:[\x00-\x7F]|(?:[\xC2-\xDF]|\xE0[\xA0-\xBF]|\xED[\x80-\x9F]|\xEF[\xA4-\xBF]|(\xED[\xA0-\xBF]|\xEF[\x80-\xA3])|(?:[\xE1-\xEC]|\xF0[\x90-\xBF]|[\xF1\xF2][\x80-\xBF]|\xF3[\x80-\xAF]|(\xEE|\xF3[\xB0-\xBF]|\xF4[\x80-\x8F]))[\x80-\xBF])[\x80-\xBF])*)" },
		// HFS+はNFD正規化を行う。そのためNFCの文字は出現しない。このことを利用して、NFC文字を検出したら非HFS+、NFD文字を検出したらHFS+、どちらでもなければ不明として非HFS+と判定する。
		// これらの正規表現はutil/hfs+.cppで生成した。 Unicode 14.0
		reNFC{ R"(\xC3[\x80-\x85\x87-\x8F\x91-\x96\x99-\x9D\xA0-\xA5\xA7-\xAF\xB1-\xB6\xB9-\xBD\xBF]|\xC4[\x80-\x8F\x92-\xA5\xA8-\xB0\xB4-\xB7\xB9-\xBE]|\xC5[\x83-\x88\x8C-\x91\x94-\xA5\xA8-\xBE]|\xC6[\xA0\xA1\xAF\xB0]|\xC7[\x8D-\x9C\x9E-\xA3\xA6-\xB0\xB4\xB5\xB8-\xBF]|\xC8[\x80-\x9B\x9E\x9F\xA6-\xB3]|\xCD[\x80\x81\x83\x84\xB4\xBE]|\xCE[\x85-\x8A\x8C\x8E-\x90\xAA-\xB0]|\xCF[\x8A-\x8E\x93\x94]|\xD0[\x80\x81\x83\x87\x8C-\x8E\x99\xB9]|\xD1[\x90\x91\x93\x97\x9C-\x9E\xB6\xB7]|\xD3[\x81\x82\x90-\x93\x96\x97\x9A-\x9F\xA2-\xA7\xAA-\xB5\xB8\xB9]|\xD8[\xA2-\xA6]|\xDB[\x80\x82\x93]|\xE0(?:\xA4[\xA9\xB1\xB4]|\xA5[\x98-\x9F]|\xA7[\x8B\x8C\x9C\x9D\x9F]|\xA8[\xB3\xB6]|\xA9[\x99-\x9B\x9E]|\xAD[\x88\x8B\x8C\x9C\x9D]|\xAE\x94|[\xAF\xB5][\x8A-\x8C]|\xB1\x88|\xB3[\x80\x87\x88\x8A\x8B]|\xB7[\x9A\x9C-\x9E]|\xBD[\x83\x8D\x92\x97\x9C\xA9\xB3\xB5\xB6\xB8]|\xBE[\x81\x93\x9D\xA2\xA7\xAC\xB9])|\xE1(?:\x80\xA6|\xAC[\x86\x88\x8A\x8C\x8E\x92\xBB\xBD]|\xAD[\x80\x81\x83]|[\xB8\xB9][\x80-\xBF]|\xBA[\x80-\x99\x9B\xA0-\xBF]|\xBB[\x80-\xB9]|\xBC[\x80-\x95\x98-\x9D\xA0-\xBF]|\xBD[\x80-\x85\x88-\x8D\x90-\x97\x99\x9B\x9D\x9F-\xBD]|\xBE[\x80-\xB4\xB6-\xBC\xBE]|\xBF[\x81-\x84\x86-\x93\x96-\x9B\x9D-\xAF\xB2-\xB4\xB6-\xBD])|\xE3(?:\x81[\x8C\x8E\x90\x92\x94\x96\x98\x9A\x9C\x9E\xA0\xA2\xA5\xA7\xA9\xB0\xB1\xB3\xB4\xB6\xB7\xB9\xBA\xBC\xBD]|\x82[\x94\x9E\xAC\xAE\xB0\xB2\xB4\xB6\xB8\xBA\xBC\xBE]|\x83[\x80\x82\x85\x87\x89\x90\x91\x93\x94\x96\x97\x99\x9A\x9C\x9D\xB4\xB7-\xBA\xBE])|\xEF(?:\xAC[\x9D\x9F\xAA-\xB6\xB8-\xBC\xBE]|\xAD[\x80\x81\x83\x84\x86-\x8E])|\xF0(?:\x91(?:\x82[\x9A\x9C\xAB]|\x84[\xAE\xAF]|\x8D[\x8B\x8C]|\x92[\xBB\xBC\xBE]|\x96[\xBA\xBB]|\xA4\xB8)|\x9D(?:\x85[\x9E-\xA4]|\x86[\xBB-\xBF]|\x87\x80)))" },
		reNFD{ R"(\xCC[\x80-\x84\x86-\x8C\x8F\x91\x93\x94\x9B\xA3-\xA8\xAD\xAE\xB0\xB1]|\xCD[\x82\x85]|\xD6[\xB4\xB7-\xB9\xBC\xBF]|\xD7[\x81\x82]|\xD9[\x93-\x95]|\xE0(?:[\xA4\xA8]\xBC|[\xA6\xAC][\xBC\xBE]|[\xA7\xAF\xB5]\x97|\xAD[\x96\x97]|[\xAE\xB4]\xBE|\xB1\x96|\xB3[\x82\x95\x96]|\xB7[\x8A\x8F\x9F]|\xBD[\xB2\xB4]|\xBE[\x80\xB5\xB7])|\xE1(?:\x80\xAE|\xAC\xB5)|\xE3\x82[\x99\x9A]|\xF0(?:\x91(?:\x82\xBA|\x84\xA7|\x8C\xBE|\x8D\x97|\x92[\xB0\xBA\xBD]|\x96\xAF|\xA4\xB0)|\x9D\x85[\xA5\xAE-\xB2]))" };
	if (boost::match_results<std::string_view::const_iterator> mJISor8BIT; boost::regex_search(begin(str), end(str), mJISor8BIT, reJISor8BIT)) {
		if (mJISor8BIT[1].matched)
			jis += 2;
		else {
			if (boost::match_results<std::string_view::const_iterator> m; boost::regex_match(begin(str), end(str), m, reSJIS))
				sjis += m[1].matched ? 1 : 2;
			if (boost::match_results<std::string_view::const_iterator> m; boost::regex_match(begin(str), end(str), m, reEUC))
				euc += m[1].matched ? 1 : 2;
			if (boost::match_results<std::string_view::const_iterator> m; boost::regex_match(begin(str), end(str), m, reUTF8)) {
				utf8 += m[1].matched || m[2].matched ? 1 : 2;
				if (!nfc && !nfd) {
					if (boost::regex_search(begin(str), end(str), reNFC))
						nfc = true;
					else if (boost::regex_search(begin(str), end(str), reNFD))
						nfd = true;
				}
			}
		}
	}
}


static void fullwidth(std::wstring& str) {
	// 半角カタカナ ｡ \uFF61 ～ ﾟ \uFF9F
	static boost::wregex re{ LR"([｡-ﾟ]+)" };
	str = replace<wchar_t>(str, re, [](auto const& m) { return NormalizeString(NormalizationKC, { m[0].first, (size_t)m.length() }); });
}


static auto tohex(boost::cmatch const& m) {
	static char hex[] = "0123456789abcdef";
	std::string converted;
	for (auto it = m[0].first; it != m[0].second; ++it) {
		unsigned char const ch = *it;
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
		static boost::regex re{ R"(^(?:[\x00-\x7F\xA1-\xDF]|[\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]|[\x00-\xFF](?!$))*)" };
		boost::smatch m;
		boost::regex_search(rest, m, re);
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
		static boost::regex re{ R"(^([\x00-\xFF]*(\x1B\([BHIJ]|\x1B\$(?:[@B]|\([DOPQ]))(?:[\x00-\xFF]{2})*))" };
		if (boost::smatch m; boost::regex_search(rest, m, re)) {
			auto const isKanji = *(m[2].first + 1) == '$';
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
		static boost::regex re{ R"(^(?:[\x00-\x7F]|\x8E[\xA1-\xDF]|\x8F?[\xA1-\xFE][\xA1-\xFE]|[\x00-\xFF](?!$))*)" };
		boost::smatch m;
		boost::regex_search(rest, m, re);
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
		static boost::regex re{ R"(^[\x00-\xFF]*(?:[\x00-\x7F]|(?:[\xC0-\xDF]|(?:[\xE0-\xEF]|[\xF0-\xF4][\x80-\xBF])[\x80-\xBF])[\x80-\xBF]))" };
		boost::smatch m;
		boost::regex_search(rest, m, re);
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


std::wstring ConvertFrom(std::string_view str, int kanji) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, 932);
	case KANJI_JIS:
		return convert(str, 50221);
	case KANJI_EUC:
		return convert(str, 51932);
	case KANJI_SMB_HEX:
	case KANJI_SMB_CAP: {
		static boost::regex re{ R"(:([0-9A-Fa-f]{2}))" };
		auto decoded = replace(str, re, [](auto const& m) {
			char ch;
			std::from_chars(m[1].first, m[1].second, ch, 16);
			return ch;
		});
		return convert(decoded, 932);
	}
	case KANJI_UTF8HFSX:
		return NormalizeString(NormalizationC, convert(str, CP_UTF8));
	case KANJI_UTF8N:
	default:
		return convert(str, CP_UTF8);
	}
}


std::string ConvertTo(std::wstring_view str, int kanji, int kana) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, 932);
	case KANJI_JIS:
		if (kana) {
			std::wstring temp{ str };
			fullwidth(temp);
			return convert(temp, 50221);
		}
		return convert(str, 50221);
	case KANJI_EUC:
		if (kana) {
			std::wstring temp{ str };
			fullwidth(temp);
			return convert(temp, 51932);
		}
		return convert(str, 51932);
	case KANJI_SMB_HEX: {
		auto sjis = convert(str, 932);
		static boost::regex re{ R"((?:[\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]|[\xA1-\xDF])+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_SMB_CAP: {
		auto sjis = convert(str, 932);
		static boost::regex re{ R"([\x80-\xFF]+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_UTF8HFSX: {
		// TODO: fullwidth
		// HFS+のNFD対象外の範囲
		// U+02000–U+02FFF       2000-2FFF
		// U+0F900–U+0FAFF       F900-FAFF
		// U+2F800–U+2FAFF  D87E DC00-DEFF
		static boost::wregex re{ LR"((.*?)((?:[\x{2000}-\x{2FFF}\x{F900}-\x{FAFF}]|\x{D87E}[\x{DC00}-\x{DEFF}])+|$))" };
		auto const temp = replace<wchar_t>(str, re, [](auto const& m) { return NormalizeString(NormalizationD, sv(m[1])).append(sv(m[2])); });
		return convert(temp, CP_UTF8);
	}
	case KANJI_UTF8N:
	default:
		return convert(str, CP_UTF8);
	}
}
