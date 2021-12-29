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

			inline static void DrawUnsupportedTypeError(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				context->ApplicationContext()->Log()->Error("DrawSerializedObject - unsupported Serializer type! (Name: \""
					, object.Serializer()->TargetName(), "\";type:", static_cast<size_t>(object.Serializer()->GetType()), ")");
			}

			template<typename Type, typename ImGuiFN>
			inline static void DrawSerializerOfType(const Serialization::SerializedObject& object, size_t viewId, EditorContext*, const ImGuiFN& imGuiFn) {
				const Type initialValue = object;
				const std::string name = GUI_ItemName(object, viewId);
				Type value = initialValue;
				imGuiFn(name.c_str(), &value);
				DrawTooltip(name, object.Serializer()->TargetHint());
				if (value != initialValue)
					object = value;
			}

			inline static void DrawBoolValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<bool>(object, viewId, context, ImGui::Checkbox);
			}

			inline static void DrawCharValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}
			inline static void DrawSCharValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}
			inline static void DrawUCharValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}
			inline static void DrawWCharValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}

			inline static void DrawShortValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<short>(object, viewId, context, [](const char* name, short* value) {
					int intVal = *value;
					ImGui::InputInt(name, &intVal);
					(*value) = static_cast<short>(intVal);
					});
			}
			inline static void DrawUShortValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<unsigned short>(object, viewId, context, [](const char* name, unsigned short* value) {
					int intVal = *value;
					ImGui::InputInt(name, &intVal);
					(*value) = static_cast<unsigned short>(intVal);
					});
			}

			inline static void DrawIntValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<int>(object, viewId, context, [](const char* name, int* value) { ImGui::InputInt(name, value); });
			}
			inline static void DrawUIntValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<unsigned int>(object, viewId, context, [](const char* name, unsigned int* value) {
					ImGui::InputInt(name, reinterpret_cast<int*>(value));
					});
			}

			inline static void DrawLongValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					DrawSerializerOfType<long>(object, viewId, context, [](const char* name, long* value) { 
						ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					// __TODO__: Implement this crap! 
					DrawUnsupportedTypeError(object, viewId, context);
				}
			}
			inline static void DrawULongValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					DrawSerializerOfType<unsigned long>(object, viewId, context, [](const char* name, unsigned long* value) {
						ImGui::InputInt(name, reinterpret_cast<int*>(value));
						});
				}
				else {
					// __TODO__: Implement this crap!
					DrawUnsupportedTypeError(object, viewId, context);
				}
			}

			inline static void DrawLongLongValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}
			inline static void DrawULongLongValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				// __TODO__: Implement this crap!
				DrawUnsupportedTypeError(object, viewId, context);
			}

			inline static void DrawFloatValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<float>(object, viewId, context, [](const char* name, float* value) { ImGui::InputFloat(name, value); });
			}
			inline static void DrawDoubleValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<double>(object, viewId, context, [](const char* name, double* value) { ImGui::InputDouble(name, value); });
			}

			inline static void DrawVector2Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<Vector2>(object, viewId, context, [](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					ImGui::InputFloat2(name, fields);
					(*value) = Vector2(fields[0], fields[1]);
					});
			}
			inline static void DrawVector3Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<Vector3>(object, viewId, context, [](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					ImGui::InputFloat3(name, fields);
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					});
			}
			inline static void DrawVector4Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawSerializerOfType<Vector4>(object, viewId, context, [](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					ImGui::InputFloat4(name, fields);
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					});
			}

			template<typename MatrixType, size_t NumSubfields, typename FieldInputCallback>
			inline static void DrawMatrixValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context, const FieldInputCallback& fieldInput) {
				DrawSerializerOfType<MatrixType>(object, viewId, context, [&](const char* name, MatrixType* value) {
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

			inline static void DrawMatrix2Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawMatrixValue<Matrix2, 2>(object, viewId, context, [](const char* name, Vector2* value) {
					float fields[] = { value->x, value->y };
					ImGui::InputFloat2(name, fields);
					(*value) = Vector2(fields[0], fields[1]);
				});
			}
			inline static void DrawMatrix3Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawMatrixValue<Matrix3, 3>(object, viewId, context, [](const char* name, Vector3* value) {
					float fields[] = { value->x, value->y, value->z };
					ImGui::InputFloat3(name, fields);
					(*value) = Vector3(fields[0], fields[1], fields[2]);
					});
			}
			inline static void DrawMatrix4Value(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawMatrixValue<Matrix4, 4>(object, viewId, context, [](const char* name, Vector4* value) {
					float fields[] = { value->x, value->y, value->z, value->w };
					ImGui::InputFloat4(name, fields);
					(*value) = Vector4(fields[0], fields[1], fields[2], fields[3]);
					});
			}

			template<typename SetNewTextCallback>
			inline static void DrawStringViewValue(
				const Serialization::SerializedObject& object, size_t viewId, EditorContext* context,
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
			inline static void DrawStringViewValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				DrawStringViewValue(object, viewId, context, object, [&](const std::string_view& newText) { object = newText; });
			}
			inline static void DrawWStringViewValue(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
				const std::wstring_view wideView = object;
				const std::string wstringToString = Convert<std::string>(wideView);
				DrawStringViewValue(object, viewId, context, wstringToString, [&](const std::string_view& newText) { 
					const std::wstring wideNewText = Convert<std::wstring>(newText);
					object = std::wstring_view(wideNewText);
					});
			}
		}

		void DrawSerializedObject(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context) {
			typedef void(*DrawSerializedObjectFn)(const Serialization::SerializedObject&, size_t, EditorContext*);
			static const DrawSerializedObjectFn* DRAW_FUNCTIONS = []() -> const DrawSerializedObjectFn* {
				static const constexpr size_t DRAW_FUNCTION_COUNT = static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static DrawSerializedObjectFn drawFunctions[DRAW_FUNCTION_COUNT];
				for (size_t i = 0; i < DRAW_FUNCTION_COUNT; i++)
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
			if (serializer == nullptr)
				context->ApplicationContext()->Log()->Warning("DrawSerializedObject - got nullptr Serializer!");
			else {
				// __TODO__: Look through attributes and maybe add some optional effects or alternate ways to draw the darn thing...

				const Serialization::ItemSerializer::Type type = serializer->GetType();
				if (type >= Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT)
					context->ApplicationContext()->Log()->Error("DrawSerializedObject - invalid Serializer type! (", static_cast<size_t>(type), ")");
				else if (type == Serialization::ItemSerializer::Type::OBJECT_PTR_VALUE)
					context->ApplicationContext()->Log()->Warning("DrawSerializedObject - OBJECT_PTR_VALUE not yet supported!");
				else if (type == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
					object.GetFields([&](const Serialization::SerializedObject& field) {
						auto drawContent = [&]() { DrawSerializedObject(field, viewId, context); };
						if (field.Serializer() != nullptr && field.Serializer()->GetType() == Serialization::ItemSerializer::Type::SERIALIZER_LIST) {
							const std::string text = GUI_ItemName(field, viewId);
							if (ImGui::TreeNode(text.c_str())) {
								drawContent();
								ImGui::TreePop();
							}
						}
						else drawContent();
						});
					
				}
				else DRAW_FUNCTIONS[static_cast<size_t>(type)](object, viewId, context);
			}
		}
	}
}
