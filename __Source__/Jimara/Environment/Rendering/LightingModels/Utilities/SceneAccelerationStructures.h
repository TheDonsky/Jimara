#pragma once
#include "../../../Scene/Scene.h"


namespace Jimara {
	/// <summary> Scene-wide shared BLAS collection and update manager </summary>
	class JIMARA_API SceneAccelerationStructures : public virtual Object {
	public:
		/// <summary>
		/// Retieves shared instance of scene-acceleration structure manager.
		/// <para/> If Hardware-Ray-Tracing is not supported on the device, this function will return nullptr.
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared instance of the scene acceleration structure set </returns>
		static Reference<SceneAccelerationStructures> Get(SceneContext* context);

		/// <summary> Virtual descriptor </summary>
		virtual ~SceneAccelerationStructures();

		/// <summary> Flags for individual bottom-level acceleration structures </summary>
		enum class JIMARA_API Flags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0u,

			/// <summary> 
			// If this flag is set, initial build will be scheduled immediately during Blas handle initialization.
			// <para/> Using this flag only guarantees the build process will be submitted to the queue immediately,
			// but will not cause any wait-time for the command's completion.
			// </summary>
			INITIAL_BUILD_SCHEDULE_URGENT = (1 << 0),

			/// <summary> If this flag is set, this blas will be rebuilt/refitted on each frame. </summary>
			REBUILD_ON_EACH_FRAME = (1 << 2)
		};

		/// <summary>
		/// Bottom-Level acceleration structure descriptor
		/// </summary>
		struct JIMARA_API BlasDesc {
			/// <summary> Vertex buffer </summary>
			Reference<Graphics::ArrayBuffer> vertexBuffer;

			/// <summary> Index buffer (Can be of U32 or U16 type, tightly packed, without any other content within) </summary>
			Reference<Graphics::ArrayBuffer> indexBuffer;

			/// <summary> Vertex format </summary>
			Graphics::BottomLevelAccelerationStructure::VertexFormat vertexFormat = Graphics::BottomLevelAccelerationStructure::VertexFormat::X32Y32Z32;

			/// <summary> Index format </summary>
			Graphics::BottomLevelAccelerationStructure::IndexFormat indexFormat = Graphics::BottomLevelAccelerationStructure::IndexFormat::U32;

			/// <summary> First vertex position offset from buffer memory start </summary>
			uint32_t vertexPositionOffset = 0u;

			/// <summary> Interval between vertex position values </summary>
			uint32_t vertexStride = 0u;

			/// <summary> Number of triangles making up the geometry </summary>
			uint32_t faceCount = 0u;

			/// <summary> Blas flags </summary>
			Flags flags = Flags::NONE;
		};


		/// <summary>
		/// Bottom-Level acceleration structure instance
		/// </summary>
		class JIMARA_API Blas : public virtual Object {
		public:
			/// <summary> Descriptor, used for Blas instance creation </summary>
			const BlasDesc& Descriptor()const;

			/// <summary>
			/// Gives access to underlying acceleration structure.
			/// <para/> Can return nullptr if the BLAS is not yet initialized (relevant if and only if INITIAL_BUILD_URGENT flag is not present).
			/// </summary>
			/// <returns> Acceleration structure handle </returns>
			Graphics::BottomLevelAccelerationStructure* AcccelerationStructure()const;
		};


		/// <summary>
		/// Creates or retrieves shared instance of a Bottom-Level acceleration structure based on the descriptor.
		/// </summary>
		/// <param name="desc"> Blas descriptor </param>
		/// <returns> Blas instance </returns>
		Reference<Blas> GetBlas(BlasDesc& desc);

		/// <summary> 
		/// Event, invoked each time the underlying build and update jobs want to collect their dependencies.
		/// <para/> Simulation-Jobs are already within the job dependencies, so no need to attach those as additional dependencies.
		/// </summary>
		Event<Callback<JobSystem::Job*>>& OnCollectBuildDependencies();

		/// <summary>
		/// Reports graphics render jobs performing BLAS build and update operations.
		/// <para/> Renderers using the acceleration structures should probably wait for the completion of these jobs.
		/// </summary>
		/// <param name="report"> Jobs will be reported through this callback </param>
		void CollectRebuildJobs(const Callback<JobSystem::Job*> report);

	private:
		// Constructor is private:
		SceneAccelerationStructures();

		// Underlying implementation is contained within Helpers struct scope...
		struct Helpers;
	};

	// Define boolen operations for the flags:
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(SceneAccelerationStructures::Flags);

	/// <summary>
	/// Compares two Blas-descriptors
	/// </summary>
	/// <param name="a"> Firs descriptor </param>
	/// <param name="b"> Second descriptor </param>
	/// <returns> True, if the descriptors are the same </returns>
	inline static bool operator==(const SceneAccelerationStructures::BlasDesc& a, const SceneAccelerationStructures::BlasDesc& b) {
		return
			(a.vertexBuffer == b.vertexBuffer) &&
			(a.indexBuffer == b.indexBuffer) &&
			(a.vertexFormat == b.vertexFormat) &&
			(a.indexFormat == b.indexFormat) &&
			(a.vertexPositionOffset == b.vertexPositionOffset) &&
			(a.vertexStride == b.vertexStride) &&
			(a.faceCount == b.faceCount) &&
			(a.flags == b.flags);
	}
}

namespace std {
	/// <summary> Hash-function for Jimara::SceneAccelerationStructures::BlasDesc </summary>
	template<>
	struct hash<Jimara::SceneAccelerationStructures::BlasDesc> {
		/// <summary>
		/// Calculates hash of Jimara::SceneAccelerationStructures::BlasDesc.
		/// </summary>
		/// <param name="key"> Jimara::SceneAccelerationStructures::BlasDesc to hash </param>
		/// <returns> Hash value </returns>
		inline size_t operator()(const Jimara::SceneAccelerationStructures::BlasDesc& key)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Graphics::ArrayBuffer*>()(key.vertexBuffer),
				std::hash<Jimara::Graphics::ArrayBuffer*>()(key.indexBuffer),
				std::hash<decltype(key.vertexFormat)>()(key.vertexFormat),
				std::hash<decltype(key.indexFormat)>()(key.indexFormat),
				std::hash<decltype(key.vertexPositionOffset)>()(key.vertexPositionOffset),
				std::hash<decltype(key.vertexStride)>()(key.vertexStride),
				std::hash<decltype(key.faceCount)>()(key.faceCount),
				std::hash<decltype(key.flags)>()(key.flags));
		}
	};
}

