#include "TonemapperKernel.h"
#include "../../../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	const Object* TonemapperKernel::TypeEnumAttribute() {
		static const Serialization::EnumAttribute<uint8_t> attribute(false,
			"REINHARD_PER_CHANNEL", Type::REINHARD_PER_CHANNEL,
			"REINHARD_LUMINOCITY", Type::REINHARD_LUMINOCITY,
			"ACES", Type::ACES);
		return &attribute;
	}

	Reference<TonemapperKernel> TonemapperKernel::Create(
		Type type,
		Graphics::GraphicsDevice* device,
		Graphics::ShaderLoader* shaderLoader,
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("TonemapperKernel::Create - ", message...);
			return nullptr;
		};

		// Shader class per type:
		static const Reference<const Graphics::ShaderClass>* const SHADER_CLASSES = []() -> const Reference<const Graphics::ShaderClass>*{
			static Reference<const Graphics::ShaderClass> shaderClasses[static_cast<uint32_t>(Type::TYPE_COUNT)];
			static const OS::Path commonPath = "Jimara/Environment/Rendering/PostFX/Tonemapper";
			shaderClasses[static_cast<uint32_t>(Type::REINHARD_PER_CHANNEL)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_PerChannel");
			shaderClasses[static_cast<uint32_t>(Type::REINHARD_LUMINOCITY)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_Luminocity");
			shaderClasses[static_cast<uint32_t>(Type::ACES)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_ACES");
			return shaderClasses;
		}();
		if (type >= Type::TYPE_COUNT)
			return fail("Invalid type(", static_cast<uint32_t>(type), ") provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Graphics::ShaderClass* shaderClass = SHADER_CLASSES[static_cast<uint8_t>(type)];
		if (shaderClass == nullptr)
			return fail("[internal error] Shader path not found for the type(", static_cast<uint32_t>(type), ")! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Settings buffer:
		Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settings;
		static const MemoryBlock* DEFAULT_SETTINGS_PER_TYPE = []() -> const MemoryBlock* {
			static Stacktor<MemoryBlock, static_cast<uint32_t>(Type::TYPE_COUNT)> settingsPerType;
			for (size_t i = 0u; i < static_cast<uint32_t>(Type::TYPE_COUNT); i++)
				settingsPerType.Push(MemoryBlock(nullptr, 0u, nullptr));
			{
				static const ReinhardPerChannelSettings DEFAULT_SETTINGS;
				settingsPerType[static_cast<uint32_t>(Type::REINHARD_PER_CHANNEL)] = 
					MemoryBlock(&DEFAULT_SETTINGS, sizeof(ReinhardPerChannelSettings), nullptr);
			}
			{
				static const ReinhardLuminocitySettings DEFAULT_SETTINGS;
				settingsPerType[static_cast<uint32_t>(Type::REINHARD_LUMINOCITY)] =
					MemoryBlock(&DEFAULT_SETTINGS, sizeof(ReinhardLuminocitySettings), nullptr);
			}
			return settingsPerType.Data();
		}();
		auto findSettingsBuffer = [&](const auto& info) -> const Graphics::ResourceBinding<Graphics::Buffer>* {
			if (info.name != "settings") return nullptr;
			if (settings != nullptr)
				return settings;
			const MemoryBlock& block = DEFAULT_SETTINGS_PER_TYPE[static_cast<uint32_t>(type)];
			if (block.Data() == nullptr)
				return fail("[internal error] Settings buffer configuration not found for type (", static_cast<uint32_t>(type), ")! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::Buffer> settingsBuffer = device->CreateConstantBuffer(block.Size());
			if (settingsBuffer == nullptr)
				return fail("Failed to allocate settings buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");
			std::memcpy(settingsBuffer->Map(), block.Data(), block.Size());
			settingsBuffer->Unmap(true);
			settings = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingsBuffer);
			return settings;
		};

		// Source & target textures:
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> target =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		auto findViewBinding = [&](const auto& info) {
			return
				(info.name == "source") ? target :
				(info.name == "result") ? target :
				nullptr;
		};


		// Kernel creation:
		Graphics::BindingSet::BindingSearchFunctions bindings;
		bindings.constantBuffer = &findSettingsBuffer;
		bindings.textureView = &findViewBinding;
		const Reference<SimpleComputeKernel> kernel = SimpleComputeKernel::Create(
			device, shaderLoader, maxInFlightCommandBuffers, shaderClass, bindings);
		if (kernel == nullptr)
			return fail("Failed to create SimpleComputeKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<TonemapperKernel> result = new TonemapperKernel(
			type, kernel, (settings == nullptr) ? nullptr : settings->BoundObject(), target);
		result->ReleaseRef();
		return result;
	}

	TonemapperKernel::TonemapperKernel(Type type, SimpleComputeKernel* kernel, Graphics::Buffer* settings, Graphics::ResourceBinding<Graphics::TextureView>* target)
		: m_type(type), m_kernel(kernel), m_settingsBuffer(settings), m_target(target) {
		assert(m_kernel != nullptr);
		assert(m_target != nullptr);
	}

	TonemapperKernel::~TonemapperKernel() {}

	TonemapperKernel::Type TonemapperKernel::Algorithm()const {
		return m_type;
	}

	Graphics::Buffer* TonemapperKernel::Settings()const {
		return m_settingsBuffer;
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
