#include "../GtestHeaders.h"
#include "../Memory.h"
#include "OS/Logging/StreamLogger.h"
#include "Components/Transform.h"
#include "Environment/Scene/Scene.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

namespace Jimara {
	namespace {
		inline static Reference<Scene> CreateScene() {
			Scene::CreateArgs args;
			args.createMode = Scene::CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_SUPRESS_WARNINGS;
			return Scene::Create(args);
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
			return Math::Dot(delta, delta) < 0.001f;
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
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, 0.0f, -1.0f)));
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
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(-1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, 0.0f, 1.0f)));
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
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(0.0f, -1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		{
			transform->SetLocalEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
			transform->SetLocalScale(Vector3(0.0f, 0.0f, 0.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), Vector3(0.0f, 1.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), Vector3(-1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(transform->LocalForward(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 0.0f, 1.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalRight(), transform->LocalToParentSpaceDirection(Vector3(1.0f, 0.0f, 0.0f))));
			EXPECT_TRUE(VectorsMatch(transform->LocalUp(), transform->LocalToParentSpaceDirection(Vector3(0.0f, 1.0f, 0.0f))));
		}

		for (size_t i = 0; i < 64; i++) {
			Vector3 rotation(dis(rng), dis(rng), dis(rng));
			transform->SetLocalEulerAngles(rotation);
			transform->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			transform->SetLocalScale(Vector3(dis(rng), dis(rng), dis(rng)));
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

	// Basic tests for local to world rotation
	TEST(TransformTest, LocalToWorldRotation) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-180.0f, 180.0f);

		Transform* parentTransform = Object::Instantiate<Transform>(scene->RootObject(), "ParentTransform");
		Transform* childTransform = Object::Instantiate<Transform>(parentTransform, "ChildTransform");

		auto logMatrix = [&] { scene->Context()->Log()->Info(MatrixToString(childTransform->WorldRotationMatrix(), "WorldRotationMatrix")); };

		{
			logMatrix();
			EXPECT_TRUE(VectorsMatch(childTransform->Forward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->Right(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->Up(), Vector3(0.0f, 1.0f, 0.0f)));
		}

		{
			parentTransform->SetLocalEulerAngles(Vector3(25.0f, 90.0f, -16.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(childTransform->Forward(), parentTransform->Forward()));
			EXPECT_TRUE(VectorsMatch(childTransform->Right(), parentTransform->Right()));
			EXPECT_TRUE(VectorsMatch(childTransform->Up(), parentTransform->Up()));
		}

		{
			parentTransform->SetLocalEulerAngles(Vector3(0.0f, 90.0f, 0.0f));
			childTransform->SetLocalEulerAngles(Vector3(0.0f, -90.0f, 0.0f));
			logMatrix();
			EXPECT_TRUE(VectorsMatch(childTransform->Forward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->Right(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->Up(), Vector3(0.0f, 1.0f, 0.0f)));
		}

		{
			parentTransform->SetLocalEulerAngles(Vector3(64.0f, 90.0f, -32.0f));
			Transform* childA = Object::Instantiate<Transform>(parentTransform, "ChildA");
			Transform* childB = Object::Instantiate<Transform>(childA, "ChildB");
			Transform* childC = Object::Instantiate<Transform>(childB, "ChildC");
			childA->SetLocalEulerAngles(Vector3(0.0f, 0.0f, 32.0f));
			childB->SetLocalEulerAngles(Vector3(-64.0f, 0.0f, 0.0f));
			childC->SetLocalEulerAngles(Vector3(0.0f, -90.0f, 0.0f));
			scene->Context()->Log()->Info(MatrixToString(childC->WorldRotationMatrix(), "C->WorldRotationMatrix"));
			EXPECT_TRUE(VectorsMatch(childC->Forward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(childC->Right(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childC->Up(), Vector3(0.0f, 1.0f, 0.0f)));
			childA->Destroy();
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 euler = Vector3(dis(rng), dis(rng), dis(rng));
			parentTransform->SetLocalEulerAngles(euler);
			Transform* childA = Object::Instantiate<Transform>(parentTransform, "ChildA");
			Transform* childB = Object::Instantiate<Transform>(childA, "ChildB");
			Transform* childC = Object::Instantiate<Transform>(childB, "ChildC");
			childA->SetLocalEulerAngles(Vector3(0.0f, 0.0f, -euler.z));
			childB->SetLocalEulerAngles(Vector3(-euler.x, 0.0f, 0.0f));
			childC->SetLocalEulerAngles(Vector3(0.0f, -euler.y, 0.0f));
			EXPECT_TRUE(VectorsMatch(childC->Forward(), Vector3(0.0f, 0.0f, 1.0f)));
			EXPECT_TRUE(VectorsMatch(childC->Right(), Vector3(1.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childC->Up(), Vector3(0.0f, 1.0f, 0.0f)));
			childA->Destroy();
		}
	}

	// Basic tests for local to world position
	TEST(TransformTest, LocalToWorldPosition) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-180.0f, 180.0f);
		std::uniform_real_distribution<float> scaleDis(-10.0f, 10.0f);

		Transform* parentTransform = Object::Instantiate<Transform>(scene->RootObject(), "ParentTransform");
		Transform* childTransform = Object::Instantiate<Transform>(parentTransform, "ChildTransform");

		auto logMatrix = [&] { scene->Context()->Log()->Info(MatrixToString(childTransform->WorldMatrix(), "WorldMatrix")); };

		for (size_t i = 0; i < 64; i++) {
			const Vector3 point(dis(rng), dis(rng), dis(rng));
			EXPECT_TRUE(VectorsMatch(childTransform->LocalToWorldPosition(point), point));
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 parentPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 childPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 point(dis(rng), dis(rng), dis(rng));
			parentTransform->SetLocalPosition(parentPosition);
			childTransform->SetLocalPosition(childPosition);
			{
				childTransform->SetLocalScale(Vector3(1.0f, 1.0f, 1.0f));
				const Vector3 calculated = childTransform->LocalToWorldPosition(point);
				const Vector3 expected = point + parentPosition + childPosition;
				bool match = VectorsMatch(calculated, expected);
				EXPECT_TRUE(match);
				if (!match) {
					logMatrix();
					scene->Context()->Log()->Info("Parent: " + VectorToString(parentPosition) + "; Child:" + VectorToString(childPosition));
					scene->Context()->Log()->Error("Calculated: " + VectorToString(calculated) + "; Expected: " + VectorToString(expected) + "; Delta:" + VectorToString(calculated - expected));
				}
			}
			{
				childTransform->SetLocalScale(Vector3(-1.0f, -1.0f, -1.0f));
				const Vector3 calculated = childTransform->LocalToWorldPosition(point);
				const Vector3 expected = parentPosition + childPosition - point;
				bool match = VectorsMatch(calculated, expected);
				EXPECT_TRUE(match);
				if (!match) {
					logMatrix();
					scene->Context()->Log()->Info("Parent: " + VectorToString(parentPosition) + "; Child:" + VectorToString(childPosition));
					scene->Context()->Log()->Error("Calculated: " + VectorToString(calculated) + "; Expected: " + VectorToString(expected) + "; Delta:" + VectorToString(calculated - expected));
				}
			}
			{
				childTransform->SetLocalScale(Vector3(dis(rng), dis(rng), dis(rng)));
				const Vector3 calculated = childTransform->LocalToWorldPosition(point);
				const Vector3 expected = parentPosition + childPosition + point * childTransform->LocalScale();
				bool match = VectorsMatch(calculated, expected);
				EXPECT_TRUE(match);
				if (!match) {
					logMatrix();
					scene->Context()->Log()->Info("Parent: " + VectorToString(parentPosition) + "; Child:" + VectorToString(childPosition));
					scene->Context()->Log()->Error("Calculated: " + VectorToString(calculated) + "; Expected: " + VectorToString(expected) + "; Delta:" + VectorToString(calculated - expected));
				}
			}
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 parentPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 childPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 parentRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 childRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 parentScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 childScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 point(dis(rng), dis(rng), dis(rng));

			parentTransform->SetLocalPosition(parentPosition);
			parentTransform->SetLocalEulerAngles(parentRotation);
			parentTransform->SetLocalScale(parentScale);
			
			childTransform->SetLocalPosition(childPosition);
			childTransform->SetLocalEulerAngles(childRotation);
			childTransform->SetLocalScale(childScale);

			const Vector3 calculated = childTransform->LocalToWorldPosition(point);
			const Vector3 expected = parentTransform->LocalToParentSpacePosition(childTransform->LocalToParentSpacePosition(point));
			bool match = VectorsMatch(calculated, expected);
			EXPECT_TRUE(match);
			if (!match) {
				logMatrix();
				scene->Context()->Log()->Info("Parent: " + VectorToString(parentPosition) + "; Child:" + VectorToString(childPosition));
				scene->Context()->Log()->Error("Calculated: " + VectorToString(calculated) + "; Expected: " + VectorToString(expected) + "; Delta:" + VectorToString(calculated - expected));
			}
		}
	}

	// Basic tests for world euler angle set & get
	TEST(TransformTest, WorldRotation) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-180.0f, 180.0f);
		std::uniform_real_distribution<float> scaleDis(-10.0f, 10.0f);

		Transform* parentTransform = Object::Instantiate<Transform>(scene->RootObject(), "ParentTransform");
		Transform* childTransform = Object::Instantiate<Transform>(parentTransform, "ChildTransform");
		Transform* control = Object::Instantiate<Transform>(scene->RootObject(), "ControlTransform");

		{
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldEulerAngles(), Vector3(0.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldEulerAngles(), Vector3(0.0f, 0.0f, 0.0f)));
		}

		{
			const Vector3 parentRotation(8.0f, 9.0f, -12.0f);
			parentTransform->SetLocalEulerAngles(parentRotation);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldEulerAngles(), parentRotation));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldEulerAngles(), parentRotation));
		}

		{
			const Vector3 parentRotation(0.0f, 90.0, 0.0f);
			const Vector3 childRotation(20.0f, 0.0f, 30.0f);
			parentTransform->SetLocalEulerAngles(parentRotation);
			childTransform->SetLocalEulerAngles(childRotation);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldEulerAngles(), parentRotation));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldEulerAngles(), parentRotation + childRotation));
		}

		{
			const Vector3 parentRotation(16.0f, 90.0, -45.0f);
			const Vector3 childRotation(20.0f, 0.0f, 30.0f);
			parentTransform->SetLocalEulerAngles(parentRotation);
			childTransform->SetWorldEulerAngles(childRotation);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldEulerAngles(), parentRotation));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldEulerAngles(), childRotation));
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 parentPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 childPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 parentRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 childRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 parentScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 childScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 childWorldRotation(dis(rng), dis(rng), dis(rng));

			parentTransform->SetLocalPosition(parentPosition);
			parentTransform->SetLocalEulerAngles(parentRotation);
			parentTransform->SetLocalScale(parentScale);

			childTransform->SetLocalPosition(childPosition);
			childTransform->SetLocalEulerAngles(childRotation);
			childTransform->SetLocalScale(childScale);

			childTransform->SetWorldEulerAngles(childWorldRotation);
			control->SetLocalEulerAngles(childWorldRotation);

			EXPECT_TRUE(VectorsMatch(childTransform->Forward(), control->Forward()));
			EXPECT_TRUE(VectorsMatch(childTransform->Right(), control->Right()));
			EXPECT_TRUE(VectorsMatch(childTransform->Up(), control->Up()));
		}
	}

	// Basic tests for world position set & get
	TEST(TransformTest, WorldPosition) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-180.0f, 180.0f);
		std::uniform_real_distribution<float> scaleDis(-10.0f, 10.0f);

		Transform* parentTransform = Object::Instantiate<Transform>(scene->RootObject(), "ParentTransform");
		Transform* childTransform = Object::Instantiate<Transform>(parentTransform, "ChildTransform");

		{
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldPosition(), Vector3(0.0f, 0.0f, 0.0f)));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldPosition(), Vector3(0.0f, 0.0f, 0.0f)));
		}

		{
			const Vector3 parentPosition(8.0f, 9.0f, -12.0f);
			parentTransform->SetLocalPosition(parentPosition);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldPosition(), parentPosition));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldPosition(), parentPosition));
		}

		{
			const Vector3 parentPosition(8.0f, 9.0f, -12.0f);
			const Vector3 childPosition(75.0f, 121.0f, 122.0f);
			parentTransform->SetLocalPosition(parentPosition);
			childTransform->SetLocalPosition(childPosition);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldPosition(), parentPosition));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldPosition(), parentPosition + childPosition));
		}

		{
			const Vector3 parentPosition(8.0f, 9.0f, -12.0f);
			const Vector3 childPosition(75.0f, 121.0f, 122.0f);
			parentTransform->SetLocalPosition(parentPosition);
			childTransform->SetWorldPosition(childPosition);
			EXPECT_TRUE(VectorsMatch(parentTransform->WorldPosition(), parentPosition));
			EXPECT_TRUE(VectorsMatch(childTransform->WorldPosition(), childPosition));
		}

		for (size_t i = 0; i < 64; i++) {
			const Vector3 parentPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 childPosition(dis(rng), dis(rng), dis(rng));
			const Vector3 parentRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 childRotation(dis(rng), dis(rng), dis(rng));
			const Vector3 parentScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 childScale(scaleDis(rng), scaleDis(rng), scaleDis(rng));
			const Vector3 point(dis(rng), dis(rng), dis(rng));

			parentTransform->SetLocalPosition(parentPosition);
			parentTransform->SetLocalEulerAngles(parentRotation);
			parentTransform->SetLocalScale(parentScale);

			childTransform->SetLocalPosition(childPosition);
			childTransform->SetLocalEulerAngles(childRotation);
			childTransform->SetLocalScale(childScale);

			childTransform->SetWorldPosition(point);
			EXPECT_TRUE(VectorsMatch(childTransform->WorldPosition(), point));
			EXPECT_TRUE(VectorsMatch(childTransform->LocalToWorldPosition(Vector3(0.0f, 0.0f, 0.0f)), point));
		}
	}
}
