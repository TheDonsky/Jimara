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
			float sampleSize = 0.2f;
			
			float maxStepDistance = 0.1f;
			float maxMergeDistance = 0.1f;
			float minAgentHeight = 2.0f;
			float maxSlopeAngle = 30.0f;

			LayerMask surfaceLayers = LayerMask::All();
			LayerMask roofLayers = LayerMask::All();

			size_t simplificationSubsteps = 10u;
			float simplificationAngleThreshold = 10.0f;

			struct JIMARA_STATE_MACHINES_API Serializer : public virtual Serialization::SerializerList::From<Settings> {
				inline Serializer(const std::string_view& name, const std::string_view& hint = "") : Serialization::ItemSerializer(name, hint) {}
				inline virtual ~Serializer() {}

				// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Settings* target)const override;
			};
		};

		enum class JIMARA_STATE_MACHINES_API State {
			UNINITIALIZED = 0u,
			INVALIDATED = 1u,
			SURFACE_SAMPLING = 2u,
			MESH_GENERATION = 3u,
			MESH_CLEANUP = 4u,
			DONE = 5u
		};

		NavMeshBaker(const Settings& settings);

		~NavMeshBaker();

		State Progress(float maxTime);

		State BakeState()const;

		float StateProgress()const;

		Reference<TriMesh> Result();

	private:
		Reference<Object> m_state;

		struct Helpers;
	};
}

