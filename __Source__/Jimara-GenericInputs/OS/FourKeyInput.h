#pragma once
#include "../Base/VectorInput.h"


namespace Jimara {
	/// <summary> Let the system know about the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::OS::FourKeyInput)

	namespace OS {
		/// <summary>
		/// Generic 2d input from 2 directional keys
		/// </summary>
		class JIMARA_GENERIC_INPUTS_API FourKeyInput : public virtual VectorInput::ComponentFrom<Vector2> {
		public:
			/// <summary> Input flags </summary>
			enum class InputFlags : uint8_t {
				/// <summary> No effect </summary>
				NONE = 0u,

				/// <summary> If present, this flag will cause the input value to always have magnitude of 1 or 0 </summary>
				NORMALIZE = (1u << 0u),

				/// <summary> 
				/// If not present, input value will be present regardless of the keyboard action; 
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
			FourKeyInput(Component* parent, const std::string_view& name = "FourKeyInput");

			/// <summary> Virtual destructor </summary>
			virtual ~FourKeyInput();

			/// <summary> 'Left' (negative X) key </summary>
			inline Input::KeyCode LeftKey()const { return m_left; }

			/// <summary>
			/// Sets 'left' key
			/// </summary>
			/// <param name="key"> Key code </param>
			inline void SetLeftKey(Input::KeyCode key) { m_left = key; }

			/// <summary> 'Right' (positive X) key </summary>
			inline Input::KeyCode RightKey()const { return m_right; }

			/// <summary>
			/// Sets 'right' key
			/// </summary>
			/// <param name="key"> Key code </param>
			inline void SetRightKey(Input::KeyCode key) { m_right = key; }

			/// <summary> 'Up' (positive Y) key </summary>
			inline Input::KeyCode UpKey()const { return m_up; }

			/// <summary>
			/// Sets 'up' key
			/// </summary>
			/// <param name="key"> Key code </param>
			inline void SetUpKey(Input::KeyCode key) { m_up = key; }

			/// <summary> 'Down' (negative Y) key </summary>
			inline Input::KeyCode DownKey()const { return m_down; }

			/// <summary>
			/// Sets 'down' key
			/// </summary>
			/// <param name="key"> Key code </param>
			inline void SetDownKey(Input::KeyCode key) { m_down = key; }

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
			std::optional<Vector2> EvaluateInput()override;

		private:
			// 'Left' key
			Input::KeyCode m_left = Input::KeyCode::A;

			// 'Right' key
			Input::KeyCode m_right = Input::KeyCode::D;

			// 'Up' key
			Input::KeyCode m_up = Input::KeyCode::W;

			// 'Down' key
			Input::KeyCode m_down = Input::KeyCode::S;

			// Device id
			uint32_t m_deviceId = 0u;
			
			// Flags
			InputFlags m_flags = static_cast<InputFlags>(
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NORMALIZE) |
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NO_VALUE_ON_NO_INPUT) |
				static_cast<std::underlying_type_t<InputFlags>>(InputFlags::NO_VALUE_IF_DISABLED));
		};
	}

	// Expose type details:
	template<> inline void TypeIdDetails::GetParentTypesOf<OS::FourKeyInput>(const Callback<TypeId>& report) {
		report(TypeId::Of<VectorInput::ComponentFrom<Vector2>>());
	}
	template<> JIMARA_GENERIC_INPUTS_API void TypeIdDetails::GetTypeAttributesOf<OS::FourKeyInput>(const Callback<const Object*>& report);
}
