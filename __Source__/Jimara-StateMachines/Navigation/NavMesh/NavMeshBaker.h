#pragma once
#include "../../Types.h"
#include <Jimara/Components/Physics/Collider.h>
#include <Jimara/Data/Geometry/Mesh.h>
#include <Jimara/Environment/Layers.h>


namespace Jimara {
	class JIMARA_STATE_MACHINES_API NavMeshBaker {
	public:
		struct JIMARA_STATE_MACHINES_API Settings {
			Component* environmentRoot = nullptr;

			Matrix4 volumePose = Math::Identity();
			Vector3 volumeSize = Vector3(0.0f);
			Size2 verticalSampleCount = Size2(100u);
			
			float maxStepDistance = 0.1f;
			float maxMergeDistance = 0.1f;
			float minAgentHeight = 2.0f;
			float maxSlopeAngle = 30.0f;

			LayerMask surfaceLayers = LayerMask::All();
			LayerMask roofLayers = LayerMask::All();

			float simplificationAngleThreshold = 5.0f;
		};

		enum class JIMARA_STATE_MACHINES_API State {
			UNINITIALIZED = 0u,
			INVALIDATED = 1u,
			SCENE_SAMPLING = 2u,
			SURFACE_FILTERING = 3u,
			MESH_GENERATION = 4u,
			MESH_CLEANUP = 5u,
			DONE = 6u
		};

		NavMeshBaker(const Settings& settings);

		~NavMeshBaker();

		State Progress(float maxTime);

		State BakeState()const;

		Reference<TriMesh> Result();

	private:
		Reference<Object> m_state;

		struct Helpers;
	};
}

