#pragma warning(disable: 26426)		// error C26426 : Global initializer calls a non - constexpr function 'XXX' (i.22).
#pragma warning(disable: 26445)		// error C26445: Do not assign gsl::span or std::string_view to a reference. They are cheap to construct and are not owners of the underlying data. (gsl.view).
#pragma warning(disable: 26446)		// error C26446: Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#pragma warning(disable: 26481)		// error C26481: Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(disable: 26496)		// error C26496: The variable 'XXX' does not change after construction, mark it as const (con.4).
#include <fstream>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <boost/regex.hpp>
#include "../filelist.h"
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace test {
TEST_CLASS(ffftp) {
public:
	TEST_METHOD(FileListPraser) {
		auto compile = [](char ch, auto const& p) {
			auto [pattern, icase] = p;
			auto boost = icase ? boost::regex{ data(pattern), data(pattern) + size(pattern), boost::regex::icase } : boost::regex{ data(pattern), data(pattern) + size(pattern) };
			auto stl = icase ? std::regex{ data(pattern), data(pattern) + size(pattern),   std::regex::icase } : std::regex{ data(pattern), data(pattern) + size(pattern) };
			return std::tuple{ ch, pattern, boost, stl };
		};
		std::tuple<char, std::string_view, boost::regex, std::regex> const patterns[] = {
			compile('m', filelistparser::mlsd),
			compile('u', filelistparser::unix),
			compile('l', filelistparser::linux),
			compile('M', filelistparser::melcom80),
			compile('a', filelistparser::agilent),
			compile('d', filelistparser::dos),
			compile('c', filelistparser::chameleon),
			compile('2', filelistparser::os2),
			compile('b', filelistparser::allied),
			compile('s', filelistparser::shibasoku),
			compile('A', filelistparser::as400),
			compile('n', filelistparser::m1800),
			compile('g', filelistparser::gp6000),
			compile('7', filelistparser::os7),
			compile('9', filelistparser::os9),
			compile('i', filelistparser::ibm),
			compile('S', filelistparser::stratus),
			compile('v', filelistparser::vms),
			compile('I', filelistparser::irmx),
			compile('t', filelistparser::tandem),
		};

		std::ifstream is{ "filelist.txt" };
		Assert::IsFalse(is.fail(), L"Open failed.");
		for (std::string line; getline(is, line);) {
			if (line.size() < 2 || line[1] != '\t')
				continue;
			auto const input = line.substr(2);
			for (auto const [ch, pattern, boost, stl] : patterns) {
				auto message = ToString("expect: "s + line[0] + ", actual: " + ch + ", <<" + input + ">>");
				boost::smatch bm;
				std::smatch sm;
				Assert::AreEqual(line[0] == ch, boost::regex_search(input, bm, boost), message.c_str());
				Assert::AreEqual(line[0] == ch, std::regex_search(input, sm, stl), message.c_str());
				if (line[0] == ch) {
					auto match = line + '\t';
					for (auto const& m : bm | std::ranges::views::drop(1))
						if (m.matched)
							match += " [" + m.str() + "]";
						else
							match += " ()";
					Logger::WriteMessage((match + '\n').c_str());
				}
			}
		}
	}
};
}
