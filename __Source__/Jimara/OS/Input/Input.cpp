#include "Input.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	namespace OS {
		const Object* Input::KeyCodeEnumAttribute() {
			typedef const Serialization::EnumAttribute<std::underlying_type_t<KeyCode>> AttributeType;
			static const Reference<AttributeType> attribute = Object::Instantiate<AttributeType>(false,
				"NONE", KeyCode::NONE,
				"MOUSE_LEFT_BUTTON", KeyCode::MOUSE_LEFT_BUTTON,
				"MOUSE_MIDDLE_BUTTON", KeyCode::MOUSE_MIDDLE_BUTTON,
				"MOUSE_RIGHT_BUTTON", KeyCode::MOUSE_RIGHT_BUTTON,
				"ALPHA_0", KeyCode::ALPHA_0,
				"ALPHA_1", KeyCode::ALPHA_1,
				"ALPHA_2", KeyCode::ALPHA_2,
				"ALPHA_3", KeyCode::ALPHA_3,
				"ALPHA_4", KeyCode::ALPHA_4,
				"ALPHA_5", KeyCode::ALPHA_5,
				"ALPHA_6", KeyCode::ALPHA_6,
				"ALPHA_7", KeyCode::ALPHA_7,
				"ALPHA_8", KeyCode::ALPHA_8,
				"ALPHA_9", KeyCode::ALPHA_9,
				"A", KeyCode::A,
				"B", KeyCode::B,
				"C", KeyCode::C,
				"D", KeyCode::D,
				"E", KeyCode::E,
				"F", KeyCode::F,
				"G", KeyCode::G,
				"H", KeyCode::H,
				"I", KeyCode::I,
				"J", KeyCode::J,
				"K", KeyCode::K,
				"L", KeyCode::L,
				"M", KeyCode::M,
				"N", KeyCode::N,
				"O", KeyCode::O,
				"P", KeyCode::P,
				"Q", KeyCode::Q,
				"R", KeyCode::R,
				"S", KeyCode::S,
				"T", KeyCode::T,
				"U", KeyCode::U,
				"V", KeyCode::V,
				"W", KeyCode::W,
				"X", KeyCode::X,
				"Y", KeyCode::Y,
				"Z", KeyCode::Z,
				"SPACE", KeyCode::SPACE,
				"COMMA", KeyCode::COMMA,
				"DOT", KeyCode::DOT,
				"SLASH", KeyCode::SLASH,
				"BACKSLASH", KeyCode::BACKSLASH,
				"SEMICOLON", KeyCode::SEMICOLON,
				"APOSTROPHE", KeyCode::APOSTROPHE,
				"LEFT_BRACKET", KeyCode::LEFT_BRACKET,
				"RIGHT_BRACKET", KeyCode::RIGHT_BRACKET,
				"MINUS", KeyCode::MINUS,
				"EQUALS", KeyCode::EQUALS,
				"GRAVE_ACCENT", KeyCode::GRAVE_ACCENT,
				"ESCAPE", KeyCode::ESCAPE,
				"ENTER", KeyCode::ENTER,
				"BACKSPACE", KeyCode::BACKSPACE,
				"DELETE_KEY", KeyCode::DELETE_KEY,
				"TAB", KeyCode::TAB,
				"CAPS_LOCK", KeyCode::CAPS_LOCK,
				"LEFT_SHIFT", KeyCode::LEFT_SHIFT,
				"RIGHT_SHIFT", KeyCode::RIGHT_SHIFT,
				"LEFT_CONTROL", KeyCode::LEFT_CONTROL,
				"RIGHT_CONTROL", KeyCode::RIGHT_CONTROL,
				"LEFT_ALT", KeyCode::LEFT_ALT,
				"RIGHT_ALT", KeyCode::RIGHT_ALT,
				"UP_ARROW", KeyCode::UP_ARROW,
				"DOWN_ARROW", KeyCode::DOWN_ARROW,
				"LEFT_ARROW", KeyCode::LEFT_ARROW,
				"RIGHT_ARROW", KeyCode::RIGHT_ARROW,
				"F1", KeyCode::F1,
				"F2", KeyCode::F2,
				"F3", KeyCode::F3,
				"F4", KeyCode::F4,
				"F5", KeyCode::F5,
				"F6", KeyCode::F6,
				"F7", KeyCode::F7,
				"F8", KeyCode::F8,
				"F9", KeyCode::F9,
				"F10", KeyCode::F10,
				"F11", KeyCode::F11,
				"F12", KeyCode::F12,
				"F13", KeyCode::F13,
				"F14", KeyCode::F14,
				"F15", KeyCode::F15,
				"PRINT_SCREEN", KeyCode::PRINT_SCREEN,
				"SCROLL_LOCK", KeyCode::SCROLL_LOCK,
				"PAUSE_BREAK", KeyCode::PAUSE_BREAK,
				"NUM_LOCK", KeyCode::NUM_LOCK,
				"INSERT", KeyCode::INSERT,
				"HOME", KeyCode::HOME,
				"PAGE_UP", KeyCode::PAGE_UP,
				"PAGE_DOWN", KeyCode::PAGE_DOWN,
				"END", KeyCode::END,
				"MENU", KeyCode::MENU,
				"NUMPAD_0", KeyCode::NUMPAD_0,
				"NUMPAD_1", KeyCode::NUMPAD_1,
				"NUMPAD_2", KeyCode::NUMPAD_2,
				"NUMPAD_3", KeyCode::NUMPAD_3,
				"NUMPAD_4", KeyCode::NUMPAD_4,
				"NUMPAD_5", KeyCode::NUMPAD_5,
				"NUMPAD_6", KeyCode::NUMPAD_6,
				"NUMPAD_7", KeyCode::NUMPAD_7,
				"NUMPAD_8", KeyCode::NUMPAD_8,
				"NUMPAD_9", KeyCode::NUMPAD_9,
				"NUMPAD_DECIMAL", KeyCode::NUMPAD_DECIMAL,
				"NUMPAD_DIVIDE", KeyCode::NUMPAD_DIVIDE,
				"NUMPAD_MULTIPLY", KeyCode::NUMPAD_MULTIPLY,
				"NUMPAD_SUBTRACT", KeyCode::NUMPAD_SUBTRACT,
				"NUMPAD_ADD", KeyCode::NUMPAD_ADD,
				"NUMPAD_ENTER", KeyCode::NUMPAD_ENTER,
				"NUMPAD_EQUAL", KeyCode::NUMPAD_EQUAL,
				"CONTROLLER_MENU", KeyCode::CONTROLLER_MENU,
				"CONTROLLER_START", KeyCode::CONTROLLER_START,
				"CONTROLLER_DPAD_UP", KeyCode::CONTROLLER_DPAD_UP,
				"CONTROLLER_DPAD_DOWN", KeyCode::CONTROLLER_DPAD_DOWN,
				"CONTROLLER_DPAD_LEFT", KeyCode::CONTROLLER_DPAD_LEFT,
				"CONTROLLER_DPAD_RIGHT", KeyCode::CONTROLLER_DPAD_RIGHT,
				"CONTROLLER_BUTTON_NORTH", KeyCode::CONTROLLER_BUTTON_NORTH,
				"CONTROLLER_BUTTON_SOUTH", KeyCode::CONTROLLER_BUTTON_SOUTH,
				"CONTROLLER_BUTTON_WEST", KeyCode::CONTROLLER_BUTTON_WEST,
				"CONTROLLER_BUTTON_EAST", KeyCode::CONTROLLER_BUTTON_EAST,
				"CONTROLLER_LEFT_BUMPER", KeyCode::CONTROLLER_LEFT_BUMPER,
				"CONTROLLER_RIGHT_BUMPER", KeyCode::CONTROLLER_RIGHT_BUMPER,
				"CONTROLLER_LEFT_ANALOG_BUTTON", KeyCode::CONTROLLER_LEFT_ANALOG_BUTTON,
				"CONTROLLER_RIGHT_ANALOG_BUTTON", KeyCode::CONTROLLER_RIGHT_ANALOG_BUTTON);
			return attribute;
		}

		const Object* Input::AxisEnumAttribute() {
			typedef const Serialization::EnumAttribute<std::underlying_type_t<Axis>> AttributeType;
			static const Reference<AttributeType> attribute = Object::Instantiate<AttributeType>(false, 
				"NONE", Axis::NONE,
				"MOUSE_X", Axis::MOUSE_X,
				"MOUSE_Y", Axis::MOUSE_Y,
				"MOUSE_POSITION_X", Axis::MOUSE_POSITION_X,
				"MOUSE_POSITION_Y", Axis::MOUSE_POSITION_Y,
				"MOUSE_DELTA_POSITION_X", Axis::MOUSE_DELTA_POSITION_X,
				"MOUSE_DELTA_POSITION_Y", Axis::MOUSE_DELTA_POSITION_Y,
				"MOUSE_SCROLL_WHEEL", Axis::MOUSE_SCROLL_WHEEL,
				"CONTROLLER_LEFT_ANALOG_X", Axis::CONTROLLER_LEFT_ANALOG_X,
				"CONTROLLER_LEFT_ANALOG_Y", Axis::CONTROLLER_LEFT_ANALOG_Y,
				"CONTROLLER_RIGHT_ANALOG_X", Axis::CONTROLLER_RIGHT_ANALOG_X,
				"CONTROLLER_RIGHT_ANALOG_Y", Axis::CONTROLLER_RIGHT_ANALOG_Y,
				"CONTROLLER_LEFT_TRIGGER", Axis::CONTROLLER_LEFT_TRIGGER,
				"CONTROLLER_RIGHT_TRIGGER", Axis::CONTROLLER_RIGHT_TRIGGER);
			return attribute;
		}
	}
}