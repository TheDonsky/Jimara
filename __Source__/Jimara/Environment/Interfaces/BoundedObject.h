#pragma once
#include "../../Core/Object.h"
#include "../../Math/Math.h"


namespace Jimara {
	/// <summary>
	/// Arbitrary Object with boundaries
	/// </summary>
	class JIMARA_API BoundedObject {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~BoundedObject() {}

		/// <summary> Retrieves object boundaries </summary>
		virtual AABB GetBoundaries()const = 0;
	};
}
