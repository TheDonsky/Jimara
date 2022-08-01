#pragma once
#include "../../Scene/Scene.h"
#include "../SceneObjects/GraphicsObjectDescriptor.h"
#include "../../Layers.h"


namespace Jimara {
	class JIMARA_API LightingModelPipelines : public virtual Object {
	public:
		struct JIMARA_API Descriptor {
			Reference<Scene::LogicContext> context = nullptr;

			LayerMask layers = LayerMask::All();

			OS::Path lightingModel;

			bool operator==(const Descriptor& other)const;
			bool operator<(const Descriptor& other)const;
		};

		struct JIMARA_API RenderPassDescriptor {
			Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_1;

			Stacktor<Graphics::Texture::PixelFormat> colorAttachmentFormats;

			Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;

			Graphics::RenderPass::Flags renderPassFlags = Graphics::RenderPass::Flags::NONE;

			bool operator==(const RenderPassDescriptor& other)const;
			bool operator<(const RenderPassDescriptor& other)const;
		};

		class JIMARA_API Instance;
		class JIMARA_API Reader;

		static Reference<LightingModelPipelines> Get(const Descriptor& descriptor);

		Reference<Instance> GetInstance(const RenderPassDescriptor& renderPassInfo)const;

		virtual ~LightingModelPipelines();

	private:
		const Descriptor m_modelDescriptor;
		const Reference<Graphics::ShaderSet> m_shaderSet;
		const Reference<Graphics::ShaderCache> m_shaderCache;
		const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjects;
		const Reference<Graphics::PipelineDescriptor> m_environmentDescriptor;
		const Reference<Object> m_pipelineDescriptorCache;
		const Reference<Object> m_instanceCache;

		struct Helpers;
		LightingModelPipelines(const Descriptor& descriptor);
	};

	

	class JIMARA_API LightingModelPipelines::Instance : public virtual Object {
	public:
		virtual ~Instance();

		Reference<Graphics::Pipeline> CreateEnvironmentPipeline(const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings)const;

		Graphics::RenderPass* RenderPass()const;

	private:
		const Reference<const LightingModelPipelines> m_pipelines;
		const Reference<Graphics::RenderPass> m_renderPass;
		const Reference<Object> m_instanceDataReference;

		Instance(const RenderPassDescriptor& renderPassInfo, const LightingModelPipelines* pipelines);
		friend class LightingModelPipelines;
	};



	class JIMARA_API LightingModelPipelines::Reader {
	public:
		Reader(const Instance* instance);

		inline Reader(const Reference<Instance>& instance) : Reader((const Instance*)instance.operator->()) {}
		inline Reader(const Reference<const Instance>& instance) : Reader((const Instance*)instance.operator->()) {}

		~Reader();

		size_t PipelineCount()const;

		Graphics::GraphicsPipeline* Pipeline(size_t index)const;

		GraphicsObjectDescriptor* GraphicsObject(size_t index)const;

	private:
		const std::shared_lock<std::shared_mutex> m_lock;
		const Reference<Object> m_data;
		size_t m_count = 0u;
		const void* m_pipelineData = nullptr;

		Reader(Reference<Object> data);
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
