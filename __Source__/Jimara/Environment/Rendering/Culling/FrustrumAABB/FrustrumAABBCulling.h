#pragma once
#include "../../../GraphicsSimulation/GraphicsSimulation.h"

#define FrustrumAABBCulling_USE_GENERIC_INPUT_AND_OUTPUT_TYPES

namespace Jimara {
	namespace Culling {
		/// <summary>
		/// Utility for GPU-side AABB-frustrum culling
		/// </summary>
		class JIMARA_API FrustrumAABBCulling : public virtual GraphicsSimulation::Task {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			FrustrumAABBCulling(SceneContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~FrustrumAABBCulling();

			/// <summary>
			/// Updates task configuration
			/// <para/> Only place this is safe to call from is the GraphicsSynchPoint
			/// <para/> CulledInstanceInfo can be any structure, as long as it's size is a multiple of 4; 
			///		it can contain arbitrary data like JM_ObjectIndex, transform or any other per-instance parameter that is relevant to the user.
			/// <para/> InstanceInfo has a few reqired fields and this is the approximate structure the code expects:
			/// <para/>		struct InstanceInfo {
			/// <para/>			struct {
			/// <para/>				alignas(16) Matrix4 instanceTransform;		// Mandatory (the alignment and name)! This represents local boundary transform;
			/// <para/>				alignas(16) Vector3 bboxMin;				// Mandatory (the alignment and name)! Represents local bounding box start;
			/// <para/>				alignas(16) Vector3 bboxMax;				// Mandatory (the alignment and name)! Represents local bounding box end;
			/// <para/>				alignas(4) uint packedViewportSizeRange;	// Mandatory (the alignment and name)! packHalf2x16(minViewportSize, maxViewportSize) 
			///	<para/>															// Where minViewportSize and maxViewportSize 0 to 1 values represent an 'on-screen' size range for the bounding box 
			/// <para/>															// (naturally lends itself for LOD system implementation)
			/// <para/>			} instanceData;
			/// <para/>			struct {
			/// <para/>				CulledInstanceInfo data; // Mandatory (name)! Both alignment and size HAVE TO BE multiple of 4. This will be copied to culledBuffer;
			/// <para/>			} culledInstance;
			/// <para/>		};
			/// <para/> If CulledInstanceInfo contains some mandatory fields from InstanceInfo::instanceData, you can use unions, similarely to how MeshRenderer does it.
			/// <para/>	Mandatory field order and spacing is not required, if preferrable, you are free to rearrange, as long as the alignment requirenment is met.
			/// </summary>
			/// <typeparam name="InstanceInfo"> Instance type </typeparam>
			/// <typeparam name="CulledInstanceInfo"> Culled instance type </typeparam>
			/// <param name="cullingFrustrum"> Frustrum to cull against </param>
			/// <param name="instanceCount"> Number of objects to cull </param>
			/// <param name="instanceBuffer"> Basic per-object instance information </param>
			/// <param name="culledBuffer"> 'Result' buffer, only containing InstanceInfo::culledInstance.data of the instances that pass culling test </param>
			/// <param name="countBuffer"> Buffer that will store number of non-culled instances </param>
			/// <param name="countValueOffset"> Offset from base of the countBuffer where the number will be stored at (HAS TO BE a multiple of 4) </param>
			template<typename InstanceInfo, typename CulledInstanceInfo>
			inline void Configure(
				const Matrix4& cullingFrustrum, const Matrix4& viewportFrustrum, size_t instanceCount,
				Graphics::ArrayBufferReference<InstanceInfo> instanceBuffer,
				Graphics::ArrayBufferReference<CulledInstanceInfo> culledBuffer,
				Reference<Graphics::ArrayBuffer> countBuffer, size_t countValueOffset) {
				static_assert((sizeof(InstanceInfo) % 16u) == 0u);

				const constexpr size_t bboxMinOffset = offsetof(InstanceInfo, instanceData.bboxMin);
				static_assert(sizeof(InstanceInfo::instanceData.bboxMin) >= sizeof(Vector3));
				static_assert((bboxMinOffset % 16u) == 0u);

				const constexpr size_t bboxMaxOffset = offsetof(InstanceInfo, instanceData.bboxMax);
				static_assert(sizeof(InstanceInfo::instanceData.bboxMax) >= sizeof(Vector3));
				static_assert((bboxMaxOffset % 16u) == 0u);

				const constexpr size_t instTransformOffset = offsetof(InstanceInfo, instanceData.instanceTransform);
				static_assert(sizeof(InstanceInfo::instanceData.instanceTransform) >= sizeof(Matrix4));
				static_assert((instTransformOffset % 16u) == 0u);

				const constexpr size_t packedViewportSizeRangeOffset = offsetof(InstanceInfo, instanceData.packedViewportSizeRange);
				static_assert(sizeof(InstanceInfo::instanceData.packedViewportSizeRange) >= sizeof(uint32_t));
				static_assert((packedViewportSizeRangeOffset % 4u) == 0u);

				const constexpr size_t culledDataOffset = offsetof(InstanceInfo, culledInstance.data);
				static_assert(sizeof(InstanceInfo::culledInstance.data) >= sizeof(CulledInstanceInfo));
				static_assert((culledDataOffset % 4u) == 0u);
				static_assert((sizeof(CulledInstanceInfo) % 4u) == 0u);

				Configure(cullingFrustrum, viewportFrustrum, instanceCount, instanceBuffer,
					bboxMinOffset, bboxMaxOffset, instTransformOffset, packedViewportSizeRangeOffset,
					culledBuffer, culledDataOffset, countBuffer, countValueOffset);
			}

		private:
			// Lock for configuration
			SpinLock m_configLock;
			using BindlessBinding = Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding>;

			// Bindless bindings from the last update
			BindlessBinding m_transformsBuffer;
			BindlessBinding m_culledBuffer;
			BindlessBinding m_countBuffer;

			// 'Backend' of the public 'Configure' call
			void Configure(
				const Matrix4& cullingFrustrum, const Matrix4& viewportFrustrum,
				size_t instanceCount, Graphics::ArrayBuffer* instanceBuffer, 
				size_t bboxMinOffset, size_t bboxMaxOffset, size_t instTransformOffset, size_t packedViewportSizeRangeOffset,
				Graphics::ArrayBuffer* culledBuffer, size_t culledDataOffset,
				Graphics::ArrayBuffer* countBuffer, size_t countValueOffset);

			// Actual implementation is hidden behind this...
			struct Helpers;
		};
	}
}
