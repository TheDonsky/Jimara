#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::Vector2NormalizeInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3NormalizeInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4NormalizeInput);


	/// <summary>
	/// Base vector normalize input
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorNormalizeInputProvider
		: public virtual VectorInput::From<Type, Args...>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Vector input to calculate Normalize of </summary>
		inline Reference<InputProvider<Type, Args...>> BaseInput()const { return m_source; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="input"> Vector input to calculate Normalize of </param>
		inline void SetBaseInput(InputProvider<Type, Args...>* input) { m_source = input; }

		/// <summary> 
		/// Evaluates base input and returns it's Normalize
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base input </param>
		/// <returns> Base input Normalize </returns>
		inline virtual std::optional<Type> EvaluateInput(Args... args) override {
			const std::optional<Type> result = InputProvider<Type, Args...>::GetInput(m_source, args...);
			return result.has_value()
				? std::optional<Type>(Math::Normalize(result.value()))
				: std::optional<Type>();
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override {
			static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create(
				"Base Input", "Vector input to calculate Normalize of");
			Reference<InputProvider<Type, Args...>> input = m_source;
			recordElement(serializer->Serialize(&input));
			m_source = input;
		}

	private:
		// Base input
		WeakReference<InputProvider<Type, Args...>> m_source;
	};


	/// <summary>
	/// Vector Normalize input provider, that is also a Component
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorNormalizeInputComponent
		: public virtual Component
		, public virtual VectorNormalizeInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline VectorNormalizeInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorNormalizeInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorNormalizeInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline VectorNormalizeInputComponent<Type, Args...>::~VectorNormalizeInputComponent() {}



	/// <summary> Concrete NormalizeInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2NormalizeInput : public virtual VectorNormalizeInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2NormalizeInput(Component* parent, const std::string_view& name = "Vector2Normalize") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2NormalizeInput() {}
	};

	/// <summary> Concrete NormalizeInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3NormalizeInput : public virtual VectorNormalizeInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3NormalizeInput(Component* parent, const std::string_view& name = "Vector3Normalize") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3NormalizeInput() {}
	};

	/// <summary> Concrete NormalizeInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4NormalizeInput : public virtual VectorNormalizeInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4NormalizeInput(Component* parent, const std::string_view& name = "Vector4Normalize") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4NormalizeInput() {}
	};





	/// <summary>
	/// Type details for VectorNormalizeInputProvider
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorNormalizeInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<Type, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorNormalizeInputComponent
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorNormalizeInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorNormalizeInputProvider<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2NormalizeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorNormalizeInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2NormalizeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2NormalizeInput>(
			"Vector2 Normalize Input", "Jimara/Input/Math/VectorNormalize/Vector2", "Normalized 2d vector direction from other vector input");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3NormalizeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorNormalizeInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3NormalizeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3NormalizeInput>(
			"Vector3 Normalize Input", "Jimara/Input/Math/VectorNormalize/Vector3", "Normalized 2d vector direction from other vector input");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4NormalizeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorNormalizeInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4NormalizeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4NormalizeInput>(
			"Vector4 Normalize Input", "Jimara/Input/Math/VectorNormalize/Vector4", "Normalized 2d vector direction from other vector input");
		report(factory);
	}
}
