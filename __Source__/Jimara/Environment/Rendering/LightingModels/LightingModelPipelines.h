#pragma once
#include "../../Scene/Scene.h"
#include "../SceneObjects/GraphicsObjectDescriptor.h"
#include "../../Layers.h"


namespace Jimara {
	class JIMARA_API LightingModelPipelines : public virtual Object {
	public:
		struct JIMARA_API Descriptor;
		struct JIMARA_API RenderPassDescriptor;
		class JIMARA_API Instance;
		class JIMARA_API Reader;

		static Reference<LightingModelPipelines> Get(const Descriptor& descriptor);

		Reference<Instance> GetInstance(const RenderPassDescriptor& renderPassInfo)const;

		virtual ~LightingModelPipelines();

	private:
		const Reference<Object> m_dataReference;

		struct Helpers;
		LightingModelPipelines(Object* dataReference);
	};



	struct JIMARA_API LightingModelPipelines::Descriptor {
		Reference<Scene::LogicContext> context = nullptr;

		LayerMask layers = LayerMask::All();

		OS::Path lightingModel;

		bool operator==(const Descriptor& other)const;
		bool operator<(const Descriptor& other)const;
	};



	struct JIMARA_API LightingModelPipelines::RenderPassDescriptor {
		Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_1;

		Stacktor<Graphics::Texture::PixelFormat> colorAttachmentFormats;

		Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;

		Graphics::RenderPass::Flags renderPassFlags = Graphics::RenderPass::Flags::NONE;

		bool operator==(const RenderPassDescriptor& other)const;
		bool operator<(const RenderPassDescriptor& other)const;
	};

	

	class JIMARA_API LightingModelPipelines::Instance : public virtual Object {
	public:
		Reference<Graphics::Pipeline> CreateEnvironmentPipeline(Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings)const;

		Graphics::RenderPass* RenderPass()const;

	private:
	};



	class JIMARA_API LightingModelPipelines::Reader {
	public:
		Reader(const Instance* instance);

		size_t PipelineCount()const;

		Graphics::GraphicsPipeline* Pipeline(size_t index)const;

		GraphicsObjectDescriptor* GraphicsObject(size_t index)const;

	private:

	};
}

namespace std {
	template<>
	struct JIMARA_API hash<Jimara::LightingModelPipelines::Descriptor> {
		size_t operator()(const Jimara::LightingModelPipelines::Descriptor& descriptor)const;
	};

	template<>
	struct JIMARA_API hash<Jimara::LightingModelPipelines::RenderPassDescriptor> {
		size_t operator()(const Jimara::LightingModelPipelines::RenderPassDescriptor& descriptor)const;
	};
}
