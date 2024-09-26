#include "HDRISkyboxRenderer.h"
#include "../../../Math/Frustrum.h"


namespace Jimara {
	struct HDRISkyboxRenderer::Helpers {
		struct Settings {
			alignas(16) Vector4 color = Vector4(1.0f);
		};

		struct Vertex {
			Vector2 position = {};
			Vector3 direction = {};
		};

		using InFlightVertexBuffers = Stacktor<Graphics::ArrayBufferReference<Vertex>, 4u>;

		class Implementation : public virtual HDRISkyboxRenderer {
		private:
			// Constants:
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<Graphics::SPIRV_Binary> m_vertexShader;
			const Reference<Graphics::SPIRV_Binary> m_fragmentShader;
			const Reference<Graphics::BindingPool> m_bindingPool;
			
			// Buffers:
			const Graphics::BufferReference<Settings> m_settingsBuffer;
			const InFlightVertexBuffers m_vertexBuffers;
			const Graphics::ArrayBufferReference<uint32_t> m_indexBuffer;
			
			// Texture:
			Vector4 m_baseColor = Vector4(1.0f);
			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_defaultTextureBinding;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_hdriBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();

			// Pipeline:
			Reference<RenderImages> m_renderImages;
			Reference<Graphics::RenderPass> m_renderPass;
			Reference<Graphics::FrameBuffer> m_frameBuffer;
			Reference<Graphics::GraphicsPipeline> m_pipeline;
			Stacktor<Reference<Graphics::VertexInput>, 4u> m_vertexInputs;
			Reference<Graphics::BindingSet> m_bindingSet;

			friend class HDRISkyboxRenderer;


			inline bool UpdateRenderImages(RenderImages* images) {
				if (m_renderImages == images)
					return true;

				auto cleanup = [&]() {
					m_renderImages = nullptr;
					m_renderPass = nullptr;
					m_frameBuffer = nullptr;
					m_pipeline = nullptr;
					m_vertexInputs.Clear();
					m_bindingSet = nullptr;
				};
				cleanup();

				auto fail = [&](const auto&... message) {
					cleanup();
					m_viewport->Context()->Log()->Error("HDRISkyboxRenderer::Helpers::Implementation::UpdateRenderImages - ", message...);
					return false;
				};

				m_renderImages = images;
				if (m_renderImages == nullptr)
					return false;

				// Update render pass:
				{
					const Graphics::Texture::PixelFormat format = RenderImages::MainColor()->Format();
					m_renderPass = m_viewport->Context()->Graphics()->Device()->GetRenderPass(
						m_renderImages->SampleCount(),
						1u, &format, RenderImages::DepthBuffer()->Format(),
						Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH |
						Graphics::RenderPass::Flags::RESOLVE_COLOR | Graphics::RenderPass::Flags::RESOLVE_DEPTH);
					if (m_renderPass == nullptr)
						return fail("Failed to get/create render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Update frame buffer:
				{
					const RenderImages::Image* const color = m_renderImages->GetImage(RenderImages::MainColor());
					if (color == nullptr)
						return fail("Failed to get color texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					Reference<Graphics::TextureView> colorAttachment = color->Multisampled();
					Reference<Graphics::TextureView> colorResolve = color->Resolve();
					const RenderImages::Image* const depth = m_renderImages->GetImage(RenderImages::DepthBuffer());
					if (depth == nullptr)
						return fail("Failed to get depth buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					m_frameBuffer = m_renderPass->CreateFrameBuffer(&colorAttachment, depth->Multisampled(), & colorResolve, depth->Resolve());
					if (m_frameBuffer == nullptr)
						return fail("Failed to create frame buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Update pipeline:
				{
					Graphics::GraphicsPipeline::Descriptor desc = {};
					desc.vertexShader = m_vertexShader;
					desc.fragmentShader = m_fragmentShader;
					desc.vertexInput.Resize(1u);
					{
						Graphics::GraphicsPipeline::VertexInputInfo& info = desc.vertexInput[0u];
						info.bufferElementSize = sizeof(Vertex);
						info.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("position", offsetof(Vertex, position)));
						info.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("direction", offsetof(Vertex, direction)));
					}
					m_pipeline = m_renderPass->GetGraphicsPipeline(desc);
					if (m_pipeline == nullptr)
						return fail("Failed to create/get graphics pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Update vertex input:
				{
					const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBuffer =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(m_indexBuffer);
					for (size_t i = 0u; i < m_vertexBuffers.Size(); i++) {
						const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> vertexBuffer =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(m_vertexBuffers[i]);
						const Graphics::ResourceBinding<Graphics::ArrayBuffer>* const vertexBufferPtr = vertexBuffer;
						const Reference<Graphics::VertexInput> vertexInput = m_pipeline->CreateVertexInput(&vertexBufferPtr, indexBuffer);
						if (vertexInput == nullptr)
							return fail("Failed to create vertex input for in-flight buffer ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_vertexInputs.Push(vertexInput);
					}
				}

				// Update binding set:
				{
					if (m_pipeline->BindingSetCount() != 1u)
						return fail("Graphics pipeline expected to have exactly one binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					Graphics::BindingSet::Descriptor desc = {};
					desc.pipeline = m_pipeline;
					desc.bindingSetId = 0u;
					const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBufferBinding =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(m_settingsBuffer);
					auto findConstantBuffer = [&](const auto&) { return settingsBufferBinding; };
					desc.find.constantBuffer = &findConstantBuffer;
					auto findTextureBuffer = [&](const auto&) { return m_hdriBinding; };
					desc.find.textureSampler = &findTextureBuffer;
					m_bindingSet = m_bindingPool->AllocateBindingSet(desc);
					if (m_bindingSet == nullptr)
						return fail("Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				return true;
			}

			inline void UpdateInput(const Graphics::InFlightBufferInfo& commandBufferInfo) {
				{
					m_settingsBuffer.Map().color = m_baseColor;
					m_settingsBuffer->Unmap(true);
				}
				const FrustrumShape shape(m_viewport->ViewMatrix(), m_viewport->ProjectionMatrix());
				const Graphics::ArrayBufferReference<Vertex>& buffer = m_vertexBuffers[commandBufferInfo];
				auto getVertex = [&](const Vector2& screenPos) -> Vertex {
					Vertex vert;
					vert.position = screenPos;
					vert.direction = Math::Normalize(
						shape.ClipToWorldSpace(Vector3(screenPos, 1.0f)) - shape.ClipToWorldSpace(Vector3(screenPos, 0.0f)));
					return vert;
				};
				{
					Vertex* vertex = buffer.Map();
					vertex[0] = getVertex(Vector2(-1.0f, -1.0f));
					vertex[1] = getVertex(Vector2(1.0f, -1.0f));
					vertex[2] = getVertex(Vector2(1.0f, 1.0f));
					vertex[3] = getVertex(Vector2(-1.0f, 1.0f));
					buffer->Unmap(true);
				}
				m_bindingSet->Update(commandBufferInfo);
			};

			inline void Draw(const Graphics::InFlightBufferInfo& commandBufferInfo) {
				static const Vector4 clearColor = Vector4(0.0f);
				m_renderPass->BeginPass(commandBufferInfo, m_frameBuffer, &clearColor);
				m_bindingSet->Bind(commandBufferInfo);
				m_vertexInputs[commandBufferInfo]->Bind(commandBufferInfo);
				m_pipeline->Draw(commandBufferInfo, m_indexBuffer->ObjectCount(), 1u);
				m_renderPass->EndPass(commandBufferInfo);
			}

		public:
			inline Implementation(
				const ViewportDescriptor* viewport,
				Graphics::SPIRV_Binary* vertexShader,
				Graphics::SPIRV_Binary* fragmentShader,
				Graphics::BindingPool* bindingPool,
				Graphics::Buffer* settingsBuffer,
				const InFlightVertexBuffers& vertexBuffers,
				Graphics::ArrayBuffer* indexBuffer,
				const Graphics::ResourceBinding<Graphics::TextureSampler>* defaultTextureBinding)
				: m_viewport(viewport)
				, m_vertexShader(vertexShader)
				, m_fragmentShader(fragmentShader)
				, m_bindingPool(bindingPool)
				, m_settingsBuffer(settingsBuffer)
				, m_vertexBuffers(vertexBuffers)
				, m_indexBuffer(indexBuffer)
				, m_defaultTextureBinding(defaultTextureBinding) {
				m_hdriBinding->BoundObject() = m_defaultTextureBinding->BoundObject();
			}

			inline virtual ~Implementation() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				if (!UpdateRenderImages(images))
					return;
				UpdateInput(commandBufferInfo);
				Draw(commandBufferInfo);
			}
		};
	};
	
	Reference<HDRISkyboxRenderer> HDRISkyboxRenderer::Create(const ViewportDescriptor* viewport) {
		if (viewport == nullptr)
			return nullptr;
		auto fail = [&](const auto&... message) {
			viewport->Context()->Log()->Error("HDRISkyboxRenderer::Create - ", message...);
			return nullptr;
		};

		static const std::string SHADER_PATH = "Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRISkyboxRenderer";
		static const std::string VERTEX_SHADER_PATH = SHADER_PATH + ".vert";
		static const std::string FRAGMENT_SHADER_PATH = SHADER_PATH + ".frag";
		const Reference<Graphics::SPIRV_Binary> vertexShader = 
			viewport->Context()->Graphics()->Configuration().ShaderLibrary()->LoadShader(VERTEX_SHADER_PATH);
		if (vertexShader == nullptr)
			return fail("Failed to load vertex shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::SPIRV_Binary> fragmentShader =
			viewport->Context()->Graphics()->Configuration().ShaderLibrary()->LoadShader(FRAGMENT_SHADER_PATH);
		if (fragmentShader == nullptr)
			return fail("Failed to load fragment shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(
			viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<Helpers::Settings> settingsBuffer =
			viewport->Context()->Graphics()->Device()->CreateConstantBuffer<Helpers::Settings>();
		if (settingsBuffer == nullptr)
			return fail("Failed to create settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Helpers::InFlightVertexBuffers vertexBuffers;
		for (size_t i = 0u; i < viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(); i++) {
			const Graphics::ArrayBufferReference<Helpers::Vertex> buffer = viewport->Context()->Graphics()->Device()
				->CreateArrayBuffer<Helpers::Vertex>(4u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
			if (buffer == nullptr)
				return fail("Failed to create in-flight vertex buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			vertexBuffers.Push(buffer);
		}

		const Graphics::ArrayBufferReference<uint32_t> indexBuffer = viewport->Context()->Graphics()->Device()
			->CreateArrayBuffer<uint32_t>(6u, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
		if (indexBuffer == nullptr)
			return fail("Failed to create index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		else {
			uint32_t* index = indexBuffer.Map();
			index[0u] = 0u;
			index[1u] = 1u;
			index[2u] = 2u;
			index[3u] = 0u;
			index[4u] = 2u;
			index[5u] = 3u;
			indexBuffer->Unmap(true);
		}

		const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> defaultTextureBinding =
			Graphics::SharedTextureSamplerBinding(Vector4(1.0f), viewport->Context()->Graphics()->Device());
		if (defaultTextureBinding == nullptr)
			return fail("Failed to retrieve fallback texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::Implementation>(
			viewport,
			vertexShader,
			fragmentShader,
			bindingPool,
			settingsBuffer,
			vertexBuffers,
			indexBuffer,
			defaultTextureBinding);
	}

	HDRISkyboxRenderer::~HDRISkyboxRenderer() {}

	void HDRISkyboxRenderer::SetEnvironmentMap(Graphics::TextureSampler* hdriSampler) {
		Helpers::Implementation* self = dynamic_cast<Helpers::Implementation*>(this);
		self->m_hdriBinding->BoundObject() = (hdriSampler != nullptr)
			? hdriSampler : self->m_defaultTextureBinding->BoundObject();
	}

	void HDRISkyboxRenderer::SetColorMultiplier(const Vector4& color) {
		dynamic_cast<Helpers::Implementation*>(this)->m_baseColor = color;
	}
}
