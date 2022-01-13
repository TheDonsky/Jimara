#include "GLFW_Input.h"
#include "../../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace OS {
		namespace {
			class InputCallbacks : public virtual ObjectCache<Reference<GLFW_Window>>::StoredObject {
			private:
				const Reference<GLFW_Window> m_window;

				class Callbacks : public virtual ObjectCache<GLFWwindow*>::StoredObject {
				public:
					class Cache : public virtual ObjectCache<GLFWwindow*> {
					public:
						inline static Reference<Callbacks> ForHandle(GLFWwindow* window) {
							static Cache cache;
							return cache.GetCachedOrCreate(window, false, [&]()->Reference<Callbacks> { return Object::Instantiate<Callbacks>(); });
						}
					};

					EventInstance<float> onScroll;
				};

				const Reference<Callbacks> m_callbacks;

				static void OnScroll(GLFWwindow* window, double, double yoffset) {
					Reference<Callbacks> instance = Callbacks::Cache::ForHandle(window);
					instance->onScroll((float)yoffset);
				}

			public:
				inline InputCallbacks(GLFW_Window* window) : m_window(window), m_callbacks(Callbacks::Cache::ForHandle(window->Handle())) {
					std::unique_lock<std::shared_mutex> lock(m_window->MessageLock());
					glfwSetScrollCallback(m_window->Handle(), InputCallbacks::OnScroll);
				}

				inline ~InputCallbacks() {
					std::unique_lock<std::shared_mutex> lock(m_window->MessageLock());
					glfwSetScrollCallback(m_window->Handle(), nullptr);
				}

				class Cache : ObjectCache<Reference<GLFW_Window>> {
				public:
					inline static Reference<InputCallbacks> ForWindow(GLFW_Window* window) {
						static Cache cache;
						return cache.GetCachedOrCreate(window, false, [&]()->Reference<InputCallbacks> { return Object::Instantiate<InputCallbacks>(window); });
					}
				};

				Event<float>& OnScroll()const { return m_callbacks->onScroll; }
			};
		}

		GLFW_Input::GLFW_Input(GLFW_Window* window) 
			: m_window(window), m_callbacks(InputCallbacks::Cache::ForWindow(window))
			, m_monitorSize([&]() {
			std::unique_lock<std::shared_mutex> lock(GLFW_Window::APILock());
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			window->Log()->Debug("GLFW_Input::GLFW_Input - Screen height: ", mode->height);
			return static_cast<float>(mode->height);
				}()) {
			m_window->OnPollEvents() += Callback(&GLFW_Input::Poll, this);
			Reference<InputCallbacks> callbacks = m_callbacks;
			callbacks->OnScroll() += Callback(&GLFW_Input::OnScroll, this);
		}

		GLFW_Input::~GLFW_Input() {
			Reference<InputCallbacks> callbacks = m_callbacks;
			callbacks->OnScroll() -= Callback(&GLFW_Input::OnScroll, this);
			m_window->OnPollEvents() -= Callback(&GLFW_Input::Poll, this);
		}


		bool GLFW_Input::KeyDown(KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).gotPressed; }
		Event<Input::KeyCode, uint8_t, const Input*>& GLFW_Input::OnKeyDown(KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).onDown; }

		bool GLFW_Input::KeyPressed(Input::KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).wasPressed; }
		Event<Input::KeyCode, uint8_t, const Input*>& GLFW_Input::OnKeyPressed(KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).onPressed; }

		bool GLFW_Input::KeyUp(KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).gotReleased; }
		Event<Input::KeyCode, uint8_t, const Input*>& GLFW_Input::OnKeyUp(KeyCode code, uint8_t deviceId)const { return GetKeyState(code, deviceId).onUp; }


		float GLFW_Input::GetAxis(Axis axis, uint8_t deviceId)const { return GetAxisState(axis, deviceId).lastValue; }
		Event<Input::Axis, float, uint8_t, const Input*>& GLFW_Input::OnInputAxis(Axis axis, uint8_t deviceId)const { return GetAxisState(axis, deviceId).onAxis; }



		void GLFW_Input::Update(float deltaTime) {
			std::unique_lock<std::mutex> updateLock(m_updateLock);

			// Update Key states:
			{
				auto updateKey = [&](KeyState& state) {
					state.gotPressed = state.signal.pressed;
					state.gotReleased = state.signal.released;
					state.wasPressed = (state.gotPressed || state.gotReleased || state.signal.currentlyPressed);
					state.signal.pressed = false;
					state.signal.released = false;
				};
				for (uint8_t i = 0; i < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); i++) updateKey(m_keys[i]);
				for (uint8_t i = 0; i < GLFW_JOYSTICK_LAST; i++)
					for (uint8_t j = static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST); j <= static_cast<uint8_t>(KeyCode::CONTROLLER_LAST); j++)
						updateKey(m_controllerKeys[i][j - static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST)]);
			}

			// Update axis states:
			{
				{
					auto deltaValue = [](AxisState& state) { return state.curValue - state.lastValue; };
					m_axis[static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_X)].curValue = deltaValue(m_axis[static_cast<uint8_t>(Axis::MOUSE_POSITION_X)]);
					m_axis[static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_Y)].curValue = deltaValue(m_axis[static_cast<uint8_t>(Axis::MOUSE_POSITION_Y)]);
				}
				{
					float divider = deltaTime * static_cast<float>(m_monitorSize);
					if (divider > 0) {
						m_axis[static_cast<uint8_t>(Axis::MOUSE_X)].curValue = m_axis[static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_X)].curValue / divider;
						m_axis[static_cast<uint8_t>(Axis::MOUSE_Y)].curValue = m_axis[static_cast<uint8_t>(Axis::MOUSE_DELTA_POSITION_Y)].curValue / divider;
					}
					else if (deltaTime > 0.0f) {
						m_axis[static_cast<uint8_t>(Axis::MOUSE_X)].curValue = 0.0f;
						m_axis[static_cast<uint8_t>(Axis::MOUSE_Y)].curValue = 0.0f;
					}
				}
				auto updateAxis = [&](AxisState& state) {
					state.changed = state.curValue != state.lastValue;
					state.lastValue = state.curValue;
				};
				for (uint8_t i = 0; i < static_cast<uint8_t>(Axis::AXIS_COUNT); i++)
					updateAxis(m_axis[i]);
				for (uint8_t i = 0; i < GLFW_JOYSTICK_LAST; i++)
					for (uint8_t j = static_cast<uint8_t>(Axis::CONTROLLER_FIRST); j <= static_cast<uint8_t>(Axis::CONTROLLER_LAST); j++)
						updateAxis(m_controllerAxis[i][j - static_cast<uint8_t>(Axis::CONTROLLER_FIRST)]);
			}

			// Invoke key events:
			{
				auto signalKey = [&](const KeyState& state, KeyCode code, uint8_t deviceId) {
					bool reportedPressed = state.signal.currentlyPressed;
					if (state.gotPressed) {
						state.onDown(code, deviceId, this);
						reportedPressed = true;
					}
					if (state.gotReleased) {
						state.onUp(code, deviceId, this);
						reportedPressed = false;
					}
					if (reportedPressed != state.signal.currentlyPressed) {
						if (state.signal.currentlyPressed) state.onDown(code, deviceId, this);
						else state.onUp(code, deviceId, this);
					}
					if (state.wasPressed) state.onPressed(code, deviceId, this);

				};
				for (uint8_t i = 0; i < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); i++)
					signalKey(m_keys[i], static_cast<KeyCode>(i), 0);
				for (uint8_t i = 0; i < GLFW_JOYSTICK_LAST; i++)
					for (uint8_t j = static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST); j <= static_cast<uint8_t>(KeyCode::CONTROLLER_LAST); j++)
						signalKey(m_controllerKeys[i][j - static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST)], static_cast<KeyCode>(j), i + 1);
			}
			
			// Invoke Axis events:
			{
				auto signalAxis = [&](AxisState& state, Axis axis, uint8_t deviceId) {
					if (axis == Axis::MOUSE_POSITION_X || axis == Axis::MOUSE_POSITION_Y || state.changed || state.lastValue != 0.0f) 
						state.onAxis(axis, state.lastValue, deviceId, this);
					state.changed = false;
					if (axis == Axis::MOUSE_SCROLL_WHEEL) state.curValue = 0;
				};
				for (uint8_t i = 0; i < static_cast<uint8_t>(Axis::AXIS_COUNT); i++)
					signalAxis(m_axis[i], static_cast<Axis>(i), 0);
				for (uint8_t i = 0; i < GLFW_JOYSTICK_LAST; i++)
					for (uint8_t j = static_cast<uint8_t>(Axis::CONTROLLER_FIRST); j <= static_cast<uint8_t>(Axis::CONTROLLER_LAST); j++)
						signalAxis(m_controllerAxis[i][j - static_cast<uint8_t>(Axis::CONTROLLER_FIRST)], static_cast<Axis>(j), i + 1);
			}
		}



		const GLFW_Input::KeyState& GLFW_Input::GetKeyState(KeyCode code, uint8_t deviceId)const {
			static const KeyState NO_STATE;
			if (code >= KeyCode::KEYCODE_COUNT) return NO_STATE;
			if (deviceId > 0) {
				deviceId--;
				if (deviceId >= GLFW_JOYSTICK_LAST || code < KeyCode::CONTROLLER_FIRST || code > KeyCode::CONTROLLER_LAST) return NO_STATE;
				else return m_controllerKeys[deviceId][static_cast<uint8_t>(code) - static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST)];
			}
			else return m_keys[static_cast<uint8_t>(code)];
		}

		const GLFW_Input::AxisState& GLFW_Input::GetAxisState(Axis axis, uint8_t deviceId)const {
			static const AxisState NO_STATE;
			if (axis >= Axis::AXIS_COUNT) return NO_STATE;
			if (deviceId > 0) {
				deviceId--;
				if (deviceId >= GLFW_JOYSTICK_LAST || axis < Axis::CONTROLLER_FIRST || axis > Axis::CONTROLLER_LAST) return NO_STATE;
				else return m_controllerAxis[deviceId][static_cast<uint8_t>(axis) - static_cast<uint8_t>(Axis::CONTROLLER_FIRST)];
			}
			else return m_axis[static_cast<uint8_t>(axis)];
		}

		void GLFW_Input::Poll(GLFW_Window* window) {
			std::unique_lock<std::mutex> updateLock(m_updateLock);

			static auto signalKey = [](KeyState& state, bool pressed) {
				if (pressed) {
					if (!state.signal.currentlyPressed) {
						state.signal.pressed = true;
						state.signal.currentlyPressed = true;
					}
				}
				else if (state.signal.currentlyPressed) {
					state.signal.released = true;
					state.signal.currentlyPressed = false;
				}
			};

			typedef void(*PollKeyStateFn)(GLFW_Window* window, KeyState& state);
			static const PollKeyStateFn* POLL_KEY_STATE = []() -> const PollKeyStateFn* {
				static PollKeyStateFn pollKeyState[static_cast<uint8_t>(KeyCode::KEYCODE_COUNT)];
				static auto doNothing = [](GLFW_Window*, KeyState&) {};
				
				for (uint8_t i = 0; i < static_cast<uint8_t>(KeyCode::KEYCODE_COUNT); i++)
					pollKeyState[i] = doNothing;

				static auto getMouseKeyState = [](GLFW_Window* window, KeyState& state, int key) {
					signalKey(state, glfwGetMouseButton(window->Handle(), key) == GLFW_PRESS);
				};
				pollKeyState[static_cast<uint8_t>(KeyCode::MOUSE_LEFT_BUTTON)] = [](GLFW_Window* window, KeyState& state) { getMouseKeyState(window, state, GLFW_MOUSE_BUTTON_LEFT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::MOUSE_MIDDLE_BUTTON)] = [](GLFW_Window* window, KeyState& state) { getMouseKeyState(window, state, GLFW_MOUSE_BUTTON_MIDDLE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::MOUSE_RIGHT_BUTTON)] = [](GLFW_Window* window, KeyState& state) { getMouseKeyState(window, state, GLFW_MOUSE_BUTTON_RIGHT); };


				static auto getKeyboardKeyState = [](GLFW_Window* window, KeyState& state, int key) {
					signalKey(state, glfwGetKey(window->Handle(), key) == GLFW_PRESS);
				};
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_0)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_0); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_1)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_1); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_2)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_2); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_3)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_3); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_4)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_4); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_5)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_5); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_6)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_6); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_7)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_7); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_8)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_8); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ALPHA_9)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_9); };
				pollKeyState[static_cast<uint8_t>(KeyCode::A)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_A); };
				pollKeyState[static_cast<uint8_t>(KeyCode::B)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_B); };
				pollKeyState[static_cast<uint8_t>(KeyCode::C)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_C); };
				pollKeyState[static_cast<uint8_t>(KeyCode::D)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_D); };
				pollKeyState[static_cast<uint8_t>(KeyCode::E)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_E); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F); };
				pollKeyState[static_cast<uint8_t>(KeyCode::G)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_G); };
				pollKeyState[static_cast<uint8_t>(KeyCode::H)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_H); };
				pollKeyState[static_cast<uint8_t>(KeyCode::I)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_I); };
				pollKeyState[static_cast<uint8_t>(KeyCode::J)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_J); };
				pollKeyState[static_cast<uint8_t>(KeyCode::K)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_K); };
				pollKeyState[static_cast<uint8_t>(KeyCode::L)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_L); };
				pollKeyState[static_cast<uint8_t>(KeyCode::M)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_M); };
				pollKeyState[static_cast<uint8_t>(KeyCode::N)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_N); };
				pollKeyState[static_cast<uint8_t>(KeyCode::O)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_O); };
				pollKeyState[static_cast<uint8_t>(KeyCode::P)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_P); };
				pollKeyState[static_cast<uint8_t>(KeyCode::Q)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_Q); };
				pollKeyState[static_cast<uint8_t>(KeyCode::R)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_R); };
				pollKeyState[static_cast<uint8_t>(KeyCode::S)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_S); };
				pollKeyState[static_cast<uint8_t>(KeyCode::T)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_T); };
				pollKeyState[static_cast<uint8_t>(KeyCode::U)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_U); };
				pollKeyState[static_cast<uint8_t>(KeyCode::V)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_V); };
				pollKeyState[static_cast<uint8_t>(KeyCode::W)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_W); };
				pollKeyState[static_cast<uint8_t>(KeyCode::X)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_X); };
				pollKeyState[static_cast<uint8_t>(KeyCode::Y)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_Y); };
				pollKeyState[static_cast<uint8_t>(KeyCode::Z)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_Z); };
				pollKeyState[static_cast<uint8_t>(KeyCode::SPACE)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_SPACE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::COMMA)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_COMMA); };
				pollKeyState[static_cast<uint8_t>(KeyCode::DOT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_PERIOD); };
				pollKeyState[static_cast<uint8_t>(KeyCode::SLASH)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_SLASH); };
				pollKeyState[static_cast<uint8_t>(KeyCode::BACKSLASH)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_BACKSLASH); };
				pollKeyState[static_cast<uint8_t>(KeyCode::SEMICOLON)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_SEMICOLON); };
				pollKeyState[static_cast<uint8_t>(KeyCode::APOSTROPHE)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_APOSTROPHE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::LEFT_BRACKET)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_LEFT_BRACKET); };
				pollKeyState[static_cast<uint8_t>(KeyCode::RIGHT_BRACKET)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_RIGHT_BRACKET); };
				pollKeyState[static_cast<uint8_t>(KeyCode::MINUS)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_MINUS); };
				pollKeyState[static_cast<uint8_t>(KeyCode::EQUALS)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_EQUAL); };
				pollKeyState[static_cast<uint8_t>(KeyCode::GRAVE_ACCENT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_GRAVE_ACCENT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ESCAPE)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_ESCAPE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::ENTER)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_ENTER); };
				pollKeyState[static_cast<uint8_t>(KeyCode::BACKSPACE)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_BACKSPACE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::DELETE_KEY)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_DELETE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::TAB)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_TAB); };
				pollKeyState[static_cast<uint8_t>(KeyCode::CAPS_LOCK)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_CAPS_LOCK); };
				pollKeyState[static_cast<uint8_t>(KeyCode::LEFT_SHIFT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_LEFT_SHIFT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::RIGHT_SHIFT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_RIGHT_SHIFT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::LEFT_CONTROL)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_LEFT_CONTROL); };
				pollKeyState[static_cast<uint8_t>(KeyCode::RIGHT_CONTROL)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_RIGHT_CONTROL); };
				pollKeyState[static_cast<uint8_t>(KeyCode::LEFT_ALT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_LEFT_ALT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::RIGHT_ALT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_RIGHT_ALT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::UP_ARROW)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_UP); };
				pollKeyState[static_cast<uint8_t>(KeyCode::DOWN_ARROW)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_DOWN); };
				pollKeyState[static_cast<uint8_t>(KeyCode::LEFT_ARROW)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_LEFT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::RIGHT_ARROW)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_RIGHT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F1)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F1); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F2)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F2); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F3)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F3); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F4)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F4); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F5)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F5); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F6)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F6); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F7)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F7); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F8)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F8); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F9)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F9); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F10)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F10); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F11)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F11); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F12)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F12); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F13)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F13); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F14)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F14); };
				pollKeyState[static_cast<uint8_t>(KeyCode::F15)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_F15); };
				pollKeyState[static_cast<uint8_t>(KeyCode::PRINT_SCREEN)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_PRINT_SCREEN); };
				pollKeyState[static_cast<uint8_t>(KeyCode::SCROLL_LOCK)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_SCROLL_LOCK); };
				pollKeyState[static_cast<uint8_t>(KeyCode::PAUSE_BREAK)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_PAUSE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUM_LOCK)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_NUM_LOCK); };
				pollKeyState[static_cast<uint8_t>(KeyCode::INSERT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_INSERT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::HOME)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_HOME); };
				pollKeyState[static_cast<uint8_t>(KeyCode::PAGE_UP)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_PAGE_UP); };
				pollKeyState[static_cast<uint8_t>(KeyCode::PAGE_DOWN)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_PAGE_DOWN); };
				pollKeyState[static_cast<uint8_t>(KeyCode::END)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_END); };
				pollKeyState[static_cast<uint8_t>(KeyCode::MENU)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_MENU); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_0)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_0); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_1)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_1); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_2)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_2); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_3)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_3); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_4)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_4); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_5)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_5); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_6)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_6); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_7)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_7); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_8)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_8); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_9)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_9); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_DECIMAL)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_DECIMAL); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_DIVIDE)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_DIVIDE); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_MULTIPLY)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_MULTIPLY); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_SUBTRACT)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_SUBTRACT); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_ADD)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_ADD); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_ENTER)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_ENTER); };
				pollKeyState[static_cast<uint8_t>(KeyCode::NUMPAD_EQUAL)] = [](GLFW_Window* window, KeyState& state) { getKeyboardKeyState(window, state, GLFW_KEY_KP_EQUAL); };

				return pollKeyState;
			}();

			for (uint8_t i = static_cast<uint8_t>(KeyCode::MOUSE_FIRST); i <= static_cast<uint8_t>(KeyCode::MOUSE_LAST); i++) POLL_KEY_STATE[i](window, m_keys[i]);
			for (uint8_t i = static_cast<uint8_t>(KeyCode::KEYBOARD_FIRST); i <= static_cast<uint8_t>(KeyCode::KEYBOARD_LAST); i++) POLL_KEY_STATE[i](window, m_keys[i]);

			{
				double posX, posY;
				glfwGetCursorPos(window->Handle(), &posX, &posY);
				m_axis[static_cast<uint8_t>(Axis::MOUSE_POSITION_X)].curValue = static_cast<float>(posX);
				m_axis[static_cast<uint8_t>(Axis::MOUSE_POSITION_Y)].curValue = static_cast<float>(posY);
			}

			auto pollControllerState = [&](KeyState* keyStates, AxisState* axisStates, int joystickId) {
				GLFWgamepadstate state;
				const uint8_t firstKey = static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST);
				const uint8_t firstAxis = static_cast<uint8_t>(Axis::CONTROLLER_FIRST);
				if (glfwJoystickPresent(joystickId) && glfwGetGamepadState(joystickId, &state)) {
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_MENU) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_START) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_DPAD_UP) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_DPAD_DOWN) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_DPAD_LEFT) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_DPAD_RIGHT) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_BUTTON_NORTH) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_BUTTON_SOUTH) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_BUTTON_WEST) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_BUTTON_EAST) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_LEFT_BUMPER) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_RIGHT_BUMPER) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_LEFT_ANALOG_BUTTON) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] == GLFW_PRESS);
					signalKey(keyStates[static_cast<uint8_t>(KeyCode::CONTROLLER_RIGHT_ANALOG_BUTTON) - firstKey], state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] == GLFW_PRESS);
					static auto deadzoned = [](float f) { return (abs(f) >= 0.2f) ? f : 0.0f; };
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_LEFT_ANALOG_X) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_LEFT_ANALOG_Y) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_RIGHT_ANALOG_X) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_RIGHT_ANALOG_Y) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_LEFT_TRIGGER) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
					axisStates[static_cast<uint8_t>(Axis::CONTROLLER_RIGHT_TRIGGER) - firstAxis].curValue = deadzoned(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);
				}
				else {
					for (uint8_t key = firstKey; key <= static_cast<uint8_t>(KeyCode::CONTROLLER_LAST); key++) signalKey(keyStates[key - firstKey], false);
					for (uint8_t axis = firstAxis; axis <= static_cast<uint8_t>(Axis::CONTROLLER_LAST); axis++) axisStates[axis - firstAxis].curValue = 0.0f;
				}
			};
			pollControllerState(m_keys + static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST), m_axis + static_cast<uint8_t>(Axis::CONTROLLER_FIRST), GLFW_JOYSTICK_1);
			for (uint8_t i = 0; i < GLFW_JOYSTICK_LAST; i++) pollControllerState(m_controllerKeys[i], m_controllerAxis[i], i + 1);
		}

		void GLFW_Input::OnScroll(float offset) {
			std::unique_lock<std::mutex> updateLock(m_updateLock);
			m_axis[static_cast<uint8_t>(Axis::MOUSE_SCROLL_WHEEL)].curValue += offset;
		}
	}
}
