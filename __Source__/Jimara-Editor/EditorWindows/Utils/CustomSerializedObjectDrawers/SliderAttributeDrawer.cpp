#include "SliderAttributeDrawer.h"
#include "../DrawTooltip.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static const SliderAttributeDrawer* MainSliderAttributeDrawer() {
				static const Reference<const SliderAttributeDrawer> drawer = Object::Instantiate<SliderAttributeDrawer>();
				return drawer;
			}
			inline static const constexpr Serialization::SerializerTypeMask SliderAttributeDrawerTypeMask() {
				return Serialization::SerializerTypeMask(
					Serialization::ItemSerializer::Type::SHORT_VALUE,
					Serialization::ItemSerializer::Type::USHORT_VALUE,
					Serialization::ItemSerializer::Type::INT_VALUE,
					Serialization::ItemSerializer::Type::UINT_VALUE,
					Serialization::ItemSerializer::Type::LONG_VALUE,
					Serialization::ItemSerializer::Type::ULONG_VALUE,
					Serialization::ItemSerializer::Type::LONG_LONG_VALUE,
					Serialization::ItemSerializer::Type::ULONG_LONG_VALUE,
					Serialization::ItemSerializer::Type::FLOAT_VALUE,
					Serialization::ItemSerializer::Type::DOUBLE_VALUE);
			}

			inline static bool DrawUnsupportedType(const Serialization::SerializedObject& object, const std::string&, OS::Logger* logger, const Object*) {
				if (logger != nullptr)
					logger->Error("SliderAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
						object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType()), ") <internal error>");
				return false;
			}

			template<typename Type, typename ImGuiFN>
			inline static bool DrawSerializerOfType(
				const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute, const ImGuiFN& imGuiFn) {
				const Serialization::SliderAttribute<Type>* attribute = dynamic_cast<const Serialization::SliderAttribute<Type>*>(sliderAttribute);
				if (attribute == nullptr) {
					if (logger != nullptr)
						logger->Error("SliderAttributeDrawer::DrawObject - Incorrect attribute type! (TargetName: ",
							object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType())
							, "; Expected attribute type: \"", TypeId::Of<Serialization::SliderAttribute<Type>>().Name(), "\")");
					return false;
				}
				const Type initialValue = object;
				const Type minValue = attribute->Min();
				const Type maxValue = attribute->Max();
				const Type minStep = attribute->MinStep();
				Type value = min(max(initialValue, minValue), maxValue);
				imGuiFn(fieldName.c_str(), &value, minValue, maxValue);
				value = min(max(initialValue, minValue), maxValue);
				if (minStep > 0 && value < maxValue)
					value = minValue + (static_cast<Type>(static_cast<uint64_t>((value - minValue) / minStep)) * minStep);
				if (value != initialValue)
					object = value;
				return true;
			}

			inline static bool DrawShortType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(short) == sizeof(ImS16));
				return DrawSerializerOfType<short>(object, fieldName, logger, sliderAttribute, 
					[&](const char* name, short* value, short minV, short maxV) {
					ImGui::SliderScalar(name, ImGuiDataType_S16, value, &minV, &maxV);
					});
			}
			inline static bool DrawUShortType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned short) == sizeof(ImU16));
				return DrawSerializerOfType<unsigned short>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, unsigned short* value, unsigned short minV, unsigned short maxV) {
						ImGui::SliderScalar(name, ImGuiDataType_U16, value, &minV, &maxV);
					});
			}
			inline static bool DrawIntType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<int>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, int* value, int minV, int maxV) {
						ImGui::SliderInt(name, value, minV, maxV);
					});
			}
			inline static bool DrawUIntType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned int) == sizeof(ImU32));
				return DrawSerializerOfType<unsigned int>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, unsigned int* value, unsigned int minV, unsigned int maxV) {
						ImGui::SliderScalar(name, ImGuiDataType_U32, value, &minV, &maxV);
					});
			}
			inline static bool DrawLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, long* value, long minV, long maxV) {
							ImGui::SliderInt(name, reinterpret_cast<int*>(value), (int)minV, (int)maxV);
						});
				}
				else {
					static_assert(sizeof(long long) == sizeof(ImS64));
					return DrawSerializerOfType<long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, long* value, long minV, long maxV) {
							ImGui::SliderScalar(name, ImGuiDataType_S64, value, &minV, &maxV);
						});
				}
			}
			inline static bool DrawULongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				static_assert(sizeof(int) == 4);
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<unsigned long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, unsigned long* value, unsigned long minV, unsigned long maxV) {
							ImGui::SliderScalar(name, ImGuiDataType_U32, value, &minV, &maxV);
						});
				}
				else {
					static_assert(sizeof(unsigned long long) == sizeof(ImU64));
					return DrawSerializerOfType<unsigned long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, unsigned long* value, unsigned long minV, unsigned long maxV) {
							ImGui::SliderScalar(name, ImGuiDataType_U64, value, &minV, &maxV);
						});
				}
			}
			inline static bool DrawLongLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(long long) == sizeof(ImS64));
				static_assert(sizeof(long long) == sizeof(ImU64));
				return DrawSerializerOfType<long long>(object, fieldName, logger, sliderAttribute,
					[](const char* name, long long* value, long long minV, long long maxV) {
					ImGui::SliderScalar(name, ImGuiDataType_S64, value, &minV, &maxV);
					});
			}
			inline static bool DrawULongLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned long long) == sizeof(ImU64));
				return DrawSerializerOfType<unsigned long long>(object, fieldName, logger, sliderAttribute,
					[](const char* name, unsigned long long* value, unsigned long long minV, unsigned long long maxV) {
					ImGui::SliderScalar(name, ImGuiDataType_U64, value, &minV, &maxV);
					});
			}
			inline static bool DrawFloatType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<float>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, float* value, float minV, float maxV) {
						ImGui::SliderFloat(name, value, minV, maxV);
					});
			}
			inline static bool DrawDoubleType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<double>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, double* value, double minV, double maxV) {
						float val = float(*value);
						ImGui::SliderFloat(name, &val, float(minV), float(maxV));
						(*value) = double(val);
					});
			}
		}

		void SliderAttributeDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Callback<const Serialization::SerializedObject&>&, const Object* sliderAttribute)const {
			if (object.Serializer() == nullptr) {
				if (logger != nullptr) logger->Error("SliderAttributeDrawer::DrawObject - Got nullptr serializer!");
				return;
			}
			Serialization::ItemSerializer::Type type = object.Serializer()->GetType();
			if (!(SliderAttributeDrawerTypeMask() & type)) {
				if (logger != nullptr) logger->Error("SliderAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
					object.Serializer()->TargetName(), "; type:", static_cast<size_t>(type), ")");
				return;
			}
			const std::string fieldName = DefaultGuiItemName(object, viewId);
			typedef bool(*DrawFn)(const Serialization::SerializedObject&, const std::string&, OS::Logger*, const Object*);
			static const DrawFn* DRAW_FUNCTIONS = []() -> const DrawFn* {
				static const constexpr size_t SERIALIZER_TYPE_COUNT = static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static DrawFn drawFunctions[SERIALIZER_TYPE_COUNT];
				for (size_t i = 0; i < SERIALIZER_TYPE_COUNT; i++)
					drawFunctions[i] = DrawUnsupportedType;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::SHORT_VALUE)] = DrawShortType;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::USHORT_VALUE)] = DrawUShortType;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::INT_VALUE)] = DrawIntType;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::UINT_VALUE)] = DrawUIntType;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_VALUE)] = DrawLongType;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_VALUE)] = DrawULongType;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_LONG_VALUE)] = DrawLongLongType;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE)] = DrawULongLongType;

				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::FLOAT_VALUE)] = DrawFloatType;
				drawFunctions[static_cast<size_t>(Serialization::ItemSerializer::Type::DOUBLE_VALUE)] = DrawDoubleType;

				return drawFunctions;
			}();
			if (DRAW_FUNCTIONS[static_cast<size_t>(type)](object, fieldName, logger, sliderAttribute))
				DrawTooltip(fieldName, object.Serializer()->TargetHint());
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SliderAttributeDrawer>() {
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::SHORT_VALUE, TypeId::Of<Serialization::SliderAttribute<short>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::USHORT_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned short>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::INT_VALUE, TypeId::Of<Serialization::SliderAttribute<int>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::UINT_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned int>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<long>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::ULONG_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned long>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::LONG_LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<long long>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned long long>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::FLOAT_VALUE, TypeId::Of<Serialization::SliderAttribute<float>>());
		Editor::MainSliderAttributeDrawer()->Register(Serialization::ItemSerializer::Type::DOUBLE_VALUE, TypeId::Of<Serialization::SliderAttribute<double>>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SliderAttributeDrawer>() {
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::SHORT_VALUE, TypeId::Of<Serialization::SliderAttribute<short>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::USHORT_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned short>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::INT_VALUE, TypeId::Of<Serialization::SliderAttribute<int>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::UINT_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned int>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<long>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::ULONG_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned long>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::LONG_LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<long long>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE, TypeId::Of<Serialization::SliderAttribute<unsigned long long>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::FLOAT_VALUE, TypeId::Of<Serialization::SliderAttribute<float>>());
		Editor::MainSliderAttributeDrawer()->Unregister(Serialization::ItemSerializer::Type::DOUBLE_VALUE, TypeId::Of<Serialization::SliderAttribute<double>>());
	}
}
