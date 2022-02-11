#pragma once
#include "../LightingModel.h"
#include "../../SceneObjects/GraphicsObjectDescriptor.h"

namespace Jimara {
	/// <summary>
	/// Queries rendered object information from viewport
	/// </summary>
	class ViewportObjectQuery : public virtual ObjectCache<Reference<const Object>>::StoredObject {
	public:
		/// <summary>
		/// Retrieves instance for a viewport
		/// </summary>
		/// <param name="viewport"> Viewport </param>
		/// <returns> ViewportObjectQuery </returns>
		static Reference<ViewportObjectQuery> GetFor(const LightingModel::ViewportDescriptor* viewport);

		/// <summary> Virtual destructor </summary>
		virtual ~ViewportObjectQuery();

		/// <summary>
		/// Single query result
		/// </summary>
		struct Result {
			/// <summary> Fragment position </summary>
			Vector3 objectPosition = Vector3(0.0f);

			/// <summary> Fragment normal </summary>
			Vector3 objectNormal = Vector3(0.0f);

			/// <summary> Rendered object index (from ObjectIdRenderer) </summary>
			uint32_t objectIndex = ~((uint32_t)0);

			/// <summary> Instance index (from GraphicsObjectDescriptor) </summary>
			uint32_t instanceIndex = 0;

			/// <summary> Index of a primitive/face within the instance </summary>
			uint32_t primitiveIndex = 0;

			/// <summary> Rendered object reference </summary>
			Reference<GraphicsObjectDescriptor> graphicsObject;

			/// <summary> 
			/// Component from graphicsObject->GetComponent(instanceIndex, primitiveIndex)
			/// Note: Evaluated after instanceIndex and primitiveIndex get retrieved; not terribly stable if the components get deleted and/or created rapidly
			/// </summary>
			Reference<Component> component;

			/// <summary> Queried position </summary>
			Size2 viewportPosition = Size2(0);
		};

		/// <summary>
		/// Queries pixel information
		/// Notes:
		///		0. Pixel will be loaded from shared ObjectIdRenderer with several frames of delay;
		///		1. processResult will be invoked from main update queue;
		///		2. processResult will be invoked even if userData is a Component and it gets destroyed before getting the results. 
		///		Therefore, some caution is advised...
		/// </summary>
		/// <param name="position"> "Screen-space" position (in pixels) </param>
		/// <param name="processResult"> Event to be invoked for reporting results </param>
		/// <param name="userData"> User data </param>
		void QueryAsynch(Size2 position, Callback<Object*, Result> processResult, Object* userData);

	private:
		// Underlying job
		const Reference<JobSystem::Job> m_job;

		// Constructor
		ViewportObjectQuery(JobSystem::Job* job);
	};
}

namespace std {
	/// <summary>
	/// Stream output operator for Jimara::ViewportObjectQuery::Result
	/// </summary>
	/// <param name="stream"> std::ostream to outpout to </param>
	/// <param name="result"> Jimara::ViewportObjectQuery::Result to output </param>
	/// <returns> stream </returns>
	inline static std::ostream& operator<<(std::ostream& stream, const Jimara::ViewportObjectQuery::Result& result) {
		stream << "{"
			<< "\n    objectPosition:   " << result.objectPosition
			<< "\n    objectNormal:     " << result.objectNormal
			<< "\n    objectIndex:      " << result.objectIndex
			<< "\n    instanceIndex:    " << result.instanceIndex
			<< "\n    primitiveIndex:   " << result.primitiveIndex
			<< "\n    graphicsObject:   " << ((size_t)result.graphicsObject.operator->())
			<< "\n    viewportPosition: " << result.viewportPosition
			<< "\n    component:        " << ((size_t)result.component.operator->()) << "(" << (result.component == nullptr ? "<None>" : result.component->Name()) << ")"
			<< "\n}" << std::endl;
		return stream;
	}
}
