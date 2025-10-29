#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::RayTracedPass::Helpers {
		inline static bool CreateRTPipeline(RayTracedPass* self) {
			SceneContext* const context = self->m_sharedBindings->viewport->Context();
			ShaderLibrary* const shaderLibrary = context->Graphics()->Configuration().ShaderLibrary();

			auto fail = [&](const auto... message) {
				self->m_pipelineBindings.Clear();
				self->m_materialIndex.clear();
				self->m_materialByIndex.clear();
				self->m_pipeline = nullptr;
				context->Log()->Error("RayTracedRenderer::Tools::RayTracedPass::Helpers::CreateRTPipeline - ", message...);
				return false;
			};

			self->m_pipelineBindings.Clear();

			self->m_materialIndex.clear();
			self->m_materialByIndex.clear();
			for (size_t i = 0u; i < shaderLibrary->LitShaders()->Size(); i++) {
				const Reference<const Material::LitShader> litShader = shaderLibrary->LitShaders()->At(i);
				if (litShader == nullptr)
					continue;
				self->m_materialIndex[litShader] = static_cast<uint32_t>(self->m_materialByIndex.size());
				self->m_materialByIndex.push_back(litShader);
			}

			Graphics::RayTracingPipeline::Descriptor pipelineDesc = {};
			pipelineDesc.raygenShader = shaderLibrary->LoadLitShader(LIGHTING_MODEL_PATH, RAY_GEN_STAGE_NAME, nullptr, Graphics::PipelineStage::RAY_GENERATION);
			if (pipelineDesc.raygenShader == nullptr)
				return fail("Failed to get Ray-Gen shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			for (size_t i = 0u; i < self->m_materialByIndex.size(); i++) {
				const Material::LitShader* const litShader = self->m_materialByIndex[i];
				const Reference<Graphics::SPIRV_Binary> shadeFragment_call = shaderLibrary->LoadLitShader(
					LIGHTING_MODEL_PATH, SHADE_FRAGMENT_CALL_NAME, litShader, Graphics::PipelineStage::CALLABLE);
				if (shadeFragment_call == nullptr)
					return fail("Failed to load 'Shade-Fragment' callable shader for '", litShader->LitShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				pipelineDesc.callableShaders.push_back(shadeFragment_call);
			}

			self->m_pipeline = context->Graphics()->Device()->CreateRayTracingPipeline(pipelineDesc);
			if (self->m_pipeline == nullptr)
				return fail("Failed to create pipeline!");
			else return true;
		}

		inline static bool CreateBindings(RayTracedPass* self) {
			self->m_pipelineBindings.Clear();
			if (self->m_pipeline == nullptr)
				return false;

			Graphics::BindingSet::BindingSearchFunctions search = {};
			const auto findTextureView = [&](const Graphics::BindingSet::BindingDescriptor& binding) 
				-> Reference<const Graphics::ResourceBinding<Graphics::TextureView>> {
				if (binding.name == PRIMITIVE_RECORD_ID_BINDING_NAME)
					return self->m_primitiveRecordIdBinding;
				else if (binding.name == FRAME_COLOR_BINDING_NAME)
					return self->m_frameColorBinding;
				else return nullptr;
			};
			search.textureView = &findTextureView;

			for (size_t i = 0u; i < self->m_pipeline->BindingSetCount(); i++) {
				const Reference<Graphics::BindingSet> set = self->m_sharedBindings->CreateBindingSet(self->m_pipeline, i, search);
				if (set == nullptr) {
					self->m_sharedBindings->viewport->Context()->Log()->Error(
						"RayTracedRenderer::Tools::RayTracedPass::Helpers::CreateBindings - ",
						"Failed to create binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					self->m_pipelineBindings.Clear();
					return false;
				}
				self->m_pipelineBindings.Push(set);
			}

			return true;
		}
	};

	RayTracedRenderer::Tools::RayTracedPass::RayTracedPass(const SharedBindings* sharedBindings, GraphicsObjectAccelerationStructure* accelerationStructure)
		: m_sharedBindings(sharedBindings)
		, m_accelerationStructure(accelerationStructure) {
		assert(m_sharedBindings != nullptr);
		assert(m_accelerationStructure != nullptr);

		if (!Helpers::CreateRTPipeline(this))
			return;

		if (!Helpers::CreateBindings(this))
			return;
	}

	RayTracedRenderer::Tools::RayTracedPass::~RayTracedPass() {}

	Reference<RayTracedRenderer::Tools::RayTracedPass> RayTracedRenderer::Tools::RayTracedPass::Create(
		const RayTracedRenderer* renderer,
		const SharedBindings* sharedBindings,
		LayerMask layers) {
		sharedBindings->viewport->Context()->Log()->Warning(__FILE__, ": ", __LINE__, " Not yet implemented!");
		
		GraphicsObjectAccelerationStructure::Descriptor accelerationStructureDescriptor = {};
		{
			// __TODO__: We need a different viewport that is around the view-origin, like some of the lights have.
			accelerationStructureDescriptor.descriptorSet = GraphicsObjectDescriptor::Set::GetInstance(sharedBindings->viewport->Context());
			accelerationStructureDescriptor.frustrumDescriptor = sharedBindings->viewport;
			accelerationStructureDescriptor.layers = layers;
		}
		const Reference<GraphicsObjectAccelerationStructure> accelerationStructure = GraphicsObjectAccelerationStructure::GetFor(accelerationStructureDescriptor);
		if (accelerationStructure == nullptr) {
			sharedBindings->viewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::RayTracedPass::Create - "
				"Failed to get GraphicsObjectAccelerationStructure! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		// Create RT-pass instance:
		const Reference<RayTracedPass> rtPass = new RayTracedPass(sharedBindings, accelerationStructure);
		rtPass->ReleaseRef();
		return rtPass;
	}

	bool RayTracedRenderer::Tools::RayTracedPass::SetFrameBuffers(const FrameBuffers& frameBuffers) {
		m_primitiveRecordIdBinding->BoundObject() = frameBuffers.primitiveRecordId;
		m_frameColorBinding->BoundObject() = frameBuffers.colorTexture;
		return true;
	}

	bool RayTracedRenderer::Tools::RayTracedPass::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		// Validate the pipeline and it's input:
		if (m_pipeline == nullptr ||
			m_pipelineBindings.Size() <= 0u ||
			m_primitiveRecordIdBinding->BoundObject() == nullptr ||
			m_frameColorBinding->BoundObject() == nullptr)
			return false;

		// Update pipeline bindings:
		for (size_t i = 0u; i < m_pipelineBindings.Size(); i++)
			m_pipelineBindings[i]->Update(commandBufferInfo);

		// Set bindings:
		for (size_t i = 0u; i < m_pipelineBindings.Size(); i++)
			m_pipelineBindings[i]->Bind(commandBufferInfo);

		// Execute pipeline:
		m_pipeline->TraceRays(commandBufferInfo, m_frameColorBinding->BoundObject()->TargetTexture()->Size());

		// Done:
		return true;
	}

	uint32_t RayTracedRenderer::Tools::RayTracedPass::MaterialIndex(const Material::LitShader* litShader)const {
		const auto it = m_materialIndex.find(litShader);
		if (it == m_materialIndex.end())
			return JM_RT_FLAG_MATERIAL_NOT_IN_RT_PIPELINE;
		else return it->second;
	}

	void RayTracedRenderer::Tools::RayTracedPass::GetDependencies(Callback<JobSystem::Job*> report) {
		// __TODO__: Implement this crap!
	}
}
