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
	}
}
