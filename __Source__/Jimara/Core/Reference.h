#pragma once
#include<atomic>


namespace Jimara {
	/// <summary>
	/// Reference to an Object (handles reference increments and decrements automatically)
	/// </summary>
	/// <typeparam name="ObjectType"> Referenced type (Safe to use with anything derived from Object, but will work with whatever, as long as it contains AddRef() and ReleaseRef() methods) </typeparam>
	template<typename ObjectType>
	class Reference {
	public:
		/// <summary> Destructor (releases the reference if held) </summary>
		inline ~Reference() { (*this) = nullptr; }

		/// <summary> Constructor </summary>
		/// <param name="address"> Address of the referenced object (default nullptr) </param>
		inline Reference(ObjectType* address = nullptr) : m_pointer(address) { if (address != nullptr) address->AddRef(); }

		/// <summary> Assignment (sets new address) </summary>
		/// <param name="address"> Address of the referenced object (can be nullptr) </param>
		/// <returns> self </returns>
		inline Reference& operator=(ObjectType* address) {
			ObjectType* oldAddress = m_pointer.exchange(address);
			if (address != nullptr) address->AddRef();
			if (oldAddress != nullptr) oldAddress->ReleaseRef();
			return (*this);
		}

		/// <summary> Copy-constructor </summary>
		/// <param name="other"> Reference to copy address from </param>
		inline Reference(const Reference& other) : Reference(other.m_pointer) { }

		/// <summary> Copy-assignment </summary>
		/// <param name="other"> Reference to copy address from </param>
		/// <returns> self </returns>
		inline Reference& operator=(const Reference& other) { (*this) = other.m_pointer; return(*this); }

		/// <summary> Copy-constructor </summary>
		/// <typeparam name="OtherObjectType"> ObjectType of the other reference </typeparam>
		/// <param name="other"> Reference to copy address from </param>
		template<typename OtherObjectType>
		inline Reference(const Reference<OtherObjectType>& other) : Reference(dynamic_cast<ObjectType*>(other.operator->())) { }

		/// <summary> Copy-assignment </summary>
		/// <typeparam name="OtherObjectType"> ObjectType of the other reference </typeparam>
		/// <param name="other"> Reference to copy address from </param>
		/// <returns> self </returns>
		template<typename OtherObjectType>
		inline Reference& operator=(const Reference<OtherObjectType>& other) { (*this) = dynamic_cast<ObjectType*>(other.operator->()); return(*this); }

		/// <summary> Move-constructor </summary>
		/// <param name="other"> Reference to transfer address from </param>
		inline Reference(Reference&& other)noexcept { m_pointer = other.m_pointer.exchange(nullptr); }

		/// <summary> Move-assignment </summary>
		/// <param name="other"> Reference to transfer address from </param>
		/// <returns> self </returns>
		inline Reference& operator=(Reference&& other)noexcept
		{
			ObjectType* oldAddress = m_pointer.exchange(other.m_pointer);
			if (oldAddress != nullptr) oldAddress->ReleaseRef();
			other.m_pointer = nullptr;
			return (*this);
		}

		/// <summary> Accesses referenced object's methods </summary>
		/// <returns> Referenced Object </returns>
		inline ObjectType* operator->()const { return m_pointer; }

		/// <summary> Type cast to underlying raw pointer </summary>
		inline operator ObjectType* ()const { return m_pointer; }


	private:
		// Internal pointer
		std::atomic<ObjectType*> m_pointer;
	};
}
