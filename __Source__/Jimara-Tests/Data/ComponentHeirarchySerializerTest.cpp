#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include "Data/Serialization/Helpers/SerializeToJson.h"
#include "Data/Serialization/Helpers/ComponentHeirarchySerializer.h"
#include "Data/Generators/MeshGenerator.h"
#include "Data/AssetDatabase/AssetSet.h"
#include "Components/Transform.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/GraphicsObjects/MeshRenderer.h"


namespace Jimara {
	namespace {
		inline static Reference<Scene> CreateScene() {
			Scene::CreateArgs args;
			args.createMode = Scene::CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_SUPRESS_WARNINGS;
			return Scene::Create(args);
		}
	}

	// Empty root object has to remain empty...
	TEST(ComponentHeirarchySerializerTest, EmptyRootObject) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		Component* root = scene->RootObject();
		ASSERT_NE(root, nullptr);
		const std::string initialName = root->Name();

		ComponentHeirarchySerializerInput serializerInput;

		bool error = false;
		serializerInput.rootComponent = scene->RootObject(); 
		nlohmann::json json = Serialization::SerializeToJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), scene->Context()->Log(), error,
			[&](const Serialization::SerializedObject&, bool&) -> nlohmann::json {
				assert(false);
				return "";
			});
		EXPECT_FALSE(error);
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		serializerInput.rootComponent = scene->RootObject();
		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		root->Name() = "DIFFERENT NAME THAN IT HAD PREVIOUSLY";
		EXPECT_NE(initialName, root->Name());

		serializerInput.rootComponent = scene->RootObject();
		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		Object::Instantiate<Component>(root, "ChildObject");
		EXPECT_EQ(scene->RootObject()->ChildCount(), 1);

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);
	}

	// Empty child object has to remain empty and keep it's type
	TEST(ComponentHeirarchySerializerTest, EmptyChildObject) {
		Reference<Scene> scene = CreateScene();
		ASSERT_NE(scene, nullptr);
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		Component* root = scene->RootObject();
		ASSERT_NE(root, nullptr);

		Component* child = Object::Instantiate<Component>(root, "ChildObject");
		const std::string initialName = child->Name();

		ComponentHeirarchySerializerInput serializerInput;

		bool error = false;
		serializerInput.rootComponent = child;
		nlohmann::json json = Serialization::SerializeToJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), scene->Context()->Log(), error,
			[&](const Serialization::SerializedObject&, bool&) -> nlohmann::json {
				assert(false);
				return "";
			});
		EXPECT_FALSE(error);
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);
		scene->Context()->Log()->Info(json.dump(1, '\t'));

		serializerInput.rootComponent = child;
		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);

		child->Name() = "DIFFERENT NAME THAN IT HAD PREVIOUSLY";
		EXPECT_NE(initialName, child->Name());
		Object::Instantiate<Component>(child, "Child Of Child");
		EXPECT_EQ(child->ChildCount(), 1);

		serializerInput.rootComponent = child;
		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);

		Transform* transform = Object::Instantiate<Transform>(root, "Transform");
		ASSERT_EQ(root->ChildCount(), 2);
		EXPECT_EQ(transform->IndexInParent(), 1);
		EXPECT_EQ(transform, root->GetChild(1));
		ASSERT_NE(transform, child);

		serializerInput.rootComponent = transform;
		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		ASSERT_EQ(root->ChildCount(), 2);
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);
		EXPECT_NE(transform, root->GetChild(1));
		EXPECT_EQ(root->GetChild(1)->Name(), child->Name());
		EXPECT_EQ(root->GetChild(1)->ChildCount(), 0);
		EXPECT_EQ(typeid(*root->GetChild(1)), typeid(Component));
	}

	namespace {
		class SphereMeshAsset : public virtual Asset::Of<TriMesh> {
		public:
			inline SphereMeshAsset() : Asset(GUID::Generate()) {}

		protected:
			virtual Reference<TriMesh> LoadItem() final override {
				return GenerateMesh::Tri::Sphere(Vector3(0.0f), 1.0f, 32, 16);
			}
		};

		class ComponentHeirarchySerializerTest_ObjectEmitter : public virtual Scene::LogicContext::UpdatingComponent {
		private:
			Reference<Transform> m_transform;

		public:
			inline ComponentHeirarchySerializerTest_ObjectEmitter(Component* parent, Transform* transform = nullptr)
				: Component(parent, "Emitter"), m_transform(transform) {}


			inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override {
				Component::GetFields(recordElement);
				static Transform* (*getFn)(ComponentHeirarchySerializerTest_ObjectEmitter*) =
					[](ComponentHeirarchySerializerTest_ObjectEmitter* target) -> Transform* { return target->m_transform; };
				static void(*setFn)(Transform* const&, ComponentHeirarchySerializerTest_ObjectEmitter*) =
					[](Transform* const& value, ComponentHeirarchySerializerTest_ObjectEmitter* target) { target->m_transform = value; };
				static const auto positionSerializer =
					Serialization::ValueSerializer<Transform*>::Create<ComponentHeirarchySerializerTest_ObjectEmitter>(
						"Transform", "Target transform",
						Function<Transform*, ComponentHeirarchySerializerTest_ObjectEmitter*>(getFn),
						Callback<Transform* const&, ComponentHeirarchySerializerTest_ObjectEmitter*>(setFn));
				recordElement(positionSerializer->Serialize(this));
			}

		protected:
			virtual void Update() final override {
				if (m_transform == nullptr) return;
				float x = 1.0f + 0.5f * sin(Context()->Time()->TotalScaledTime());
				m_transform->SetLocalScale(Vector3(x, 1.0f + 0.5f * cos(Context()->Time()->TotalScaledTime()), x));
			}
		};
	}

	template<> inline void TypeIdDetails::GetTypeAttributesOf<ComponentHeirarchySerializerTest_ObjectEmitter>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ComponentHeirarchySerializerTest_ObjectEmitter> serializer("ObjectEmitterSerializer");
		report(&serializer);
	}

	// Some MeshRenderers, lights and intertwined pointes
	TEST(ComponentHeirarchySerializerTest, ReloadScene) {
		Jimara::Test::TestEnvironment environment("This scene will be destroyed shortly...");

		Reference<const Object> objectEmitterToken = TypeId::Of<ComponentHeirarchySerializerTest_ObjectEmitter>().Register();

		AssetSet* database = dynamic_cast<AssetSet*>(environment.RootObject()->Context()->AssetDB());
		ASSERT_NE(database, nullptr);

		Reference<SphereMeshAsset> meshAsset = Object::Instantiate<SphereMeshAsset>();
		database->InsertAsset(meshAsset);

		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<DirectionalLight>(
				Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(0.0f), Vector3(45.0f, 60.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Mesh Transform");
			Reference<TriMesh> mesh = meshAsset->Load();
			Object::Instantiate<MeshRenderer>(transform, "Mesh Renderer", mesh);
			Object::Instantiate<ComponentHeirarchySerializerTest_ObjectEmitter>(environment.RootObject(), transform);
			});
		std::this_thread::sleep_for(std::chrono::seconds(2));

		bool error = false;
		nlohmann::json json;
		environment.ExecuteOnUpdateNow([&]() {
			ComponentHeirarchySerializerInput serializerInput;
			serializerInput.rootComponent = environment.RootObject();
			json = Serialization::SerializeToJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), environment.RootObject()->Context()->Log(), error,
				[&](const Serialization::SerializedObject&, bool&) -> nlohmann::json {
					environment.RootObject()->Context()->Log()->Fatal("ComponentHeirarchySerializer Not expected to yild any Object serializers!");
					return "";
				});
			});
		EXPECT_FALSE(error);
		environment.RootObject()->Context()->Log()->Info(json.dump(1, '\t'));

		environment.SetWindowName("Let's have a few blank seconds...");
		environment.ExecuteOnUpdateNow([&]() {
			environment.RootObject()->Destroy();
			});
		ASSERT_NE(environment.RootObject(), nullptr);
		std::this_thread::sleep_for(std::chrono::seconds(2));

		{
			ComponentHeirarchySerializerInput serializerInput;
			serializerInput.rootComponent = environment.RootObject();
			serializerInput.useUpdateQueue = true;
			error = !Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(serializerInput), json, environment.RootObject()->Context()->Log(),
				[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
					environment.RootObject()->Context()->Log()->Fatal("ComponentHeirarchySerializer Not expected to yild any Object serializers!");
					return false;
				});
			EXPECT_FALSE(error);
		};
		environment.SetWindowName("You should be looking at the restored scene");
	}
}
