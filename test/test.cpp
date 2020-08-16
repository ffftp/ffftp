#include "../xp_mutex.h"							// include "xp_mutex.h" instead of <mutex>.
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <boost/regex.hpp>
#include <Windows.h>
#include <ImageHlp.h>
#include "CppUnitTest.h"
#pragma comment(lib,"ImageHlp.lib")
using namespace std::string_literals;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "../filelist.h"

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
				Assert::AreEqual(line[0] == ch, boost::regex_search(input, boost), message.c_str());
				Assert::AreEqual(line[0] == ch, std::regex_search(input, stl), message.c_str());
			}
		}
	}

	static inline std::once_flag CallOnce_flg0;
	static inline long CallOnce_global = 0;
	static void CallOnce_init0() {
		++CallOnce_global;
	}
	static void CallOnce_f0() {
		std::call_once(CallOnce_flg0, CallOnce_init0);
		Assert::AreEqual(1L, CallOnce_global, L"CallOnce");
	}
	TEST_METHOD(CallOnce) {
		std::thread t0(CallOnce_f0);
		std::thread t1(CallOnce_f0);
		t0.join();
		t1.join();
		Assert::AreEqual(1L, CallOnce_global, L"CallOnce");
	}

	template<class Fn>
	void load(const char* imageName, USHORT directoryEntry, Fn&& fn) {
		LOADED_IMAGE li;
		auto result = MapAndLoad(imageName, nullptr, &li, false, true);
		Assert::IsTrue(result && li.FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386);
		ULONG size;
		PIMAGE_SECTION_HEADER section;
		auto data = ImageDirectoryEntryToDataEx(li.MappedAddress, false, directoryEntry, &size, &section);
		Assert::IsTrue(data && section);
		fn(data, li.MappedAddress + section->PointerToRawData - section->VirtualAddress);
		UnMapAndLoad(&li);
	}
	TEST_METHOD(LinkXP) {
#if 0
		{
			std::ofstream f{ "xpexports.txt" };
			for (auto it : std::filesystem::directory_iterator{ LR"(xp)" })
				load(it.path().string().c_str(), IMAGE_DIRECTORY_ENTRY_EXPORT, [&f](const void* data, PUCHAR rva2va) {
					auto directory = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(data);
					auto names = reinterpret_cast<DWORD*>(rva2va + directory->AddressOfNames);
					for (DWORD i = 0; i < directory->NumberOfNames; ++i)
						f << reinterpret_cast<const char*>(rva2va + directory->Name) << ':' << reinterpret_cast<const char*>(rva2va + names[i]) << std::endl;
				});
		}
#endif
#if _WIN32_WINNT < _WIN32_WINNT_VISTA
		load((L"../../.." / std::filesystem::relative(L".", L"../..") / L"ffftp.exe").string().c_str(), IMAGE_DIRECTORY_ENTRY_IMPORT, [](const void* data, PUCHAR rva2va) {
			std::set<std::string> exports;
			std::ifstream f{ "xpexports.txt" };
			for (std::string line; getline(f, line);)
				exports.insert(line);
			for (auto descriptor = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(data); descriptor->Characteristics; descriptor++) {
				auto dll = reinterpret_cast<const char*>(rva2va + descriptor->Name) + ":"s;
				for (auto nameTable = reinterpret_cast<const IMAGE_THUNK_DATA32*>(rva2va + descriptor->OriginalFirstThunk); nameTable->u1.AddressOfData; nameTable++)
					if (!IMAGE_SNAP_BY_ORDINAL32(nameTable->u1.Ordinal)) {
						auto func = dll + reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(rva2va + nameTable->u1.AddressOfData)->Name;
						if (exports.find(func) == exports.end())
							Assert::Fail(ToString(func).c_str());
					}
			}
		});
#endif
	}
};
}
