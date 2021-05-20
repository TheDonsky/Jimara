#pragma once
#include "../../Core/Object.h"
#include "../../Core/Systems/Event.h"
#include "../../Math/Math.h"


namespace Jimara {
	namespace OS {
		class Input : public virtual Object {
		public:
			enum class KeyCode;

			virtual bool KeyDown(KeyCode code)const = 0;

			virtual Event<KeyCode, const Input*> OnKeyDown(KeyCode code)const = 0;

			virtual bool KeyPressed(KeyCode code)const = 0;

			virtual Event<KeyCode, const Input*> OnKeyPressed(KeyCode code)const = 0;

			virtual bool KeyUp(KeyCode code)const = 0;

			virtual Event<KeyCode, const Input*> OnKeyUp(KeyCode code)const = 0;



			enum class Axis;

			virtual float GetAxis(Axis axis)const = 0;

			virtual Event<Axis, const Input*> OnAxisChanged()const = 0;



			virtual void Update() = 0;
		};

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
