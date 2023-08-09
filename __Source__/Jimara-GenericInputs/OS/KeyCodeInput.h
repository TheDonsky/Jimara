#pragma once
#include "../Base/VectorInput.h"


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::OS::KeyCodeInput)

	namespace OS {
		/// <summary>
		/// Generic boolean input from OS KeyCode input
		/// </summary>
		class JIMARA_GENERIC_INPUTS_API KeyCodeInput : public virtual VectorInput::ComponentFrom<bool> {
		public:
			/// <summary> Input mode/action </summary>
			enum class Mode : uint8_t {
				/// <summary> Positive input is never produced </summary>
				NO_INPUT = 0u,

				/// <summary> Positive input produced when key gets pressed </summary>
				ON_KEY_DOWN = 1u,

				/// <summary> Positive input produced while key is pressed </summary>
				ON_KEY_PRESSED = 2u,

				/// <summary> Positive input produced when key gets released </summary>
				ON_KEY_UP = 3u
			};

			/// <summary> Input flags </summary>
			enum class Flags : uint8_t {
				/// <summary> No effect </summary>
				NONE = 0u,

				/// <summary> If present, input value will be inverted </summary>
				INVERT_INPUT_MODE = (1u << 0u),

				/// <summary> 
				/// If not present, input value will be present regardless of the keyboard action; 
				/// Otherwise, value will only be present if it evaluates to true 
				/// </summary>
				NO_VALUE_ON_FALSE_INPUT = (1u << 1u),

				/// <summary> If true, input will not be produced if the input component is disabled in hierarchy </summary>
				NO_VALUE_IF_DISABLED = (1u << 2u)
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Component name </param>
			KeyCodeInput(Component* parent, const std::string_view& name = "KeyCodeInput");
			
			/// <summary> Virtual destructor </summary>
			virtual ~KeyCodeInput();

			/// <summary> Input key code </summary>
			inline Input::KeyCode KeyCode()const { return m_key; }

			/// <summary>
			/// Sets input key
			/// </summary>
			/// <param name="code"> Key code </param>
			inline void SetKeyCode(Input::KeyCode code) { m_key = Math::Min(code, Input::KeyCode::KEYCODE_COUNT); }

			/// <summary> Input device index (for gamepads, mostly) </summary>
			inline uint32_t DeviceId()const { return m_deviceId; }

			/// <summary>
			/// Sets device index
			/// </summary>
			/// <param name="id"> Device to use </param>
			inline void SetDeviceId(uint32_t id) { m_deviceId = id; }

			/// <summary> Input mode </summary>
			inline Mode InputMode()const { return m_mode; }

			/// <summary>
			/// Sets input mode
			/// </summary>
			/// <param name="mode"> Input mode/action </param>
			inline void SetInputMode(Mode mode) { m_mode = Math::Min(mode, Mode::ON_KEY_UP); }

			/// <summary> Input flags/settings </summary>
			inline Flags InputFlags()const { return m_flags; }

			/// <summary>
			/// Sets input flags
			/// </summary>
			/// <param name="flags"> Settings </param>
			inline void SetFlags(Flags flags) { m_flags = flags; }

			/// <summary>
			/// Exposes fields
			/// </summary>
			/// <param name="recordElement"> Fields will be exposed through this callback </param>
			virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) override;

			/// <summary> Evaluates input </summary>
			virtual std::optional<bool> EvaluateInput() override;

		private:
			// Input key code
			Input::KeyCode m_key = Input::KeyCode::NONE;
			
			// Device id
			uint32_t m_deviceId = 0u;

			// Pulse mode
			Mode m_mode = Mode::ON_KEY_DOWN;

			// Flags
			Flags m_flags = Flags::NO_VALUE_IF_DISABLED;
		};
	}


	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<OS::KeyCodeInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<VectorInput::From<bool>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<OS::KeyCodeInput>(const Callback<const Object*>& report);
}
