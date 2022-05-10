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
			inline EditorInput(OS::Input* baseInput) : m_baseInput(baseInput) {
				for (uint8_t code = 0; code < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); code++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						m_baseInput->OnKeyDown(static_cast<KeyCode>(code), deviceId) += Callback(&EditorInput::InvokeOnKeyDownEvent, this);
						m_baseInput->OnKeyPressed(static_cast<KeyCode>(code), deviceId) += Callback(&EditorInput::InvokeOnKeyPressedEvent, this);
						m_baseInput->OnKeyUp(static_cast<KeyCode>(code), deviceId) += Callback(&EditorInput::InvokeOnKeyUpEvent, this);
					}
				for (uint8_t axis = 0; axis < static_cast<uint8_t>(Axis::AXIS_COUNT); axis++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++)
						m_baseInput->OnInputAxis(static_cast<Axis>(axis), deviceId) += Callback(&EditorInput::InvokeAxisEvent, this);
			}

			/// <summary> Virtual destructor </summary>
			inline virtual ~EditorInput() {
				for (uint8_t code = 0; code < static_cast<size_t>(KeyCode::KEYCODE_COUNT); code++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						m_baseInput->OnKeyDown(static_cast<KeyCode>(code), deviceId) -= Callback(&EditorInput::InvokeOnKeyDownEvent, this);
						m_baseInput->OnKeyPressed(static_cast<KeyCode>(code), deviceId) -= Callback(&EditorInput::InvokeOnKeyPressedEvent, this);
						m_baseInput->OnKeyUp(static_cast<KeyCode>(code), deviceId) -= Callback(&EditorInput::InvokeOnKeyUpEvent, this);
					}
				for (uint8_t axis = 0; axis < static_cast<uint8_t>(Axis::AXIS_COUNT); axis++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++)
						m_baseInput->OnInputAxis(static_cast<Axis>(axis), deviceId) -= Callback(&EditorInput::InvokeAxisEvent, this);
			}

			/// <summary> maximal number of controllers supported </summary>
			inline static constexpr uint8_t MaxControllerCount() { return MAX_CONTROLLER_COUNT; }

			/// <summary> True, if the input is enabled </summary>
			inline bool Enabled()const { return m_enabled.load(); }

			/// <summary>
			/// Enables/disables the input (useful, when the window gets or looses focus)
			/// </summary>
			/// <param name="enabled"> If true, the input will become active </param>
			inline void SetEnabled(bool enabled) { m_enabled = enabled; }

			/// <summary> Mouse offset (useful for 'following' the active window and reporting stuff relative to it) </summary>
			inline Vector2 MouseOffset()const { return Vector2(m_mouseOffsetX.load(), m_mouseOffsetY.load()); }

			/// <summary>
			/// Sets mouse offset (useful for 'following' the active window and reporting stuff relative to it)
			/// </summary>
			/// <param name="offset"> Focused window's position </param>
			inline void SetMouseOffset(const Vector2& offset) {
				m_mouseOffsetX = offset.x;
				m_mouseOffsetY = offset.y;
			}

			/// <summary> Mouse position/delta position scale </summary>
			inline float MouseScale()const { return m_mouseScale.load(); }

			/// <summary>
			/// Sets mouse position/delta position scale (useful for 'following' the active window and reporting stuff relative to it)
			/// </summary>
			/// <param name="scale"> Scale to use </param>
			inline void SetMouseScale(float scale) { m_mouseScale = scale; }

			/// <summary>
			/// True, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was down </returns>
			inline virtual bool KeyDown(KeyCode code, uint8_t deviceId = 0)const override { 
				return m_keyStates[static_cast<uint8_t>(code)][deviceId].gotPressed;
			}

			/// <summary>
			/// Invoked, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key press event (params: code, deviceId, this) </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyDown(KeyCode code, uint8_t deviceId = 0)const {
				return GetEvent(m_onKeyDown, code, deviceId);
			}

			/// <summary>
			/// True, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was pressed </returns>
			inline virtual bool KeyPressed(KeyCode code, uint8_t deviceId = 0)const override {
				return m_keyStates[static_cast<uint8_t>(code)][deviceId].wasPressed;
			}

			/// <summary>
			/// Invoked, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key pressed event (params: code, deviceId, this) </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyPressed(KeyCode code, uint8_t deviceId = 0)const {
				return GetEvent(m_onKeyPressed, code, deviceId);
			}

			/// <summary>
			/// True, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was released </returns>
			inline virtual bool KeyUp(KeyCode code, uint8_t deviceId = 0)const override {
				return m_keyStates[static_cast<uint8_t>(code)][deviceId].gotReleased;
			}

			/// <summary>
			/// Invoked, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key released event (params: code, deviceId, this) </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyUp(KeyCode code, uint8_t deviceId = 0)const override {
				return GetEvent(m_onKeyUp, code, deviceId);
			}

			/// <summary>
			/// Gets current input from the axis
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Current input value </returns>
			inline virtual float GetAxis(Axis axis, uint8_t deviceId = 0)const override {
				return TransformAxisValue(axis, m_axisStates[static_cast<uint8_t>(axis)][deviceId]);
			}

			/// <summary>
			/// Reports axis input whenevr it's active (ei, mainly, non-zero)
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Axis value reporter (params: axis, value, deviceId, this) </returns>
			inline virtual Event<Axis, float, uint8_t, const Input*>& OnInputAxis(Axis axis, uint8_t deviceId = 0)const override {
				return GetEvent(m_onInputAxis, axis, deviceId);
			}

			/// <summary>
			/// Updates the underlying input
			/// </summary>
			/// <param name="deltaTime"> Time since last update </param>
			inline virtual void Update(float deltaTime) override {
				for (uint8_t code = 0; code < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); code++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						KeyCodeState& state = m_keyStates[code][deviceId];
						state.gotPressed = m_currentlyEnabled.load() && m_baseInput->KeyDown(static_cast<KeyCode>(code), deviceId);
						state.wasPressed = (m_currentlyEnabled.load() || state.wasPressed) && m_baseInput->KeyPressed(static_cast<KeyCode>(code), deviceId);
						state.gotReleased = m_currentlyEnabled.load() && m_baseInput->KeyUp(static_cast<KeyCode>(code), deviceId);
					}
				for (uint8_t axis = 0; axis < static_cast<uint8_t>(Axis::AXIS_COUNT); axis++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						float& state = m_axisStates[axis][deviceId];
						state = (m_currentlyEnabled.load() || std::abs(state) > std::numeric_limits<float>::epsilon())
							? m_baseInput->GetAxis(static_cast<Axis>(axis), deviceId) : 0.0f;
					}
				m_currentlyEnabled = m_enabled.load();
				m_baseInput->Update(deltaTime);
			}

			
		private:
			// Base input
			const Reference<OS::Input> m_baseInput;

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
				bool gotPressed = false;
				bool wasPressed = false;
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

			// Event invokers
			inline void InvokeOnKeyDownEvent(KeyCode code, uint8_t deviceId, const Input*) {
				if (!m_currentlyEnabled.load()) return;
				else if (code >= KeyCode::KEYCODE_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				m_keyStates[static_cast<uint8_t>(code)][deviceId].gotPressed = true;
				GetEvent(m_onKeyDown, code, deviceId)(code, deviceId, this);
			}
			inline void InvokeOnKeyPressedEvent(KeyCode code, uint8_t deviceId, const Input*) {
				if (code >= KeyCode::KEYCODE_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				else if ((!m_currentlyEnabled.load()) && (!(KeyPressed(code, deviceId) || KeyDown(code, deviceId)))) return;
				m_keyStates[static_cast<uint8_t>(code)][deviceId].wasPressed = true;
				GetEvent(m_onKeyPressed, code, deviceId)(code, deviceId, this);
			}
			inline void InvokeOnKeyUpEvent(KeyCode code, uint8_t deviceId, const Input*) {
				if (code >= KeyCode::KEYCODE_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				else if ((!m_currentlyEnabled.load()) && (!(KeyPressed(code, deviceId) || KeyDown(code, deviceId)))) return;
				m_keyStates[static_cast<uint8_t>(code)][deviceId].gotReleased = true;
				GetEvent(m_onKeyUp, code, deviceId)(code, deviceId, this);
			}
			inline void InvokeAxisEvent(Axis axis, float value, uint8_t deviceId, const Input*) {
				if (axis >= Axis::AXIS_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				else if ((!m_currentlyEnabled.load()) && std::abs(m_axisStates[static_cast<uint8_t>(axis)][deviceId]) <= std::numeric_limits<float>::epsilon()) return;
				m_axisStates[static_cast<uint8_t>(axis)][deviceId] = value;
				GetEvent(m_onInputAxis, axis, deviceId)(axis, TransformAxisValue(axis, value), deviceId, this);
			}
		};
	}
}
