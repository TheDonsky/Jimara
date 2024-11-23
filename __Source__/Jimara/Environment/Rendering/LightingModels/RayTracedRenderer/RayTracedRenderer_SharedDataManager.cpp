#include "RayTracedRenderer_Tools.h"
#include "../../TransientImage.h"


namespace Jimara {
	RayTracedRenderer::Tools::SharedData::SharedData(SharedData&& other)noexcept {
		m_primitiveRecordIdBuffer = std::move(other.m_primitiveRecordIdBuffer);
	}

	RayTracedRenderer::Tools::SharedData::~SharedData() {}

	RayTracedRenderer::Tools::SharedData::operator bool()const {
		return
			m_primitiveRecordIdBuffer != nullptr &&
			m_targetColorTexture != nullptr &&
			m_targetDepthTexture != nullptr;
	}

	struct RayTracedRenderer::Tools::SharedDataManager::Helpers {
		struct ManagerData;
	};

	struct RayTracedRenderer::Tools::SharedDataManager::Helpers::ManagerData : public virtual Object {
		Reference<TransientImage> primitiveRecordIdBuffer;
		Reference<Graphics::TextureView> primitiveRecordIdBufferView;
	};

	Reference<RayTracedRenderer::Tools::SharedDataManager> RayTracedRenderer::Tools::SharedDataManager::Create(
		const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers) {
		if (viewport == nullptr || viewport->Context() == nullptr)
			return nullptr;
		if (renderer == nullptr) {
			viewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::SharedDataManager::Create - Renderer not provided! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		Reference<Helpers::ManagerData> data = Object::Instantiate<Helpers::ManagerData>();
		Reference<SharedDataManager> instance = new SharedDataManager(renderer, viewport, layers, data);
		assert(instance->RefCount() == 2u);
		instance->ReleaseRef();
		assert(instance->RefCount() == 1u);
		return instance;
	}


	RayTracedRenderer::Tools::SharedDataManager::SharedDataManager(
		const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers, Object* additionalData)
		: m_renderer(renderer)
		, m_viewport(viewport)
		, m_layerMask(layers)
		, m_additionalData(additionalData) {
		assert(m_renderer != nullptr);
		assert(m_viewport != nullptr);
		assert(m_additionalData != nullptr);
	}

	RayTracedRenderer::Tools::SharedDataManager::~SharedDataManager() {}

	RayTracedRenderer::Tools::SharedData RayTracedRenderer::Tools::SharedDataManager::StartPass(RenderImages* images) {
		Helpers::ManagerData* additionalData = dynamic_cast<Helpers::ManagerData*>(m_additionalData.operator->());
		assert(additionalData != nullptr);
		SharedData data;

		auto fail = [&](const auto&... message) {
			m_viewport->Context()->Log()->Error("RayTracedRenderer::Tools::SharedDataManager::StartPass - ", message...);
			return SharedData();
		};

		// Early exit if resolution is zero:
		const Size2 resolution = (images == nullptr) ? Size2(0u) : images->Resolution();
		if (resolution.x * resolution.y <= 0u)
			return data;

		// Update PrimitiveRecordIdBuffer:
		{
			if (additionalData->primitiveRecordIdBuffer == nullptr ||
				Size2(additionalData->primitiveRecordIdBuffer->Texture()->Size()) != resolution) {
				additionalData->primitiveRecordIdBufferView = nullptr;
				additionalData->primitiveRecordIdBuffer = TransientImage::Get(
					m_viewport->Context()->Graphics()->Device(),
					Graphics::Texture::TextureType::TEXTURE_2D,
					Graphics::Texture::PixelFormat::R32G32B32A32_UINT,
					Size3(resolution, 1u), 1u,
					Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
				if (additionalData->primitiveRecordIdBuffer == nullptr)
					return fail("Failed to obtain transient image for Primitive Record Id Buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				additionalData->primitiveRecordIdBufferView = 
					additionalData->primitiveRecordIdBuffer->Texture()->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
				if (additionalData->primitiveRecordIdBufferView == nullptr)
					return fail("Failed to create view for the Primitive Record Id Buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
			data.m_primitiveRecordIdBuffer = additionalData->primitiveRecordIdBufferView;
		}

		// Update TargetColorTexture:
		{
			RenderImages::Image* image = images->GetImage(RenderImages::MainColor());
			data.m_targetColorTexture = image->Resolve();
		}

		// Update TargetDepthTexture:
		{
			RenderImages::Image* image = images->GetImage(RenderImages::DepthBuffer());
			data.m_targetDepthTexture = image->Resolve();
		}

		return data;
	}
}
