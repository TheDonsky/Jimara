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
		/// Creates a new CombinedParticleKernel for some shader
		/// </summary>
		/// <typeparam name="SimulationTaskSettings"> 
		/// Type of the task settings for the underlying CombinedGraphicsSimulationKernel (refer to it's documentation for further details)
		/// </typeparam>
		/// <param name="shaderPath"> Shader path </param>
		/// <returns> New instance of a CombinedParticleKernel </returns>
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> Create(const std::string_view& shaderPath) {
			return Create(sizeof(SimulationTaskSettings), shaderPath,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		/// <summary>
		/// Returns a shared(cached) instance CombinedParticleKernel for some shader
		/// </summary>
		/// <typeparam name="SimulationTaskSettings"> 
		/// Type of the task settings for the underlying CombinedGraphicsSimulationKernel (refer to it's documentation for further details)
		/// </typeparam>
		/// <param name="shaderPath"> path to the compute shader </param>
		/// <returns> Shared instance of a CombinedParticleKernel </returns>
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> GetCached(const std::string_view& shaderPath) {
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
		// Underlying shader path
		const std::string m_shaderPath;

		// CombinedGraphicsSimulationKernel Instance constructor
		typedef Function<
			Reference<GraphicsSimulation::KernelInstance>,
			SceneContext*, const std::string_view&,
			const Graphics::BindingSet::BindingSearchFunctions&> CreateInstanceFn;
		const CreateInstanceFn m_createInstance;

		// CountTotalElementNumber<SimulationTaskSettings>
		typedef Function<size_t, const GraphicsSimulation::Task* const*, size_t> CountTotalElementNumberFn;
		const CountTotalElementNumberFn m_countTotalElementCount;

		// To maintain integrity, constructor is private
		CombinedParticleKernel(
			const size_t settingsSize, const std::string_view& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// "backend" of CombinedParticleKernel::Create<>
		static Reference<CombinedParticleKernel> Create(
			const size_t settingsSize, const std::string_view& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// "backend" of CombinedParticleKernel::GetCached<>
		static Reference<CombinedParticleKernel> GetCached(
			const size_t settingsSize, const std::string_view& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		// Wrapper on top of CombinedGraphicsSimulationKernel<SimulationTaskSettings>
		template<typename SimulationTaskSettings>
		inline static Reference<GraphicsSimulation::KernelInstance> CreateSharedKernel(
			SceneContext* context, const std::string_view& shaderPath,
			const Graphics::BindingSet::BindingSearchFunctions& bindings) {
			return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, shaderPath, bindings);
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
