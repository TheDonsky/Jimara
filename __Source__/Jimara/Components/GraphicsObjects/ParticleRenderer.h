#pragma once
#include "TriMeshRenderer.h"
#include "../../Environment/GraphicsSimulation/GraphicsSimulation.h"
#include "../../Environment/Rendering/Particles/ParticleBuffers.h"


namespace Jimara {
	/// <summary> Let the engine know this class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleRenderer);

	/// <summary>
	/// A renderer, responsible for simulating and rendering particle systems
	/// </summary>
	class JIMARA_API ParticleRenderer : public virtual TriMeshRenderer {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="particleBudget"> Initial limit on live particles </param>
		ParticleRenderer(Component* parent, const std::string_view& name = "ParticleRenderer", size_t particleBudget = 1000u);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleRenderer();

		/// <summary> Maximal number of particles that can simultanuously be alive at the same time </summary>
		size_t ParticleBudget()const;

		/// <summary>
		/// Updates particle budget
		/// </summary>
		/// <param name="budget"> Maximal number of particles that can simultanuously be alive at the same time </param>
		void SetParticleBudget(size_t budget);

		/// <summary> Particle buffers for this system (this will chane if and when ParticleBudget gets altered) </summary>
		inline ParticleBuffers* Buffers()const { return m_buffers; }

		/// <summary> Number of particles emitted per second </summary>
		inline float EmissionRate()const { return m_emissionRate; }

		/// <summary>
		/// Sets the number of particles emitted per second
		/// </summary>
		/// <param name="emissionRate"> Particles per second </param>
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
		// Info about this particle system
		const Reference<ParticleSystemInfo> m_systemInfo;

		// Particle buffers
		Reference<ParticleBuffers> m_buffers;

		// Simulation step reference
		Reference<GraphicsSimulation::Task> m_simulationStep;
		
		// When particle system is active, instance buffer generator is bound to this binding
		GraphicsSimulation::TaskBinding m_particleSimulationTask;

		// Number of particles emitted per second
		float m_emissionRate = 10.0f;

		// Last frame time snapshot
		float m_timeSinceLastEmission = 0.0f;

		// Graphics pipeline descriptor
		Reference<Object> m_pipelineDescriptor;

		// Private stuff resides here
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<ParticleRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<TriMeshRenderer>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report);
}
