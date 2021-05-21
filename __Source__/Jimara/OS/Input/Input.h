#pragma once
#include "../../Core/Object.h"
#include "../../Core/Systems/Event.h"
#include "../../Math/Math.h"


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Interface for a generic user input from keyboard/mouse/controller
		/// </summary>
		class Input : public virtual Object {
		public:
			/// <summary> keyboard/mouse/controller button </summary>
			enum class KeyCode : uint8_t;

			/// <summary>
			/// True, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was down </returns>
			virtual bool KeyDown(KeyCode code, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key press event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyDown(KeyCode code, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// True, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was pressed </returns>
			virtual bool KeyPressed(KeyCode code, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key pressed event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyPressed(KeyCode code, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// True, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> True, if the key was released </returns>
			virtual bool KeyUp(KeyCode code, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers) </param>
			/// <returns> Key released event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, uint8_t, const Input*>& OnKeyUp(KeyCode code, uint8_t deviceId = 0)const = 0;



			/// <summary> Mouse movement, analog stick on controller or any other input that has a non-binary value </summary>
			enum class Axis : uint8_t;

			/// <summary>
			/// Gets current input from the axis
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Current input value </returns>
			virtual float GetAxis(Axis axis, uint8_t deviceId = 0)const = 0;

			/// <summary>
			/// Reports axis input whenevr it's active (ei, mainly, non-zero)
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers) </param>
			/// <returns> Axis value reporter (params: axis, value, deviceId, this) </returns>
			virtual Event<Axis, float, uint8_t, const Input*>& OnInputAxis(Axis axis, uint8_t deviceId = 0)const = 0;



			/// <summary>
			/// Updates Input
			/// Notes:
			///		0. Input class has to retain values from the getters in-between Update calls;
			///		1. Input events can only be invoked during Update() call on a single thread, to avoid thread safety considerations on the receiver's side;
			///		2. Implementation of Input does not have to be polling anything if it does not have to, 
			///		   but all the information accumulated in between Update()-s have to be made public only during the Updates.
			/// </summary>
			virtual void Update() = 0;
		};



		/// <summary> keyboard/mouse/controller button </summary>
		enum class Input::KeyCode : uint8_t {
			/// <summary> No key (OnKey events never invoked and Key calls always return false) </summary>
			NONE,


			/// <summary> First mouse button mapping </summary>
			MOUSE_FIRST,

			/// <summary> Left mouse button </summary>
			MOUSE_LEFT_BUTTON = MOUSE_FIRST,

			/// <summary> Middle mouse button </summary>
			MOUSE_MIDDLE_BUTTON,

			/// <summary> Right mouse button </summary>
			MOUSE_RIGHT_BUTTON,

			/// <summary> First mouse button mapping </summary>
			MOUSE_LAST = MOUSE_RIGHT_BUTTON,


			/// <summary> First keyboard mapping </summary>
			KEYBOARD_FIRST,

			/// <summary> Number 0 on alphanumeric keyboard </summary>
			ALPHA_0 = KEYBOARD_FIRST,

			/// <summary> Number 1 on alphanumeric keyboard </summary>
			ALPHA_1,

			/// <summary> Number 2 on alphanumeric keyboard </summary>
			ALPHA_2,

			/// <summary> Number 3 on alphanumeric keyboard </summary>
			ALPHA_3,

			/// <summary> Number 4 on alphanumeric keyboard </summary>
			ALPHA_4,

			/// <summary> Number 5 on alphanumeric keyboard </summary>
			ALPHA_5,

			/// <summary> Number 6 on alphanumeric keyboard </summary>
			ALPHA_6,

			/// <summary> Number 7 on alphanumeric keyboard </summary>
			ALPHA_7,

			/// <summary> Number 8 on alphanumeric keyboard </summary>
			ALPHA_8,

			/// <summary> Number 9 on alphanumeric keyboard </summary>
			ALPHA_9,


			/// <summary> Letter A </summary>
			A,

			/// <summary> Letter B </summary>
			B,

			/// <summary> Letter C </summary>
			C,

			/// <summary> Letter D </summary>
			D,

			/// <summary> Letter E </summary>
			E,

			/// <summary> Letter F </summary>
			F,

			/// <summary> Letter G </summary>
			G,

			/// <summary> Letter H </summary>
			H,

			/// <summary> Letter I </summary>
			I,

			/// <summary> Letter J </summary>
			J,

			/// <summary> Letter K </summary>
			K,

			/// <summary> Letter L </summary>
			L,

			/// <summary> Letter M </summary>
			M,
			
			/// <summary> Letter N </summary>
			N,
				
			/// <summary> Letter O </summary>
			O,

			/// <summary> Letter P </summary>
			P,
			
			/// <summary> Letter Q </summary>
			Q,
			
			/// <summary> Letter R </summary>
			R,
			
			/// <summary> Letter S </summary>
			S,
			
			/// <summary> Letter T </summary>
			T,
			
			/// <summary> Letter U </summary>
			U,
			
			/// <summary> Letter V </summary>
			V,
			
			/// <summary> Letter W </summary>
			W,
			
			/// <summary> Letter X </summary>
			X,
			
			/// <summary> Letter Y </summary>
			Y,
			
			/// <summary> Letter Z </summary>
			Z,


			/// <summary> Space bar </summary>
			SPACE,

			/// <summary> Comma </summary>
			COMMA,
				
			/// <summary> Dot/Period, whatever you like to call it </summary>
			DOT,
				
			/// <summary> Forward slash ('/') </summary>
			SLASH,
				
			/// <summary> Backslash ('\\') </summary>
			BACKSLASH,
				
			/// <summary> Semicolon (you are using this one much more frequently than normal people) </summary>
			SEMICOLON,
				
			/// <summary> Apostrophe </summary>
			APOSTROPHE,
				
			/// <summary> Left bracket ('[') </summary>
			LEFT_BRACKET,
				
			/// <summary> Right bracket (']') </summary>
			RIGHT_BRACKET,
				
			/// <summary> Minus on alphanumeric keyboard </summary>
			MINUS,
				
			/// <summary> Equals on alphanumeric keyboard </summary>
			EQUALS,

			/// <summary> back quote/grave accent, whatever the hell '`' character means... </summary>
			GRAVE_ACCENT,


			/// <summary> Escape key </summary>
			ESCAPE,
				
			/// <summary> Enter/Return key </summary>
			ENTER,
				
			/// <summary> Backspace </summary>
			BACKSPACE,
			
			/// <summary> Delete key </summary>
			DELETE_KEY,
				
			/// <summary> Tab </summary>
			TAB,
				
			/// <summary> One key that SHOULD NOT EXIST </summary>
			CAPS_LOCK,
			
			/// <summary> Left Shift </summary>
			LEFT_SHIFT,
				
			/// <summary> Right Shift </summary>
			RIGHT_SHIFT,
			
			/// <summary> Left control </summary>
			LEFT_CONTROL,
				
			/// <summary> Right control </summary>
			RIGHT_CONTROL,
			
			/// <summary> Left alt </summary>
			LEFT_ALT,
				
			/// <summary> Right alt </summary>
			RIGHT_ALT,
				
				
			/// <summary> Up arrow </summary>
			UP_ARROW,

			/// <summary> Down arrow </summary>
			DOWN_ARROW,
				
			/// <summary> Left arrow </summary>
			LEFT_ARROW,
				
			/// <summary> Right arrow </summary>
			RIGHT_ARROW,

				
			/// <summary> Function key #1 </summary>
			F1,
				
			/// <summary> Function key #2 </summary>
			F2,
				
			/// <summary> Function key #3 </summary>
			F3,
				
			/// <summary> Function key #4 </summary>
			F4,
				
			/// <summary> Function key #5 </summary>
			F5,
				
			/// <summary> Function key #6 </summary>
			F6,
				
			/// <summary> Function key #7 </summary>
			F7,
				
			/// <summary> Function key #8 </summary>
			F8,
				
			/// <summary> Function key #9 </summary>
			F9,
				
			/// <summary> Function key #10 </summary>
			F10,
				
			/// <summary> Function key #11 </summary>
			F11,
				
			/// <summary> Function key #12 </summary>
			F12,
				
			/// <summary> Function key #13 </summary>
			F13,
				
			/// <summary> Function key #14 </summary>
			F14,
				
			/// <summary> Function key #15 </summary>
			F15,


			/// <summary> Print screen </summary>
			PRINT_SCREEN,

			/// <summary> Scroll lock </summary>
			SCROLL_LOCK,
				
			/// <summary> Pause/Break button (it's on keyboard; not that hard to find if you know where to look...) </summary>
			PAUSE_BREAK,
				
			/// <summary> Num lock </summary>
			NUM_LOCK,

				
			/// <summary> Insert button </summary>
			INSERT,
				
			/// <summary> Home button </summary>
			HOME,
				
			/// <summary> Page up </summary>
			PAGE_UP,
				
			/// <summary> Page down </summary>
			PAGE_DOWN,
				
			/// <summary> End button </summary>
			END,
				
			/// <summary> Menu button </summary>
			MENU,


			/// <summary> Number 0 on numpad </summary>
			NUMPAD_0,

			/// <summary> Number 1 on numpad </summary>
			NUMPAD_1,

			/// <summary> Number 2 on numpad </summary>
			NUMPAD_2,

			/// <summary> Number 3 on numpad </summary>
			NUMPAD_3,

			/// <summary> Number 4 on numpad </summary>
			NUMPAD_4,

			/// <summary> Number 5 on numpad </summary>
			NUMPAD_5,

			/// <summary> Number 6 on numpad </summary>
			NUMPAD_6,

			/// <summary> Number 7 on numpad </summary>
			NUMPAD_7,

			/// <summary> Number 8 on numpad </summary>
			NUMPAD_8,

			/// <summary> Number 9 on numpad </summary>
			NUMPAD_9,


			/// <summary> Dot/delete on numpad </summary>
			NUMPAD_DECIMAL,
				
			/// <summary> Division/slash on numpad </summary>
			NUMPAD_DIVIDE,
				
			/// <summary> Multiply/* on numpad </summary>
			NUMPAD_MULTIPLY,
				
			/// <summary> Subtraction/minus on numpad </summary>
			NUMPAD_SUBTRACT,
				
			/// <summary> Addition/plus on numpad </summary>
			NUMPAD_ADD,
				
			/// <summary> Enter/Return on numpad </summary>
			NUMPAD_ENTER,
				
			/// <summary> Equals on numpad </summary>
			NUMPAD_EQUAL,
			
			/// <summary> Last keyboard mapping </summary>
			KEYBOARD_LAST = NUMPAD_EQUAL,


			/// <summary> First controller mapping </summary>
			CONTROLLER_FIRST,
			
			/// <summary> Controller menu button </summary>
			CONTROLLER_MENU = CONTROLLER_FIRST,
				
			/// <summary> Controller start button </summary>
			CONTROLLER_START,
				
			/// <summary> Controller DPad up </summary>
			CONTROLLER_DPAD_UP,
				
			/// <summary> Controller DPad down </summary>
			CONTROLLER_DPAD_DOWN,
				
			/// <summary> Controller DPad left </summary>
			CONTROLLER_DPAD_LEFT,
				
			/// <summary> Controller DPad right </summary>
			CONTROLLER_DPAD_RIGHT,
				
			/// <summary> Controller upper button (Y for Xbox, Triangle for PS) </summary>
			CONTROLLER_BUTTON_NORTH,
				
			/// <summary> Controller lower button (A for Xbox, Cross for PS) </summary>
			CONTROLLER_BUTTON_SOUTH,
				
			/// <summary> Controller left button (X for Xbox, Square for PS) </summary>
			CONTROLLER_BUTTON_WEST,
				
			/// <summary> Controller right button (B for Xbox, Circle for PS) </summary>
			CONTROLLER_BUTTON_EAST,
				
			/// <summary> Left shoulder button (LB for Xbox, L1 for PS) </summary>
			CONTROLLER_LEFT_BUMPER,
				
			/// <summary> Right shoulder button (RB for Xbox, R1 for PS) </summary>
			CONTROLLER_RIGHT_BUMPER,

			/// <summary> Press on left analog button on controller (L3 for PS, if you need formal names) </summary>
			CONTROLLER_LEFT_ANALOG_BUTTON,
				
			/// <summary> Press on right analog button on controller (R3 for PS, if you need formal names) </summary>
			CONTROLLER_RIGHT_ANALOG_BUTTON,

			/// <summary> Last controller mapping </summary>
			CONTROLLER_LAST = CONTROLLER_RIGHT_ANALOG_BUTTON,

				
			/// <summary> Number of abailable keycodes, not an actual key </summary>
			KEYCODE_COUNT
		};



		enum class Input::Axis : uint8_t {
			/// <summary> No axis (OnInputAxis never invoked and always 0 for GetAxis) </summary>
			NONE,


			/// <summary> "Normalized" mouse movement speed on X axis (Independent of screen/window size, simply something derived from hand movement speed and sensitivity) </summary>
			MOUSE_X,

			/// <summary> "Normalized" mouse movement speed on Y axis (Independent of screen/window size, simply something derived from hand movement speed and sensitivity) </summary>
			MOUSE_Y,

			/// <summary> Mouse cursor position on screen/window on X axis (in pixels) </summary>
			MOUSE_POSITION_X,

			/// <summary> Mouse cursor position on screen/window on Y axis (in pixels) </summary>
			MOUSE_POSITION_Y,

			/// <summary> Mouse cursor position difference between the last two Update cycles on screen/window on X axis (in pixels) </summary>
			MOUSE_DELTA_POSITION_X,

			/// <summary> Mouse cursor position difference between the last two Update cycles on screen/window on Y axis (in pixels) </summary>
			MOUSE_DELTA_POSITION_Y,

			/// <summary> Mouse scroll wheel input (positive "Up") </summary>
			MOUSE_SCROLL_WHEEL,


			/// <summary> First controller mapping </summary>
			CONTROLLER_FIRST,

			/// <summary> Left analog stick X axis on a controller </summary>
			CONTROLLER_LEFT_ANALOG_X = CONTROLLER_FIRST,

			/// <summary> Left analog stick Y axis on a controller </summary>
			CONTROLLER_LEFT_ANALOG_Y,

			/// <summary> Right analog stick X axis on a controller </summary>
			CONTROLLER_RIGHT_ANALOG_X,

			/// <summary> Right analog stick Y axis on a controller </summary>
			CONTROLLER_RIGHT_ANALOG_Y,

			/// <summary> Left trigger on a controller (LT for Xbox, L2 for PS) </summary>
			CONTROLLER_LEFT_TRIGGER,

			/// <summary> Right trigger on a controller (RT for Xbox, R2 for PS) </summary>
			CONTROLLER_RIGHT_TRIGGER,

			/// <summary> Last controller mapping </summary>
			CONTROLLER_LAST = CONTROLLER_RIGHT_TRIGGER,


			/// <summary> Number of available axis inputs </summary>
			AXIS_COUNT
		};
	}
}
