#pragma once
#include "Component.h"


namespace Jimara {
	/// <summary>
	/// Simple wrapper for weakly referencing Component types
	/// <para/> Note that this is not designed to be thread-safe and will work reliably 
	///		only inside the main update loop. 
	/// </summary>
	/// <typeparam name="Type"> Any type derived from Component </typeparam>
	template<typename Type>
	class WeakComponentReference {
	public:
		/// <summary> Default Constructor </summary>
		inline WeakComponentReference() {}

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name="other"> Reference </param>
		inline WeakComponentReference(const WeakComponentReference& other) {
			(*this) = other.m_reference;
		}

		/// <summary> Move-Constructor (always copies) </summary>
		inline WeakComponentReference(WeakComponentReference&&) = delete;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="reference"> Reference </param>
		inline WeakComponentReference(const Reference<Type>& reference) {
			(*this) = reference;
		}

		/// <summary> Destructor </summary>
		inline ~WeakComponentReference() { 
			(*this) = nullptr; 
		}

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Reference </param>
		/// <returns> Self </returns>
		inline WeakComponentReference& operator=(const WeakComponentReference& other) {
			return (*this) = other.m_reference;
		}

		/// <summary> Move-Assignment (always copies) </summary>
		inline WeakComponentReference& operator=(WeakComponentReference&&) = delete;

		/// <summary>
		/// Sets reference
		/// </summary>
		/// <param name="pointer"> Raw pointer to the value </param>
		/// <returns> Self </returns>
		inline WeakComponentReference& operator=(Type* pointer) {
			return (*this) = Reference<Type>(pointer);
		}

		/// <summary>
		/// Sets reference
		/// </summary>
		/// <param name="reference"> Strong reference to the value </param>
		/// <returns> Self </returns>
		inline WeakComponentReference& operator=(const Reference<Type>& reference) {
			if (m_reference == reference)
				return *this;
			typedef void(*OnComponentDestroyedFn)(WeakComponentReference*, Component*);
			static const OnComponentDestroyedFn onComponentDestroyed = [](WeakComponentReference* self, Component* address) {
#ifndef NDEBUG
				assert(address == self->m_reference);
				assert(self->m_reference != nullptr);
#endif
				self->m_reference->OnDestroyed() -= Callback<Component*>(onComponentDestroyed, self);
			};
			if (m_reference != nullptr)
				m_reference->OnDestroyed() -= Callback<Component*>(onComponentDestroyed, this);
			m_reference = (reference == nullptr || reference->Destroyed()) ? nullptr : reference;
			if (m_reference != nullptr)
				m_reference->OnDestroyed() += Callback<Component*>(onComponentDestroyed, this);
			return *this;
		}

		/// <summary> Retrieves strong reference, if it is still present </summary>
		inline operator Reference<Type>()const {
#ifndef NDEBUG
			assert(m_reference == nullptr || (!m_reference->Destroyed()));
#endif
			return m_reference;
		}

	private:
		// Underlying strong reference
		Reference<Type> m_reference;
	};
}
