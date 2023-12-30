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

			inline static const HotKey& Copy();

			inline static const HotKey& Cut();

			inline static const HotKey& Paste();

			inline static const HotKey& Delete();

		private:
			class SaveHotKey;
			class UndoHotKey;

			class CopyHotKey;
			class CutHotKey;
			class PasteHotKey;

			class DeleteHotKey;
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

		class HotKey::CopyHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return (input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| input->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
					&& input->KeyDown(OS::Input::KeyCode::C);
			}
		};

		class HotKey::CutHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return (input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| input->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
					&& input->KeyDown(OS::Input::KeyCode::X);
			}
		};

		class HotKey::PasteHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return (input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| input->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
					&& input->KeyDown(OS::Input::KeyCode::V);
			}
		};

		class HotKey::DeleteHotKey : public virtual HotKey {
		public:
			inline virtual bool Check(const OS::Input* input)const final override {
				return input->KeyDown(OS::Input::KeyCode::DELETE_KEY);
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

		inline const HotKey& HotKey::Copy() {
			static CopyHotKey hotKey;
			return hotKey;
		}

		inline const HotKey& HotKey::Cut() {
			static CutHotKey hotKey;
			return hotKey;
		}

		inline const HotKey& HotKey::Paste() {
			static PasteHotKey hotKey;
			return hotKey;
		}

		inline const HotKey& HotKey::Delete() {
			static DeleteHotKey hotKey;
			return hotKey;
		}
	}
}
