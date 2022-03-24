#include "DrawSerializedObject.h"
#include "DrawTooltip.h"
#include <Data/Serialization/Attributes/HideInEditorAttribute.h>
#include <Data/Serialization/Attributes/EulerAnglesAttribute.h>
#include <optional>

namespace Jimara {
	namespace Editor {
		namespace {
			static const constexpr float BASE_DRAG_SPEED = 0.1f;
			static const constexpr float EULER_DRAG_SPEED = 1.0f;

			inline static float DragSpeed(const Serialization::SerializedObject& object) {
				return object.Serializer()->FindAttributeOfType<Serialization::EulerAnglesAttribute>() != nullptr ? EULER_DRAG_SPEED : BASE_DRAG_SPEED;
			}

			inline static bool DrawUnsupportedTypeError(const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger) {
				if (logger != nullptr) logger->Error("DrawSerializedObject - unsupported Serializer type! (Name: \""
					, object.Serializer()->TargetName(), "\";type:", static_cast<size_t>(object.Serializer()->GetType()), ")");
				return true;
			}

			template<typename Type, bool AutoTooltip = true, typename ImGuiFN>
			inline static bool DrawSerializerOfType(const Serialization::SerializedObject& object, size_t viewId, const ImGuiFN& imGuiFn) {
				const Type initialValue = object;
				const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, viewId);
				
				static thread_local const Serialization::ItemSerializer* lastSerializer = nullptr;
				static thread_local const void* lastTargetAddr = nullptr;
				const bool isSameObject = (object.Serializer() == lastSerializer && object.TargetAddr() == lastTargetAddr);

				static const constexpr bool isInteger = std::numeric_limits<Type>::is_integer;
				static thread_local std::optional<Type> lastValue;

				Type value = (isSameObject && lastValue.has_value()) ? lastValue.value() : initialValue;
				bool changed = imGuiFn(name.c_str(), &value);
				if (AutoTooltip)
					DrawTooltip(name, object.Serializer()->TargetHint());
				if (changed) {
					ImGuiRenderer::FieldModified();
					lastSerializer = object.Serializer();
					lastTargetAddr = object.TargetAddr();
				}
				bool nothingActive = !ImGui::IsAnyItemActive();
				if (changed && value != initialValue) {
					if (isInteger) lastValue = value;
					else object = value;
				}
				changed = nothingActive && (isSameObject || changed);
				if (nothingActive) {
					lastSerializer = nullptr;
					lastTargetAddr = nullptr;
					if (lastValue.has_value() && isSameObject) {
						object = value;
						lastValue = std::optional<Type>();
					}
				}
				return changed;
			}

			inline static bool DrawBoolValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<bool>(object, viewId, ImGui::Checkbox);
			}

			inline static bool DrawCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImS8, signed char>);
				return DrawSerializerOfType<char>(object, viewId, [](const char* name, char* value) {
					return ImGui::InputScalar(name, ImGuiDataType_S8, reinterpret_cast<signed char*>(value));
					});
			}
			inline static bool DrawSCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImS8, signed char>);
				return DrawSerializerOfType<signed char>(object, viewId, [](const char* name, signed char* value) {
					return ImGui::InputScalar(name, ImGuiDataType_S8, value);
					});
			}
			inline static bool DrawUCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImU8, unsigned char>);
				return DrawSerializerOfType<unsigned char>(object, viewId, [](const char* name, unsigned char* value) {
					return ImGui::InputScalar(name, ImGuiDataType_U8, value);
					});
			}
			inline static bool DrawWCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(
					((sizeof(ImWchar16) == sizeof(wchar_t)) && std::is_same_v<ImU16, ImWchar16>) ||
					((sizeof(ImWchar32) == sizeof(wchar_t)) && std::is_same_v<ImU32, ImWchar32>));
				if ((sizeof(ImWchar16) == sizeof(wchar_t)) && std::is_same_v<ImU16, ImWchar16>) {
					return DrawSerializerOfType<wchar_t>(object, viewId, [](const char* name, wchar_t* value) {
						return ImGui::InputScalar(name, ImGuiDataType_U16, reinterpret_cast<ImWchar16*>(value));
						});
				}
				else return DrawSerializerOfType<wchar_t>(object, viewId, [](const char* name, wchar_t* value) {
					return ImGui::InputScalar(name, ImGuiDataType_U32, reinterpret_cast<ImWchar32*>(value));
					});
			}

			inline static bool DrawShortValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(short) == sizeof(ImS16));
				return DrawSerializerOfType<short>(object, viewId, [](const char* name, short* value) {
					return ImGui::InputScalar(name, ImGuiDataType_S16, value);
					});
			}
			inline static bool DrawUShortValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned short) == sizeof(ImU16));
				return DrawSerializerOfType<unsigned short>(object, viewId, [](const char* name, unsigned short* value) {
					return ImGui::InputScalar(name, ImGuiDataType_U16, value);
					});
			}

			inline static bool DrawIntValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<int>(object, viewId, [](const char* name, int* value) { return ImGui::InputInt(name, value); });
			}
			inline static bool DrawUIntValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned int) == sizeof(ImU32));
				return DrawSerializerOfType<unsigned int>(object, viewId, [](const char* name, unsigned int* value) {
					return ImGui::InputScalar(name, ImGuiDataType_U32, value);
					});
			}

			inline static bool DrawLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<long>(object, viewId, [](const char* name, long* value) {
						return ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					static_assert(sizeof(long long) == sizeof(ImS64));
					return DrawSerializerOfType<long>(object, viewId, [](const char* name, long* value) {
						return ImGui::InputScalar(name, ImGuiDataType_S64, value);
						});
				}
			}
			inline static bool DrawULongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				static_assert(sizeof(int) == 4);
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<unsigned long>(object, viewId, [](const char* name, unsigned long* value) {
						return ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					static_assert(sizeof(unsigned long long) == sizeof(ImU64));
					return DrawSerializerOfType<unsigned long>(object, viewId, [](const char* name, unsigned long* value) {
						return ImGui::InputScalar(name, ImGuiDataType_U64, value);
						});
				}
			}

			inline static bool DrawLongLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(long long) == sizeof(ImS64));
				static_assert(sizeof(long long) == sizeof(ImU64));
				return DrawSerializerOfType<long long>(object, viewId, [](const char* name, long long* value) {
					return ImGui::InputScalar(name, ImGuiDataType_S64, value);
					});
			}
			inline static bool DrawULongLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned long long) == sizeof(ImU64));
				return DrawSerializerOfType<unsigned long long>(object, viewId, [](const char* name, unsigned long long* value) {
					return ImGui::InputScalar(name, ImGuiDataType_U64, value);
					});
			}

			inline static bool DrawFloatValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<float>(object, viewId, [](const char* name, float* value) { return ImGui::InputFloat(name, value); });
			}
			inline static bool DrawDoubleValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<double>(object, viewId, [](const char* name, double* value) { return ImGui::InputDouble(name, value); });
			}

			inline static bool DrawVector2Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<Vector2>(object, viewId, [&](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					bool rv = ImGui::DragFloat2(name, fields, DragSpeed(object));
					(*value) = Vector2(fields[0], fields[1]);
					return rv;
					});
			}
			inline static bool DrawVector3Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<Vector3>(object, viewId, [&](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					bool rv = ImGui::DragFloat3(name, fields, DragSpeed(object));
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					return rv;
					});
			}
			inline static bool DrawVector4Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawSerializerOfType<Vector4>(object, viewId, [&](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					bool rv = ImGui::DragFloat4(name, fields, DragSpeed(object));
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					return rv;
					});
			}

			template<typename MatrixType, size_t NumSubfields, typename FieldInputCallback>
			inline static bool DrawMatrixValue(const Serialization::SerializedObject& object, size_t viewId, const FieldInputCallback& fieldInput) {
				return DrawSerializerOfType<MatrixType, false>(object, viewId, [&](const char* name, MatrixType* value) {
					bool nodeExpanded = ImGui::TreeNode(name);
					DrawTooltip(name, object.Serializer()->TargetHint());
					bool rv = false;
					if (nodeExpanded) {
						for (typename MatrixType::length_type i = 0; i < NumSubfields; i++) {
							const std::string fieldName = [&]() {
								std::stringstream stream;
								stream << "###DrawSerializedObject_for_view_" << viewId
									<< "_serializer_" << ((size_t)(object.Serializer()))
									<< "_target_" << ((size_t)(object.TargetAddr()))
									<< "_subfield_" << i;
								return stream.str();
							}();
							rv |= fieldInput(fieldName.c_str(), &(*value)[i]);
						}
						ImGui::TreePop();
					}
					return rv;
					});
			}

			inline static bool DrawMatrix2Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawMatrixValue<Matrix2, 2>(object, viewId, [](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					bool rv = ImGui::InputFloat2(name, fields);
					(*value) = Vector2(fields[0], fields[1]);
					return rv;
				});
			}
			inline static bool DrawMatrix3Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawMatrixValue<Matrix3, 3>(object, viewId, [](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					bool rv = ImGui::InputFloat3(name, fields);
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					return rv;
					});
			}
			inline static bool DrawMatrix4Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawMatrixValue<Matrix4, 4>(object, viewId, [](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					bool rv = ImGui::InputFloat4(name, fields);
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					return rv;
					});
			}

			template<typename SetNewTextCallback>
			inline static bool DrawStringViewValue(
				const Serialization::SerializedObject& object, size_t viewId,
				const std::string_view& currentText, const SetNewTextCallback& setNewText) {
				static thread_local std::vector<char> textBuffer;
				{
					if (textBuffer.size() <= (currentText.length() + 1))
						textBuffer.resize(currentText.length() + 512);
					memcpy(textBuffer.data(), currentText.data(), currentText.length());
					textBuffer[currentText.length()] = '\0';
				}
				const std::string nameId = CustomSerializedObjectDrawer::DefaultGuiItemName(object, viewId);
				bool rv = ImGui::InputTextWithHint(nameId.c_str(), object.Serializer()->TargetHint().c_str(), textBuffer.data(), textBuffer.size());
				DrawTooltip(nameId, object.Serializer()->TargetHint());
				if (strlen(textBuffer.data()) != currentText.length() || memcmp(textBuffer.data(), currentText.data(), currentText.length()) != 0)
					setNewText(std::string_view(textBuffer.data()));
				if (rv) ImGuiRenderer::FieldModified();
				return ImGui::IsItemDeactivatedAfterEdit();
			}
			inline static bool DrawStringViewValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				return DrawStringViewValue(object, viewId, object, [&](const std::string_view& newText) { object = newText; });
			}
			inline static bool DrawWStringViewValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				const std::wstring_view wideView = object;
				const std::string wstringToString = Convert<std::string>(wideView);
				return DrawStringViewValue(object, viewId, wstringToString, [&](const std::string_view& newText) { 
					const std::wstring wideNewText = Convert<std::wstring>(newText);
					object = std::wstring_view(wideNewText);
					});
			}


			static const constexpr size_t SERIALIZER_TYPE_COUNT = static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
			typedef std::unordered_multiset<Reference<const CustomSerializedObjectDrawer>> CustomSerializedObjectDrawersSet;
			struct CustomSerializedObjectDrawersPerSerializerType {
				CustomSerializedObjectDrawersSet drawFunctions[SERIALIZER_TYPE_COUNT];
			};
			typedef std::unordered_map<std::type_index, CustomSerializedObjectDrawersPerSerializerType> CustomSerializedObjectDrawersPerAttributeType;
			inline static SpinLock& CustomSerializedObjectDrawerLock() {
				static SpinLock lock;
				return lock;
			}
			inline static CustomSerializedObjectDrawersPerAttributeType& CustomSerializedObjectDrawers() {
				static CustomSerializedObjectDrawersPerAttributeType drawers;
				return drawers;
			}
			class CustomSerializedObjectDrawersPerAttributeTypeSnapshot : public virtual Object {
			private:
				inline static Reference<CustomSerializedObjectDrawersPerAttributeTypeSnapshot>& Current() {
					static Reference<CustomSerializedObjectDrawersPerAttributeTypeSnapshot> current;
					return current;
				}

			public:
				CustomSerializedObjectDrawersPerAttributeType snapshot;

				inline static Reference<const CustomSerializedObjectDrawersPerAttributeTypeSnapshot> GetCurrent() {
					std::unique_lock<SpinLock> lock(CustomSerializedObjectDrawerLock());
					Reference<CustomSerializedObjectDrawersPerAttributeTypeSnapshot> current = Current();
					if (current == nullptr) {
						current = Object::Instantiate<CustomSerializedObjectDrawersPerAttributeTypeSnapshot>();
						current->snapshot = CustomSerializedObjectDrawers();
						Current() = current;
					}
					return current;
				}

				inline static void InvalidateCurrent() {
					Current() = nullptr;
				}
			};
		}

		bool DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject) {
			
			typedef bool(*DrawSerializedObjectFn)(const Serialization::SerializedObject&, size_t, OS::Logger*);
			static const DrawSerializedObjectFn* DRAW_FUNCTIONS = []() -> const DrawSerializedObjectFn* {
				static DrawSerializedObjectFn drawFunctions[SERIALIZER_TYPE_COUNT];
				for (size_t i = 0; i < SERIALIZER_TYPE_COUNT; i++)
					drawFunctions[i] = DrawUnsupportedTypeError;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::BOOL_VALUE)] = DrawBoolValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::CHAR_VALUE)] = DrawCharValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::SCHAR_VALUE)] = DrawSCharValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::UCHAR_VALUE)] = DrawUCharValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::WCHAR_VALUE)] = DrawWCharValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::SHORT_VALUE)] = DrawShortValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::USHORT_VALUE)] = DrawUShortValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::INT_VALUE)] = DrawIntValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::UINT_VALUE)] = DrawUIntValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_VALUE)] = DrawLongValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_VALUE)] = DrawULongValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_LONG_VALUE)] = DrawLongLongValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE)] = DrawULongLongValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::FLOAT_VALUE)] = DrawFloatValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::DOUBLE_VALUE)] = DrawDoubleValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR2_VALUE)] = DrawVector2Value;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR3_VALUE)] = DrawVector3Value;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR4_VALUE)] = DrawVector4Value;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX2_VALUE)] = DrawMatrix2Value;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX3_VALUE)] = DrawMatrix3Value;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX4_VALUE)] = DrawMatrix4Value;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::STRING_VIEW_VALUE)] = DrawStringViewValue;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::WSTRING_VIEW_VALUE)] = DrawWStringViewValue;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::OBJECT_PTR_VALUE)] = DrawUnsupportedTypeError;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_LIST)] = DrawUnsupportedTypeError;

				return drawFunctions;
			}();
			
			auto result = [](bool rv) { 
				if (rv) ImGuiRenderer::FieldModified();
				return rv;
			};

			const Serialization::ItemSerializer* serializer = object.Serializer();
			if (serializer == nullptr) {
				if (logger != nullptr) logger->Warning("DrawSerializedObject - got nullptr Serializer!");
				return false;
			}
			else {
				const Serialization::ItemSerializer::Type type = serializer->GetType();
				if (type >= Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT) {
					if(logger != nullptr) logger->Error("DrawSerializedObject - invalid Serializer type! (", static_cast<size_t>(type), ")");
					return false;
				}
				else {
					if (serializer->FindAttributeOfType<Serialization::HideInEditorAttribute>() != nullptr) return false;
					Reference<const CustomSerializedObjectDrawersPerAttributeTypeSnapshot> customDrawers = CustomSerializedObjectDrawersPerAttributeTypeSnapshot::GetCurrent();
					for (size_t i = 0; i < serializer->AttributeCount(); i++) {
						const Object* attribute = serializer->Attribute(i);
						if (attribute == nullptr) continue;
						CustomSerializedObjectDrawersPerAttributeType::const_iterator it = customDrawers->snapshot.find(typeid(*attribute));
						if (it == customDrawers->snapshot.end()) continue;
						const CustomSerializedObjectDrawersPerSerializerType& typeDrawers = it->second;
						const CustomSerializedObjectDrawersSet& drawFunctions = typeDrawers.drawFunctions[static_cast<size_t>(type)];
						if (drawFunctions.empty()) continue;
						return result((*drawFunctions.begin())->DrawObject(object, viewId, logger, drawObjectPtrSerializedObject, attribute));
					}
				}
				if (type == Serialization::ItemSerializer::Type::OBJECT_PTR_VALUE)
					return result(drawObjectPtrSerializedObject(object));
				else if (type == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
					bool rv = false;
					object.GetFields([&](const Serialization::SerializedObject& field) {
						auto drawContent = [&]() { rv |= DrawSerializedObject(field, viewId, logger, drawObjectPtrSerializedObject); };
						if (field.Serializer() != nullptr && field.Serializer()->GetType() == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
							const std::string text = CustomSerializedObjectDrawer::DefaultGuiItemName(field, viewId);
							bool nodeOpen = ImGui::TreeNode(text.c_str());
							DrawTooltip(text, field.Serializer()->TargetHint());
							if (nodeOpen) {
								drawContent();
								ImGui::TreePop();
							}
						}
						else drawContent();
						});
					return result(rv);
				}
				else return result(DRAW_FUNCTIONS[static_cast<size_t>(type)](object, viewId, logger));
			}
		}

		void CustomSerializedObjectDrawer::Register(Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType)const {
			if (this == nullptr) return;
			std::unique_lock<SpinLock> lock(CustomSerializedObjectDrawerLock());
			CustomSerializedObjectDrawersPerAttributeType& registry = CustomSerializedObjectDrawers();
			CustomSerializedObjectDrawersPerSerializerType& typeDrawers = registry[serializerAttributeType.TypeIndex()];
			for (size_t i = 0; i < SERIALIZER_TYPE_COUNT; i++) {
				if (serializerTypes & static_cast<Serialization::ItemSerializer::Type>(i))
					typeDrawers.drawFunctions[i].insert(this);
			}
			CustomSerializedObjectDrawersPerAttributeTypeSnapshot::InvalidateCurrent();
		}
		void CustomSerializedObjectDrawer::Unregister(Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType)const {
			if (this == nullptr) return;
			std::unique_lock<SpinLock> lock(CustomSerializedObjectDrawerLock());
			CustomSerializedObjectDrawersPerAttributeType& registry = CustomSerializedObjectDrawers();
			CustomSerializedObjectDrawersPerAttributeType::iterator it = registry.find(serializerAttributeType.TypeIndex());
			if (it == registry.end()) return;
			bool empty = [&]() -> bool {
				bool rv = true;
				CustomSerializedObjectDrawersPerSerializerType& typeDrawers = it->second;
				for (size_t i = 0; i < SERIALIZER_TYPE_COUNT; i++) {
					CustomSerializedObjectDrawersSet& drawFunctions = typeDrawers.drawFunctions[i];
					if (serializerTypes & static_cast<Serialization::ItemSerializer::Type>(i))
						drawFunctions.erase(this);
					rv &= drawFunctions.empty();
				}
				return false;
			}();
			if (empty)
				registry.erase(it);
			CustomSerializedObjectDrawersPerAttributeTypeSnapshot::InvalidateCurrent();
		}
	}
}
