#include "CombinedParticleKernel.h"
#include "../Algorithms/Random/GraphicsRNG.h"


namespace Jimara {
	struct CombinedParticleKernel::Helpers {
		struct TimeInfo {
			alignas(16) Vector4 deltaTimes = {};
			alignas(16) Vector4 totalTimes = {};
		};

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			const Graphics::BufferReference<Helpers::TimeInfo> m_timeInfoBuffer;
			const Reference<GraphicsRNG> m_graphicsRNG;
			const CountTotalElementNumberFn m_countTotalElementCount;
			const Reference<GraphicsSimulation::KernelInstance> m_combinedKernel;
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_rngBufferBinding;

		public:
			inline KernelInstance(
				SceneContext* context,
				Graphics::Buffer* timeInfoBuffer,
				GraphicsRNG* graphicsRNG,
				const CountTotalElementNumberFn& countTotalElementCount,
				GraphicsSimulation::KernelInstance* combinedKernel,
				Graphics::ShaderResourceBindings::StructuredBufferBinding* rngBufferBinding)
				: m_context(context)
				, m_timeInfoBuffer(timeInfoBuffer)
				, m_graphicsRNG(graphicsRNG)
				, m_countTotalElementCount(countTotalElementCount)
				, m_combinedKernel(combinedKernel)
				, m_rngBufferBinding(rngBufferBinding) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) final override {
				if (m_timeInfoBuffer != nullptr) {
					TimeInfo& info = m_timeInfoBuffer.Map();
					info.deltaTimes = Vector4(
						0.0f,
						m_context->Time()->UnscaledDeltaTime(),
						m_context->Time()->ScaledDeltaTime(),
						m_context->Physics()->Time()->ScaledDeltaTime());
					info.totalTimes = Vector4(
						0.0f,
						m_context->Time()->TotalUnscaledTime(),
						m_context->Time()->TotalScaledTime(),
						m_context->Physics()->Time()->TotalScaledTime());
					m_timeInfoBuffer->Unmap(true);
				}
				if (m_rngBufferBinding != nullptr) {
					const size_t totalElementCount = m_countTotalElementCount(tasks, taskCount);
					if (m_rngBufferBinding->BoundObject() == nullptr || m_rngBufferBinding->BoundObject()->ObjectCount() < totalElementCount) {
						m_rngBufferBinding->BoundObject() = m_graphicsRNG->GetBuffer(totalElementCount);
						if (m_rngBufferBinding->BoundObject() == nullptr) {
							m_context->Log()->Error(
								"PlaceOnSphere::Kernel::KernelInstance::Execute - Failed to retrieve Graphics RNG buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
					}
				}
				m_combinedKernel->Execute(commandBufferInfo, tasks, taskCount);
			}
		};

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			mutable Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> timeBufferBinding;
			mutable Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> rngBufferBinding;

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				static const std::string timeBufferName = "jimara_CombinedParticleKernel_timeBuffer";
				if (name != timeBufferName) return nullptr;
				if (timeBufferBinding == nullptr)
					timeBufferBinding = Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>();
				return timeBufferBinding;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				static const std::string rngBufferName = "jimara_CombinedParticleKernel_rngBuffer";
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
		class CachedInstance : public virtual CombinedParticleKernel, public virtual ObjectCache<Reference<const Graphics::ShaderClass>>::StoredObject {
		public:
			inline CachedInstance(
				const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
				const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount)
				: GraphicsSimulation::Kernel(settingsSize)
				, CombinedParticleKernel(settingsSize, shaderClass, createFn, countTotalElementCount) {}
		};

		class InstanceCache : public virtual ObjectCache<Reference<const Graphics::ShaderClass>> {
		public:
			static Reference<CombinedParticleKernel> GetFor(
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

	Reference<CombinedParticleKernel> CombinedParticleKernel::Create(
		const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
		const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
		if (shaderClass == nullptr) return nullptr;
		Reference<CombinedParticleKernel> instance =
			new CombinedParticleKernel(settingsSize, shaderClass, createFn, countTotalElementCount);
		instance->ReleaseRef();
		return instance;
	}

	Reference<CombinedParticleKernel> CombinedParticleKernel::GetCached(
		const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
		const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
		if (shaderClass == nullptr) return nullptr;
		else return Helpers::InstanceCache::GetFor(settingsSize, shaderClass, createFn, countTotalElementCount);
	}

	Reference<CombinedParticleKernel> CombinedParticleKernel::GetCached(
		const size_t settingsSize, const OS::Path& shaderPath,
		const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount) {
		const Reference<Graphics::ShaderClass> shaderClass = Helpers::ShaderClassCache::GetFor(shaderPath);
		assert(shaderClass != nullptr);
		return GetCached(settingsSize, shaderClass, createFn, countTotalElementCount);
	}

	CombinedParticleKernel::CombinedParticleKernel(
		const size_t settingsSize, const Graphics::ShaderClass* shaderClass,
		const CreateInstanceFn& createFn, const CountTotalElementNumberFn& countTotalElementCount)
		: GraphicsSimulation::Kernel(settingsSize)
		, m_shaderClass(shaderClass)
		, m_createInstance(createFn)
		, m_countTotalElementCount(countTotalElementCount) {}

	CombinedParticleKernel::~CombinedParticleKernel() {}

	Reference<GraphicsSimulation::KernelInstance> CombinedParticleKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		Helpers::BindingSet bindings;
		Reference<GraphicsSimulation::KernelInstance> comnbinedKernelInstance = m_createInstance(context, m_shaderClass, bindings);
		if (comnbinedKernelInstance == nullptr) {
			context->Log()->Error(
				"CombinedParticleKernel::CreateInstance - Failed to create combined kernel instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		if (bindings.timeBufferBinding != nullptr) {
			bindings.timeBufferBinding->BoundObject() = context->Graphics()->Device()->CreateConstantBuffer<Helpers::TimeInfo>();
			if (bindings.timeBufferBinding->BoundObject() == nullptr) {
				context->Log()->Error(
					"CombinedParticleKernel::CreateInstance - Failed to create time info buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
		}
		Reference<GraphicsRNG> graphicsRNG;
		if (bindings.rngBufferBinding != nullptr) {
			graphicsRNG = GraphicsRNG::GetShared(context);
			if (graphicsRNG == nullptr) {
				context->Log()->Error(
					"CombinedParticleKernel::CreateInstance - Failed to get graphics RNG instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
		}
		return Object::Instantiate<Helpers::KernelInstance>(
			context,
			(bindings.timeBufferBinding == nullptr) ? nullptr : bindings.timeBufferBinding->BoundObject(),
			graphicsRNG, m_countTotalElementCount, comnbinedKernelInstance, bindings.rngBufferBinding);
	}
}
