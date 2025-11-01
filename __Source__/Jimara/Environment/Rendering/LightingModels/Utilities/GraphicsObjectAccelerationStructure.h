#pragma once
#include "../../../Scene/Scene.h"
#include "../../SceneObjects/Objects/ViewportGraphicsObjectSet.h"
#include "JM_StandardVertexInputStructure.h"
#include "../../../Layers.h"


namespace Jimara {
	/// <summary>
	/// Object that builds scene TLAS for a viewport
	/// </summary>
	class JIMARA_API GraphicsObjectAccelerationStructure : public virtual Object {
	public:
		/// <summary> Basic information about a garaphics object contained within the acceleration structure </summary>
		struct JIMARA_API ObjectInformation;

		/// <summary>
		/// TLAS snapshot reader.
		/// <para/> Keep in mind, that resources accessed via the reader will be repeatedly updated over multiple frames
		/// and will only stay consistent for given frame;
		/// <para/> As long as jobs reported via CollectBuildJobs as waited on before readder creation, the underlying data will not be modified.
		/// </summary>
		class JIMARA_API Reader;

		/// <summary> Configuration flags for GraphicsObjectAccelerationStructure instances </summary>
		enum class JIMARA_API Flags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0
		};

		struct JIMARA_API Descriptor {
			/// <summary> Set of GraphicsObjectDescriptor-s </summary>
			Reference<GraphicsObjectDescriptor::Set> descriptorSet;

			/// <summary> Renderer frustrum descriptor </summary>
			Reference<const RendererFrustrumDescriptor> frustrumDescriptor;

			/// <summary> GraphicsObjectDescriptor layers for filtering </summary>
			LayerMask layers = LayerMask::All();

			/// <summary> Additional flags for filtering and overrides </summary>
			Flags flags = Flags::NONE;

			/// <summary> Comparator (equals) </summary>
			bool operator==(const Descriptor& other)const;

			/// <summary> Comparator (less) </summary>
			bool operator<(const Descriptor& other)const;
		};

		/// <summary>
		/// Gets shared cached instance of a GraphicsObjectAccelerationStructure.
		/// </summary>
		/// <param name="desc"> Descriptor </param>
		/// <returns> Shared GraphicsObjectAccelerationStructure instance </returns>
		static Reference<GraphicsObjectAccelerationStructure> GetFor(const Descriptor& desc);

		/// <summary>
		/// Reports graphics render jobs performing TLAS build and update operations.
		/// <para/> Renderers using the acceleration structures should probably wait for the completion of these jobs;
		/// <para/> Readers can be created at any point, but it's only going to be effitient if one waits on these events;
		/// </summary>
		/// <param name="report"> Jobs will be reported through this callback </param>
		void CollectBuildJobs(const Callback<JobSystem::Job*> report)const;

	private:
		// Underlying implementation goes here..
		struct Helpers;

		// Only internal implementation is allowed to create instances:
		inline GraphicsObjectAccelerationStructure() {}
	};


	/// <summary> Boolean opeartors for the flags. </summary>
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(GraphicsObjectAccelerationStructure::Flags);


	/// <summary> Basic information about a garaphics object contained within the acceleration structure </summary>
	struct JIMARA_API GraphicsObjectAccelerationStructure::ObjectInformation {
		/// <summary> Graphics object descriptor </summary>
		Reference<const GraphicsObjectDescriptor> graphicsObject;

		/// <summary> Viewport data of graphicsObject </summary>
		Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;

		/// <summary> Geometry information, retrieved from viewportData </summary>
		GraphicsObjectDescriptor::GeometryDescriptor geometry;

		/// <summary> Index of the first BLAS instance within the TLAS </summary>
		uint32_t firstInstanceIndex = 0u;

		/// <summary> Count of BLAS instances within the TLAS </summary>
		uint32_t instanceCount = 0u;

		/// <summary> Index of the first BLAS (corresponding to index from Reader::Blas()) </summary>
		uint32_t firstBlas = 0u;

		/// <summary> 
		/// Number of different BLAS-es
		/// <para/> Blas-per-instance will be generated if vertex position buffer's perInstanceStride is non-zero;
		/// otherwise, we'll only have a single BLAS.
		/// </summary>
		uint32_t blasCount = 0u;
	};

	/// <summary>
	/// TLAS snapshot reader.
	/// <para/> Keep in mind, that resources accessed via the reader will be repeatedly updated over multiple frames
	/// and will only stay consistent for given frame;
	/// <para/> As long as jobs reported via CollectBuildJobs as waited on before readder creation, the underlying data will not be modified.
	/// </summary>
	class JIMARA_API GraphicsObjectAccelerationStructure::Reader final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="accelerationStructure"> GraphicsObjectAccelerationStructure </param>
		Reader(const GraphicsObjectAccelerationStructure* accelerationStructure);

		/// <summary> Destructor </summary>
		~Reader();

		/// <summary>
		/// Top level acceleration structure.
		/// <para/> If something fails, or the instance in not yet initialized properly, this can be nullptr. Keep that in mind.
		/// </summary>
		inline Graphics::TopLevelAccelerationStructure* Tlas()const { return m_tlas; }

		/// <summary> 
		/// Number of graphics objects within the TLAS.
		/// <para/> index will generally correspond to the custom object index within the TLAS. 
		/// </summary>
		inline size_t ObjectCount()const { return m_objectCount; }

		/// <summary>
		/// Graphics object by ID.
		/// <para/> index will generally correspond to the custom object index within the TLAS.
		/// </summary>
		/// <param name="index"> Graphics object index (valid range is [0-ObjectCount())) </param>
		/// <returns> Graphics object information for given custom object index </returns>
		inline const ObjectInformation& ObjectInfo(size_t index)const { return m_objects[index]; }

		/// <summary>
		/// Number of bottom-level acceleration structures, corresponding to [ObjectInformation::firstBlas; firstBlas + blasCount) ranges.
		/// <para/> Underlying BLAS list might contain duplicates, there is no filtering for that.
		/// </summary>
		inline size_t BlasCount()const { return m_blasCount; }

		/// <summary>
		/// Acceleration structure contained within the TLAS.
		/// <para/> Indices will correspond to [ObjectInformation::firstBlas; firstBlas + blasCount) ranges.
		/// </summary>
		/// <param name="index"> Blas index (valid range is [0-BlasCount())) </param>
		/// <returns> Blas instance reference </returns>
		inline Graphics::BottomLevelAccelerationStructure* Blas(size_t index)const { return m_blasses[index]; }

		/// <summary>
		/// World to TLAS space transformation.
		/// <para/> Tlas might be constructed in view-space and this transform can serve us 
		/// to translate cast rays from world-space to appropriate coordinate system.
		/// </summary>
		inline Matrix4 RayTransform()const { return m_rayTransform; }


	private:
		Reference<Object> m_as;
		Reference<Graphics::TopLevelAccelerationStructure> m_tlas = nullptr;
		size_t m_objectCount = 0u;
		const ObjectInformation* m_objects = 0u;
		size_t m_blasCount = 0u;
		const Reference<Graphics::BottomLevelAccelerationStructure>* m_blasses = nullptr;
		Matrix4 m_rayTransform = Math::Identity();

		// Make sure the reader can not be copied for safety reasons:
		Reader(const Reader&) = delete;
		Reader(Reader&&) = delete;
		Reader& operator=(const Reader&) = delete;
		Reader& operator=(Reader&&) = delete;
	};
}


namespace std {
	/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
	template<>
	struct JIMARA_API hash<Jimara::GraphicsObjectAccelerationStructure::Descriptor> {
		/// <summary> std::hash override for Jimara::GraphicsObjectPipelines::Descriptor </summary>
		size_t operator()(const Jimara::GraphicsObjectAccelerationStructure::Descriptor& descriptor)const;
	};
}

