#include "LinearToSRGB.h"
#include "../../SimpleComputeKernel.h"

namespace Jimara {
	struct LinearToSrgbKernel::Helpers {
		class Implementation final : public virtual LinearToSrgbKernel {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<SimpleComputeKernel> m_kernel;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_sourceBinding;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_resultBinding;
			std::mutex m_lock;
			Reference<RenderImages> m_renderImages;

			friend class LinearToSrgbKernel;

		public:
			inline Implementation(
				Graphics::GraphicsDevice* device,
				SimpleComputeKernel* kernel,
				Graphics::ResourceBinding<Graphics::TextureView>* sourceBinding,
				Graphics::ResourceBinding<Graphics::TextureView>* resultBinding) 
				: m_device(device)
				, m_kernel(kernel)
				, m_sourceBinding(sourceBinding)
				, m_resultBinding(resultBinding) {
				assert(m_device != nullptr);
				assert(m_sourceBinding != nullptr);
				assert(m_resultBinding != nullptr);
				SetCategory(~uint32_t(0u));
				SetPriority(0u);
			}

			inline virtual ~Implementation() {}

			inline void ExecutePipeline(const Graphics::InFlightBufferInfo& commandBuffer) {
				if (m_sourceBinding->BoundObject() == nullptr || m_resultBinding->BoundObject() == nullptr)
					return;
				static const constexpr uint32_t BLOCK_SIZE = 16u;
				const Size2 resolution = m_sourceBinding->BoundObject()->TargetTexture()->Size();
				m_kernel->Dispatch(commandBuffer, Size3((resolution + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u));
			}

			inline void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_renderImages != images) {
					m_renderImages = images;
					if (images == nullptr) {
						m_sourceBinding->BoundObject() = nullptr;
						m_resultBinding->BoundObject() = nullptr;
					}
					else {
						RenderImages::Image* image = images->GetImage(RenderImages::MainColor());
						m_sourceBinding->BoundObject() = image->Resolve();
						m_resultBinding->BoundObject() = image->Resolve();
					}
				}
				ExecutePipeline(commandBufferInfo);
			}
		};
	};

	Reference<LinearToSrgbKernel> LinearToSrgbKernel::Create(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> sourceBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> resultBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		auto findViewBinding = [&](const auto& info) {
			return
				(info.name == "source") ? sourceBinding :
				(info.name == "result") ? resultBinding :
				nullptr;
		};
		Graphics::BindingSet::BindingSearchFunctions bindings;
		bindings.textureView = &findViewBinding;
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/PostFX/LinearToSRGB/LinearToSRGB");
		const Reference<SimpleComputeKernel> kernel = SimpleComputeKernel::Create(
			device, shaderLoader, maxInFlightCommandBuffers, &SHADER_CLASS, bindings);
		if (kernel == nullptr) {
			device->Log()->Error("LinearToSrgbKernel::Create - Failed to create SimpleComputeKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		else return Object::Instantiate<Helpers::Implementation>(device, kernel, sourceBinding, resultBinding);
	}

	void LinearToSrgbKernel::Execute(
		Graphics::TextureView* source, Graphics::TextureView* result, const Graphics::InFlightBufferInfo& commandBuffer) {
		Helpers::Implementation* self = dynamic_cast<Helpers::Implementation*>(this);
		std::unique_lock<std::mutex> lock(self->m_lock);
		self->m_sourceBinding->BoundObject() = source;
		self->m_resultBinding->BoundObject() = result;
		if (source != nullptr && result != nullptr && source->TargetTexture()->Size() != result->TargetTexture()->Size())
			self->m_device->Log()->Warning("LinearToSrgbKernel::Execute - Source and result should have the same size! ",
				"Mismatched size will result in undefined behaviour! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		self->m_renderImages = nullptr;
		self->ExecutePipeline(commandBuffer);
	}
}
