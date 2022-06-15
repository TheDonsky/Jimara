#pragma once
#include "JimaraApi.h"
#include <string>
#include <string_view>


namespace Jimara {
	/// <summary>
	/// To make sure, we don't have compiler warnings about unused parameters, you may just call Unused(...) with anything, you are not using
	/// </summary>
	/// <typeparam name="...Types"> Anything, really </typeparam>
	/// <param name="..."> Any number of values you are not intending to use </param>
	template<typename... Types>
	inline static void Unused(Types...) {}


	/// <summary>
	/// Converts a std::string_view to something else
	/// </summary>
	/// <typeparam name="Type"> Type to convert to </typeparam>
	/// <param name="text"> string_view </param>
	/// <returns> Converted </returns>
	template<typename Type>
	JIMARA_API inline Type Convert(const std::string_view& text) { return text.data(); }

	/// <summary>
	/// Converts a std::string_view (utf8) to std::wstring (utf16)
	/// </summary>
	/// <param name="text"> string_view </param>
	/// <returns> Converted </returns>
	template<>
	JIMARA_API std::wstring Convert<std::wstring>(const std::string_view& text);

	/// <summary>
	/// Moves string to something else
	/// </summary>
	/// <typeparam name="Type"> Type to move to </typeparam>
	/// <param name="text"> string out of scope </param>
	/// <returns> Moved value </returns>
	template<typename Type>
	JIMARA_API inline Type Convert(std::string&& text) { return Type(std::move(text)); }

	/// <summary>
	/// Moves std::string (utf8) to std::wstring (utf16)
	/// </summary>
	/// <param name="text"> string out of scope </param>
	/// <returns> Moved value </returns>
	template<>
	JIMARA_API std::wstring Convert<std::wstring>(std::string&& text);

	/// <summary>
	/// Converts a std::wstring_view to something else
	/// </summary>
	/// <typeparam name="Type"> Type to convert to </typeparam>
	/// <param name="text"> wstring_view </param>
	/// <returns> Converted </returns>
	template<typename Type>
	JIMARA_API inline Type Convert(const std::wstring_view& text) { return text.data(); }

	/// <summary>
	/// Converts a std::wstring_view (utf16) to std::string (utf8)
	/// </summary>
	/// <param name="text"> wstring_view </param>
	/// <returns> Converted </returns>
	template<>
	JIMARA_API std::string Convert<std::string>(const std::wstring_view& text);

	/// <summary>
	/// Moves std::wstring to something else
	/// </summary>
	/// <typeparam name="Type"> Type to move to </typeparam>
	/// <param name="text"> wstring out of scope </param>
	/// <returns> Moved value </returns>
	template<typename Type>
	JIMARA_API inline Type Convert(std::wstring&& text) { return std::move(text); }

	/// <summary>
	/// Moves std::wstring (utf16) to std::string (utf8)
	/// </summary>
	/// <param name="text"> wstring out of scope </param>
	/// <returns> Moved value </returns>
	template<>
	JIMARA_API std::string Convert<std::string>(std::wstring&& text);
}
