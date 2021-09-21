#pragma once
#include <new>
#include <utility>
#include <cassert>


namespace Jimara {
	/// <summary>
	/// Vector, that can store some amount of values on stack 
	/// (Useful, for example, when storing arbitrary polygons that are mostly quads but not always in an array of sorts, 
	/// but worry that every single one of them having a heap allocation will be suboptimal)
	/// </summary>
	/// <typeparam name="Type"> Type of the stored objects </typeparam>
	/// <typeparam name="StackSize"> Number of items that can be stored on stack </typeparam>
	template<typename Type, size_t StackSize = 0>
	class Stacktor {
	public:
		/// <summary> Default constructor </summary>
		inline Stacktor() : m_data(reinterpret_cast<Type*>(m_stackBuffer)), m_allocation(StackSize) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="...ConstructorArgs"> Value types, passed as stored objects' constructor arguments </typeparam>
		/// <param name="size"> Initial Stacktor size </param>
		/// <param name="...args"> Values, passed as stored objects' constructor arguments </param>
		template<typename... ConstructorArgs>
		inline Stacktor(size_t size, ConstructorArgs... args) : Stacktor() { Resize(size, args...); }

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="data"> Data to initialize with </param>
		/// <param name="size"> Data size </param>
		inline Stacktor(const Type* data, size_t size) : Stacktor() { SetData(data, size); }

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="ArraySize"> Data size </typeparam>
		/// <param name="data"> Data to initialize with </param>
		template<size_t ArraySize>
		inline Stacktor(const Type(&data)[ArraySize]) : Stacktor(data, ArraySize) { }

		/// <summary>
		/// Sets content to given data
		/// </summary>
		/// <typeparam name="ArraySize"> Data size </typeparam>
		/// <param name="data"> Data to set </param>
		/// <returns> self </returns>
		template<size_t ArraySize>
		inline Stacktor& operator=(const Type(&data)[ArraySize]) {
			SetData(data, ArraySize);
			return *this;
		}

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name="other"> Collection to copy </param>
		inline Stacktor(const Stacktor& other) : Stacktor() { (*this) = other; }

		/// <summary>
		/// Copy-Assignment
		/// </summary>
		/// <param name="other"> Collection to copy </param>
		/// <returns> self </returns>
		inline Stacktor& operator=(const Stacktor& other) {
			SetData(other.m_data, other.m_size);
			return *this;
		}

		/// <summary>
		/// Move-Constructor
		/// </summary>
		/// <param name="other"> Collection to move </param>
		/// <returns> self </returns>
		inline Stacktor(Stacktor&& other)noexcept : Stacktor() { (*this) = std::move(other); }

		/// <summary>
		/// Move-Assignment
		/// </summary>
		/// <param name="other"> Collection to move </param>
		/// <returns> self </returns>
		inline Stacktor& operator=(Stacktor&& other)noexcept {
			if (this == (&other)) return (*this);
			while (m_size > other.m_size) Pop();
			if (other.StoredOnStack()) {
				assert(m_allocation >= other.m_allocation);
				const size_t mid = (m_size < other.m_size) ? m_size : other.m_size;
				for (size_t i = 0; i < mid; i++)
					m_data[i] = std::move(other.m_data[i]);
				for (size_t i = mid; i < other.m_size; i++)
					new (m_data + i) Type(std::move(other.m_data[i]));
				m_size = other.m_size;
			}
			else {
				Clear(true);
				m_data = other.m_data;
				m_size = other.m_size;
				m_allocation = other.m_allocation;
				other.m_data = reinterpret_cast<Type*>(other.m_stackBuffer);
				other.m_size = 0;
				other.m_allocation = StackSize;
			}
			return (*this);
		}

		/// <summary> Destructor </summary>
		inline ~Stacktor() { Clear(true); }

		/// <summary> True, if Data() is stored on stack alongside the collection itself </summary>
		inline bool StoredOnStack()const { return reinterpret_cast<const char*>(m_data) == m_stackBuffer; }

		/// <summary>
		/// Clears the collection
		/// </summary>
		/// <param name="releaseMemory"> If true, any heap allocation the collection may hold will be freed as well </param>
		inline void Clear(bool releaseMemory = false) {
			for (size_t i = 0; i < m_size; i++)
				m_data[i].~Type();
			m_size = 0;
			if (releaseMemory && (!StoredOnStack())) {
				char* data = reinterpret_cast<char*>(m_data);
				delete[] data;
				m_data = reinterpret_cast<Type*>(m_stackBuffer);
				m_allocation = StackSize;
			}
		}

		/// <summary>
		/// Resets the collection with given data
		/// </summary>
		/// <param name="data"> Data to set </param>
		/// <param name="size"> Data size </param>
		inline void SetData(const Type* data, size_t size) {
			if (data == m_data) {
				while (m_size > size) Pop();
			}
			else if (data > m_data && data < (m_data + m_size)) {
				assert(((reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(m_data)) % sizeof(Type)) == 0);
				const size_t count = (data - m_data);
				assert(size <= (m_size - count));
				RemoveAt(0, count);
				while (m_size > size) Pop();
			}
			else {
				RequestCapacity(size, false);
				while (m_size > size) Pop();
				const size_t mid = (m_size < size) ? m_size : size;
				for (size_t i = 0; i < mid; i++)
					m_data[i] = data[i];
				for (size_t i = mid; i < size; i++)
					new (m_data + i) Type(data[i]);
				m_size = size;
			}
		}

		/// <summary>
		///  Resets the collection with given data
		/// </summary>
		/// <typeparam name="ArraySize"> Data size </typeparam>
		/// <param name="data"> Data to set </param>
		template<size_t ArraySize>
		inline void SetData(const Type(&data)[ArraySize]) {
			SetData(data, ArraySize);
		}

		/// <summary>
		/// Applies a capacity requirenment
		/// </summary>
		/// <param name="capacity"> Requested capacity (requests at least this if setExact is false, exactly the value otherwise) </param>
		/// <param name="setExact"> If true, the capacity will become exactly the given value potentially truncating the stacktor, otherwise makes sure minimal capacity requirenment is met </param>
		inline void RequestCapacity(size_t capacity, bool setExact = false) {
			if (capacity < StackSize) capacity = StackSize;
			if ((capacity == m_allocation) || ((!setExact) && (m_allocation > capacity))) return;
			const size_t doubleAllocation = (m_allocation << 1);
			const size_t allocation = ((setExact || (doubleAllocation < capacity)) ? capacity : doubleAllocation);
			const size_t size = ((m_size <= allocation) ? m_size : allocation);

			char* newBuffer;
			if (allocation > StackSize) newBuffer = new char[sizeof(Type) * allocation];
			else newBuffer = reinterpret_cast<char*>(m_stackBuffer);
			Type* newData = reinterpret_cast<Type*>(newBuffer);
			assert(m_data != newData);
			for (size_t i = 0; i < size; i++)
				new(newData + i) Type(std::move(m_data[i]));
			Clear(true);

			m_data = newData;
			m_size = size;
			m_allocation = allocation;
		}

		/// <summary>
		/// Sets collection size
		/// </summary>
		/// <typeparam name="...ConstructorArgs"> Type of the Constructor arguments for new entries </typeparam>
		/// <param name="size"> New size for the collection </param>
		/// <param name="...args"> Constructor arguments for new entries </param>
		template<typename... ConstructorArgs>
		inline void Resize(size_t size, ConstructorArgs... args) {
			RequestCapacity(size, false);
			while (m_size > size) Pop();
			while (m_size < size) {
				new(m_data + m_size) Type(args...);
				m_size++;
			}
		}

		/// <summary> Element count </summary>
		inline size_t Size()const { return m_size; }

		/// <summary> Number of elements that we can hold without additional allocations </summary>
		inline size_t Capacity()const { return m_allocation; }

		/// <summary>
		/// Stored object by index
		/// </summary>
		/// <param name="index"> Element index </param>
		/// <returns> Stored object </returns>
		inline Type& operator[](size_t index) { return m_data[index]; }

		/// <summary>
		/// Stored object by index (const)
		/// </summary>
		/// <param name="index"> Element index </param>
		/// <returns> Stored object </returns>
		inline const Type& operator[](size_t index)const { return m_data[index]; }

		/// <summary> Underlying data </summary>
		inline Type* Data() { return m_data; }

		/// <summary> Underlying data (const) </summary>
		inline const Type* Data()const { return m_data; }

		/// <summary> Underlying data </summary>
		inline operator Type* () { return m_data; }

		/// <summary> Underlying data (const) </summary>
		inline operator const Type* ()const { return m_data; }

		/// <summary>
		/// Adds a new member
		/// </summary>
		/// <param name="value"> New element </param>
		inline void Push(Type&& value) {
			RequestCapacity(m_size + 1, false);
			new (m_data + m_size) Type(std::move(value));
			m_size++;
		}

		/// <summary>
		/// Adds a new member
		/// </summary>
		/// <param name="value"> New element </param>
		inline void Push(const Type& value) {
			RequestCapacity(m_size + 1, false);
			new (m_data + m_size) Type(value);
			m_size++;
		}

		/// <summary> Removes last element </summary>
		inline void Pop() {
			assert(m_size > 0);
			m_size--;
			m_data[m_size].~Type();
		}

		/// <summary>
		/// Removes an amount of elements from some index onwards
		/// </summary>
		/// <param name="index"> Index to start removing from </param>
		/// <param name="count"> Number of elements to remove </param>
		inline void RemoveAt(size_t index, size_t count = 1) {
			assert(index < m_size);
			if (count <= 0) return;
			size_t next = index + count;
			while (next < m_size) {
				m_data[index] = std::move(m_data[next]);
				index++;
				next++;
			}
			while (index < m_size) Pop();
		}


	private:
		// Current data
		Type* m_data;

		// Number of elements
		size_t m_size = 0;

		// m_data size (in elements)
		size_t m_allocation;

		// Buffer to be used as stack data
		char m_stackBuffer[(StackSize <= 0) ? 1 : (sizeof(Type) * StackSize)] = {};
	};
}
