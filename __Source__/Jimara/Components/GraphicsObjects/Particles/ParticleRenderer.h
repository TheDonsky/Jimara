#pragma once
#include "../../Transform.h"
#include "../../../Environment/Rendering/Particles/ParticleSimulation.h"


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::ParticleRenderer);

	class JIMARA_API ParticleRenderer : public virtual Component {
	public:
		ParticleRenderer(Component* parent, const std::string_view& name = "ParticleRenderer", size_t particleBudget = 1000u);

		virtual ~ParticleRenderer();

		size_t ParticleBudget()const;

		void SetParticleBudget(size_t budget);

		inline ParticleBuffers* Buffers()const { return m_buffers; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked by the scene on the first frame this component gets instantiated </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

	private:
		Reference<ParticleBuffers> m_buffers;

		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<ParticleRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report);
}
