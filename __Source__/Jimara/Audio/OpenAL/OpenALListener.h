#pragma once
#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL-backed AudioListener
			/// </summary>
			class OpenALListener : public virtual AudioListener {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="settings"> Initial listener settings </param>
				/// <param name="scene"> OpenALScene, the listener resides on </param>
				OpenALListener(const Settings& settings, OpenALScene* scene);

				/// <summary> Virtual destructor (removes the listener from the scene) </summary>
				virtual ~OpenALListener();

				/// <summary>
				/// Updates listener settings
				/// </summary>
				/// <param name="newSettings"> New settings to set </param>
				virtual void Update(const Settings& newSettings) override;

				/// <summary> OpenAL context, tied to the listener </summary>
				ListenerContext* Context()const;

			private:
				// OpenAL context, tied to the listener
				const Reference<ListenerContext> m_context;

				// OpenALScene, the listener resides on
				const Reference<OpenALScene> m_scene;

				// Lock used during Update() call to keep everything in synch
				std::mutex m_updateLock;

				// Current volume (if 0 or negative, the listener will be excluded from the scene for optimisation)
				std::atomic<float> m_volume = -1.0f;
			};
		}
	}
}

