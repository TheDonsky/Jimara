#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include <Core/Stopwatch.h>
#include <Core/Collections/Octree.h>
#include <Core/Collections/ThreadBlock.h>
#include <Math/Primitives/Triangle.h>
#include <Data/Formats/WavefrontOBJ.h>
#include <Graphics/GraphicsInstance.h>



namespace Jimara {
	TEST(OctreeTest, Visual) {
		Stopwatch timer;
		const Reference<OS::Logger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
		
		logger->Info("Loading geometry...");
		std::vector<Reference<TriMesh>> geometry = TriMeshesFromOBJ("Assets/Meshes/OBJ/Bear/ursus_proximus.obj");
		ASSERT_FALSE(geometry.empty());
		logger->Info("Load time: ", timer.Reset());

		logger->Info("Compiling triangle list...");
		std::vector<Triangle3> tris;
		for (size_t i = 0u; i < geometry.size(); i++) {
			TriMesh::Reader reader(geometry[i]);
			for (uint32_t tId = 0u; tId < reader.FaceCount(); tId++) {
				const TriangleFace face = reader.Face(tId);
				tris.push_back(Triangle3(
					reader.Vert(face.a).position,
					reader.Vert(face.b).position,
					reader.Vert(face.c).position));
			}
		}
		logger->Info("Compile time: ", timer.Reset());

		logger->Info("Building Octree...");
		const Octree<Triangle3> octree = Octree<Triangle3>::Build(tris.begin(), tris.end());
		logger->Info("Build time: ", timer.Reset());

		const Reference<Application::AppInformation> graphicsAppInfo = Object::Instantiate<Application::AppInformation>();
		const Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, graphicsAppInfo);
		ASSERT_NE(graphicsInstance, nullptr);

		const Reference<OS::Window> window = OS::Window::Create(logger, "OctreeTest", Size2(64u, 64u));
		ASSERT_NE(window, nullptr);
		const Reference<Graphics::RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(window);
		ASSERT_NE(renderSurface, nullptr);

		Graphics::PhysicalDevice* const graphicsPhysDevice = renderSurface->PrefferedDevice();
		ASSERT_NE(graphicsPhysDevice, nullptr);
		const Reference<Graphics::GraphicsDevice> graphicsDevice = graphicsPhysDevice->CreateLogicalDevice();
		ASSERT_NE(graphicsDevice, nullptr);

		const Reference<Graphics::RenderEngine> surfaceEngine = graphicsDevice->CreateRenderEngine(renderSurface);
		ASSERT_NE(surfaceEngine, nullptr);

		class Renderer : public virtual Graphics::ImageRenderer {
		private:
			const Octree<Triangle3> m_octree;
			ThreadBlock m_threadBlock;

		public:
			Vector3 target = Vector3(0.0f, 1.0f, 0.0);
			Vector3 eulerAngles = Vector3(30.0f, 0.0f, 0.0f);
			float distance = 8.0f;
			float fieldOfView = 60.0f;

			std::atomic<float> frameTime = std::numeric_limits<float>::quiet_NaN();
			std::atomic<float> avgFrameTime = 0.0f;

			inline Renderer(const Octree<Triangle3>& octree) : m_octree(octree) {}

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
						if (pixelIndex >= (imageSize.x * imageSize.y))
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
						Octree<Triangle3>::RaycastResult result = m_octree.Raycast(cameraPosition, rayDir);
						if (!result) {
							pixel = Vector4(0.0f);
							continue;
						}
						
						const Triangle3& face = result;
						const Vector3 normal = Math::Normalize(Math::Cross(face[1u] - face[0u], face[2u] - face[0u]));
						pixel = Vector4((normal + 1.0f) * 0.5f, 1.0f);
					}
				};
				
				m_threadBlock.Execute(std::thread::hardware_concurrency(), nullptr, Callback<ThreadBlock::ThreadInfo, void*>::FromCall(&render));
				texture->Unmap(true);
				targetTexture->Blit(bufferInfo, texture);

				frameTime = timer.Elapsed();
				avgFrameTime = Math::Lerp(avgFrameTime.load(), frameTime.load(), 0.05f);
				eulerAngles.y = std::fmod(eulerAngles.y + frameTime * 10.0f, 360.0f);
			}
		};
		const Reference<Renderer> renderer = Object::Instantiate<Renderer>(octree);
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
			stream << std::fixed << std::setprecision(3) << "OctreeTest ("
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

