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

			OS::Path lightingModel;

			LayerMask layers = LayerMask::All();

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		class JIMARA_API ObjectInfo final {
		public:
			inline virtual ~ObjectInfo() {}

			inline const GraphicsObjectDescriptor* Descriptor()const { return m_descriptor; }

			inline const GraphicsObjectDescriptor::ViewportData* ViewData()const { return m_viewportData; }

			void ExecutePipeline(const Graphics::InFlightBufferInfo& inFlightBuffer)const;

		private:
			Reference<const GraphicsObjectDescriptor> m_descriptor;

			Reference<const GraphicsObjectDescriptor::ViewportData> m_viewportData;

			Reference<Graphics::Experimental::GraphicsPipeline> m_graphicsPipeline;

			Stacktor<Reference<Graphics::BindingSet>, 4u> m_bindingSets;

			friend class GraphicsObjectPipelines;
		};

		static Reference<GraphicsObjectPipelines> Get(const Descriptor& desc);

		virtual ~GraphicsObjectPipelines();

		inline Graphics::Experimental::Pipeline* EnvironmentPipeline()const { return m_environmentPipeline; }

		inline size_t ObjectCount()const;

		inline const ObjectInfo& Object(size_t index)const;

		void GetUpdateTasks(const Callback<JobSystem::Job*> recordUpdateTasks)const;

	private:
		const Reference<Graphics::Experimental::Pipeline> m_environmentPipeline;
		const void* m_objectInfos = nullptr;
		size_t m_objectInfoCount = 0u;
		size_t m_objectInfoSize = 0u;

		struct Helpers;
		GraphicsObjectPipelines(Graphics::Experimental::Pipeline* environmentPipeline) : m_environmentPipeline(environmentPipeline) {}
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
