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
