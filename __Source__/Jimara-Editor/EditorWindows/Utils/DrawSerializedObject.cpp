#include "DrawSerializedObject.h"
#include "DrawTooltip.h"

namespace Jimara {
	namespace Editor {
		namespace {
			inline static std::string GUI_ItemName(const Serialization::SerializedObject& object, size_t viewId) {
				std::stringstream stream;
				stream << object.Serializer()->TargetName()
					<< "###DrawSerializedObject_for_view_" << viewId
					<< "_serializer_" << ((size_t)(object.Serializer()))
					<< "_target_" << ((size_t)(object.TargetAddr()));
				return stream.str();
			}

			inline static void DrawUnsupportedTypeError(const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger) {
				if (logger != nullptr) logger->Error("DrawSerializedObject - unsupported Serializer type! (Name: \""
					, object.Serializer()->TargetName(), "\";type:", static_cast<size_t>(object.Serializer()->GetType()), ")");
			}

			template<typename Type, typename ImGuiFN>
			inline static void DrawSerializerOfType(const Serialization::SerializedObject& object, size_t viewId, const ImGuiFN& imGuiFn) {
				const Type initialValue = object;
				const std::string name = GUI_ItemName(object, viewId);
				Type value = initialValue;
				imGuiFn(name.c_str(), &value);
				DrawTooltip(name, object.Serializer()->TargetHint());
				if (value != initialValue)
					object = value;
			}

			inline static void DrawBoolValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<bool>(object, viewId, ImGui::Checkbox);
			}

			inline static void DrawCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImS8, signed char>);
				DrawSerializerOfType<char>(object, viewId, [](const char* name, char* value) {
					ImGui::InputScalar(name, ImGuiDataType_S8, reinterpret_cast<signed char*>(value));
					});
			}
			inline static void DrawSCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImS8, signed char>);
				DrawSerializerOfType<signed char>(object, viewId, [](const char* name, signed char* value) {
					ImGui::InputScalar(name, ImGuiDataType_S8, value);
					});
			}
			inline static void DrawUCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(std::is_same_v<ImU8, unsigned char>);
				DrawSerializerOfType<unsigned char>(object, viewId, [](const char* name, unsigned char* value) {
					ImGui::InputScalar(name, ImGuiDataType_U8, value);
					});
			}
			inline static void DrawWCharValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(
					((sizeof(ImWchar16) == sizeof(wchar_t)) && std::is_same_v<ImU16, ImWchar16>) ||
					((sizeof(ImWchar32) == sizeof(wchar_t)) && std::is_same_v<ImU32, ImWchar32>));
				if ((sizeof(ImWchar16) == sizeof(wchar_t)) && std::is_same_v<ImU16, ImWchar16>) {
					DrawSerializerOfType<wchar_t>(object, viewId, [](const char* name, wchar_t* value) {
						ImGui::InputScalar(name, ImGuiDataType_U16, reinterpret_cast<ImWchar16*>(value));
						});
				}
				else DrawSerializerOfType<wchar_t>(object, viewId, [](const char* name, wchar_t* value) {
						ImGui::InputScalar(name, ImGuiDataType_U32, reinterpret_cast<ImWchar32*>(value));
						});
			}

			inline static void DrawShortValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(short) == sizeof(ImS16));
				DrawSerializerOfType<short>(object, viewId, [](const char* name, short* value) {
					ImGui::InputScalar(name, ImGuiDataType_S16, value);
					});
			}
			inline static void DrawUShortValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned short) == sizeof(ImU16));
				DrawSerializerOfType<unsigned short>(object, viewId, [](const char* name, unsigned short* value) {
					ImGui::InputScalar(name, ImGuiDataType_U16, value);
					});
			}

			inline static void DrawIntValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<int>(object, viewId, [](const char* name, int* value) { ImGui::InputInt(name, value); });
			}
			inline static void DrawUIntValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned int) == sizeof(ImU32));
				DrawSerializerOfType<unsigned int>(object, viewId, [](const char* name, unsigned int* value) {
					ImGui::InputScalar(name, ImGuiDataType_U32, value);
					});
			}

			inline static void DrawLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					DrawSerializerOfType<long>(object, viewId, [](const char* name, long* value) { 
						ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					static_assert(sizeof(long long) == sizeof(ImS64));
					DrawSerializerOfType<long>(object, viewId, [](const char* name, long* value) {
						ImGui::InputScalar(name, ImGuiDataType_S64, value);
						});
				}
			}
			inline static void DrawULongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				static_assert(sizeof(int) == 4);
				if (sizeof(long) == sizeof(int)) {
					DrawSerializerOfType<unsigned long>(object, viewId, [](const char* name, unsigned long* value) {
						ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					static_assert(sizeof(unsigned long long) == sizeof(ImU64));
					DrawSerializerOfType<unsigned long>(object, viewId, [](const char* name, unsigned long* value) {
						ImGui::InputScalar(name, ImGuiDataType_U64, value);
						});
				}
			}

			inline static void DrawLongLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(long long) == sizeof(ImS64));
				static_assert(sizeof(long long) == sizeof(ImU64));
				DrawSerializerOfType<long long>(object, viewId, [](const char* name, long long* value) {
					ImGui::InputScalar(name, ImGuiDataType_S64, value);
					});
			}
			inline static void DrawULongLongValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				static_assert(sizeof(unsigned long long) == sizeof(ImU64));
				DrawSerializerOfType<unsigned long long>(object, viewId, [](const char* name, unsigned long long* value) {
					ImGui::InputScalar(name, ImGuiDataType_U64, value);
					});
			}

			inline static void DrawFloatValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<float>(object, viewId, [](const char* name, float* value) { ImGui::InputFloat(name, value); });
			}
			inline static void DrawDoubleValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<double>(object, viewId, [](const char* name, double* value) { ImGui::InputDouble(name, value); });
			}

			inline static void DrawVector2Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<Vector2>(object, viewId, [](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					ImGui::InputFloat2(name, fields);
					(*value) = Vector2(fields[0], fields[1]);
					});
			}
			inline static void DrawVector3Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<Vector3>(object, viewId, [](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					ImGui::InputFloat3(name, fields);
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					});
			}
			inline static void DrawVector4Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawSerializerOfType<Vector4>(object, viewId, [](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					ImGui::InputFloat4(name, fields);
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					});
			}

			template<typename MatrixType, size_t NumSubfields, typename FieldInputCallback>
			inline static void DrawMatrixValue(const Serialization::SerializedObject& object, size_t viewId, const FieldInputCallback& fieldInput) {
				DrawSerializerOfType<MatrixType>(object, viewId, [&](const char* name, MatrixType* value) {
					if (ImGui::TreeNode(name)) {
						for (typename MatrixType::length_type i = 0; i < NumSubfields; i++) {
							const std::string fieldName = [&]() {
								std::stringstream stream;
								stream << "###DrawSerializedObject_for_view_" << viewId
									<< "_serializer_" << ((size_t)(object.Serializer()))
									<< "_target_" << ((size_t)(object.TargetAddr()))
									<< "_subfield_" << i;
								return stream.str();
							}();
							fieldInput(fieldName.c_str(), &(*value)[i]);
						}
						ImGui::TreePop();
					}
					});
			}

			inline static void DrawMatrix2Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawMatrixValue<Matrix2, 2>(object, viewId, [](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					ImGui::InputFloat2(name, fields);
					(*value) = Vector2(fields[0], fields[1]);
				});
			}
			inline static void DrawMatrix3Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawMatrixValue<Matrix3, 3>(object, viewId, [](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					ImGui::InputFloat3(name, fields);
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					});
			}
			inline static void DrawMatrix4Value(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawMatrixValue<Matrix4, 4>(object, viewId, [](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					ImGui::InputFloat4(name, fields);
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					});
			}

			template<typename SetNewTextCallback>
			inline static void DrawStringViewValue(
				const Serialization::SerializedObject& object, size_t viewId,
				const std::string_view& currentText, const SetNewTextCallback& setNewText) {
				static thread_local std::vector<char> textBuffer;
				{
					if (textBuffer.size() <= (currentText.length() + 1))
						textBuffer.resize(currentText.length() + 512);
					memcpy(textBuffer.data(), currentText.data(), currentText.length());
					textBuffer[currentText.length()] = '\0';
				}
				const std::string nameId = GUI_ItemName(object, viewId);
				ImGui::InputTextWithHint(nameId.c_str(), object.Serializer()->TargetHint().c_str(), textBuffer.data(), textBuffer.size());
				DrawTooltip(nameId, object.Serializer()->TargetHint());
				if (strlen(textBuffer.data()) != currentText.length() || memcmp(textBuffer.data(), currentText.data(), currentText.length()) != 0)
					setNewText(std::string_view(textBuffer.data()));
			}
			inline static void DrawStringViewValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				DrawStringViewValue(object, viewId, object, [&](const std::string_view& newText) { object = newText; });
			}
			inline static void DrawWStringViewValue(const Serialization::SerializedObject& object, size_t viewId, OS::Logger*) {
				const std::wstring_view wideView = object;
				const std::string wstringToString = Convert<std::string>(wideView);
				DrawStringViewValue(object, viewId, wstringToString, [&](const std::string_view& newText) { 
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

		void DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Callback<const Serialization::SerializedObject&>& drawObjectPtrSerializedObject) {
			
			typedef void(*DrawSerializedObjectFn)(const Serialization::SerializedObject&, size_t, OS::Logger*);
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
			
			const Serialization::ItemSerializer* serializer = object.Serializer();
			if (serializer == nullptr) {
				if (logger != nullptr) logger->Warning("DrawSerializedObject - got nullptr Serializer!");
			}
			else {
				const Serialization::ItemSerializer::Type type = serializer->GetType();
				if (type >= Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT) {
					if(logger != nullptr) logger->Error("DrawSerializedObject - invalid Serializer type! (", static_cast<size_t>(type), ")");
				}
				else {
					Reference<const CustomSerializedObjectDrawersPerAttributeTypeSnapshot> customDrawers = CustomSerializedObjectDrawersPerAttributeTypeSnapshot::GetCurrent();
					for (size_t i = 0; i < serializer->AttributeCount(); i++) {
						const Object* attribute = serializer->Attribute(i);
						if (attribute == nullptr) continue;
						CustomSerializedObjectDrawersPerAttributeType::const_iterator it = customDrawers->snapshot.find(typeid(*attribute));
						if (it == customDrawers->snapshot.end()) continue;
						const CustomSerializedObjectDrawersPerSerializerType& typeDrawers = it->second;
						const CustomSerializedObjectDrawersSet& drawFunctions = typeDrawers.drawFunctions[static_cast<size_t>(type)];
						if (drawFunctions.empty()) continue;
						(*drawFunctions.begin())->DrawObject(object, viewId, logger, drawObjectPtrSerializedObject, attribute);
						return;
					}
				}
				if (type == Serialization::ItemSerializer::Type::OBJECT_PTR_VALUE)
					drawObjectPtrSerializedObject(object);
				else if (type == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
					object.GetFields([&](const Serialization::SerializedObject& field) {
						auto drawContent = [&]() { DrawSerializedObject(field, viewId, logger, drawObjectPtrSerializedObject); };
						if (field.Serializer() != nullptr && field.Serializer()->GetType() == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
							const std::string text = GUI_ItemName(field, viewId);
							if (ImGui::TreeNode(text.c_str())) {
								drawContent();
								ImGui::TreePop();
							}
							DrawTooltip(text, field.Serializer()->TargetHint());
						}
						else drawContent();
						});
					
				}
				else DRAW_FUNCTIONS[static_cast<size_t>(type)](object, viewId, logger);
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
