#pragma once
#include "OpenALInstance.h"
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALContext : public virtual Object {
			public:
				OpenALContext(ALCdevice* device, OpenALInstance* instance);

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
				ALCcontext* m_context = nullptr;
			};
		}
	}
}
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALListener : public virtual OpenALContext {
			public:

			private:
				const Reference<OpenALDevice> m_device;
			};
		}
	}
}

