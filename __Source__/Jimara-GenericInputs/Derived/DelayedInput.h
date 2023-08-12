#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::DelayedFloatInput);
	JIMARA_REGISTER_TYPE(Jimara::DelayedVector2Input);
	JIMARA_REGISTER_TYPE(Jimara::DelayedVector3Input);
	JIMARA_REGISTER_TYPE(Jimara::DelayedVector4Input);





	/// <summary>
	/// Base class for delayed inputs
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class DelayedInputBase : public virtual SceneContext::UpdatingComponent {
	public:
		/// <summary> Constructor </summary>
		inline DelayedInputBase() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedInputBase() = 0;

		/// <summary> Value from the latest update </summary>
		inline const std::optional<Type>& DelayedValue()const { return m_lastValue; }

		/// <summary> Current value </summary>
		inline std::optional<Type>& DelayedValue() { return m_lastValue; }

		/// <summary> Input to follow </summary>
		inline Reference<Jimara::InputProvider<Type>> BaseInput()const { return m_baseInput; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="input"> Input to follow </param>
		inline void SetBaseInput(Jimara::InputProvider<Type>* input) { m_baseInput = input; }

		/// <summary> Input value lerp speed </summary>
		inline float UpdateSpeed()const { return m_updateSpeed; }

		/// <summary>
		/// Sets update speed
		/// </summary>
		/// <param name="speed"> Update speed (negative values will be clamped to 0) </param>
		inline void SetUpdateSpeed(float speed) { m_updateSpeed = Math::Max(speed, 0.0f); }

		/// <summary> If true, unscaled delta time will be used </summary>
		inline bool UsesUnscaledTime()const { return m_useUnscaledTime; }

		/// <summary>
		/// Sets delta time mode
		/// </summary>
		/// <param name="useUnscaled"> If true, unscaled delta time will be used </param>
		inline void UseUnscaledTime(bool useUnscaled) { m_useUnscaledTime = useUnscaled; }

		/// <summary>
		/// Exposes properties
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Updates input value </summary>
		inline virtual void Update()override;

	private:
		// Source input
		WeakReference<Jimara::InputProvider<Type>> m_baseInput;

		// Input value lerp speed
		float m_updateSpeed = 1.0f;

		// If true, update will use unscaled delta time instead of the scaled one
		bool m_useUnscaledTime = false;

		// Last input value
		std::optional<Type> m_lastValue;
	};
	


	/// <summary>
	/// Delayed input for Vector types
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class DelayedVectorInput
		: public virtual DelayedInputBase<Type>
		, public virtual VectorInput::From<Type> {
	public:
		/// <summary> Constructor </summary>
		inline DelayedVectorInput() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedVectorInput() = 0;

		/// <summary> Retrieves current input value </summary>
		inline virtual std::optional<Type> EvaluateInput()override { return this->DelayedValue(); }

	public:
		/// <summary> Directly inherit Component's implementation </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::FillWeakReferenceHolder(holder);
		}

		/// <summary> Directly inherit Component's implementation </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::ClearWeakReferenceHolder(holder);
		}
	};

	/// <summary> Virtual destructor </summary>
	template<typename Type>
	DelayedVectorInput<Type>::~DelayedVectorInput() {}



	/// <summary>
	/// Input that smoothly 'follows' another input value with some latency
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class DelayedGenericInput
		: public virtual DelayedInputBase<Type>
		, public virtual InputProvider<Type> {
	public:
		/// <summary> Constructor </summary>
		inline DelayedGenericInput() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedGenericInput() = 0;

		/// <summary> Retrieves current input value </summary>
		inline virtual std::optional<Type> GetInput() override { return this->DelayedValue(); }

	public:
		/// <summary> Directly inherit Component's implementation </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::FillWeakReferenceHolder(holder);
		}

		/// <summary> Directly inherit Component's implementation </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
			Component::ClearWeakReferenceHolder(holder);
		}
	};

	/// <summary> Virtual destructor </summary>
	template<typename Type>
	DelayedGenericInput<Type>::~DelayedGenericInput() {}



	/// <summary>
	/// Type definition for a Delayed input (Vector, where possible, Generic otherwise)
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	using DelayedInput = std::conditional_t<VectorInput::IsCompatibleType<Type>, DelayedVectorInput<Type>, DelayedGenericInput<Type>>;



	/// <summary> Concrete DelayedInput Component for float type </summary>
	class JIMARA_GENERIC_INPUTS_API DelayedFloatInput : public virtual DelayedInput<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline DelayedFloatInput(Component* parent, const std::string_view& name = "DelayedFloat") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedFloatInput() {}
	};

	/// <summary> Concrete DelayedInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API DelayedVector2Input : public virtual DelayedInput<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline DelayedVector2Input(Component* parent, const std::string_view& name = "DelayedVector2") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedVector2Input() {}
	};

	/// <summary> Concrete DelayedInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API DelayedVector3Input : public virtual DelayedInput<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline DelayedVector3Input(Component* parent, const std::string_view& name = "DelayedVector3") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedVector3Input() {}
	};

	/// <summary> Concrete DelayedInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API DelayedVector4Input : public virtual DelayedInput<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline DelayedVector4Input(Component* parent, const std::string_view& name = "DelayedVector4") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~DelayedVector4Input() {}
	};





	template<typename Type>
	inline DelayedInputBase<Type>::~DelayedInputBase() {}

	template<typename Type>
	inline void DelayedInputBase<Type>::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::SceneContext::UpdatingComponent::GetFields(recordElement);
		{
			static const auto serializer = Serialization::DefaultSerializer<Reference<Jimara::InputProvider<Type>>>::Create(
				"Base Input", "Input to follow");
			Reference<Jimara::InputProvider<Type>> input = m_baseInput;
			recordElement(serializer->Serialize(&input));
			m_baseInput = input;
		}
		{
			static const auto serializer = Serialization::FloatSerializer::Create(
				"Update Speed", "Tells, how fast the input value is updated (input value lerp speed)");
			recordElement(serializer->Serialize(&m_updateSpeed));
			m_updateSpeed = Math::Max(m_updateSpeed, 0.0f);
		}
		{
			typedef bool(*GetFn)(DelayedInputBase*);
			typedef void(*SetFn)(const bool&, DelayedInputBase*);
			static const GetFn get = [](DelayedInputBase* data) { return data->m_useUnscaledTime; };
			static const SetFn set = [](const bool& value, DelayedInputBase* data) {
				data->m_useUnscaledTime = value;
				data->m_lastValue = std::optional<Type>();
			};
			static const auto serializer = Jimara::Serialization::BoolSerializer::Create<DelayedInputBase>("Use Unscaled Time",
				"If true, update will use unscaled delta time instead of the scaled one; changing/resetting this resets the input value",
				Function<bool, DelayedInputBase*>(get), Callback<const bool&, DelayedInputBase*>(set));
			recordElement(serializer->Serialize(this));
		}
	}

	template<typename Type>
	inline void DelayedInputBase<Type>::Update() {
		const std::optional<Type> curInput = Jimara::InputProvider<Type>::GetInput(m_baseInput);
		if (curInput.has_value() && m_lastValue.has_value()) {
			float deltaTime = m_useUnscaledTime ? Context()->Time()->UnscaledDeltaTime() : Context()->Time()->ScaledDeltaTime();
			const float lerpAmount = (1.0f - std::exp(-deltaTime * m_updateSpeed));
			m_lastValue.value() = Math::Lerp(m_lastValue.value(), curInput.value(), lerpAmount);
		}
		else m_lastValue = curInput;
	}





	/// <summary>
	/// Type details for DelayedInputBase
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<DelayedInputBase<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<SceneContext::UpdatingComponent>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for DelayedGenericInput
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<DelayedGenericInput<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<DelayedInputBase<Type>>());
			reportParentType(TypeId::Of<InputProvider<Type>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for DelayedVectorInput
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<DelayedVectorInput<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<DelayedInputBase<Type>>());
			reportParentType(TypeId::Of<VectorInput::From<Type>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for concrete types:
	template<> inline void TypeIdDetails::GetParentTypesOf<DelayedFloatInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<DelayedInput<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<DelayedFloatInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<DelayedFloatInput>(
			"Delayed Float Input", "Jimara/Input/Delayed/Float", "Delayed floating point Input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<DelayedVector2Input>(const Callback<TypeId>& report) {
		report(TypeId::Of<DelayedInput<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<DelayedVector2Input>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<DelayedVector2Input>(
			"Delayed Vector2 Input", "Jimara/Input/Delayed/Vector2", "Delayed Vector2 Input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<DelayedVector3Input>(const Callback<TypeId>& report) {
		report(TypeId::Of<DelayedInput<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<DelayedVector3Input>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<DelayedVector3Input>(
			"Delayed Vector3 Input", "Jimara/Input/Delayed/Vector3", "Delayed Vector3 Input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<DelayedVector4Input>(const Callback<TypeId>& report) {
		report(TypeId::Of<DelayedInput<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<DelayedVector4Input>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<DelayedVector4Input>(
			"Delayed Vector4 Input", "Jimara/Input/Delayed/Vector4", "Delayed Vector4 Input provider");
		report(factory);
	}
}
