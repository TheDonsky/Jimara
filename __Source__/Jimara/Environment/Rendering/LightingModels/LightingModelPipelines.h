#pragma once
#include "../../Scene/Scene.h"
#include "../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include "../../Layers.h"


namespace Jimara {
	/// <summary>
	/// A simple itility for accessing graphics pipelines per-lighting model without too much worrying about the underlying complexity of loading shader sets,
	/// creating descriptors, instantiating pipelines and so on and so on...
	/// <para/> Notes:
	///		<para/> 0. LightingModelPipelines is responsible for generating and sharing graphics pipeline descriptors;
	///		<para/> 1. Individual graphics pipeline instances are created by shared LightingModelPipelines::Instance, that is aware of the render pass layout;
	///		<para/> 2. This class may change a lot once we properly refactor the graphics pipeline API...
	/// </summary>
	class JIMARA_API LightingModelPipelines : public virtual Object {
	public:
		/// <summary>
		/// Basic information about the lighting models
		/// </summary>
		struct JIMARA_API Descriptor {
			/// <summary> Viewport descriptor (Objects will be created for per-viewport data; if null, context HAS TO BE specified) </summary>
			Reference<const ViewportDescriptor> viewport = nullptr;

			/// <summary> 
			/// Target scene context 
			/// <para/> If Viewport is specified, this field will be ignored
			/// <para/> All pipelines will be created such that they can be used from this context's render thread.
			/// </summary>
			Reference<Scene::LogicContext> context = nullptr;

			/// <summary> Layer filter for the graphics objects </summary>
			LayerMask layers = LayerMask::All();

			/// <summary> Lighting model path </summary>
			OS::Path lightingModel;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		/// <summary>
		/// Basic information about the render pass type
		/// <para/> Note: Needed for actually creating and sharing graphics pipelines and render passes via LightingModelPipelines::Instance
		/// </summary>
		struct JIMARA_API RenderPassDescriptor {
			/// <summary> Sample count of color/depth attachments </summary>
			Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_1;

			/// <summary> Color attachment formats </summary>
			Stacktor<Graphics::Texture::PixelFormat, 4u> colorAttachmentFormats;

			/// <summary> Depth attachment format </summary>
			Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;

			/// <summary> Resolve & Clear flags for each attachment type </summary>
			Graphics::RenderPass::Flags renderPassFlags = Graphics::RenderPass::Flags::NONE;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const RenderPassDescriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const RenderPassDescriptor& other)const;
		};

		class JIMARA_API Instance;
		class JIMARA_API Reader;

		/// <summary>
		/// Gets shared instance of LightingModelPipelines for given lighting model descriptor
		/// </summary>
		/// <param name="descriptor"> Lighting model descriptor </param>
		/// <returns> Shared LightingModelPipelines instance </returns>
		static Reference<LightingModelPipelines> Get(const Descriptor& descriptor);

		/// <summary>
		/// Gets shared pipeline instance collection per render pass type
		/// </summary>
		/// <param name="renderPassInfo"> Render pass information </param>
		/// <returns> Shared LightingModelPipelines::Instance </returns>
		Reference<Instance> GetInstance(const RenderPassDescriptor& renderPassInfo)const;

		/// <summary>
		/// Creates an environment pipeline
		/// </summary>
		/// <param name="bindings"> Binding information </param>
		/// <returns> New instance of an environment pipeline </returns>
		Reference<Graphics::Pipeline> CreateEnvironmentPipeline(const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings)const;

		/// <summary> Virtual destructor </summary>
		virtual ~LightingModelPipelines();

	private:
		// Internals:
		const Descriptor m_modelDescriptor;
		const Reference<Graphics::ShaderSet> m_shaderSet;
		const Reference<Graphics::ShaderCache> m_shaderCache;
		const Reference<const ViewportGraphicsObjectSet> m_viewportObjects;
		const Reference<Graphics::PipelineDescriptor> m_environmentDescriptor;
		const Reference<Object> m_pipelineDescriptorCache;
		const Reference<Object> m_instanceCache;

		// Some private helper utilities:
		struct Helpers;

		// Constructor is private:
		LightingModelPipelines(const Descriptor& descriptor);
	};

	

	/// <summary>
	/// Graphics pipeline collection, generated per render pass type
	/// <para/> Note: Use LightingModelPipelines::Reader to access the pipelines within in a thread-safe manner
	/// </summary>
	class JIMARA_API LightingModelPipelines::Instance : public virtual Object {
	public:
		/// <summary> Virtual destructor </summary>
		virtual ~Instance();

		/// <summary> Main RenderPass of this instance </summary>
		Graphics::RenderPass* RenderPass()const;

	private:
		// Internals:
		const Reference<const LightingModelPipelines> m_pipelines;
		const Reference<Graphics::RenderPass> m_renderPass;
		const Reference<Object> m_instanceDataReference;

		// Constructor needs to be private:
		Instance(const RenderPassDescriptor& renderPassInfo, const LightingModelPipelines* pipelines);
		
		// Friendos:
		friend class LightingModelPipelines;
		friend class Reader;
	};



	/// <summary>
	/// One willing to access pipelines within LightingModelPipelines::Instance in a thread-safe manner should use this reader
	/// </summary>
	class JIMARA_API LightingModelPipelines::Reader {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="Instance"> LightingModelPipelines::Instance to read pipelines from </param>
		Reader(const Instance* instance);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="Instance"> LightingModelPipelines::Instance to read pipelines from </param>
		inline Reader(const Reference<Instance>& instance) : Reader((const Instance*)instance.operator->()) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="Instance"> LightingModelPipelines::Instance to read pipelines from </param>
		inline Reader(const Reference<const Instance>& instance) : Reader((const Instance*)instance.operator->()) {}

		/// <summary> Destructor </summary>
		~Reader();

		/// <summary> Number of pipelines within the LightingModelPipelines::Instance </summary>
		size_t PipelineCount()const;

		/// <summary>
		/// Graphics pipeline by index
		/// </summary>
		/// <param name="index"> Pipeline index [0 - PipelineCount()) </param>
		/// <returns> Graphics pipeline </returns>
		Graphics::GraphicsPipeline* Pipeline(size_t index)const;

		/// <summary>
		/// Graphics object, index'th pipeline was generated from
		/// </summary>
		/// <param name="index"> Pipeline index [0 - PipelineCount()) </param>
		/// <returns> Graphics object descriptor </returns>
		ViewportGraphicsObjectSet::ObjectInfo GraphicsObject(size_t index)const;

	private:
		// Internals:
		const std::shared_lock<std::shared_mutex> m_lock;
		const Reference<Object> m_data;
		size_t m_count = 0u;
		const void* m_pipelineData = nullptr;

		// Constructor from pipeline data
		Reader(Reference<Object> data);
	};
}

namespace std {
	/// <summary> std::hash override for Jimara::LightingModelPipelines::Descriptor </summary>
	template<>
	struct JIMARA_API hash<Jimara::LightingModelPipelines::Descriptor> {
		/// <summary> std::hash override for Jimara::LightingModelPipelines::Descriptor </summary>
		size_t operator()(const Jimara::LightingModelPipelines::Descriptor& descriptor)const;
	};

	/// <summary> std::hash override for Jimara::LightingModelPipelines::RenderPassDescriptor </summary>
	template<>
	struct JIMARA_API hash<Jimara::LightingModelPipelines::RenderPassDescriptor> {
		/// <summary> std::hash override for Jimara::LightingModelPipelines::RenderPassDescriptor </summary>
		size_t operator()(const Jimara::LightingModelPipelines::RenderPassDescriptor& descriptor)const;
	};
}
