#pragma once
#include <OS/Input/Input.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Input for editor windows (mainly for Editor scene)
		/// </summary>
		class EditorInput : public virtual OS::Input {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="baseInput"> Input, to bese the EditoInput on </param>
			EditorInput(OS::Input* baseInput);

			/// <summary> Virtual destructor </summary>
			virtual ~EditorInput();

			/// <summary> maximal number of controllers supported </summary>
			static constexpr uint8_t MaxControllerCount() { return MAX_CONTROLLER_COUNT; }

			/// <summary> True, if the input is enabled </summary>
			bool Enabled()const;

			/// <summary>
			/// Enables/disables the input (useful, when the window gets or looses focus)
			/// </summary>
			/// <param name="enabled"> If true, the input will become active </param>
			void SetEnabled(bool enabled);

			/// <summary> Mouse offset (useful for 'following' the active window and reporting stuff relative to it) </summary>
			Vector2 MouseOffset()const;

			/// <summary>
			/// Sets mouse offset (useful for 'following' the active window and reporting stuff relative to it)
			/// </summary>
			/// <param name="offset"> Focused window's position </param>
			void SetMouseOffset(const Vector2& offset);

			/// <summary> Mouse position/delta position scale </summary>
			float MouseScale()const;

			/// <summary>
			/// Sets mouse position/delta position scale (useful for 'following' the active window and reporting stuff relative to it)
			/// </summary>
			/// <param name="scale"> Scale to use </param>
			void SetMouseScale(float scale);

			/// <summary>
			/// True, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was down </returns>
			virtual bool KeyDown(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Invoked, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key press event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyDown(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// True, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was pressed </returns>
			virtual bool KeyPressed(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Invoked, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key pressed event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyPressed(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// True, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was released </returns>
			virtual bool KeyUp(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Invoked, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key released event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyUp(KeyCode code, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Gets current input from the axis
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Current input value </returns>
			virtual float GetAxis(Axis axis, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Reports axis input whenever it's active (ei, mainly, non-zero)
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Axis value reporter (params: axis, value, deviceId, this) </returns>
			virtual Event<Axis, float, uint8_t, const Input*>& OnInputAxis(Axis axis, uint8_t deviceId = 0)const override;

			/// <summary>
			/// Updates the underlying input
			/// </summary>
			/// <param name="deltaTime"> Time since last update </param>
			virtual void Update(float deltaTime) override;

			
		private:
			// Base input
			const Reference<OS::Input> m_baseInput;

			// Lock
			std::mutex m_updateLock;

			// True, if enabled
			std::atomic<bool> m_enabled = true;

			// Mouse offset
			std::atomic<float> m_mouseOffsetX = 0.0f;
			std::atomic<float> m_mouseOffsetY = 0.0f;

			// Mouse scale
			std::atomic<float> m_mouseScale = 1.0f;

			// Events
			static const constexpr uint8_t MAX_CONTROLLER_COUNT = 8;
			mutable EventInstance<KeyCode, uint8_t, const Input*> m_onKeyDown[static_cast<size_t>(KeyCode::KEYCODE_COUNT) * MAX_CONTROLLER_COUNT];
			mutable EventInstance<KeyCode, uint8_t, const Input*> m_onKeyPressed[static_cast<size_t>(KeyCode::KEYCODE_COUNT) * MAX_CONTROLLER_COUNT];
			mutable EventInstance<KeyCode, uint8_t, const Input*> m_onKeyUp[static_cast<size_t>(KeyCode::KEYCODE_COUNT) * MAX_CONTROLLER_COUNT];
			mutable EventInstance<Axis, float, uint8_t, const Input*> m_onInputAxis[static_cast<size_t>(Axis::AXIS_COUNT) * MAX_CONTROLLER_COUNT];

			// States
			std::atomic<bool> m_currentlyEnabled = true;
			struct KeyCodeState {
				bool gotPressedSignal = false;
				bool gotPressed = false;

				bool wasPressed = false;

				bool gotReleasedSignal = false;
				bool gotReleased = false;
			};
			KeyCodeState m_keyStates[static_cast<size_t>(KeyCode::KEYCODE_COUNT)][MAX_CONTROLLER_COUNT];
			float m_axisStates[static_cast<size_t>(Axis::AXIS_COUNT)][MAX_CONTROLLER_COUNT] = { 0.0f };

			// 'Transformed' Axis value
			inline float TransformAxisValue(Axis axis, float value)const {
				return
					(axis == Axis::MOUSE_POSITION_X) ? ((value - m_mouseOffsetX.load()) * m_mouseScale.load()) :
					(axis == Axis::MOUSE_POSITION_Y) ? ((value - m_mouseOffsetY.load()) * m_mouseScale.load()) :
					(axis == Axis::MOUSE_DELTA_POSITION_X || axis == Axis::MOUSE_DELTA_POSITION_Y) ? (value * m_mouseScale.load()) : value;
			}

			// Event getter
			template<typename KeyType, typename... ValueTypes>
			inline static EventInstance<KeyType, ValueTypes...>& GetEvent(
				EventInstance<KeyType, ValueTypes...>* events, KeyType key, uint8_t deviceId) {
				static EventInstance<KeyType, ValueTypes...> emptyEvent;
				const constexpr size_t CodeCount = std::is_same_v<KeyType, KeyCode> ? static_cast<size_t>(KeyCode::KEYCODE_COUNT) : static_cast<size_t>(Axis::AXIS_COUNT);
				if (static_cast<size_t>(key) >= CodeCount || deviceId >= MAX_CONTROLLER_COUNT) return emptyEvent;
				else return events[(static_cast<size_t>(key) * MAX_CONTROLLER_COUNT) + deviceId];
			}

			// Some private stuff is here..
			struct Helpers;
		};
	}
}
