#include "../GtestHeaders.h"
#include "Data/Serialization/Helpers/SerializeToJson.h"
#include "Data/Serialization/Helpers/ComponentHeirarchySerializer.h"


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

		bool error = false;
		nlohmann::json json = Serialization::SerializeToJson(ComponentHeirarchySerializer::Instance()->Serialize(scene->RootObject()), scene->Context()->Log(), error,
			[&](const Serialization::SerializedObject&, bool&) -> nlohmann::json {
				assert(false);
				return "";
			});
		EXPECT_FALSE(error);
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(scene->RootObject()), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		root->Name() = "DIFFERENT NAME THAN IT HAD PREVIOUSLY";
		EXPECT_NE(initialName, root->Name());

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(scene->RootObject()), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(scene->RootObject(), root);
		EXPECT_EQ(initialName, root->Name());
		EXPECT_EQ(scene->RootObject()->ChildCount(), 0);

		Object::Instantiate<Component>(root, "ChildObject");
		EXPECT_EQ(scene->RootObject()->ChildCount(), 1);

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(scene->RootObject()), json, scene->Context()->Log(),
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

		bool error = false;
		nlohmann::json json = Serialization::SerializeToJson(ComponentHeirarchySerializer::Instance()->Serialize(child), scene->Context()->Log(), error,
			[&](const Serialization::SerializedObject&, bool&) -> nlohmann::json {
				assert(false);
				return "";
			});
		EXPECT_FALSE(error);
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);
		scene->Context()->Log()->Info(json.dump(1, '\t'));

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(child), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);

		child->Name() = "DIFFERENT NAME THAN IT HAD PREVIOUSLY";
		EXPECT_NE(initialName, child->Name());

		EXPECT_TRUE(Serialization::DeserializeFromJson(ComponentHeirarchySerializer::Instance()->Serialize(child), json, scene->Context()->Log(),
			[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
				assert(false);
				return false;
			}));
		EXPECT_EQ(child, root->GetChild(0));
		EXPECT_EQ(initialName, child->Name());
		EXPECT_EQ(child->ChildCount(), 0);
	}
}
