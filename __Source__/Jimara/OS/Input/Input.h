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
			enum class KeyCode;

			/// <summary>
			/// True, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> True, if the key was down </returns>
			virtual bool KeyDown(KeyCode code, int deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key got pressed during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> Key press event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, int, const Input*> OnKeyDown(KeyCode code, int deviceId = 0)const = 0;

			/// <summary>
			/// True, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> True, if the key was pressed </returns>
			virtual bool KeyPressed(KeyCode code, int deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key was pressed at any time throught the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that was pressed </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> Key pressed event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, int, const Input*> OnKeyPressed(KeyCode code, int deviceId = 0)const = 0;

			/// <summary>
			/// True, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> True, if the key was released </returns>
			virtual bool KeyUp(KeyCode code, int deviceId = 0)const = 0;

			/// <summary>
			/// Invoked, if the key got released during the last Update cycle
			/// </summary>
			/// <param name="code"> Identifier of the key that got released </param>
			/// <param name="deviceId"> Index of the device the key was located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> Key released event (params: code, deviceId, this) </returns>
			virtual Event<KeyCode, int, const Input*> OnKeyUp(KeyCode code, int deviceId = 0)const = 0;



			/// <summary> Mouse movement, analog stick on controller or any other input that has a non-binary value </summary>
			enum class Axis;

			/// <summary>
			/// Gets current input from the axis
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> Current input value </returns>
			virtual float GetAxis(Axis axis, int deviceId = 0)const = 0;

			/// <summary>
			/// Reports axis input whenevr it's active (ei, mainly, non-zero)
			/// </summary>
			/// <param name="axis"> Axis to get value of </param>
			/// <param name="deviceId"> Index of the device the axis is located on (mainly used for controllers; negative values mean "any device") </param>
			/// <returns> Axis value reporter (params: axis, deviceId, this) </returns>
			virtual Event<Axis, int, const Input*> OnInputAxis(Axis axis, int deviceId = 0)const = 0;



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
			NONE = 0,
			A = 1,
			B = 2
			// __TODO__: Cover all keycodes supported (Feel free to shamelessly copy Unity)
		};



		enum class Input::Axis : uint8_t {
			NONE = 0,
			MOUSE_X = 1,
			MOUSE_Y = 2,
			// __TODO__: Cover all supported axis inputs (Feel free to shamelessly copy Unity)
		};
	}
}
