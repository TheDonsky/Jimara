#include "OpenALListener.h"



namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALListener::OpenALListener(const Settings& settings, OpenALScene* scene)
				: m_context(Object::Instantiate<ListenerContext>(dynamic_cast<OpenALDevice*>(scene->Device())))
				, m_scene(scene) {
				Update(settings);
			}
			
			OpenALListener::~OpenALListener() {
				if (m_volume > 0.0f) m_scene->RemoveListenerContext(m_context);
			}

			void OpenALListener::Update(const Settings& newSettings) {
				const Vector3 position = newSettings.pose[3];
				const Vector3 forward = Math::Normalize((Vector3)newSettings.pose[2]);
				const Vector3 up = Math::Normalize((Vector3)newSettings.pose[1]);
				const Vector3& velocity = newSettings.velocity;

				std::unique_lock<std::mutex> updateLock(m_updateLock);
				{
					const ALfloat orientationData[] = {
						static_cast<ALfloat>(forward.x), static_cast<ALfloat>(forward.y), static_cast<ALfloat>(-forward.z),
						static_cast<ALfloat>(up.x), static_cast<ALfloat>(up.y), static_cast<ALfloat>(-up.z)
					};
					OpenALDevice* const device = dynamic_cast<OpenALDevice*>(m_scene->Device());

					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					ListenerContext::SwapCurrent swap(m_context);

					alListenerf(AL_GAIN, static_cast<ALfloat>(max(newSettings.volume, 0.0f)));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_GAIN, volume) failed!");

					alListener3f(AL_POSITION, static_cast<ALfloat>(position.x), static_cast<ALfloat>(position.y), static_cast<ALfloat>(-position.z));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_POSITION, position) failed!");

					alListener3f(AL_VELOCITY, static_cast<ALfloat>(velocity.x), static_cast<ALfloat>(velocity.y), static_cast<ALfloat>(-velocity.z));
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_VELOCITY, velocity) failed!");

					alListenerfv(AL_ORIENTATION, orientationData);
					device->ALInstance()->ReportALError("OpenALListener::Update - alListenerf(AL_ORIENTATION, forward, up) failed!");
				}

				bool active = (newSettings.volume > 0.0f);
				if ((m_volume > 0.0f) != active) {
					if (active) m_scene->AddListenerContext(m_context);
					else m_scene->RemoveListenerContext(m_context);
				}
				m_volume = newSettings.volume;
			}


			ListenerContext* OpenALListener::Context()const { return m_context; }
		}
	}
}


