#pragma once
#include "Mesh.h"


namespace Jimara {
	namespace ModifyMesh {
		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		JIMARA_API Reference<TriMesh> ShadeFlat(const TriMesh* mesh, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		JIMARA_API Reference<TriMesh> ShadeFlat(const TriMesh* mesh);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		JIMARA_API Reference<PolyMesh> ShadeFlat(const PolyMesh* mesh, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		JIMARA_API Reference<PolyMesh> ShadeFlat(const PolyMesh* mesh);





		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <param name="transformation"> Transformation matrix </param>
		/// <returns> Transformed mesh </returns>
		JIMARA_API Reference<TriMesh> Transform(const TriMesh* mesh, const Matrix4& transformation, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="transformation"> Transformation matrix </param>
		/// <returns> Transformed mesh </returns>
		JIMARA_API Reference<TriMesh> Transform(const TriMesh* mesh, const Matrix4& transformation);
		
		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="transformation"> Transformation matrix </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Transformed mesh </returns>
		JIMARA_API Reference<PolyMesh> Transform(const PolyMesh* mesh, const Matrix4& transformation, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but transformed (rotated/moved/scaled/whatever)
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="transformation"> Transformation matrix </param>
		/// <returns> Transformed mesh </returns>
		JIMARA_API Reference<PolyMesh> Transform(const PolyMesh* mesh, const Matrix4& transformation);





		/// <summary>
		/// Generates a mesh that has 'unified geoemtry' form two meshes
		/// </summary>
		/// <param name="a"> First mesh </param>
		/// <param name="b"> Second mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Combined mesh </returns>
		JIMARA_API Reference<TriMesh> Merge(const TriMesh* a, const TriMesh* b, const std::string_view& name);

		/// <summary>
		/// Generates a mesh that has 'unified geoemtry' form two meshes
		/// </summary>
		/// <param name="a"> First mesh </param>
		/// <param name="b"> Second mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Combined mesh </returns>
		JIMARA_API Reference<PolyMesh> Merge(const PolyMesh* a, const PolyMesh* b, const std::string_view& name);
	}
}
