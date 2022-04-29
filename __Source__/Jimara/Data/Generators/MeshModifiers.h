#pragma once
#include "../Mesh.h"


namespace Jimara {
	namespace ModifyMesh {
		namespace Tri {
			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="mesh"> Source mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Flat-shaded copy of the mesh </returns>
			Reference<TriMesh> ShadedFlat(const TriMesh* mesh, const std::string_view& name);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="mesh"> Source mesh </param>
			/// <returns> Flat-shaded copy of the mesh </returns>
			Reference<TriMesh> ShadedFlat(const TriMesh* mesh);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="transformation"> Transformation matrix </param>
			/// <param name="mesh"> Source mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Transformed mesh </returns>
			Reference<TriMesh> Transformed(const Matrix4& transformation, const TriMesh* mesh, const std::string_view& name);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="transformation"> Transformation matrix </param>
			/// <param name="mesh"> Source mesh </param>
			/// <returns> Transformed mesh </returns>
			Reference<TriMesh> Transformed(const Matrix4& transformation, const TriMesh* mesh);

			/// <summary>
			/// Generates a mesh that has 'unified geoemtry' form two meshes
			/// </summary>
			/// <param name="a"> First mesh </param>
			/// <param name="b"> Second mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Combined mesh </returns>
			Reference<TriMesh> Merge(const TriMesh* a, const TriMesh* b, const std::string_view& name);
		}

		namespace Poly {
			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="mesh"> Source mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Flat-shaded copy of the mesh </returns>
			Reference<PolyMesh> ShadedFlat(const PolyMesh* mesh, const std::string_view& name);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="mesh"> Source mesh </param>
			/// <returns> Flat-shaded copy of the mesh </returns>
			Reference<PolyMesh> ShadedFlat(const PolyMesh* mesh);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="transformation"> Transformation matrix </param>
			/// <param name="mesh"> Source mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Transformed mesh </returns>
			Reference<PolyMesh> Transformed(const Matrix4& transformation, const PolyMesh* mesh, const std::string_view& name);

			/// <summary>
			/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
			/// <para/> Note: Does not generate a skinned mesh
			/// </summary>
			/// <param name="transformation"> Transformation matrix </param>
			/// <param name="mesh"> Source mesh </param>
			/// <returns> Transformed mesh </returns>
			Reference<PolyMesh> Transformed(const Matrix4& transformation, const PolyMesh* mesh);
			
			/// <summary>
			/// Generates a mesh that has 'unified geoemtry' form two meshes
			/// </summary>
			/// <param name="a"> First mesh </param>
			/// <param name="b"> Second mesh </param>
			/// <param name="name"> Generated mesh name </param>
			/// <returns> Combined mesh </returns>
			Reference<PolyMesh> Merge(const PolyMesh* a, const PolyMesh* b, const std::string_view& name);

		}
	}
}
