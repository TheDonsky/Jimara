#pragma once
#include "../../Scene/Scene.h"
#include "../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include "../../Layers.h"


namespace Jimara {
	class JIMARA_API GraphicsObjectPipelines : public virtual Object {
	public:
		struct JIMARA_API Descriptor final {
			Reference<GraphicsObjectDescriptor::Set> descriptorSet;

			Reference<const ViewportDescriptor> viewportDescriptor;

			Reference<Graphics::RenderPass> renderPass;

			LayerMask layers = LayerMask::All();

			OS::Path lightingModel;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		class JIMARA_API ObjectInfo final {
		public:
			inline virtual ~ObjectInfo() {}

			inline GraphicsObjectDescriptor* Descriptor()const { return m_descriptor; }

			inline const GraphicsObjectDescriptor::ViewportData* ViewData()const { return m_viewportData; }

			void ExecutePipeline(const Graphics::InFlightBufferInfo& inFlightBuffer)const;

		private:
			Reference<GraphicsObjectDescriptor> m_descriptor;

			Reference<const GraphicsObjectDescriptor::ViewportData> m_viewportData;

			Reference<Graphics::GraphicsPipeline> m_graphicsPipeline;

			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>>* m_vertexBufferBindings = nullptr;
			size_t m_vertexBufferBindingCount = 0u;
			Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_indexBufferBinding;

			Reference<Graphics::VertexInput> m_vertexInput;

			const Reference<Graphics::BindingSet>* m_bindingSets = nullptr;
			size_t m_bindingSetCount = 0u;

			friend class GraphicsObjectPipelines;
		};

		class JIMARA_API Reader final {
		public:
			Reader(const GraphicsObjectPipelines& pipelines);

			~Reader();

			size_t ObjectCount()const;

			const ObjectInfo& Object(size_t index)const;

		private:
			const Reference<const Jimara::Object> m_data;
			const std::shared_lock<std::shared_mutex> m_lock;
			const void* m_objectInfos = nullptr;
			size_t m_objectInfoCount = 0u;

			Reader(const Reference<const Jimara::Object>& data);
		};

		static Reference<GraphicsObjectPipelines> Get(const Descriptor& desc);

		virtual ~GraphicsObjectPipelines();

		inline Graphics::RenderPass* RenderPass()const { return m_renderPass; }

		inline Graphics::Pipeline* EnvironmentPipeline()const { return m_environmentPipeline; }

		void GetUpdateTasks(const Callback<JobSystem::Job*> recordUpdateTasks)const;

	private:
		const Reference<Graphics::RenderPass> m_renderPass;
		const Reference<Graphics::Pipeline> m_environmentPipeline;

		struct Helpers;
		GraphicsObjectPipelines(Graphics::RenderPass* renderPass, Graphics::Pipeline* environmentPipeline)
			: m_renderPass(renderPass), m_environmentPipeline(environmentPipeline) {
			assert(m_renderPass != nullptr);
			assert(m_environmentPipeline != nullptr);
		}
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
