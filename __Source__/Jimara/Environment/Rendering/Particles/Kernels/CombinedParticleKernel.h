#pragma once
#include "../ParticleKernels.h"
#include "../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"


namespace Jimara {
	class JIMARA_API CombinedParticleKernel : public virtual GraphicsSimulation::Kernel {
	public:
		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> Create(const Graphics::ShaderClass* shaderClass) {
			return Create(sizeof(SimulationTaskSettings), shaderClass,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> GetCached(const Graphics::ShaderClass* shaderClass) {
			return GetCached(sizeof(SimulationTaskSettings), shaderClass,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		template<typename SimulationTaskSettings>
		inline static Reference<CombinedParticleKernel> GetCached(const OS::Path& shaderPath) {
			return GetCached(sizeof(SimulationTaskSettings), shaderPath,
				CreateInstanceFn(CombinedParticleKernel::CreateSharedKernel<SimulationTaskSettings>),
				CountTotalElementNumberFn(CombinedParticleKernel::CountTotalElementNumber<SimulationTaskSettings>));
		}

		virtual ~CombinedParticleKernel();

		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;

	private:
		typedef Function<
			Reference<GraphicsSimulation::KernelInstance>,
			SceneContext*, const Graphics::ShaderClass*,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet&> CreateInstanceFn;
		typedef Function<size_t, const GraphicsSimulation::Task* const*, size_t> CountTotalElementNumberFn;
		const Reference<const Graphics::ShaderClass> m_shaderClass;
		const CreateInstanceFn m_createInstance;
		const CountTotalElementNumberFn m_countTotalElementCount;

		CombinedParticleKernel(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		static Reference<CombinedParticleKernel> Create(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		static Reference<CombinedParticleKernel> GetCached(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		static Reference<CombinedParticleKernel> GetCached(
			const size_t settingsSize, const OS::Path& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount);

		template<typename SimulationTaskSettings>
		inline static Reference<GraphicsSimulation::KernelInstance> CreateSharedKernel(
			SceneContext* context, const Graphics::ShaderClass* shaderClass,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings) {
			return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, shaderClass, bindings);
		}

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

		struct Helpers;
	};
}
