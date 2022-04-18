#pragma once
#include <OS/Input/Input.h>
#include <Math/BitMask.h>
#include <Core/Collections/Stacktor.h>
#include <vector>


namespace Jimara {
	namespace Editor {
		// __TODO__: Actually create a better way to define a hotkey.... This stuff is temporary

		class HotKey {
		public:
			virtual bool Check(const OS::Input* input)const = 0;

			inline static const HotKey& Save();

			inline static const HotKey& Undo();

		private:
			class SaveHotKey;
			class UndoHotKey;
		};


		class HotKey::SaveHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return (input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| input->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
					&& input->KeyDown(OS::Input::KeyCode::S);
			}
		};

		class HotKey::UndoHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return (input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| input->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
					&& input->KeyDown(OS::Input::KeyCode::Z);
			}
		};

		inline const HotKey& HotKey::Save() {
			static const SaveHotKey hotKey;
			return hotKey;
		}

		inline const HotKey& HotKey::Undo() {
			static const UndoHotKey hotKey;
			return hotKey;
		}
	}
}