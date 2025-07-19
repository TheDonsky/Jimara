#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	RayTracedRenderer::Tools::FrameBufferManager::FrameBufferManager(SceneContext* context) 
		: m_context(context) {
		assert(m_context != nullptr);
	}

	RayTracedRenderer::Tools::FrameBufferManager::~FrameBufferManager() {}

	struct RayTracedRenderer::Tools::FrameBufferManager::Lock::Helpers {
		inline static void InitializeLock(Lock* self, RenderImages* images) {
			if (images == self->m_manager->m_lastRenderImages)
				return;

			auto cleanup = [&]() {
				self->m_manager->m_lastRenderImages = nullptr;
				self->m_manager->m_lastPrimitiveRecordId = nullptr;
				self->m_manager->m_buffers.primitiveRecordId = nullptr;
				self->m_manager->m_buffers.colorTexture = nullptr;
				self->m_manager->m_buffers.depthBuffer = nullptr;
			};
			cleanup();

			const Size2 resolution = images->Resolution();
			if ((resolution.x * resolution.y) <= 0u)
				return;

			auto fail = [&](const auto... message) -> void {
				cleanup();
				self->m_manager->m_context->Log()->Error(TypeId::Of<Lock>().Name(), "::Lock - ", message...);
				};

			RenderImages::Image* colorTexture = images->GetImage(RenderImages::MainColor());
			if (colorTexture != nullptr)
				self->m_manager->m_buffers.colorTexture = USE_HARDWARE_MULTISAMPLING ? colorTexture->Multisampled() : colorTexture->Resolve();
			if (self->m_manager->m_buffers.colorTexture == nullptr) {
				fail("Could not obtain color texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}

			RenderImages::Image* depthTexture = images->GetImage(RenderImages::DepthBuffer());
			if (depthTexture != nullptr)
				self->m_manager->m_buffers.depthBuffer = USE_HARDWARE_MULTISAMPLING ? depthTexture->Multisampled() : depthTexture->Resolve();
			if (depthTexture == nullptr || (depthTexture->Resolve() == nullptr)) {
				fail("Could not obtain depth texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}

			self->m_manager->m_lastPrimitiveRecordId = TransientImage::Get(
				self->m_manager->m_context->Graphics()->Device(),
				Graphics::Texture::TextureType::TEXTURE_2D,
				PRIMITIVE_RECORD_ID_FORMAT,
				Size3(resolution, 1u), 1u,
				self->m_manager->m_buffers.colorTexture->TargetTexture()->SampleCount());
			const Reference<Graphics::Texture> primitiveRecordIdTex =
				(self->m_manager->m_lastPrimitiveRecordId == nullptr) ? nullptr
				: self->m_manager->m_lastPrimitiveRecordId->Texture();
			if (primitiveRecordIdTex != nullptr)
				self->m_manager->m_buffers.primitiveRecordId = primitiveRecordIdTex->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (self->m_manager->m_buffers.primitiveRecordId == nullptr) {
				fail("Could not obtain transient image for primitive record Id buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}

			self->m_manager->m_lastRenderImages = images;
		}
	};

	RayTracedRenderer::Tools::FrameBufferManager::Lock::Lock(FrameBufferManager* manager, RenderImages* images) 
		: m_manager(manager)
		, m_lock(manager->m_lock) {
		Helpers::InitializeLock(this, images);
	}

	RayTracedRenderer::Tools::FrameBufferManager::Lock::~Lock() {}

	bool RayTracedRenderer::Tools::FrameBufferManager::Lock::Good()const {
		return
			m_manager->m_buffers.primitiveRecordId != nullptr &&
			m_manager->m_buffers.colorTexture != nullptr &&
			m_manager->m_buffers.depthBuffer != nullptr;
	}
}
