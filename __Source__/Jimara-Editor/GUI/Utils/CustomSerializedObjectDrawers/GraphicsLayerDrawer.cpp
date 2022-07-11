#include "GraphicsLayerDrawer.h"
#include <Environment/Rendering/SceneObjects/GraphicsLayer.h>



namespace Jimara {
	namespace Editor {
		bool GraphicsLayerDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			const Serialization::ValueSerializer<GraphicsLayer>* serializer =
				dynamic_cast<const Serialization::ValueSerializer<GraphicsLayer>*>(object.Serializer());
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("GraphicsLayerDrawer::DrawObject - Unexpected serializer type! GraphicsLayers::LayerAttribute only supports Serialization::ValueSerializer<GraphicsLayer>");
				return false;
			}

			GraphicsLayer currentValue = static_cast<GraphicsLayer>(serializer->Get(object.TargetAddr()));
			GraphicsLayers::Reader layers;

			auto layerName = [&](auto id) {
				std::stringstream stream;
				stream << static_cast<size_t>(id) << ". " << layers[static_cast<GraphicsLayer>(id)];
				return stream.str();
			};

			const std::string fieldName = DefaultGuiItemName(object, viewId);
			const std::string preview = layerName(currentValue);
			if (!ImGui::BeginCombo(fieldName.c_str(), preview.c_str()))
				return false;

			GraphicsLayer newValue = currentValue;
			for (size_t i = 0; i < GraphicsLayers::Count(); i++) {
				const GraphicsLayer layer = static_cast<GraphicsLayer>(i);
				const std::string previewName = layerName(layer);
				const bool selected = (currentValue == layer);
				if (ImGui::Selectable(previewName.c_str(), selected))
					newValue = layer;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
			if (newValue != currentValue) {
				serializer->Set(newValue, object.TargetAddr());
				return true;
			}
			else return false;
		}

		bool GraphicsLayerMaskDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			const GraphicsLayerMask::Serializer* serializer = dynamic_cast<const GraphicsLayerMask::Serializer*>(object.Serializer());
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("GraphicsLayerMaskDrawer::DrawObject - Unexpected serializer type! GraphicsLayers::LayerMaskAttribute only supports GraphicsLayerMask::Serializer");
				return false;
			}

			GraphicsLayerMask& currentValue = *(GraphicsLayerMask*)object.TargetAddr();
			GraphicsLayers::Reader layers;

			auto layerName = [&](auto id) {
				std::stringstream stream;
				stream << static_cast<size_t>(id) << ". " << layers[static_cast<GraphicsLayer>(id)];
				return stream.str();
			};

			const std::string fieldName = DefaultGuiItemName(object, viewId);
			const std::string preview = [&]() -> std::string {
				if (currentValue == GraphicsLayerMask::All()) return "ALL";
				else if (currentValue == GraphicsLayerMask::Empty()) return "NONE";
				std::stringstream stream;
				bool isFirst = true;
				for (size_t i = 0; i < GraphicsLayers::Count(); i++) {
					const GraphicsLayer layer = static_cast<GraphicsLayer>(i);
					if (currentValue[layer]) {
						if (!isFirst) stream << ", ";
						if (layers[layer].length() > 0)
							stream << layers[layer];
						else stream << i;
						isFirst = false;
					}
				}
				return stream.str();
			}();
			if (!ImGui::BeginCombo(fieldName.c_str(), preview.c_str()))
				return false;

			GraphicsLayerMask newValue = currentValue;
			for (size_t i = 0; i < GraphicsLayers::Count(); i++) {
				const GraphicsLayer layer = static_cast<GraphicsLayer>(i);
				const std::string previewName = layerName(layer);
				bool selected = currentValue[layer];
				if (ImGui::Checkbox(previewName.c_str(), &selected))
					newValue[layer] = selected;
			}

			ImGui::EndCombo();
			if (newValue != currentValue) {
				currentValue = newValue;
				return true;
			}
			else return false;
		}

		namespace {
			static const GraphicsLayerDrawer mainGraphicsLayerDrawer;
			static const GraphicsLayerMaskDrawer mainGraphicsLayerMaskDrawer;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::GraphicsLayerDrawer>() {
		Editor::mainGraphicsLayerDrawer.Register(Serialization::ValueSerializer<GraphicsLayer>::SerializerType(), TypeId::Of<GraphicsLayers::LayerAttribute>());

	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::GraphicsLayerDrawer>() {
		Editor::mainGraphicsLayerDrawer.Unregister(Serialization::ValueSerializer<GraphicsLayer>::SerializerType(), TypeId::Of<GraphicsLayers::LayerAttribute>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::GraphicsLayerMaskDrawer>() {
		Editor::mainGraphicsLayerMaskDrawer.Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<GraphicsLayers::LayerMaskAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::GraphicsLayerMaskDrawer>() {
		Editor::mainGraphicsLayerMaskDrawer.Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<GraphicsLayers::LayerMaskAttribute>());
	}
}
