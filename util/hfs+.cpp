#include <charconv>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <cassert>
using namespace std::literals;

static auto hex(std::ssub_match const& sm) {
	int code;
	std::from_chars(&*sm.first, &*sm.second, code, 16);
	return code;
}

static auto hex(int code) {
	assert(0 <= code && code < 0x100);
	std::string hex{ R"(\x00)"sv };
	std::to_chars(data(hex) + (code < 0x10 ? 3 : 2), data(hex) + size(hex), code, 16);
	return hex;
}

static auto join(std::vector<std::string> const& terms) {
	if (size(terms) == 1)
		return terms.front();
	std::string pattern{ "(?:"sv };
	for (auto const& term : terms) {
		pattern += term;
		pattern += '|';
	}
	pattern[size(pattern) - 1] = ')';
	return pattern;
}

static auto classRange(int start, int last) {
	assert(0 <= start && start <= last);
	auto classRange = hex(start);
	if (start < last) {
		if (start + 1 < last)
			classRange += '-';
		classRange += hex(last);
	}
	return classRange;
}

static auto pattern(std::set<int> const& set) {
	assert(!empty(set));
	std::vector<std::string> atoms;
	if (size(set) == 1)
		atoms.emplace_back(hex(*begin(set)));
	else {
		std::string characterClass{ '[' };
		int start = -1, last = -1;
		for (auto code : set)
			if (start == -1)
				start = last = code;
			else if (code == last + 1)
				last = code;
			else {
				characterClass += classRange(start, last);
				start = last = code;
			}
		characterClass += classRange(start, last);
		characterClass += ']';
		atoms.emplace_back(characterClass);
	}
	return atoms;
}

template<class Map>
static auto pattern(Map const& map) {
	std::map<std::string, std::set<int>> group;
	for (auto [code, map] : map)
		group[join(pattern(map))].emplace(code);
	std::map<int, std::string> sort;
	for (auto [code, set] : group)
		sort.emplace(*begin(set), join(pattern(set)) + code);
	std::vector<std::string> result;
	for (auto [_, atom] : sort)
		result.emplace_back(atom);
	return result;
}

template<class Vector, class Map, class = std::enable_if_t<std::is_same_v<std::remove_cvref_t<Vector>, std::vector<std::string>>>>
static inline auto& operator<<(Vector&& atoms, Map const& map) {
	if (!empty(map)) {
		auto patterns = pattern(map);
		atoms.insert(end(atoms), begin(patterns), end(patterns));
	}
	return atoms;
}

static auto regex(std::set<int> const& codes) {
	std::set<int> u1;
	std::map<int, std::set<int>> u2;
	std::map<int, std::map<int, std::set<int>>> u3;
	std::map<int, std::map<int, std::map<int, std::set<int>>>> u4;
	for (auto code : codes) {
		assert(0 <= code && code <= 0x10'FFFF);
		if (code < 0x80)
			u1.emplace(code);
		else if (code < 0x800)
			u2[0xC0 | code >> 6].emplace(0x80 | code & 0x3F);
		else if (code < 0x1'0000)
			u3[0xE0 | code >> 12][0x80 | code >> 6 & 0x3F].emplace(0x80 | code & 0x3F);
		else
			u4[0xF0 | code >> 18][0x80 | code >> 12 & 0x3F][0x80 | code >> 6 & 0x3F].emplace(0x80 | code & 0x3F);
	}
	return join(std::vector<std::string>{} << u1 << u2 << u3 << u4);
}

int main() {
	std::set<int> nfc, nfd;
	{
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
	}
	std::cout << "NFC:"sv << std::endl << regex(nfc) << std::endl;
	std::cout << "NFD:"sv << std::endl << regex(nfd) << std::endl;
}
