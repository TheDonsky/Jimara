#pragma once
#include "GLFW_Window.h"


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Input from a GLFW window
		/// </summary>
		class JIMARA_API GLFW_Input : public virtual Input {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="window"> Window to get input from </param>
			GLFW_Input(GLFW_Window* window);

			/// <summary> Virtual destructor </summary>
			virtual ~GLFW_Input();


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



			/// <summary> Updates Input </summary>
			/// <param name="deltaTime"> Time since last update </param>
			virtual void Update(float deltaTime) override;



		private:
			// Window to get input from
			const Reference<GLFW_Window> m_window;

			// Callback manager object
			const Reference<Object> m_callbacks;

			// Monitor size for resolution-independent MOUSE_X/MOUSE_Y axis inputs
			const float m_monitorSize;

			// Lock for updates
			std::mutex m_updateLock;

			// KeyCode state
			struct KeyState {
				struct {
					bool released = false;
					bool pressed = false;
					bool currentlyPressed = false;
				} signal;

				bool gotPressed = false;
				bool wasPressed = false;
				bool gotReleased = false;
				mutable EventInstance<KeyCode, uint8_t, const Input*> onDown;
				mutable EventInstance<KeyCode, uint8_t, const Input*> onPressed;
				mutable EventInstance<KeyCode, uint8_t, const Input*> onUp;
			} m_keys[static_cast<uint8_t>(KeyCode::KEYCODE_COUNT)];

			// Key states for all the controllers with non-zero index
			KeyState m_controllerKeys[GLFW_JOYSTICK_LAST][1 + static_cast<uint8_t>(KeyCode::CONTROLLER_LAST) - static_cast<uint8_t>(KeyCode::CONTROLLER_FIRST)];

			// Axis state
			struct AxisState {
				float curValue = 0.0f;
				bool changed = false;
				float lastValue = 0.0f;
				mutable EventInstance<Axis, float, uint8_t, const Input*> onAxis;
			} m_axis[static_cast<uint8_t>(Axis::AXIS_COUNT)];

			// Axis states for all the controllers with non-zero index
			AxisState m_controllerAxis[GLFW_JOYSTICK_LAST][1 + static_cast<uint8_t>(Axis::CONTROLLER_LAST) - static_cast<uint8_t>(Axis::CONTROLLER_FIRST)];

			// Gets key state by code and deviceId
			const KeyState& GetKeyState(KeyCode code, uint8_t deviceId)const;

			// Gets axis state by code and device id
			const AxisState& GetAxisState(Axis axis, uint8_t deviceId)const;

			// Polls new updates
			void Poll(GLFW_Window* window);

			// Invoked when scrolled
			void OnScroll(float offset);
		};
	}
}
