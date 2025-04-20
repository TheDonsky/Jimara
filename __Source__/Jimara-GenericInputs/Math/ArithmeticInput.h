#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>


namespace Jimara {
	// Let the system know about the component types
	JIMARA_REGISTER_TYPE(Jimara::FloatArithmeticInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector2ArithmeticInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector3ArithmeticInput);
	JIMARA_REGISTER_TYPE(Jimara::Vector4ArithmeticInput);



	/// <summary>
	/// General input provider that performs basic arithmetic on two values
	/// </summary>
	/// <typeparam name="Type"> Value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	class ArithmeticInputProvider
		: public virtual VectorInput::From<Type, Args...>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Arithmetic operator </summary>
		enum class Operand : uint8_t {
			/// <summary> First()->GetInput() + Second()->GetInput() </summary>
			ADD = 0,

			/// <summary> First()->GetInput() - Second()->GetInput() </summary>
			SUBTRACT = 1,

			/// <summary> First()->GetInput() * Second()->GetInput() </summary>
			MULTIPLY = 2,

			/// <summary> First()->GetInput() / Second()->GetInput() </summary>
			DIVIDE = 3,

			/// <summary> pow(First()->GetInput(), Second()->GetInput()) [For vectors, field-by-field] </summary>
			POW = 4,

			/// <summary> min(First()->GetInput(), Second()->GetInput()) [For vectors, field-by-field] </summary>
			MIN = 5,

			/// <summary> max(First()->GetInput(), Second()->GetInput()) [For vectors, field-by-field] </summary>
			MAX = 6
		};

		/// <summary> Virtual destructor </summary>
		inline virtual ~ArithmeticInputProvider() {}

		/// <summary> 'Left side'/'A' of the equasion </summary>
		inline Reference<InputProvider<Type, Args...>> First()const { return m_a; }

		/// <summary>
		/// Sets first input
		/// </summary>
		/// <param name="provider"> 'Left side' of the equasion </param>
		inline void SetFirst(InputProvider<Type, Args...>* provider) { m_a = provider; }

		/// <summary> 'Right side'/'B' of the equasion </summary>
		inline Reference<InputProvider<Type, Args...>> Second()const { return m_b; }

		/// <summary>
		/// Sets second input
		/// </summary>
		/// <param name="provider"> 'Right side' of the equasion </param>
		inline void SetSecond(InputProvider<Type, Args...>* provider) { m_b = provider; }

		/// <summary> Operator </summary>
		inline Operand Mode()const { return m_operand; }

		/// <summary>
		/// Sets mode
		/// </summary>
		/// <param name="mode"> Operator </param>
		inline void SetMode(Operand mode) { m_operand = Math::Min(mode, Operand::MAX); }

		/// <summary> 
		/// Computes input from First(A) and Second(B) According to the configuration 
		/// </summary>
		/// <param name="...args"> Input arguments, passed through to A and B </param>
		/// <returns> Computed result </returns>
		inline virtual std::optional<Type> EvaluateInput(Args... args)override {
			std::optional<Type> a = Jimara::InputProvider<Type, Args...>::GetInput(m_a, args...);
			std::optional<Type> b = Jimara::InputProvider<Type, Args...>::GetInput(m_b, args...);
			if ((!a.has_value()) || (!b.has_value()))
				return std::optional<Type>();
			const Type& aV = a.value();
			const Type& bV = b.value();
			switch (m_operand)
			{
			case Operand::ADD: return aV + bV;
			case Operand::SUBTRACT: return aV - bV;
			case Operand::MULTIPLY: return aV * bV;
			case Operand::DIVIDE: return aV / bV;
			case Operand::POW: return EvalPow(aV, bV);
			case Operand::MIN: return EvalMin(aV, bV);
			case Operand::MAX: return EvalMax(aV, bV);
			default: return std::optional<Type>();
			}
			return std::optional<Type>();
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
					"Operator", "Arithmetic operator/mode", std::vector<Reference<const Object>> {
					Object::Instantiate<Serialization::EnumAttribute<UnderlyingT>>(false,
						"ADD", Operand::ADD,
						"SUBTRACT", Operand::SUBTRACT,
						"MULTIPLY", Operand::MULTIPLY,
						"DIVIDE", Operand::DIVIDE,
						"POW", Operand::POW,
						"MIN", Operand::MIN,
						"MAX", Operand::MAX)});
				UnderlyingT op = static_cast<UnderlyingT>(m_operand);
				recordElement(serializer->Serialize(&op));
				SetMode(static_cast<Operand>(op));
			}
		}

	private:
		// First
		WeakReference<InputProvider<Type, Args...>> m_a;

		// Second
		WeakReference<InputProvider<Type, Args...>> m_b;

		// Mode
		Operand m_operand = Operand::ADD;

		using VectorFieldType = std::conditional_t<
			std::is_same_v<Type, Vector2> || 
			std::is_same_v<Type, Vector3> || 
			std::is_same_v<Type, Vector4>,
			float, Type>;
		static_assert(std::is_same_v<Vector2::value_type, float>);
		static_assert(std::is_same_v<Vector3::value_type, float>);
		static_assert(std::is_same_v<Vector4::value_type, float>);

		// Pow-helpers
		template<typename T>
		inline static std::enable_if_t<false && !(
			std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, int>
			), T> EvalPowBase(const T& a, const T& b) { 
			using namespace Jimara::Math;
			return Pow(a, b); 
		}

		template<typename T>
		inline static std::enable_if_t<std::is_same_v<T, float>, T> EvalPowBase(const T& a, const T& b) { return std::pow(a, b); }
		template<typename T>
		inline static std::enable_if_t<std::is_same_v<T, double>, T> EvalPowBase(const T& a, const T& b) { return std::pow(a, b); }
		template<typename T>
		inline static std::enable_if_t<std::is_same_v<T, int>, T> EvalPowBase(const T& a, const T& b) { 
			return static_cast<int>(std::pow(static_cast<double>(a), static_cast<double>(b)));
		}

		inline static VectorFieldType EvalPow(const VectorFieldType& a, const VectorFieldType& b) { return EvalPowBase<VectorFieldType>(a, b); }
		inline static Vector2 EvalPow(const Vector2& a, const Vector2& b) { return Vector2(EvalPow(a.x, b.x), EvalPow(a.y, b.y)); }
		inline static Vector3 EvalPow(const Vector3& a, const Vector3& b) { return Vector3(EvalPow(a.x, b.x), EvalPow(a.y, b.y), EvalPow(a.z, b.z)); }
		inline static Vector4 EvalPow(const Vector4& a, const Vector4& b) { return Vector4(EvalPow(a.x, b.x), EvalPow(a.y, b.y), EvalPow(a.z, b.z), EvalPow(a.w, b.w)); }

		// Min-helpers
		inline static VectorFieldType EvalMin(const VectorFieldType& a, const VectorFieldType& b) { return Math::Min(a, b); }
		inline static Vector2 EvalMin(const Vector2& a, const Vector2& b) { return Vector2(EvalMin(a.x, b.x), EvalMin(a.y, b.y)); }
		inline static Vector3 EvalMin(const Vector3& a, const Vector3& b) { return Vector3(EvalMin(a.x, b.x), EvalMin(a.y, b.y), EvalMin(a.z, b.z)); }
		inline static Vector4 EvalMin(const Vector4& a, const Vector4& b) { return Vector4(EvalMin(a.x, b.x), EvalMin(a.y, b.y), EvalMin(a.z, b.z), EvalMin(a.w, b.w)); }

		// Max-helpers
		inline static VectorFieldType EvalMax(const VectorFieldType& a, const VectorFieldType& b) { return Math::Max(a, b); }
		inline static Vector2 EvalMax(const Vector2& a, const Vector2& b) { return Vector2(EvalMax(a.x, b.x), EvalMax(a.y, b.y)); }
		inline static Vector3 EvalMax(const Vector3& a, const Vector3& b) { return Vector3(EvalMax(a.x, b.x), EvalMax(a.y, b.y), EvalMax(a.z, b.z)); }
		inline static Vector4 EvalMax(const Vector4& a, const Vector4& b) { return Vector4(EvalMax(a.x, b.x), EvalMax(a.y, b.y), EvalMax(a.z, b.z), EvalMax(a.w, b.w)); }
	};


	/// <summary>
	/// Arithmetic input provider, that is also a Component
	/// </summary>
	template<typename Type, typename... Args>
	class ArithmeticInputComponent
		: public virtual Component
		, public virtual ArithmeticInputProvider<Type, Args...> {
	public:
		/// <summary> Constructor </summary>
		inline ArithmeticInputComponent() {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ArithmeticInputComponent() = 0;

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			ArithmeticInputProvider<Type, Args...>::GetFields(recordElement);
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
	inline ArithmeticInputComponent<Type, Args...>::~ArithmeticInputComponent() {}



	/// <summary> Concrete ArithmeticInput Component for float type </summary>
	class JIMARA_GENERIC_INPUTS_API FloatArithmeticInput : public virtual ArithmeticInputComponent<float> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatArithmeticInput(Component* parent, const std::string_view& name = "Float Arithmetic") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~FloatArithmeticInput() {}
	};

	/// <summary> Concrete ArithmeticInput Component for 2d vector type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector2ArithmeticInput : public virtual ArithmeticInputComponent<Vector2> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector2ArithmeticInput(Component* parent, const std::string_view& name = "Vector2 Arithmetic") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector2ArithmeticInput() {}
	};

	/// <summary> Concrete ArithmeticInput Component for 3d vector type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector3ArithmeticInput : public virtual ArithmeticInputComponent<Vector3> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector3ArithmeticInput(Component* parent, const std::string_view& name = "Vector3 Arithmetic") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector3ArithmeticInput() {}
	};

	/// <summary> Concrete ArithmeticInput Component for 4d vector type </summary>
	class JIMARA_GENERIC_INPUTS_API Vector4ArithmeticInput : public virtual ArithmeticInputComponent<Vector4> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline Vector4ArithmeticInput(Component* parent, const std::string_view& name = "Vector4 Arithmetic") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Vector4ArithmeticInput() {}
	};





	/// <summary>
	/// Type details for ArithmeticInputProvider
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<ArithmeticInputProvider<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::From<Type, Args...>>());
			reportParentType(TypeId::Of<Serialization::Serializable>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for ArithmeticInputComponent
	/// </summary>
	/// <typeparam name="Type"> Compared value type </typeparam>
	/// <typeparam name="...Args"> Input arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<ArithmeticInputComponent<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<ArithmeticInputComponent<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};



	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatArithmeticInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ArithmeticInputComponent<float>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatArithmeticInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<FloatArithmeticInput>(
			"Float Arithmetic Input", "Jimara/Input/Math/Arithmetic/Float", "Input provider that performs arithmetic operation on two floating point values");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector2ArithmeticInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ArithmeticInputComponent<Vector2>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector2ArithmeticInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector2ArithmeticInput>(
			"Vector2 Arithmetic Input", "Jimara/Input/Math/Arithmetic/Vector2", "Input provider that performs arithmetic operation on two 2d vector values");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector3ArithmeticInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ArithmeticInputComponent<Vector3>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector3ArithmeticInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector3ArithmeticInput>(
			"Vector3 Arithmetic Input", "Jimara/Input/Math/Arithmetic/Vector3", "Input provider that performs arithmetic operation on two 3d vector values");
		report(factory);
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<Vector4ArithmeticInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<ArithmeticInputComponent<Vector4>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<Vector4ArithmeticInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Vector4ArithmeticInput>(
			"Vector4 Arithmetic Input", "Jimara/Input/Math/Arithmetic/Vector4", "Input provider that performs arithmetic operation on two 4d vector values");
		report(factory);
	}
}

