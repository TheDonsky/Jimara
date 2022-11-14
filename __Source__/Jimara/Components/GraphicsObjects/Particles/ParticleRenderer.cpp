#include "ParticleRenderer.h"
#include "../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	struct ParticleRenderer::Helpers {
		// __TODO__: Implement this crap!
	};

	ParticleRenderer::ParticleRenderer(Component* parent, const std::string_view& name, size_t particleBudget)
		: Component(parent, name) {
		// __TODO__: Implement this crap!
		SetParticleBudget(particleBudget);
	}

	ParticleRenderer::~ParticleRenderer() {
		SetParticleBudget(0u);
		// __TODO__: Implement this crap!
	}

	size_t ParticleRenderer::ParticleBudget()const { return (m_buffers == nullptr) ? 0u : m_buffers->ParticleBudget(); }

	void ParticleRenderer::SetParticleBudget(size_t budget) {
		if (budget == ParticleBudget()) return;
		{
			m_buffers = nullptr;
			// __TODO__: Destroy all underlying tasks (maybe keep the old particles somehow)!
		}
		if (budget > 0u) {
			m_buffers = new ParticleBuffers(Context(), budget);
			// __TODO__: Create tasks!
		}
	}

	void ParticleRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		TriMeshRenderer::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(ParticleBudget, SetParticleBudget, "Particle Budget", "Maximal number of particles within the system");

			// __TODO__: Implement this crap!
		};
	}

	void ParticleRenderer::OnTriMeshRendererDirty() {

	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ParticleRenderer> serializer("Jimara/Graphics/Particles/ParticleRenderer", "Particle Renderer");
		report(&serializer);
	}
}
