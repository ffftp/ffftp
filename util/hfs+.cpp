// Copyright(C) Kurata Sayuri. All rights reserved.
// cl /std:c++latest /EHsc "hfs+.cpp"
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
using namespace std::literals;

static auto hex(std::ssub_match const& sm) {
	int code = std::stoi(sm, nullptr, 16);
	assert(0 < code && code <= 0x10'FFFF);
	return code;
}

static auto hex(int code) {
	return std::format(R"(\x{:02X})"sv, code);
}

static auto pattern(std::vector<int> const& set) {
	if (set.empty())
		return ""s;
	if (set.size() == 1)
		return hex(set.front());
	auto result = "["s;
	int start = -1, last = -1;
	for (auto code : set)
		if (code == last + 1) {
			if (start + 2 == code)
				result += '-';
			last = code;
		} else {
			if (start != -1 && start != last)
				result += hex(last);
			result += hex(start = last = code);
		}
	if (start != last)
		result += hex(last);
	result += ']';
	return result;
}

static auto pattern(auto const& map, bool wrap = true) {
	if (map.empty())
		return ""s;
	std::map<std::string, std::vector<int>> group;
	for (auto& [code, map] : map)
		group[pattern(map)].emplace_back(code);
	std::map<int, std::string> sort;
	for (auto& [code, set] : group)
		sort.emplace(set.front(), pattern(set) + code);
	if (sort.size() == 1)
		return sort.begin()->second;
	auto result = ""s;
	for (auto& [_, atom] : sort) {
		if (!result.empty())
			result += '|';
		result += atom;
	}
	return wrap ? std::format("(?:{})"sv, result) : result;
}

static auto regex(std::set<int> const& codes, bool wrap) {
	std::map<int, std::map<int, std::map<int, std::vector<int>>>> map;
	for (auto code : codes)
		if (code < 0x80)
			map[code];
		else if (code < 0x800)
			map[0xC0 | code >> 6][0x80 | code & 0x3F];
		else if (code < 0x1'0000)
			map[0xE0 | code >> 12][0x80 | code >> 6 & 0x3F][0x80 | code & 0x3F];
		else
			map[0xF0 | code >> 18][0x80 | code >> 12 & 0x3F][0x80 | code >> 6 & 0x3F].emplace_back(0x80 | code & 0x3F);
	return pattern(map, wrap);
}

int main() {
	std::set<int> nfc, nfd;
	std::regex re{ R"(^([0-9A-F]+);(?:[^;]*;){4}[0-9A-F]+(?: ([0-9A-F]+))?;)", std::regex_constants::optimize };
	// https://www.unicode.org/Public/UCD/latest/ucd/
	std::ifstream ud{ "UnicodeData.txt" };
	for (std::string line; getline(ud, line);)
		if (std::smatch m; std::regex_search(line, m, re))
			// https://developer.apple.com/library/archive/qa/qa1173/
			if (auto code = hex(m[1]); !(0x2000 <= code && code <= 0x2FFF || 0xF900 <= code && code <= 0xFAFF || 0x2F800 <= code && code <= 0x2FAFF)) {
				nfc.emplace(code);
				if (m[2].matched)
					nfd.emplace(hex(m[2]));
			}
	std::cout << "NFC:"sv << std::endl << regex(nfc, false) << std::endl;
	std::cout << "NFD:"sv << std::endl << regex(nfd, false) << std::endl;
}
