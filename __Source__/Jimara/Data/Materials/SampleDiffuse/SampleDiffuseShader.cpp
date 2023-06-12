#include "SampleDiffuseShader.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../../Math/Helpers.h"

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
			static const constexpr std::string_view TexSamplerName() { return "texSampler"; }
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

	namespace {
#pragma warning(disable: 4250)
		class SampleDiffuseShaderMaterialInstance 
			: public virtual Material::Instance
			, public virtual ObjectCache<SampleDiffuseShader_DeviceMaterialInstanceCacheIndex>::StoredObject {
		public:
			inline SampleDiffuseShaderMaterialInstance(Material::Reader& reader) : Material::Instance(&reader) {}

			class Cache : public virtual ObjectCache<SampleDiffuseShader_DeviceMaterialInstanceCacheIndex> {
			public:
				inline static Reference<SampleDiffuseShaderMaterialInstance> GetFor(Graphics::GraphicsDevice* device, Vector3 baseColor) {
					const SampleDiffuseShader_DeviceMaterialInstanceCacheIndex index = { device, baseColor };
					static Cache cache;
					return cache.GetCachedOrCreate(index, false, [&]()->Reference<SampleDiffuseShaderMaterialInstance> {
						Reference<Material> material = Object::Instantiate<Material>(device);
						{
							Material::Writer writer(material);
							auto sharedBinding = Graphics::ShaderClass::SharedConstantBufferBinding<Vector3>(index.color, index.device);
							writer.SetShader(SampleDiffuseShader::Instance());
							writer.SetConstantBuffer(BaseColorName(), sharedBinding->BoundObject());
						}
						return Object::Instantiate<SampleDiffuseShaderMaterialInstance>(Material::Reader(material));
						});
				}
			};
		};
#pragma warning(default: 4250)
	}

	Reference<Material::Instance> SampleDiffuseShader::MaterialInstance(Graphics::GraphicsDevice* device, Vector3 baseColor) {
		return SampleDiffuseShaderMaterialInstance::Cache::GetFor(device, baseColor);
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(Graphics::Texture* texture, Graphics::GraphicsDevice* device) {
		Reference<Material> material = Object::Instantiate<Material>(device);
		Material::Writer writer(material);
		writer.SetShader(Instance());
		writer.SetTextureSampler(TexSamplerName().data(), texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler());
		return material;
	}

	Reference<const Graphics::ShaderClass::ConstantBufferBinding> SampleDiffuseShader::DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == BaseColorName()) return SharedConstantBufferBinding<Vector3>(Vector3(1.0f), device);
		else return nullptr;
	}

	Reference<const Graphics::ShaderClass::TextureSamplerBinding> SampleDiffuseShader::DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == NormalMapName())
			return ShaderClass::SharedTextureSamplerBinding(Vector4(0.5f, 0.5f, 1.0f, 1.0f), device);
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
