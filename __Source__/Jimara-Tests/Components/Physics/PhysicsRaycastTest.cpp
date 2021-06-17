#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "Physics/PhysicsInstance.h"
#include "../../CountingLogger.h"


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

			static PhysicsCollider* PRE_BLOCKED = nullptr;
			static auto PRE_BLOCKING_FILTER = Function<PhysicsScene::QueryFilterFlag, PhysicsCollider*>([](PhysicsCollider* collider) {
				return (collider == PRE_BLOCKED) ? PhysicsScene::QueryFilterFlag::DISCARD : PhysicsScene::QueryFilterFlag::REPORT;
				});

			static PhysicsCollider* POST_BLOCKED = nullptr;
			static auto POST_BLOCKING_FILTER = Function<PhysicsScene::QueryFilterFlag, const RaycastHit&>([](const RaycastHit& hit) {
				return (hit.collider == POST_BLOCKED) ? PhysicsScene::QueryFilterFlag::DISCARD : PhysicsScene::QueryFilterFlag::REPORT;
				});

			static std::vector<RaycastHit>* HITS = nullptr;
			static const Callback<const RaycastHit&> RECORD_HITS([](const RaycastHit& hit) { HITS->push_back(hit); });
		}

		// Simple tests for single hit raycasts, with or without layer based filtering
		TEST(PhysicsRaycastTest, Raycast_Single_Basic) {
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

			{
				logger->Info("Checking missing (small) distance");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 0.1f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); }));
				EXPECT_EQ(cnt, 0);
				EXPECT_EQ(cnt, hits.size());
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


		// Simple tests for single hit raycasts, with pre and post filtering
		TEST(PhysicsRaycastTest, Raycast_Single_CustomFilter) {
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

			static PhysicsCollider* PRE_BLOCKED = nullptr;
			static auto PRE_BLOCKING_FILTER = Function<PhysicsScene::QueryFilterFlag, PhysicsCollider*>([](PhysicsCollider* collider) {
				return (collider == PRE_BLOCKED) ? PhysicsScene::QueryFilterFlag::DISCARD : PhysicsScene::QueryFilterFlag::REPORT;
				});


			{
				logger->Info("Blocking boxB with pre filtering");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); })
					, PhysicsCollider::LayerMask::All(), false, &PRE_BLOCKING_FILTER);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxA);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.95f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxA with pre filtering");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); })
					, PhysicsCollider::LayerMask::All(), false, &PRE_BLOCKING_FILTER);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxB);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.45f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}


			{
				logger->Info("Blocking boxB with post filtering");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				POST_BLOCKED = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); })
					, PhysicsCollider::LayerMask::All(), false
					, nullptr, &POST_BLOCKING_FILTER);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxA);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.95f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxA with post filtering");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				POST_BLOCKED = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); })
					, PhysicsCollider::LayerMask::All(), false, nullptr, &POST_BLOCKING_FILTER);
				ASSERT_EQ(cnt, 1);
				ASSERT_EQ(cnt, hits.size());
				EXPECT_EQ(hits[0].collider, boxB);
				EXPECT_EQ(hits[0].point, Vector3(0, -0.45f, 0.0f));
				EXPECT_EQ(hits[0].normal, Vector3(0.0f, 1.0f, 0.0f));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxA with post filtering and boxB with preFiltering");
				static std::vector<RaycastHit>* HITS = nullptr;
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxB;
				POST_BLOCKED = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) { HITS->push_back(hit); })
					, PhysicsCollider::LayerMask::All(), false, &PRE_BLOCKING_FILTER, &POST_BLOCKING_FILTER);
				EXPECT_EQ(cnt, 0);
				EXPECT_EQ(cnt, hits.size());
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}


		// Simple tests for 'all' hit raycasts, without filtering, as well as with layer-based filtering
		TEST(PhysicsRaycastTest, Raycast_Multi_Basic) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<PhysicsInstance> physics = PhysicsInstance::Create(logger);
			Reference<PhysicsScene> scene = physics->CreateScene();
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

			int aId = -1, bId = -1;
			auto checkPresence = [&](const std::vector<RaycastHit>& hits) {
				aId = bId = -1;
				for (size_t i = 0; i < hits.size(); i++) {
					if (hits[i].collider == boxA) aId = static_cast<int>(i);
					else if (hits[i].collider == boxB) bId = static_cast<int>(i);
				}
			};

			{
				logger->Info("Checking no filtering...");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS, PhysicsCollider::LayerMask::All(), true);
				EXPECT_EQ(cnt, 2);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				ASSERT_GE(aId, 0);
				EXPECT_EQ(hits[aId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[aId].point, Vector3(0.0f, -0.95f, 0.0));
				ASSERT_GE(bId, 0);
				EXPECT_EQ(hits[bId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[bId].point, Vector3(0.0f, -0.45f, 0.0));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Checking only layer 0...");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS, PhysicsCollider::LayerMask(0), true);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				ASSERT_GE(aId, 0);
				EXPECT_EQ(hits[aId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[aId].point, Vector3(0.0f, -0.95f, 0.0));
				ASSERT_LT(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Checking only layer 63...");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS, PhysicsCollider::LayerMask(63), true);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				ASSERT_GE(bId, 0);
				EXPECT_EQ(hits[bId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[bId].point, Vector3(0.0f, -0.45f, 0.0));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Checking layers 64 and 7...");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS, PhysicsCollider::LayerMask(64, 7), true);
				EXPECT_EQ(cnt, 0);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				EXPECT_LT(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}

		// Simple tests for 'all' hit raycasts, with custom filters, but without blocking
		TEST(PhysicsRaycastTest, Raycast_Multi_NonBlockingFilters) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<PhysicsInstance> physics = PhysicsInstance::Create(logger);
			Reference<PhysicsScene> scene = physics->CreateScene();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			Reference<PhysicsCollider> boxA = CreateBox(scene, Vector3(0.0f, -1.0f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			Reference<PhysicsCollider> boxB = CreateBox(scene, Vector3(0.0f, -0.5f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			boxB->SetLayer(63u);

			ASSERT_EQ(logger->NumUnsafe(), 0);
			scene->SimulateAsynch(0.05f);
			scene->SynchSimulation();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			int aId = -1, bId = -1;
			auto checkPresence = [&](const std::vector<RaycastHit>& hits) {
				aId = bId = -1;
				for (size_t i = 0; i < hits.size(); i++) {
					if (hits[i].collider == boxA) aId = static_cast<int>(i);
					else if (hits[i].collider == boxB) bId = static_cast<int>(i);
				}
			};

			{
				logger->Info("Blocking boxB with pre filtering");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, &PRE_BLOCKING_FILTER);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				ASSERT_GE(aId, 0);
				EXPECT_EQ(hits[aId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[aId].point, Vector3(0.0f, -0.95f, 0.0));
				EXPECT_LT(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxB with post filtering");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				POST_BLOCKED = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, nullptr, &POST_BLOCKING_FILTER);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_GE(aId, 0);
				EXPECT_LT(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxA with pre filtering");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, &PRE_BLOCKING_FILTER);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				ASSERT_GE(bId, 0);
				EXPECT_EQ(hits[bId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[bId].point, Vector3(0.0f, -0.45f, 0.0));
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Blocking boxA with pre filtering and boxB with post filtering");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				PRE_BLOCKED = boxA;
				POST_BLOCKED = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, &PRE_BLOCKING_FILTER, &POST_BLOCKING_FILTER);
				EXPECT_EQ(cnt, 0);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				EXPECT_LT(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}

		// Simple tests for 'all' hit raycasts, with custom filters with blocking
		TEST(PhysicsRaycastTest, Raycast_Multi_BlockingFilters) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<PhysicsInstance> physics = PhysicsInstance::Create(logger);
			Reference<PhysicsScene> scene = physics->CreateScene();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			Reference<PhysicsCollider> boxA = CreateBox(scene, Vector3(0.0f, -1.0f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			Reference<PhysicsCollider> boxB = CreateBox(scene, Vector3(0.0f, -0.5f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			boxB->SetLayer(63u);

			ASSERT_EQ(logger->NumUnsafe(), 0);
			scene->SimulateAsynch(0.05f);
			scene->SynchSimulation();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			int aId = -1, bId = -1;
			auto checkPresence = [&](const std::vector<RaycastHit>& hits) {
				aId = bId = -1;
				for (size_t i = 0; i < hits.size(); i++) {
					if (hits[i].collider == boxA) aId = static_cast<int>(i);
					else if (hits[i].collider == boxB) bId = static_cast<int>(i);
				}
			};

			static PhysicsCollider* BLOCK_ON = nullptr;
			static auto PRE_BLOCK = Function<PhysicsScene::QueryFilterFlag, PhysicsCollider*>([](PhysicsCollider* collider) {
				return (collider != BLOCK_ON) ? PhysicsScene::QueryFilterFlag::REPORT : PhysicsScene::QueryFilterFlag::REPORT_BLOCK;
				});
			static auto POST_BLOCK = Function<PhysicsScene::QueryFilterFlag, const RaycastHit&>([](const RaycastHit& hit) {
				return (hit.collider != BLOCK_ON) ? PhysicsScene::QueryFilterFlag::REPORT : PhysicsScene::QueryFilterFlag::REPORT_BLOCK;
				});

			{
				logger->Info("Pre-Blocking on boxA");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				BLOCK_ON = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, &PRE_BLOCK);
				EXPECT_EQ(cnt, 2);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_GE(aId, 0);
				EXPECT_GE(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Pre-Blocking on boxB");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				BLOCK_ON = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, &PRE_BLOCK);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				EXPECT_GE(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Post-Blocking on boxA");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				BLOCK_ON = boxA;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, nullptr, &POST_BLOCK);
				EXPECT_EQ(cnt, 2);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_GE(aId, 0);
				EXPECT_GE(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}

			{
				logger->Info("Post-Blocking on boxB");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				BLOCK_ON = boxB;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, RECORD_HITS
					, PhysicsCollider::LayerMask::All(), true, nullptr, &POST_BLOCK);
				EXPECT_EQ(cnt, 1);
				EXPECT_EQ(cnt, hits.size());
				checkPresence(hits);
				EXPECT_LT(aId, 0);
				EXPECT_GE(bId, 0);
				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}

		// Basically, makes a query from a query callback
		TEST(PhysicsRaycastTest, Raycast_Multi_Recursive) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<PhysicsInstance> physics = PhysicsInstance::Create(logger);
			Reference<PhysicsScene> scene = physics->CreateScene();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			Reference<PhysicsCollider> boxA = CreateBox(scene, Vector3(0.0f, -1.0f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			Reference<PhysicsCollider> boxB = CreateBox(scene, Vector3(0.0f, -0.5f, 0.0f), Vector3(0.5f, 0.1f, 0.5f));
			boxB->SetLayer(63u);

			ASSERT_EQ(logger->NumUnsafe(), 0);
			scene->SimulateAsynch(0.05f);
			scene->SynchSimulation();
			ASSERT_EQ(logger->NumUnsafe(), 0);

			int aId = -1, bId = -1;
			auto checkPresence = [&](const std::vector<RaycastHit>& hits) {
				aId = bId = -1;
				for (size_t i = 0; i < hits.size(); i++) {
					if (hits[i].collider == boxA) aId = static_cast<int>(i);
					else if (hits[i].collider == boxB) bId = static_cast<int>(i);
				}
			};

			{
				logger->Info("Requesting raycast from within a raycast");
				std::vector<RaycastHit> hits;
				HITS = &hits;
				static std::vector<RaycastHit>* INNER = nullptr;
				std::vector<RaycastHit> innerHits;
				INNER = &innerHits;
				static PhysicsScene* SCENE = nullptr;
				SCENE = scene;
				static size_t innerCNT;
				size_t cnt = scene->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f
					, Callback<const RaycastHit&>([](const RaycastHit& hit) {
						RECORD_HITS(hit);
						innerCNT = SCENE->Raycast(Vector3(0.0f), Vector3(0.0f, -1.0f, 0.0f), 100.0f, Callback<const RaycastHit&>([](const RaycastHit& h) {
							INNER->push_back(h);
							}), PhysicsCollider::LayerMask::All(), true);
						}), PhysicsCollider::LayerMask::All(), true);
				EXPECT_EQ(cnt, 2);
				EXPECT_EQ(cnt, hits.size());

				checkPresence(hits);
				ASSERT_GE(aId, 0);
				EXPECT_EQ(hits[aId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[aId].point, Vector3(0.0f, -0.95f, 0.0));
				ASSERT_GE(bId, 0);
				EXPECT_EQ(hits[bId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[bId].point, Vector3(0.0f, -0.45f, 0.0));

				EXPECT_EQ(innerCNT, 2);
				EXPECT_EQ(innerHits.size(), 4);
				hits = innerHits;
				checkPresence(hits);
				ASSERT_GE(aId, 2);
				EXPECT_EQ(hits[aId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[aId].point, Vector3(0.0f, -0.95f, 0.0));
				ASSERT_GE(bId, 2);
				EXPECT_EQ(hits[bId].normal, Vector3(0.0f, 1.0f, 0.0));
				EXPECT_EQ(hits[bId].point, Vector3(0.0f, -0.45f, 0.0));

				EXPECT_EQ(logger->NumUnsafe(), 0);
			}
		}
	}
}
