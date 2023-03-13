#include "ImageOverlayRenderer.h"



namespace Jimara {
	struct ImageOverlayRenderer::Helpers {
		struct SettingsBuffer {
			alignas(8) Vector2 scale = Vector2(1.0f);
			alignas(8) Vector2 offset = Vector2(0.0f);
			alignas(8) Vector2 uvScale = Vector2(1.0f);
			alignas(8) Vector2 uvOffset = Vector2(0.0f);
			alignas(8) Size2 imageSize = Size2(0u);
			alignas(4u) int sampleCount = 0u;
		};
		static_assert(sizeof(SettingsBuffer) == 48u);

		struct PipelineDescriptor 
			: public virtual Graphics::GraphicsPipeline::Descriptor
			, public virtual Graphics::VertexBuffer
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			// Params:
			const Reference<Graphics::Shader> vertexShader;
			const Reference<Graphics::Shader> fragmentShader;
			const Graphics::ArrayBufferReference<Vector4> vertexBuffer;
			const Graphics::ArrayBufferReference<uint32_t> indexBuffer;
			const Graphics::BufferReference<SettingsBuffer> imageInfo;
			const Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> sourceImage;

			// Constructor & destructor:
			inline PipelineDescriptor(
				Graphics::Shader* vertex, Graphics::Shader* fragment,
				Graphics::ArrayBuffer* verts, Graphics::ArrayBuffer* indices,
				Graphics::Buffer* imageInfoBuffer,
				const Graphics::ShaderResourceBindings::TextureSamplerBinding* source)
				: vertexShader(vertex), fragmentShader(fragment)
				, vertexBuffer(verts), indexBuffer(indices)
				, imageInfo(imageInfoBuffer), sourceImage(source) {
				assert(vertexShader != nullptr);
				assert(fragmentShader != nullptr);
				assert(vertexBuffer != nullptr);
				assert(indexBuffer != nullptr);
				assert(imageInfo != nullptr);
				assert(sourceImage != nullptr);
			}
			inline virtual ~PipelineDescriptor() {}

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1u; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t)const override { return this; }

			// Graphics::GraphicsPipeline::Descriptor: 
			inline virtual Reference<Graphics::Shader> VertexShader()const override { return vertexShader; }
			inline virtual Reference<Graphics::Shader> FragmentShader()const override { return fragmentShader; }
			inline virtual size_t VertexBufferCount()const override { return 1u; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t) override { return this; }
			inline virtual size_t InstanceBufferCount()const override { return 0u; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t) override { return nullptr; }
			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override { return indexBuffer; }
			inline virtual Graphics::IndirectDrawBufferReference IndirectBuffer() override { return nullptr; }
			inline virtual Graphics::GraphicsPipeline::IndexType GeometryType() override { return Graphics::GraphicsPipeline::IndexType::TRIANGLE; }
			inline virtual size_t IndexCount() override { return indexBuffer->ObjectCount(); }
			inline virtual size_t InstanceCount() override { return 1u; }

			// Graphics::VertexBuffer:
			inline virtual size_t AttributeCount()const override { return 1u; }
			inline virtual Graphics::VertexBuffer::AttributeInfo Attribute(size_t)const override {
				return { Graphics::VertexBuffer::AttributeInfo::Type::FLOAT4, 0u, 0u }; 
			}
			inline virtual size_t BufferElemSize()const override { return sizeof(Vector4); }
			inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return vertexBuffer; }

			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }
			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo ConstantBufferInfo(size_t index)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::FRAGMENT, Graphics::PipelineStage::VERTEX), 0u };
			}
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return imageInfo; }
			inline virtual size_t StructuredBufferCount()const override { return 0u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo StructuredBufferInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }
			inline virtual size_t TextureSamplerCount()const override { return 1u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo TextureSamplerInfo(size_t)const override { 
				return { Graphics::StageMask(Graphics::PipelineStage::FRAGMENT), 1u };
			}
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return sourceImage->BoundObject(); }
			inline virtual size_t TextureViewCount()const { return 0; }
		};

		struct VertexAndIndexBufferCacheEntry : public virtual ObjectCache<Reference<Object>>::StoredObject {
			const Graphics::ArrayBufferReference<Vector4> vertex;
			const Graphics::ArrayBufferReference<uint32_t> index;

			inline VertexAndIndexBufferCacheEntry(Graphics::ArrayBuffer* vert, Graphics::ArrayBuffer* ind)
				: vertex(vert), index(ind) {
				assert(vertex != nullptr);
				assert(index != nullptr);
			}

			struct Cache : public virtual ObjectCache<Reference<Object>> {
				inline static Reference<VertexAndIndexBufferCacheEntry> Get(Graphics::GraphicsDevice* device) {
					static Cache cache;
					return cache.GetCachedOrCreate(device, false, [&]() -> Reference<VertexAndIndexBufferCacheEntry> {
						auto fail = [&](const auto&... message) {
							device->Log()->Error("ImageOverlayRenderer::Helpers::VertexAndIndexBufferCacheEntry::Cache::Get - ", message...);
							return nullptr;
						};
						
						const Graphics::ArrayBufferReference<Vector4> vertex = device->CreateArrayBuffer<Vector4>(4u);
						if (vertex == nullptr)
							return fail("Failed to create vertex buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");

						const Graphics::ArrayBufferReference<uint32_t> index = device->CreateArrayBuffer<uint32_t>(6u);
						if (index == nullptr)
							return fail("Failed to create index buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");

						{
							Vector4* vertexData = vertex.Map();
							vertexData[0] = Vector4(-1.0f, 1.0f, 0.0f, 0.0f);
							vertexData[1] = Vector4(-1.0f, -1.0f, 0.0f, 1.0f);
							vertexData[2] = Vector4(1.0f, -1.0f, 1.0f, 1.0f);
							vertexData[3] = Vector4(1.0f, 1.0f, 1.0f, 0.0f);
							vertex->Unmap(true);
						}

						{
							uint32_t* indexData = index.Map();
							indexData[0] = 0u;
							indexData[1] = 1u;
							indexData[2] = 2u;
							indexData[3] = 0u;
							indexData[4] = 2u;
							indexData[5] = 3u;
							index->Unmap(true);
						}

						return Object::Instantiate<VertexAndIndexBufferCacheEntry>(vertex, index);
						});
				}
			};
		};

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> device;
			const Reference<VertexAndIndexBufferCacheEntry> buffers;
			const Graphics::BufferReference<SettingsBuffer> settings;
			const Reference<Graphics::Shader> vertexShader;
			const Reference<Graphics::Shader> fragmentShader;
			const Reference<Graphics::Shader> fragmentShaderMS;
			const size_t maxInFlightCommandBuffers;

			inline Data(
				Graphics::GraphicsDevice* graphicsDevice,
				VertexAndIndexBufferCacheEntry* cachedBuffers,
				Graphics::Buffer* settingsBuffer,
				Graphics::Shader* vertex,
				Graphics::Shader* fragment,
				Graphics::Shader* fragmentMS,
				size_t maxInFlightBuffers)
				: device(graphicsDevice), buffers(cachedBuffers), settings(settingsBuffer)
				, vertexShader(vertex), fragmentShader(fragment), fragmentShaderMS(fragmentMS)
				, maxInFlightCommandBuffers(Math::Max(size_t(1u), maxInFlightBuffers)) {
				assert(device != nullptr);
				assert(buffers != nullptr);
				assert(settings != nullptr);
				assert(vertexShader != nullptr);
				assert(fragmentShader != nullptr);
				assert(fragmentShaderMS != nullptr);
			}

			SpinLock lock;
			Reference<Graphics::TextureView> targetTexture;
			Reference<Graphics::TextureView> targetResolve;
			const Reference<Graphics::ShaderResourceBindings::TextureSamplerBinding> sourceTexture =
				Object::Instantiate<Graphics::ShaderResourceBindings::TextureSamplerBinding>();
			Rect sourceRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			Rect targetRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			bool settingsDirty = true;

			Reference<Graphics::RenderPass> renderPass;
			Reference<Graphics::FrameBuffer> frameBuffer;
			Reference<Graphics::Pipeline> pipeline;
		};

		inline static Data& State(const ImageOverlayRenderer* self) { return *dynamic_cast<Data*>(self->m_data.operator->()); }
	};

	Reference<ImageOverlayRenderer> ImageOverlayRenderer::Create(
		Graphics::GraphicsDevice* device, 
		Graphics::ShaderLoader* shaderLoader, 
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("ImageOverlayRenderer::Create - ", message...);
			return nullptr;
		};
		
		if (shaderLoader == nullptr)
			return fail("Shader loader was not provided! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Helpers::VertexAndIndexBufferCacheEntry> buffers = Helpers::VertexAndIndexBufferCacheEntry::Cache::Get(device);
		if (buffers == nullptr)
			return fail("Could not create vertex & index buffers! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<Helpers::SettingsBuffer> settings = device->CreateConstantBuffer<Helpers::SettingsBuffer>();
		if (settings == nullptr)
			return fail("Could not create settings buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		if (shaderSet == nullptr)
			return fail("Could not retrieve shader set! [File:", __FILE__, "; Line: ", __LINE__, "]");

		static const OS::Path PROJECT_PATH = "Jimara/Environment/Rendering/Helpers/ImageOverlay";
		static const Graphics::ShaderClass SHADER_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer");
		static const Graphics::ShaderClass SHADER_CLASS_MS(PROJECT_PATH / "Jimara_ImageOverlayRendererMS");
		
		const Reference<Graphics::Shader> vertex = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::VERTEX);
		if (vertex == nullptr) 
			return fail("Could not load vertex shader! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::Shader> fragment = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::FRAGMENT);
		if (fragment == nullptr)
			return fail("Could not load fragment shader! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::Shader> fragmentMS = shaderSet->GetShaderModule(&SHADER_CLASS_MS, Graphics::PipelineStage::FRAGMENT);
		if (fragmentMS == nullptr)
			return fail("Could not load multisampled fragment shader! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Helpers::Data> data = Object::Instantiate<Helpers::Data>(
			device, buffers, settings, vertex, fragment, fragmentMS, maxInFlightCommandBuffers);
		const Reference<ImageOverlayRenderer> renderer = new ImageOverlayRenderer(data);
		renderer->ReleaseRef();
		return renderer;
	}

	ImageOverlayRenderer::~ImageOverlayRenderer() {}

	void ImageOverlayRenderer::SetSourceRegion(const Rect& region) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		if (data.sourceRegion.start == region.start && data.sourceRegion.end == region.end) return;
		data.sourceRegion = region;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::SetSource(Graphics::TextureSampler* sampler) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		data.settingsDirty = true;

		if (data.sourceTexture->BoundObject() == nullptr || sampler == nullptr ||
			((data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1) !=
				(sampler->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1)))
			data.pipeline = nullptr;

		data.sourceTexture->BoundObject() = sampler;
	}

	void ImageOverlayRenderer::SetTargetRegion(const Rect& region) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		if (data.targetRegion.start == region.start && data.targetRegion.end == region.end) return;
		data.targetRegion = region;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::SetTarget(Graphics::TextureView* target, Graphics::TextureView* targetResolve) {
		Helpers::Data& data = Helpers::State(this);
		if (targetResolve != nullptr && targetResolve->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1) {
			data.device->Log()->Error(
				"ImageOverlayRenderer::SetTarget - Resolve attachment has sample count of 1; It will be ignored! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			targetResolve = nullptr;
		}
		std::unique_lock<SpinLock> lock(data.lock);
		data.settingsDirty = true;

		if (data.targetTexture != target || data.targetResolve != targetResolve)
			data.frameBuffer = nullptr;

		if (data.targetTexture == nullptr || target == nullptr ||
			data.targetTexture->TargetTexture()->ImageFormat() != target->TargetTexture()->ImageFormat() ||
			data.targetTexture->TargetTexture()->SampleCount() != target->TargetTexture()->SampleCount() ||
			(targetResolve != nullptr) != (data.targetResolve != nullptr)) {
			data.renderPass = nullptr;
			data.pipeline = nullptr;
		}
	}

	void ImageOverlayRenderer::Execute(Graphics::Pipeline::CommandBufferInfo commandBuffer) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);

		// Make sure the images are valid (ei not null and with non-zero sizes)
		{
			auto pixelCount = [](Graphics::TextureView* view) -> size_t {
				if (view == nullptr) return 0u;
				Size2 size = view->TargetTexture()->Size();
				return size.x * size.y;
			};
			if (data.sourceTexture->BoundObject() == nullptr || 
				pixelCount(data.sourceTexture->BoundObject()->TargetView()) <= 0u || 
				pixelCount(data.targetTexture) <= 0u) return;
		}

		// (Re)Create render pass:
		if (data.renderPass == nullptr) {
			data.frameBuffer = nullptr;
			data.pipeline = nullptr;
			const Graphics::Texture::PixelFormat format = data.targetTexture->TargetTexture()->ImageFormat();
			data.renderPass = data.device->CreateRenderPass(
				data.targetTexture->TargetTexture()->SampleCount(), 1u,
				&format, Graphics::Texture::PixelFormat::OTHER,
				data.targetResolve == nullptr ? Graphics::RenderPass::Flags::NONE : Graphics::RenderPass::Flags::RESOLVE_COLOR);
			if (data.renderPass == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to create render pass!",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// (Re)Create frame buffer:
		if (data.frameBuffer == nullptr) {
			data.frameBuffer = data.renderPass->CreateFrameBuffer(&data.targetTexture, nullptr, &data.targetResolve, nullptr);
			if (data.frameBuffer == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to create frame buffer!",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// (Re)Create graphics pipeline:
		if (data.pipeline == nullptr) {
			const bool isMultisampled = 
				data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1;
			const Reference<Helpers::PipelineDescriptor> pipelineDescriptor = Object::Instantiate<Helpers::PipelineDescriptor>(
				data.vertexShader, isMultisampled ? data.fragmentShaderMS : data.fragmentShader,
				data.buffers->vertex, data.buffers->index, data.settings, data.sourceTexture);
			data.pipeline = data.renderPass->CreateGraphicsPipeline(pipelineDescriptor, data.maxInFlightCommandBuffers);
			if (data.pipeline == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to create graphics pipeline!",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Update settings:
		if (data.settingsDirty) {
			Helpers::SettingsBuffer& settings = data.settings.Map();
			settings.scale = data.sourceRegion.Size();
			settings.offset = data.sourceRegion.start * 2.0f;
			settings.uvScale = data.targetRegion.Size();
			settings.uvOffset = data.targetRegion.start;
			settings.imageSize = data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->Size();
			settings.sampleCount = (int)data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount();
			data.settings->Unmap(true);
			data.settingsDirty = false;
		}

		// Execute graphics pipeline:
		{
			data.renderPass->BeginPass(commandBuffer.commandBuffer, data.frameBuffer, nullptr);
			data.pipeline->Execute(commandBuffer);
			data.renderPass->EndPass(commandBuffer.commandBuffer);
		}
	}
}
