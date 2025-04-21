#pragma once
#include "../Base/VectorInput.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>


namespace Jimara {
	// Let the system know about the component type
	JIMARA_REGISTER_TYPE(Jimara::FloatingPointOperationInput);



	/// <summary>
	/// Generic input provider that performs various floating-point operations
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API FloatingPointOperationInputProvider
		: public virtual VectorInput::From<float>
		, public virtual Serialization::Serializable {
	public:
		/// <summary> Floating-point operation </summary>
		enum class Operand : uint8_t {
			/// <summary> value = base </summary>
			VALUE,

			/// <summary> value = -base </summary>
			INVERSE,

			/// <summary> value = 1.0f / base </summary>
			ONE_OVER_VALUE,

			/// <summary> value = abs(base) </summary>
			ABS,

			/// <summary> value = (base < 0.0f) ? (-1.0f) : (base > 0.0f) ? 1.0f : 0.0f </summary>
			SIGN,

			/// <summary> value = ceil(base) </summary>
			CEIL,

			/// <summary> value = floor(base) </summary>
			FLOOR,

			/// <summary> value = log(base, 2) </summary>
			LOG_2,

			/// <summary> value = log(base, 10) </summary>
			LOG_10,

			/// <summary> value = log(base, e) </summary>
			LOG_E,

			/// <summary> value = base * base </summary>
			SQR,

			/// <summary> value = sqrt(base) </summary>
			SQRT,

			/// <summary> value = 1.0f / sqrt(base) </summary>
			ONE_OVER_SQRT,

			/// <summary> value = sin(base) </summary>
			SIN,

			/// <summary> value = cos(base) </summary>
			COS,

			/// <summary> value = tan(base) </summary>
			TAN,

			/// <summary> value = 1.0f / tan(base) </summary>
			CTG,

			/// <summary> value = asin(base) </summary>
			ASIN,

			/// <summary> value = acos(base) </summary>
			ACOS,

			/// <summary> value = atan(base) </summary>
			ATAN,

			/// <summary> value = atan(1.0f / base) </summary>
			ACTG,

			/// <summary> value = sin(base * PI / 180.0f) </summary>
			SIN_ANGLE,

			/// <summary> value = cos(base * PI / 180.0f) </summary>
			COS_ANGLE,

			/// <summary> value = tan(base * PI / 180.0f) </summary>
			TAN_ANGLE,

			/// <summary> value = 1.0f / tan(base * PI / 180.0f) </summary>
			CTG_ANGLE,

			/// <summary> value = asin(base) * 180.0f / PI </summary>
			ASIN_ANGLE,

			/// <summary> value = acos(base) * 180.0f / PI </summary>
			ACOS_ANGLE,

			/// <summary> value = atan(base) * 180.0f / PI </summary>
			ATAN_ANGLE,

			/// <summary> value = atan(1.0f / base) * 180.0f / PI </summary>
			ACTG_ANGLE,

			/// <summary> value = base * PI / 180.0f </summary>
			ANGLE_TO_RADIAN,

			/// <summary> value = base * 180.0f / PI </summary>
			RADIAN_TO_ANGLE,

			/// <summary> Number of options </summary>
			COUNT
		};


		/// <summary> Options for operand values </summary>
		static const Serialization::EnumAttribute<std::underlying_type_t<Operand>>* OperandOptions() {
			static const Reference<Serialization::EnumAttribute<std::underlying_type_t<Operand>>> options =
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Operand>>>(false,
					"VALUE", Operand::VALUE,
					"INVERSE", Operand::INVERSE,
					"ONE_OVER_VALUE", Operand::ONE_OVER_VALUE,
					"ABS", Operand::ABS,
					"SIGN", Operand::SIGN,
					"CEIL", Operand::CEIL,
					"FLOOR", Operand::FLOOR,
					"LOG_2", Operand::LOG_2,
					"LOG_10", Operand::LOG_10,
					"LOG_E", Operand::LOG_E,
					"SQR", Operand::SQR,
					"SQRT", Operand::SQRT,
					"ONE_OVER_SQRT", Operand::ONE_OVER_SQRT,
					"SIN", Operand::SIN,
					"COS", Operand::COS,
					"TAN", Operand::TAN,
					"CTG", Operand::CTG,
					"ASIN", Operand::ASIN,
					"ACOS", Operand::ACOS,
					"ATAN", Operand::ATAN,
					"ACTG", Operand::ACTG,
					"SIN_ANGLE", Operand::SIN_ANGLE,
					"COS_ANGLE", Operand::COS_ANGLE,
					"TAN_ANGLE", Operand::TAN_ANGLE,
					"CTG_ANGLE", Operand::CTG_ANGLE,
					"ASIN_ANGLE", Operand::ASIN_ANGLE,
					"ACOS_ANGLE", Operand::ACOS_ANGLE,
					"ATAN_ANGLE", Operand::ATAN_ANGLE,
					"ACTG_ANGLE", Operand::ACTG_ANGLE,
					"ANGLE_TO_RADIAN", Operand::ANGLE_TO_RADIAN,
					"RADIAN_TO_ANGLE", Operand::RADIAN_TO_ANGLE);
			return options;
		}

		/// <summary> Operation </summary>
		inline Operand Operation()const { return m_operand; }

		/// <summary>
		/// Sets operand to be performed on the base input
		/// </summary>
		/// <param name="operand"> Operation </param>
		inline void SetOperation(Operand operand) { m_operand = (operand < Operand::COUNT) ? operand : Operand::VALUE; }

		/// <summary> Base input provider </summary>
		inline Reference<InputProvider<float>> BaseInput()const { return m_baseInput; }

		/// <summary>
		/// Sets base input
		/// </summary>
		/// <param name="input"> Base input provider </param>
		inline void SetBaseInput(InputProvider<float>* input) { m_baseInput = input; }

		/// <summary> 
		/// Evaluates input
		/// </summary>
		/// <returns> Transformed floating point value </returns>
		inline virtual std::optional<float> EvaluateInput() override {
			const std::optional<float> result = InputProvider<float>::GetInput(m_baseInput);
			if (!result.has_value()) return std::optional<float>();
			const float base = result.value();

			static_assert((1.0f / std::numeric_limits<float>::infinity()) == 0.0f);

			switch (Operation()) {
			case Operand::VALUE: return base;
			case Operand::INVERSE: return -base;
			case Operand::ONE_OVER_VALUE: return 1.0f / base;
			case Operand::ABS: return std::abs(base);
			case Operand::SIGN: return (base < 0.0f) ? (-1.0f) : (base > 0.0f) ? 1.0f : 0.0f;
			case Operand::CEIL: return std::ceil(base);
			case Operand::FLOOR: return std::floor(base);
			case Operand::LOG_2: return std::log2(base);
			case Operand::LOG_10: return std::log10(base);
			case Operand::LOG_E: return std::log(base);
			case Operand::SQR: return base * base;
			case Operand::SQRT: return std::sqrt(base);
			case Operand::ONE_OVER_SQRT: return 1.0f / std::sqrt(base);
			case Operand::SIN: return std::sin(base);
			case Operand::COS: return std::cos(base);
			case Operand::TAN: return std::tan(base);
			case Operand::CTG: return 1.0f / std::tan(base);
			case Operand::ASIN: return std::asin(base);
			case Operand::ACOS: return std::acos(base);
			case Operand::ATAN: return std::atan(base);
			case Operand::ACTG: return (std::abs(base) < std::numeric_limits<float>::epsilon()) ? 0.0f : std::atan(1.0f / base);
			case Operand::SIN_ANGLE: return std::sin(base * (Math::Pi() / 180.0f));
			case Operand::COS_ANGLE: return std::cos(base * (Math::Pi() / 180.0f));
			case Operand::TAN_ANGLE: return std::tan(base * (Math::Pi() / 180.0f));
			case Operand::CTG_ANGLE: return 1.0f / std::tan(base * (Math::Pi() / 180.0f));
			case Operand::ASIN_ANGLE: return std::asin(base) * (180.0f / Math::Pi());
			case Operand::ACOS_ANGLE: return std::acos(base) * (180.0f / Math::Pi());
			case Operand::ATAN_ANGLE: return std::atan(base) * (180.0f / Math::Pi());
			case Operand::ACTG_ANGLE: return (std::abs(base) < std::numeric_limits<float>::epsilon()) ? 0.0f : (std::atan(1.0f / base) * (180.0f / Math::Pi()));
			case Operand::ANGLE_TO_RADIAN: return base * (Math::Pi() / 180.0f);
			case Operand::RADIAN_TO_ANGLE: return base * (180.0f / Math::Pi());
			default: return base;
			}
			return base;
			return result.has_value()
				? std::optional<float>(Math::Magnitude(result.value()))
				: std::optional<float>();
		}

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			{
				static const auto serializer = Serialization::DefaultSerializer<Reference<InputProvider<float>>>::Create(
					"Base Input", "Base input provider");
				Reference<InputProvider<float>> base = BaseInput();
				recordElement(serializer->Serialize(&base));
				SetBaseInput(base);
			}
			{
				using UnderlyingT = std::underlying_type_t<Operand>;
				static const auto serializer = Serialization::DefaultSerializer<UnderlyingT>::Create(
					"Operator", "Floating point operation", std::vector<Reference<const Object>> { OperandOptions() });
				UnderlyingT op = static_cast<UnderlyingT>(Operation());
				recordElement(serializer->Serialize(&op));
				SetOperation(static_cast<Operand>(op));
			}
		}

	private:
		// Base input
		WeakReference<InputProvider<float>> m_baseInput;

		// Operation
		Operand m_operand = Operand::VALUE;
	};


	/// <summary>
	/// Floating-point operation input, that's also a component
	/// </summary>
	class JIMARA_GENERIC_INPUTS_API FloatingPointOperationInput
		: public virtual Component
		, public virtual FloatingPointOperationInputProvider {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline FloatingPointOperationInput(Component* parent, const std::string_view& name = "FloatingPointOperation") : Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~FloatingPointOperationInput() {};

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		inline virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override {
			Component::GetFields(recordElement);
			FloatingPointOperationInputProvider::GetFields(recordElement);
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





	// Type details for component types:
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatingPointOperationInputProvider>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::From<float>>());
		report(TypeId::Of<Serialization::Serializable>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<FloatingPointOperationInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<FloatingPointOperationInputProvider>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<FloatingPointOperationInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<FloatingPointOperationInput>(
			"Floating-Point Operation Input", "Jimara/Input/Math/Floating Point Operation", "Input provider that performs various floating-point operations");
		report(factory);
	}
}
