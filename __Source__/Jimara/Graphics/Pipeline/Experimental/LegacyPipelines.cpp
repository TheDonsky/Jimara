#include "LegacyPipelines.h"



namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		Reference<LegacyPipeline> LegacyPipeline::Create(
			GraphicsDevice* device, size_t maxInFlightCommandBuffers, 
			Graphics::Pipeline* pipeline, const Graphics::Legacy::PipelineDescriptor* descriptor) {
			if (device == nullptr) return nullptr;
			device->Log()->Warning(
				"LegacyPipeline::Create - Legacy pipelines are depricated. Please use the new API instead! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			auto fail = [&](const auto&... message) { 
				device->Log()->Error("LegacyPipeline::Create - ", message...);
				return nullptr;
			};

			if (pipeline == nullptr)
				return fail("Pipeline not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			if (descriptor == nullptr)
				return fail("Descriptor not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			if (device == nullptr)
				return fail("Device not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			PipelineData data = {};
			data.device = device;
			data.descriptor = descriptor;
			data.bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
			if (data.bindingPool == nullptr)
				return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			const size_t bindingSetCount = pipeline->BindingSetCount();
			if (descriptor->BindingSetCount() != bindingSetCount)
				return fail("Descriptor does not have correct number of binding sets(", 
					descriptor->BindingSetCount(), " instead of ", bindingSetCount, ")! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			for (size_t bindingSetId = 0u; bindingSetId < bindingSetCount; bindingSetId++) {
				const Graphics::Legacy::PipelineDescriptor::BindingSetDescriptor* bindingSetDescriptor = descriptor->BindingSet(bindingSetId);
				if (bindingSetDescriptor == nullptr)
					return fail("Null binding set descriptor returned for set ", bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				if (bindingSetDescriptor->SetByEnvironment()) 
					continue;

				BindingSetMappings mappings = {};
				mappings.bindingSetIndex = bindingSetId;

				BindingSet::Descriptor setDescriptor = {};
				setDescriptor.pipeline = pipeline;
				setDescriptor.bindingSetId = bindingSetId;

				auto findOrCreateBinding = [](
					const BindingSet::BindingDescriptor& desc,
					auto& existingBindings, const size_t setBindingCount, const auto& getSetBindingInfo) {

					using BindingReference = std::remove_pointer_t<decltype(existingBindings.Data())>;
					using BindingType = std::remove_pointer_t<decltype(BindingReference().operator->())>;
					for (size_t i = 0u; i < existingBindings.Size(); i++) {
						BindingType* binding = existingBindings[i];
						if (getSetBindingInfo(binding->BindingIndex()).binding == desc.binding)
							return binding;
					}

					for (size_t i = 0u; i < setBindingCount; i++) {
						if (getSetBindingInfo(i).binding != desc.binding) continue;
						const Reference<BindingType> newBinding = Object::Instantiate<BindingType>(i);
						existingBindings.Push(newBinding);
						return newBinding.operator->();
					}

					return Reference<BindingType>().operator->();
				};

				auto findConstantBuffer = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<Buffer>> {
					return findOrCreateBinding(desc,
						mappings.constantBuffers, bindingSetDescriptor->ConstantBufferCount(),
						[&](size_t index) { return bindingSetDescriptor->ConstantBufferInfo(index); });
				};
				setDescriptor.find.constantBuffer = &findConstantBuffer;

				auto findStructuredBuffer = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<ArrayBuffer>> {
					return findOrCreateBinding(desc,
						mappings.structuredBuffers, bindingSetDescriptor->StructuredBufferCount(),
						[&](size_t index) { return bindingSetDescriptor->StructuredBufferInfo(index); });
				};
				setDescriptor.find.structuredBuffer = &findStructuredBuffer;

				auto findTextureSampler = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<TextureSampler>> {
					return findOrCreateBinding(desc,
						mappings.textureSamplers, bindingSetDescriptor->TextureSamplerCount(),
						[&](size_t index) { return bindingSetDescriptor->TextureSamplerInfo(index); });
				};
				setDescriptor.find.textureSampler = &findTextureSampler;

				auto findTextureView = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<TextureView>> {
					return findOrCreateBinding(desc,
						mappings.textureViews, bindingSetDescriptor->TextureViewCount(),
						[&](size_t index) { return bindingSetDescriptor->TextureViewInfo(index); });
				};
				setDescriptor.find.textureView = &findTextureView;

				auto findBindlessStructuredBuffers = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<BindlessSet<ArrayBuffer>::Instance>> {
					if (!bindingSetDescriptor->IsBindlessArrayBufferArray())
						return nullptr;
					if (mappings.bindlessStructuredBuffers == nullptr)
						mappings.bindlessStructuredBuffers = Object::Instantiate<ResourceBinding<typename BindlessSet<ArrayBuffer>::Instance>>();
					return mappings.bindlessStructuredBuffers;
				};
				setDescriptor.find.bindlessStructuredBuffers = &findBindlessStructuredBuffers;

				auto findBindlessTextureSamplers = [&](BindingSet::BindingDescriptor desc) -> Reference<const ResourceBinding<BindlessSet<TextureSampler>::Instance>> {
					if (!bindingSetDescriptor->IsBindlessTextureSamplerArray())
						return nullptr;
					if (mappings.bindlessTextureSamplers == nullptr)
						mappings.bindlessTextureSamplers = Object::Instantiate<ResourceBinding<typename BindlessSet<TextureSampler>::Instance>>();
					return mappings.bindlessTextureSamplers;
				};
				setDescriptor.find.bindlessTextureSamplers = &findBindlessTextureSamplers;

				mappings.bindingSet = data.bindingPool->AllocateBindingSet(setDescriptor);
				if (mappings.bindingSet == nullptr)
					return fail("Failed to create binding set for descriptor set ", bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				data.pipelineBindings.Push(mappings);
			}

			Reference<LegacyPipeline> result = new LegacyPipeline(std::move(data));
			result->ReleaseRef();
			return result;
		}

		LegacyPipeline::LegacyPipeline(PipelineData&& data) : m_data(std::move(data)) {
			assert(m_data.device != nullptr);
			assert(m_data.descriptor != nullptr);
			assert(m_data.bindingPool != nullptr);
			for (size_t i = 0u; i < m_data.pipelineBindings.Size(); i++)
				assert(m_data.pipelineBindings[i].bindingSet != nullptr);
		}

		LegacyPipeline::~LegacyPipeline() {}

		void LegacyPipeline::Execute(const InFlightBufferInfo& bufferInfo) {
			auto error = [&](const auto&... message) { m_data.device->Log()->Error("LegacyPipeline::Execute - ", message...); };
			
			const size_t setCount = m_data.descriptor->BindingSetCount();
			for (size_t setIndex = 0u; setIndex < m_data.pipelineBindings.Size(); setIndex++) {
				const BindingSetMappings& mappings = m_data.pipelineBindings[setIndex];
				if (mappings.bindingSetIndex >= setCount) {
					error("Binding set index out of bounds! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				
				const Legacy::PipelineDescriptor::BindingSetDescriptor* bindingSet = m_data.descriptor->BindingSet(mappings.bindingSetIndex);
				if (bindingSet == nullptr) {
					error("Failed to get BindingSetDescriptor! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				
				if (mappings.bindlessStructuredBuffers != nullptr)
					mappings.bindlessStructuredBuffers->BoundObject() = bindingSet->BindlessArrayBuffers();
				else if (mappings.bindlessTextureSamplers != nullptr)
					mappings.bindlessTextureSamplers->BoundObject() = bindingSet->BindlessTextureSamplers();
				else {
					auto refreshBindingMappings = [](const auto& mappingList, const auto& getBoundObject) {
						for (size_t i = 0u; i < mappingList.Size(); i++) {
							const auto& mapping = mappingList[i];
							mapping->BoundObject() = getBoundObject(mapping->BindingIndex());
						}
					};
					refreshBindingMappings(mappings.constantBuffers, [&](size_t index) -> Reference<Buffer> { return bindingSet->ConstantBuffer(index); });
					refreshBindingMappings(mappings.structuredBuffers, [&](size_t index) -> Reference<ArrayBuffer> { return bindingSet->StructuredBuffer(index); });
					refreshBindingMappings(mappings.textureSamplers, [&](size_t index) -> Reference<TextureSampler> { return bindingSet->Sampler(index); });
					refreshBindingMappings(mappings.textureViews, [&](size_t index) -> Reference<TextureView> { return bindingSet->View(index); });
				}

				mappings.bindingSet->Update(bufferInfo.inFlightBufferId);
				mappings.bindingSet->Bind(bufferInfo);
			}
		}



		Reference<LegacyComputePipeline> LegacyComputePipeline::Create(
			GraphicsDevice* device, size_t maxInFlightCommandBuffers,
			const Graphics::Legacy::ComputePipeline::Descriptor* descriptor) {
			if (device == nullptr) return nullptr;
			device->Log()->Warning("LegacyComputePipeline::Create - ",
				"Legacy::ComputePipeline interface is depricated! It is strongly recommended to switch to the new pipeline API. ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");

			auto fail = [&](const auto&... message) {
				device->Log()->Error("LegacyComputePipeline::Create - ", message...);
				return nullptr;
			};

			if (descriptor == nullptr)
				return fail("Descriptor not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			if (device == nullptr)
				return fail("Device not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<Shader> shader = descriptor->ComputeShader();
			if (shader == nullptr)
				return fail("Shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(shader->Binary());
			if (pipeline == nullptr)
				return fail("Failed to create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<LegacyPipeline> bindingSets = LegacyPipeline::Create(device, maxInFlightCommandBuffers, pipeline, descriptor);
			if (bindingSets == nullptr)
				return fail("Failed to create legacy pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<LegacyComputePipeline> result = new LegacyComputePipeline(descriptor, pipeline, bindingSets);
			result->ReleaseRef();
			return result;
		}

		LegacyComputePipeline::LegacyComputePipeline(
			const Graphics::Legacy::ComputePipeline::Descriptor* descriptor,
			Graphics::ComputePipeline* pipeline, LegacyPipeline* bindingSets)
			: m_descriptor(descriptor), m_computePipeline(pipeline), m_bindingSets(bindingSets) {
			assert(m_descriptor != nullptr);
			assert(m_computePipeline != nullptr);
			assert(m_bindingSets != nullptr);
		}

		LegacyComputePipeline::~LegacyComputePipeline() {}

		void LegacyComputePipeline::Execute(const InFlightBufferInfo& bufferInfo) {
			m_bindingSets->Execute(bufferInfo);
			m_computePipeline->Dispatch(bufferInfo.commandBuffer, m_descriptor->NumBlocks());
		}



		Reference<LegacyGraphicsPipeline> LegacyGraphicsPipeline::Create(
			RenderPass* renderPass, size_t maxInFlightCommandBuffers,
			Graphics::Legacy::GraphicsPipeline::Descriptor* descriptor) {
			if (renderPass == nullptr) return nullptr;
			auto fail = [&](const auto&... message) {
				renderPass->Device()->Log()->Error("LegacyGraphicsPipeline::Create - ", message...);
				return nullptr;
			};

			if (descriptor == nullptr)
				return fail("Descriptor not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			if (renderPass == nullptr)
				return fail("Render pass not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<Shader> vertexShader = descriptor->VertexShader();
			if (vertexShader == nullptr)
				return fail("Vertex shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<Shader> fragmentShader = descriptor->FragmentShader();
			if (fragmentShader == nullptr)
				return fail("Fragment shader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			VertexBuffers vertexBuffers;
			const Reference<ResourceBinding<ArrayBuffer>> indexBuffer = Object::Instantiate<ResourceBinding<ArrayBuffer>>();
			const Reference<Graphics::GraphicsPipeline> pipeline = [&]() {
				Graphics::GraphicsPipeline::Descriptor desc = {};
				desc.vertexShader = vertexShader->Binary();
				desc.fragmentShader = fragmentShader->Binary();
				desc.blendMode = descriptor->BlendMode();
				desc.indexType = descriptor->GeometryType();
				auto addVertexInput = [&](
					size_t count, 
					Graphics::GraphicsPipeline::VertexInputInfo::InputRate inputRate,
					const auto& getBuffer) {
					for (size_t i = 0u; i < count; i++) {
						Graphics::Legacy::VertexBuffer* vertexBuffer = getBuffer(i);
						Graphics::GraphicsPipeline::VertexInputInfo info = {};
						info.inputRate = inputRate;
						info.bufferElementSize = vertexBuffer->BufferElemSize();
						for (size_t j = 0u; j < vertexBuffer->AttributeCount(); j++) {
							const Graphics::Legacy::VertexBuffer::AttributeInfo attributeInfo = vertexBuffer->Attribute(j);
							Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo locationInfo = {};
							locationInfo.location = attributeInfo.location;
							locationInfo.bufferElementOffset = attributeInfo.offset;
							info.locations.Push(locationInfo);
						}
						desc.vertexInput.Push(info);
						vertexBuffers.Push(Object::Instantiate<ResourceBinding<ArrayBuffer>>());
					}
				};
				addVertexInput(descriptor->VertexBufferCount(),
					Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX,
					[&](size_t i) { return descriptor->VertexBuffer(i); });
				addVertexInput(descriptor->InstanceBufferCount(),
					Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE,
					[&](size_t i) { return descriptor->InstanceBuffer(i); });

				return renderPass->GetGraphicsPipeline(desc);
			}();
			if (pipeline == nullptr)
				return fail("Failed to create graphics pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<LegacyPipeline> bindingSets = LegacyPipeline::Create(renderPass->Device(), maxInFlightCommandBuffers, pipeline, descriptor);
			if (bindingSets == nullptr)
				return fail("Failed to create legacy pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<Graphics::VertexInput> vertexInput = [&]() {
				static thread_local std::vector<const ResourceBinding<ArrayBuffer>*> buffers;
				buffers.clear();
				for (size_t i = 0u; i < vertexBuffers.Size(); i++)
					buffers.push_back(vertexBuffers[i]);
				const Reference<Graphics::VertexInput> result = pipeline->CreateVertexInput(buffers.data(), indexBuffer);
				buffers.clear();
				return result;
			}();
			if (vertexInput == nullptr)
				return fail("Failed to create vertex input! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const Reference<LegacyGraphicsPipeline> result = new LegacyGraphicsPipeline(
				descriptor, pipeline, vertexInput, bindingSets, std::move(vertexBuffers), indexBuffer);
			result->ReleaseRef();
			return result;
		}

		LegacyGraphicsPipeline::LegacyGraphicsPipeline(
			Graphics::Legacy::GraphicsPipeline::Descriptor* descriptor,
			Graphics::GraphicsPipeline* graphicsPipeline,
			Graphics::VertexInput* vertexInput,
			LegacyPipeline* bindingSets,
			VertexBuffers&& vertexBuffers,
			ResourceBinding<ArrayBuffer>* indexBuffer)
			: m_descriptor(descriptor)
			, m_graphicsPipeline(graphicsPipeline)
			, m_vertexInput(vertexInput)
			, m_bindingSets(bindingSets)
			, m_vertexBuffers(std::move(vertexBuffers))
			, m_indexBuffer(indexBuffer) {
			assert(m_descriptor != nullptr);
			assert(m_graphicsPipeline != nullptr);
			assert(m_vertexInput != nullptr);
			assert(m_bindingSets != nullptr);
			assert(m_vertexBuffers.Size() == (m_descriptor->VertexBufferCount() + m_descriptor->InstanceBufferCount()));
			assert(m_indexBuffer != nullptr);
		}

		LegacyGraphicsPipeline::~LegacyGraphicsPipeline() {}

		void LegacyGraphicsPipeline::Execute(const InFlightBufferInfo& bufferInfo) {
			// Update and bind binding sets:
			m_bindingSets->Execute(bufferInfo);

			// Update and bind vertex input:
			{
				size_t vertexInputId = 0u;
				auto updateVertexBufers = [&](size_t count, const auto& getBuffer) {
					for (size_t i = 0u; i < count; i++) {
						m_vertexBuffers[vertexInputId]->BoundObject() = getBuffer(i)->Buffer();
						vertexInputId++;
					}
				};
				updateVertexBufers(m_descriptor->VertexBufferCount(), 
					[&](size_t i) { return m_descriptor->VertexBuffer(i); });
				updateVertexBufers(m_descriptor->InstanceBufferCount(), 
					[&](size_t i) { return m_descriptor->InstanceBuffer(i); });
				m_indexBuffer->BoundObject() = m_descriptor->IndexBuffer();
				m_vertexInput->Bind(bufferInfo.commandBuffer);
			}

			// Execute the pipeline:
			{
				const size_t indexCount = m_descriptor->IndexCount();
				const size_t instanceCount = m_descriptor->InstanceCount();
				const IndirectDrawBufferReference indirectBuffer = m_descriptor->IndirectBuffer();
				if (indirectBuffer == nullptr)
					m_graphicsPipeline->Draw(bufferInfo.commandBuffer, indexCount, instanceCount);
				else m_graphicsPipeline->DrawIndirect(bufferInfo.commandBuffer, indirectBuffer, instanceCount);
			}
		}
	}
	}
}
