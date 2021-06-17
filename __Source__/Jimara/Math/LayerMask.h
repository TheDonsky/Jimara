#pragma once
#include <atomic>
#include <stdint.h>
#include "Helpers.h"

namespace Jimara {
	/// <summary>
	/// Layer mask, with arbitrary bit width layer
	/// </summary>
	/// <typeparam name="Layer"> Layer type (normally, something like uint8_t) </typeparam>
	template<typename Layer>
	class LayerMask {
	private:
		// Underlying data
		std::atomic<uint64_t> m_words[(static_cast<size_t>(1) << (sizeof(Layer) << 3u)) / (sizeof(uint64_t) << 3u)];

		// Number of elements in m_words array
		inline static constexpr size_t NumWords() noexcept { return (sizeof(static_cast<LayerMask*>(nullptr)->m_words) / sizeof(uint64_t)); }

		// Element in m_words array for the given layer
		inline static size_t Word(Layer layer) { return layer / (sizeof(uint64_t) << 3u); }

		// Bit, that corresponds to the value of the layer within m_words[Word(layer)]
		inline static uint64_t Bit(Layer layer) { return static_cast<uint64_t>(1) << static_cast<uint64_t>(layer & (sizeof(uint64_t) << 3u)); }

		// Yep, if we want to use this one with unordered collections, a fast hash will kinda benefit from access
		friend struct std::hash<LayerMask>;

	public:
		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="mask"> Mask to copy </param>
		inline LayerMask(const LayerMask& mask) { for (size_t i = 0; i < NumWords(); i++) m_words[i].store(mask.m_words[i]); }

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="mask"> Mask to copy </param>
		inline LayerMask& operator=(const LayerMask& mask) { for (size_t i = 0; i < NumWords(); i++) m_words[i].store(mask.m_words[i]); return (*this); }

		/// <summary> Constructor (same as None) </summary>
		inline LayerMask() { for (size_t i = 0; i < NumWords(); i++) m_words[i] = 0; }

		/// <summary>
		/// Modifiable reference to the bit, corresponding to a layer
		/// </summary>
		class LayerReference {
		private:
			// Word
			std::atomic<uint64_t>* const m_word;

			// Bit, corresponding to the layer inside the word
			const uint64_t m_bit;

			// Constructor
			inline LayerReference(std::atomic<uint64_t>* word, uint64_t bit) : m_word(word), m_bit(bit) {}

			// Only the LayerMask can construct LayerReference
			friend class LayerMask;

		public:
			/// <summary> Type cast to the boolean value </summary>
			inline operator bool()const { return ((*m_word) & m_bit) != 0; }

			/// <summary>
			/// Sets corresponding bit
			/// </summary>
			/// <param name="value"> If true, the layer will be inculded in the layer mask </param>
			/// <returns> self </returns>
			inline LayerReference& operator=(bool value) { if (value) (*m_word) |= m_bit; else (*m_word) &= (~m_bit); return (*this); }

			/// <summary>
			/// Sets corresponding bit to true, if the value is true
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline LayerReference& operator|=(bool value) { if (value) (*this) = true; return (*this); }

			/// <summary>
			/// Sets corresponding bit to false, if the value is false
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline LayerReference& operator&=(bool value) { if (!value) (*this) = false; return (*this); }

			/// <summary>
			/// Sets the bit to exclusive or of this and the value
			/// </summary>
			/// <param name="value"> Value to apply operator to </param>
			/// <returns> self </returns>
			inline LayerReference& operator^=(bool value) { (*this) = ((*this) ^ false); return (*this); }
		};

		/// <summary>
		/// Checks if the layer is included in the mask
		/// </summary>
		/// <param name="layer"> Layer to check </param>
		/// <returns> True, if the flag, corresponding to the layer is set </returns>
		inline bool operator[](Layer layer)const { return (m_words[Word(layer)] & Bit(layer)) != 0; }

		/// <summary>
		/// Reference to the bit, corresponding to the layer
		/// </summary>
		/// <param name="layer"> Layer of interest </param>
		/// <returns> Layer reference </returns>
		inline LayerReference operator[](Layer layer) { return LayerReference(m_words + Word(layer), Bit(layer)); }

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="...Rest"> A bounch of layers </typeparam>
		/// <param name="first"> First layer to include </param>
		/// <param name="...rest"> Rest of the layers to include </param>
		template<typename... Rest>
		inline LayerMask(Layer first, Rest... rest) : LayerMask(rest...) { (*this)[first] = true; }

		/// <summary>
		/// Layer mask, consisting of all layers, that are included in either this or the other masks
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline LayerMask operator|(const LayerMask& other)const { LayerMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] | other.m_words[i]; return result; }

		/// <summary>
		/// Includes all layers, that are a part of the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline LayerMask& operator|=(const LayerMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] |= other.m_words[i]; return (*this); }

		/// <summary>
		/// Layer mask, consisting of the layers, that are included in both this and the other masks
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline LayerMask operator&(const LayerMask& other)const { LayerMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] & other.m_words[i]; return result; }

		/// <summary>
		/// Excludes all layers, that are not included in the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline LayerMask& operator&=(const LayerMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] &= other.m_words[i]; return (*this); }

		/// <summary>
		/// Logical exclusive-or of this and the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> Combined mask </returns>
		inline LayerMask operator^(const LayerMask& other)const { LayerMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = m_words[i] ^ other.m_words[i]; return result; }

		/// <summary>
		/// Sets to logical exclusive-or of this and the other mask
		/// </summary>
		/// <param name="other"> Mask to combine with </param>
		/// <returns> self </returns>
		inline LayerMask& operator^=(const LayerMask& other) { for (size_t i = 0; i < NumWords(); i++) m_words[i] ^= other.m_words[i]; return (*this); }

		/// <summary>
		/// Inverts the layer mask
		/// </summary>
		/// <returns> Layer mask, including none of the layers within this and all that are not a part of this mask </returns>
		inline LayerMask operator~()const { LayerMask result; for (size_t i = 0; i < NumWords(); i++) result.m_words[i] = (~m_words[i]); return result; }


		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'less than' to the other </returns>
		inline bool operator<(const LayerMask& other)const {
			for (size_t i = 0; i < NumWords(); i++) {
				uint64_t a = m_words[i], b = other.m_words[i];
				if (a < b) return true;
				else if (a > b) return false;
			}
			return false;
		}

		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'less than or equal to' to the other </returns>
		inline bool operator<=(const LayerMask& other)const {
			for (size_t i = 0; i < NumWords(); i++) {
				uint64_t a = m_words[i], b = other.m_words[i];
				if (a < b) return true;
				else if (a > b) return false;
			}
			return true;
		}

		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is the same as the other </returns>
		inline bool operator==(const LayerMask& other)const {
			for (size_t i = 0; i < NumWords(); i++)
				if (m_words[i] != other.m_words[i]) return false;
			return true;
		}

		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is not the same as the other </returns>
		inline bool operator!=(const LayerMask& other)const { return !((*this) == other); }

		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'greater than or equal to' to the other </returns>
		inline bool operator>=(const LayerMask& other)const { return !((*this) < other); }

		/// <summary>
		/// Compares to other LayerMask
		/// </summary>
		/// <param name="other"> Mask to compare to </param>
		/// <returns> True, if this mask is 'greater than' to the other </returns>
		inline bool operator>(const LayerMask& other)const { return !((*this) <= other); }


		/// <summary> Empty layer mask </summary>
		inline static LayerMask None() { return LayerMask(); }

		/// <summary> Layer mask, covering all layers </summary>
		inline static LayerMask All() { return ~LayerMask(); }
	};
}


namespace std {
	/// <summary> std::hash specialisation for Jimara::LayerMask </summary>
	template<typename Layer>
	struct hash<Jimara::LayerMask<Layer>> {
		/// <summary>
		/// Hashes the layer mask
		/// </summary>
		/// <param name="mask"> Later mask </param>
		/// <returns> Hashed mask </returns>
		inline size_t operator()(const Jimara::LayerMask<Layer>& mask) {
			size_t h = std::hash<uint64_t>()(mask.m_words[0].load());
			for (size_t i = 1; i < Jimara::LayerMask<Layer>::NumWords(); i++)
				h = Jimara::MergeHashes(h, std::hash<uint64_t>()(mask.m_words[i].load()));
			return h;
		}
	};
}
