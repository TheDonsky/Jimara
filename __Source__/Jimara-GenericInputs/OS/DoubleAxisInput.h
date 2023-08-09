#pragma once
#include "../Base/VectorInput.h"


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::OS::DoubleAxisInput)

	namespace OS {
		/// <summary>
		/// Generic 2d input from OS axis
		/// </summary>
		class JIMARA_GENERIC_INPUTS_API DoubleAxisInput : public virtual VectorInput::ComponentFrom<Vector2> {
		public:
			/// <summary> Input flags </summary>
			enum class Flags : uint8_t {
				/// <summary> No effect </summary>
				NONE = 0u,

				/// <summary> If present, this flag will cause the input value to always have magnitude of 1 or 0 </summary>
				NORMALIZE = (1u << 0u),

				/// <summary> 
				/// If not present, input value will be present regardless of the trigger action; 
				/// Otherwise, value will only be present if it does not evaluate to Vector2(0) 
				/// </summary>
				NO_VALUE_ON_NO_INPUT = (1u << 1u),

				/// <summary> If true, input will not be produced if the input component is disabled in hierarchy </summary>
				NO_VALUE_IF_DISABLED = (1u << 2u)
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Component name </param>
			DoubleAxisInput(Component* parent, const std::string_view& name = "DoubleAxisInput");

			/// <summary> Virtual destructor </summary>
			virtual ~DoubleAxisInput();

			/// <summary> Horizontal input axis </summary>
			inline Input::Axis HorizontalAxis()const { return m_horizontal; }

			/// <summary>
			/// Sets horizontal input axis
			/// </summary>
			/// <param name="axis"> Input axis </param>
			inline void SetHorizontalAxis(Input::Axis axis) { m_horizontal = axis; }

			/// <summary> Vertical input axis </summary>
			inline Input::Axis VerticalAxis()const { return m_vertical; }

			/// <summary>
			/// Sets vertical input axis
			/// </summary>
			/// <param name="axis"> Input axis </param>
			inline void SetVerticalAxis(Input::Axis axis) { m_vertical = axis; }

			/// <summary> Input device index (for gamepads, mostly) </summary>
			inline uint32_t DeviceId()const { return m_deviceId; }

			/// <summary>
			/// Sets device index
			/// </summary>
			/// <param name="id"> Device to use </param>
			inline void SetDeviceId(uint32_t id) { m_deviceId = id; }

			/// <summary> Input flags/settings </summary>
			inline Flags InputFlags()const { return m_flags; }

			/// <summary>
			/// Sets input flags
			/// </summary>
			/// <param name="flags"> Settings </param>
			inline void SetFlags(Flags flags) { m_flags = flags; }

			/// <summary>
			/// Maximal magnitude of the output;
			/// <para/> If NORMALIZE flag is set and MaxMagnitude is not infinite, it'll act as a value scaler 
			/// </summary>
			inline float MaxMagnitude()const { return m_maxMagnitude; }

			/// <summary>
			/// Sets maximal input magnitude
			/// </summary>
			/// <param name="maxMagnitude"> Maximal input length </param>
			inline void SetMaxMagnitude(float maxMagnitude) { m_maxMagnitude = Math::Max(maxMagnitude, 0.0f); }

			/// <summary>
			/// Exposes fields
			/// </summary>
			/// <param name="recordElement"> Fields will be exposed through this callback </param>
			virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

			/// <summary> Evaluates input </summary>
			virtual std::optional<Vector2> EvaluateInput()override;

		private:
			// Horizontal axis
			Input::Axis m_horizontal = Input::Axis::MOUSE_X;

			// Vertical axis
			Input::Axis m_vertical = Input::Axis::MOUSE_Y;
			
			// Device id
			uint32_t m_deviceId = 0u;

			// Flags
			Flags m_flags = static_cast<Flags>(
				static_cast<std::underlying_type_t<Flags>>(Flags::NO_VALUE_ON_NO_INPUT) |
				static_cast<std::underlying_type_t<Flags>>(Flags::NO_VALUE_IF_DISABLED));

			// Maximal magnitude of the input
			float m_maxMagnitude = std::numeric_limits<float>::infinity();
		};
	}

	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<OS::DoubleAxisInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<Vector2>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<OS::DoubleAxisInput>(const Callback<const Object*>& report);
}
