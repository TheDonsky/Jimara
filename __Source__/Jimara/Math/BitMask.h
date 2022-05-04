#pragma once
#include <atomic>
#include <stdint.h>
#include "Helpers.h"

namespace Jimara {
	/// <summary>
	/// Bitmask, with arbitrary bit width bit indices
	/// </summary>
	/// <typeparam name="BitId"> Bit index type (normally, something like uint8_t) </typeparam>
	template<typename BitId>
	class BitMask {
	private:
		// Underlying data
		std::atomic<uint64_t> m_words[(static_cast<size_t>(1) << (sizeof(BitId) << 3u)) / (sizeof(uint64_t) << 3u)];

		// Number of elements in m_words array
		inline static constexpr size_t NumWords() noexcept { return (sizeof(static_cast<BitMask*>(nullptr)->m_words) / sizeof(uint64_t)); }

		// Element in m_words array for the given bit
		inline static constexpr size_t Word(BitId bit) { return bit / (sizeof(uint64_t) << 3u); }

		// Bit, that corresponds to the value of the bit within m_words[Word(bit)]
		inline static constexpr uint64_t Bit(BitId bit) { return static_cast<uint64_t>(1) << static_cast<uint64_t>(bit & ((sizeof(uint64_t) << 3u) - 1)); }

		// Yep, if we want to use this one with unordered collections, a fast hash will kinda benefit from access
		friend struct std::hash<BitMask>;

	public:
		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="mask"> Mask to copy </param>
		inline constexpr BitMask(const BitMask& mask) { for (size_t i = 0; i < NumWords(); i++) m_words[i].store(mask.m_words[i]); }

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="mask"> Mask to copy </param>
		inline constexpr BitMask& operator=(const BitMask& mask) { for (size_t i = 0; i < NumWords(); i++) m_words[i].store(mask.m_words[i]); return (*this); }

		/// <summary> Constructor (same as Empty) </summary>
		inline constexpr BitMask() { for (size_t i = 0; i < NumWords(); i++) m_words[i] = 0; }

		/// <summary>
		/// Modifiable reference to the bit, corresponding to a bit index
		/// </summary>
		class BitReference {
		private:
			// Word
			std::atomic<uint64_t>* const m_word;

			// Bit, corresponding to the bit id inside the word
			const uint64_t m_bit;

			// Constructor
			inline constexpr BitReference(std::atomic<uint64_t>* word, uint64_t bit) : m_word(word), m_bit(bit) {}

			// Only the BitMask can construct BitReference
			friend class BitMask;

		public:
			/// <summary> Type cast to the boolean value </summary>
			inline constexpr operator bool()const { return ((*m_word) & m_bit) != 0; }

			/// <summary>
			/// Sets corresponding bit
			/// </summary>
			/// <param name="value"> If true, the bit if will be inculded in the bitmask </param>
			/// <returns> self </returns>
			inline constexpr BitReference& operator=(bool value) { if (value) (*m_word) |= m_bit; else (*m_word) &= (~m_bit); return (*this); }

			/// <summary>
			/// Sets corresponding bit to true, if the value is true
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline constexpr BitReference& operator|=(bool value) { if (value) (*this) = true; return (*this); }

			/// <summary>
			/// Sets corresponding bit to false, if the value is false
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline constexpr BitReference& operator&=(bool value) { if (!value) (*this) = false; return (*this); }

			/// <summary>
			/// Sets the bit to exclusive or of this and the value
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline constexpr BitReference& operator^=(bool value) { (*this) = ((*this) ^ false); return (*this); }
		};

		/// <summary>
		/// Checks if the bit is included in the mask
		/// </summary>
		/// <param name="bit"> Bit id to check </param>
		/// <returns> True, if the flag, corresponding to the bit id is set </returns>
		inline constexpr bool operator[](BitId bit)const { return (m_words[Word(bit)] & Bit(bit)) != 0; }

		/// <summary>
		/// Reference to the bit, corresponding to the bit index
		/// </summary>
		/// <param name="bit"> Bit id of interest </param>
		/// <returns> Bit reference </returns>
		inline constexpr BitReference operator[](BitId bit) { return BitReference(m_words + Word(bit), Bit(bit)); }

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="...Rest"> A bounch of bit indices </typeparam>
		/// <param name="first"> First bit index to include </param>
		/// <param name="...rest"> Rest of the bit indices to include </param>
		template<typename... Rest>
		inline constexpr BitMask(BitId first, Rest... rest) : BitMask(rest...) { (*this)[first] = true; }

		/// <summary>
		/// Bitmask, consisting of all bit indices, that are included in either this or the other masks
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline constexpr BitMask operator|(const BitMask& other)const { BitMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] | other.m_words[i]; return result; }

		/// <summary>
		/// Includes all bit indices, that are a part of the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline constexpr BitMask& operator|=(const BitMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] |= other.m_words[i]; return (*this); }

		/// <summary>
		/// Bitmask, consisting of the bit indices, that are included in both this and the other masks
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline constexpr BitMask operator&(const BitMask& other)const { BitMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] & other.m_words[i]; return result; }

		/// <summary>
		/// Excludes all bit indices, that are not included in the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline constexpr BitMask& operator&=(const BitMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] &= other.m_words[i]; return (*this); }

		/// <summary>
		/// Logical exclusive-or of this and the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline constexpr BitMask operator^(const BitMask& other)const { BitMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] ^ other.m_words[i]; return result; }

		/// <summary>
		/// Sets to logical exclusive-or of this and the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline constexpr BitMask& operator^=(const BitMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] ^= other.m_words[i]; return (*this); }

		/// <summary>
		/// Inverts the bitmask
		/// </summary>
		/// <returns> Bitmask, including none of the bit indices within this and all that are not a part of this mask </returns>
		inline constexpr BitMask operator~()const { BitMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = (~m_words[i]); return result; }


		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'less than' to the other </returns>
		inline constexpr bool operator<(const BitMask& other)const {
			for (size_t i = 0; i < NumWords(); i++) {
				uint64_t a = m_words[i], b = other.m_words[i];
				if (a < b) return true;
				else if (a > b) return false;
			}
			return false;
		}

		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'less than or equal to' to the other </returns>
		inline constexpr bool operator<=(const BitMask& other)const {
			for (size_t i = 0; i < NumWords(); i++) {
				uint64_t a = m_words[i], b = other.m_words[i];
				if (a < b) return true;
				else if (a > b) return false;
			}
			return true;
		}

		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is the same as the other </returns>
		inline constexpr bool operator==(const BitMask& other)const {
			for (size_t i = 0; i < NumWords(); i++)
				if (m_words[i] != other.m_words[i]) return false;
			return true;
		}

		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is not the same as the other </returns>
		inline constexpr bool operator!=(const BitMask& other)const { return !((*this) == other); }

		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'greater than or equal to' to the other </returns>
		inline constexpr bool operator>=(const BitMask& other)const { return !((*this) < other); }

		/// <summary>
		/// Compares to other BitMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'greater than' to the other </returns>
		inline constexpr bool operator>(const BitMask& other)const { return !((*this) <= other); }


		/// <summary> Empty bitmask </summary>
		inline constexpr static BitMask Empty() { return BitMask(); }

		/// <summary> Bitmask, covering all bit indices </summary>
		inline constexpr static BitMask All() { return ~BitMask(); }
	};
}


namespace std {
	/// <summary> std::hash specialisation for Jimara::BitMask </summary>
	template<typename BitId>
	struct hash<Jimara::BitMask<BitId>> {
		/// <summary>
		/// Hashes the bitmask
		/// </summary>
		/// <param name="mask"> Later mask </param>
		/// <returns> Hashed mask </returns>
		inline size_t operator()(const Jimara::BitMask<BitId>& mask) {
			size_t h = std::hash<uint64_t>()(mask.m_words[0].load());
			for (size_t i = 1; i < Jimara::BitMask<BitId>::NumWords(); i++)
				h = Jimara::MergeHashes(h, std::hash<uint64_t>()(mask.m_words[i].load()));
			return h;
		}
	};
}
