#pragma once
#include "../LightingModel.h"
#include "../../SceneObjects/GraphicsObjectDescriptor.h"
#include <shared_mutex>


namespace Jimara {
#pragma warning(disable: 4250)
	/// <summary>
	/// Renders scene to a frame buffer, consisting of position, normal and object & instance indices.
	/// Note: Job is designed to run as a part of the graphics render job system.
	/// </summary>
	class ObjectIdRenderer : public virtual JobSystem::Job, public virtual ObjectCache<Reference<const Object>>::StoredObject {
	public:
		/// <summary>
		/// Creates ObjectIdRenderer for given viewport
		/// </summary>
		/// <param name="viewport"> Render viewport </param>
		/// <param name="cached"> If true, the viewport will be used for reference-caching and the ObjectIdRenderer will be reused </param>
		/// <returns> ObjectIdRenderer </returns>
		static Reference<ObjectIdRenderer> GetFor(const LightingModel::ViewportDescriptor* viewport, bool cached = true);

		/// <summary>
		/// Sets target resolution
		/// </summary>
		/// <param name="resolution"> Target resolution </param>
		void SetResolution(Size2 resolution);

		/// <summary>
		/// Result of ObjectIdRenderer execution
		/// </summary>
		struct ResultBuffers {
			/// <summary> vec4(Jimara_GeometryBuffer.position.xyz, 1) </summary>
			Reference<Graphics::TextureView> vertexPosition;

			/// <summary> vec4(Jimara_GeometryBuffer.normal.xyz, 0) </summary>
			Reference<Graphics::TextureView> vertexNormal;

			/// <summary> Index of the GraphicsObjectDescriptor </summary>
			Reference<Graphics::TextureView> objectIndex;

			/// <summary> Index of the instance from GraphicsObjectDescriptor </summary>
			Reference<Graphics::TextureView> instanceIndex;

			/// <summary> Screen-space vertexNormal, but as a color </summary>
			Reference<Graphics::TextureView> vertexNormalColor;

			/// <summary> Depth attachment, used for rendering </summary>
			Reference<Graphics::TextureView> depthAttachment;
		};

		/// <summary>
		/// Result buffers from the last execution
		/// Notes: 
		///		0. Mostly useful for other jobs that depend on this one... Otherwise, there's no guarantee that they are of the current frame;
		///		1. Depending on the timing, there's a chance of this being from previous frame, unless we have a job system dependency;
		///		2. SetResolution() is 'Applied' on the next execution, so the resolution is not guaranteed to be updated immediately.
		/// </summary>
		/// <returns> ResultBuffers </returns>
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
		// Viewport
		const Reference<const LightingModel::ViewportDescriptor> m_viewport;

		// Pipeline object collection
		const Reference<Object> m_pipelineObjects;

		// Environment descriptor
		const Reference<Graphics::ShaderResourceBindings::ShaderResourceBindingSet> m_environmentDescriptor;

		// Lock for result buffer & resolution
		mutable std::shared_mutex m_bufferLock;

		// Desired resolution
		Size2 m_resolution = Size2(1, 1);

		// Internal result buffer
		struct TargetBuffers : public ResultBuffers {
			// Frame buffer
			Reference<Graphics::FrameBuffer> frameBuffer;
		} m_buffers;

		// Environment pipeline
		Reference<Graphics::Pipeline> m_environmentPipeline;

		// Constructor
		ObjectIdRenderer(const LightingModel::ViewportDescriptor* viewport);

		// Updates result buffers
		bool UpdateBuffers();
	};
#pragma warning(default: 4250)
}
