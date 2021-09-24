#pragma once
#include "../Mesh.h"


namespace Jimara {
	/// <summary>
	/// Loads all meshes from a wavefront obj file as TriMesh objects
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> List of all objects within the file (empty, if failed) </returns>
	std::vector<Reference<TriMesh>> TriMeshesFromOBJ(const std::string_view& filename, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads a TriMesh from a wavefront obj file
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="objectName"> Name of an individual object within the file </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> Instance of a loaded mesh (nullptr, if failed) </returns>
	Reference<TriMesh> TriMeshFromOBJ(const std::string_view& filename, const std::string_view& objectName, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads all meshes from a wavefront obj file as PolyMesh objects
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> List of all objects within the file (empty, if failed) </returns>
	std::vector<Reference<PolyMesh>> PolyMeshesFromOBJ(const std::string_view& filename, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads a PolyMesh from a wavefront obj file
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="objectName"> Name of an individual object within the file </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> Instance of a loaded mesh (nullptr, if failed) </returns>
	Reference<PolyMesh> PolyMeshFromOBJ(const std::string_view& filename, const std::string_view& objectName, OS::Logger* logger = nullptr);
}
