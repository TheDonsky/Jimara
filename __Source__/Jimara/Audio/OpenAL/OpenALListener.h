#pragma once
#include "OpenALInstance.h"
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALContext : public virtual Object {
			public:
				OpenALContext(ALCdevice* device, OpenALInstance* instance, const Object* deviceHolder);

				virtual ~OpenALContext();

				operator ALCcontext* ()const;

				class SwapCurrent {
				public:
					SwapCurrent(const OpenALContext* context);

					~SwapCurrent();

				private:
					const Reference<const OpenALContext> m_context;
					ALCcontext* const m_old = nullptr;
				};

			private:
				const Reference<OpenALInstance> m_instance;
				const Reference<const Object> m_deviceHolder;
				ALCcontext* m_context = nullptr;
			};
		}
	}
}
#include "OpenALDevice.h"
#include "OpenALScene.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALListener : public virtual AudioListener, public virtual OpenALContext {
			public:
				OpenALListener(const Settings& settings, OpenALScene* scene);

				virtual void Update(const Settings& newSettings) override;

			private:
				const Reference<OpenALScene> m_scene;
			};
		}
	}
}

