#pragma once
#include "Object.h"


namespace Jimara {
	/// <summary>
	/// Parent interface for in-engine weakly referensable objects
	/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
	/// </summary>
	class JIMARA_API WeaklyReferenceable : public virtual Object {
	public:
		/// <summary> 
		/// A small object that is tied to some WeaklyReferenceable instance
		/// and can retrieve it's strong reference back, as long as it still exists.
		/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
		/// </summary>
		class JIMARA_API StrongReferenceProvider : public virtual Object {
		public:
			/// <summary> Should retrieve strong reference to the object associated with the provider </summary>
			virtual Reference<WeaklyReferenceable> RestoreStrongReference() = 0;
		};

		/// <summary> WeakReferenceHolder will simply be a strong reference to the StrongReferenceProvider </summary>
		using WeakReferenceHolder = Reference<StrongReferenceProvider>;

		/// <summary>
		/// Should fill WeakReferenceHolder with a StrongReferenceProvider, 
		/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
		/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void FillWeakReferenceHolder(WeakReferenceHolder& holder) = 0;

		/// <summary>
		/// Should clear the link to the StrongReferenceProvider;
		/// <para/> Address of the holder will be the same as the one, previously passed to FillWeakReferenceHolder() method
		/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void ClearWeakReferenceHolder(WeakReferenceHolder& holder) = 0;

		/// <summary>
		/// Required by WeakReference; Wrapper for StrongReferenceProvider::GetStrongReference()
		/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
		/// </summary>
		/// <param name="holder"> Reference of a StrongReferenceProvider </param>
		/// <returns> Strong reference to the object </returns>
		inline static Reference<WeaklyReferenceable> GetStrongReference(const WeakReferenceHolder& holder) {
			return (holder == nullptr) ? Reference<WeaklyReferenceable>(nullptr) : holder->RestoreStrongReference();
		}

		/// <summary>
		/// Required by WeakReference; Replaces contents of WeakReferenceHolder 
		/// by invoking ClearWeakReferenceHolder on the old object and FillWeakReferenceHolder on the new one.
		/// <para/> Note that thread-safety is not guaranteed by this interface and will vary based on the implementation.
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		/// <param name="object"> New object reference to replace the link with </param>
		inline static void UpdateWeakReferenceHolder(WeakReferenceHolder& holder, WeaklyReferenceable* object) {
			const Reference<WeaklyReferenceable> oldObject = GetStrongReference(holder);
			if (oldObject == object)
				return;
			if (oldObject != nullptr)
				oldObject->ClearWeakReferenceHolder(holder);
			if (object != nullptr)
				object->FillWeakReferenceHolder(holder);
		}
	};


	/// <summary>
	/// Weak reference for supported types
	/// <para/> In order for a type to be weakly referensable, it has to satisfy following requirenments:
	/// <para/>		0. Type::WeakReferenceHolder should be a valid type, that'll hold some arbitrary 'link' to the actual object;
	/// <para/>		1. Type::GetStrongReference(const Type::WeakReferenceHolder&#38;) static method should return back the reference to the actual object,
	///				if it is still valid;
	/// <para/>		2. Type::UpdateWeakReferenceHolder(WeakReferenceHolder&#38;, Type*) static method should replace
	///				the contents of the WeakReferenceHolder with the new address, clearing any previous links.
	/// <para/>	Notes:
	/// <para/>		0. This class does not take any responsibility for making sure anything is thread-safe. 
	///				Safety should be guaranteed or ignored on the side of the referenced type;
	/// <para/>		1. WeaklyReferenceable is an example interface for one parent type that supports weak-refs; 
	///				you do not have to derive it, but it may come in handy;
	/// <para/>		2. WeakReference does not guarantee actual 'weakness' of the reference; 
	///				Type may "choose" to always store strong references instead. 
	///				It's up to the user to be aware of the limitations of specific referenced types.
	/// </summary>
	/// <typeparam name="Type"> Any type that can be weakly referencable </typeparam>
	template<typename Type>
	class WeakReference {
	public:
		/// <summary> Default Constructor </summary>
		inline WeakReference() {}

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name="other"> Reference </param>
		inline WeakReference(const WeakReference& other) {
			Reference<Type> address = Type::GetStrongReference(other.m_refHolder);
			(*this) = address;
		}

		/// <summary> Move-Constructor (always copies) </summary>
		inline WeakReference(WeakReference&&) = delete;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="reference"> Reference </param>
		inline WeakReference(const Reference<Type>& reference) {
			(*this) = reference;
		}

		/// <summary> Destructor </summary>
		inline ~WeakReference() {
			(*this) = (Type*)nullptr;
		}

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Reference </param>
		/// <returns> Self </returns>
		inline WeakReference& operator=(const WeakReference& other) {
			Reference<Type> address = Type::GetStrongReference(other.m_refHolder);
			return (*this) = address;
		}

		/// <summary> Move-Assignment (always copies) </summary>
		inline WeakReference& operator=(WeakReference&&) = delete;

		/// <summary>
		/// Sets reference
		/// </summary>
		/// <param name="pointer"> Raw pointer to the value </param>
		/// <returns> Self </returns>
		inline WeakReference& operator=(Type* pointer) {
			Type::UpdateWeakReferenceHolder(m_refHolder, pointer);
			return (*this);
		}

		/// <summary>
		/// Sets reference
		/// </summary>
		/// <param name="reference"> Strong reference to the value </param>
		/// <returns> Self </returns>
		inline WeakReference& operator=(const Reference<Type>& reference) {
			Type::UpdateWeakReferenceHolder(m_refHolder, reference);
			return (*this);
		}

		/// <summary> Retrieves strong reference, if it is still present </summary>
		inline operator Reference<Type>()const {
			return Type::GetStrongReference(m_refHolder);
		}

		/// <summary> 
		/// WeakReference is a 'wrapper' on Reference<Type> in a sense that we can store and retrieve strong references from it;
		/// <para/> Do not pay much attention to this; it's just for making serialization simpler...
		/// </summary>
		using WrappedType = Reference<Type>;

	private:
		// Actual weak reference token thing...
		typename Type::WeakReferenceHolder m_refHolder;
	};
}
