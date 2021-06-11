#pragma once

namespace Jimara {
	/// <summary>
	/// Combines two hash values
	/// </summary>
	/// <param name="hashA"> First hash </param>
	/// <param name="hashB"> Second hash </param>
	/// <returns> 'Combined' hash </returns>
	inline static size_t MergeHashes(size_t hashA, size_t hashB) {
		return hashA ^ (hashB + 0x9e3779b9 + (hashA << 6) + (hashA >> 2)); 
	}
}
