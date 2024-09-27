#include "SampleDiffuseShader.h"
#include "../../../Math/Helpers.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	const OS::Path SampleDiffuseShader::PATH = "Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader";

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
					inst->vec3.Push({ std::string(COLOR_NAME), color });
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
				writer.SetPropertyValue(DIFFUSE_NAME, sampler.operator->());
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
}
