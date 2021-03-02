#include "../GtestHeaders.h"
#include "OS/Logging/StreamLogger.h"
#include "Components/Transform.h"
#include "Environment/Scene.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

namespace Jimara {
	namespace {
		inline static Reference<Scene> CreateScene() {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("TransformTest", Application::AppVersion(1, 0, 0));
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo, Graphics::GraphicsInstance::Backend::VULKAN);
			if (graphicsInstance == nullptr) {
				logger->Fatal("TransformTest - CreateScene: Failed to create graphics instance!");
				return nullptr;
			}
			else if (graphicsInstance->PhysicalDeviceCount() > 0) {
				Reference<Graphics::GraphicsDevice> graphicsDevice = graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();
				if (graphicsDevice == nullptr) {
					logger->Fatal("TransformTest - CreateScene: Failed to create graphics device!");
					return nullptr;
				}
				else {
					Reference<AppContext> context = Object::Instantiate<AppContext>(graphicsDevice);
					return Object::Instantiate<Scene>(context);
				}
			}
			else {
				logger->Fatal("TransformTest - CreateScene: No physical device present!");
				return nullptr;
			}
		}

		inline static std::string MatrixToString(const Matrix4& matrix, const char* matrixName = "Matrix") {
			std::stringstream stream;
			stream << matrixName << ": " << std::fixed << std::setprecision(4) << std::endl
				<< "   [{" << matrix[0][0] << ", " << matrix[0][1] << ", " << matrix[0][2] << ", " << matrix[0][3] << "}," << std::endl
				<< "    {" << matrix[1][0] << ", " << matrix[1][1] << ", " << matrix[1][2] << ", " << matrix[1][3] << "}," << std::endl
				<< "    {" << matrix[2][0] << ", " << matrix[2][1] << ", " << matrix[2][2] << ", " << matrix[2][3] << "}," << std::endl
				<< "    {" << matrix[3][0] << ", " << matrix[3][1] << ", " << matrix[3][2] << ", " << matrix[3][3] << "}]";
			return stream.str();
		}

		inline static std::string VectorToString(const Vector3& vector) {
			std::stringstream stream;
			stream << std::fixed << std::setprecision(4) << "{" << vector.x << ", " << vector.y << ", " << vector.z << "}";
			return stream.str();
		}

		inline static bool VectorsMatch(const Vector3& a, const Vector3& b) {
			const Vector3& delta = (a - b);
			return Dot(delta, delta) < 0.001f;
		};
	}


	// Basic tests for local transform fields
	TEST(TransformTest, LocalFields) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		Transform* transform = Object::Instantiate<Transform>(scene->RootObject(), "Transform");

		EXPECT_EQ(transform->LocalPosition(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalPosition(Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(0.0f, 0.0f, 0.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalEulerAngles(Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(1.0f, 1.0f, 1.0f));

		transform->SetLocalScale(Vector3(8.0f, 16.0f, 32.0f));
		EXPECT_EQ(transform->LocalPosition(), Vector3(1.0f, 2.0f, 3.0f));
		EXPECT_EQ(transform->LocalEulerAngles(), Vector3(30.0f, 60.0f, 80.0f));
		EXPECT_EQ(transform->LocalScale(), Vector3(8.0f, 16.0f, 32.0f));

		transform->Destroy();
	}

	// Basic tests for local rotation
	TEST(TransformTest, LocalRotation) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-360.0f, 360.0f);

		Transform* transform = Object::Instantiate<Transform>(scene->RootObject(), "Transform");

		auto logMatrix = [&] { scene->Context()->Log()->Info(MatrixToString(transform->LocalRotationMatrix(), "LocalRotationMatrix")); };

		{
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(0.0f, 1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		{
			transform->SetLocalEulerAngles(Vector3(0.0f, 90.0f, 0.0f));
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			transform->SetLocalScale(Vector3(1.0f, 0.0f, -1.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(-1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(0.0f, 1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		{
			transform->SetLocalEulerAngles(Vector3(0.0f, -90.0f, 0.0f));
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			transform->SetLocalScale(Vector3(dis(rng), dis(rng), dis(rng)));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, 0.0f, -1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(0.0f, 1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		{
			transform->SetLocalEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			transform->SetLocalScale(Vector3(dis(rng), dis(rng), dis(rng)));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(0.0f, 1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(0.0f, 0.0f, -1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		{
			transform->SetLocalEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
			transform->SetLocalScale(Vector3(0.0f, 0.0f, 0.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, -1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		for (size_t i = 0; i < 64; i++) {
			Vector3 rotation(dis(rng), dis(rng), dis(rng));
			transform->SetLocalEulerAngles(rotation);
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			transform->SetLocalScale(Vector3(dis(rng), dis(rng), dis(rng)));
			scene->Context()->Log()->Info("Euler angles: " + VectorToString(rotation));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}
	}

	// Basic tests for local position
	TEST(TransformTest, LocalPosition) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-180.0f, 180.0f);

		Transform* transform = Object::Instantiate<Transform>(scene->RootObject(), "Transform");

		auto logMatrix = [&] { scene->Context()->Log()->Info(MatrixToString(transform->LocalMatrix(), "LocalMatrix")); };

		{
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalForward()), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalRight()), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalUp()), Vector3(0.0f, 1.0f, 0.0f)));
		}

		{
			transform->SetLocalScale(Vector3(0.5f, 1.0f, -1.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalForward()), Vector3(0.0f, 0.0f, -1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalRight()), Vector3(0.5f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(transform->LocalUp()), Vector3(0.0f, 1.0f, 0.0f)));
			transform->SetLocalScale(Vector3(1.0f, 1.0f, 1.0f));
		}

		for (size_t i = 0; i < 64; i++) {
			Vector3 position(dis(rng), dis(rng), dis(rng));
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(position), position));
		}

		for (size_t i = 0; i < 64; i++) {
			Vector3 scale(dis(rng), dis(rng), dis(rng));
			Vector3 position(dis(rng), dis(rng), dis(rng));
			transform->SetLocalScale(scale);
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(position), scale * position));
		}

		transform->SetLocalScale(Vector3(1.0f, 1.0f, 1.0f));
		for (size_t i = 0; i < 64; i++) {
			Vector3 rotation(dis(rng), dis(rng), dis(rng));
			Vector3 position(dis(rng), dis(rng), dis(rng));
			transform->SetLocalEulerAngles(rotation);
			EXPECT_TRUE(VectorsMatch(transform->LocalToParentSpacePosition(position)
				, position.x * transform->LocalRight() + position.y * transform->LocalUp() + position.z * transform->LocalForward()));
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 scale(dis(rng), dis(rng), dis(rng));
			const Vector3 rotation(dis(rng), dis(rng), dis(rng));
			const Vector3 position(dis(rng), dis(rng), dis(rng));
			
			transform->SetLocalScale(scale);
			transform->SetLocalEulerAngles(rotation);
			
			const Vector3 expected =
				position.x * transform->LocalRight() * scale.x
				+ position.y * transform->LocalUp() * scale.y
				+ position.z * transform->LocalForward() * scale.z;
			const Vector3 computed = transform->LocalToParentSpacePosition(position);

			const bool match = VectorsMatch(computed, expected);
			if (!match) 
				scene->Context()->Log()->Info("Expected: " + VectorToString(expected) + "; Got: " + VectorToString(computed) + "; match: " + (match ? "YES" : "NO") + ";");
			EXPECT_TRUE(match);
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 scale(dis(rng), dis(rng), dis(rng));
			const Vector3 rotation(dis(rng), dis(rng), dis(rng));
			const Vector3 position(dis(rng), dis(rng), dis(rng));
			const Vector3 point(dis(rng), dis(rng), dis(rng));

			transform->SetLocalScale(scale);
			transform->SetLocalPosition(position);
			transform->SetLocalEulerAngles(rotation);

			const Vector3 expected = position 
				+ point.x * transform->LocalRight() * scale.x
				+ point.y * transform->LocalUp() * scale.y
				+ point.z * transform->LocalForward() * scale.z;
			const Vector3 computed = transform->LocalToParentSpacePosition(point);

			const bool match = VectorsMatch(computed, expected);
			if (!match) {
				logMatrix();
				scene->Context()->Log()->Info("Expected: " + VectorToString(expected) + "; Got: " + VectorToString(computed) + "; delta: " + VectorToString(expected - computed));
			}
			EXPECT_TRUE(match);
		}
	}
}
