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

#pragma warning(disable: 4250)
		class SharedInstance 
			: public virtual GraphicsRNG
			, public virtual ObjectCache<GraphicsRNG_SharedInstanceKey>::StoredObject {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<Graphics::Experimental::ComputePipeline> m_seedPipeline;
			const Reference<Graphics::BindingSet> m_bindingSet;
			const Graphics::BufferReference<SeedPipelineSettings> m_settings;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_generators;
			const std::vector<Reference<Graphics::PrimaryCommandBuffer>> m_commandBuffers;
			std::mutex m_allocationLock;
			size_t m_commandBufferId = 0u;


		public:
			inline SharedInstance(
				Graphics::GraphicsDevice* device,
				Graphics::Experimental::ComputePipeline* seedPipeline,
				Graphics::BindingSet* bindingSet,
				Graphics::Buffer* settings,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* generators,
				std::vector<Reference<Graphics::PrimaryCommandBuffer>>&& commandBuffers) 
				: m_device(device)
				, m_seedPipeline(seedPipeline)
				, m_bindingSet(bindingSet)
				, m_settings(settings)
				, m_generators(generators)
				, m_commandBuffers(std::move(commandBuffers)) {
				assert(m_device != nullptr);
				assert(m_seedPipeline != nullptr);
				assert(m_bindingSet != nullptr);
				assert(m_settings != nullptr);
				assert(m_generators != nullptr);
				assert(m_commandBuffers.size() == IN_FLIGHT_COMMAND_BUFFERS);
				m_buffer = m_device->CreateArrayBuffer<State>(0u);
				if (m_buffer == nullptr)
					m_device->Log()->Error("GraphicsRNG::Helpers::SharedInstance - Failed to initialize the buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
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
					m_generators->BoundObject() = buffer;

					// Update seed settings:
					{
						SeedPipelineSettings& settings = m_settings.Map();
						settings.bufferStart = 0u;
						settings.bufferEnd = static_cast<uint32_t>(buffer->ObjectCount());
						settings.seed = static_cast<uint32_t>(Random::ThreadRNG()());
						m_settings->Unmap(true);
					}

					// Record and execute pipeline:
					{
						Graphics::PrimaryCommandBuffer* const commandBuffer = m_commandBuffers[m_commandBufferId];
						const Graphics::InFlightBufferInfo inFlightBuffer = Graphics::InFlightBufferInfo(commandBuffer, m_commandBufferId);
						commandBuffer->Wait();
						commandBuffer->BeginRecording();
						m_bindingSet->Update(inFlightBuffer);
						m_bindingSet->Bind(inFlightBuffer);
						m_seedPipeline->Dispatch(commandBuffer,
							Size3((static_cast<uint32_t>(buffer->ObjectCount()) + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u));
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
					[&]() -> Reference<SharedInstance> { 
						if (device == nullptr) return nullptr;
						auto fail = [&](const auto&... message) {
							device->Log()->Error("GraphicsRNG::Helpers::InstanceCache::GetFor - ", message...);
							return nullptr;
						};

						if (shaderLoader == nullptr)
							return fail("Shader loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
						if (shaderSet == nullptr)
							return fail("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						static const Graphics::ShaderClass SEED_SHADER_CLASS = Graphics::ShaderClass("Jimara/Environment/Rendering/Algorithms/Random/Jimara_RNG_Seed");
						const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&SEED_SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
						if (binary == nullptr)
							return fail("Failed to shader module for '", SEED_SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						const Reference<Graphics::Experimental::ComputePipeline> computePipeline = device->GetComputePipeline(binary);
						if (computePipeline == nullptr)
							return fail("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						else if (computePipeline->BindingSetCount() != 1u)
							return fail("Compute pipeline expected to have exactly 1 binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						const Reference<Graphics::ResourceBinding<Graphics::Buffer>> settings =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>();
						settings->BoundObject() = device->CreateConstantBuffer<SeedPipelineSettings>();
						if (settings->BoundObject() == nullptr)
							return fail("Failed to create seed pipeline settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						auto findConstantBuffer = [&](const auto&) { return settings; };

						const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> generators =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
						auto findStructuredBuffer = [&](const auto&) { return generators; };

						const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(IN_FLIGHT_COMMAND_BUFFERS);
						if (bindingPool == nullptr)
							return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						Graphics::BindingSet::Descriptor bindingSetDescriptor = {};
						{
							bindingSetDescriptor.pipeline = computePipeline;
							bindingSetDescriptor.bindingSetId = 0u;
							bindingSetDescriptor.find.constantBuffer = &findConstantBuffer;
							bindingSetDescriptor.find.structuredBuffer = &findStructuredBuffer;
						}
						const Reference<Graphics::BindingSet> bindingSet = bindingPool->AllocateBindingSet(bindingSetDescriptor);
						if (bindingSet == nullptr)
							return fail("Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						Reference<Graphics::CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
						if (commandPool == nullptr)
							return fail("Failed create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						std::vector<Reference<Graphics::PrimaryCommandBuffer>> commandBuffers = 
							commandPool->CreatePrimaryCommandBuffers(IN_FLIGHT_COMMAND_BUFFERS);
						if (commandBuffers.size() != IN_FLIGHT_COMMAND_BUFFERS)
							return fail("Failed create enough command buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						for (size_t i = 0; i < commandBuffers.size(); i++)
							if (commandBuffers[i] == nullptr)
								return fail("Command buffer ", i, " not created! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						const Reference<SharedInstance> result = new SharedInstance(
							device, computePipeline, bindingSet, settings->BoundObject(), generators, std::move(commandBuffers));
						result->ReleaseRef();
						return result; 
					});
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
