#pragma once
#include "../LightingModel.h"
#include <shared_mutex>


namespace Jimara {
#pragma warning(disable: 4250)
	class ObjectIdRenderer : public virtual JobSystem::Job, public virtual ObjectCache<Reference<const Object>>::StoredObject {
	public:
		static Reference<ObjectIdRenderer> GetFor(const LightingModel::ViewportDescriptor* viewport, bool cached = true);

		void SetResolution(Size2 resolution);

		struct ResultBuffers {
			Reference<Graphics::TextureView> vertexPosition;
			Reference<Graphics::TextureView> vertexNormal;
			Reference<Graphics::TextureView> objectIndex;
			Reference<Graphics::TextureView> instanceIndex;
			Reference<Graphics::TextureView> depthAttachment;
		};

		ResultBuffers GetLastResults()const;

	protected:
		/// <summary> Invoked by job system to render what's needed </summary>
		virtual void Execute() override;

		/// <summary>
		/// Reports dependencies, if there are any
		/// </summary>
		/// <param name="addDependency"> Calling this will record dependency for given job </param>
		virtual void CollectDependencies(Callback<Job*> addDependency) override;

	private:
		const Reference<const LightingModel::ViewportDescriptor> m_viewport;
		const Reference<Object> m_pipelineObjects;
		const Reference<Object> m_environmentDescriptor;

		mutable std::shared_mutex m_bufferLock;
		Size2 m_resolution = Size2(1, 1);
		struct TargetBuffers : public ResultBuffers {
			Reference<Graphics::FrameBuffer> frameBuffer;
		} m_buffers;

		Reference<Graphics::Pipeline> m_environmentPipeline;

		ObjectIdRenderer(const LightingModel::ViewportDescriptor* viewport);

		bool UpdateBuffers();
	};
#pragma warning(default: 4250)
}
