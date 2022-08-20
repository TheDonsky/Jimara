#pragma once
#include "../../../Scene/SceneObjectCollection.h"
#include "../../ViewportDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Object that describes a light within the graphics scene
	/// </summary>
	class JIMARA_API LightDescriptor : public virtual Object {
	public:
		/// <summary>
		/// Information about the light
		/// </summary>
		struct JIMARA_API LightInfo {
			/// <summary> Light type identifier </summary>
			uint32_t typeId;

			/// <summary> Light data </summary>
			const void* data;

			/// <summary> Size of data in bytes </summary>
			size_t dataSize;
		};

		/// <summary>
		/// Per-viewport light data provider
		/// </summary>
		class JIMARA_API ViewportData : public virtual Object {
		public:
			/// <summary> Information about the light </summary>
			virtual LightInfo GetLightInfo()const = 0;

			/// <summary> Axis aligned bounding box, within which the light is relevant (world space) </summary>
			virtual AABB GetLightBounds()const = 0;
		};

		/// <summary>
		/// Retrieves viewport-specific light data
		/// </summary>
		/// <param name="viewport"> "Target viewport" (can be nullptr and that specific case means the "default" descriptor, whatever that means for each light type) </param>
		/// <returns> Per-viewport light data provider </returns>
		virtual Reference<const ViewportData> GetViewportData(const ViewportDescriptor* viewport) = 0;

		/// <summary>
		/// SceneObjectCollection<LightDescriptor> will flush on Scene::GraphicsContext::OnGraphicsSynch
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Scene::GraphicsContext::OnGraphicsSynch </returns>
		static Event<>& OnFlushSceneObjectCollections(SceneContext* context) { return context->Graphics()->OnGraphicsSynch(); }

		/// <summary> Set of all LightDescriptors tied to a scene </summary>
		typedef SceneObjectCollection<LightDescriptor> Set;
	};
}
