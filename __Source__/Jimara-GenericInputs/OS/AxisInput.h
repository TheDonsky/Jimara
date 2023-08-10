#pragma once
#include "../Base/VectorInput.h"


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::OS::AxisInput)

		namespace OS {
		/// <summary>
		/// Generic float input from OS axis
		/// </summary>
		class JIMARA_GENERIC_INPUTS_API AxisInput : public virtual VectorInput::ComponentFrom<float> {
		public:
			/// <summary> Input flags </summary>
			enum class InputFlags : uint8_t {
				/// <summary> No effect </summary>
				NONE = 0u,

				/// <summary> 
				/// If not present, input value will be present regardless of the trigger action; 
				/// Otherwise, value will only be present if it does not evaluate to 0 
				/// </summary>
				NO_VALUE_ON_NO_INPUT = (1u << 0u),

				/// <summary> If true, input will not be produced if the input component is disabled in hierarchy </summary>
				NO_VALUE_IF_DISABLED = (1u << 1u)
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Component name </param>
			AxisInput(Component* parent, const std::string_view& name = "AxisInput");

			/// <summary> Virtual destructor </summary>
			virtual ~AxisInput();

			/// <summary> Input axis </summary>
			inline Input::Axis Axis()const { return m_axis; }

			/// <summary>
			/// Sets input axis
			/// </summary>
			/// <param name="axis"> Input axis </param>
			inline void SetAxis(Input::Axis axis) { m_axis = axis; }

			/// <summary> Input device index (for gamepads, mostly) </summary>
			inline uint32_t DeviceId()const { return m_deviceId; }

			/// <summary>
			/// Sets device index
			/// </summary>
			/// <param name="id"> Device to use </param>
			inline void SetDeviceId(uint32_t id) { m_deviceId = id; }

			/// <summary> Input flags/settings </summary>
			inline InputFlags Flags()const { return m_flags; }

			/// <summary>
			/// Sets input flags
			/// </summary>
			/// <param name="flags"> Settings </param>
			inline void SetFlags(InputFlags flags) { m_flags = flags; }

			/// <summary>
			/// Exposes fields
			/// </summary>
			/// <param name="recordElement"> Fields will be exposed through this callback </param>
			virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

			/// <summary> Evaluates input </summary>
			virtual std::optional<float> EvaluateInput()override;

		private:
			// Input axis
			Input::Axis m_axis = Input::Axis::MOUSE_X;

			// Device id
			uint32_t m_deviceId = 0u;

			// Flags
			InputFlags m_flags = static_cast<InputFlags>(
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NO_VALUE_ON_NO_INPUT) |
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NO_VALUE_IF_DISABLED));
		};
	}

	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<OS::AxisInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<float>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<OS::AxisInput>(const Callback<const Object*>& report);
}
