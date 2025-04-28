#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::Vector3CrossInput);

	/// <summary>
	/// Generic Vector-Cross product input provider
	/// </summary>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename... Args>
	class VectorCrossInputProvider
		: public virtual VectorInput::From<Vector3, Args...>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Alias for Vector3 </summary>
		using Type = Vector3;

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
		/// Evaluates cross-product of A and B
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base inputs </param>
		/// <returns> Cross product </returns>
		inline virtual std::optional<Type> EvaluateInput(Args... args) override {
			const std::optional<Type> a = InputProvider<Type, Args...>::GetInput(m_a, args...);
			const std::optional<Type> b = InputProvider<Type, Args...>::GetInput(m_b, args...);
			return (a.has_value() && b.has_value())
				? std::optional<Type>(Math::Cross(a.value(), b.value()))
				: std::optional<Type>();
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
	/// Vector corss-product input provider, that is also a Component
	/// </summary>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename... Args>
	class VectorCrossInputComponent
		: public virtual Component
		, public virtual VectorCrossInputProvider<Args...> {
	public:
		/// <summary> Alias for the vector type </summary>
		using Type = VectorCrossInputProvider<Args...>::Type;

		/// <summary> Constructor </summary>
		inline VectorCrossInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorCrossInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorCrossInputProvider<Args...>::GetFields(recordElement);
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
	template<typename... Args>
	inline VectorCrossInputComponent<Args...>::~VectorCrossInputComponent() {}



	/// <summary> Concrete CrossInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3CrossInput : public virtual VectorCrossInputComponent<> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3CrossInput(Component* parent, const std::string_view& name = "Vector3Cross") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3CrossInput() {}
	};


	/// <summary>
	/// Type details for VectorCrossInputProvider
	/// </summary>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename... Args>
	struct TypeIdDetails::TypeDetails<VectorCrossInputProvider<Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<typename VectorInput::From<VectorCrossInputProvider<Args...>::Type, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorCrossInputComponent
	/// </summary>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename... Args>
	struct TypeIdDetails::TypeDetails<VectorCrossInputComponent<Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorCrossInputProvider<Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3CrossInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorCrossInputComponent<>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3CrossInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3CrossInput>(
			"Vector3 Cross Input", "Jimara/Input/Math/VectorCross/Vector3", "Vector3 point input provider that calculates cross product of 3d vectors");
		report(factory);
	}
}
