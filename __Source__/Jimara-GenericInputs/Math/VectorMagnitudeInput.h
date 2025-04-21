#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::Vector2MagnitudeInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3MagnitudeInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4MagnitudeInput);


	/// <summary>
	/// Base vector magnitude input
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorMagnitudeInputProvider
		: public virtual VectorInput::From<float, Args...>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Vector input to calculate magnitude of </summary>
		inline Reference<InputProvider<Type, Args...>> BaseInput()const { return m_source; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="input"> Vector input to calculate magnitude of </param>
		inline void SetBaseInput(InputProvider<Type, Args...>* input) { m_source = input; }

		/// <summary> 
		/// Evaluates base input and returns it's magnitude
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base input </param>
		/// <returns> Base input magnitude </returns>
		inline virtual std::optional<float> EvaluateInput(Args... args) override {
			const std::optional<Type> result = InputProvider<Type, Args...>::GetInput(m_source, args...);
			return result.has_value()
				? std::optional<float>(Math::Magnitude(result.value()))
				: std::optional<float>();
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override {
			static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create(
				"Base Input", "Vector input to calculate magnitude of");
			Reference<InputProvider<Type, Args...>> input = m_source;
			recordElement(serializer->Serialize(&input));
			m_source = input;
		}

	private:
		// Base input
		WeakReference<InputProvider<Type, Args...>> m_source;
	};


	/// <summary>
	/// Vector magnitude input provider, that is also a Component
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorMagnitudeInputComponent
		: public virtual Component
		, public virtual VectorMagnitudeInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline VectorMagnitudeInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorMagnitudeInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorMagnitudeInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline VectorMagnitudeInputComponent<Type, Args...>::~VectorMagnitudeInputComponent() {}



	/// <summary> Concrete MagnitudeInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2MagnitudeInput : public virtual VectorMagnitudeInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2MagnitudeInput(Component* parent, const std::string_view& name = "Vector2Magnitude") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2MagnitudeInput() {}
	};

	/// <summary> Concrete MagnitudeInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3MagnitudeInput : public virtual VectorMagnitudeInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3MagnitudeInput(Component* parent, const std::string_view& name = "Vector3Magnitude") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3MagnitudeInput() {}
	};

	/// <summary> Concrete MagnitudeInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4MagnitudeInput : public virtual VectorMagnitudeInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4MagnitudeInput(Component* parent, const std::string_view& name = "Vector4Magnitude") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4MagnitudeInput() {}
	};





	/// <summary>
	/// Type details for VectorMagnitudeInputProvider
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorMagnitudeInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<float, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorMagnitudeInputComponent
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorMagnitudeInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorMagnitudeInputProvider<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2MagnitudeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorMagnitudeInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2MagnitudeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2MagnitudeInput>(
			"Vector2 Magnitude Input", "Jimara/Input/Math/VectorMagnitude/Vector2", "Floating point input provider that calculates magnitude of a 2d vector");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3MagnitudeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorMagnitudeInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3MagnitudeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3MagnitudeInput>(
			"Vector3 Magnitude Input", "Jimara/Input/Math/VectorMagnitude/Vector3", "Floating point input provider that calculates magnitude of a 3d vector");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4MagnitudeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorMagnitudeInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4MagnitudeInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4MagnitudeInput>(
			"Vector4 Magnitude Input", "Jimara/Input/Math/VectorMagnitude/Vector4", "Floating point input provider that calculates magnitude of a 4d vector");
		report(factory);
	}
}
