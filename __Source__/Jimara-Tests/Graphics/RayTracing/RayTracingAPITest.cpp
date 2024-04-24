#include "../../GtestHeaders.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Data/Geometry/MeshConstants.h"
#include "Data/Geometry/GraphicsMesh.h"
#include "../../CountingLogger.h"


namespace Jimara {
	namespace Graphics {
		TEST(RayTracingAPITest, AccelerationStructureBuild) {
			const Reference<Jimara::Test::CountingLogger> log = Object::Instantiate<Jimara::Test::CountingLogger>();
			const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("GraphicsAtomicsTest");
			const Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(log, appInfo);
			ASSERT_NE(graphicsInstance, nullptr);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			const size_t warningCount = log->NumWarning();
			const size_t failureCount = log->Numfailures();

			bool deviceFound = false;
			for (size_t deviceId = 0u; deviceId < graphicsInstance->PhysicalDeviceCount(); deviceId++) {
				PhysicalDevice* const physDevice = graphicsInstance->GetPhysicalDevice(deviceId);
				if (!physDevice->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					continue;
				deviceFound = true;

				const Reference<GraphicsDevice> device = physDevice->CreateLogicalDevice();
				ASSERT_NE(device, nullptr);

				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);

				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);
				const Reference<PrimaryCommandBuffer> commands = commandPool->CreatePrimaryCommandBuffer();
				ASSERT_NE(commandPool, nullptr);

				commands->BeginRecording();
				blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
				commands->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commands);
				commands->Wait();
			}

			EXPECT_EQ(warningCount, log->NumWarning());
			EXPECT_EQ(failureCount, log->Numfailures());

			if (!deviceFound)
				log->Warning("No RT-Capable GPU was found!");
		}
	}
}
