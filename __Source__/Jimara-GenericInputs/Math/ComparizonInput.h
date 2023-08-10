#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::BooleanComparizonInput);
	JIMARA_REGISTER_TYPE(Jimara::FloatComparizonInput);
	JIMARA_REGISTER_TYPE(Jimara::IntComparizonInput);



	/// <summary>
	/// Base comparator object with input references and operators
	/// </summary>
	/// <typeparam name="Type"> Compared input value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class ComparizonInputProvider 
		: public virtual VectorInput::From<bool, Args...>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Comparizon operator </summary>
		enum class Operand : uint8_t {
			/// <summary> First()->GetInput() < Second()->GetInput() </summary>
			LESS = 0u,

			/// <summary> First()->GetInput() <= Second()->GetInput() </summary>
			LESS_OR_EQUAL = 1u,

			/// <summary> First()->GetInput() == Second()->GetInput() </summary>
			EQUAL = 2u,

			/// <summary> First()->GetInput() >= Second()->GetInput() </summary>
			GREATER_OR_EQUAL = 3u,

			/// <summary> First()->GetInput() > Second()->GetInput() </summary>
			GREATER = 4u
		};

		/// <summary> Comparizon flags </summary>
		enum class InputFlags : uint8_t {
			/// <summary> No effect </summary>
			NONE = 0u,

			/// <summary> Operand will be inverted </summary>
			INVERSE_VALUE = (1u << 0u)
		};

		/// <summary> Virtual destructor </summary>
		inline virtual ~ComparizonInputProvider() {}

		/// <summary> 'Left side'/'A' of the comparizon </summary>
		inline Reference<InputProvider<Type, Args...>> First()const { return m_a; }

		/// <summary>
		/// Sets first input
		/// </summary>
		/// <param name="provider"> 'Left side' of the comparizon </param>
		inline void SetFirst(InputProvider<Type, Args...>* provider) { m_a = provider; }

		/// <summary> 'Right side'/'B' of the comparizon </summary>
		inline Reference<InputProvider<Type, Args...>> Second()const { return m_b; }

		/// <summary>
		/// Sets second input
		/// </summary>
		/// <param name="provider"> 'Right side' of the comparizon </param>
		inline void SetSecond(InputProvider<Type, Args...>* provider) { m_b = provider; }

		/// <summary> Comparizon operator </summary>
		inline Operand Mode()const { return m_operand; }

		/// <summary>
		/// Sets mode
		/// </summary>
		/// <param name="mode"> Comparizon operator </param>
		inline void SetMode(Operand mode) { m_operand = Math::Min(mode, Operand::GREATER); }

		/// <summary> Input flags/settings </summary>
		inline InputFlags Flags()const { return m_flags; }

		/// <summary>
		/// Sets input flags
		/// </summary>
		/// <param name="flags"> Settings </param>
		inline void SetFlags(InputFlags flags) { m_flags = flags; }

		/// <summary> 
		/// Compares First(A) and Second(B) According to the configuration 
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to A and B </param>
		/// <returns> Comparizon result </returns>
		inline virtual std::optional<bool> EvaluateInput(Args... args)override {
			std::optional<Type> a = Jimara::InputProvider<Type, Args...>::GetInput(m_a, args...);
			std::optional<Type> b = Jimara::InputProvider<Type, Args...>::GetInput(m_b, args...);
			const int vDelta = a.has_value()
				? (b.has_value()
					? ((b < a) ? 1 :
						(a < b) ? -1 : 0)
					: 1)
				: (b.has_value() ? -1 : 0);
			const bool inverse = ((static_cast<std::underlying_type_t<InputFlags>>(m_flags) & 
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::INVERSE_VALUE)) != 0u);
			return inverse ^ ([&]() {
				switch (m_operand)
				{
				case Operand::LESS: return (vDelta < 0);
				case Operand::LESS_OR_EQUAL: return (vDelta <= 0);
				case Operand::EQUAL: return (vDelta == 0);
				case Operand::GREATER_OR_EQUAL: return (vDelta >= 0);
				case Operand::GREATER: return (vDelta > 0);
				default: return false;
				}
				return false;
				}());
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create(
					"A", "First input / Left side of the equasion");
				Reference<InputProvider<Type, Args...>> a = m_a;
				recordElement(serializer->Serialize(&a));
				m_a = a;
			}
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<Type, Args...>>>::Create(
					"B", "Second input / Right side of the equasion");
				Reference<InputProvider<Type, Args...>> b = m_b;
				recordElement(serializer->Serialize(&b));
				m_b = b;
			}
			{
				using UnderlyingT = std::underlying_type_t<Operand>;
				static const auto serializer = Serialization::DefaultSerializer<UnderlyingT>::Create(
					"Operator", "Comparizon operator/mode", std::vector<Reference<const Object>> {
					Object::Instantiate<Serialization::EnumAttribute<UnderlyingT>>(false,
						"LESS", Operand::LESS,
						"LESS_OR_EQUAL", Operand::LESS_OR_EQUAL,
						"EQUAL", Operand::EQUAL,
						"GREATER_OR_EQUAL", Operand::GREATER_OR_EQUAL,
						"GREATER", Operand::GREATER)});
				UnderlyingT op = static_cast<UnderlyingT>(m_operand);
				recordElement(serializer->Serialize(&op));
				SetMode(static_cast<Operand>(op));
			}
			{
				using UnderlyingT = std::underlying_type_t<InputFlags>;
				static const auto serializer = Serialization::DefaultSerializer<UnderlyingT>::Create(
					"Flags", "Input settings", std::vector<Reference<const Object>> {
					Object::Instantiate<Serialization::EnumAttribute<UnderlyingT>>(true,
						"INVERSE_VALUE", InputFlags::INVERSE_VALUE)});
				UnderlyingT flags = static_cast<UnderlyingT>(m_flags);
				recordElement(serializer->Serialize(&flags));
				SetFlags(static_cast<InputFlags>(flags));
			}
		}

	private:
		// First
		WeakReference<InputProvider<Type, Args...>> m_a;

		// Second
		WeakReference<InputProvider<Type, Args...>> m_b;

		// Mode
		Operand m_operand = Operand::LESS;

		// Flags
		InputFlags m_flags = InputFlags::NONE;
	};



	/// <summary>
	/// Comparizon input provider, that is also a Component
	/// </summary>
	template<typename Type, typename... Args>
	class ComparizonInputComponent
		: public virtual Component
		, public virtual ComparizonInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline ComparizonInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ComparizonInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			ComparizonInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline ComparizonInputComponent<Type, Args...>::~ComparizonInputComponent() {}



	/// <summary> Concrete ComparizonInput Component for bool type </summary>
	class JIMARA_GENERIC_INPUTS_API BooleanComparizonInput : public virtual ComparizonInputComponent<bool> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline BooleanComparizonInput(Component* parent, const std::string_view& name = "Boolean") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~BooleanComparizonInput() {}
	};

	/// <summary> Concrete ComparizonInput Component for float type </summary>
	class JIMARA_GENERIC_INPUTS_API FloatComparizonInput : public virtual ComparizonInputComponent<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatComparizonInput(Component* parent, const std::string_view& name = "Float") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~FloatComparizonInput() {}
	};

	/// <summary> Concrete ComparizonInput Component for int type </summary>
	class JIMARA_GENERIC_INPUTS_API IntComparizonInput : public virtual ComparizonInputComponent<int> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline IntComparizonInput(Component* parent, const std::string_view& name = "Integer") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~IntComparizonInput() {}
	};





	/// <summary>
	/// Type details for ComparizonInputProvider
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<ComparizonInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<bool, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for ComparizonInputComponent
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<ComparizonInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<ComparizonInputComponent<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<BooleanComparizonInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ComparizonInputComponent<bool>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<BooleanComparizonInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<BooleanComparizonInput> serializer("Jimara/Input/Math/Compare/Boolean", "Boolean Comparator Input");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatComparizonInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ComparizonInputComponent<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatComparizonInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<FloatComparizonInput> serializer("Jimara/Input/Math/Compare/Float", "Floating point Comparator Input");
		report(&serializer);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<IntComparizonInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ComparizonInputComponent<int>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<IntComparizonInput>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<IntComparizonInput> serializer("Jimara/Input/Math/Compare/Integer", "Integer Comparator Input");
		report(&serializer);
	}
}
