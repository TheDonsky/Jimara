#pragma once
#include <Jimara/Components/Transform.h>
#include <Jimara/Components/UI/UIText.h>
#include <Jimara/Components/UI/UITransform.h>
#include <Jimara/Core/Stopwatch.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace SampleGame {
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::FPSMeter);

		class FPSMeter : public virtual Component {
		public:
			inline FPSMeter(Component* parent, const std::string_view& name = "FPSMeter") : Component(parent, name) {}

			inline virtual ~FPSMeter() {}

			inline void Start() {
				Object::Instantiate<Process>(this);
			}

			/// <summary>
			/// Override for Jimara::Serialization::Serializable to expose controller settings
			/// </summary>
			/// <param name="reportItem"> Callback, with which the serialization utilities access fields </param>
			inline virtual void GetFields(Callback<Serialization::SerializedObject> reportItem)override {
				Component::GetFields(reportItem);
				JIMARA_SERIALIZE_FIELDS(this, reportItem) {
					JIMARA_SERIALIZE_FIELD(m_target, "Target", "Target UI text field to display the value on.");
					JIMARA_SERIALIZE_FIELD(m_duration, "Duration", "Measuring duration.");
					JIMARA_SERIALIZE_FIELD(m_measureTimeText, "MeasuringText", "Text to display during measurement.");
				};
			}

			/// <summary>
			/// Reports actions associated with the component.
			/// </summary>
			/// <param name="report"> Actions will be reported through this callback </param>
			inline virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override {
				Component::GetSerializedActions(report);
				report(Serialization::SerializedCallback::Create<>::From("Start", Callback<>(&FPSMeter::Start, this)));
			}

		private:
			WeakReference<UI::UIText> m_target;
			float m_duration = 10.0;
			std::string m_measureTimeText = "...";

			struct Process : public virtual Object {
				Reference<UI::UIText> target;
				Stopwatch stopwatch;
				size_t frames = 0u;
				float time = 0.0;

				inline Process(FPSMeter* meter) {
					if (meter == nullptr)
						return;
					target = meter->m_target;
					if (target == nullptr)
						target = meter->GetComponentInParents<UI::UIText>();
					if (target == nullptr)
						return;
					target->Text() = meter->m_measureTimeText;
					time = meter->m_duration;
					stopwatch.Reset();
					target->Context()->StoreDataObject(this);
					target->Context()->OnSynchOrUpdate() += Callback<>(&Process::Update, this);
				}

				inline virtual ~Process() {
					target->Context()->Log()->Info(frames, " frames in ", time, " seconds.");
				}

				void Update() {
					float elapsed = stopwatch.Elapsed();
					frames++;
					if (elapsed < time)
						return;
					time = elapsed;
					target->Text() = std::to_string(static_cast<uint64_t>(frames / elapsed));
					target->Context()->OnSynchOrUpdate() -= Callback<>(&Process::Update, this);
					target->Context()->EraseDataObject(this);
				}
			};
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleGame::FPSMeter>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SampleGame::FPSMeter>(
			"FPSMeter", "SampleGame/UI/FPSMeter", "Framerate measuring tool");
		report(factory);
	}
}
