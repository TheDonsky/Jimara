#include "LayerDrawer.h"
#include <Jimara/Environment/Layers.h>



namespace Jimara {
	namespace Editor {
		bool LayerDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			const Serialization::ValueSerializer<Layer>* serializer =
				dynamic_cast<const Serialization::ValueSerializer<Layer>*>(object.Serializer());
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("LayerDrawer::DrawObject - Unexpected serializer type! Layers::LayerAttribute only supports Serialization::ValueSerializer<Layer>");
				return false;
			}

			Layer currentValue = static_cast<Layer>(serializer->Get(object.TargetAddr()));
			Layers::Reader layers;

			auto layerName = [&](auto id) {
				std::stringstream stream;
				stream << static_cast<size_t>(id) << ". " << layers[static_cast<Layer>(id)];
				return stream.str();
			};

			const std::string fieldName = DefaultGuiItemName(object, viewId);
			const std::string preview = layerName(currentValue);
			if (!ImGui::BeginCombo(fieldName.c_str(), preview.c_str()))
				return false;

			Layer newValue = currentValue;
			for (size_t i = 0; i < Layers::Count(); i++) {
				const Layer layer = static_cast<Layer>(i);
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

		bool LayerMaskDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			const LayerMask::Serializer* serializer = dynamic_cast<const LayerMask::Serializer*>(object.Serializer());
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("LayerMaskDrawer::DrawObject - Unexpected serializer type! Layers::LayerMaskAttribute only supports LayerMask::Serializer");
				return false;
			}

			LayerMask& currentValue = *(LayerMask*)object.TargetAddr();
			Layers::Reader layers;

			auto layerName = [&](auto id) {
				std::stringstream stream;
				stream << static_cast<size_t>(id) << ". " << layers[static_cast<Layer>(id)];
				return stream.str();
			};

			const std::string fieldName = DefaultGuiItemName(object, viewId);
			const std::string preview = [&]() -> std::string {
				if (currentValue == LayerMask::All()) return "ALL";
				else if (currentValue == LayerMask::Empty()) return "NONE";
				std::stringstream stream;
				bool isFirst = true;
				for (size_t i = 0; i < Layers::Count(); i++) {
					const Layer layer = static_cast<Layer>(i);
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

			LayerMask newValue = currentValue;

			// All/None:
			{
				bool selectAll = (newValue == LayerMask::All());
				if (ImGui::Checkbox("All", &selectAll)) {
					if (selectAll)
						newValue = LayerMask::All();
					else newValue = LayerMask::Empty();
				}
			}

			// Relevant layers:
			for (size_t i = 0; i < Layers::Count(); i++) {
				const Layer layer = static_cast<Layer>(i);
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
			static const LayerDrawer mainLayerDrawer;
			static const LayerMaskDrawer mainLayerMaskDrawer;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::LayerDrawer>() {
		Editor::mainLayerDrawer.Register(Serialization::ValueSerializer<Layer>::SerializerType(), TypeId::Of<Layers::LayerAttribute>());

	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::LayerDrawer>() {
		Editor::mainLayerDrawer.Unregister(Serialization::ValueSerializer<Layer>::SerializerType(), TypeId::Of<Layers::LayerAttribute>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::LayerMaskDrawer>() {
		Editor::mainLayerMaskDrawer.Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<Layers::LayerMaskAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::LayerMaskDrawer>() {
		Editor::mainLayerMaskDrawer.Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<Layers::LayerMaskAttribute>());
	}
}
