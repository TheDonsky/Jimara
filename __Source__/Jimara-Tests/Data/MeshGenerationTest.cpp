#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshFromSpline.h>
#include <Components/Lights/DirectionalLight.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace {
		inline static void CreateLights(Component* rootObject) {
			Reference<Transform> sun = Object::Instantiate<Transform>(rootObject, "Sun", Vector3(0.0f), Vector3(64.0f, 32.0f, 0.0f));
			Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
			Reference<Transform> back = Object::Instantiate<Transform>(rootObject, "Sun");
			back->LookTowards(-sun->Forward());
			Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
		}
	}

	// Basic tests for MeshFromSpline
	TEST(MeshGenerationTest, MeshFromSpline) {
		Jimara::Test::TestEnvironment environment("MeshFromSpline");

		Reference<MeshRenderer> renderer;
		environment.ExecuteOnUpdateNow([&]() {
			CreateLights(environment.RootObject());
			const Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject());
			renderer = Object::Instantiate<MeshRenderer>(transform, "Renderer");
			});
		auto setMesh = [&](TriMesh* mesh) { 
			environment.ExecuteOnUpdateNow([&]() {
				renderer->SetMesh(mesh);
				environment.SetWindowName("MeshFromSpline - " + TriMesh::Reader(mesh).Name());
				}); 
		};

		typedef std::vector<MeshFromSpline::SplineVertex> Spline;
		typedef std::vector<Vector2> Shape;
		auto setMeshFromSpline = [&](const Spline& spline, const Shape& shape, MeshFromSpline::Flags flags, const std::string_view& name) {
			auto getSplineVertex = [&](uint32_t index) -> MeshFromSpline::SplineVertex { return spline[index]; };
			auto getShapeVertex = [&](uint32_t index) -> Vector2 { return shape[index]; };
			const Reference<TriMesh> mesh = MeshFromSpline::Tri(
				MeshFromSpline::SplineCurve::FromCall(&getSplineVertex), static_cast<uint32_t>(spline.size()),
				MeshFromSpline::RingCurve::FromCall(&getShapeVertex), static_cast<uint32_t>(shape.size()),
				flags, name);
			setMesh(mesh);
			return mesh;
		};

		const Shape circle = []() {
			Shape shape;
			const constexpr uint32_t segments = 24;
			const constexpr float angleStep = Math::Radians(360.0f / static_cast<float>(segments));
			for (uint32_t i = 0; i < segments; i++) {
				float angle = angleStep * i;
				shape.push_back(Vector2(std::cos(angle), std::sin(angle)));
			}
			return shape;
		}();

		{
			const Spline splineVerts = {
				{ Vector3(0.0f), Math::Right(), Math::Forward() },
				{ Math::Up(), Math::Right(), Math::Forward() }
			};
			EXPECT_NE(setMeshFromSpline(splineVerts, circle, MeshFromSpline::Flags::CAP_ENDS, "Cylinder (XZ)"), nullptr);
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(4));
			const Spline splineVerts = {
				{ Vector3(0.0f), -Math::Forward() * 0.5f, Math::Right() * 0.5f },
				{ Math::Up(), -Math::Forward() * 0.5f, Math::Right() * 0.5f }
			};
			EXPECT_NE(setMeshFromSpline(splineVerts, circle, MeshFromSpline::Flags::CAP_ENDS, "Cylinder (-ZX)"), nullptr);
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(4));
			const Spline splineVerts = {
				{ Vector3(0.0f), Math::Right(), Math::Forward() },
				{ Math::Up(), Math::Right(), Math::Forward() }
			};
			EXPECT_NE(setMeshFromSpline(splineVerts, circle, MeshFromSpline::Flags::CLOSE_SHAPE, "Cylinder (NO CAPS)"), nullptr);
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(4));
			const Spline splineVerts = {
				{ Vector3(0.0f), Math::Right(), Math::Forward() },
				{ Math::Up(), Math::Right(), Math::Forward() }
			};
			EXPECT_NE(setMeshFromSpline(splineVerts, circle, MeshFromSpline::Flags::NONE, "Cylinder (NO CAPS NO CLOSE)"), nullptr);
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(4));
			const constexpr uint32_t segments = 32;
			const constexpr float angleStep = Math::Radians(360.0f / static_cast<float>(segments));
			Spline splineVerts;
			for (uint32_t i = 0; i < segments; i++) {
				MeshFromSpline::SplineVertex vertex = {};
				float angle = angleStep * i;
				vertex.position = Vector3(std::cos(angle), 0.0f, std::sin(angle));
				vertex.right = Math::Up() * 0.5f;
				vertex.up = vertex.position * 0.5f;
				splineVerts.push_back(vertex);
			}
			EXPECT_NE(setMeshFromSpline(splineVerts, circle, MeshFromSpline::Flags::CLOSE_SPLINE_AND_SHAPE, "Torus"), nullptr);
		}
	}
}
