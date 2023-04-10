#include "GraphicsRNG.h"
#include "../../../../Math/Helpers.h"
#include "../../../../Math/Random.h"


namespace Jimara {
	namespace {
		struct GraphicsRNG_SharedInstanceKey {
			Reference<Graphics::GraphicsDevice> device;
			Reference<Graphics::ShaderLoader> shaderLoader;

			inline bool operator<(const GraphicsRNG_SharedInstanceKey& other)const {
				return device < other.device || (device == other.device && shaderLoader < other.shaderLoader);
			}
			inline bool operator==(const GraphicsRNG_SharedInstanceKey& other)const {
				return device == other.device && shaderLoader == other.shaderLoader;
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::GraphicsRNG_SharedInstanceKey> {
		size_t operator()(const Jimara::GraphicsRNG_SharedInstanceKey& key)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Graphics::GraphicsDevice*>()(key.device),
				std::hash<Jimara::Graphics::ShaderLoader*>()(key.shaderLoader));
		}
	};
}

namespace Jimara {
	struct GraphicsRNG::Helpers {
		static const constexpr uint32_t IN_FLIGHT_COMMAND_BUFFERS = 5u;
		static const constexpr uint32_t BLOCK_SIZE = 256u;

		struct SeedPipelineSettings {
			alignas(4) int32_t bufferStart = 0u;
			alignas(4) uint32_t bufferEnd = 0u;
			alignas(4) uint32_t seed = 0u;
		};

		inline static const Graphics::ShaderClass* SeedShaderClass() {
			static const Graphics::ShaderClass SEED_SHADER_CLASS = Graphics::ShaderClass("Jimara/Environment/Rendering/Algorithms/Random/Jimara_RNG_Seed");
			return &SEED_SHADER_CLASS;
		}

		struct PipelineDescriptor 
			: public virtual Graphics::ComputePipeline::Descriptor 
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			Reference<Graphics::Shader> shader;
			Graphics::ArrayBufferReference<GraphicsRNG::State> generators;
			Reference<Graphics::Buffer> settings;
			size_t elemCount = 0u;

			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }

			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 1u }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return settings; }
			
			inline virtual size_t StructuredBufferCount()const override { return 1u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t)const override { return BindingInfo { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0u }; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return generators; };
			
			inline virtual size_t TextureSamplerCount()const override { return 0u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return {}; }

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1u; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return this; }

			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
			inline virtual Size3 NumBlocks()const override { return Size3((elemCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u); }
		};

#pragma warning(disable: 4250)
		class SharedInstance 
			: public virtual GraphicsRNG
			, public virtual ObjectCache<GraphicsRNG_SharedInstanceKey>::StoredObject {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<Graphics::ShaderLoader> m_shaderLoader;
			const Reference<PipelineDescriptor> m_seedPipelineDescriptor = Object::Instantiate<PipelineDescriptor>();
			std::mutex m_allocationLock;
			Reference<Graphics::ComputePipeline> m_seedPipeline;
			std::vector<Reference<Graphics::PrimaryCommandBuffer>> m_commandBuffers;
			size_t m_commandBufferId = 0u;


		public:
			inline SharedInstance(Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader) 
				: m_device(device), m_shaderLoader(shaderLoader) {
				m_buffer = m_device->CreateArrayBuffer<State>(0u);
				if (m_buffer == nullptr)
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed to initialize the buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Reference<Graphics::ShaderSet> shaderSet = m_shaderLoader->LoadShaderSet("");
				if (shaderSet == nullptr) {
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(SeedShaderClass(), Graphics::PipelineStage::COMPUTE);
				if (binary == nullptr) {
					m_device->Log()->Error(
						"GraphicsRNG::Helpers::SharedInstance - Failed to shader module for '", SeedShaderClass()->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(m_device);
				if (shaderCache == nullptr) {
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed get shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_seedPipelineDescriptor->shader = shaderCache->GetShader(binary);
				if (m_seedPipelineDescriptor->shader == nullptr) {
					m_device->Log()->Error(
						"GraphicsRNG::Helpers::SharedInstance - Failed get shader module for '", SeedShaderClass()->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_seedPipelineDescriptor->settings = m_device->CreateConstantBuffer<SeedPipelineSettings>();
				if (m_seedPipelineDescriptor->settings == nullptr) {
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed to create seed pipeline settings! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_seedPipeline = m_device->CreateComputePipeline(m_seedPipelineDescriptor, IN_FLIGHT_COMMAND_BUFFERS);
				if (m_seedPipeline == nullptr) {
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed create seed pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				Reference<Graphics::CommandPool> commandPool = m_device->GraphicsQueue()->CreateCommandPool();
				if (commandPool == nullptr) {
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_commandBuffers = commandPool->CreatePrimaryCommandBuffers(IN_FLIGHT_COMMAND_BUFFERS);
				if (m_commandBuffers.size() != IN_FLIGHT_COMMAND_BUFFERS) 
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed create enough command buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				for (size_t i = 0; i < m_commandBuffers.size(); i++)
					if (m_commandBuffers[i] == nullptr) {
						m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Command buffer ", i, " not created! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_commandBuffers[i] = m_commandBuffers[m_commandBuffers.size() - 1u];
						m_commandBuffers.pop_back();
					}
			}

			inline Graphics::ArrayBufferReference<State> ExpandBuffer(size_t minSize) {
				std::unique_lock<std::mutex> lock(m_allocationLock);
				Graphics::ArrayBufferReference<State> buffer = (*this);
				if (buffer != nullptr && buffer->ObjectCount() >= minSize) return buffer;
				
				// Create new buffer:
				{
					size_t size = 1; while (size < minSize) size <<= 1u;
					buffer = m_device->CreateArrayBuffer<State>(size);
				}

				if (buffer == nullptr)
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance::ExpandBuffer - Failed to create new buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (m_commandBuffers.size() <= 0u || m_seedPipeline == nullptr)
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance::ExpandBuffer - Pipeline and/or command buffer not initialized! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else {
					// Update pipeline descriptor:
					{
						m_seedPipelineDescriptor->generators = buffer;
						m_seedPipelineDescriptor->elemCount = static_cast<uint32_t>(buffer->ObjectCount());
					}

					// Update seed settings:
					{
						Graphics::BufferReference<SeedPipelineSettings> seedSettings = m_seedPipelineDescriptor->settings.operator->();
						SeedPipelineSettings& settings = seedSettings.Map();
						settings.bufferStart = 0u;
						settings.bufferEnd = static_cast<uint32_t>(buffer->ObjectCount());
						settings.seed = static_cast<uint32_t>(Random::ThreadRNG()());
						seedSettings->Unmap(true);
					}

					// Record and execute pipeline:
					{
						Graphics::PrimaryCommandBuffer* commandBuffer = m_commandBuffers[m_commandBufferId];
						commandBuffer->Wait();
						commandBuffer->BeginRecording();
						m_seedPipeline->Execute(Graphics::InFlightBufferInfo(commandBuffer, m_commandBufferId));
						commandBuffer->EndRecording();
						m_device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
						m_commandBufferId = (m_commandBufferId + 1) % m_commandBuffers.size();
					}
				}

				// Update buffer reference
				{
					std::unique_lock<SpinLock> lock(m_bufferLock);
					m_buffer = buffer;
				}

				return buffer;
			}
		};
#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<GraphicsRNG_SharedInstanceKey> {
		public:
			inline static Reference<SharedInstance> GetFor(Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader) {
				if (device == nullptr || shaderLoader == nullptr) return nullptr;
				static InstanceCache cache;
				return cache.GetCachedOrCreate(
					GraphicsRNG_SharedInstanceKey{ Reference<Graphics::GraphicsDevice>(device), Reference<Graphics::ShaderLoader>(shaderLoader) }, false,
					[&]() -> Reference<SharedInstance> { return Object::Instantiate<SharedInstance>(device, shaderLoader); });
			}
		};
	};

	Reference<GraphicsRNG> GraphicsRNG::GetShared(Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader) {
		if (device != nullptr && shaderLoader == nullptr)
			device->Log()->Error("GraphicsRNG::GetShared - Shader loader not provided!");
		return Helpers::InstanceCache::GetFor(device, shaderLoader);
	}

	Reference<GraphicsRNG> GraphicsRNG::GetShared(SceneContext* context) {
		if (context == nullptr) return nullptr;
		Reference<GraphicsRNG> instance = GetShared(context->Graphics()->Device(), context->Graphics()->Configuration().ShaderLoader());
		context->StoreDataObject(instance);
		return instance;
	}

	GraphicsRNG::GraphicsRNG() {}

	GraphicsRNG::~GraphicsRNG() {}

	Graphics::ArrayBufferReference<GraphicsRNG::State> GraphicsRNG::GetBuffer(size_t minSize) {
		Graphics::ArrayBufferReference<GraphicsRNG::State> buffer = (*this);
		if (buffer != nullptr && buffer->ObjectCount() >= minSize) return buffer;
		else return dynamic_cast<Helpers::SharedInstance*>(this)->ExpandBuffer(minSize);
	}
}
