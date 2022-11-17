#pragma once

namespace Jimara {
	/// <summary>
	/// Performs binary search with 'less than or equal to' logic on an arbitrary sorted list, given elem count and a comparator 
	/// <para/> Example usage: BinarySearch_LE(list.length(), result, [](size_t index) { return list[index] > value });
	/// </summary>
	/// <typeparam name="LessThanFn"> Any comparator that takes an index within the sequence as argumant and returns true, if list at that index is greater than the desired value </typeparam>
	/// <param name="elemCount"> Number of elements inside the 'list'/sequence </param>
	/// <param name="isLessThan"> Any comparator that takes an index within the sequence as argumant and returns true, if list at that index is greater than the desired value </param>
	/// <returns> 
	/// <para/> If element was found in the list, the index of said element;
	/// <para/> Otherwise, if we found indices i, i+1 such that (list[i] is less than value) and (list[i+1] is greater than value or (i + 1) >= elemCount), i will be returned;
	/// <para/> In case there is no value in the sequence less than what we seached for, size_t max is returned.
	/// </returns>
	template<typename LessThanFn>
	inline size_t BinarySearch_LE(size_t elemCount, const LessThanFn& isLessThan) {
		size_t regionStart = 0u;
		size_t regionEnd = elemCount;
		while ((regionStart + 1) < regionEnd) {
			const size_t regionMid = (regionStart + regionEnd) >> 1;
			if (isLessThan(regionMid))
				regionEnd = regionMid;
			else regionStart = regionMid;
		}
		if (regionStart <= 0 && isLessThan(regionStart))
			return ~size_t(0u);
		else return regionStart;
	}
}
