#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include <Core/Stopwatch.h>
#include <Core/Collections/VoxelGrid.h>
#include <Core/Collections/ThreadBlock.h>
#include <Math/Primitives/Triangle.h>
#include <Data/Formats/WavefrontOBJ.h>
#include <Graphics/GraphicsInstance.h>
#include <Data/Geometry/MeshConstants.h>



namespace Jimara {
	namespace {
		const constexpr std::string_view BEAR_SCENE_PATH = "Assets/Meshes/OBJ/Bear/ursus_proximus.obj";

		inline static void GeometryQueries_AddMeshTriangles(const TriMesh* mesh, std::vector<Triangle3>& tris) {
			TriMesh::Reader reader(mesh);
			for (uint32_t tId = 0u; tId < reader.FaceCount(); tId++) {
				const TriangleFace face = reader.Face(tId);
				tris.push_back(Triangle3(
					reader.Vert(face.a).position,
					reader.Vert(face.b).position,
					reader.Vert(face.c).position));
			}
		}

		inline static std::vector<Triangle3> GeometryQueries_LoadGeometryFromFile(
			OS::Logger* logger, const std::string_view filename = BEAR_SCENE_PATH) {
			Stopwatch timer;
			logger->Info("Loading geometry...");
			std::vector<Reference<TriMesh>> geometry = TriMeshesFromOBJ(filename);
			assert(!geometry.empty());
			logger->Info("Load time: ", timer.Reset());

			logger->Info("Collecting triangle list...");
			std::vector<Triangle3> tris;
			for (size_t i = 0u; i < geometry.size(); i++)
				GeometryQueries_AddMeshTriangles(geometry[i], tris);
			logger->Info("Triangle collection time: ", timer.Reset());
			return tris;
		}

		inline static std::vector<Octree<Triangle3>> GeometryQueries_GeneratGeometryOctrees(
			OS::Logger* logger, const std::vector<Reference<TriMesh>>& geometry) {
			Stopwatch timer;
			logger->Info("Building octrees...");
			std::vector<Octree<Triangle3>> octrees;
			for (size_t i = 0u; i < geometry.size(); i++) {
				std::vector<Triangle3> tris;
				GeometryQueries_AddMeshTriangles(geometry[i], tris);
				octrees.push_back(Octree<Triangle3>::Build(tris.begin(), tris.end()));
			}
			logger->Info("Octree build time: ", timer.Reset());
			return octrees;
		}

		inline static std::vector<Octree<Triangle3>> GeometryQueries_LoadGeometryAsOctreesFromFile(
			OS::Logger* logger, const std::string_view filename = BEAR_SCENE_PATH) {
			Stopwatch timer;
			logger->Info("Loading geometry...");
			std::vector<Reference<TriMesh>> geometry = TriMeshesFromOBJ(filename);
			assert(!geometry.empty());
			logger->Info("Load time: ", timer.Reset());
			return GeometryQueries_GeneratGeometryOctrees(logger, geometry);
		}

		template<typename SceneType, typename GetTriangleRefFn, typename OnRenderFrameFn>
		inline static void GeometryQueries_RenderWithRaycasts(
			OS::Logger* logger, const std::string_view& testName, const SceneType& scene, 
			const GetTriangleRefFn& getTriangleRef, const OnRenderFrameFn& onRenderFrame) {
			const Reference<Application::AppInformation> graphicsAppInfo = Object::Instantiate<Application::AppInformation>();
			const Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, graphicsAppInfo);
			assert(graphicsInstance != nullptr);

			const Reference<OS::Window> window = OS::Window::Create(logger, testName.data(), Size2(512u, 256u));
			assert(window != nullptr);
			const Reference<Graphics::RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(window);
			assert(renderSurface != nullptr);

			Graphics::PhysicalDevice* const graphicsPhysDevice = renderSurface->PrefferedDevice();
			assert(graphicsPhysDevice != nullptr);
			const Reference<Graphics::GraphicsDevice> graphicsDevice = graphicsPhysDevice->CreateLogicalDevice();
			assert(graphicsDevice != nullptr);

			const Reference<Graphics::RenderEngine> surfaceEngine = graphicsDevice->CreateRenderEngine(renderSurface);
			assert(surfaceEngine != nullptr);

			class Renderer : public virtual Graphics::ImageRenderer {
			private:
				const SceneType* m_scene;
				const GetTriangleRefFn m_getTriangleRef;
				const OnRenderFrameFn m_onRenderFrame;
				ThreadBlock m_threadBlock;

			public:
				Vector3 target = Vector3(0.0f, 1.0f, 0.0);
				Vector3 eulerAngles = Vector3(16.0f, 0.0f, 0.0f);
				float distance = 8.0f;
				float fieldOfView = 60.0f;

				std::atomic<float> frameTime = 0.0f;
				std::atomic<float> avgFrameTime = 0.0f;

				inline Renderer(const SceneType* scene, 
					const GetTriangleRefFn& getTriangleRef, const OnRenderFrameFn& onRenderFrame)
					: m_scene(scene), m_getTriangleRef(getTriangleRef), m_onRenderFrame(onRenderFrame) {}

				virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) final override { return engineInfo; }

				virtual void Render(Object* engineData, Graphics::InFlightBufferInfo bufferInfo) {
					Graphics::RenderEngineInfo* engineInfo = dynamic_cast<Graphics::RenderEngineInfo*>(engineData);
					assert(engineInfo != nullptr);

					Graphics::Texture* targetTexture = engineInfo->Image(bufferInfo);
					assert(targetTexture != nullptr);
					const Size2 imageSize = targetTexture->Size();
					if (Math::Min(imageSize.x, imageSize.y) <= 1u)
						return;

					Stopwatch timer;

					const Reference<Graphics::ImageTexture> texture = engineInfo->Device()->CreateTexture(
						Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
						Size3(imageSize, 1u), 1u, false, Graphics::ImageTexture::AccessFlags::CPU_READ);
					assert(targetTexture != nullptr);

					const Matrix4 rotationMatrix = Math::MatrixFromEulerAngles(eulerAngles);
					const Vector3 cameraPosition = target - Vector3(rotationMatrix[2u]) * distance;
					const float tangent = std::tan(Math::Radians(fieldOfView * 0.5f));

					std::atomic<size_t> pixelCounter = 0u;
					Vector4* textureData = reinterpret_cast<Vector4*>(texture->Map());
					assert(textureData != nullptr);
					auto render = [&](const ThreadBlock::ThreadInfo& threadInfo, void*) {
						while (true) {
							const size_t pixelIndex = pixelCounter.fetch_add(1u);
							if (pixelIndex >= (size_t(imageSize.x) * imageSize.y))
								break;
							const size_t yi = pixelIndex / imageSize.x;
							const size_t xi = pixelIndex - (yi * imageSize.x);

							const Vector2 pixelPos = Vector2(
								float(xi) / float(imageSize.x - 1u) - 0.5f,
								-float(yi) / float(imageSize.y - 1u) + 0.5f);
							const Vector3 localRayDir = Vector3(
								float(imageSize.x) / float(imageSize.y) * tangent * pixelPos.x,
								pixelPos.y * tangent, 1.0f);
							const Vector3 rayDir = Math::Normalize(rotationMatrix * Vector4(localRayDir, 0.0f));

							Vector4& pixel = textureData[texture->Pitch().x * yi + xi];
							const auto result = m_scene->Raycast(cameraPosition, rayDir);
							if (!result) {
								pixel = Vector4(0.0f);
								continue;
							}

							const Triangle3 face = m_getTriangleRef(result);
							const Vector3 normal = Math::Normalize(Math::Cross(face[1u] - face[0u], face[2u] - face[0u]));
							pixel = Vector4((normal + 1.0f) * 0.5f, 1.0f);
						}
					};

					m_threadBlock.Execute(std::thread::hardware_concurrency(),
						nullptr, Callback<ThreadBlock::ThreadInfo, void*>::FromCall(&render));
					texture->Unmap(true);
					targetTexture->Blit(bufferInfo, texture);

					m_onRenderFrame(frameTime.load());
					frameTime = timer.Elapsed();
					avgFrameTime = Math::Lerp(avgFrameTime.load(), frameTime.load(), 0.05f);
					eulerAngles.y = std::fmod(eulerAngles.y + frameTime * 10.0f, 360.0f);
				}
			};
			const Reference<Renderer> renderer = Object::Instantiate<Renderer>(&scene, getTriangleRef, onRenderFrame);
			surfaceEngine->AddRenderer(renderer);

			const auto staggerWindowUpdateRate = [&](OS::Window*) {
				surfaceEngine->Update();
				};
			window->OnUpdate() += Callback<OS::Window*>::FromCall(&staggerWindowUpdateRate);

			Stopwatch windowTimeout;
			std::optional<Size2> expectedWindowSize = window->FrameBufferSize();
			while (true) {
				if (window->Closed())
					break;

				std::stringstream stream;
				stream << std::fixed << std::setprecision(3) << testName << " ("
					<< (1.0f / renderer->avgFrameTime) << " fps; " << (renderer->frameTime * 1000.0f) << " ms)";

				if (expectedWindowSize.has_value()) {
					if (window->FrameBufferSize() != expectedWindowSize.value()) {
						expectedWindowSize = std::optional<Size2>();
					}
					else {
						const constexpr float TIMEOUT = 5.0f;
						const float elapsed = windowTimeout.Elapsed();
						if (elapsed > TIMEOUT)
							break;
						stream << " [Auto close in " << std::fixed << std::setprecision(1) << (TIMEOUT - elapsed) << " seconds]";
					}
				}

				window->SetName(stream.str());
				std::this_thread::sleep_for(std::chrono::microseconds(8u));
			}

			window->OnUpdate() -= Callback<OS::Window*>::FromCall(&staggerWindowUpdateRate);
		}
	}

	TEST(GeometryQueryTest, OctreeTest_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Triangle3> tris = GeometryQueries_LoadGeometryFromFile(logger);

		Stopwatch timer;
		logger->Info("Building Octree...");
		const Octree<Triangle3> octree = Octree<Triangle3>::Build(tris.begin(), tris.end());
		logger->Info("Build time: ", timer.Reset());

		GeometryQueries_RenderWithRaycasts(logger, "OctreeTest", octree, [&](const auto& hit) { return hit; }, [&](auto) {});
	}

	TEST(GeometryQueryTest, VoxelGrid_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Triangle3> tris = GeometryQueries_LoadGeometryFromFile(logger);
		ASSERT_FALSE(tris.empty());

		Stopwatch timer;
		logger->Info("Building VoxelGrid...");
		VoxelGrid<Triangle3> grid;
		grid.BoundingBox() = Math::BoundingBox(tris);
		grid.GridSize() = Size3(128u);
		for (size_t i = 0u; i < tris.size(); i++)
			grid.Push(tris[i]);
		logger->Info("Build time: ", timer.Reset());

		GeometryQueries_RenderWithRaycasts(logger, "VoxelGridTest", grid, [&](const auto& hit) { return hit; }, [&](auto) {});
	}

	TEST(GeometryQueryTest, OctreesOfOctrees_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Octree<Triangle3>> octrees = GeometryQueries_LoadGeometryAsOctreesFromFile(logger);
		ASSERT_FALSE(octrees.empty());

		Stopwatch timer;
		logger->Info("Building compound Octree...");
		const Octree<Octree<Triangle3>> octree = Octree<Octree<Triangle3>>::Build(octrees.begin(), octrees.end());
		logger->Info("Octree build time: ", timer.Reset());

		GeometryQueries_RenderWithRaycasts(logger, "OctreesOfOctreesTest", octree,
			[&](const VoxelGrid<Octree<Triangle3>>::RaycastResult& hit) -> const Triangle3& { return hit.hit; }, [&](auto) {});
	}

	TEST(GeometryQueryTest, VoxelGridOfOctrees_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Octree<Triangle3>> octrees = GeometryQueries_LoadGeometryAsOctreesFromFile(logger);
		ASSERT_FALSE(octrees.empty());

		Stopwatch timer;
		logger->Info("Building VoxelGrid...");
		VoxelGrid<Octree<Triangle3>> grid;
		grid.BoundingBox() = Math::BoundingBox(octrees);
		grid.GridSize() = Size3(8u);
		for (size_t i = 0u; i < octrees.size(); i++)
			grid.Push(octrees[i]);
		logger->Info("Grid build time: ", timer.Reset());

		GeometryQueries_RenderWithRaycasts(logger, "VoxelGridOfOctreesTest", grid, 
			[&](const VoxelGrid<Octree<Triangle3>>::RaycastResult& hit) -> const Triangle3& { return hit.hit; }, [&](auto) {});
	}

	TEST(GeometryQueryTest, OctreeOfPosedOctrees_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Reference<TriMesh>> meshes = TriMeshesFromOBJ(BEAR_SCENE_PATH);
		ASSERT_FALSE(meshes.empty());
		const std::vector<Octree<Triangle3>> octrees = GeometryQueries_GeneratGeometryOctrees(logger, meshes);
		ASSERT_TRUE(octrees.size() == meshes.size());

		Stopwatch timer;
		logger->Info("Building compound Octree...");
		std::vector<PosedOctree<Triangle3>> posedOctrees;
		size_t bearIndex = ~size_t(0u);
		for (size_t i = 0u; i < octrees.size(); i++) {
			if (TriMesh::Reader(meshes[i]).Name() == "bear")
				bearIndex = i;
			posedOctrees.push_back({ octrees[i], Math::Identity() });
		}
		Octree<PosedOctree<Triangle3>> octree = Octree<PosedOctree<Triangle3>>::Build(posedOctrees.begin(), posedOctrees.end());
		logger->Info("Octree build time: ", timer.Reset());
		ASSERT_LT(bearIndex, octrees.size());

		Stopwatch totalTime;
		GeometryQueries_RenderWithRaycasts(logger, "OctreeOfPosedOctreesTest", octree,
			[&](const VoxelGrid<PosedOctree<Triangle3>>::RaycastResult& hit) -> const Triangle3& { return hit.hit; }, 
			[&](float) {
				float total = totalTime.Elapsed();
				const float sinAbs = std::abs(std::sin(total * 2.0f));
				Matrix4 pose = Math::MatrixFromEulerAngles(Vector3(0.0f, total * 60.0f, 0.0f));
				pose[1u] *= sinAbs * 0.75f + 0.75f;
				pose[3u] = Vector4(0.0f, sinAbs * 0.5f, 0.0f, 1.0f);
				posedOctrees[bearIndex] = { octrees[bearIndex], pose };
				octree = Octree<PosedOctree<Triangle3>>::Build(posedOctrees.begin(), posedOctrees.end());
			});
	}

	TEST(GeometryQueryTest, VoxelGridOfPosedOctrees_Visual) {
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		const std::vector<Reference<TriMesh>> meshes = TriMeshesFromOBJ(BEAR_SCENE_PATH);
		ASSERT_FALSE(meshes.empty());
		const std::vector<Octree<Triangle3>> octrees = GeometryQueries_GeneratGeometryOctrees(logger, meshes);
		ASSERT_TRUE(octrees.size() == meshes.size());

		Stopwatch timer;
		logger->Info("Building VoxelGrid...");
		VoxelGrid<PosedOctree<Triangle3>> grid;
		grid.BoundingBox() = Math::BoundingBox(octrees);
		grid.GridSize() = Size3(8u);
		size_t bearIndex = ~size_t(0u);
		for (size_t i = 0u; i < octrees.size(); i++) {
			if (TriMesh::Reader(meshes[i]).Name() == "bear")
				bearIndex = i;
			grid.Push({ octrees[i], Math::Identity() });
		}
		logger->Info("Grid build time: ", timer.Reset());
		ASSERT_LT(bearIndex, octrees.size());

		Stopwatch totalTime;
		GeometryQueries_RenderWithRaycasts(logger, "VoxelGridOfPosedOctreesTest", grid,
			[&](const VoxelGrid<PosedOctree<Triangle3>>::RaycastResult& hit) -> const Triangle3& { return hit.hit; }, 
			[&](float) {
				float total = totalTime.Elapsed();
				const float sinAbs = std::abs(std::sin(total * 2.0f));
				Matrix4 pose = Math::MatrixFromEulerAngles(Vector3(0.0f, total * 60.0f, 0.0f));
				pose[1u] *= sinAbs * 0.75f + 0.75f;
				pose[3u] = Vector4(0.0f, sinAbs * 0.5f, 0.0f, 1.0f);
				grid[bearIndex] = PosedOctree<Triangle3> { octrees[bearIndex], pose };
			});
	}
}
