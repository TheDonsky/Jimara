#include "../GtestHeaders.h"
#include "Data/Formats/WavefrontOBJ.h"
#include "OS/Logging/StreamLogger.h"
#include <sstream>

namespace Jimara {
	TEST(OBJTest, LoadAllFromOBJ) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		std::vector<Reference<TriMesh>> meshes = TriMeshesFromOBJ("Assets/Meshes/OBJ/Bear/ursus_proximus.obj", logger);

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
			const TriMesh::Reader reader(mesh);
			if (reader.Name() == "text") textFound = true;
			if (reader.Name() == "backdrop") backdropFound = true;
			if (reader.Name() == "platform") platformFound = true;
			if (reader.Name() == "surface") surfaceFound = true;
			if (reader.Name() == "bear") bearFound = true;
			stream << "Mesh " << i
				<< " - name:'" << reader.Name()
				<< "' verts:" << reader.VertCount()
				<< " faces:" << reader.FaceCount();
			return stream.str(); }());
		EXPECT_TRUE(textFound);
		EXPECT_TRUE(backdropFound);
		EXPECT_TRUE(platformFound);
		EXPECT_TRUE(surfaceFound);
		EXPECT_TRUE(bearFound);
	}

	TEST(OBJTest, LoadOneFromOBJ) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		Reference<TriMesh> mesh = TriMeshFromOBJ("Assets/Meshes/OBJ/Bear/ursus_proximus.obj", "bear", logger);
		ASSERT_NE(mesh, nullptr);
		const TriMesh::Reader reader(mesh);
		EXPECT_EQ(reader.Name(), "bear");
		std::stringstream stream;
		stream << "Mesh - "
			<< " name:'" << reader.Name()
			<< "' verts:" << reader.VertCount()
			<< " faces:" << reader.FaceCount();
		logger->Info(stream.str());
	}

	TEST(OBJTest, LoadFromNonAscii) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		std::vector<Reference<TriMesh>> meshes = TriMeshesFromOBJ(L"Assets/Meshes/OBJ/ხო... კუბი.obj", logger);
		ASSERT_EQ(meshes.size(), 1);
	}
}
