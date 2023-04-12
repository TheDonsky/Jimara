#pragma once
#include "ParticleKernels.h"
#include "../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	/// <summary>
	/// We have CombinedGraphicsSimulationKernel for simplifying arbitrary graphics simulation task kernel unification.
	/// This goes a bit further and optionally provides a few extra automatic bindings like RNG and time that are commonly used by particle simulation steps.
	/// </summary>
	class JIMARA_API CombinedParticleKernel : public virtual GraphicsSimulation::Kernel {
	public:
		/// <summary>
		/// Creates a new CombinedParticleKernel for some shader class
		/// </summary>
		/// <typeparam name="SimulationTaskSettings"> 
		/// Type of the task settings for the underlying CombinedGraphicsSimulationKernel (refer to it's documentation for further details)
		/// </typeparam>
		/// <param name="shaderClass"> Shader class </param>
		/// <returns> New instance of a CombinedParticleKernel </returns>
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> Create(const Graphics::ShaderClass* shaderClass) {
			return Create(sizeof(SimulationTaskSettings), shaderClass,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		/// <summary>
		/// Returns a shared(cached) instance CombinedParticleKernel for some shader class
		/// </summary>
		/// <typeparam name="SimulationTaskSettings"> 
		/// Type of the task settings for the underlying CombinedGraphicsSimulationKernel (refer to it's documentation for further details)
		/// </typeparam>
		/// <param name="shaderClass"> Shader class </param>
		/// <returns> Shared instance of a CombinedParticleKernel </returns>
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> GetCached(const Graphics::ShaderClass* shaderClass) {
			return GetCached(sizeof(SimulationTaskSettings), shaderClass,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		/// <summary>
		/// Returns a shared(cached) instance CombinedParticleKernel for some shader class
		/// </summary>
		/// <typeparam name="SimulationTaskSettings"> 
		/// Type of the task settings for the underlying CombinedGraphicsSimulationKernel (refer to it's documentation for further details)
		/// </typeparam>
		/// <param name="shaderPath"> path to the compute shader </param>
		/// <returns> Shared instance of a CombinedParticleKernel </returns>
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> GetCached(const OS::Path& shaderPath) {
			return GetCached(sizeof(SimulationTaskSettings), shaderPath,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		/// <summary> Virtual destructor </summary>
		virtual ~CombinedParticleKernel();

		/// <summary>
		/// Creates a graphics simulation kernel instance (Used by GraphicsSimulation; Users are not really expected to do anything with it)
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> A new instance of the kernel </returns>
		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;

	private:
		// Underlying shader class
		const Reference<const Graphics::ShaderClass> m_shaderClass;

		// CombinedGraphicsSimulationKernel Instance constructor
		typedef Function<
			Reference<GraphicsSimulation::KernelInstance>,
			SceneContext*, const Graphics::ShaderClass*,
			const Graphics::BindingSet::Descriptor::BindingSearchFunctions&> CreateInstanceFn;
		const CreateInstanceFn m_createInstance;

		// CountTotalElementNumber<SimulationTaskSettings>
		typedef Function<size_t, const GraphicsSimulation::Task* const*, size_t> CountTotalElementNumberFn;
		const CountTotalElementNumberFn m_countTotalElementCount;

		// To maintain integrity, constructor is private
		CombinedParticleKernel(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// "backend" of CombinedParticleKernel::Create<>
		static Reference<CombinedParticleKernel> Create(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// "backend" of CombinedParticleKernel::GetCached<>
		static Reference<CombinedParticleKernel> GetCached(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// "backend" of CombinedParticleKernel::GetCached<>
		static Reference<CombinedParticleKernel> GetCached(
			const size_t settingsSize, const OS::Path& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// Wrapper on top of CombinedGraphicsSimulationKernel<SimulationTaskSettings>
		template<typename SimulationTaskSettings>
		inline static Reference<GraphicsSimulation::KernelInstance> CreateSharedKernel(
			SceneContext* context, const Graphics::ShaderClass* shaderClass,
			const Graphics::BindingSet::Descriptor::BindingSearchFunctions& bindings) {
			return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, shaderClass, bindings);
		}

		// Counts total thread count based on task settings
		template<typename SimulationTaskSettings>
		inline static size_t CountTotalElementNumber(const GraphicsSimulation::Task* const* tasks, size_t taskCount) {
			size_t totalElementCount = 0u;
			const GraphicsSimulation::Task* const* ptr = tasks;
			const GraphicsSimulation::Task* const* const end = ptr + taskCount;
			while (ptr < end) {
				totalElementCount += (*ptr)->GetSettings<SimulationTaskSettings>().taskThreadCount;
				ptr++;
			}
			return totalElementCount;
		}

		// Some private stuff resides here
		struct Helpers;
	};
}
