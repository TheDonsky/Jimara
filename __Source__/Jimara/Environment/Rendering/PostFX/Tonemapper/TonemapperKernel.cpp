#include "TonemapperKernel.h"
#include "../../../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	const Object* TonemapperKernel::TypeEnumAttribute() {
		static const Serialization::EnumAttribute<uint8_t> attribute(false,
			"REINHARD", Type::REINHARD,
			"REINHARD_LUMA", Type::REINHARD_LUMA);
		return &attribute;
	}

	Reference<TonemapperKernel> TonemapperKernel::Create(
		Type type,
		Graphics::GraphicsDevice* device,
		Graphics::ShaderLoader* shaderLoader,
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;

		static const Reference<const Graphics::ShaderClass>* const SHADER_CLASSES = []() -> const Reference<const Graphics::ShaderClass>*{
			static Reference<const Graphics::ShaderClass> shaderClasses[static_cast<uint32_t>(Type::TYPE_COUNT)];
			static const OS::Path commonPath = "Jimara/Environment/Rendering/PostFX/Tonemapper";
			shaderClasses[static_cast<uint32_t>(Type::REINHARD)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_PerChannel");
			shaderClasses[static_cast<uint32_t>(Type::REINHARD_LUMA)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_Luma");
			return shaderClasses;
		}();
		if (type >= Type::TYPE_COUNT) {
			device->Log()->Error(
				"TonemapperKernel::Create - Invalid type(", static_cast<uint32_t>(type), ") provided! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Graphics::ShaderClass* shaderClass = SHADER_CLASSES[static_cast<uint8_t>(type)];
		if (shaderClass == nullptr) {
			device->Log()->Error(
				"TonemapperKernel::Create - [internal error] Shader path not found for the type(", static_cast<uint32_t>(type), ")! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

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

		const Reference<SimpleComputeKernel> kernel = SimpleComputeKernel::Create(
			device, shaderLoader, maxInFlightCommandBuffers, shaderClass, bindings);
		if (kernel == nullptr) {
			device->Log()->Error(
				"TonemapperKernel::Create - Failed to create SimpleComputeKernel! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<TonemapperKernel> result = new TonemapperKernel(type, kernel, target);
		result->ReleaseRef();
		return result;
	}

	TonemapperKernel::TonemapperKernel(Type type, SimpleComputeKernel* kernel, Graphics::ResourceBinding<Graphics::TextureView>* target)
		: m_type(type), m_kernel(kernel), m_target(target) {
		assert(m_kernel != nullptr);
		assert(m_target != nullptr);
	}

	TonemapperKernel::~TonemapperKernel() {}

	TonemapperKernel::Type TonemapperKernel::Algorithm()const {
		return m_type;
	}

	Graphics::TextureView* TonemapperKernel::Target()const {
		return m_target->BoundObject();
	}

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
