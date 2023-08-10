#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::Vector2SplitInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3SplitInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4SplitInput);



	/// <summary>
	/// Helper structure for VectorComponent input types
	/// </summary>
	/// <typeparam name="VectorType"> Vector type </typeparam>
	template<typename VectorType>
	struct VectorSplitInputProvider_AxisInfo final {
		/// <summary> Axis index </summary>
		enum class Options : size_t {
			/// <summary> Maximal index </summary>
			LAST = ~size_t(0u)
		};

		/// <summary> Enumeration options </summary>
		inline static const Object* OptionsAttribute() { return nullptr; }

	private:
		// Can not be constructed!
		inline VectorSplitInputProvider_AxisInfo() {}
	};

	/// <summary>
	/// VectorSplitInputProvider_AxisInfo override for 2D inputs
	/// </summary>
	template<>
	struct VectorSplitInputProvider_AxisInfo<Vector2> final {
		/// <summary> Axis index </summary>
		enum class Options : uint8_t {
			/// <summary> X axis </summary>
			X = 0u,

			/// <summary> Y axis </summary>
			Y = 1u,

			/// <summary> Maximal index </summary>
			LAST = Y
		};

		/// <summary> Enumeration options </summary>
		inline static const Object* OptionsAttribute() { 
			static const auto attribute = Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<Options>>>(false,
				"X", Options::X,
				"Y", Options::Y);
			return attribute;
		}

	private:
		// Can not be constructed!
		inline VectorSplitInputProvider_AxisInfo() {}
	};

	/// <summary>
	/// VectorSplitInputProvider_AxisInfo override for 3D inputs
	/// </summary>
	template<>
	struct VectorSplitInputProvider_AxisInfo<Vector3> final {
		/// <summary> Axis index </summary>
		enum class Options : uint8_t {
			/// <summary> X axis </summary>
			X = 0u,

			/// <summary> Y axis </summary>
			Y = 1u,

			/// <summary> Z axis </summary>
			Z = 2u,

			/// <summary> Maximal index </summary>
			LAST = Z
		};

		/// <summary> Enumeration options </summary>
		inline static const Object* OptionsAttribute() {
			static const auto attribute = Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<Options>>>(false,
				"X", Options::X,
				"Y", Options::Y,
				"Z", Options::Z);
			return attribute;
		}

	private:
		// Can not be constructed!
		inline VectorSplitInputProvider_AxisInfo() {}
	};

	/// <summary>
	/// VectorSplitInputProvider_AxisInfo override for 4D inputs
	/// </summary>
	template<>
	struct VectorSplitInputProvider_AxisInfo<Vector4> final {
		/// <summary> Axis index </summary>
		enum class Options : uint8_t {
			/// <summary> X axis </summary>
			X = 0u,

			/// <summary> Y axis </summary>
			Y = 1u,

			/// <summary> Z axis </summary>
			Z = 2u,

			/// <summary> W axis </summary>
			W = 3u,

			/// <summary> Maximal index </summary>
			LAST = W
		};

		/// <summary> Enumeration options </summary>
		inline static const Object* OptionsAttribute() {
			static const auto attribute = Object::Instantiate<Jimara::Serialization::EnumAttribute<std::underlying_type_t<Options>>>(false,
				"X", Options::X,
				"Y", Options::Y,
				"Z", Options::Z,
				"W", Options::W);
			return attribute;
		}

	private:
		// Can not be constructed!
		inline VectorSplitInputProvider_AxisInfo() {}
	};


	/// <summary>
	/// Base component split input
	/// </summary>
	/// <typeparam name="Type"> base input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class VectorSplitInputProvider
		: public virtual VectorInput::From<float, Args...>
		, Serialization::Serializable {
	public:
		/// <summary> Axis index </summary>
		using Axis = typename VectorSplitInputProvider_AxisInfo<Type>::Options;

		/// <summary> Input to split </summary>
		inline Reference<InputProvider<Type, Args...>> BaseInput()const { return m_source; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="input"> Input to split </param>
		inline void SetBaseInput(InputProvider<Type, Args...>* input) { m_source = Math::Min(input, Axis::LAST); }

		/// <summary> Vector component </summary>
		inline Axis InputAxis()const { return m_axis; }

		/// <summary>
		/// Sets Axis
		/// </summary>
		/// <param name="axis"> Vector component </param>
		inline void SetInputAxis(Axis axis) { m_axis = axis; }

		/// <summary> 
		/// Evaluates input and extracts channel
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to the base input </param>
		/// <returns> Vector channel </returns>
		inline virtual std::optional<float> EvaluateInput(Args... args) override {
			const std::optional<Type> result = InputProvider<Type, Args...>::GetInput(m_source, args...);
			return result.has_value() 
				? std::optional<float>(result.value()[static_cast<size_t>(m_axis)]) 
				: std::optional<float>();
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override {
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create(
					"Base Input", "Input value will be a component of this input");
				Reference<InputProvider<Type, Args...>> input = m_source;
				recordElement(serializer->Serialize(&input));
				m_source = input;
			}
			{
				using UnderlyingT = std::underlying_type_t<Axis>;
				static const auto serializer = []() {
					std::vector<Reference<const Object>> attributes;
					const Reference<const Object> attribute = VectorSplitInputProvider_AxisInfo<Type>::OptionsAttribute();
					if (attribute != nullptr)
						attributes.push_back(attribute);
					return Serialization::DefaultSerializer<UnderlyingT>::Create("Axis", "Vector component", attributes);
				}();
				UnderlyingT axis = static_cast<UnderlyingT>(m_axis);
				recordElement(serializer->Serialize(&axis));
				SetInputAxis(static_cast<Axis>(axis));
			}
		}

	private:
		// Base input
		WeakReference<InputProvider<Type, Args...>> m_source;

		// Vector component index
		Axis m_axis = static_cast<Axis>(0u);
	};

	/// <summary>
	/// Vector component input provider, that is also a Component
	/// </summary>
	template<typename Type, typename... Args>
	class VectorSplitInputComponent
		: public virtual Component
		, public virtual VectorSplitInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline VectorSplitInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~VectorSplitInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			VectorSplitInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline VectorSplitInputComponent<Type, Args...>::~VectorSplitInputComponent() {}



	/// <summary> Concrete SplitInput Component for Vector2 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2SplitInput : public virtual VectorSplitInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2SplitInput(Component* parent, const std::string_view& name = "Vector2Split") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2SplitInput() {}
	};

	/// <summary> Concrete SplitInput Component for Vector3 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3SplitInput : public virtual VectorSplitInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3SplitInput(Component* parent, const std::string_view& name = "Vector3Split") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3SplitInput() {}
	};

	/// <summary> Concrete SplitInput Component for Vector4 type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4SplitInput : public virtual VectorSplitInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4SplitInput(Component* parent, const std::string_view& name = "Vector4Split") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4SplitInput() {}
	};





	/// <summary>
	/// Type details for VectorSplitInputProvider
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorSplitInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<float, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorSplitInputComponent
	/// </summary>
	/// <typeparam name="Type"> Vector value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorSplitInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorSplitInputProvider<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2SplitInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorSplitInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2SplitInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector2SplitInput> serializer("Jimara/Input/Math/VectorSplit/Vector2", "Vector2 Split Input");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3SplitInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorSplitInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3SplitInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector3SplitInput> serializer("Jimara/Input/Math/VectorSplit/Vector3", "Vector3 Split Input");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4SplitInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorSplitInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4SplitInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Vector4SplitInput> serializer("Jimara/Input/Math/VectorSplit/Vector4", "Vector4 Split Input");
		report(&serializer);
	}
}
