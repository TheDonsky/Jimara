#include "TonemapperKernel.h"
#include "../../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../../../Data/Serialization/Attributes/DragSpeedAttribute.h"


namespace Jimara {
	struct TonemapperKernel::Helpers {
		static Reference<Settings> CreteSettings(Type type, Graphics::GraphicsDevice* device) {
			if (type >= Type::TYPE_COUNT || device == nullptr) 
				return nullptr;
			static const auto fail = [&](Graphics::GraphicsDevice* device, const auto&... message) {
				device->Log()->Error("TonemapperKernel::Helpers::CreteSettings - ", message...);
				return nullptr;
			};

			typedef Settings* (*CreateFn)(Graphics::GraphicsDevice*);
			static const CreateFn* CREATE_FUNCTION = [&]() -> const CreateFn* {
				static CreateFn createFunctions[static_cast<uint32_t>(Type::TYPE_COUNT)];
				for (size_t i = 0u; i < static_cast<uint32_t>(Type::TYPE_COUNT); i++)
					createFunctions[i] = [](Graphics::GraphicsDevice*) -> Settings* { return nullptr; };
				
				createFunctions[static_cast<uint32_t>(Type::REINHARD_PER_CHANNEL)] = 
					createFunctions[static_cast<uint32_t>(Type::REINHARD_LUMINOCITY)] = [](Graphics::GraphicsDevice* device) -> Settings* {
					const Graphics::BufferReference<Vector3> buffer = device->CreateConstantBuffer<Vector3>();
					if (buffer == nullptr)
						return fail(device, "Could not create Reinhard settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return new ReinhardSettings(buffer);
				};
				
				createFunctions[static_cast<uint32_t>(Type::ACES)] =
					createFunctions[static_cast<uint32_t>(Type::ACES_APPROX)] = [](Graphics::GraphicsDevice*)->Settings* {
					return new ACESSettings();
				};

				createFunctions[static_cast<uint32_t>(Type::CUSTOM_CURVE)] = [](Graphics::GraphicsDevice* device)->Settings* {
					const Graphics::BufferReference<Vector3> buffer = device->CreateConstantBuffer<Vector3>();
					if (buffer == nullptr)
						return fail(device, "Could not custom curve settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return new CustomCurveSettings(device, buffer);
				};
				
				return createFunctions;
			}();

			const Reference<Settings> settings = CREATE_FUNCTION[static_cast<size_t>(type)](device);
			if (settings == nullptr)
				return fail(device, "Could not create settings! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			settings->ReleaseRef();
			settings->Apply();
			return settings;
		}

		static Graphics::Buffer* FindBuffer(Type type, Settings* settings, const Graphics::BindingSet::BindingDescriptor& binding) {
			if (type >= Type::TYPE_COUNT)
				return nullptr;
			typedef Graphics::Buffer* (*FindFn)(Settings*, const Graphics::BindingSet::BindingDescriptor&);
			static const FindFn* FIND_FUNCTION = [&]() -> const FindFn* {
				static FindFn findFunctions[static_cast<uint32_t>(Type::TYPE_COUNT)];
				for (size_t i = 0u; i < static_cast<uint32_t>(Type::TYPE_COUNT); i++)
					findFunctions[i] = [](Settings*, const Graphics::BindingSet::BindingDescriptor&) -> Graphics::Buffer* { return nullptr; };

				findFunctions[static_cast<uint32_t>(Type::REINHARD_PER_CHANNEL)] =
					findFunctions[static_cast<uint32_t>(Type::REINHARD_LUMINOCITY)] =
					[](Settings* settings, const Graphics::BindingSet::BindingDescriptor& binding) -> Graphics::Buffer* {
					return binding.name == "settings" ? dynamic_cast<ReinhardSettings*>(settings)->m_settingsBuffer : nullptr;
				};

				findFunctions[static_cast<uint32_t>(Type::CUSTOM_CURVE)] =
					[](Settings* settings, const Graphics::BindingSet::BindingDescriptor& binding) -> Graphics::Buffer* {
					return binding.name == "settings" ? dynamic_cast<CustomCurveSettings*>(settings)->m_settingsBuffer : nullptr;
				};

				return findFunctions;
			}();
			return FIND_FUNCTION[static_cast<size_t>(type)](settings, binding);
		}

		static const Graphics::ResourceBinding<Graphics::ArrayBuffer>* FindStructuredBuffer(
			Type type, Settings* settings, const Graphics::BindingSet::BindingDescriptor& binding) {
			if (type >= Type::TYPE_COUNT)
				return nullptr;
			typedef const Graphics::ResourceBinding<Graphics::ArrayBuffer>* (*FindFn)(Settings*, const Graphics::BindingSet::BindingDescriptor&);
			static const FindFn* FIND_FUNCTION = [&]() -> const FindFn* {
				static FindFn findFunctions[static_cast<uint32_t>(Type::TYPE_COUNT)];
				for (size_t i = 0u; i < static_cast<uint32_t>(Type::TYPE_COUNT); i++)
					findFunctions[i] = [](Settings*, const Graphics::BindingSet::BindingDescriptor&)
					-> const Graphics::ResourceBinding<Graphics::ArrayBuffer>*{ return nullptr; };

				findFunctions[static_cast<uint32_t>(Type::CUSTOM_CURVE)] =
					[](Settings* settings, const Graphics::BindingSet::BindingDescriptor& binding) -> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
					return binding.name == "responseCurve" ? dynamic_cast<CustomCurveSettings*>(settings)->m_responseCurveBinding : nullptr;
				};

				return findFunctions;
			}();
			return FIND_FUNCTION[static_cast<size_t>(type)](settings, binding);
		}
	};

	const Object* TonemapperKernel::TypeEnumAttribute() {
		static const Serialization::EnumAttribute<uint8_t> attribute(false,
			"NONE", Type::NONE,
			"REINHARD_PER_CHANNEL", Type::REINHARD_PER_CHANNEL,
			"REINHARD_LUMINOCITY", Type::REINHARD_LUMINOCITY,
			"ACES", Type::ACES,
			"ACES_APPROX", Type::ACES_APPROX,
			"CUSTOM_CURVE", Type::CUSTOM_CURVE);
		return &attribute;
	}

	Reference<TonemapperKernel> TonemapperKernel::Create(
		Type type,
		Graphics::GraphicsDevice* device,
		ShaderLibrary* shaderLibrary,
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr || type >= Type::TYPE_COUNT) 
			return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("TonemapperKernel::Create - ", message...);
			return nullptr;
		};

		// Create settings
		const Reference<Settings> settings = Helpers::CreteSettings(type, device);
		if (settings == nullptr)
			return fail("Failed to create settings for the TonemapperKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Shader class per type:
		static const Reference<const Graphics::ShaderClass>* const SHADER_CLASSES = []() -> const Reference<const Graphics::ShaderClass>*{
			static Reference<const Graphics::ShaderClass> shaderClasses[static_cast<uint32_t>(Type::TYPE_COUNT)];
			static const OS::Path commonPath = "Jimara/Environment/Rendering/PostFX/Tonemapper";
			shaderClasses[static_cast<uint32_t>(Type::REINHARD_PER_CHANNEL)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_PerChannel");
			shaderClasses[static_cast<uint32_t>(Type::REINHARD_LUMINOCITY)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Reinhard_Luminocity");
			shaderClasses[static_cast<uint32_t>(Type::ACES)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_ACES");
			shaderClasses[static_cast<uint32_t>(Type::ACES_APPROX)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_ACES_Approx");
			shaderClasses[static_cast<uint32_t>(Type::CUSTOM_CURVE)] = Object::Instantiate<Graphics::ShaderClass>(commonPath / "Tonemapper_Custom");
			return shaderClasses;
		}();
		const Graphics::ShaderClass* shaderClass = SHADER_CLASSES[static_cast<uint8_t>(type)];
		if (shaderClass == nullptr)
			return fail("[internal error] Shader path not found for the type(", static_cast<uint32_t>(type), ")! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Configuration buffers:
		Stacktor<Reference<const Graphics::ResourceBinding<Graphics::Buffer>>, 1u> settingsBindings;
		auto findSettingsBuffer = [&](const auto& info) -> const Graphics::ResourceBinding<Graphics::Buffer>* {
			Graphics::Buffer* buffer = Helpers::FindBuffer(type, settings, info);
			if (buffer == nullptr)
				return nullptr;
			for (size_t i = 0u; i < settingsBindings.Size(); i++) {
				const Graphics::ResourceBinding<Graphics::Buffer>* const binding = settingsBindings[i];
				if (binding->BoundObject() == buffer)
					return binding;
			}
			const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> binding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(buffer);
			settingsBindings.Push(binding);
			return binding;
		};
		auto findStructuredBufer = [&](const auto& info) {
			return Helpers::FindStructuredBuffer(type, settings, info);
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
		bindings.structuredBuffer = &findStructuredBufer;
		bindings.textureView = &findViewBinding;
		const Reference<SimpleComputeKernel> kernel = SimpleComputeKernel::Create(
			device, shaderLibrary, maxInFlightCommandBuffers, shaderClass, bindings);
		if (kernel == nullptr)
			return fail("Failed to create SimpleComputeKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<TonemapperKernel> result = new TonemapperKernel(type, settings, kernel, target);
		result->ReleaseRef();
		return result;
	}

	TonemapperKernel::TonemapperKernel(
		Type type,
		Settings* settings,
		SimpleComputeKernel* kernel, 
		Graphics::ResourceBinding<Graphics::TextureView>* target)
		: m_type(type), m_settings(settings), m_kernel(kernel), m_target(target) {
		assert(m_kernel != nullptr);
		assert(m_settings != nullptr);
		assert(m_target != nullptr);
	}

	TonemapperKernel::~TonemapperKernel() {}

	TonemapperKernel::Type TonemapperKernel::Algorithm()const {
		return m_type;
	}

	TonemapperKernel::Settings* TonemapperKernel::Configuration()const {
		return m_settings;
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




	
	void TonemapperKernel::ReinhardSettings::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD(maxWhite, "Max White", "Radiance value to be mapped to 1",
				Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			JIMARA_SERIALIZE_FIELD(maxWhiteTint, "Max White Tint",
				"'Tint' of the max white value; generally, white is recommended, but anyone is free to experiment",
				Object::Instantiate<Serialization::ColorAttribute>());
		};
	}

	void TonemapperKernel::ReinhardSettings::Apply() {
		const float tintLuminocity = std::abs(Math::Dot(maxWhiteTint, Vector3(0.2126f, 0.7152f, 0.0722f)));
		const Vector3 maxWhiteColor = maxWhite * tintLuminocity / maxWhiteTint;
		m_settingsBuffer.Map() = maxWhiteColor;
		m_settingsBuffer->Unmap(true);
	}





	TonemapperKernel::CustomCurveSettings::CustomCurveSettings(Graphics::GraphicsDevice* device, Graphics::Buffer* buffer)
		: responseCurve(device, "Response Curve", "Color response curve", std::vector<Reference<const Object>> { 
		Object::Instantiate<Serialization::CurveGraphCoordinateLimits>(0.0f, 1.0f, 0.0f, 1.0f) })
		, m_settingsBuffer(buffer) {
		assert(m_settingsBuffer != nullptr);
		static const std::map<float, BezierNode<Vector3>> defaultCurve = {
			{ 0.0f, BezierNode<Vector3>(Vector3(0.0f), Vector3(0.75f)) },
			{ 1.0f, BezierNode<Vector3>(Vector3(1.0f)) }
		};
		responseCurve.SetContent(defaultCurve);
	}

	void TonemapperKernel::CustomCurveSettings::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD(maxWhite, "Max White", "Radiance value to be mapped to 1",
				Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			JIMARA_SERIALIZE_FIELD(maxWhiteTint, "Max White Tint",
				"'Tint' of the max white value; generally, white is recommended, but anyone is free to experiment",
				Object::Instantiate<Serialization::ColorAttribute>());
			responseCurve.GetFields(recordElement);
		};
	}

	void TonemapperKernel::CustomCurveSettings::Apply() {
		const float tintLuminocity = std::abs(Math::Dot(maxWhiteTint, Vector3(0.2126f, 0.7152f, 0.0722f)));
		const Vector3 maxWhiteColor = maxWhite * tintLuminocity / maxWhiteTint;
		m_settingsBuffer.Map() = maxWhiteColor;
		m_settingsBuffer->Unmap(true);
		m_responseCurveBinding->BoundObject() = responseCurve.GetCurveBuffer();
	}
}
