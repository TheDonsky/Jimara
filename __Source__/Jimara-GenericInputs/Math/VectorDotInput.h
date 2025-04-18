#pragma once
#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::Vector2DotInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3DotInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4DotInput);


	/// <summary>
	/// Base vector Dot input
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorDotInputProvider
		: public virtual VectorInput::From<typename Type::value_type, Args...>
		, Serialization::Serializable {
	public:
		/// <summary> Value-type of an individual axis </summary>
		using ValueType = typename Type::value_type;

		/// <summary> First vector </summary>
		inline Reference<InputProvider<Type, Args...>> A()const { return m_a; }

		/// <summary>
		/// Sets first input
		/// </summary>
		/// <param name="input"> Input to use on the 'left' side </param>
		inline void SetA(InputProvider<Type, Args...>* input) { m_a = input; }

		/// <summary> Second vector </summary>
		inline Reference<InputProvider<Type, Args...>> B()const { return m_b; }

		/// <summary>
		/// Sets second input
		/// </summary>
		/// <param name="input"> Input to use on the 'right' side </param>
		inline void SetB(InputProvider<Type, Args...>* input) { m_b = input; }

		/// <summary> 
		/// Evaluates dot product of A and B
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base inputs </param>
		/// <returns> Dot product </returns>
		inline virtual std::optional<ValueType> EvaluateInput(Args... args) override {
			const std::optional<Type> a = InputProvider<Type, Args...>::GetInput(m_a, args...);
			const std::optional<Type> b = InputProvider<Type, Args...>::GetInput(m_b, args...);
			return (a.has_value() && b.has_value())
				? std::optional<ValueType>(Math::Dot(a.value(), b.value()))
				: std::optional<ValueType>();
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override {
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create("A", "First input");
				Reference<InputProvider<Type, Args...>> input = A();
				recordElement(serializer->Serialize(&input));
				SetA(input);
			}
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create("B", "Second input");
				Reference<InputProvider<Type, Args...>> input = B();
				recordElement(serializer->Serialize(&input));
				SetB(input);
			}
		}

	private:
		// First input
		WeakReference<InputProvider<Type, Args...>> m_a;

		// Second input
		WeakReference<InputProvider<Type, Args...>> m_b;
	};


	/// <summary>
	/// Vector Dot input provider, that is also a Component
	/// </summary>
	/// <typeparam name="Type"> Base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorDotInputComponent
		: public virtual Component
		, public virtual VectorDotInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline VectorDotInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorDotInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorDotInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline VectorDotInputComponent<Type, Args...>::~VectorDotInputComponent() {}



	/// <summary> Concrete DotInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2DotInput : public virtual VectorDotInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2DotInput(Component* parent, const std::string_view& name = "Vector2Dot") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2DotInput() {}
	};

	/// <summary> Concrete DotInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3DotInput : public virtual VectorDotInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3DotInput(Component* parent, const std::string_view& name = "Vector3Dot") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3DotInput() {}
	};

	/// <summary> Concrete DotInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4DotInput : public virtual VectorDotInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4DotInput(Component* parent, const std::string_view& name = "Vector4Dot") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4DotInput() {}
	};





	/// <summary>
	/// Type details for VectorDotInputProvider
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorDotInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<float, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorDotInputComponent
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorDotInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorDotInputProvider<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2DotInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorDotInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2DotInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2DotInput>(
			"Vector2 Dot Input", "Jimara/Input/Math/VectorDot/Vector2", "Floating point input provider that calculates Dot product of a 2d vectors");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3DotInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorDotInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3DotInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3DotInput>(
			"Vector3 Dot Input", "Jimara/Input/Math/VectorDot/Vector3", "Floating point input provider that calculates Dot product of a 3d vectors");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4DotInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorDotInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4DotInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4DotInput>(
			"Vector4 Dot Input", "Jimara/Input/Math/VectorDot/Vector4", "Floating point input provider that calculates Dot product of a 4d vectors");
		report(factory);
	}
}
