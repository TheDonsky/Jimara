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
			/// <summary>
			/// Wrapper on top of OpenAL context
			/// </summary>
			class OpenALContext : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> OpenAL device (API Object, not in-engine reference) </param>
				/// <param name="instance"> API Instance </param>
				/// <param name="deviceHolder"> Optional Object to hold on to while the context is alive </param>
				OpenALContext(ALCdevice* device, OpenALInstance* instance, const Object* deviceHolder);

				/// <summary> Virtual destructor </summary>
				virtual ~OpenALContext();

				/// <summary> Underlying API object </summary>
				operator ALCcontext* ()const;

				/// <summary>
				/// Swaps out ALCcontext while in scope
				/// Note: This does not keep references alive and does not lock, so it's the user's responsibility to use APILock() and keep OpenALContext alive
				/// </summary>
				class SwapCurrent {
				public:
					/// <summary>
					/// Constructor
					/// </summary>
					/// <param name="context"> Context to use </param>
					SwapCurrent(const OpenALContext* context);

					/// <summary> Destructor </summary>
					~SwapCurrent();

				private:
					// Context, used while the SwapCurrent is active
					const OpenALContext* const m_context;

					// Context, that was being used before SwapCurrent got created
					ALCcontext* const m_old = nullptr;
				};

			private:
				// API Instance
				const Reference<OpenALInstance> m_instance;

				// Optional Object to hold on to while the context is alive
				const Reference<const Object> m_deviceHolder;

				// Underlying API object
				ALCcontext* m_context = nullptr;
			};

			/// <summary>
			/// Context, used by the listeners (holds reference to logical device and can allocate and recycle audio sources)
			/// </summary>
			class ListenerContext : public virtual OpenALContext {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Logical device, this context resides on </param>
				ListenerContext(OpenALDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~ListenerContext();

				/// <summary> Aquires OpenAL source </summary>
				ALuint GetSource();

				/// <summary>
				/// Releases OpenAL source
				/// </summary>
				/// <param name="source"> Source to release </param>
				void FreeSource(ALuint source);

				/// <summary> Logical device, this context resides on </summary>
				OpenALDevice* Device()const;

			private:
				// Logical device, this context resides on
				const Reference<OpenALDevice> m_device;

				// Lock for GetSource/FreeSource
				std::mutex m_freeLock;

				// All sources, that have ever been allocated
				std::vector<ALuint> m_sources;

				// Sources, that have been allocated, but are not currently being used
				std::stack<ALuint> m_freeSources;
			};
		}
	}
}
