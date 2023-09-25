#pragma once
#include <functional>

namespace Jimara {
	/// <summary>
	/// Combines two hash values
	/// </summary>
	/// <param name="hashA"> First hash </param>
	/// <param name="hashB"> Second hash </param>
	/// <returns> 'Combined' hash </returns>
	inline static constexpr size_t MergeHashes(size_t hashA, size_t hashB) {
		return hashA ^ (hashB + 0x9e3779b9 + (hashA << 6) + (hashA >> 2)); 
	}

	/// <summary>
	/// Combines multiple hash values
	/// </summary>
	/// <typeparam name="...Rest"> Any number of size_t-s following hashC </typeparam>
	/// <param name="hashA"> First hash </param>
	/// <param name="hashB"> Second hash </param>
	/// <param name="hashC"> Third hash </param>
	/// <param name="...rest"> Fourth, fifth and so on </param>
	/// <returns> 'Combined' hash  </returns>
	template<typename... Rest>
	inline static constexpr size_t MergeHashes(size_t hashA, size_t hashB, size_t hashC, Rest... rest) {
		return MergeHashes(MergeHashes(hashA, hashB), hashC, rest...);
	}

	/// <summary>
	/// std::hash alternative for std::pair objects
	/// </summary>
	/// <typeparam name="A"> First template argument to std::pair </typeparam>
	/// <typeparam name="B"> Second template argument to std::pair </typeparam>
	/// <typeparam name="HashA"> Hasher for A </typeparam>
	/// <typeparam name="HashB"> Hasher for B </typeparam>
	template<typename A, typename B, typename HashA = std::hash<A>, typename HashB = std::hash<B>>
	struct PairHasher {
		/// <summary>
		/// Hash function
		/// </summary>
		/// <param name="pair"> Pair to hash </param>
		/// <returns> Hashed pair </returns>
		inline size_t operator()(const std::pair<A, B>& pair)const {
			return MergeHashes(HashA()(pair.first), HashB()(pair.second));
		}
	};

	/// <summary>
	/// std::equal_to alternative for std::pair objects
	/// </summary>
	/// <typeparam name="A"> First template argument to std::pair </typeparam>
	/// <typeparam name="B"> Second template argument to std::pair </typeparam>
	/// <typeparam name="EqualA"> Equality checker for A </typeparam>
	/// <typeparam name="EqualB"> Equality checker for B </typeparam>
	template<typename A, typename B, typename EqualA = std::equal_to<A>, typename EqualB = std::equal_to<B>>
	struct PairEquals {
		/// <summary>
		/// Checks, if the two pairs are equal
		/// </summary>
		/// <param name="a"> First pair </param>
		/// <param name="b"> Second pair </param>
		/// <returns> True, if and only if EqualA and EqualB return true </returns>
		inline size_t operator()(const std::pair<A, B>& a, const std::pair<A, B>& b)const {
			return EqualA()(a.first, b.first) && EqualB()(a.second, b.second);
		}
	};
}
