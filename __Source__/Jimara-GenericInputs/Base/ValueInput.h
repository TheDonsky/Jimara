#pragma once
#include "VectorInput.h"
#include <Jimara/Data/Serialization/Serializable.h>
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/Components/Transform.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::BooleanValueInput);
	JIMARA_REGISTER_TYPE(Jimara::FloatValueInput);
	JIMARA_REGISTER_TYPE(Jimara::IntValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector2ValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3ValueInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4ValueInput);



	/// <summary>
	/// Fixed value input provider for any type
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class GenericValueInputProvider
		: public virtual InputProvider<Type>
		, public virtual std::optional<Type> {
	public:
		/// <summary> Returns itself </summary>
		inline std::optional<Type> GetInput() override { 
			const std::optional<Type>* self = this;
			return *self;
		}

		/// <summary>
		/// Helper function for serialization;
		/// </summary>
		/// <param name="reportField"> Fields will be reported through this </param>
		/// <param name="value"> Value to serialize </param>
		inline static void GetFields(const Callback<Serialization::SerializedObject>& reportField, std::optional<Type>* value) {
			{
				static const auto serializer = Serialization::BoolSerializer::Create("Has Value", "If true, input will emit a value");
				bool hasValue = value->has_value();
				reportField(serializer->Serialize(hasValue));
				if (hasValue && (!value->has_value()))
					(*value) = Type();
				else if ((!hasValue) && value->has_value())
					(*value) = std::optional<Type>();
			}
			if (value->has_value()) {
				static const auto serializer = Serialization::DefaultSerializer<Type>::Create("Value", "Input value");
				reportField(serializer->Serialize(&value->value()));
			}
		}
	};

	/// <summary>
	/// Fixed value input provider for VectorInput-compatible types
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class VectorValueInputProvider
		: public virtual VectorInput::From<Type>
		, public virtual std::optional<Type> {
	public:
		// Only enabled for VectorInput types
		static_assert(VectorInput::IsCompatibleType<Type>);

		/// <summary> Returns itself </summary>
		inline std::optional<Type> EvaluateInput() override {
			const std::optional<Type>* self = this;
			return *self;
		}

		/// <summary>
		/// Helper function for serialization;
		/// </summary>
		/// <param name="reportField"> Fields will be reported through this </param>
		/// <param name="value"> Value to serialize </param>
		inline static void GetFields(const Callback<Serialization::SerializedObject>& reportField, std::optional<Type>* value) {
			GenericValueInputProvider<Type>::GetFields(reportField, value);
		}
	};


	/// <summary>
	/// Default fixed value input implementation
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	using ValueInputProvider = std::conditional_t<VectorInput::IsCompatibleType<Type>, VectorValueInputProvider<Type>, GenericValueInputProvider<Type>>;





	/// <summary>
	/// Value input provider that is also a Component
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	class ValueInputComponent
		: public virtual Component
		, public virtual ValueInputProvider<Type> {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~ValueInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			GenericValueInputProvider<Type>::GetFields(recordElement, this);
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
	template<typename Type>
	inline ValueInputComponent<Type>::~ValueInputComponent() {}


	/// <summary> Concrete ValueInput Component for bool type </summary>
	class JIMARA_GENERIC_INPUTS_API BooleanValueInput : public virtual ValueInputComponent<bool> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline BooleanValueInput(Component* parent, const std::string_view& name = "Boolean") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~BooleanValueInput() {}
	};

	/// <summary> Concrete ValueInput Component for float type </summary>
	class JIMARA_GENERIC_INPUTS_API FloatValueInput : public virtual ValueInputComponent<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatValueInput(Component* parent, const std::string_view& name = "Float") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~FloatValueInput() {}
	};

	/// <summary> Concrete ValueInput Component for int type </summary>
	class JIMARA_GENERIC_INPUTS_API IntValueInput : public virtual ValueInputComponent<int> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline IntValueInput(Component* parent, const std::string_view& name = "Integer") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~IntValueInput() {}
	};

	/// <summary> Concrete ValueInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2ValueInput : public virtual ValueInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2ValueInput(Component* parent, const std::string_view& name = "Vector2") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2ValueInput() {}
	};

	/// <summary> Concrete ValueInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3ValueInput : public virtual ValueInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3ValueInput(Component* parent, const std::string_view& name = "Vector3") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3ValueInput() {}
	};

	/// <summary> Concrete ValueInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4ValueInput : public virtual ValueInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4ValueInput(Component* parent, const std::string_view& name = "Vector4") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4ValueInput() {}
	};





	/// <summary>
	/// Type details for VectorValueInputProvider
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<VectorValueInputProvider<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<Type>>());
			reportParentType(TypeId::Of<std::optional<Type>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for GenericValueInputProvider
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<GenericValueInputProvider<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<InputProvider<Type>>());
			reportParentType(TypeId::Of<std::optional<Type>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for ValueInputComponent
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<ValueInputComponent<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<ValueInputProvider<Type>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<BooleanValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<bool>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<BooleanValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<BooleanValueInput>(
			"Boolean Value Input", "Jimara/Input/Value/Boolean", "Fixed boolean value input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<FloatValueInput>(
			"Float Value Input", "Jimara/Input/Value/Float", "Fixed floating point value input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<IntValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<int>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<IntValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<IntValueInput>(
			"Integer Value Input", "Jimara/Input/Value/Integer", "Fixed integer value input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2ValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2ValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2ValueInput>(
			"Vector2 Value Input", "Jimara/Input/Value/Vector2", "Fixed Vector2 value input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3ValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3ValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3ValueInput>(
			"Vector3 Value Input", "Jimara/Input/Value/Vector3", "Fixed Vector3 value input provider");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4ValueInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ValueInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4ValueInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4ValueInput>(
			"Vector4 Value Input", "Jimara/Input/Value/Vector4", "Fixed Vector4 value input provider");
		report(factory);
	}
}
