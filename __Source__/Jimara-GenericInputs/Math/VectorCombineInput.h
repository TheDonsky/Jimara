#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>



namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::Vector2CombineInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3CombineInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4CombineInput);


	/// <summary>
	/// Generic axis-combine input provider for vectors
	/// </summary>
	/// <typeparam name="VectorType"> Type of the vector </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename VectorType, typename... Args>
	class VectorCombineInputProvider
		: public virtual VectorInput::From<VectorType, Args...>
		, Serialization::Serializable {
	public:
		/// <summary> Value-type of an individual axis </summary>
		using ValueType = typename VectorType::value_type;

		/// <summary> Generic axis input definition </summary>
		using AxisInput = InputProvider<ValueType, Args...>;

		/// <summary> Number of axis components, making up the vector </summary>
		static const constexpr size_t AXIS_COUNT = static_cast<size_t>(VectorType::length());

		/// <summary>
		/// Retrieves value source for an axis by index
		/// </summary>
		/// <param name="axis"> Axis index </param>
		/// <returns> Base input for the axis </returns>
		inline Reference<AxisInput> AxisSource(size_t axis)const { return m_sources[axis]; }

		/// <summary>
		/// Sets value source for an axis by index
		/// </summary>
		/// <param name="axis"> Axis index </param>
		/// <param name="input"> Base input for the axis </param>
		inline void SetAxisSource(size_t axis, AxisInput* input) { m_sources[axis] = input; }

		/// <summary> 
		/// Evaluates input by combining channels
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base input </param>
		/// <returns> Combined vector (filled with 0-s) </returns>
		inline virtual std::optional<VectorType> EvaluateInput(Args... args) override {
			VectorType result;
			for (typename VectorType::length_type i = 0u; i < static_cast<typename VectorType::length_type>(AXIS_COUNT); i++) {
				const std::optional<ValueType> axisValue = AxisInput::GetInput(m_sources[i], args...);
				if (axisValue.has_value()) {
					result[i] = axisValue.value();
				}
				else {
					result[i] = static_cast<ValueType>(0.0f);
				}
			}
			return result;
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override {
			static const Reference<const Serialization::ItemSerializer::Of<Reference<AxisInput>>>* SERIALIZERS = []() {
				static Reference<const Serialization::ItemSerializer::Of<Reference<AxisInput>>> serializers[AXIS_COUNT];
				for (size_t i = 0u; i < AXIS_COUNT; i++) {
					std::stringstream axisNameStream;
					if (AXIS_COUNT > 4u)
						axisNameStream << i;
					else axisNameStream << (
						(i == 0u) ? 'X' :
						(i == 1u) ? 'Y' :
						(i == 2u) ? 'Z' :
						'W');
					const std::string axisName = axisNameStream.str();
					const std::string hint = "Input source for " + axisName + " axis";
					serializers[i] = Serialization::DefaultSerializer<Reference<AxisInput>>::Create(axisName, hint);
				}
				return serializers;
				}();
			for (size_t i = 0u; i < AXIS_COUNT; i++) {
				Reference<AxisInput> input = AxisSource(i);
				recordElement(SERIALIZERS[i]->Serialize(&input));
				SetAxisSource(i, input);
			}
		}

	private:
		// Source-input per axis
		WeakReference<AxisInput> m_sources[AXIS_COUNT];

		// A few assertions to check sanity
		static_assert(std::is_same_v<Vector2::value_type, float>);
		static_assert(std::is_same_v<Vector3::value_type, float>);
		static_assert(std::is_same_v<Vector4::value_type, float>);
		static_assert(Vector2::length() == 2u);
		static_assert(Vector3::length() == 3u);
		static_assert(Vector4::length() == 4u);
	};


	/// <summary>
	/// Type details for VectorCombineInputProvider
	/// </summary>
	/// <typeparam name="VectorType"> Type of the vector </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename VectorType, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorCombineInputProvider<VectorType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<VectorType, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};


	/// <summary>
	/// Vector axis-combine input provider, that is also a Component
	/// </summary>
	/// <typeparam name="VectorType"> Type of the vector </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename VectorType, typename... Args>
	class VectorCombineInputComponent
		: public virtual Component
		, public virtual VectorCombineInputProvider<VectorType, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline VectorCombineInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorCombineInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorCombineInputProvider<VectorType, Args...>::GetFields(recordElement);
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
	template<typename VectorType, typename... Args>
	inline VectorCombineInputComponent<VectorType, Args...>::~VectorCombineInputComponent() {}


	/// <summary>
	/// Type details for VectorCombineInputComponent
	/// </summary>
	/// <typeparam name="VectorType"> Type of the vector </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename VectorType, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorCombineInputComponent<VectorType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorCombineInputProvider<VectorType, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};



	/// <summary> Concrete CombineInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2CombineInput : public virtual VectorCombineInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2CombineInput(Component* parent, const std::string_view& name = "Vector2Combine") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2CombineInput() {}
	};

	/// <summary> Concrete CombineInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3CombineInput : public virtual VectorCombineInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3CombineInput(Component* parent, const std::string_view& name = "Vector3Combine") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3CombineInput() {}
	};

	/// <summary> Concrete CombineInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4CombineInput : public virtual VectorCombineInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4CombineInput(Component* parent, const std::string_view& name = "Vector4Combine") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4CombineInput() {}
	};


	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2CombineInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorCombineInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2CombineInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2CombineInput>(
			"Vector2 Combine Input", "Jimara/Input/Math/VectorCombine/Vector2", "Input provider that produces 2d Vector value by combining channels");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3CombineInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorCombineInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3CombineInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3CombineInput>(
			"Vector3 Combine Input", "Jimara/Input/Math/VectorCombine/Vector3", "Input provider that produces 3d Vector value by combining channels");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4CombineInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorCombineInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4CombineInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4CombineInput>(
			"Vector4 Combine Input", "Jimara/Input/Math/VectorCombine/Vector4", "Input provider that produces 4d Vector value by combining channels");
		report(factory);
	}
}
