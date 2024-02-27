#pragma once
#include "../../Types.h"
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
		enum class SurfaceFlags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE,

			/// <summary> 
			/// If set, this flag tells the Surface that the underlying (baked) navigation data 
			/// can be updated asynchronously, if the mesh geometry gets altered.
			/// (Helpful, when there's a complex mesh that gets changed during runtime, potentially causing some hitches otherwise)
			/// </summary>
			UPDATE_ASYNCHRONOUSLY = (1u << 0u)
		};

		/// <summary> Navigation mesh surface </summary>
		class JIMARA_STATE_MACHINES_API Surface : public virtual ConfigurableResource {
		public:
			TriMesh* Mesh()const;

			void SetMesh(TriMesh* mesh);

			float SimplificationAngleThreshold()const;

			void SetSimplificationAngleThreshold(float threshold);

			SurfaceFlags Flags()const;

			void SetFlags(SurfaceFlags flags);

			// TODO: Define further details..
		};

		/// <summary> Instance of a navigation mesh surface </summary>
		class JIMARA_STATE_MACHINES_API SurfaceInstance : public virtual Object {
		public:
			SurfaceInstance(NavMesh* navMesh);

			virtual ~SurfaceInstance();

			Surface* Shape()const;

			void SetShape(Surface* surface);

			Matrix4 Transform()const;

			void SetTransform(const Matrix4& transform);

			bool Enabled()const;

			bool Enable(bool enable);

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
	};
}
