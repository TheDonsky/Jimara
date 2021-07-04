#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALContext;
			class ListenerContext;
		}
	}
}
#include "OpenALDevice.h"
#include <stack>

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
					const OpenALContext* const m_context;
					ALCcontext* const m_old = nullptr;
				};

			private:
				const Reference<OpenALInstance> m_instance;
				const Reference<const Object> m_deviceHolder;
				ALCcontext* m_context = nullptr;
			};

			class ListenerContext : public virtual OpenALContext {
			public:
				ListenerContext(OpenALDevice* device);

				virtual ~ListenerContext();

				ALuint GetSource();

				void FreeSource(ALuint source);

				OpenALDevice* Device()const;

			private:
				const Reference<OpenALDevice> m_device;
				std::mutex m_freeLock;
				std::vector<ALuint> m_sources;
				std::stack<ALuint> m_freeSources;
			};
		}
	}
}
