#pragma once
#include "../LightingModel.h"
#include "../../SceneObjects/GraphicsObjectDescriptor.h"

namespace Jimara {
	class ViewportObjectQuery : public virtual ObjectCache<Reference<const Object>>::StoredObject {
	public:
		static Reference<ViewportObjectQuery> GetFor(const LightingModel::ViewportDescriptor* viewport);

		virtual ~ViewportObjectQuery();

		struct Result {
			Vector3 objectPosition;
			Vector3 objectNormal;
			uint32_t objectIndex;
			uint32_t instanceIndex;
			Reference<GraphicsObjectDescriptor> graphicsObject;
			Size2 viewportPosition;
		};

		void QueryAsynch(Size2 position, Callback<Object*, Result> processResult, Object* userData);

	private:
		const Reference<JobSystem::Job> m_job;

		ViewportObjectQuery(JobSystem::Job* job);
	};
}
