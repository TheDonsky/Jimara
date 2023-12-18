#pragma once
#include "Mesh.h"

namespace Jimara {
	namespace MeshConstants {
		namespace Tri {
			/// <summary> 
			/// 'Shared' unit cube mesh instance (start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Cube();

			/// <summary>
			/// 'Shared' unit cube made from edge lines (Only viable for WIRE-type rendering; start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> WireCube();

			/// <summary> 
			/// 'Shared' unit sphere mesh instance (Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Sphere();

			/// <summary>
			/// 'Shared' unit sphere made from three circular lines (Only viable for WIRE-type rendering; Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> WireSphere();

			/// <summary>
			/// 'Shared' capsule mesh instance (Radius = 1.0f; midHeight = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Capsule();

			/// <summary>
			/// 'Shared' capsule-shaped wireframe thingie (Only viable for WIRE-type rendering)
			/// </summary>
			/// <param name="radius"> Shape radius </param>
			/// <param name="height"> Capsule mid-section height </param>
			/// <returns> Instance of a 'wire-capsule' </returns>
			JIMARA_API Reference<TriMesh> WireCapsule(float radius = 1.0f, float height = 1.0f);

			/// <summary>
			/// 'Shared' cylinder mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Cylinder();

			/// <summary>
			/// 'Shared' cone mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Cone();

			/// <summary>
			/// 'Shared' torus mesh instance (Major Radius = 1.0f; Minor Radius = 0.5f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Torus();

			/// <summary>
			/// 'Shared' unit circle made from three circular lines (Only viable for WIRE-type rendering; Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> WireCircle();

			/// <summary>
			/// 'Shared' plane mesh instance (start = Vector3(-0.5f, 0.0f, -0.5f), end = Vector3(0.5f, 0.0f, 0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<TriMesh> Plane();
		}

		namespace Poly {
			/// <summary> 
			/// 'Shared' unit cube mesh instance (start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Cube();

			/// <summary> 
			/// 'Shared' unit sphere mesh instance (Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Sphere();

			/// <summary>
			/// 'Shared' capsule mesh instance (Radius = 1.0f; midHeight = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Capsule();

			/// <summary>
			/// 'Shared' cylinder mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Cylinder();

			/// <summary>
			/// 'Shared' cone mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Cone();

			/// <summary>
			/// 'Shared' torus mesh instance (Major Radius = 1.0f; Minor Radius = 0.5f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Torus();

			/// <summary>
			/// 'Shared' plane mesh instance (start = Vector3(-0.5f, 0.0f, -0.5f), end = Vector3(0.5f, 0.0f, 0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			JIMARA_API Reference<PolyMesh> Plane();
		}
	}
}
