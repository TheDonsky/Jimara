#include "ForwardLightingModel_OIT_Pass.h"
#include "../Utilities/GraphicsObjectPipelines.h"
#include "../../SceneObjects/Lights/LightmapperJobs.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"
#include "../../SceneObjects/Lights/SceneLightGrid.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../../../Graphics/Memory/TransientBufferSet.h"


namespace Jimara {
	struct ForwardLightingModel_OIT_Pass::Helpers {
		static const constexpr Size3 WORKGROUP_SIZE = Math::MakeSize3(16u, 16u, 1u);

		struct SettingsBuffer {
			alignas(8) Size2 frameBufferSize = Size2(0u);
			alignas(4) uint32_t fragsPerPixel = 0u;
			alignas(4) float transmittanceBias = 0.0f; // 0 for transparent (alpha blended); 1 for additive

			alignas(16) Matrix4 view = Math::Identity();
			alignas(16) Matrix4 projection = Math::Identity();
			alignas(16) Matrix4 viewPose = Math::Identity();
		};

		struct PixelState {
			alignas(4) uint32_t lock = 0u;
			alignas(4) uint32_t fragmentCount = 0u;
		};

		struct FragmentInfo {
			alignas(4) float depth = 0.0f;
			alignas(4) uint32_t packedRG = 0u; // Color is stored premultiplied
			alignas(4) uint32_t packedBA = 0u; // Instead of alpha, we store transmittance (1 - a) for transparent and 1 for additive
		};

		typedef Stacktor<Reference<Graphics::BindingSet>, 4u> PassBindingSets;

		struct OIT_Buffers {
			Reference<Graphics::TransientBufferSet> transientBuffers;
			Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> pixelDataBinding;
			Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> fragmentDataBinding;

			inline bool Initialize(Graphics::GraphicsDevice* device) {
				transientBuffers = Graphics::TransientBufferSet::Get(device);
				pixelDataBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				fragmentDataBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				return (transientBuffers != nullptr);
			}
		};

		struct LightBuffers {
			Reference<SceneLightGrid> lightGrid;
			Reference<LightDataBuffer> lightDataBuffer;
			Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> lightDataBinding;
			Reference<LightTypeIdBuffer> lightTypeIdBuffer;
			Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> lightTypeIdBinding;

			bool Initialize(const ViewportDescriptor* viewport) {
				auto fail = [&](const auto&... message) {
					viewport->Context()->Log()->Error("ForwardLightingModel_IOT_Pass::Helpers::LightBuffers::Initialize - ", message...);
					return false;
				};

				lightGrid = SceneLightGrid::GetFor(viewport);
				if (lightGrid == nullptr)
					return fail("Failed to get scene light grid pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				
				lightDataBuffer = LightDataBuffer::Instance(viewport);
				if (lightDataBuffer == nullptr)
					return fail("Failed to get light data buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				lightDataBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

				lightTypeIdBuffer = LightTypeIdBuffer::Instance(viewport);
				if (lightTypeIdBuffer == nullptr)
					return fail("Failed to get light type id buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				lightTypeIdBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

				return true;
			}
		};

		struct FrameBuffer {
			Reference<RenderImages> images;
			Reference<Graphics::FrameBuffer> frameBuffer;
			Reference<RenderImages::Image> colorImage;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> colorTexture;
			Reference<RenderImages::Image> depthImage;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> depthTexture;

			inline FrameBuffer() 
				: colorTexture(Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>())
				, depthTexture(Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>()) {}

			inline FrameBuffer(const FrameBuffer&) = default;
		};

		inline static Reference<Graphics::BindingSet> AllocateBindingSet(
			Graphics::Pipeline* pipeline, Graphics::BindingPool* bindingPool, 
			const OIT_Buffers& oitBuffers, const FrameBuffer& frameBuffer, const Graphics::ResourceBinding<Graphics::Buffer>* settingsBuffer) {
			Graphics::BindingSet::Descriptor desc = {};
			desc.pipeline = pipeline;
			desc.bindingSetId = 0u;
			auto findSettingsBuffer = [&](const auto&) { return settingsBuffer; };
			desc.find.constantBuffer = &findSettingsBuffer;
			auto findIOTBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
				if (info.name == "resultBufferPixels")
					return oitBuffers.pixelDataBinding;
				else if (info.name == "fragmentData")
					return oitBuffers.fragmentDataBinding;
				else return nullptr;
				};
			desc.find.structuredBuffer = &findIOTBuffer;
			auto findTextures = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::TextureView>> {
				return
					(info.name == "colorAttachment") ? frameBuffer.colorTexture :
					(info.name == "depthAttachment") ? frameBuffer.depthTexture : nullptr;
				};
			desc.find.textureView = &findTextures;
			return bindingPool->AllocateBindingSet(desc);
		}

		struct ComputePipelineWithInput {
			Reference<Graphics::ComputePipeline> pipeline;
			Reference<Graphics::BindingSet> input;

			inline bool Initialize(
				SceneContext* context, const std::string_view& shaderPath, Graphics::BindingPool* bindingPool,
				const OIT_Buffers& oitBuffers, const FrameBuffer& frameBuffer, const Graphics::ResourceBinding<Graphics::Buffer>* settingsBuffer) {
				auto fail = [&](const auto&... message) {
					context->Log()->Error("ForwardLightingModel_IOT_Pass::Helpers::ComputePipelineWithInput::Initialize - ", message...);
					return false;
				};

				const Reference<Graphics::SPIRV_Binary> binary = 
					context->Graphics()->Configuration().ShaderLibrary()->LoadShader(shaderPath);
				if (binary == nullptr)
					return fail("Failed to load shader binary for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				pipeline = context->Graphics()->Device()->GetComputePipeline(binary);
				if (pipeline == nullptr)
					return fail("Failed to get compute pipeline for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				input = AllocateBindingSet(pipeline, bindingPool, oitBuffers, frameBuffer, settingsBuffer);
				if (input == nullptr)
					return fail("Failed to create binding set for for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return true;
			}
		};

		struct FullScreenPipelineWithInput {
			Reference<Graphics::GraphicsPipeline> pipeline;
			Reference<Graphics::VertexInput> vertexInput;
			Reference<Graphics::BindingSet> input;

			inline bool Initialize(
				SceneContext* context, Graphics::RenderPass* renderPass, const std::string_view& shaderPath, Graphics::BindingPool* bindingPool,
				const OIT_Buffers& oitBuffers, const FrameBuffer& frameBuffer, const Graphics::ResourceBinding<Graphics::Buffer>* settingsBuffer) {
				auto fail = [&](const auto&... message) {
					context->Log()->Error("ForwardLightingModel_IOT_Pass::Helpers::FullScreenPipelineWithInput::Initialize - ", message...);
					return false;
					};

				const std::string vertexShaderPath = std::string(shaderPath) + ".vert";
				const Reference<Graphics::SPIRV_Binary> vertex =
					context->Graphics()->Configuration().ShaderLibrary()->LoadShader(vertexShaderPath);
				if (vertex == nullptr)
					return fail("Failed to load vertex shader binary for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const std::string fragmentShaderPath = std::string(shaderPath) + ".frag";
				const Reference<Graphics::SPIRV_Binary> fragment = 
					context->Graphics()->Configuration().ShaderLibrary()->LoadShader(fragmentShaderPath);
				if (fragment == nullptr)
					return fail("Failed to load fragment shader binary for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				Graphics::GraphicsPipeline::Descriptor desc = {};
				desc.vertexShader = vertex;
				desc.fragmentShader = fragment;
				pipeline = renderPass->GetGraphicsPipeline(desc);
				if (pipeline == nullptr)
					return fail("Failed to get compute pipeline for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Graphics::ArrayBufferReference<uint16_t> indexBuffer = context->Graphics()->Device()->CreateArrayBuffer<uint16_t>(6u);
				if (indexBuffer == nullptr)
					return fail("Failed to allocate index buffer for vertex input! [File:", __FILE__, "; Line: ", __LINE__, "]");
				else {
					uint16_t* indices = indexBuffer.Map();
					for (size_t i = 0u; i < indexBuffer->ObjectCount(); i++)
						indices[i] = static_cast<uint16_t>(i);
					indexBuffer->Unmap(true);
				}
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBufferBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(indexBuffer);
				vertexInput = pipeline->CreateVertexInput(nullptr, indexBufferBinding);
				if (vertexInput == nullptr)
					return fail("Failed to create vertex input for the pipeline! [File:", __FILE__, "; Line: ", __LINE__, "]");

				input = AllocateBindingSet(pipeline, bindingPool, oitBuffers, frameBuffer, settingsBuffer);
				if (input == nullptr)
					return fail("Failed to create binding set for for ", shaderPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return true;
			}
		};

		struct OIT_PassPipelines {
			Graphics::BufferReference<SettingsBuffer> settingsBuffer;
			Reference<const GraphicsObjectPipelines> pipelines;
			Stacktor<Reference<Graphics::BindingSet>, 4u> bindingSets;

			inline Reference<const Graphics::ResourceBinding<Graphics::Buffer>> Initialize(
				const ViewportDescriptor* viewport, GraphicsObjectDescriptor::Set* graphicsObjects, 
				Graphics::RenderPass* renderPass, Graphics::BindingPool* bindingPool,
				const LayerMask& layers, GraphicsObjectPipelines::Flags flags, 
				const OIT_Buffers& oitBuffers, const Graphics::BindingSet::BindingSearchFunctions& externalBindings) {
				auto fail = [&](const auto&... message) {
					viewport->Context()->Log()->Error("ForwardLightingModel_IOT_Pass::Helpers::OIT_PassPipelines::Initialize - ", message...);
					return nullptr;
				};

				settingsBuffer = viewport->Context()->Graphics()->Device()->CreateConstantBuffer<SettingsBuffer>();
				if (settingsBuffer == nullptr)
					return fail("Failed to create settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingsBuffer);

				pipelines = [&]() -> Reference<const GraphicsObjectPipelines> {
					GraphicsObjectPipelines::Descriptor desc = {};
					desc.descriptorSet = graphicsObjects;
					desc.frustrumDescriptor = viewport;
					desc.renderPass = renderPass;
					desc.layers = layers;
					desc.flags = flags;
					desc.pipelineFlags = Graphics::GraphicsPipeline::Flags::NONE;
					desc.lightingModel = viewport->Context()->Graphics()->Device()->PhysicalDevice()->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK) ? 
						OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer_OIT_Pass_Interlocked.jlm") :
						OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer_OIT_Pass_SpinLock.jlm");
					desc.lightingModelStage = "OIT_Pass";
					return GraphicsObjectPipelines::Get(desc);
				}();
				if (pipelines == nullptr)
					return fail("Failed to get pipeline set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				bindingSets.Clear();
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = pipelines->EnvironmentPipeline();
				desc.find = externalBindings;

				auto findSettingsBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::Buffer>> {
					return (info.name == "jimara_ForwardRenderer_ViewportBuffer") ? settingsBinding :
						externalBindings.constantBuffer(info);
				};
				desc.find.constantBuffer = &findSettingsBuffer;

				auto findStructuredBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
					return
						(info.name == "jimara_ForwardRenderer_ResultBufferPixels") 
							? Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>>(oitBuffers.pixelDataBinding) :
						(info.name == "jimara_ForwardRenderer_FragmentData") 
							? Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>>(oitBuffers.fragmentDataBinding) :
						externalBindings.structuredBuffer(info);
				};
				desc.find.structuredBuffer = &findStructuredBuffer;

				for (size_t i = 0u; i < pipelines->EnvironmentPipeline()->BindingSetCount(); i++) {
					desc.bindingSetId = i;
					const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
					if (set == nullptr)
						return fail("Failed to get pipeline set ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else bindingSets.Push(set);
				}

				return settingsBinding;
			}
		};

		class Renderer : public virtual RenderStack::Renderer {
		private:
			const Reference<const ForwardLightingModel_OIT_Pass> m_pass;

			const Reference<const LightmapperJobs> m_lightmapperJobs;
			const Reference<GraphicsSimulation::JobDependencies> m_graphicsSimulation;

			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<Graphics::RenderPass> m_renderPass;
			const Reference<Graphics::BindingPool> m_bindingPool;

			const LightBuffers m_lightBuffers;
			const OIT_Buffers m_iotBuffers;
			FrameBuffer m_frameBuffer;

			ComputePipelineWithInput m_clearPipeline;
			OIT_PassPipelines m_alphaBlendedPipelines;
			OIT_PassPipelines m_additivePipelines;
			FullScreenPipelineWithInput m_blitDepthPipeline;

			bool UpdateLightBuffers() {
				auto fail = [&](const auto&... message) {
					m_viewport->Context()->Log()->Error("ForwardLightingModel_IOT_Pass::Helpers::Renderer::UpdateLightBuffers - ", message...);
					return false;
				};

				m_lightBuffers.lightDataBinding->BoundObject() = m_lightBuffers.lightDataBuffer->Buffer();
				if (m_lightBuffers.lightDataBinding->BoundObject() == nullptr)
					return fail("Light data could not be retrieved! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				m_lightBuffers.lightTypeIdBinding->BoundObject() = m_lightBuffers.lightTypeIdBuffer->Buffer();
				if (m_lightBuffers.lightDataBinding->BoundObject() == nullptr)
					return fail("Light type id buffer could not be retrieved! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				return true;
			}

			bool UpdatePerPixelSamples(RenderImages* images, uint32_t samplesPerPixel) {
				const Size2 resolution = (images == nullptr) ? Size2(0u) : images->Resolution();
				const size_t pixelCount = size_t(resolution.x) * size_t(resolution.y);
				m_iotBuffers.pixelDataBinding->BoundObject() = m_iotBuffers.transientBuffers->GetBuffer(
					sizeof(PixelState) * pixelCount, 
					Graphics::TransientBufferSet::RecursionDepth());
				m_iotBuffers.fragmentDataBinding->BoundObject() = m_iotBuffers.transientBuffers->GetBuffer(
					sizeof(FragmentInfo) * pixelCount * samplesPerPixel,
					Graphics::TransientBufferSet::RecursionDepth() + 1u);
				if ((m_iotBuffers.pixelDataBinding->BoundObject() != nullptr) && (m_iotBuffers.fragmentDataBinding->BoundObject() != nullptr))
					return true;
				m_viewport->Context()->Log()->Error(
					"ForwardLightingModel_IOT_Pass::Helpers::Renderer::UpdatePerPixelSamples - Failed to get/allocate transient buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			bool UpdateFrameBuffer(RenderImages* images) {
				if (m_frameBuffer.images == images && m_frameBuffer.frameBuffer != nullptr)
					return true;
				if (images == nullptr) {
					m_frameBuffer.images = nullptr;
					m_frameBuffer.frameBuffer = nullptr;
					m_frameBuffer.colorTexture->BoundObject() = nullptr;
					m_frameBuffer.depthTexture->BoundObject() = nullptr;
					return false;
				}
				
				m_frameBuffer.colorImage = images->GetImage(RenderImages::MainColor());
				m_frameBuffer.depthImage = images->GetImage(RenderImages::DepthBuffer());
				if (m_frameBuffer.colorImage == nullptr || m_frameBuffer.depthImage == nullptr) {
					m_viewport->Context()->Log()->Error(
						"ForwardLightingModel_IOT_Pass::Helpers::Renderer::UpdateFrameBuffer - Failed to get required images! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				m_frameBuffer.colorTexture->BoundObject() = m_frameBuffer.colorImage->Resolve();
				m_frameBuffer.depthTexture->BoundObject() = m_frameBuffer.depthImage->Resolve();
				
				m_frameBuffer.frameBuffer = m_renderPass->CreateFrameBuffer(nullptr, m_frameBuffer.depthTexture->BoundObject(), nullptr, nullptr);
				if (m_frameBuffer.frameBuffer == nullptr)
					m_viewport->Context()->Log()->Error(
						"ForwardLightingModel_IOT_Pass::Helpers::Renderer::UpdateFrameBuffer - Failed to create frame buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else m_frameBuffer.images = images;
				return m_frameBuffer.frameBuffer != nullptr;
			}

			void UpdateSettingsBuffers(RenderImages* images, uint32_t samplesPerPixel) {
				SettingsBuffer settings = {};
				{
					settings.frameBufferSize = (images == nullptr) ? Size2(0u) : images->Resolution();
					settings.fragsPerPixel = samplesPerPixel;
					settings.view = m_viewport->ViewMatrix();
					settings.projection = m_viewport->ProjectionMatrix();
					settings.viewPose = Math::Inverse(settings.view);
				}
				{
					settings.transmittanceBias = 0.0f;
					m_alphaBlendedPipelines.settingsBuffer.Map() = settings;
					m_alphaBlendedPipelines.settingsBuffer->Unmap(true);
				}
				{
					settings.transmittanceBias = 1.0f;
					m_additivePipelines.settingsBuffer.Map() = settings;
					m_additivePipelines.settingsBuffer->Unmap(true);
				}
			}

		public:
			inline Renderer(
				const ForwardLightingModel_OIT_Pass* pass,

				const LightmapperJobs* lightmapperJobs,
				GraphicsSimulation::JobDependencies* graphicsSimulation,
				
				const ViewportDescriptor* viewport,
				Graphics::RenderPass* renderPass,
				Graphics::BindingPool* bindingPool,

				const LightBuffers lightBuffers,
				const OIT_Buffers iotBuffers,
				FrameBuffer frameBuffer,

				ComputePipelineWithInput clearPipeline,
				OIT_PassPipelines alphaBlendedPipelines,
				OIT_PassPipelines additivePipelines,
				FullScreenPipelineWithInput blitDepthPipeline)
				: m_pass(pass)

				, m_lightmapperJobs(lightmapperJobs)
				, m_graphicsSimulation(graphicsSimulation)
				
				, m_viewport(viewport)
				, m_renderPass(renderPass)
				, m_bindingPool(bindingPool)
				
				, m_lightBuffers(lightBuffers)
				, m_iotBuffers(iotBuffers)
				, m_frameBuffer(frameBuffer)
				
				, m_clearPipeline(clearPipeline)
				, m_alphaBlendedPipelines(alphaBlendedPipelines)
				, m_additivePipelines(additivePipelines)
				, m_blitDepthPipeline(blitDepthPipeline) {}

			inline virtual ~Renderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				const uint32_t samplesPerPixel = m_pass->SamplesPerPixel();

				if (!UpdateLightBuffers())
					return;

				if (!UpdatePerPixelSamples(images, samplesPerPixel))
					return;
				
				if (!UpdateFrameBuffer(images)) 
					return;

				UpdateSettingsBuffers(images, samplesPerPixel);
				m_bindingPool->UpdateAllBindingSets(commandBufferInfo);

				const Size3 workgroupCount = (Size3(images->Resolution(), 1u) + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;

				// Execute clear pipeline:
				{
					m_clearPipeline.input->Bind(commandBufferInfo);
					m_clearPipeline.pipeline->Dispatch(commandBufferInfo, workgroupCount);
				}

				// Draw stuff:
				{
					m_renderPass->BeginPass(commandBufferInfo, m_frameBuffer.frameBuffer, nullptr);
					auto drawObjects = [&](const OIT_PassPipelines& pipelines) {
						const GraphicsObjectPipelines::Reader reader(*pipelines.pipelines);
						for (size_t i = 0u; i < pipelines.bindingSets.Size(); i++)
							pipelines.bindingSets[i]->Bind(commandBufferInfo);
						for (size_t i = 0u; i < reader.Count(); i++)
							reader[i].ExecutePipeline(commandBufferInfo);
					};
					drawObjects(m_alphaBlendedPipelines);
					drawObjects(m_additivePipelines);
					m_renderPass->EndPass(commandBufferInfo);
				}

				// Execute blit pipeline:
				{
					m_renderPass->BeginPass(commandBufferInfo, m_frameBuffer.frameBuffer, nullptr);
					m_blitDepthPipeline.input->Bind(commandBufferInfo);
					m_blitDepthPipeline.vertexInput->Bind(commandBufferInfo);
					m_blitDepthPipeline.pipeline->Draw(commandBufferInfo, 6u, 1u);
					m_renderPass->EndPass(commandBufferInfo);
				}
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				m_alphaBlendedPipelines.pipelines->GetUpdateTasks(report);
				m_additivePipelines.pipelines->GetUpdateTasks(report);
				report(m_lightBuffers.lightDataBuffer);
				report(m_lightBuffers.lightTypeIdBuffer);
				report(m_lightBuffers.lightGrid->UpdateJob());
				m_lightmapperJobs->GetAll(report);
				m_graphicsSimulation->CollectDependencies(report);
			}
		};
	};

	const ForwardLightingModel_OIT_Pass* ForwardLightingModel_OIT_Pass::Instance() {
		static const ForwardLightingModel_OIT_Pass instance;
		return &instance;
	}

	Reference<RenderStack::Renderer> ForwardLightingModel_OIT_Pass::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		if (viewport == nullptr || viewport->Context() == nullptr) return nullptr;
		auto fail = [&](const auto&... message) { 
			viewport->Context()->Log()->Error("ForwardLightingModel_IOT_Pass::CreateRenderer - ", message...); 
			return nullptr; 
		};

		const Reference<GraphicsObjectDescriptor::Set> graphicsObjects = GraphicsObjectDescriptor::Set::GetInstance(viewport->Context());
		if (graphicsObjects == nullptr)
			return fail("Failed to retrieve GraphicsObjectDescriptor::Set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::RenderPass> renderPass = viewport->Context()->Graphics()->Device()->GetRenderPass(
			Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 0u, nullptr, RenderImages::DepthBuffer()->Format(), flags);
		if (renderPass == nullptr)
			return fail("Could not create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Helpers::FrameBuffer frameBuffer;
		Helpers::OIT_Buffers oitBuffers;
		if (!oitBuffers.Initialize(viewport->Context()->Graphics()->Device()))
			return fail("Failed to initialize OIT buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(
			viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Helpers::LightBuffers lightBuffers;
		if (!lightBuffers.Initialize(viewport))
			return fail("Failed to get scene lighting information! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Graphics::BindingSet::BindingSearchFunctions lightGridBindings = lightBuffers.lightGrid->BindingDescriptor();

		Graphics::BindingSet::BindingSearchFunctions externalBindings = lightGridBindings;
		auto searchStructuredBuffers = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
			return
				(info.name == "jimara_ForwardRenderer_LightTypeIds") 
					? Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>>(lightBuffers.lightTypeIdBinding) :
				(info.name == "jimara_LightDataBinding") 
					? Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>>(lightBuffers.lightDataBinding) :
				lightGridBindings.structuredBuffer(info);
		};
		externalBindings.structuredBuffer = &searchStructuredBuffers;
		
		const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> bindlessBuffers =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(viewport->Context()->Graphics()->Bindless().BufferBinding());
		auto findBindlessBuffers = [&](const auto&) { return bindlessBuffers; };
		externalBindings.bindlessStructuredBuffers = &findBindlessBuffers;

		const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> bindlessTextures =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(viewport->Context()->Graphics()->Bindless().SamplerBinding());
		auto findBindlessSamplers = [&](const auto&) { return bindlessTextures; };
		externalBindings.bindlessTextureSamplers = &findBindlessSamplers;

		Helpers::OIT_PassPipelines alphaBlendedPipelines;
		const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBuffer = alphaBlendedPipelines.Initialize(
			viewport, graphicsObjects, renderPass, bindingPool, layers,
			GraphicsObjectPipelines::Flags::EXCLUDE_OPAQUE_OBJECTS | GraphicsObjectPipelines::Flags::EXCLUDE_NON_OPAQUE_OBJECTS ^
			GraphicsObjectPipelines::Flags::EXCLUDE_ALPHA_BLENDED_OBJECTS, oitBuffers, externalBindings);
		if (settingsBuffer == nullptr)
			return fail("Failed to create alpha-blended pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Helpers::OIT_PassPipelines additivePipelines;
		if (additivePipelines.Initialize(
			viewport, graphicsObjects, renderPass, bindingPool, layers,
			GraphicsObjectPipelines::Flags::EXCLUDE_OPAQUE_OBJECTS | GraphicsObjectPipelines::Flags::EXCLUDE_NON_OPAQUE_OBJECTS ^
			GraphicsObjectPipelines::Flags::EXCLUDE_ADDITIVELY_BLENDED_OBJECTS, oitBuffers, externalBindings) == nullptr)
			return fail("Failed to create additive pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Helpers::ComputePipelineWithInput clearPipeline;
		{
			if (!clearPipeline.Initialize(viewport->Context(), 
				"Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer_OIT_Clear.comp",
				bindingPool, oitBuffers, frameBuffer, settingsBuffer))
				return fail("Failed to create clear pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		Helpers::FullScreenPipelineWithInput blitDepthPipeline;
		{
			if (!blitDepthPipeline.Initialize(viewport->Context(), renderPass, 
				"Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer_OIT_Blit",
				bindingPool, oitBuffers, frameBuffer, settingsBuffer))
				return fail("Failed to create blit depth pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		const Reference<LightmapperJobs> lightmapperJobs = LightmapperJobs::GetInstance(viewport->Context());
		if (lightmapperJobs == nullptr)
			return fail("Failed to get lightmapper jobs! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<GraphicsSimulation::JobDependencies> simulationJobs = GraphicsSimulation::JobDependencies::For(viewport->Context());
		if (simulationJobs == nullptr)
			return fail("Failed to get simulation job dependencies! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::Renderer>(this,
			lightmapperJobs, simulationJobs,
			viewport, renderPass, bindingPool,
			lightBuffers, oitBuffers, frameBuffer,
			clearPipeline, alphaBlendedPipelines, additivePipelines, blitDepthPipeline);
	}

	void ForwardLightingModel_OIT_Pass::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(SamplesPerPixel, SetSamplesPerPixel, "Samples Per Pixel",
				"Transparent sample transparent count per pixel \n"
				"Notes: \n"
				"    0. Higher the better, but at the expense of VRAM and performance; \n"
				"    1. On a per-pixel basis, if actual fragment count exceeds SamplesPerPixel(), collective transparency will be approximated; \n"
				"    2. This pass DOES NOT SUPPORT hardware multisampling; do not confuse this parameter with that.",
				Object::Instantiate<Serialization::SliderAttribute<uint32_t>>(1u, 32u));
		};
	}
}
