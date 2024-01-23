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
			/// <summary>
			/// Combine mode for friction/bouncincess
			/// </summary>
			enum class JIMARA_API CombineMode : uint8_t {
				/// <summary> (a + b) / 2.0f </summary>
				AVERAGE = 0,

				/// <summary> (a < b) ? a : b </summary>
				MIN = 1u,

				/// <summary> a * b </summary>
				MULTIPLY = 2u,

				/// <summary> (a > b) ? a : b </summary>
				MAX = 3u,

				/// <summary> Number of available options </summary>
				MODE_COUNT = 4u
			};

			/// <summary> Enumeration attribute for combine mode </summary>
			static const Object* CombineModeEnumAttribute();

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

			/// <summary> Combine mode for the friction settings </summary>
			virtual CombineMode FrictionCombineMode()const = 0;

			/// <summary>
			/// Sets combine mode for friction settings
			/// </summary>
			/// <param name="mode"> Combine mode </param>
			virtual void SetFrictionCombineMode(CombineMode mode) = 0;

			/// <summary> Material bounciness </summary>
			virtual float Bounciness()const = 0;

			/// <summary>
			/// Alters material bounciness
			/// </summary>
			/// <param name="bounciness"> New value (anything beyond 1.0f is 'non-physical') </param>
			virtual void SetBounciness(float bounciness) = 0;

			/// <summary> Combine mode for the bounciness </summary>
			virtual CombineMode BouncinessCombineMode()const = 0;

			/// <summary>
			/// Sets combine mode for bounciness
			/// </summary>
			/// <param name="mode"> Combine mode </param>
			virtual void SetBouncinessCombineMode(CombineMode mode) = 0;

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override;
		};
	}
}
