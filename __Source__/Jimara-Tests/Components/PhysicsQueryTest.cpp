#pragma once
#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Physics/PhysicsInstance.h"
#include "../CountingLogger.h"


namespace Jimara {
	namespace Physics {
		namespace {
			inline static void SetPosition(PhysicsCollider* collider, const Vector3& position) {
				Matrix4 pose = Math::Identity();
				pose[3] = Vector4(position, 1.0f);
				collider->SetLocalPose(pose);
			}

			inline static Reference<PhysicsCollider> CreateBox(PhysicsScene* scene, const Vector3& position, const Vector3& size) {
				Reference<PhysicsBody> body = scene->AddStaticBody(Math::Identity());
				Reference<PhysicsCollider> collider = body->AddCollider(BoxShape(size), nullptr);
				SetPosition(collider, position);
				return collider;
			}
		}

		TEST(PhysicsQueryTest, Raycast_Single_Basic) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			ASSERT_EQ(logger->NumUnsafe(), 0);
			
			Reference<PhysicsInstance> physics = PhysicsInstance::Create(logger);
			ASSERT_NE(physics, nullptr);
			ASSERT_EQ(logger->NumUnsafe(), 0);

			Reference<PhysicsScene> scene = physics->CreateScene();
			ASSERT_NE(scene, nullptr);
			ASSERT_EQ(logger->NumUnsafe(), 0);

			Reference<PhysicsCollider> boxA = CreateBox(scene, Vector3(0.0f, -1.0f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			ASSERT_NE(boxA, nullptr);

			Reference<PhysicsCollider> boxB = CreateBox(scene, Vector3(0.0f, -0.5f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			ASSERT_NE(boxB, nullptr);
			boxB->SetLayer(63u);

			ASSERT_EQ(logger->NumUnsafe(), 0);
			scene->SimulateAsynch(0.05f);
			scene->SynchSimulation();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			{
				logger->Info("Checking no filtering...");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }));
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxB);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.45f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Checking only layer 0...");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PhysicsCollider::LayerMask mask(0);
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }), mask);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxA);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.95f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			SetPosition(boxA, Vector3(0.0f, -0.5f, 0.0f));
			SetPosition(boxB, Vector3(0.0f, -1.0f, 0.0f));

			ASSERT_EQ(logger->NumUnsafe(), 0);
			scene->SimulateAsynch(0.05f);
			scene->SynchSimulation();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			{
				logger->Info("[Reverse height] Checking no filtering...");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }));
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxA);
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("[Reverse height] Checking only layer 0...");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PhysicsCollider::LayerMask mask(0);
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }), mask);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxA);
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("[Reverse height] Checking only layer 63...");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PhysicsCollider::LayerMask mask(63);
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }), mask);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxB);
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}
	}
}
