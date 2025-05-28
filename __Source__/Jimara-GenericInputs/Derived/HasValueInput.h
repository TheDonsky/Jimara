#pragma once
#include "../Base/VectorInput.h"



namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::IntInputHasValueInput);
	JIMARA_REGISTER_TYPE(Jimara::BoolInputHasValueInput);
	JIMARA_REGISTER_TYPE(Jimara::FloatInputHasValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector2InputHasValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3InputHasValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4InputHasValueInput);





	/// <summary>
	/// A basic generic input that evaluates in an input of some other type has value or not.
	/// </summary>
	/// <typeparam name="ValueType"> Value-type of the base input </typeparam>
	/// <typeparam name="...Args"> Base input arguments </typeparam>
	template<typename ValueType, typename... Args>
	class HasValueInput : public virtual VectorInput::From<bool, Args...>, public virtual Serialization::Serializable {
	public:
		/// <summary>
		/// Provides "input" value;
		/// <para/> Return type is optional and, therefore, the input is allowed to be empty.
		/// </summary>
		/// <param name="...args"> Some contextual arguments </param>
		/// <returns> Input value if present </returns>
		inline virtual std::optional<bool> EvaluateInput(Args... args) override {
			const Reference<InputProvider<ValueType, Args...>> input = m_input;
			if (input == nullptr)
				return std::nullopt;
			const std::optional<ValueType> val = input->GetInput(args...);
			return val.has_value();
		}

		/// <summary> HasValueInput evaluates this input and returns BaseInput()->GetInput().has_value() </summary>
		inline Reference<InputProvider<ValueType, Args...>> BaseInput()const { return m_input; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="baseInput"> HasValueInput will evaluate this input and returns BaseInput()->GetInput().has_value() </param>
		inline void SetBaseInput(InputProvider<ValueType, Args...>* baseInput) { m_input = baseInput; }

		/// <summary>
		/// Exposes properties
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			typedef InputProvider<ValueType, Args...>*(*GetFn)(HasValueInput*);
			typedef void(*SetFn)(InputProvider<ValueType, Args...>* const&, HasValueInput*);
			static const auto serializer = Serialization::ValueSerializer<InputProvider<ValueType, Args...>*>::template Create<HasValueInput>(
				"BaseInput", "HasValueInput evaluates this input and returns BaseInput()->GetInput().has_value()",
				(GetFn)[](HasValueInput* self)-> InputProvider<ValueType, Args...>* { return self->BaseInput(); },
				(SetFn)[](InputProvider<ValueType, Args...>* const& value, HasValueInput* self) { self->SetBaseInput(value); });
			const Reference<InputProvider<ValueType, Args...>> input = m_input; // Just to protect reference in case there's a random change...
			recordElement(serializer->Serialize(this));
		}


	private:
		// Base input
		WeakReference<InputProvider<ValueType, Args...>> m_input;
	};


	/// <summary>
	/// Type details for HasValueInput
	/// </summary>
	/// <typeparam name="ValueType"> Value-type of the base input </typeparam>
	/// <typeparam name="...Args"> Base input arguments </typeparam>
	template<typename ValueType, typename... Args>
	struct TypeIdDetails::TypeDetails<HasValueInput<ValueType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<bool, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};





	/// <summary>
	/// HasValueInput that is also a Component type
	/// </summary>
	/// <typeparam name="ValueType"> Value-type of the base input </typeparam>
	/// <typeparam name="...Args"> Base input arguments </typeparam>
	template<typename ValueType, typename... Args>
	class HasValueInputComponent : public virtual Component, public virtual HasValueInput<ValueType, Args...> {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~HasValueInputComponent() = 0;

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

		/// <summary>
		/// Exposes properties
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			using BaseHasValueInput = HasValueInput<ValueType, Args...>;
			BaseHasValueInput::GetFields(recordElement);
		}
	};

	/// <summary> Virtual destructor </summary>
	template<typename ValueType, typename... Args>
	inline HasValueInputComponent<ValueType, Args...>::~HasValueInputComponent() {}


	/// <summary>
	/// Type details for HasValueInputComponent
	/// </summary>
	/// <typeparam name="ValueType"> Value-type of the base input </typeparam>
	/// <typeparam name="...Args"> Base input arguments </typeparam>
	template<typename ValueType, typename... Args>
	struct TypeIdDetails::TypeDetails<HasValueInputComponent<ValueType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<HasValueInput<ValueType, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};





	/// <summary>
	/// HasValueInputComponent for integer inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API IntInputHasValueInput final : public HasValueInputComponent<int> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline IntInputHasValueInput(Component* parent, const std::string_view& name = "IntInputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<IntInputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<int>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<IntInputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<IntInputHasValueInput>(
			"Int Input Has Value", "Jimara/Input/HasValue/Int", "HasValueInputComponent for integer inputs");
		report(factory);
	}


	/// <summary>
	/// HasValueInputComponent for boolean inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API BoolInputHasValueInput final : public HasValueInputComponent<bool> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline BoolInputHasValueInput(Component* parent, const std::string_view& name = "BoolInputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<BoolInputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<bool>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<BoolInputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<BoolInputHasValueInput>(
			"Bool Input Has Value", "Jimara/Input/HasValue/Bool", "HasValueInputComponent for booleger inputs");
		report(factory);
	}


	/// <summary>
	/// HasValueInputComponent for floating point inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API FloatInputHasValueInput final : public HasValueInputComponent<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatInputHasValueInput(Component* parent, const std::string_view& name = "FloatInputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatInputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatInputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<FloatInputHasValueInput>(
			"Float Input Has Value", "Jimara/Input/HasValue/Float", "HasValueInputComponent for floating point inputs");
		report(factory);
	}


	/// <summary>
	/// HasValueInputComponent for Vector2 inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2InputHasValueInput final : public HasValueInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2InputHasValueInput(Component* parent, const std::string_view& name = "Vector2InputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2InputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2InputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2InputHasValueInput>(
			"Vector2 Input Has Value", "Jimara/Input/HasValue/Vector2", "HasValueInputComponent for Vector2 inputs");
		report(factory);
	}


	/// <summary>
	/// HasValueInputComponent for Vector3 inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3InputHasValueInput final : public HasValueInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3InputHasValueInput(Component* parent, const std::string_view& name = "Vector3InputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3InputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3InputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3InputHasValueInput>(
			"Vector3 Input Has Value", "Jimara/Input/HasValue/Vector3", "HasValueInputComponent for Vector3 inputs");
		report(factory);
	}


	/// <summary>
	/// HasValueInputComponent for Vector4 inputs
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4InputHasValueInput final : public HasValueInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4InputHasValueInput(Component* parent, const std::string_view& name = "Vector4InputHasValue") : Component(parent, name) {}
	};

	// Type details for concrete type:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4InputHasValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<HasValueInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4InputHasValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4InputHasValueInput>(
			"Vector4 Input Has Value", "Jimara/Input/HasValue/Vector4", "HasValueInputComponent for Vector4 inputs");
		report(factory);
	}
}
