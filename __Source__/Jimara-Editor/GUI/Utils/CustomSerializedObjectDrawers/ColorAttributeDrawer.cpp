#include "ColorAttributeDrawer.h"
#include "../DrawTooltip.h"
#include <optional>


namespace Jimara {
	namespace Editor {
		namespace {
			inline static const ColorAttributeDrawer* MainColorAttributeDrawer() {
				static const Reference<const ColorAttributeDrawer> drawer = Object::Instantiate<ColorAttributeDrawer>();
				return drawer;
			}
			inline static const constexpr Serialization::SerializerTypeMask ColorAttributeDrawerTypeMask() {
				return Serialization::SerializerTypeMask(Serialization::ItemSerializer::Type::VECTOR3_VALUE, Serialization::ItemSerializer::Type::VECTOR4_VALUE);
			}
		}

		bool ColorAttributeDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const  Function<bool, const Serialization::SerializedObject&>&, const Object*)const {
			if (object.Serializer() == nullptr) {
				if (logger != nullptr) logger->Error("ColorAttributeDrawer::DrawObject - Got nullptr serializer!");
				return false;
			}
			Serialization::ItemSerializer::Type type = object.Serializer()->GetType();
			if (!(ColorAttributeDrawerTypeMask() & type)) {
				if (logger != nullptr) logger->Error("ColorAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
					object.Serializer()->TargetName(), "; type:", static_cast<size_t>(type), ")");
				return false;
			}

			static thread_local const Serialization::ItemSerializer* lastSerializer = nullptr;
			static thread_local const void* lastTargetAddr = nullptr;
			const bool isSameSerializer = (lastSerializer == object.Serializer() && lastTargetAddr == object.TargetAddr());

			const std::string fieldName = DefaultGuiItemName(object, viewId);
			static const constexpr ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
			auto getRv = [&](auto call, auto... args) {
				bool rv = call(args...);
				if (rv) {
					ImGuiRenderer::FieldModified();
					lastSerializer = object.Serializer();
					lastTargetAddr = object.TargetAddr();
				}
				rv = ImGui::IsItemDeactivatedAfterEdit();
				if (rv) {
					lastSerializer = nullptr;
					lastTargetAddr = nullptr;
				}
				return rv;
			};
			bool rv = false;
			if (type == Serialization::ItemSerializer::Type::VECTOR3_VALUE) {
				Vector3 oldValue = object;
				Vector3 curValue = oldValue;
				float values[] = { curValue.x, curValue.y, curValue.z };
				rv = getRv(ImGui::ColorEdit3, fieldName.c_str(), values, colorEditFlags);
				if ((values[0] != oldValue.x) || (values[1] != oldValue.y) || (values[2] != oldValue.z))
					object = Vector3(values[0], values[1], values[2]);
			}
			else if (type == Serialization::ItemSerializer::Type::VECTOR4_VALUE) {
				Vector4 oldValue = object;
				Vector4 curValue = oldValue;
				float values[] = { curValue.x, curValue.y, curValue.z, curValue.w };
				rv = getRv(ImGui::ColorEdit4, fieldName.c_str(), values, colorEditFlags);
				if ((values[0] != oldValue.x) || (values[1] != oldValue.y) || (values[2] != oldValue.z) || (values[3] != oldValue.w))
					object = Vector4(values[0], values[1], values[2], values[3]);
			}
			else {
				if (logger != nullptr)
					logger->Error("ColorAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
						object.Serializer()->TargetName(), "; type:", static_cast<size_t>(type), ") <internal error>");
				return false;
			}
			DrawTooltip(fieldName, object.Serializer()->TargetHint());
			if (rv) ImGuiRenderer::FieldModified();
			return rv;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::ColorAttributeDrawer>() {
		Editor::MainColorAttributeDrawer()->Register(Editor::ColorAttributeDrawerTypeMask(), TypeId::Of<Serialization::ColorAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::ColorAttributeDrawer>() {
		Editor::MainColorAttributeDrawer()->Unregister(Editor::ColorAttributeDrawerTypeMask(), TypeId::Of<Serialization::ColorAttribute>());
	}
}
