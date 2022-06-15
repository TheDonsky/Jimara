#pragma once
#include "../Mesh.h"
#include "../../OS/IO/Path.h"


namespace Jimara {
	/// <summary>
	/// Loads all meshes from a wavefront obj file as TriMesh objects
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> List of all objects within the file (empty, if failed) </returns>
	JIMARA_API std::vector<Reference<TriMesh>> TriMeshesFromOBJ(const OS::Path& filename, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads a TriMesh from a wavefront obj file
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="objectName"> Name of an individual object within the file </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> Instance of a loaded mesh (nullptr, if failed) </returns>
	JIMARA_API Reference<TriMesh> TriMeshFromOBJ(const OS::Path& filename, const std::string_view& objectName, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads all meshes from a wavefront obj file as PolyMesh objects
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> List of all objects within the file (empty, if failed) </returns>
	JIMARA_API std::vector<Reference<PolyMesh>> PolyMeshesFromOBJ(const OS::Path& filename, OS::Logger* logger = nullptr);

	/// <summary>
	/// Loads a PolyMesh from a wavefront obj file
	/// </summary>
	/// <param name="filename"> .obj file name </param>
	/// <param name="objectName"> Name of an individual object within the file </param>
	/// <param name="logger"> Logger for error & warning reporting </param>
	/// <returns> Instance of a loaded mesh (nullptr, if failed) </returns>
	JIMARA_API Reference<PolyMesh> PolyMeshFromOBJ(const OS::Path& filename, const std::string_view& objectName, OS::Logger* logger = nullptr);

	/// <summary> Register .obj asset importer </summary>
	JIMARA_REGISTER_TYPE(Jimara::WavefrontOBJAssetImporter);

	/// <summary>
	///  Registers FileSystemDatabase::AssetImporter for obj files
	/// Note: This one should be of no interest for the user; FileSystemDatabase will "automagically" be able to utilize it's functionality.
	/// </summary>
	class JIMARA_API WavefrontOBJAssetImporter {
	private:
		// Nobody's gonna create an instance of this
		inline WavefrontOBJAssetImporter() = delete;
	};

	// Type registration callbacks
	template<> void TypeIdDetails::OnRegisterType<WavefrontOBJAssetImporter>();
	template<> void TypeIdDetails::OnUnregisterType<WavefrontOBJAssetImporter>();
}
