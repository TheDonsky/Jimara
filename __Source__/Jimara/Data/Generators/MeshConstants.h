#pragma once
#include "../Mesh.h"

namespace Jimara {
	namespace MeshContants {
		namespace Tri {
			/// <summary> 
			/// 'Shared' unit cube mesh instance (start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Cube();

			/// <summary>
			/// 'Shared' unit cube made from edge lines (Only viable for WIRE-type rendering; start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> WireCube();

			/// <summary> 
			/// 'Shared' unit sphere mesh instance (Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Sphere();

			/// <summary>
			/// 'Shared' unit sphere made from three circular lines (Only viable for WIRE-type rendering; Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> WireSphere();

			/// <summary>
			/// 'Shared' capsule mesh instance (Radius = 1.0f; midHeight = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Capsule();

			/// <summary>
			/// 'Shared' capsule-shaped wireframe thingie (Only viable for WIRE-type rendering)
			/// </summary>
			/// <param name="radius"> Shape radius </param>
			/// <param name="height"> Capsule mid-section height </param>
			/// <returns> Instance of a 'wire-capsule' </returns>
			Reference<TriMesh> WireCapsule(float radius = 1.0f, float height = 1.0f);

			/// <summary>
			/// 'Shared' cylinder mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Cylinder();

			/// <summary>
			/// 'Shared' cone mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Cone();

			/// <summary>
			/// 'Shared' torus mesh instance (Major Radius = 1.0f; Minor Radius = 0.5f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Torus();

			/// <summary>
			/// 'Shared' plane mesh instance (start = Vector3(-0.5f, 0.0f, -0.5f), end = Vector3(0.5f, 0.0f, 0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<TriMesh> Plane();
		}

		namespace Poly {
			/// <summary> 
			/// 'Shared' unit cube mesh instance (start = Vector3(-0.5f), end = Vector3(0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Cube();

			/// <summary> 
			/// 'Shared' unit sphere mesh instance (Radius = 1.0f) 
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Sphere();

			/// <summary>
			/// 'Shared' capsule mesh instance (Radius = 1.0f; midHeight = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Capsule();

			/// <summary>
			/// 'Shared' cylinder mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Cylinder();

			/// <summary>
			/// 'Shared' cone mesh instance (Radius = 1.0f; height = 1.0f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Cone();

			/// <summary>
			/// 'Shared' torus mesh instance (Major Radius = 1.0f; Minor Radius = 0.5f)
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Torus();

			/// <summary>
			/// 'Shared' plane mesh instance (start = Vector3(-0.5f, 0.0f, -0.5f), end = Vector3(0.5f, 0.0f, 0.5f))
			/// <para/> Note: the mesh has a global asset, but it will not be accessible through asset database.
			/// </summary>
			Reference<PolyMesh> Plane();
		}
	}
}