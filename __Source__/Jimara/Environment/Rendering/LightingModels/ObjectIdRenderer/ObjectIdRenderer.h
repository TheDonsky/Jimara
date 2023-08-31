#pragma once
#include "../LightingModel.h"
#include "../GraphicsObjectPipelines.h"
#include "../../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include <shared_mutex>


namespace Jimara {
#pragma warning(disable: 4250)
	/// <summary>
	/// Renders scene to a frame buffer, consisting of position, normal and object & instance indices.
	/// Note: Job is designed to run as a part of the graphics render job system.
	/// </summary>
	class JIMARA_API ObjectIdRenderer : public virtual JobSystem::Job {
	public:
		/// <summary>
		/// Creates ObjectIdRenderer for given viewport
		/// </summary>
		/// <param name="viewport"> Render viewport </param>
		/// <param name="layers"> Layers to include </param>
		/// <param name="cached"> If true, the viewport & layers will be used for reference-caching and the ObjectIdRenderer will be reused </param>
		/// <returns> ObjectIdRenderer </returns>
		static Reference<ObjectIdRenderer> GetFor(const ViewportDescriptor* viewport, LayerMask layers, bool cached = true);

		/// <summary> Virtual destructor </summary>
		virtual ~ObjectIdRenderer();

		/// <summary>
		/// Sets target resolution
		/// </summary>
		/// <param name="resolution"> Target resolution </param>
		void SetResolution(Size2 resolution);

		/// <summary>
		/// Result of ObjectIdRenderer execution
		/// </summary>
		struct JIMARA_API ResultBuffers {
			/// <summary> vec4(Jimara_GeometryBuffer.position.xyz, 1) </summary>
			Reference<Graphics::TextureSampler> vertexPosition;

			/// <summary> vec4(Jimara_GeometryBuffer.normal.xyz, 0) </summary>
			Reference<Graphics::TextureSampler> vertexNormal;

			/// <summary> Index of the GraphicsObjectDescriptor </summary>
			Reference<Graphics::TextureSampler> objectIndex;

			/// <summary> Index of the instance from GraphicsObjectDescriptor </summary>
			Reference<Graphics::TextureSampler> instanceIndex;

			/// <summary> Index of a primitive/face within the instance </summary>
			Reference<Graphics::TextureSampler> primitiveIndex;

			/// <summary> Screen-space vertexNormal, but as a color </summary>
			Reference<Graphics::TextureSampler> vertexNormalColor;

			/// <summary> Depth attachment, used for rendering </summary>
			Reference<Graphics::TextureSampler> depthAttachment;
		};

		/// <summary>
		/// Reader for getting the latest state
		/// </summary>
		class JIMARA_API Reader {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="renderer"> ObjectIdRenderer to read results of (can not be null) </param>
			Reader(const ObjectIdRenderer* renderer);

			/// <summary>
			/// Result buffers from the last execution
			/// Notes: 
			///		0. Mostly useful for other jobs that depend on this one... Otherwise, there's no guarantee that they are of the current frame;
			///		1. Depending on the timing, there's a chance of this being from previous frame, unless we have a job system dependency;
			///		2. SetResolution() is 'Applied' on the next execution, so the resolution is not guaranteed to be updated immediately.
			/// </summary>
			/// <returns> ResultBuffers </returns>
			ResultBuffers LastResults()const;

			/// <summary> Number of GraphicsObjectDescriptors </summary>
			uint32_t DescriptorCount()const;

			/// <summary>
			/// Object descriptor per objectId (matches ResultBuffers.objectIndex)
			/// </summary>
			/// <param name="objectIndex"> Object identifier from ResultBuffers.objectIndex; valid range is [0 - DescriptorCount()) </param>
			/// <returns> Graphics object descriptor </returns>
			ViewportGraphicsObjectSet::ObjectInfo Descriptor(uint32_t objectIndex)const;

		private:
			// ObjectIdRenderer
			const Reference<const ObjectIdRenderer> m_renderer;

			// Read lock
			const std::shared_lock<std::shared_mutex> m_readLock;
		};

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
		const Reference<const ViewportDescriptor> m_viewport;

		// Layer mask
		const LayerMask m_layerMask;

		// Graphics object pipelines
		const Reference<GraphicsObjectPipelines> m_graphicsObjectPipelines;

		// Binding pool for creating entries within m_lightingModelBindings
		const Reference<Graphics::BindingPool> m_bindingPool;

		// Binding sets for bindless bindings
		using BindlessBindingSet = Stacktor<Reference<Graphics::BindingSet>, 4u>;
		using BindlessBindings = Stacktor<BindlessBindingSet, 2u>;
		const BindlessBindings m_bindlessBindings;

		// Since the objectId has to be unique for each draw call, we will create separate binding set for each;
		// m_lightingModelBindings is the list of those and m_objectIdBindings is a shared object that allocates constant buffers for each.
		const Reference<Object> m_objectIdBindings;
		std::vector<Reference<Graphics::BindingSet>> m_lightingModelBindings;

		// Viewport info buffer and staging CPU-RW buffers for updates
		struct ViewportBuffer_t {
			alignas(16) Matrix4 view;
			alignas(16) Matrix4 projection;
			alignas(16) Matrix4 viewPose;
		};
		const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_viewportBuffer;
		const Graphics::ArrayBufferReference<ViewportBuffer_t> m_stagingViewportBuffers;

		// Lock for updates
		mutable std::shared_mutex m_updateLock;

		// Last frame this one got rendered on (for preventing double renders)
		uint64_t m_lastFrame = ~uint64_t(0);

		// Desired resolution
		Size2 m_resolution = Size2(1, 1);

		// Internal result buffer
		struct TargetBuffers : public ResultBuffers {
			// Frame buffer
			Reference<Graphics::FrameBuffer> frameBuffer;
		} m_buffers;

		// Descriptors from the last update
		using DescriptorInfo = std::pair<Reference<GraphicsObjectDescriptor>, Reference<const GraphicsObjectDescriptor::ViewportData>>;
		std::vector<DescriptorInfo> m_descriptors;

		// Constructor
		ObjectIdRenderer(
			const ViewportDescriptor* viewport, LayerMask layers,
			GraphicsObjectPipelines* pipelines, 
			Graphics::BindingPool* bindingPool, 
			const BindlessBindings& bindlessBindings, 
			Object* objectIdBindings,
			const Graphics::ResourceBinding<Graphics::ArrayBuffer>* viewportBuffer,
			const Graphics::ArrayBufferReference<ViewportBuffer_t>& stagingViewportBuffers);

		// Updates result buffers
		bool UpdateBuffers();

		// Helper functionality resides in here
		struct Helpers;
	};
#pragma warning(default: 4250)
}
