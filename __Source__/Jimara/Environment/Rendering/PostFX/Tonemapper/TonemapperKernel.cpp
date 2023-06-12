#include "TonemapperKernel.h"


namespace Jimara {
	Reference<TonemapperKernel> TonemapperKernel::Create(
		Graphics::GraphicsDevice* device,
		Graphics::ShaderLoader* shaderLoader,
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> target =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		auto findViewBinding = [&](const auto& info) {
			return
				(info.name == "source") ? target :
				(info.name == "result") ? target :
				nullptr;
		};
		Graphics::BindingSet::BindingSearchFunctions bindings;
		bindings.textureView = &findViewBinding;
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/PostFX/Tonemapper/Tonemapper_Reinhard");
		const Reference<SimpleComputeKernel> kernel = SimpleComputeKernel::Create(
			device, shaderLoader, maxInFlightCommandBuffers, &SHADER_CLASS, bindings);
		if (kernel == nullptr) {
			device->Log()->Error("TonemapperKernel::Create - Failed to create SimpleComputeKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<TonemapperKernel> result = new TonemapperKernel(kernel, target);
		result->ReleaseRef();
		return result;
	}

	TonemapperKernel::TonemapperKernel(SimpleComputeKernel* kernel, Graphics::ResourceBinding<Graphics::TextureView>* target) 
		: m_kernel(kernel), m_target(target) {
		assert(m_kernel != nullptr);
		assert(m_target != nullptr);
	}

	TonemapperKernel::~TonemapperKernel() {}

	void TonemapperKernel::SetTarget(Graphics::TextureView* target) {
		m_target->BoundObject() = target;
	}

	void TonemapperKernel::Execute(const Graphics::InFlightBufferInfo& commandBuffer) {
		if (m_target->BoundObject() == nullptr)
			return;
		static const constexpr Size3 WORKGROUP_SIZE = Size3(16u, 16u, 1u);
		m_kernel->Dispatch(commandBuffer, (m_target->BoundObject()->TargetTexture()->Size() + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE);
	}
}
