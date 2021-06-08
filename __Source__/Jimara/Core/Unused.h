#pragma once


namespace Jimara {
	/// <summary>
	/// To make sure, we don't have compiler warnings about unused parameters, you may just call Unused(...) with anything, you are not using
	/// </summary>
	/// <typeparam name="...Types"> Anything, really </typeparam>
	/// <param name="..."> Any number of values you are not intending to use </param>
	template<typename... Types>
	inline static void Unused(Types...) {}
}
