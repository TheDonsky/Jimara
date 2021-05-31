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
			inline physx::PxMat44 Translate(const Matrix4& matrix) {
				physx::PxMat44 mat;
				for (uint8_t i = 0; i < 4; i++)
					for (uint8_t j = 0; j < 4; j++)
						mat[i][j] = matrix[i][j];
				return mat;
			}

			inline Matrix4 Translate(const physx::PxMat44& matrix) {
				Matrix4 mat;
				for (uint8_t i = 0; i < 4; i++)
					for (uint8_t j = 0; j < 4; j++)
						mat[i][j] = matrix[i][j];
				return mat;
			}
		}
	}
}
