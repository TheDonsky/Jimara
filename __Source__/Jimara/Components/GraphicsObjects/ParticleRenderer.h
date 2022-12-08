#pragma once
#include "TriMeshRenderer.h"
#include "../../Environment/GraphicsSimulation/GraphicsSimulation.h"
#include "../../Environment/Rendering/Particles/ParticleBuffers.h"


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::ParticleRenderer);

	class JIMARA_API ParticleRenderer : public virtual TriMeshRenderer {
	public:
		ParticleRenderer(Component* parent, const std::string_view& name = "ParticleRenderer", size_t particleBudget = 1000u);

		virtual ~ParticleRenderer();

		size_t ParticleBudget()const;

		void SetParticleBudget(size_t budget);

		inline ParticleBuffers* Buffers()const { return m_buffers; }

		inline float EmissionRate()const { return m_emissionRate; }

		inline void SetEmissionRate(float emissionRate) { m_emissionRate = Math::Max(emissionRate, 0.0f); }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() final override;

	private:
		Reference<ParticleBuffers> m_buffers;
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer;
		Reference<GraphicsSimulation::Task> m_simulationStep;
		GraphicsSimulation::TaskBinding m_particleSimulationTask;
		float m_emissionRate = 10.0f;
		float m_timeSinceLastEmission = 0.0f;

		Reference<Object> m_pipelineDescriptor;

		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<ParticleRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<TriMeshRenderer>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report);
}
