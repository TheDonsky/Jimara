#pragma once
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


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
			/// Information about an instance
			/// </summary>
			struct JIMARA_API CulledInstanceInfo {
				/// <summary> Instance transform </summary>
				alignas(16) Matrix4 transform = {};

				/// <summary> JM_ObjectIndex </summary>
				alignas(4) uint32_t index = 0u;
			};
			static_assert(sizeof(CulledInstanceInfo) == (16u * 5u));

			/// <summary>
			/// Updates task configuration
			/// <para/> Only place this is safe to call from is the GraphicsSynchPoint
			/// </summary>
			/// <param name="frustrum"> Frustrum to cull against </param>
			/// <param name="boundaries"> Identity-transform boundaries </param>
			/// <param name="transformCount"> Number of transforms to cull </param>
			/// <param name="transforms"> Instance information </param>
			/// <param name="culledBuffer"> Instances that pass culling test will be stored here </param>
			/// <param name="countBuffer"> Buffer that will store number of non-culled instances </param>
			/// <param name="countValueOffset"> Offset from base of the countBuffer where the number will be stored at (HAS TO BE a multiple of 4) </param>
			void Configure(
				const Matrix4& frustrum,
				const AABB& boundaries,
				size_t transformCount,
				Graphics::ArrayBufferReference<CulledInstanceInfo> transforms,
				Graphics::ArrayBufferReference<CulledInstanceInfo> culledBuffer,
				Reference<Graphics::ArrayBuffer> countBuffer,
				size_t countValueOffset);

		private:
			// Lock for configuration
			SpinLock m_configLock;
			using BindlessBinding = Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding>;

			// Bindless bindings from the last update
			BindlessBinding m_transformsBuffer;
			BindlessBinding m_culledBuffer;
			BindlessBinding m_countBuffer;

			// Actual implementation is hidden behind this...
			struct Helpers;
		};
	}
}
