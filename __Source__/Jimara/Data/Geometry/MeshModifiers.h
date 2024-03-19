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
		/// Takes a mesh and generates another mesh with identical geometry, 
		/// but shaded smooth with vertices merged based on their positions
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <param name="ignoreUV"> If true, distinct UV-s will be ignored when trying to merge the vertices </param>
		/// <returns> Smooth-shader copy of the mesh </returns>
		JIMARA_API Reference<TriMesh> ShadeSmooth(const TriMesh* mesh, bool ignoreUV, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, 
		/// but shaded smooth with vertices merged based on their positions
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="ignoreUV"> If true, distinct UV-s will be ignored when trying to merge the vertices </param>
		/// <returns> Smooth-shader copy of the mesh </returns>
		JIMARA_API Reference<TriMesh> ShadeSmooth(const TriMesh* mesh, bool ignoreUV);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, 
		/// but shaded smooth with vertices merged based on their positions
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <param name="ignoreUV"> If true, distinct UV-s will be ignored when trying to merge the vertices </param>
		/// <returns> Smooth-shader copy of the mesh </returns>
		JIMARA_API Reference<PolyMesh> ShadeSmooth(const PolyMesh* mesh, bool ignoreUV, const std::string_view& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, 
		/// but shaded smooth with vertices merged based on their positions
		/// <para/> Note: Does not generate a skinned mesh
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="ignoreUV"> If true, distinct UV-s will be ignored when trying to merge the vertices </param>
		/// <returns> Smooth-shader copy of the mesh </returns>
		JIMARA_API Reference<PolyMesh> ShadeSmooth(const PolyMesh* mesh, bool ignoreUV);





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





		/// <summary>
		/// Generates a simplified/decimated mesh
		/// </summary>
		/// <param name="mesh"> Geometry </param>
		/// <param name="angleThreshold"> Vertices that have faces around that deviate from the average normal by no more than this amount will be removed </param>
		/// <param name="maxIterations"> Maximal number of vertex removal iterations </param>
		/// <param name="name"> Name of the resulting mesh </param>
		/// <returns> Simplified mesh </returns>
		JIMARA_API Reference<TriMesh> SimplifyMesh(const TriMesh* mesh, float angleThreshold, size_t maxIterations, const std::string_view& name);
	}
}
