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
			Vector3 objectPosition;

			/// <summary> Fragment normal </summary>
			Vector3 objectNormal;

			/// <summary> Rendered object index (from ObjectIdRenderer) </summary>
			uint32_t objectIndex;

			/// <summary> Instance index (from GraphicsObjectDescriptor) </summary>
			uint32_t instanceIndex;

			/// <summary> Rendered object reference </summary>
			Reference<GraphicsObjectDescriptor> graphicsObject;

			/// <summary> Queried position </summary>
			Size2 viewportPosition;
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
