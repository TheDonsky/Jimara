#include "SampleDiffuseShader.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../../Math/Helpers.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	namespace {
		struct SampleDiffuseShader_DeviceMaterialInstanceCacheIndex {
			Reference<Graphics::GraphicsDevice> device;
			Vector3 color = Vector4(0.0f);

			inline bool operator==(const SampleDiffuseShader_DeviceMaterialInstanceCacheIndex& other)const {
				return device == other.device && color == other.color;
			}
		};

		namespace {
			static const constexpr std::string_view BaseColorName() { return "baseColor"; }
			static const constexpr std::string_view TexSamplerName() { return "colorTexture"; }
			static const constexpr std::string_view NormalMapName() { return "normalMap"; }
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::SampleDiffuseShader_DeviceMaterialInstanceCacheIndex> {
		inline size_t operator()(const Jimara::SampleDiffuseShader_DeviceMaterialInstanceCacheIndex& index)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Graphics::GraphicsDevice*>()(index.device), Jimara::MergeHashes(
					Jimara::MergeHashes(std::hash<float>()(index.color.r), std::hash<float>()(index.color.g)),
					std::hash<float>()(index.color.b)));
		}
	};
}

namespace Jimara {
	SampleDiffuseShader* SampleDiffuseShader::Instance() {
		static SampleDiffuseShader instance;
		return &instance;
	}

	Reference<const Material::Instance> SampleDiffuseShader::MaterialInstance(
		Graphics::GraphicsDevice* device,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		const Material::LitShaderSet* shaders,
		Vector3 baseColor) {

		// Validate input:
		if (device == nullptr || bindlessBuffers == nullptr || bindlessSamplers == nullptr || shaders == nullptr)
			return nullptr;

		// Find shader:
		const Reference<const Material::LitShader> shader = shaders->FindByPath(PATH);
		if (shader == nullptr) {
			device->Log()->Error("SampleUIShader::MaterialInstance - Failed to find lit-shader for SampleUIShader!");
			return nullptr;
		}

		// Get cached override list instance:
		struct Hash {
			inline size_t operator()(const Vector3& key)const {
				return MergeHashes(std::hash<float>()(key.x), std::hash<float>()(key.y), std::hash<float>()(key.z));
			}
		};
		using CacheT = ObjectCache<Vector3, Hash>;
#pragma warning(disable: 4250)
		struct CachedOverride 
			: public virtual CacheT::StoredObject
			, public virtual MaterialInstanceCache::Overrides {
			inline virtual ~CachedOverride() {}
		};
#pragma warning(default: 4250)
		struct Cache : public virtual CacheT {
			static Reference<const CachedOverride> Get(const Vector3& color) {
				static Cache cache;
				return cache.GetCachedOrCreate(color, [&]() -> Reference<CachedOverride> {
					Reference<CachedOverride> inst = Object::Instantiate<CachedOverride>();
					inst->vec3.Push({ "baseColor", color });
					return inst;
					});
			}
		};
		const Reference<const CachedOverride> overrides = Cache::Get(baseColor);

		// Get cached material:
		return MaterialInstanceCache::SharedInstance(device, bindlessBuffers, bindlessSamplers, shader, overrides);
	}

	Reference<const Material::Instance> SampleDiffuseShader::MaterialInstance(const SceneContext* context, Vector3 baseColor) {
		if (context == nullptr)
			return nullptr;
		return MaterialInstance(
			context->Graphics()->Device(),
			context->Graphics()->Bindless().Buffers(),
			context->Graphics()->Bindless().Samplers(),
			context->Graphics()->Configuration().ShaderLibrary()->LitShaders(),
			baseColor);
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(
		Graphics::GraphicsDevice* device,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		const Material::LitShaderSet* shaders,
		Graphics::Texture* texture) {
		// Validate input:
		if (device == nullptr || bindlessBuffers == nullptr || bindlessSamplers == nullptr || shaders == nullptr)
			return nullptr;

		// Find shader:
		const Reference<const Material::LitShader> shader = shaders->FindByPath(PATH);
		if (shader == nullptr) {
			device->Log()->Error("SampleDiffuseShader::CreateMaterial - Failed to find lit-shader for SampleUIShader!");
			return nullptr;
		}

		// Create material:
		const Reference<Material> mat = Object::Instantiate<Material>(device, bindlessBuffers, bindlessSamplers);
		{
			Material::Writer writer(mat);
			writer.SetShader(shader);
			if (texture != nullptr) {
				const auto sampler = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler();
				writer.SetPropertyValue(TexSamplerName(), sampler.operator->());
			}
		}
		return mat;
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(const SceneContext* context, Graphics::Texture* texture) {
		if (context == nullptr)
			return nullptr;
		return CreateMaterial(
			context->Graphics()->Device(),
			context->Graphics()->Bindless().Buffers(),
			context->Graphics()->Bindless().Samplers(),
			context->Graphics()->Configuration().ShaderLibrary()->LitShaders(),
			texture);
	}

	Reference<const Graphics::ShaderClass::ConstantBufferBinding> SampleDiffuseShader::DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == BaseColorName()) 
			return Graphics::SharedConstantBufferBinding<Vector3>(Vector3(1.0f), device);
		else return nullptr;
	}

	Reference<const Graphics::ShaderClass::TextureSamplerBinding> SampleDiffuseShader::DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == NormalMapName())
			return Graphics::SharedTextureSamplerBinding(Vector4(0.5f, 0.5f, 1.0f, 1.0f), device);
		else return ShaderClass::DefaultTextureSamplerBinding(name, device);
	}

	void SampleDiffuseShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		{
			static const ConstantBufferSerializer<Vector3> serializer(BaseColorName(),
				Serialization::ValueSerializer<Vector3>::Create("Color", "Diffuse Color", { Object::Instantiate<Serialization::ColorAttribute>() }), Vector3(1.0f));
			serializer.Serialize(reportField, bindings);
		}
		{
			static const TextureSamplerSerializer serializer(TexSamplerName(), "Diffuse", "Diffuse texture");
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(NormalMapName(), "Normal", "Tangent space normal map");
			reportField(serializer.Serialize(bindings));
		}
	}

	SampleDiffuseShader::SampleDiffuseShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader") {}
}
