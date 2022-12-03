#include "CombinedParticleInitializationKernel.h"
#include "../../../Algorithms/Random/GraphicsRNG.h"


namespace Jimara {
	namespace InitializationKernels {
		struct CombinedParticleInitializationKernel::Helpers {
			class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
			private:
				const Reference<SceneContext> m_context;
				const Reference<GraphicsRNG> m_graphicsRNG;
				const CountTotalElementNumberFn m_countTotalElementCount;
				const Reference<GraphicsSimulation::KernelInstance> m_combinedKernel;
				const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_rngBufferBinding;

			public:
				inline KernelInstance(
					SceneContext* context,
					GraphicsRNG* graphicsRNG,
					const CountTotalElementNumberFn& countTotalElementCount,
					GraphicsSimulation::KernelInstance* combinedKernel,
					Graphics::ShaderResourceBindings::StructuredBufferBinding* rngBufferBinding)
					: m_context(context)
					, m_graphicsRNG(graphicsRNG)
					, m_countTotalElementCount(countTotalElementCount)
					, m_combinedKernel(combinedKernel)
					, m_rngBufferBinding(rngBufferBinding) {}

				inline virtual ~KernelInstance() {}

				inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) final override {
					const size_t totalElementCount = m_countTotalElementCount(tasks, taskCount);
					if (m_rngBufferBinding != nullptr &&
						(m_rngBufferBinding->BoundObject() == nullptr || m_rngBufferBinding->BoundObject()->ObjectCount() < totalElementCount)) {
						m_rngBufferBinding->BoundObject() = m_graphicsRNG->GetBuffer(totalElementCount);
						if (m_rngBufferBinding->BoundObject() == nullptr) {
							m_context->Log()->Error(
								"PlaceOnSphere::Kernel::KernelInstance::Execute - Failed to retrieve Graphics RNG buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
					}
					m_combinedKernel->Execute(commandBufferInfo, tasks, taskCount);
				}
			};

			struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
				mutable Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> rngBufferBinding;
				
				inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override { 
					static const std::string rngBufferName = "jimara_CombinedParticleInitializationKernel_rngBuffer";
					if (name != rngBufferName) return nullptr;
					if (rngBufferBinding == nullptr) 
						rngBufferBinding = Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>();
					return rngBufferBinding;
				}
				inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
			};

#pragma warning(disable: 4250)
			class CachedInstance : public virtual CombinedParticleInitializationKernel, public virtual ObjectCache<Reference<const Graphics::ShaderClass>>::StoredObject {
			public:
				inline CachedInstance(
					const size_t settingsSize, const Graphics::ShaderClass* shaderClass, 
					const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount)
					: GraphicsSimulation::Kernel(settingsSize)
					, CombinedParticleInitializationKernel(settingsSize, shaderClass, createFn, countTotalElementCount) {}
			};

			class InstanceCache : public virtual ObjectCache<Reference<const Graphics::ShaderClass>> {
			public:
				static Reference<CombinedParticleInitializationKernel> GetFor(
					const size_t settingsSize, const Graphics::ShaderClass* shaderClass, 
					const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
					static InstanceCache cache;
					return cache.GetCachedOrCreate(shaderClass, false, [&]() -> Reference<CachedInstance> {
						return Object::Instantiate<CachedInstance>(settingsSize, shaderClass, createFn, countTotalElementCount);
						});
				}
			};

			class CachedShaderClass : public virtual Graphics::ShaderClass, public virtual ObjectCache<OS::Path>::StoredObject {
			public:
				inline CachedShaderClass(const OS::Path& shaderPath) : Graphics::ShaderClass(shaderPath) {}
			};

			class ShaderClassCache : public virtual ObjectCache<OS::Path> {
			public:
				static Reference<CachedShaderClass> GetFor(const OS::Path& path) {
					static ShaderClassCache cache;
					return cache.GetCachedOrCreate(path, false, [&]()->Reference<CachedShaderClass> { return Object::Instantiate<CachedShaderClass>(path); });
				}
			};
#pragma warning(default: 4250)
		};

		Reference<CombinedParticleInitializationKernel> CombinedParticleInitializationKernel::Create(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
			if (shaderClass == nullptr) return nullptr;
			Reference<CombinedParticleInitializationKernel> instance =
				new CombinedParticleInitializationKernel(settingsSize, shaderClass, createFn, countTotalElementCount);
			instance->ReleaseRef();
			return instance;
		}

		Reference<CombinedParticleInitializationKernel> CombinedParticleInitializationKernel::GetCached(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
			if (shaderClass == nullptr) return nullptr;
			else return Helpers::InstanceCache::GetFor(settingsSize, shaderClass, createFn, countTotalElementCount);
		}

		Reference<CombinedParticleInitializationKernel> CombinedParticleInitializationKernel::GetCached(
			const size_t settingsSize, const OS::Path& shaderPath,
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
			const Reference<Graphics::ShaderClass> shaderClass = Helpers::ShaderClassCache::GetFor(shaderPath);
			assert(shaderClass != nullptr);
			return GetCached(settingsSize, shaderClass, createFn, countTotalElementCount);
		}

		CombinedParticleInitializationKernel::CombinedParticleInitializationKernel(
			const size_t settingsSize, const Graphics::ShaderClass* shaderClass, 
			const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount)
			: GraphicsSimulation::Kernel(settingsSize)
			, m_shaderClass(shaderClass)
			, m_createInstance(createFn)
			, m_countTotalElementCount(countTotalElementCount) {}

		CombinedParticleInitializationKernel::~CombinedParticleInitializationKernel() {}

		Reference<GraphicsSimulation::KernelInstance> CombinedParticleInitializationKernel::CreateInstance(SceneContext* context)const {
			if (context == nullptr) return nullptr;
			Helpers::BindingSet bindings;
			Reference<GraphicsSimulation::KernelInstance> comnbinedKernelInstance = m_createInstance(context, m_shaderClass, bindings);
			if (comnbinedKernelInstance == nullptr) {
				context->Log()->Error(
					"CombinedParticleInitializationKernel::CreateInstance - Failed to create combined kernel instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			Reference<GraphicsRNG> graphicsRNG;
			if (bindings.rngBufferBinding != nullptr) {
				graphicsRNG = GraphicsRNG::GetShared(context);
				if (graphicsRNG == nullptr) {
					context->Log()->Error(
						"CombinedParticleInitializationKernel::CreateInstance - Failed to get graphics RNG instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
			}
			return Object::Instantiate<Helpers::KernelInstance>(context, graphicsRNG, m_countTotalElementCount, comnbinedKernelInstance, bindings.rngBufferBinding);
		}
	}
}
