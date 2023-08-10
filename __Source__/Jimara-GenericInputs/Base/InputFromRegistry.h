#pragma once
#include "VectorInput.h"
#include <Jimara/Components/Level/RegistryReference.h>

namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::BooleanInputFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::FloatInputFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::IntInputFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::Vector2InputFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::Vector3InputFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::Vector4InputFromRegistry);



	/// <summary>
	/// Generic component that receives a reference to another input of the same type from Registry and 'emits' it's value on demand
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class GenericInputFromRegistry
		: public virtual Component
		, public virtual InputProvider<Type, Args...>
		, public virtual RegistryReference<InputProvider<Type, Args...>> {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~GenericInputFromRegistry() = 0;

		/// <summary> 
		/// Evaluates stored object input
		/// </summary>
		/// <param name="...args"> Input arguments, passed through registry entry </param>
		/// <returns> Comparizon result </returns>
		inline virtual std::optional<Type> GetInput(Args... args)override {
			const Reference<InputProvider<Type, Args...>> provider = this->StoredObject();
			return InputProvider<Type, Args...>::GetInput(provider.operator->(), args...);
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			RegistryReference<InputProvider<Type, Args...>>::GetFields(recordElement);
		}

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Fills WeakReferenceHolder with a StrongReferenceProvider, 
		/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::FillWeakReferenceHolder(holder);
		}

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Should clear the link to the StrongReferenceProvider;
		/// <para/> Address of the holder has to be the same as the one, previously passed to FillWeakReferenceHolder() method
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::ClearWeakReferenceHolder(holder);
		}
	};

	/// <summary> Virtual destructor </summary>
	template<typename Type, typename... Args>
	GenericInputFromRegistry<Type, Args...>::~GenericInputFromRegistry() {}



	/// <summary>
	/// Generic component that receives a reference to another vector input of the same type from Registry and 'emits' it's value on demand
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorInputFromRegistry
		: public virtual VectorInput::ComponentFrom<Type, Args...>
		, public virtual RegistryReference<InputProvider<Type, Args...>> {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorInputFromRegistry() = 0;

		/// <summary> 
		/// Evaluates stored object input
		/// </summary>
		/// <param name="...args"> Input arguments, passed through registry entry </param>
		/// <returns> Comparizon result </returns>
		inline virtual std::optional<Type> EvaluateInput(Args... args)override {
			const Reference<InputProvider<Type, Args...>> provider = this->StoredObject();
			return InputProvider<Type, Args...>::GetInput(provider.operator->(), args...);
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			RegistryReference<InputProvider<Type, Args...>>::GetFields(recordElement);
		}

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Fills WeakReferenceHolder with a StrongReferenceProvider, 
		/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::FillWeakReferenceHolder(holder);
		}

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Should clear the link to the StrongReferenceProvider;
		/// <para/> Address of the holder has to be the same as the one, previously passed to FillWeakReferenceHolder() method
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::ClearWeakReferenceHolder(holder);
		}
	};

	/// <summary> Virtual destructor </summary>
	template<typename Type, typename... Args>
	VectorInputFromRegistry<Type, Args...>::~VectorInputFromRegistry() {}



	/// <summary>
	/// Generic component that receives a reference to another input of the same type from Registry and 'emits' it's value on demand
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	using InputFromRegistry = std::conditional_t<VectorInput::IsCompatibleType<Type>,
		VectorInputFromRegistry<Type, Args...>, GenericInputFromRegistry<Type, Args...>>;


	/// <summary> Concrete InputFromRegistry Component for bool type </summary>
	class JIMARA_GENERIC_INPUTS_API BooleanInputFromRegistry : public virtual InputFromRegistry<bool> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline BooleanInputFromRegistry(Component* parent, const std::string_view& name = "BooleanFromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~BooleanInputFromRegistry() {}
	};

	/// <summary> Concrete InputFromRegistry Component for float type </summary>
	class JIMARA_GENERIC_INPUTS_API FloatInputFromRegistry : public virtual InputFromRegistry<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatInputFromRegistry(Component* parent, const std::string_view& name = "FloatFromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~FloatInputFromRegistry() {}
	};

	/// <summary> Concrete InputFromRegistry Component for int type </summary>
	class JIMARA_GENERIC_INPUTS_API IntInputFromRegistry : public virtual InputFromRegistry<int> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline IntInputFromRegistry(Component* parent, const std::string_view& name = "IntegerFromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~IntInputFromRegistry() {}
	};

	/// <summary> Concrete InputFromRegistry Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2InputFromRegistry : public virtual InputFromRegistry<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2InputFromRegistry(Component* parent, const std::string_view& name = "Vector2FromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2InputFromRegistry() {}
	};

	/// <summary> Concrete InputFromRegistry Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3InputFromRegistry : public virtual InputFromRegistry<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3InputFromRegistry(Component* parent, const std::string_view& name = "Vector3FromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3InputFromRegistry() {}
	};

	/// <summary> Concrete InputFromRegistry Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4InputFromRegistry : public virtual InputFromRegistry<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4InputFromRegistry(Component* parent, const std::string_view& name = "Vector4FromRegistry") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4InputFromRegistry() {}
	};





	/// <summary>
	/// Type details for GenericInputFromRegistry
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<GenericInputFromRegistry<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<InputProvider<Type, Args...>>());
			reportParentType(TypeId::Of<RegistryReference<InputProvider<Type, Args...>>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorInputFromRegistry
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorInputFromRegistry<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::ComponentFrom<Type, Args...>>());
			reportParentType(TypeId::Of<RegistryReference<InputProvider<Type, Args...>>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};


	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<BooleanInputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<bool>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<BooleanInputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<BooleanInputFromRegistry> serializer("Jimara/Input/RegistryReference/Boolean", "Boolean Input From Registry");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatInputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatInputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<FloatInputFromRegistry> serializer("Jimara/Input/RegistryReference/Float", "Floating point Input From Registry");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<IntInputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<int>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<IntInputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<IntInputFromRegistry> serializer("Jimara/Input/RegistryReference/Integer", "Integer Input From Registry");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2InputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2InputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector2InputFromRegistry> serializer("Jimara/Input/RegistryReference/Vector2", "Vector2 Input From Registry");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3InputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3InputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector3InputFromRegistry> serializer("Jimara/Input/RegistryReference/Vector3", "Vector3 Input From Registry");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4InputFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<InputFromRegistry<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4InputFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector4InputFromRegistry> serializer("Jimara/Input/RegistryReference/Vector4", "Vector4 Input From Registry");
		report(&serializer);
	}
}
