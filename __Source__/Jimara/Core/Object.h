#pragma once
#include "Reference.h"

namespace Jimara {
	/// <summary> Basic parent class for Jimara objects </summary>
	class Object {
	public:
		/// <summary>
		/// Instantiates an Object of the given type
		/// </summary>
		/// <typeparam name="ObjectType"> Referenced type (Safe to use with anything derived from Object, but will work with anything, as long as it contains AddRef() and ReleaseRef() methods) </typeparam>
		/// <typeparam name="...ConstructorArgs"> Constructor argument types </typeparam>
		/// <param name="...args"> Constructor arguments (instance is instantiated as: new ObjectType(args...)) </param>
		/// <returns> New instance of an Object of given type </returns>
		template<typename ObjectType, typename... ConstructorArgs>
		inline static Reference<ObjectType> Instantiate(ConstructorArgs... args) {
			ObjectType* instance = new ObjectType(args...);
			Reference<ObjectType> reference(instance);
			instance->ReleaseRef();
			return reference;
		}

		/// <summary> Constructor (sets initial reference counter to 1) </summary>
		Object();

		/// <summary> Virtual destructor </summary>
		virtual ~Object();

		/// <summary>
		/// Increments reference counter
		/// Note: Not safe to use for Objects that are initialized on stack
		/// </summary>
		void AddRef()const;

		/// <summary> 
		/// Decreases reference counter ('self-destructs', when reference counter reaches 0)
		/// Note: Not safe to use for Objects that are initialized on stack
		/// </summary>
		void ReleaseRef()const;



	protected:
		/// <summary> 
		/// Invoked, when reference counter hits 0;
		/// Default implementation is deleteing itself.
		/// </summary>
		virtual void OnOutOfScope()const;



	private:
		// Internal reference counter
		mutable std::atomic<std::size_t> m_referenceCount;

		// Reference counter should not be able to be a reson of some random fuck-ups, so let us disable copy/move-constructor/assignments:
		Object(const Object&) = delete;
		Object& operator=(const Object&) = delete;
		Object(Object&&) = delete;
		Object& operator=(Object&&) = delete;
	};
}
