#pragma once
#include "../../Types.h"
#include <Jimara/Core/Collections/VoxelGrid.h>
#include <Jimara/Math/Primitives/Triangle.h>
#include <Jimara/Components/Transform.h>
#include <Jimara/Data/Geometry/Mesh.h>
#include <Jimara/Data/ConfigurableResource.h>


namespace Jimara {
	class JIMARA_STATE_MACHINES_API NavMesh : public virtual Object {
	public:
		/// <summary> Flags for agents </summary>
		enum class JIMARA_STATE_MACHINES_API AgentFlags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0u,

			/// <summary> 
			/// If this flag is not set, the NavMesh assumes the agent can "walk on walls" and 
			/// maxTiltAngle only matters when it comes to neighboring surface normals
			/// </summary>
			FIXED_UP_DIRECTION = (1u << 0u)
		};

		/// <summary> General information about an agent </summary>
		struct JIMARA_STATE_MACHINES_API AgentOptions {
			/// <summary> Flags </summary>
			AgentFlags flags = AgentFlags::FIXED_UP_DIRECTION;
			
			/// <summary> Agent radius </summary>
			float radius = 0.0f;

			/// <summary> Agent height </summary>
			float height = 1.0f;

			/// <summary> Maximal slope angle the agent can climb </summary>
			float maxTiltAngle = 20.0f;
		};

		/// <summary> Navigation path waypoint </summary>
		struct JIMARA_STATE_MACHINES_API PathNode {
			/// <summary> 'Foot' Position </summary>
			Vector3 position = Vector3(0.0f);

			/// <summary> NavMesh surface normal at the position </summary>
			Vector3 normal = Math::Up();
		};
		
		/// <summary> Flags for navigation mesh surfaces </summary>
		enum class JIMARA_STATE_MACHINES_API SurfaceFlags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE,

			/// <summary> 
			/// If set, this flag tells the Surface that the underlying (baked) navigation data 
			/// can be updated asynchronously, if the mesh geometry gets altered.
			/// <para/> Notes:
			/// <para/>		0. Helpful, when there's a complex mesh that gets changed during runtime, potentially causing some hitches otherwise;
			/// <para/>		1. This Only applies to changes due to MeshDirty events; Field modifications will still have immediate effects.
			/// </summary>
			UPDATE_ASYNCHRONOUSLY = (1u << 0u)
		};

		/// <summary> Unified surface settings </summary>
		struct JIMARA_STATE_MACHINES_API SurfaceSettings {
			/// <summary> Navigation surface geometry </summary>
			Reference<TriMesh> mesh;

			/// <summary> Surface bake flags </summary>
			SurfaceFlags flags = SurfaceFlags::NONE;
		};

		/// <summary>
		/// "Baked" surface data
		/// </summary>
		struct JIMARA_STATE_MACHINES_API BakedSurfaceData : public virtual Object {
			/// <summary> 
			/// 'Reduced' mesh geometry 
			/// <para/> Not directly used internally by the NavMesh, but Editor can use it for display and the agents 
			/// and/or other parts of the code are free to use it for decision-making.
			/// <para/> Modifying the content of this mesh is not adviced.
			/// </summary>
			Reference<TriMesh> geometry;

			/// <summary> Octree, generated from geometry triangles </summary>
			Octree<Triangle3> octree;

			/// <summary> For each triangle, a list of neighboring face indices </summary>
			std::vector<Stacktor<uint32_t, 3u>> triNeighbors;
		};

		/// <summary> Navigation mesh surface </summary>
		class JIMARA_STATE_MACHINES_API Surface : public virtual ConfigurableResource {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="createArgs"> Basic environment information </param>
			Surface(const ConfigurableResource::CreateArgs& createArgs);

			/// <summary> Virtual destructor </summary>
			virtual ~Surface();

			/// <summary> Surface settings </summary>
			SurfaceSettings Settings()const;

			/// <summary> Surface settings </summary>
			Property<SurfaceSettings> Settings();

			/// <summary>
			/// Baked surface data
			/// <para/> Will be nullptr, if the surface mesh is not set
			/// </summary>
			Reference<const BakedSurfaceData> Data()const;

			/// <summary> Invoked each time the last surface data gets invalidated </summary>
			Event<>& OnDirty()const;

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		private:
			// Underlying data
			const Reference<Object> m_data;
			struct SurfaceHelpers;
		};

		/// <summary> Instance of a navigation mesh surface </summary>
		class JIMARA_STATE_MACHINES_API SurfaceInstance : public virtual Object {
		public:
			SurfaceInstance(NavMesh* navMesh);

			virtual ~SurfaceInstance();

			const Surface* Shape()const;

			Property<const Surface*> Shape();

			Matrix4 Transform()const;

			Property<Matrix4> Transform();

			bool Enabled()const;

			Property<bool> Enabled();

		private:
			std::recursive_mutex m_lock;
			const Reference<NavMesh> m_navMesh;
			Reference<const Surface> m_shape;
			Matrix4 m_transform = Math::Identity();
			std::atomic_bool m_enabled = true;
			// TODO: Define further details..
		};

		/// <summary>
		/// Shared navigation mesh instance for given scene context
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared instance for the context </returns>
		static Reference<NavMesh> Instance(const SceneContext* context);

		/// <summary>
		/// Creates a new instance of a NavMesh
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> New NavMesh </returns>
		static Reference<NavMesh> Create(const SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~NavMesh();

		/// <summary>
		/// Calculates agent path between two points
		/// <para/> Notes:
		/// <para/>		0. If the return value is empty, that means no path was found;
		/// <para/>		1. The first element in the path chain will be the point on NavMesh closest to the start, but will not necessarily match it exactly;
		/// <para/>		2. The last element in the path chain will be the point on NavMesh closest to the end, but will not necessarily match it exactly.
		/// </summary>
		/// <param name="start"> Start point (or something close to it) </param>
		/// <param name="end"> Destination/End point </param>
		/// <param name="agentUp"> Agent's 'up' direction/orientation </param>
		/// <param name="agentOptions"> Agent information </param>
		/// <returns> Path between start and end points for the given agent </returns>
		std::vector<PathNode> CalculatePath(Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions);

	private:
		// Underlying data
		const Reference<Object> m_data;
		struct Helpers;
	};


	/// <summary>
	/// "And" operator for NavMesh::SurfaceFlags
	/// </summary>
	/// <param name="a"> NavMesh::SurfaceFlags </param>
	/// <param name="b"> NavMesh::SurfaceFlags </param>
	/// <returns> a & b </returns>
	inline static constexpr NavMesh::SurfaceFlags operator&(NavMesh::SurfaceFlags a, NavMesh::SurfaceFlags b) {
		return static_cast<NavMesh::SurfaceFlags>(
			static_cast<std::underlying_type_t<NavMesh::SurfaceFlags>>(a) &
			static_cast<std::underlying_type_t<NavMesh::SurfaceFlags>>(b));
	}

	/// <summary>
	/// "Or" operator for NavMesh::SurfaceFlags
	/// </summary>
	/// <param name="a"> NavMesh::SurfaceFlags </param>
	/// <param name="b"> NavMesh::SurfaceFlags </param>
	/// <returns> a | b </returns>
	inline static constexpr NavMesh::SurfaceFlags operator|(NavMesh::SurfaceFlags a, NavMesh::SurfaceFlags b) {
		return static_cast<NavMesh::SurfaceFlags>(
			static_cast<std::underlying_type_t<NavMesh::SurfaceFlags>>(a) |
			static_cast<std::underlying_type_t<NavMesh::SurfaceFlags>>(b));
	}
}
