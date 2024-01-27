#include "EnumAttributeDrawer.h"
#include "../DrawTooltip.h"
#include "../DrawMenuAction.h"
#include <Data/Serialization/Attributes/DrawDropdownMenuFoldersAttribute.h>
#include <numeric>


namespace Jimara {
	namespace Editor {
		namespace {
			struct DrawerResult {
				bool drawn;
				bool modified;
				inline DrawerResult(bool valid, bool changed) : drawn(valid), modified(changed) {}
			};

			inline static const EnumAttributeDrawer* MainEnumAttributeDrawer() {
				static const Reference<const EnumAttributeDrawer> drawer = Object::Instantiate<EnumAttributeDrawer>();
				return drawer;
			}

			template<typename GetValue, typename CallType>
			inline static void CallRegistrationCallback(const CallType& callback) {
				callback(Serialization::ItemSerializer::Type::BOOL_VALUE, GetValue::template Of<bool>());

				callback(Serialization::ItemSerializer::Type::CHAR_VALUE, GetValue::template Of<char>());
				callback(Serialization::ItemSerializer::Type::SCHAR_VALUE, GetValue::template Of<signed char>());
				callback(Serialization::ItemSerializer::Type::UCHAR_VALUE, GetValue::template Of<unsigned char>());
				callback(Serialization::ItemSerializer::Type::WCHAR_VALUE, GetValue::template Of<wchar_t>());

				callback(Serialization::ItemSerializer::Type::SHORT_VALUE, GetValue::template Of<short>());
				callback(Serialization::ItemSerializer::Type::USHORT_VALUE, GetValue::template Of<unsigned short>());

				callback(Serialization::ItemSerializer::Type::INT_VALUE, GetValue::template Of<int>());
				callback(Serialization::ItemSerializer::Type::UINT_VALUE, GetValue::template Of<unsigned int>());

				callback(Serialization::ItemSerializer::Type::LONG_VALUE, GetValue::template Of<long>());
				callback(Serialization::ItemSerializer::Type::ULONG_VALUE, GetValue::template Of<unsigned long>());

				callback(Serialization::ItemSerializer::Type::LONG_LONG_VALUE, GetValue::template Of<long long>());
				callback(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE, GetValue::template Of<unsigned long long>());

				callback(Serialization::ItemSerializer::Type::FLOAT_VALUE, GetValue::template Of<float>());
				callback(Serialization::ItemSerializer::Type::DOUBLE_VALUE, GetValue::template Of<double>());

				callback(Serialization::ItemSerializer::Type::VECTOR2_VALUE, GetValue::template Of<Vector2>());
				callback(Serialization::ItemSerializer::Type::VECTOR3_VALUE, GetValue::template Of<Vector3>());
				callback(Serialization::ItemSerializer::Type::VECTOR4_VALUE, GetValue::template Of<Vector4>());

				callback(Serialization::ItemSerializer::Type::MATRIX2_VALUE, GetValue::template Of<Matrix2>());
				callback(Serialization::ItemSerializer::Type::MATRIX3_VALUE, GetValue::template Of<Matrix3>());
				callback(Serialization::ItemSerializer::Type::MATRIX4_VALUE, GetValue::template Of<Matrix4>());

				callback(Serialization::ItemSerializer::Type::STRING_VIEW_VALUE, GetValue::template Of<std::string_view>());
				callback(Serialization::ItemSerializer::Type::WSTRING_VIEW_VALUE, GetValue::template Of<std::wstring_view>());
			}

			typedef DrawerResult(*DrawFn)(const Serialization::SerializedObject&, const std::string&, OS::Logger*, const Object*);

			inline static DrawerResult DrawUnsupportedType(const Serialization::SerializedObject& object, const std::string&, OS::Logger* logger, const Object*) {
				if (logger != nullptr)
					logger->Error("EnumAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
						object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType()), ") <internal error>");
				return DrawerResult(false, false);
			}

			template<typename Type>
			inline static std::enable_if_t<!std::is_same_v<Type, bool>, Type> InvertBits(Type bits) { return Type(~static_cast<unsigned long long>(bits)); }
			inline static bool InvertBits(bool bits) { return !bits; }

			template<typename Type>
			inline static std::enable_if_t<std::numeric_limits<Type>::is_integer, DrawerResult> DrawBitmaskComboMenu(
				const Serialization::SerializedObject& object, const std::string& name, const Serialization::EnumAttribute<Type>* attribute) {
				if (!attribute->IsBitmask()) DrawerResult(false, false);
				const Type initialValue = object;
				const std::string selectedName = [&]() -> std::string {
					std::stringstream stream;
					bool notFirst = false;
					for (size_t i = 0; i < attribute->ChoiceCount(); i++) {
						const typename Serialization::EnumAttribute<Type>::Choice& choice = attribute->operator[](i);
						if ((choice.value & initialValue) == choice.value) {
							if (notFirst) stream << ", ";
							else notFirst = true;
							stream << choice.name;
						}
					}
					return stream.str();
				}();
				bool modified = false;
				if (ImGui::BeginCombo(name.c_str(), selectedName.c_str())) {
					Type currentValue = initialValue;
					for (size_t i = 0; i < attribute->ChoiceCount(); i++) {
						const typename Serialization::EnumAttribute<Type>::Choice& choice = attribute->operator[](i);
						bool contains = ((currentValue & choice.value) == choice.value);
						if (ImGui::Checkbox(choice.name.c_str(), &contains)) {
							if (contains) currentValue |= choice.value;
							else currentValue &= InvertBits(choice.value);
							modified = true;
						}
					}
					ImGui::EndCombo();
					if (currentValue != initialValue)
						object = currentValue;
				}
				return DrawerResult(true, modified);
			}

			template<typename Type>
			inline static std::enable_if_t<!std::numeric_limits<Type>::is_integer, DrawerResult> DrawBitmaskComboMenu(
				const Serialization::SerializedObject&, const std::string&, const Serialization::EnumAttribute<Type>*) {
				return DrawerResult(false, false);
			}

			template<typename Type>
			inline static DrawerResult DrawComboMenuFor(const Serialization::SerializedObject& object, const std::string& name, OS::Logger* logger, const Object* enumAttribute) {
				const Serialization::EnumAttribute<Type>* attribute = dynamic_cast<const Serialization::EnumAttribute<Type>*>(enumAttribute);
				if (attribute == nullptr) {
					if (logger != nullptr)
						logger->Error("EnumAttribute::DrawObject - Incorrect attribute type! (TargetName: ",
							object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType())
							, "; Expected attribute type: \"", TypeId::Of<Serialization::EnumAttribute<Type>>().Name(), "\")");
					return DrawerResult(false, false);
				}
				if (attribute->IsBitmask()) {
					DrawerResult bitmaskMenuRV = DrawBitmaskComboMenu<Type>(object, name, attribute);
					if (bitmaskMenuRV.drawn) return bitmaskMenuRV;
				}
				{
					const Type initialValue = object;
					const size_t currentItemIndex = [&]() -> size_t {
						for (size_t i = 0; i < attribute->ChoiceCount(); i++)
							if (attribute->operator[](i).value == initialValue) return i;
						return attribute->ChoiceCount();
					}();
					bool modified = false;
					if (ImGui::BeginCombo(name.c_str(), (currentItemIndex >= attribute->ChoiceCount()) ? "" : attribute->operator[](currentItemIndex).name.c_str())) {
						size_t selected = currentItemIndex;
						const bool shouldDrawMenuActions = object.Serializer()->FindAttributeOfType<Serialization::DrawDropdownMenuFoldersAttribute>() != nullptr;
						for (size_t i = 0; i < attribute->ChoiceCount(); i++) {
							bool isSelected = (selected == i);
							const typename Serialization::EnumAttribute<Type>::Choice& choice = attribute->operator[](i);
							const bool pressed = shouldDrawMenuActions
								? DrawMenuAction(choice.name.c_str(), object.Serializer()->TargetHint(), &choice, isSelected)
								: ImGui::Selectable(choice.name.c_str(), isSelected);
							if (pressed) {
								selected = i;
								modified = true;
							}
							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
						if (selected != currentItemIndex)
							object = attribute->operator[](selected).value;
					}
					return DrawerResult(true, modified);
				}
			}

			struct GetSimpleDrawFunction {
				template<typename Type>
				inline static constexpr DrawFn Of() { return DrawComboMenuFor<Type>; };
			};

			struct GetTypeIdOfAttribute {
				template<typename Type>
				inline static constexpr TypeId Of() { return TypeId::Of<Serialization::EnumAttribute<Type>>(); };
			};
		}

		bool EnumAttributeDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>&, const Object* attribute)const {
			if (object.Serializer() == nullptr) {
				if (logger != nullptr) logger->Error("EnumAttributeDrawer::DrawObject - Got nullptr serializer!");
				return false;
			}
			Serialization::ItemSerializer::Type type = object.Serializer()->GetType();
			if (!(Serialization::SerializerTypeMask::AllValueTypes() & type)) {
				if (logger != nullptr) logger->Error("EnumAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
					object.Serializer()->TargetName(), "; type:", static_cast<size_t>(type), ")");
				return false;
			}
			const std::string fieldName = DefaultGuiItemName(object, viewId);
			static const DrawFn* DRAW_FUNCTIONS = []() -> const DrawFn* {
				static const constexpr size_t SERIALIZER_TYPE_COUNT = static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static DrawFn drawFunctions[SERIALIZER_TYPE_COUNT];
				for (size_t i = 0; i < SERIALIZER_TYPE_COUNT; i++)
					drawFunctions[i] = DrawUnsupportedType;

				CallRegistrationCallback<GetSimpleDrawFunction>([&](Serialization::ItemSerializer::Type type, DrawFn fn) {
					drawFunctions[static_cast<size_t>(type)] = fn;
					});

				return drawFunctions;
			}();
			DrawerResult result = DRAW_FUNCTIONS[static_cast<size_t>(type)](object, fieldName, logger, attribute);
			if (result.drawn)
				DrawTooltip(fieldName, object.Serializer()->TargetHint());
			else if (result.modified) ImGuiRenderer::FieldModified();
			return result.modified;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::EnumAttributeDrawer>() {
		Editor::CallRegistrationCallback<Editor::GetTypeIdOfAttribute>([](Serialization::ItemSerializer::Type type, TypeId id) {
			Editor::MainEnumAttributeDrawer()->Register(type, id);
			});
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::EnumAttributeDrawer>() {
		Editor::CallRegistrationCallback<Editor::GetTypeIdOfAttribute>([](Serialization::ItemSerializer::Type type, TypeId id) {
			Editor::MainEnumAttributeDrawer()->Unregister(type, id);
			});
	}
}
