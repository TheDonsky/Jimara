#include "SliderAttributeDrawer.h"
#include "../DrawTooltip.h"
#include <optional>


namespace Jimara {
	namespace Editor {
		namespace {
			struct DrawerResult {
				bool drawn;
				bool modified;
				inline DrawerResult(bool valid, bool changed) : drawn(valid), modified(changed) {}
			};

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

			inline static DrawerResult DrawUnsupportedType(const Serialization::SerializedObject& object, const std::string&, OS::Logger* logger, const Object*) {
				if (logger != nullptr)
					logger->Error("SliderAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
						object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType()), ") <internal error>");
				return DrawerResult(false, false);
			}

			template<typename Type, typename ImGuiFN>
			inline static DrawerResult DrawSerializerOfType(
				const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute, const ImGuiFN& imGuiFn) {
				const Serialization::SliderAttribute<Type>* attribute = dynamic_cast<const Serialization::SliderAttribute<Type>*>(sliderAttribute);
				if (attribute == nullptr) {
					if (logger != nullptr)
						logger->Error("SliderAttributeDrawer::DrawObject - Incorrect attribute type! (TargetName: ",
							object.Serializer()->TargetName(), "; type:", static_cast<size_t>(object.Serializer()->GetType())
							, "; Expected attribute type: \"", TypeId::Of<Serialization::SliderAttribute<Type>>().Name(), "\")");
					return DrawerResult(false, false);
				}
				const Type currentValue = object;
				const Type minValue = attribute->Min();
				const Type maxValue = attribute->Max();
				const Type minStep = attribute->MinStep();

				static thread_local std::optional<Type> lastValue;
				static thread_local const Serialization::ItemSerializer* lastSerializer = nullptr;
				static thread_local const void* lastTargetAddr = nullptr;
				
				const bool isSameObject = (lastValue.has_value() && object.Serializer() == lastSerializer && object.TargetAddr() == lastTargetAddr);
				Type value = min(max(isSameObject ? lastValue.value() : currentValue, minValue), maxValue);
				bool modified = imGuiFn(fieldName.c_str(), &value, minValue, maxValue);
				bool finished = ImGui::IsItemDeactivatedAfterEdit();
				
				value = min(max(value, minValue), maxValue);
				if (minStep > 0 && value < maxValue)
					value = minValue + (static_cast<Type>(static_cast<uint64_t>((value - minValue) / minStep)) * minStep);
				
				if (finished) {
					if (value != currentValue)
						object = value;
					lastValue = std::optional<Type>();
					lastSerializer = nullptr;
					lastTargetAddr = nullptr;
				}
				else if (modified) {
					lastValue = value;
					lastSerializer = object.Serializer();
					lastTargetAddr = object.TargetAddr();
				}
				
				return DrawerResult(true, finished);
			}

			inline static DrawerResult DrawShortType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(short) == sizeof(ImS16));
				return DrawSerializerOfType<short>(object, fieldName, logger, sliderAttribute, 
					[&](const char* name, short* value, short minV, short maxV) {
					return ImGui::SliderScalar(name, ImGuiDataType_S16, value, &minV, &maxV);
					});
			}
			inline static DrawerResult DrawUShortType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned short) == sizeof(ImU16));
				return DrawSerializerOfType<unsigned short>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, unsigned short* value, unsigned short minV, unsigned short maxV) {
						return ImGui::SliderScalar(name, ImGuiDataType_U16, value, &minV, &maxV);
					});
			}
			inline static DrawerResult DrawIntType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<int>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, int* value, int minV, int maxV) {
						return ImGui::SliderInt(name, value, minV, maxV);
					});
			}
			inline static DrawerResult DrawUIntType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned int) == sizeof(ImU32));
				return DrawSerializerOfType<unsigned int>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, unsigned int* value, unsigned int minV, unsigned int maxV) {
						return ImGui::SliderScalar(name, ImGuiDataType_U32, value, &minV, &maxV);
					});
			}
			inline static DrawerResult DrawLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, long* value, long minV, long maxV) {
							return ImGui::SliderInt(name, reinterpret_cast<int*>(value), (int)minV, (int)maxV);
						});
				}
				else {
					static_assert(sizeof(long long) == sizeof(ImS64));
					return DrawSerializerOfType<long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, long* value, long minV, long maxV) {
							return ImGui::SliderScalar(name, ImGuiDataType_S64, value, &minV, &maxV);
						});
				}
			}
			inline static DrawerResult DrawULongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert((sizeof(long) == sizeof(int)) || (sizeof(long) == sizeof(long long)));
				static_assert(sizeof(int) == 4);
				if (sizeof(long) == sizeof(int)) {
					return DrawSerializerOfType<unsigned long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, unsigned long* value, unsigned long minV, unsigned long maxV) {
							return ImGui::SliderScalar(name, ImGuiDataType_U32, value, &minV, &maxV);
						});
				}
				else {
					static_assert(sizeof(unsigned long long) == sizeof(ImU64));
					return DrawSerializerOfType<unsigned long>(object, fieldName, logger, sliderAttribute,
						[](const char* name, unsigned long* value, unsigned long minV, unsigned long maxV) {
							return ImGui::SliderScalar(name, ImGuiDataType_U64, value, &minV, &maxV);
						});
				}
			}
			inline static DrawerResult DrawLongLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(long long) == sizeof(ImS64));
				static_assert(sizeof(long long) == sizeof(ImU64));
				return DrawSerializerOfType<long long>(object, fieldName, logger, sliderAttribute,
					[](const char* name, long long* value, long long minV, long long maxV) {
					return ImGui::SliderScalar(name, ImGuiDataType_S64, value, &minV, &maxV);
					});
			}
			inline static DrawerResult DrawULongLongType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				static_assert(sizeof(unsigned long long) == sizeof(ImU64));
				return DrawSerializerOfType<unsigned long long>(object, fieldName, logger, sliderAttribute,
					[](const char* name, unsigned long long* value, unsigned long long minV, unsigned long long maxV) {
					return ImGui::SliderScalar(name, ImGuiDataType_U64, value, &minV, &maxV);
					});
			}
			inline static DrawerResult DrawFloatType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<float>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, float* value, float minV, float maxV) {
						return ImGui::SliderFloat(name, value, minV, maxV);
					});
			}
			inline static DrawerResult DrawDoubleType(const Serialization::SerializedObject& object, const std::string& fieldName, OS::Logger* logger, const Object* sliderAttribute) {
				return DrawSerializerOfType<double>(object, fieldName, logger, sliderAttribute,
					[&](const char* name, double* value, double minV, double maxV) {
						float val = float(*value);
						bool rv = ImGui::SliderFloat(name, &val, float(minV), float(maxV));
						(*value) = double(val);
						return rv;
					});
			}
		}

		bool SliderAttributeDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>&, const Object* sliderAttribute)const {
			if (object.Serializer() == nullptr) {
				if (logger != nullptr) logger->Error("SliderAttributeDrawer::DrawObject - Got nullptr serializer!");
				return false;
			}
			Serialization::ItemSerializer::Type type = object.Serializer()->GetType();
			if (!(SliderAttributeDrawerTypeMask() & type)) {
				if (logger != nullptr) logger->Error("SliderAttributeDrawer::DrawObject - Unsupported serializer type! (TargetName: ",
					object.Serializer()->TargetName(), "; type:", static_cast<size_t>(type), ")");
				return false;
			}
			const std::string fieldName = DefaultGuiItemName(object, viewId);
			typedef DrawerResult(*DrawFn)(const Serialization::SerializedObject&, const std::string&, OS::Logger*, const Object*);
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
			DrawerResult rv = DRAW_FUNCTIONS[static_cast<size_t>(type)](object, fieldName, logger, sliderAttribute);
			if (rv.drawn)
				DrawTooltip(fieldName, object.Serializer()->TargetHint());
			else if (rv.modified) ImGuiRenderer::FieldModified();
			return rv.modified;
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
