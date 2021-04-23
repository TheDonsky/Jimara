#pragma once
#include "../../../Core/Object.h"
#include "../../../Math/Math.h"

namespace Jimara {
	/// <summary>
	/// Object that describes a light within the graphics scene
	/// </summary>
	class LightDescriptor : public virtual Object {
	public:
		/// <summary>
		/// Information about the light
		/// </summary>
		struct LightInfo {
			/// <summary> Light type identifier </summary>
			uint32_t typeId;

			/// <summary> Light data </summary>
			const void* data;

			/// <summary> Size of data in bytes </summary>
			size_t dataSize;
		};

		/// <summary> Information about the light </summary>
		virtual LightInfo GetLightInfo()const = 0;

		/// <summary> Axis aligned bounding box, within which the light is relevant </summary>
		virtual AABB GetLightBounds()const = 0;
	};
}
