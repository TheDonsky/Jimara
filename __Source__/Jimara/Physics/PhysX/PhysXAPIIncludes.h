#pragma once

#pragma warning(disable: 26812)
#pragma warning(disable: 26495)
#pragma warning(disable: 26451)
#pragma warning(disable: 33010)

#define PX_PHYSX_STATIC_LIB

#ifdef _DEBUG
#ifdef NDEBUG
#define PX_INCLUDES_NDEBUG_WAS_DEFINED
#undef NDEBUG
#endif
#else
#ifndef NDEBUG
#define PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
#define NDEBUG
#endif
#endif

#include "PxPhysicsAPI.h"
#include "PxPhysicsVersion.h"

#ifdef PX_INCLUDES_NDEBUG_WAS_DEFINED
#undef PX_INCLUDES_NDEBUG_WAS_DEFINED
#define NDEBUG
#else
#ifdef PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
#undef PX_INCLUDES_NDEBUG_WAS_NOT_DEFINED
#undef NDEBUG
#endif
#endif

#pragma warning(default: 26812)
#pragma warning(default: 26495)
#pragma warning(default: 26451)
#pragma warning(default: 33010)


#include "../../Math/Math.h"
namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// Translates Matrix4 to physx::PxMat44
			/// </summary>
			/// <param name="matrix"> Matrix4 </param>
			/// <returns> physx::PxMat44 </returns>
			inline static physx::PxMat44 Translate(const Matrix4& matrix) {
				physx::PxMat44 mat;
				for (uint8_t i = 0; i < 4; i++)
					for (uint8_t j = 0; j < 4; j++)
						mat[i][j] = matrix[i][j];
				return mat;
			}

			/// <summary>
			/// Translates physx::PxMat44 to Matrix4
			/// </summary>
			/// <param name="matrix"> physx::PxMat44 </param>
			/// <returns> Matrix4 </returns>
			inline static Matrix4 Translate(const physx::PxMat44& matrix) {
				Matrix4 mat;
				for (uint8_t i = 0; i < 4; i++)
					for (uint8_t j = 0; j < 4; j++)
						mat[i][j] = matrix[i][j];
				return mat;
			}


			/// <summary>
			/// Translates Vector3 to physx::PxVec3
			/// </summary>
			/// <param name="vector"> Vector3 </param>
			/// <returns> physx::PxVec3 </returns>
			inline static physx::PxVec3 Translate(const Vector3& vector) { return physx::PxVec3(vector.x, vector.y, vector.z); }

			/// <summary>
			/// Translates physx::PxVec3 to Vector3
			/// </summary>
			/// <param name="vector"> physx::PxVec3 </param>
			/// <returns> Vector3 </returns>
			inline static Vector3 Translate(const physx::PxVec3& vector) { return Vector3(vector.x, vector.y, vector.z); }


			/// <summary>
			/// Reference counter for physx objects (for PhysXReference)
			/// </summary>
			struct PhysXReferenceCounter {
				/// <summary>
				/// invokes object->acquireReference()
				/// </summary>
				/// <typeparam name="ObjectType"> PhysX object type </typeparam>
				/// <param name="object"> PhysX object </param>
				template<typename ObjectType>
				inline static void AddReference(ObjectType* object) { if (object != nullptr) object->acquireReference(); }

				/// <summary>
				/// invokes object->release()
				/// </summary>
				/// <typeparam name="ObjectType"> PhysX object type </typeparam>
				/// <param name="object"> PhysX object </param>
				template<typename ObjectType>
				inline static void ReleaseReference(ObjectType* object) { if (object != nullptr) object->release(); }
			};

			/// <summary>
			/// Reference specification for PhysX objects
			/// </summary>
			/// <typeparam name="ObjectType"> PhysX object type </typeparam>
			template<typename ObjectType>
			class PhysXReference : public Reference<ObjectType, PhysXReferenceCounter> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="address"> PhysX object </param>
				inline PhysXReference(ObjectType* address = nullptr) : Reference<ObjectType, PhysXReferenceCounter>(address) {}
			};
		}
	}
}
