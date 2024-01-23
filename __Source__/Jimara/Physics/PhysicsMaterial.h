#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"
#include "../Data/Serialization/Serializable.h"
#include "../Data/AssetDatabase/AssetDatabase.h"


namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Physics material
		/// </summary>
		class JIMARA_API PhysicsMaterial : public virtual Resource, public virtual Serialization::Serializable {
		public:
			/// <summary> Static friction ceofficient </summary>
			virtual float StaticFriction()const = 0;

			/// <summary>
			/// Sets static friction ceofficient
			/// </summary>
			/// <param name="friction"> New value </param>
			virtual void SetStaticFriction(float friction) = 0;

			/// <summary> Dynamic friction ceofficient </summary>
			virtual float DynamicFriction()const = 0;

			/// <summary>
			/// Sets dynamic friction ceofficient
			/// </summary>
			/// <param name="friction"> New value </param>
			virtual void SetDynamicFriction(float friction) = 0;

			/// <summary> Material bounciness </summary>
			virtual float Bounciness()const = 0;

			/// <summary>
			/// Alters material bounciness
			/// </summary>
			/// <param name="bounciness"> New value (anything beyond 1.0f is 'non-physical') </param>
			virtual void SetBounciness(float bounciness) = 0;

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override;
		};
	}
}
