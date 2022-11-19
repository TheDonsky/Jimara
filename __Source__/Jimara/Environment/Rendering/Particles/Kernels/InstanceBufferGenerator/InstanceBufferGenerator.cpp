#include "InstanceBufferGenerator.h"
#include "../CombinedParticleKernel.h"


namespace Jimara {
	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator() 
		: ParticleKernel(sizeof(TaskSettings)) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	const ParticleInstanceBufferGenerator* ParticleInstanceBufferGenerator::Instance() {
		static const ParticleInstanceBufferGenerator instance;
		return &instance;
	}

	Reference<ParticleKernel::Instance> ParticleInstanceBufferGenerator::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/InstanceBufferGenerator/InstanceBufferGenerator_Kernel");
		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> binding = 
				Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return binding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
		} bindingSet;
		bindingSet.binding->BoundObject() = context->Graphics()->Bindless().BufferBinding();
		return CombinedParticleKernel<TaskSettings>::Create(context, &SHADER_CLASS, bindingSet);
	}
}
