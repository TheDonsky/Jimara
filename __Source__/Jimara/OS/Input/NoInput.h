#pragma once
#include "Input.h"
#include "../../Core/Helpers.h"


namespace Jimara {
	namespace OS {
		/// <summary>
		/// A basic class for mocking input;
		/// Does nothing, returns nothing, is worth nothing, is useless except the automated test cases that have no need of any input...
		/// </summary>
		class JIMARA_API NoInput final : public virtual Input {
		private:
			// Empty event...
			template<typename... Args>
			struct NoEvent : public virtual Event<Args...> {
				inline virtual void operator+=(Callback<Args...> callback) final override { Unused(callback); }
				inline virtual void operator-=(Callback<Args...> callback) final override { Unused(callback); }
				inline static NoEvent& Instance() { static NoEvent instance; return instance; }
			};
			mutable Input::CursorLock m_lockMode = Input::CursorLock::NONE;

		public:
			/// <summary>
			/// Well, MuteInput never does anything interesting, so ALWAYS false
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Always false </returns>
			inline virtual bool KeyDown(KeyCode code, uint8_t deviceId = 0)const final override { Unused(code, deviceId); return false; }

			/// <summary>
			/// Never invoked
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Event that never gets invoked </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyDown(KeyCode code, uint8_t deviceId = 0)const final override { 
				return NoEvent<KeyCode, uint8_t, const Input*>::Instance(); 
			}

			/// <summary>
			/// Well, MuteInput never does anything interesting, so ALWAYS false
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Always false </returns>
			inline virtual bool KeyPressed(KeyCode code, uint8_t deviceId = 0)const final override { Unused(code, deviceId); return false; }

			/// <summary>
			/// Never invoked
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Event that never gets invoked </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyPressed(KeyCode code, uint8_t deviceId = 0)const final override { 
				return NoEvent<KeyCode, uint8_t, const Input*>::Instance(); 
			}

			/// <summary>
			/// Well, MuteInput never does anything interesting, so ALWAYS false
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Always false </returns>
			inline virtual bool KeyUp(KeyCode code, uint8_t deviceId = 0)const final override { Unused(code, deviceId); return false; }

			/// <summary>
			/// Never invoked
			/// </summary>
			/// <param name="code"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Event that never gets invoked </returns>
			inline virtual Event<KeyCode, uint8_t, const Input*>& OnKeyUp(KeyCode code, uint8_t deviceId = 0)const final override { 
				return NoEvent<KeyCode, uint8_t, const Input*>::Instance(); 
			}

			/// <summary>
			/// Well.. Mute input is mute and this always reurns 0.0f
			/// </summary>
			/// <param name="axis"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> 0.0f </returns>
			inline virtual float GetAxis(Axis axis, uint8_t deviceId = 0)const final override { Unused(axis, deviceId); return 0.0f; }

			/// <summary>
			/// Never invoked
			/// </summary>
			/// <param name="axis"> Ignored </param>
			/// <param name="deviceId"> Ignored </param>
			/// <returns> Event that never gets invoked </returns>
			inline virtual Event<Axis, float, uint8_t, const Input*>& OnInputAxis(Axis axis, uint8_t deviceId = 0)const final override { 
				return NoEvent<Axis, float, uint8_t, const Input*>::Instance(); 
			}

			/// <summary> Cursor-Lock mode </summary>
			virtual CursorLock CursorLockMode()const final override { return m_lockMode; }

			/// <summary>
			/// Sets Cursor-Lock mode (ignored..)
			/// </summary>
			/// <param name="mode"> Mode to use </param>
			virtual void SetCursorLockMode(CursorLock mode)const { m_lockMode = mode; }

			/// <summary> Does nothing </summary>
			/// <param name="deltaTime"> Ignored </param>
			inline virtual void Update(float deltaTime) final override { Unused(deltaTime); }
		};
	}
}
