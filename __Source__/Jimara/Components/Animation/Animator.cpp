#include "Animator.h"
#include "../../Data/Serialization/Attributes/EulerAnglesAttribute.h"


namespace Jimara {
	Animator::Animator(Component* parent, const std::string_view& name) : Component(parent, name) {
		OnDestroyed() += Callback(&Animator::OnComponentDead, this);
		OnParentChanged() += Callback(&Animator::OnTransformHeirarchyChanged, this);
	}

	Animator::~Animator() {
		OnParentChanged() -= Callback(&Animator::OnTransformHeirarchyChanged, this);
		OnDestroyed() -= Callback(&Animator::OnComponentDead, this);
		OnComponentDead(this);
	}


	void Animator::SetClipState(const AnimationClip* clip, ClipPlaybackState state) {
		if (clip == nullptr || state == ClipState(clip)) return;
		ClipStates::iterator it = m_clipStates.find(clip);
		if (it != m_clipStates.end()) {
			if (state.weight > 0.0f) {
				it->second = state;
				return; // No real need to unbind anything here...
			}
			else m_clipStates.erase(it);
		}
		else if (state.weight > 0.0f)
			m_clipStates.insert(std::make_pair(Reference<const AnimationClip>(clip), state));
		Unbind();
	}

	void Animator::Play(const AnimationClip* clip, bool loop, float speed, float weight, float timeOffset) {
		SetClipState(clip, ClipPlaybackState(timeOffset, weight, speed, loop));
	}

	Animator::ClipPlaybackState Animator::ClipState(const AnimationClip* clip)const {
		auto noState = []() { return ClipPlaybackState(0.0f, 0.0f, 0.0f, false); };
		if (clip == nullptr) return noState();
		ClipStates::const_iterator it = m_clipStates.find(clip);
		if (it != m_clipStates.end()) return it->second;
		else return noState();
	}

	bool Animator::ClipPlaying(const AnimationClip* clip)const { return (m_clipStates.find(clip) != m_clipStates.end()); }

	bool Animator::Playing()const { return (!m_clipStates.empty()); }

	void Animator::Update() {
		if (m_dead) return;
		Bind();
		Apply();
		AdvanceTime();
	}



	void Animator::Apply() {
		for (ObjectBindings::const_iterator objectIt = m_objectBindings.begin(); objectIt != m_objectBindings.end(); ++objectIt) {
			const FieldBindings& fieldBindings = objectIt->second;
			for (FieldBindings::const_iterator fieldIt = fieldBindings.begin(); fieldIt != fieldBindings.end(); ++fieldIt) {
				const FieldBinding& fieldBinding = fieldIt->second;
				fieldBinding.update(fieldIt->first, fieldBinding);
			}
		}
	}

	void Animator::AdvanceTime() {
		float deltaTime = Context()->Time()->ScaledDeltaTime();
		for (ClipStates::iterator it = m_clipStates.begin(); it != m_clipStates.end(); ++it) {
			float clipDeltaTime = deltaTime * it->second.speed;
			float newTime = it->second.time + clipDeltaTime;
			const float clipDuration = it->first->Duration();
			const bool endReached = (newTime > clipDuration);
			if (endReached && (!it->second.loop))
				m_completeClipBuffer.push_back(it->first);
			else if (newTime < 0.0f || endReached) {
				if (clipDuration > 0.0f)
					newTime = Math::FloatRemainder(newTime, clipDuration);
				else newTime = 0.0f;
			}
			it->second.time = newTime;
		}
		if (m_completeClipBuffer.size() > 0) {
			Unbind();
			for (size_t i = 0; i < m_completeClipBuffer.size(); i++)
				m_clipStates.erase(m_completeClipBuffer[i]);
			m_completeClipBuffer.clear();
		}
	}

	void Animator::Unbind() {
		if (!m_bound) return;
		for (ClipStates::const_iterator it = m_clipStates.begin(); it != m_clipStates.end(); ++it)
			it->first->OnDirty() -= Callback(&Animator::OnAnimationClipDirty, this);
		for (ObjectBindings::const_iterator it = m_objectBindings.begin(); it != m_objectBindings.end(); ++it) {
			it->first->OnParentChanged() -= Callback(&Animator::OnTransformHeirarchyChanged, this);
			it->first->OnDestroyed() -= Callback(&Animator::OnComponentDead, this);
		}
		m_objectBindings.clear();
		m_bound = false;
	}

	struct Animator::BindingHelper {
		typedef FieldBinding::UpdateFn(*GetUpdaterFn)(const Serialization::SerializedObject&);
		typedef bool(*CheckTrackFn)(const AnimationTrack*);

		template<typename ValueType>
		struct InterpolationState {
			ValueType value = {};
			float totalWeight = 0.0f;

			inline static void SetValue(const Animator::SerializedField& field, const ValueType& value) {
				dynamic_cast<const Serialization::ValueSerializer<ValueType>*>(field.serializer.operator->())->Set(value, field.targetAddr);
			}

			template<typename Type>
			inline bool AddValue(const TrackBinding& trackBinding) {
				const ParametricCurve<Type, float>* const curve = dynamic_cast<const ParametricCurve<Type, float>*>(trackBinding.track);
				if (curve == nullptr) return false;
				value += ValueType(curve->Value(trackBinding.state->time) * trackBinding.state->weight);
				totalWeight += trackBinding.state->weight;
				return true;
			}

			template<typename Type0, typename Type1, typename... Rest>
			inline bool AddValue(const TrackBinding& trackBinding) {
				if (AddValue<Type0>(trackBinding)) return true;
				else return AddValue<Type1, Rest...>(trackBinding);
			}

			template<typename... CastableTypes>
			inline static void Interpolate(const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				InterpolationState state;
				for (size_t i = 0; i < binding.bindings.Size(); i++)
					state.AddValue<ValueType, CastableTypes...>(binding.bindings[i]);
				if (state.totalWeight != 0.0f)
					state.value = ValueType(state.value / state.totalWeight);
				SetValue(field, state.value);
			}

			template<typename Type>
			inline bool AddValueEuler(const TrackBinding& trackBinding) {
				const ParametricCurve<Type, float>* const curve = dynamic_cast<const ParametricCurve<Type, float>*>(trackBinding.track);
				if (curve == nullptr) return false;
				totalWeight += trackBinding.state->weight;
				value = Math::LerpAngles(value, ValueType(curve->Value(trackBinding.state->time)), trackBinding.state->weight / totalWeight);
				return true;
			}

			template<typename Type0, typename Type1, typename... Rest>
			inline bool AddValueEuler(const TrackBinding& trackBinding) {
				if (AddValueEuler<Type0>(trackBinding)) return true;
				else return AddValueEuler<Type1, Rest...>(trackBinding);
			}

			template<typename... CastableTypes>
			inline static void InterpolateEuler(const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				InterpolationState state;
				for (size_t i = 0; i < binding.bindings.Size(); i++)
					state.AddValueEuler<ValueType, CastableTypes...>(binding.bindings[i]);
				SetValue(field, state.value);
			}

			template<typename Type>
			inline static bool SetValue(const Animator::SerializedField& field, const TrackBinding& trackBinding) {
				const ParametricCurve<Type, float>* const curve = dynamic_cast<const ParametricCurve<Type, float>*>(trackBinding.track);
				if (curve == nullptr) return false;
				SetValue(field, curve->Value(trackBinding.state->time));
				return true;
			}

			template<typename Type0, typename Type1, typename... Rest>
			inline static bool SetValue(const Animator::SerializedField& field, const TrackBinding& trackBinding) {
				if (SetValue<Type0>(field, trackBinding)) return true;
				else return SetValue<Type1, Rest...>(field, trackBinding);
			}

			template<typename... CastableTypes>
			inline static void SetFirst(const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				for (size_t i = 0; i < binding.bindings.Size(); i++)
					if (SetValue<ValueType, CastableTypes...>(field, binding.bindings[i])) return;
			}
		};

		struct InterpolationFunctions {
			template<typename ValueType, typename... CastableTypes>
			inline static std::enable_if_t<
				std::is_same_v<ValueType, bool> ||
				std::is_same_v<ValueType, char> ||
				std::is_same_v<ValueType, signed char> ||
				std::is_same_v<ValueType, unsigned char> ||
				std::is_same_v<ValueType, wchar_t> ||
				std::is_same_v<ValueType, std::string_view> ||
				std::is_same_v<ValueType, std::wstring_view>
				, FieldBinding::UpdateFn> GetUpdateFn(const Serialization::SerializedObject&) {
				return InterpolationState<ValueType>::template SetFirst<CastableTypes...>;
			}

			template<typename ValueType, typename... CastableTypes>
			inline static std::enable_if_t<
				std::is_same_v<ValueType, short> ||
				std::is_same_v<ValueType, unsigned short> ||
				std::is_same_v<ValueType, int> ||
				std::is_same_v<ValueType, unsigned int> ||
				std::is_same_v<ValueType, long> ||
				std::is_same_v<ValueType, unsigned long> ||
				std::is_same_v<ValueType, long long> ||
				std::is_same_v<ValueType, unsigned long long> ||
				std::is_same_v<ValueType, double> ||
				std::is_same_v<ValueType, Vector2> ||
				std::is_same_v<ValueType, Vector4> ||
				std::is_same_v<ValueType, Matrix2> ||
				std::is_same_v<ValueType, Matrix3> ||
				std::is_same_v<ValueType, Matrix4>
				, FieldBinding::UpdateFn> GetUpdateFn(const Serialization::SerializedObject&) {
				return InterpolationState<ValueType>::template Interpolate<CastableTypes...>;
			}

			template<typename ValueType, typename... CastableTypes>
			inline static std::enable_if_t<
				std::is_same_v<ValueType, float> ||
				std::is_same_v<ValueType, Vector3>
				, FieldBinding::UpdateFn> GetUpdateFn(const Serialization::SerializedObject& object) {
				if (object.Serializer()->FindAttributeOfType<Serialization::EulerAnglesAttribute>() != nullptr)
					return InterpolationState<ValueType>::template InterpolateEuler<CastableTypes...>;
				else return InterpolationState<ValueType>::template Interpolate<CastableTypes...>;
			}
		};

		template<typename ValueType>
		inline static bool IsCurveOfType(const AnimationTrack* track) {
			return dynamic_cast<const ParametricCurve<ValueType, float>*>(track) != nullptr;
		}

		template<typename ValueType0, typename ValueType1, typename... ValueTypeRest>
		inline static bool IsCurveOfType(const AnimationTrack* track) {
			return IsCurveOfType<ValueType0>(track) || IsCurveOfType<ValueType1, ValueTypeRest...>(track);
		}

		template<typename ValueType, typename... CastableTypes>
		inline static constexpr std::pair<GetUpdaterFn, CheckTrackFn> CheckAndApply() {
			return std::make_pair(InterpolationFunctions::GetUpdateFn<ValueType, CastableTypes...>, IsCurveOfType<ValueType, CastableTypes...>);
		}

		inline static FieldBinding::UpdateFn GetUpdateFn(const Serialization::SerializedObject& serializedField, const AnimationClip::Track* track) {
			static const std::pair<GetUpdaterFn, CheckTrackFn>* APPLY_FUNCTIONS = []() -> const std::pair<GetUpdaterFn, CheckTrackFn>*{
				static const constexpr size_t FUNCTION_COUNT = static_cast<size_t>(Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static std::pair<GetUpdaterFn, CheckTrackFn> functions[FUNCTION_COUNT];
				for (size_t i = 0; i < FUNCTION_COUNT; i++)
					functions[i] = std::make_pair(nullptr, (CheckTrackFn)[](const AnimationTrack*) { return false; });
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::BOOL_VALUE)] = CheckAndApply<bool>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::CHAR_VALUE)] = CheckAndApply<char>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::SCHAR_VALUE)] = CheckAndApply<signed char>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::UCHAR_VALUE)] = CheckAndApply<unsigned char>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::WCHAR_VALUE)] = CheckAndApply<wchar_t>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::SHORT_VALUE)] = CheckAndApply<short>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::USHORT_VALUE)] = CheckAndApply<unsigned short>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::INT_VALUE)] = CheckAndApply<int>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::UINT_VALUE)] = CheckAndApply<unsigned int>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_VALUE)] = CheckAndApply<long>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_VALUE)] = CheckAndApply<unsigned long>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::LONG_LONG_VALUE)] = CheckAndApply<long long>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::ULONG_LONG_VALUE)] = CheckAndApply<unsigned long long>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::FLOAT_VALUE)] = CheckAndApply<float>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::DOUBLE_VALUE)] = CheckAndApply<double>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR2_VALUE)] = CheckAndApply<Vector2>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR3_VALUE)] = CheckAndApply<Vector3>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::VECTOR4_VALUE)] = CheckAndApply<Vector4>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX2_VALUE)] = CheckAndApply<Matrix2>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX3_VALUE)] = CheckAndApply<Matrix3>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::MATRIX4_VALUE)] = CheckAndApply<Matrix4>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::STRING_VIEW_VALUE)] = CheckAndApply<std::string_view>();
				functions[static_cast<size_t>(Serialization::ItemSerializer::Type::WSTRING_VIEW_VALUE)] = CheckAndApply<std::wstring_view>();
				return functions;
			}();

			if (track == nullptr) return nullptr;
			else if (track->TargetField() != serializedField.Serializer()->TargetName()) return nullptr;

			const Serialization::ItemSerializer* serializer = serializedField.Serializer();
			if (serializer == nullptr) return nullptr;

			const Serialization::ItemSerializer::Type type = serializer->GetType();
			if (type >= Serialization::ItemSerializer::Type::SERIALIZER_TYPE_COUNT) return nullptr;

			const std::pair<GetUpdaterFn, CheckTrackFn>& applyFn = APPLY_FUNCTIONS[static_cast<size_t>(type)];
			if (applyFn.second(track))
				return applyFn.first(serializedField);
			else return nullptr;
		}
	};

	void Animator::Bind() {
		if (m_dead || m_bound) return;
		const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
		for (ClipStates::const_iterator clipIt = m_clipStates.begin(); clipIt != m_clipStates.end(); ++clipIt) {
			const AnimationClip* clip = clipIt->first;
			const ClipPlaybackState& clipState = clipIt->second;
			clip->OnDirty() += Callback(&Animator::OnAnimationClipDirty, this);
			for (size_t trackId = 0; trackId < clip->TrackCount(); trackId++) {
				const AnimationClip::Track* track = clip->GetTrack(trackId);
				if (track == nullptr) continue;
				
				// Find target component:
				Component* animatedComponent = dynamic_cast<Component*>(track->FindTarget(Parent()));
				if (animatedComponent == nullptr) continue;
				
				// Find target serialized field:
				SerializedField serializedObject;
				FieldBinding::UpdateFn updateFn;
				{
					const ComponentSerializer* serializer = serializers->FindSerializerOf(animatedComponent);
					if (serializer == nullptr) continue;
					Serialization::SerializedObject serialized = serializer->Serialize(animatedComponent);
					serialized.GetFields([&](Serialization::SerializedObject serializedField) {
						if (serializedField.Serializer() == nullptr || serializedObject.serializer != nullptr) return;
						updateFn = BindingHelper::GetUpdateFn(serializedField, track);
						if (updateFn == nullptr) return;
						serializedObject.serializer = serializedField.Serializer();
						serializedObject.targetAddr = serializedField.TargetAddr();
						});
					if (serializedObject.serializer == nullptr) continue;
				}

				// Add necessary callbacks for safety:
				FieldBindings& fieldBindings = m_objectBindings[animatedComponent];
				if (fieldBindings.empty()) {
					animatedComponent->OnParentChanged() += Callback(&Animator::OnTransformHeirarchyChanged, this);
					animatedComponent->OnDestroyed() += Callback(&Animator::OnComponentDead, this);
				}

				// Add bindings:
				FieldBinding& binding = fieldBindings[serializedObject];
				binding.update = updateFn;
				binding.bindings.Push({ track, &clipState });
			}
		}
		m_bound = true;
	}

	void Animator::OnAnimationClipDirty(const AnimationClip*) { Unbind(); }

	void Animator::OnTransformHeirarchyChanged(const Component*) { Unbind(); }
	
	void Animator::OnComponentDead(Component* component) {
		if (component == this)
			m_dead = true;
		Unbind();
	}

	namespace {
		class AnimatorSerializer : public virtual ComponentSerializer::Of<Animator> {
		public:
			inline AnimatorSerializer() : ItemSerializer("Jimara/Animator", "Animator") {}

			inline static const AnimatorSerializer* Instance() {
				static const AnimatorSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Animator* target)const final override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report) {
		report(AnimatorSerializer::Instance());
	}
}
