#include "EditorInput.h"


namespace Jimara {
	namespace Editor {
		struct EditorInput::Helpers {
			inline static void InvokeOnKeyDownEvent(EditorInput* self, KeyCode code, uint8_t deviceId, const Input*) {
				if (!self->m_currentlyEnabled.load()) return;
				else if (code >= KeyCode::KEYCODE_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				self->m_keyStates[static_cast<uint8_t>(code)][deviceId].gotPressedSignal = true;
			}
			inline static void InvokeOnKeyUpEvent(EditorInput* self, KeyCode code, uint8_t deviceId, const Input*) {
				if (code >= KeyCode::KEYCODE_COUNT || deviceId >= MAX_CONTROLLER_COUNT) return;
				else if ((!self->m_currentlyEnabled.load()) && (!(self->KeyPressed(code, deviceId) || self->KeyDown(code, deviceId)))) return;
				self->m_keyStates[static_cast<uint8_t>(code)][deviceId].gotReleasedSignal = true;
			}
		};

		EditorInput::EditorInput(OS::Input* baseInput) : m_baseInput(baseInput) {
			for (uint8_t code = 0; code < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); code++)
				for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
					m_baseInput->OnKeyDown(static_cast<KeyCode>(code), deviceId) += Callback(&Helpers::InvokeOnKeyDownEvent, this);
					m_baseInput->OnKeyUp(static_cast<KeyCode>(code), deviceId) += Callback(&Helpers::InvokeOnKeyUpEvent, this);
				}
		}

		EditorInput::~EditorInput() {
			for (uint8_t code = 0; code < static_cast<size_t>(KeyCode::KEYCODE_COUNT); code++)
				for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
					m_baseInput->OnKeyDown(static_cast<KeyCode>(code), deviceId) -= Callback(&Helpers::InvokeOnKeyDownEvent, this);
					m_baseInput->OnKeyUp(static_cast<KeyCode>(code), deviceId) -= Callback(&Helpers::InvokeOnKeyUpEvent, this);
				}
		}

		bool EditorInput::Enabled()const { return m_enabled.load(); }
		void EditorInput::SetEnabled(bool enabled) { m_enabled = enabled; }

		Vector2 EditorInput::MouseOffset()const { return Vector2(m_mouseOffsetX.load(), m_mouseOffsetY.load()); }
		void EditorInput::SetMouseOffset(const Vector2& offset) {
			m_mouseOffsetX = offset.x;
			m_mouseOffsetY = offset.y;
		}

		float EditorInput::MouseScale()const { return m_mouseScale.load(); }
		void EditorInput::SetMouseScale(float scale) { m_mouseScale = scale; }

		bool EditorInput::KeyDown(KeyCode code, uint8_t deviceId)const {
			return m_keyStates[static_cast<uint8_t>(code)][deviceId].gotPressed;
		}
		Event<OS::Input::KeyCode, uint8_t, const OS::Input*>& EditorInput::OnKeyDown(KeyCode code, uint8_t deviceId)const {
			return GetEvent(m_onKeyDown, code, deviceId);
		}

		bool EditorInput::KeyPressed(KeyCode code, uint8_t deviceId)const {
			return m_keyStates[static_cast<uint8_t>(code)][deviceId].wasPressed;
		}
		Event<OS::Input::KeyCode, uint8_t, const OS::Input*>& EditorInput::OnKeyPressed(KeyCode code, uint8_t deviceId)const {
			return GetEvent(m_onKeyPressed, code, deviceId);
		}

		bool EditorInput::KeyUp(KeyCode code, uint8_t deviceId)const {
			return m_keyStates[static_cast<uint8_t>(code)][deviceId].gotReleased;
		}
		Event<OS::Input::KeyCode, uint8_t, const OS::Input*>& EditorInput::OnKeyUp(KeyCode code, uint8_t deviceId)const {
			return GetEvent(m_onKeyUp, code, deviceId);
		}

		float EditorInput::GetAxis(Axis axis, uint8_t deviceId)const {
			return TransformAxisValue(axis, m_axisStates[static_cast<uint8_t>(axis)][deviceId]);
		}
		Event<OS::Input::Axis, float, uint8_t, const OS::Input*>& EditorInput::OnInputAxis(Axis axis, uint8_t deviceId)const {
			return GetEvent(m_onInputAxis, axis, deviceId);
		}

		OS::Input::CursorLock EditorInput::CursorLockMode()const {
			return static_cast<Input::CursorLock>(m_lockMode.load());
		}
		void EditorInput::SetCursorLockMode(OS::Input::CursorLock mode)const {
			m_lockMode.store(static_cast<std::underlying_type_t<decltype(mode)>>(mode));
		}

		void EditorInput::Update(float deltaTime) {
			m_currentlyEnabled = m_enabled.load();
			m_baseInput->Update(deltaTime);
			const bool currentlyEnabled = m_currentlyEnabled.load();
			float oldAxisStates[static_cast<size_t>(Axis::AXIS_COUNT)][MAX_CONTROLLER_COUNT] = { 0.0f };

			// Update internal state:
			{
				std::unique_lock<std::mutex> lock(m_updateLock);
				for (uint8_t code = 0; code < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); code++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						KeyCodeState& state = m_keyStates[code][deviceId];
						if (currentlyEnabled) {
							const bool baseIsPressed = m_baseInput->KeyPressed(static_cast<KeyCode>(code), deviceId);
							state.gotPressed = state.gotPressedSignal || ((!state.wasPressed) && baseIsPressed);
							state.gotReleased = state.gotReleasedSignal || (state.wasPressed && (!baseIsPressed));
							state.wasPressed = baseIsPressed;
						}
						else {
							state.gotReleased = state.gotPressed || state.wasPressed;
							state.gotPressed = state.wasPressed = false;
						}
						state.gotPressedSignal = state.gotReleasedSignal = false;
					}
				for (uint8_t axis = 0; axis < static_cast<uint8_t>(Axis::AXIS_COUNT); axis++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++)
					{
						float& state = m_axisStates[axis][deviceId];
						const float lastValue = state;
						oldAxisStates[axis][deviceId] = lastValue;
						const float baseValue = m_baseInput->GetAxis(static_cast<Axis>(axis), deviceId);
						if (axis == static_cast<uint8_t>(Axis::MOUSE_POSITION_X) || axis == static_cast<uint8_t>(Axis::MOUSE_POSITION_Y)) {
							uint8_t deltaAxis = static_cast<uint8_t>((axis == static_cast<uint8_t>(Axis::MOUSE_POSITION_X))
								? Axis::MOUSE_DELTA_POSITION_X
								: Axis::MOUSE_DELTA_POSITION_Y);
							state = baseValue;
							m_axisStates[deltaAxis][deviceId] = (baseValue - lastValue);
						}
						else if (axis == static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_X) || axis == static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_Y)) continue;
						else if (currentlyEnabled || std::abs(state) > std::numeric_limits<float>::epsilon())
							state = baseValue;
						else state = 0.0f;
					}
			}

			// Fire events:
			{
				for (uint8_t code = 0; code < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); code++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						const KeyCodeState& state = m_keyStates[code][deviceId];
						const KeyCode keyCode = static_cast<KeyCode>(code);
						if (state.gotPressed) GetEvent(m_onKeyDown, keyCode, deviceId)(keyCode, deviceId, this);
						if (state.wasPressed) GetEvent(m_onKeyPressed, keyCode, deviceId)(keyCode, deviceId, this);
						if (state.gotReleased) GetEvent(m_onKeyUp, keyCode, deviceId)(keyCode, deviceId, this);
					}
				for (uint8_t axis = 0; axis < static_cast<uint8_t>(Axis::AXIS_COUNT); axis++)
					for (uint8_t deviceId = 0; deviceId < MAX_CONTROLLER_COUNT; deviceId++) {
						const float state = m_axisStates[axis][deviceId];
						const float oldState = oldAxisStates[axis][deviceId];
						const Axis inputAxis = static_cast<Axis>(axis);
						if (inputAxis == Axis::MOUSE_POSITION_X || inputAxis == Axis::MOUSE_POSITION_Y || (state != oldState) || state != 0.0f)
							GetEvent(m_onInputAxis, inputAxis, deviceId)(inputAxis, TransformAxisValue(inputAxis, state), deviceId, this);
					}
			}
		}
	}
}
