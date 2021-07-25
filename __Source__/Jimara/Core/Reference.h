#pragma once
#include <atomic>
#include <functional>


namespace Jimara {
	/// <summary>
	/// Reference counter for Objects and anything, that has internal reference counter with AddRef() & ReleaseRef() callbacks
	/// </summary>
	struct JimaraReferenceCounter {
		/// <summary>
		/// Invokes AddRef on the object if not null
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="object"> Subject </param>
		template<typename ObjectType>
		inline static void AddReference(ObjectType* object) { if (object != nullptr) object->AddRef(); }

		/// <summary>
		/// Invokes ReleaseRef on the object if not null
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="object"> Subject </param>
		template<typename ObjectType>
		inline static void ReleaseReference(ObjectType* object) { if (object != nullptr) object->ReleaseRef(); }
	};

	/// <summary>
	/// Reference to an Object (handles reference increments and decrements automatically)
	/// </summary>
	/// <typeparam name="ObjectType"> Referenced type (Safe to use with anything derived from Object, but will work with whatever, as long as it's memory management supports ReferenceCounter) </typeparam>
	/// <typeparam name="ReferenceCounter"> Reference counter (if your objecs have ways of adding and removing references that differ from that of the Jimara::Object, you may override this) </typeparem>
	template<typename ObjectType, typename ReferenceCounter = JimaraReferenceCounter>
	class Reference {
	public:
		/// <summary> Destructor (releases the reference if held) </summary>
		inline ~Reference() { (*this) = nullptr; }

		/// <summary> Constructor </summary>
		/// <param name="address"> Address of the referenced object (default nullptr) </param>
		inline Reference(ObjectType* address = nullptr) : m_pointer(address) { ReferenceCounter::AddReference(address); }

		/// <summary> Assignment (sets new address) </summary>
		/// <param name="address"> Address of the referenced object (can be nullptr) </param>
		/// <returns> self </returns>
		inline Reference& operator=(ObjectType* address) {
			ObjectType* oldAddress = m_pointer.exchange(address);
			ReferenceCounter::AddReference(address);
			ReferenceCounter::ReleaseReference(oldAddress);
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
			ReferenceCounter::ReleaseReference(oldAddress);
			other.m_pointer = nullptr;
			return (*this);
		}

		/// <summary> Accesses referenced object's methods </summary>
		/// <returns> Referenced Object </returns>
		inline ObjectType* operator->()const { return m_pointer; }

		/// <summary> Accesses referenced object's methods </summary>
		/// <returns> Referenced Object </returns>
		inline ObjectType* operator->()const volatile { return m_pointer; }

		/// <summary> Type cast to underlying raw pointer </summary>
		inline operator ObjectType* ()const { return m_pointer; }


	private:
		// Internal pointer
		std::atomic<ObjectType*> m_pointer;
	};
}

namespace std {
	/// <summary>
	/// std::hash override for Reference
	/// </summary>
	/// <typeparam name="ObjectType"> Referenced object type </typeparam>
	template<typename ObjectType>
	struct hash<Jimara::Reference<ObjectType>> {
		/// <summary> Just hash function </summary>
		inline std::size_t operator()(const Jimara::Reference<ObjectType>& reference)const {
			return std::hash<ObjectType*>()(reference.operator->());
		}
	};
}
