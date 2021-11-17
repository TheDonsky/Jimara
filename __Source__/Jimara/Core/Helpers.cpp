#include "Helpers.h"
#include <clocale>
#include <wchar.h>
#include <cstring>
#include <cassert>
#include <mutex>

namespace Jimara {
#pragma warning(disable: 4996)
	namespace {
		static mbstate_t& GetPerThread_mbstate_t() {
			static thread_local mbstate_t threadState = []() -> mbstate_t {
				{
					static bool localeSet = false;
					static std::mutex localeLock;
					std::unique_lock<std::mutex> lock(localeLock);
					if (!localeSet) {
						setlocale(LC_ALL, "");
						localeSet = true;
					}
				}
				mbstate_t state;
				memset(&state, 0, sizeof(mbstate_t));
				return state;
			}();
			return threadState;
		}
	}

	template<>
	std::wstring Convert<std::wstring>(const std::string_view& text) {
		mbstate_t& state = GetPerThread_mbstate_t();
		const char* data = text.data();
		size_t count = mbsrtowcs(nullptr, &data, 0, &state);
		assert(count != ((size_t)-1));
		std::wstring result;
		result.resize(count);
		assert(mbsrtowcs(result.data(), &data, count + 1, &state) != ((size_t)-1));
		return result;
	}

	template<>
	std::wstring Convert<std::wstring>(std::string&& text) {
		return Convert<std::wstring>(std::string_view(text));
	}

	template<>
	std::string Convert<std::string>(const std::wstring_view& text) {
		mbstate_t& state = GetPerThread_mbstate_t();
		const wchar_t* data = text.data();
		size_t count = wcsrtombs(nullptr, &data, 0, &state);
		assert(count != ((size_t)-1));
		std::string result;
		result.resize(count);
		assert(wcsrtombs(result.data(), &data, count + 1, &state) != ((size_t)-1));
		return result;
	}

	template<>
	std::string Convert<std::string>(std::wstring&& text) {
		return Convert<std::string>(std::wstring_view(text));
	}
#pragma warning(default: 4996)
}
