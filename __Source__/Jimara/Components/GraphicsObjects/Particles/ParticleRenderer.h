#pragma once
#include "../TriMeshRenderer.h"
#include "../../../Environment/Rendering/Particles/ParticleSimulation.h"


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::ParticleRenderer);

	class JIMARA_API ParticleRenderer : public virtual TriMeshRenderer {
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
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() final override;

	private:
		Reference<ParticleBuffers> m_buffers;
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer;
		Reference<ParticleKernel::Task> m_simulationStep;
		ParticleSimulation::TaskBinding m_particleSimulationTask;

		Reference<Object> m_pipelineDescriptor;

		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<ParticleRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<TriMeshRenderer>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report);
}
