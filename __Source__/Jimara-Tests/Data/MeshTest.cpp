#include "../GtestHeaders.h"
#include "Data/Mesh.h"
#include "OS/Logging/StreamLogger.h"
#include <sstream>

namespace Jimara {
	TEST(MeshTest, LoadAllFromOBJ) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		std::vector<Reference<TriMesh>> meshes = TriMesh::FromOBJ("Assets/Meshes/Bear/ursus_proximus.obj", logger);

		EXPECT_EQ(meshes.size(), 5);
		bool textFound = false;
		bool backdropFound = false;
		bool platformFound = false;
		bool surfaceFound = false;
		bool bearFound = false;
		for (size_t i = 0; i < meshes.size(); i++)
			logger->Info([&]() {
			std::stringstream stream;
			const TriMesh* mesh = meshes[i];
			if (mesh->Name() == "text") textFound = true;
			if (mesh->Name() == "backdrop") backdropFound = true;
			if (mesh->Name() == "platform") platformFound = true;
			if (mesh->Name() == "surface") surfaceFound = true;
			if (mesh->Name() == "bear") bearFound = true;
			stream << "Mesh " << i
				<< " - name:'" << mesh->Name()
				<< "' verts:" << mesh->VertCount()
				<< " faces:" << mesh->FaceCount();
			return stream.str(); }());
		EXPECT_TRUE(textFound);
		EXPECT_TRUE(backdropFound);
		EXPECT_TRUE(platformFound);
		EXPECT_TRUE(surfaceFound);
		EXPECT_TRUE(bearFound);
	}

	TEST(MeshTest, LoadOneFromOBJ) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		Reference<TriMesh> mesh = TriMesh::FromOBJ("Assets/Meshes/Bear/ursus_proximus.obj", "bear", logger);
		ASSERT_NE(mesh, nullptr);
		EXPECT_EQ(mesh->Name(), "bear");
		std::stringstream stream;
		stream << "Mesh - "
			<< " name:'" << mesh->Name()
			<< "' verts:" << mesh->VertCount()
			<< " faces:" << mesh->FaceCount();
		logger->Info(stream.str());
	}
}
