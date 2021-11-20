#include "Helpers.h"
#include <stdexcept>
#include <clocale>
#include <wchar.h>
#include <cstring>
#include <cassert>
#include <sstream>
#include <locale>
#include <codecvt>
#include <mutex>

namespace Jimara {
#pragma warning(disable: 4996)
	namespace {
		static std::mutex& wstring_convert_Lock() {
			static std::mutex lock;
			return lock;
		}
	}

	template<>
	std::wstring Convert<std::wstring>(const std::string_view& text) {
		std::unique_lock<std::mutex> lock(wstring_convert_Lock());
		return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(text.data());
	}

	template<>
	std::wstring Convert<std::wstring>(std::string&& text) {
		return Convert<std::wstring>(std::string_view(text));
	}

	template<>
	std::string Convert<std::string>(const std::wstring_view& text) {
		std::unique_lock<std::mutex> lock(wstring_convert_Lock());
		return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(text.data());
	}

	template<>
	std::string Convert<std::string>(std::wstring&& text) {
		return Convert<std::string>(std::wstring_view(text));
	}
#pragma warning(default: 4996)
}
