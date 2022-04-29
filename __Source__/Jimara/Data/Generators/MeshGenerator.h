#pragma once
#include "../Mesh.h"


namespace Jimara {
	namespace GenerateMesh {
		namespace Tri {
			/// <summary>
			/// Generates an axis aligned bounding box
			/// </summary>
			/// <param name="start"> "Left-bottom-closest" point </param>
			/// <param name="end"> "Right-top-farthest" point </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Box-shaped mesh instance </returns>
			Reference<TriMesh> Box(const Vector3& start, const Vector3& end, const std::string_view& name = "Box");

			/// <summary>
			/// Generates a spherical mesh
			/// </summary>
			/// <param name="center"> Mesh center </param>
			/// <param name="radius"> Sphere radius </param>
			/// <param name="segments"> Radial segment count </param>
			/// <param name="rings"> Horizontal ring count </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Sphere-shaped mesh instance </returns>
			Reference<TriMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name = "Sphere");

			/// <summary>
			/// Generates a capsule mesh
			/// </summary>
			/// <param name="center"> Mesh center </param>
			/// <param name="radius"> Capsule radius </param>
			/// <param name="midHeight"> Height of the capsule mid section </param>
			/// <param name="segments"> Radial segment count </param>
			/// <param name="tipRings"> Horizontal ring count per upper and lower half-spheres </param>
			/// <param name="midDivisions"> Mid section division count </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Capsule-shaped mesh instance </returns>
			Reference<TriMesh> Capsule(const Vector3& center, float radius, float midHeight, uint32_t segments, uint32_t tipRings, uint32_t midDivisions = 1, const std::string_view& name = "Capsule");

			/// <summary>
			/// Generates a flat rectangular mesh
			/// </summary>
			/// <param name="center"> Gemoetric center </param>
			/// <param name="u"> "U" direction (default assumption is "Right" direction; Vector size directly correlates to plane width; U and V define what "Up" is) </param>
			/// <param name="v"> "V" direction (default assumption is "Forward" direction; Vector size directly correlates to plane length; U and V define what "Up" is) </param>
			/// <param name="divisions"> Number of divisions across U and V axis </param>
			/// <param name="name"> Name of the generated mesh </param>
			/// <returns> Plane-shaped mesh instance </returns>
			Reference<TriMesh> Plane(const Vector3& center, const Vector3& u = Math::Right(), const Vector3& v = Math::Forward(), Size2 divisions = Size2(1, 1), const std::string_view& name = "Plane");
			
			/// <summary>
			/// Creates a cone-shaped mesh
			/// </summary>
			/// <param name="origin"> Center of the cone base </param>
			/// <param name="height"> Cone height </param>
			/// <param name="radius"> Cone base radius </param>
			/// <param name="segments"> Number of vertices at the base circle </param>
			/// <param name="name"> Name of the generated mesh </param>
			/// <returns> Cone-shaped mesh instance </returns>
			Reference<TriMesh> Cone(const Vector3& origin, float height, float radius, uint32_t segments = 32, const std::string_view& name = "Cone");
		}

		namespace Poly {
			/// <summary>
			/// Generates an axis aligned bounding box
			/// </summary>
			/// <param name="start"> "Left-bottom-closest" point </param>
			/// <param name="end"> "Right-top-farthest" point </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Box-shaped mesh instance </returns>
			Reference<PolyMesh> Box(const Vector3& start, const Vector3& end, const std::string_view& name = "Box");

			/// <summary>
			/// Generates a spherical mesh
			/// </summary>
			/// <param name="center"> Mesh center </param>
			/// <param name="radius"> Sphere radius </param>
			/// <param name="segments"> Radial segment count </param>
			/// <param name="rings"> Horizontal ring count </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Sphere-shaped mesh instance </returns>
			Reference<PolyMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name = "Sphere");

			/// <summary>
			/// Generates a capsule mesh
			/// </summary>
			/// <param name="center"> Mesh center </param>
			/// <param name="radius"> Capsule radius </param>
			/// <param name="midHeight"> Height of the capsule mid section </param>
			/// <param name="segments"> Radial segment count </param>
			/// <param name="tipRings"> Horizontal ring count per upper and lower half-spheres </param>
			/// <param name="midDivisions"> Mid section division count </param>
			/// <param name="name"> Name of the object </param>
			/// <returns> Capsule-shaped mesh instance </returns>
			Reference<PolyMesh> Capsule(const Vector3& center, float radius, float midHeight, uint32_t segments, uint32_t tipRings, uint32_t midDivisions = 1, const std::string_view& name = "Capsule");

			/// <summary>
			/// Generates a flat rectangular mesh
			/// </summary>
			/// <param name="center"> Gemoetric center </param>
			/// <param name="u"> "U" direction (default assumption is "Right" direction; Vector size directly correlates to plane width; U and V define what "Up" is) </param>
			/// <param name="v"> "V" direction (default assumption is "Forward" direction; Vector size directly correlates to plane length; U and V define what "Up" is) </param>
			/// <param name="divisions"> Number of divisions across U and V axis </param>
			/// <param name="name"> Name of the generated mesh </param>
			/// <returns> Plane-shaped mesh instance </returns>
			Reference<PolyMesh> Plane(const Vector3& center, const Vector3& u = Math::Right(), const Vector3& v = Math::Forward(), Size2 divisions = Size2(1, 1), const std::string_view& name = "Plane");
		

			/// <summary>
			/// Creates a cone-shaped mesh
			/// </summary>
			/// <param name="origin"> Center of the cone base </param>
			/// <param name="height"> Cone height </param>
			/// <param name="radius"> Cone base radius </param>
			/// <param name="segments"> Number of vertices at the base circle </param>
			/// <param name="name"> Name of the generated mesh </param>
			/// <returns> Cone-shaped mesh instance </returns>
			Reference<PolyMesh> Cone(const Vector3& origin, float height, float radius, uint32_t segments = 32, const std::string_view& name = "Cone");
		}
	}
}
