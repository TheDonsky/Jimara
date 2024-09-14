#pragma once
#include "../../Scene/Scene.h"
#include "../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include "../../Layers.h"


namespace Jimara {
	/// <summary>
	/// Lighting models that are aiming to render scene or bake lightmaps/other similar data are encouraged to use GraphicsObjectPipelines.
	/// <para/> General use case is as follows:
	/// <para/>		0. Renderer gets shared instance of GraphicsObjectPipelines via GraphicsObjectPipelines::Get function;
	/// <para/>		1. Renderer reports additional pipeline creation and binding set update jobs via GraphicsObjectPipelines::GetUpdateTasks() method to make sure stuff is safe to use;
	/// <para/>		2. One can render the scene using GraphicsObjectPipelines::Reader interface which will safetly expose existing pipelines 
	///				as long as the reader is created inside the scene render job system context and is accessed after tasks from GraphicsObjectPipelines::GetUpdateTasks() have finished their execution.
	/// <para/> Notes: 
	///	<para/>		0. Respecting GraphicsObjectPipelines::GetUpdateTasks() is vital; Ignoring them will result in unstability and crashes!
	/// <para/>		1. if and when GraphicsObjectPipelines::Get() returns a new instance of GraphicsObjectPipelines, it will initially be empty; 
	///				If Get() query happens on graphics synch point, the collection will be ready to use by the time update tasks are executed, otherwise, it'll stay empty till the next update cycle.
	/// </summary>
	class JIMARA_API GraphicsObjectPipelines : public virtual Object {
	public:
		/// <summary>
		/// Flags for filtering and overrides
		/// </summary>
		enum class JIMARA_API Flags : uint32_t {
			/// <summary> No effect </summary>
			NONE = 0u,

			/// <summary> Will not generate pipelines for graphics objects that have their blend mode set to GraphicsPipeline::BlendMode::REPLACE </summary>
			EXCLUDE_OPAQUE_OBJECTS = (1u << 0u),

			/// <summary> Will not generate pipelines for graphics objects that have their blend mode set to GraphicsPipeline::BlendMode::ALPHA_BLEND </summary>
			EXCLUDE_ALPHA_BLENDED_OBJECTS = (1 << 1u),

			/// <summary> Will not generate pipelines for graphics objects that have their blend mode set to GraphicsPipeline::BlendMode::ADDITIVE </summary>
			EXCLUDE_ADDITIVELY_BLENDED_OBJECTS = (1 << 2u),

			/// <summary> Will not generate pipelines for graphics objects that have their blend mode set to anything other than GraphicsPipeline::BlendMode::REPLACE </summary>
			EXCLUDE_NON_OPAQUE_OBJECTS = (EXCLUDE_ALPHA_BLENDED_OBJECTS | EXCLUDE_ADDITIVELY_BLENDED_OBJECTS),

			/// <summary> Enforces blend mode for all pipelines to be overriden by GraphicsPipeline::BlendMode::REPLACE </summary>
			DISABLE_ALPHA_BLENDING = (1 << 3u)
		};

		/// <summary>
		/// Unique identifier of a GraphicsObjectPipelines object
		/// </summary>
		struct JIMARA_API Descriptor final {
			/// <summary>
			/// Set of GraphicsObjectDescriptor-s;
			/// <para/> Note that pipeline creation and descriptor set update system will automatically be addeRed to the context of this set.
			/// </summary>
			Reference<GraphicsObjectDescriptor::Set> descriptorSet;

			/// <summary> Renderer frustrum descriptor </summary>
			Reference<const RendererFrustrumDescriptor> frustrumDescriptor;

			/// <summary> Graphics render pass for generating compatible pipelines </summary>
			Reference<Graphics::RenderPass> renderPass;

			/// <summary> GraphicsObjectDescriptor layers for filtering </summary>
			LayerMask layers = LayerMask::All();

			/// <summary> Additional flags for filtering and overrides </summary>
			Flags flags = Flags::NONE;

			/// <summary> Graphics pipeline flags </summary>
			Graphics::GraphicsPipeline::Flags pipelineFlags = Graphics::GraphicsPipeline::Flags::DEFAULT;

			/// <summary> Path to the lighting model .jlm for pipeline retrieval </summary>
			OS::Path lightingModel;

			/// <summary> JM_LightingModelStage name </summary>
			std::string lightingModelStage;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		/// <summary>
		/// Basic information about GraphicsObjectDescriptor and corresponding ViewportData
		/// alongside the associated graphics pipeline and it's inputs within the set.
		/// </summary>
		class JIMARA_API ObjectInfo final {
		public:
			/// <summary> Graphics object descriptor </summary>
			inline GraphicsObjectDescriptor* Descriptor()const { return m_descriptor; }

			/// <summary> Descriptor's ViewportData </summary>
			inline const GraphicsObjectDescriptor::ViewportData* ViewData()const { return m_viewportData; }

			/// <summary> Corresponding graphics pipeline </summary>
			inline Graphics::GraphicsPipeline* Pipeline()const { return m_graphicsPipeline; }

			/// <summary> 
			/// Number of non-environment binding sets 
			/// (ei the ones not used by the environment pipeline; = Pipeline()->BindingSetCount() - EnvironmentPipeline()->BindingSetCount()) 
			/// </summary>
			inline size_t BindingSetCount()const { return m_bindingSetCount; }

			/// <summary>
			/// Binding set by index
			/// <para/> Note: Writing to the binding set is not safe, since that stuff is done by update tasks in a more managable manner.
			/// </summary>
			/// <param name="index"> Index in range [0 - BindingSetCount()) </param>
			/// <returns> Binding set </returns>
			inline Graphics::BindingSet* BindingSet(size_t index)const { return m_bindingSets[index]; }

			/// <summary> Vertex input </summary>
			inline Graphics::VertexInput* VertexInput()const { return m_vertexInput; }

			/// <summary>
			/// Binds all binding sets alongside Vertex input and executes pipeline
			/// </summary>
			/// <param name="inFlightBuffer"> Command buffer and index </param>
			void ExecutePipeline(const Graphics::InFlightBufferInfo& inFlightBuffer)const;

		private:
			// Graphics object descriptor
			Reference<GraphicsObjectDescriptor> m_descriptor;

			// Descriptor's ViewportData
			Reference<const GraphicsObjectDescriptor::ViewportData> m_viewportData;

			// Corresponding graphics pipeline
			Reference<Graphics::GraphicsPipeline> m_graphicsPipeline;

			// Vertex input
			Reference<Graphics::VertexInput> m_vertexInput;

			// Binding sets
			const Reference<Graphics::BindingSet>* m_bindingSets = nullptr;
			size_t m_bindingSetCount = 0u;

			// GraphicsObjectPipelines has access to internals:
			friend class GraphicsObjectPipelines;
		};

		/// <summary>
		/// Class for reading GraphicsObjectPipelines's content
		/// </summary>
		class JIMARA_API Reader final {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="pipelines"> GraphicsObjectPipelines to read </param>
			Reader(const GraphicsObjectPipelines& pipelines);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="pipelines"> GraphicsObjectPipelines to read </param>
			Reader(const GraphicsObjectPipelines* pipelines);

			/// <summary> Destructor </summary>
			~Reader();

			/// <summary> Number of ObjectInfo-s inside the GraphicsObjectPipelines </summary>
			size_t Count()const;

			/// <summary>
			/// ObjectInfo by index
			/// </summary>
			/// <param name="index"> Object index [0 - Count()) </param>
			/// <returns> Object information </returns>
			const ObjectInfo& operator[](size_t index)const;

		private:
			// Reference to the underlying data container
			const Reference<const Jimara::Object> m_data;

			// Lock to make sure unerlying data is not altered
			const std::shared_lock<std::shared_mutex> m_lock;

			// Pointer to object information
			const void* m_objectInfos = nullptr;

			// Number of ObjectInfo-s inside the GraphicsObjectPipelines
			size_t m_objectInfoCount = 0u;

			// Constructor
			Reader(const Reference<const Jimara::Object>& data);
		};

		/// <summary>
		/// Gets or creates shared instance of GraphicsObjectPipelines
		/// </summary>
		/// <param name="desc"> Filtered set descriptor </param>
		/// <returns> Shared GraphicsObjectPipelines instance </returns>
		static Reference<GraphicsObjectPipelines> Get(const Descriptor& desc);

		/// <summary> Virtual destructor </summary>
		virtual ~GraphicsObjectPipelines();

		/// <summary> Render pass </summary>
		inline Graphics::RenderPass* RenderPass()const { return m_renderPass; }

		/// <summary> 
		/// Pipeline shape for the empty shader 
		/// (GraphicsObjectPipelines does not contain binding sets for first few indices and the renderer is required to create, update and bind them) 
		/// </summary>
		inline Graphics::Pipeline* EnvironmentPipeline()const { return m_environmentPipeline; }

		/// <summary>
		/// Reports jobs that HAVE TO BE executed before it is safe to create and use a Reader.
		/// <para/> Note: It is crutial to respect the requirenment and it is completely unsafe to ignore it!
		/// </summary>
		/// <param name="recordUpdateTasks"> Relevant tasks are reported through this callback </param>
		void GetUpdateTasks(const Callback<JobSystem::Job*> recordUpdateTasks)const;

	private:
		// Render pass
		const Reference<Graphics::RenderPass> m_renderPass;
		
		// Environment pipeline
		const Reference<Graphics::Pipeline> m_environmentPipeline;

		// Private stuff resides in here... And it's plenty
		struct Helpers;

		// Constructor (internals have actual concrete implementation with tasks and data)
		GraphicsObjectPipelines(Graphics::RenderPass* renderPass, Graphics::Pipeline* environmentPipeline)
			: m_renderPass(renderPass), m_environmentPipeline(environmentPipeline) {
			assert(m_renderPass != nullptr);
			assert(m_environmentPipeline != nullptr);
		}
	};

	/// <summary> Logical 'or' between two GraphicsObjectPipelines::Flags </summary>
	inline static GraphicsObjectPipelines::Flags operator|(GraphicsObjectPipelines::Flags a, GraphicsObjectPipelines::Flags b) {
		return static_cast<GraphicsObjectPipelines::Flags>(
			static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(a) | static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(b));
	};

	/// <summary> Logical 'and' of two GraphicsObjectPipelines::Flags </summary>
	inline static GraphicsObjectPipelines::Flags operator&(GraphicsObjectPipelines::Flags a, GraphicsObjectPipelines::Flags b) {
		return static_cast<GraphicsObjectPipelines::Flags>(
			static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(a) & static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(b));
	};

	/// <summary> Logical 'xor' of two GraphicsObjectPipelines::Flags </summary>
	inline static GraphicsObjectPipelines::Flags operator^(GraphicsObjectPipelines::Flags a, GraphicsObjectPipelines::Flags b) {
		return static_cast<GraphicsObjectPipelines::Flags>(
			static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(a) ^ static_cast<std::underlying_type_t<GraphicsObjectPipelines::Flags>>(b));
	};
}

namespace std {
	/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
	template<>
	struct JIMARA_API hash<Jimara::GraphicsObjectPipelines::Descriptor> {
		/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
		size_t operator()(const Jimara::GraphicsObjectPipelines::Descriptor& descriptor)const;
	};
}
