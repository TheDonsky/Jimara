#include "ObjectIdRenderer.h"


namespace Jimara {
	Reference<ObjectIdRenderer> ObjectIdRenderer::Instance() {
		static ObjectIdRenderer model;
		return &model;
	}

	Reference<Scene::GraphicsContext::Renderer> ObjectIdRenderer::CreateRenderer(const ViewportDescriptor* viewport) {
		viewport->Context()->Log()->Error("ObjectIdRenderer::CreateRenderer - Not yet implemented!");
		return nullptr;
	}
}
