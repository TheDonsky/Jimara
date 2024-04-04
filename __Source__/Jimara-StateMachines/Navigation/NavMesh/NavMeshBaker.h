#pragma once
#include "../../Types.h"
#include <Jimara/Components/Physics/Collider.h>
#include <Jimara/Data/Geometry/Mesh.h>
#include <Jimara/Environment/Layers.h>


namespace Jimara {
	/// <summary>
	/// An utility for baking Navigation Mesh surface from the scene geometry
	/// </summary>
	class JIMARA_STATE_MACHINES_API NavMeshBaker {
	public:
		/// <summary>
		/// Settings for the bake process
		/// </summary>
		struct JIMARA_STATE_MACHINES_API Settings {
			/// <summary> Geometry will be baked based on the colliders from the Component subtree consisting of this Component's children </summary>
			Component* environmentRoot = nullptr;

			/// <summary> Rotation & position of the processed boundary in world-space (Raycasts will be performed in 'down' direction) </summary>
			Matrix4 volumePose = Math::Identity();

			/// <summary> Volume to process (in volumePose space; center is volumePose position) </summary>
			Vector3 volumeSize = Vector3(0.0f);

			/// <summary> Ray-samples will be taken with this interval in-between (higher values will give more accurate results, but it'll take more time and RAM) </summary>
			float sampleSize = 0.2f;
			
			/// <summary> When generating the mesh, samples quads with vertical distance no larger than this will be turned into "walkable" geometry </summary>
			float maxStepDistance = 0.1f;

			/// <summary> Samples in the same column will be merged if the distance between them is less than this value </summary>
			float maxMergeDistance = 0.1f;

			/// <summary> If distance between a sample and the roof above is smaller than this, sample will be discarded </summary>
			float minAgentHeight = 2.0f;

			/// <summary> Maximal slope angle between volumePose 'up' direction and collider's normal for the sample to be considered as "walkable" geometry </summary>
			float maxSlopeAngle = 30.0f;

			/// <summary> Surface collider layer mask </summary>
			LayerMask surfaceLayers = LayerMask::All();

			/// <summary> Roof collider layer mask </summary>
			LayerMask roofLayers = LayerMask::All();

			/// <summary> 
			/// After generation, mesh is optionally smoothed before being simplified. 
			/// This is the number, controlling the smoothing passes. 
			/// </summary>
			size_t meshSmoothingSteps = 2u;

			/// <summary> 
			/// Mesh is simplified over multiple substeps, where the angle and edge size thresholds are gradually increased. 
			/// This is the number of those substeps 
			/// </summary>
			size_t simplificationSubsteps = 10u;

			/// <summary> 
			/// Edges that are shorter than this value will be removed 
			/// (value will grow to this by the last simplification iteration) 
			/// </summary>
			float edgeLengthThreshold = 0.25f;

			/// <summary> 
			/// Vertices that have faces around that deviate from the average normal by no more than this amount will be removed 
			/// (value will grow to this by the last simplification iteration) 
			/// </summary>
			float simplificationAngleThreshold = 10.0f;

			/// <summary>
			/// Serializer for NavMeshBaker::Settings
			/// </summary>
			struct JIMARA_STATE_MACHINES_API Serializer : public virtual Serialization::SerializerList::From<Settings> {
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="name"> Serializer name </param>
				/// <param name="hint"> Serializer hint </param>
				inline Serializer(const std::string_view& name, const std::string_view& hint = "") : Serialization::ItemSerializer(name, hint) {}

				/// <summary> Virtual destructor </summary>
				inline virtual ~Serializer() {}

				// <summary>
				/// Gives access to sub-serializers/fields
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object </param>
				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Settings* target)const override;
			};
		};

		/// <summary> Bake process state </summary>
		enum class JIMARA_STATE_MACHINES_API State {
			/// <summary> Empty/Invalid configuration (can not progress) </summary>
			UNINITIALIZED = 0u,

			/// <summary> Process was created successfully, but something wnet wrong during processing (for example, environmentRoot being deleted can cause this) </summary>
			INVALIDATED = 1u,
			
			/// <summary> Scene geometry is being sampled </summary>
			SURFACE_SAMPLING = 2u,

			/// <summary> Process is "in the middle" of generating a mesh </summary>
			MESH_GENERATION = 3u,

			/// <summary> After generation, we have a few smoothing passes </summary>
			MESH_SMOOTHING = 4u,

			/// <summary> After smoothing, the mesh is simplified such that there are no 'extra' vertices on surfaces that are flat enough </summary>
			MESH_SIMPLIFICATION = 5u,

			/// <summary> Processing is done; You can get the result mesh without waiting </summary>
			DONE = 6u
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="settings"> Bake settings </param>
		NavMeshBaker(const Settings& settings);

		/// <summary> Destructor </summary>
		virtual ~NavMeshBaker();

		/// <summary>
		/// Performs baking substeps untill it's invalidated/done or enough time elapes
		/// </summary>
		/// <param name="maxTime"> After each substep is performed, the baker checks if elapsed time exceeds this value and if so, terminates early </param>
		/// <returns> Current bake state </returns>
		State Progress(float maxTime);

		/// <summary> Current bake state </summary>
		State BakeState()const;

		/// <summary> Fraction, depicting roughly how much progress has been made for current bake state </summary>
		float StateProgress()const;

		/// <summary> Progresses till completion/failure and returns generated surface geometry </summary>
		Reference<TriMesh> Result();

	private:
		Reference<Object> m_state;

		struct Helpers;
	};
}
