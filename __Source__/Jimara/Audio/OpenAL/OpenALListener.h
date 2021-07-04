#pragma once
#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALListener : public virtual AudioListener {
			public:
				OpenALListener(const Settings& settings, OpenALScene* scene);

				virtual ~OpenALListener();

				virtual void Update(const Settings& newSettings) override;

				ListenerContext* Context()const;

			private:
				const Reference<ListenerContext> m_context;
				const Reference<OpenALScene> m_scene;
				std::mutex m_updateLock;
				std::atomic<float> m_volume = -1.0f;
			};
		}
	}
}

