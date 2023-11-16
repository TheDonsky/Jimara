#pragma once
#include "MemoryBlock.h"


namespace Jimara {
	/// <summary>
	/// A fixed-size general purpose buffer that is also a referenceable object
	/// </summary>
	class JIMARA_API RAMBuffer : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="size"> Buffer size </param>
		inline RAMBuffer(size_t size = 0u) : m_data(size) {
			assert(m_data.size() == size);
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="data"> Initial data to fill-in with </param>
		/// <param name="size"> Number of bytes </param>
		inline RAMBuffer(const void* data, size_t size) {
			if (size <= 0u)
				return;
			if (data == nullptr)
				m_data.resize(size);
			else m_data = std::vector(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
		}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="initialData"> Memory block to copy </param>
		inline RAMBuffer(const MemoryBlock& initialData)
			: RAMBuffer(initialData.Data(), initialData.Size()) { }

		/// <summary> Type-cast to a memory block (if you keep this block, no need to maintain reference) </summary>
		inline operator MemoryBlock()const {
			return MemoryBlock(m_data.data(), m_data.size(), this);
		}

		/// <summary> Type-cast to a memory block (if you keep this block, no need to maintain reference) </summary>
		inline operator MemoryBlockRW() {
			return MemoryBlockRW(m_data.data(), m_data.size(), this);
		}

		/// <summary> Buffer size </summary>
		inline size_t Size()const { return m_data.size(); }

		/// <summary> Buffer data </summary>
		inline void* Data() { return m_data.data(); }

		/// <summary> Buffer data </summary>
		inline const void* Data()const { return m_data.data(); }

	private:
		static_assert(sizeof(uint8_t) == 1);
		// Buffer data
		std::vector<uint8_t> m_data;
	};
}
