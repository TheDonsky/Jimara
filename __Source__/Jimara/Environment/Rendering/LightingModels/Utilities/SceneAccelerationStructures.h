#pragma once
#include "../../../Scene/Scene.h"
#define JIMARA_SceneAccelerationStructures_ENABLE_BlasVariant false

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
			REBUILD_ON_EACH_FRAME = (1 << 1),

			/// <summary> 
			/// If true, each subsequent build after the first one will be refit (only relevant if REBUILD_ON_EACH_FRAME flag is present)
#if JIMARA_SceneAccelerationStructures_ENABLE_BlasVariant
			/// <para/> In case of a BLAS-variant, first build will always be a refit; subscequently, 
			/// one is free to control the refit-source with DO_NOT_USE_BASE_BLAS_AFTER_FIRST_BUILD.
#endif
			/// </summary>
			REFIT_ON_REBUILD = (1 << 2),

			/// <summary> 
			/// If set, this flag will tell the underlying API to prioritize build time over trace performance 
			/// (may come in handy whem there are frequent updates) 
			/// </summary>
			PREFER_FAST_BUILD = (1 << 3),

			/// <summary> If set, this flag will guarantee that the any-hit shader will be invoked no more than once per primitive during a single trace </summary>
			PREVENT_DUPLICATE_ANY_HIT_INVOCATIONS = (1 << 4),

#if JIMARA_SceneAccelerationStructures_ENABLE_BlasVariant
			/// <summary>
			/// For BLAS-variants, if specified, this flag will tell the system to ignore the base-blas after the initial build
			/// and in case REBUILD_ON_EACH_FRAME and REFIT_ON_REBUILD are set, just use the last state.
			/// <para/> This flag is only relevant for the VariantDesc. It will be automatically removed from BlasDesc;
			/// <para/> The flag will not have any effect, unless REBUILD_ON_EACH_FRAME and REFIT_ON_REBUILD flags are also used;
			/// <para/> First build of the BLAS-variant is always going to use the base-blas. This flag does not effect that.
			/// </summary>
			DO_NOT_USE_BASE_BLAS_AFTER_FIRST_BUILD = (1 << 5)
#endif
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

			/// <summary> First vertex position offset from buffer memory start (in bytes) </summary>
			uint32_t vertexPositionOffset = 0u;

			/// <summary> Interval between vertex position values </summary>
			uint32_t vertexStride = 0u;

			/// <summary> Number of vertices making up the geometry </summary>
			uint32_t vertexCount = 0u;

			/// <summary> Number of triangles making up the geometry </summary>
			uint32_t faceCount = 0u;

			/// <summary> First index offset from indexBuffer origin (in multiples of index-size, based on index format) </summary>
			uint32_t indexOffset = 0u;

			/// <summary> Blas flags </summary>
			Flags flags = Flags::NONE;

			/// <summary> 
			/// There may be an occasion, when we need to share the vertex/index buffer(s), and generate multiple BLAS instances based on different displacements.
			/// To accomodate that requirenment, the descriptor can optionally contain a displacement job, 
			/// that will receive build-time command buffer and displacementJobId as arguments, before the BLAS build command gets executed.
			/// </summary>
			Callback<Graphics::CommandBuffer*, uint64_t> displacementJob = Unused<Graphics::CommandBuffer*, uint64_t>;

			/// <summary> 
			/// There may be an occasion, when we need to share the vertex/index buffer(s), and generate multiple BLAS instances based on different displacements.
			/// To accomodate that requirenment, the descriptor can optionally contain a displacement job, 
			/// that will receive build-time command buffer and displacementJobId as arguments, before the BLAS build command gets executed.
			/// </summary>
			uint64_t displacementJobId = 0u;
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
			/// <para/> Reliable, as long as the user waits for the build-jobs to be completed.
			/// </summary>
			/// <returns> Acceleration structure handle </returns>
			Graphics::BottomLevelAccelerationStructure* AcccelerationStructure()const;

		private:
			// Constructor can only be invoked from within.
			inline Blas() {}
			friend class SceneAccelerationStructures;
		};

		/// <summary>
		/// Creates or retrieves shared instance of a Bottom-Level acceleration structure based on the descriptor.
		/// </summary>
		/// <param name="desc"> Blas descriptor </param>
		/// <returns> Blas instance </returns>
		Reference<Blas> GetBlas(BlasDesc& desc);


#if JIMARA_SceneAccelerationStructures_ENABLE_BlasVariant
		/// <summary>
		/// Descriptor of a Blas-variant
		/// </summary>
		struct JIMARA_API VariantDesc {
			/// <summary> 
			/// Base blas instance to refit from.
			/// <para/> BLAS variants can be used to efficiently create multiple deformations of the same basic shape, defined by baseBlas;
			/// <para/> Also useful, when you might have an 'unstable' vertexBuffer with a known-good base shape;
			/// <para/> If the variant is not configured to update on each frame using the same baseBlas as refit-source, 
			/// the system will take a limited amount of effort to keep the BLAS instance alive for only as long as strictly necessary.
			/// If a longer lifecycle is required for the baseBlas, that should be made sure of externally.
			/// </summary>
			Reference<Blas> baseBlas;

			/// <summary> Vertex buffer (Should be the same format as the baseBlas vertex buffer; same applies to vertex count) </summary>
			Reference<Graphics::ArrayBuffer> vertexBuffer;

			/// <summary> First vertex position offset from buffer memory start (in bytes) </summary>
			uint32_t vertexPositionOffset = 0u;

			/// <summary> Interval between vertex position values </summary>
			uint32_t vertexStride = 0u;

			/// <summary> Blas flags </summary>
			Flags flags = Flags::NONE;

			/// <summary> 
			/// There may be an occasion, when we need to share the vertex/index buffer(s), and generate multiple BLAS instances based on different displacements.
			/// To accomodate that requirenment, the descriptor can optionally contain a displacement job, 
			/// that will receive build-time command buffer and displacementJobId as arguments, before the BLAS build command gets executed.
			/// </summary>
			Callback<Graphics::CommandBuffer*, uint64_t> displacementJob = Unused<Graphics::CommandBuffer*, uint64_t>;

			/// <summary> 
			/// There may be an occasion, when we need to share the vertex/index buffer(s), and generate multiple BLAS instances based on different displacements.
			/// To accomodate that requirenment, the descriptor can optionally contain a displacement job, 
			/// that will receive build-time command buffer and displacementJobId as arguments, before the BLAS build command gets executed.
			/// </summary>
			uint64_t displacementJobId = 0u;
		};

		/// <summary>
		/// Creates or retrieves shared instance of a 'BLAS-variant' acceleration structure by refitting a base-blas.
		/// <para/> If instance is not new, there is no guarantte the derived BLAS will be re-fitted again from the potentially updated baseBlas. 
		/// That will only hold water if REBUILD_ON_EACH_FRAME flag is present;
		/// </summary>
		/// <param name="desc"> Blas variant descriptor </param>
		/// <returns> Blas variant instance </returns>
		Reference<Blas> GetBlas(VariantDesc& desc);
#endif


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
		void CollectBuildJobs(const Callback<JobSystem::Job*> report);

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
			(a.vertexCount == b.vertexCount) &&
			(a.faceCount == b.faceCount) &&
			(a.indexOffset == b.indexOffset) &&
			(a.flags == b.flags) &&
			(a.displacementJob == b.displacementJob) &&
			(a.displacementJobId == b.displacementJobId);
	}

	/// <summary>
	/// Compares two Blas-descriptors
	/// </summary>
	/// <param name="a"> Firs descriptor </param>
	/// <param name="b"> Second descriptor </param>
	/// <returns> True, if the descriptors are not the same </returns>]>
	inline static bool operator!=(const SceneAccelerationStructures::BlasDesc& a, const SceneAccelerationStructures::BlasDesc& b) {
		return !(a == b);
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
				std::hash<decltype(key.vertexCount)>()(key.vertexCount),
				std::hash<decltype(key.faceCount)>()(key.faceCount),
				std::hash<decltype(key.indexOffset)>()(key.indexOffset),
				std::hash<decltype(key.flags)>()(key.flags),
				std::hash<decltype(key.displacementJob)>()(key.displacementJob),
				std::hash<decltype(key.displacementJobId)>()(key.displacementJobId));
		}
	};
}

