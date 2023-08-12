#include "Animator.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EulerAnglesAttribute.h"
#include "../../Data/Serialization/Attributes/HideInEditorAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Math/BinarySearch.h"


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





	size_t Animator::ChannelCount()const { 
		return m_channelCount; 
	}

	void Animator::SetChannelCount(size_t count) {
		if (m_channelCount == count)
			return;
		Unbind();
		m_channelStates.resize(Math::Max(count, m_channelStates.size()));
		while (m_channelCount > count) {
			m_channelCount--;
			m_channelStates[m_channelCount] = {};
		}
		m_channelCount = count;
	}

	Animator::AnimationChannel Animator::Channel(size_t index) {
		SetChannelCount(Math::Max(index + 1, m_channelCount));
		return AnimationChannel(this, index);
	}

	size_t Animator::AnimationChannel::Index()const {
		return m_index;
	}

	AnimationClip* Animator::AnimationChannel::Clip()const {
		return m_animator->m_channelStates[m_index].clip;
	}

	void Animator::AnimationChannel::SetClip(AnimationClip* clip) {
		PlaybackState& state = m_animator->m_channelStates[m_index];
		if (state.clip == clip || m_index >= m_animator->m_channelCount)
			return;
		m_animator->Unbind();
		state.clip = clip;
		state.isPlaying &= (state.clip != nullptr);
		SetTime(state.time);
	}

	float Animator::AnimationChannel::Time()const {
		return m_animator->m_channelStates[m_index].time;
	}

	void Animator::AnimationChannel::SetTime(float time) {
		PlaybackState& state = m_animator->m_channelStates[m_index];
		state.time = (state.clip == nullptr) ? 0.0f : Math::Min(Math::Max(0.0f, time), std::abs(state.clip->Duration()));
	}

	float Animator::AnimationChannel::BlendWeight()const {
		return m_animator->m_channelStates[m_index].weight;
	}

	void Animator::AnimationChannel::SetBlendWeight(float weight) {
		m_animator->m_channelStates[m_index].weight = Math::Max(weight, 0.0f);
	}

	float Animator::AnimationChannel::Speed()const {
		return m_animator->m_channelStates[m_index].speed;
	}

	void Animator::AnimationChannel::SetSpeed(float speed) {
		m_animator->m_channelStates[m_index].speed = speed;
	}

	bool Animator::AnimationChannel::Looping()const {
		return m_animator->m_channelStates[m_index].loop;
	}

	void Animator::AnimationChannel::SetLooping(bool loop) {
		m_animator->m_channelStates[m_index].loop = loop;
	}

	bool Animator::AnimationChannel::Playing()const {
		return m_animator->m_channelStates[m_index].isPlaying;
	}

	void Animator::AnimationChannel::Play() {
		PlaybackState& state = m_animator->m_channelStates[m_index];
		state.isPlaying = (state.clip != nullptr);
		if (state.isPlaying)
			m_animator->m_reactivatedChannels.insert(&state);
	}

	void Animator::AnimationChannel::Stop() {
		PlaybackState& state = m_animator->m_channelStates[m_index];
		state.isPlaying = false;
		state.time = (state.clip == nullptr || state.speed >= 0.0f)
			? 0.0f : state.clip->Duration();
	}

	void Animator::AnimationChannel::Pause() {
		m_animator->m_channelStates[m_index].isPlaying = false;
	}





	bool Animator::Playing()const { return (!m_activeChannelStates.empty()) || (!m_reactivatedChannels.empty()); }

	float Animator::PlaybackSpeed()const { return m_playbackSpeed; }

	void Animator::SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

	Transform* Animator::RootMotionSource()const { 
		const Reference<Transform> source = m_rootMotionSource;
		return source;
	}

	void Animator::SetRootMotionSource(Transform* source) {
		if (source == RootMotionSource())
			return;
		m_rootMotionSource = source;
		Unbind();
	}

	Animator::RootMotionFlags Animator::RootMotionSettings()const { return m_rootMotionSettings; }

	void Animator::SetRootMotionSettings(RootMotionFlags flags) {
		m_rootMotionSettings = flags;
	}

	Rigidbody* Animator::RootMotionTarget()const {
		const Reference<Rigidbody> body = m_rootRigidbody;
		return body;
	}

	void Animator::SetRootMotionTarget(Rigidbody* body) {
		m_rootRigidbody = body;
	}


	struct Animator::SerializedPlayState : 
		public virtual Serialization::SerializerList::From<AnimationChannel> {
		inline SerializedPlayState(const std::string_view& name) : Serialization::ItemSerializer(name, "Animator channel state") {}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AnimationChannel* channel)const override {
			JIMARA_SERIALIZE_FIELDS(channel, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(Clip, SetClip, "Clip", "Animation clip (set to nullptr to remove; set to any clip to add if nullptr)");
				if (channel->Clip() != nullptr) {
					JIMARA_SERIALIZE_FIELD_GET_SET(Time, SetTime, "Time", "Animation time point");
					JIMARA_SERIALIZE_FIELD_GET_SET(BlendWeight, SetBlendWeight, "Weight", "Blending weight (less than or equal to zeor will result in removing the clip)");
					JIMARA_SERIALIZE_FIELD_GET_SET(Speed, SetSpeed, "Speed", "Animation playback speed");
					JIMARA_SERIALIZE_FIELD_GET_SET(Looping, SetLooping, "Loop", "If true, animation will be looping");
					bool playing = channel->Playing();
					JIMARA_SERIALIZE_FIELD(playing, "Play", "If true, animation will play");
					if (playing)
						channel->Play();
					else channel->Pause();
				}
			};
		}

		typedef std::vector<Reference<SerializedPlayState>> List;
		struct EntryStack;
	};

	struct Animator::SerializedPlayState::EntryStack : public virtual Serialization::Serializable {
		Animator* animator = nullptr;

		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(animator, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ChannelCount, SetChannelCount, "Channel Count", "Animation channel count");
			};
			static thread_local List serializers;
			while (serializers.size() < animator->ChannelCount()) {
				std::stringstream stream;
				stream << serializers.size();
				const std::string name = stream.str();
				serializers.push_back(Object::Instantiate<SerializedPlayState>(name));
			}
			for (size_t i = 0u; i < animator->ChannelCount(); i++) {
				AnimationChannel channel = animator->Channel(i);
				recordElement(serializers[i]->Serialize(channel));
			}
		}
	};

	void Animator::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		// Serialize entries:
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			SerializedPlayState::EntryStack stack = {};
			stack.animator = this;
			JIMARA_SERIALIZE_FIELD(stack, "Animations", "Animation states");
			JIMARA_SERIALIZE_FIELD_GET_SET(RootMotionSource, SetRootMotionSource, "Root Motion Bone", "Root motion source transform.");
			if (RootMotionSource() != nullptr) {
				JIMARA_SERIALIZE_FIELD_GET_SET(RootMotionSettings, SetRootMotionSettings, "Root Motion Flags", "Settings for root motion",
					Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<RootMotionFlags>>>(true,
						"MOVE_X", RootMotionFlags::MOVE_X,
						"MOVE_Y", RootMotionFlags::MOVE_Y,
						"MOVE_Z", RootMotionFlags::MOVE_Z,
						"ROTATE_X", RootMotionFlags::ROTATE_X,
						"ROTATE_Y", RootMotionFlags::ROTATE_Y,
						"ROTATE_Z", RootMotionFlags::ROTATE_Z,
						"ANIMATE_BONE_POS_X", RootMotionFlags::ANIMATE_BONE_POS_X,
						"ANIMATE_BONE_POS_Y", RootMotionFlags::ANIMATE_BONE_POS_Y,
						"ANIMATE_BONE_POS_Z", RootMotionFlags::ANIMATE_BONE_POS_Z,
						"ANIMATE_BONE_ROT_X", RootMotionFlags::ANIMATE_BONE_ROT_X,
						"ANIMATE_BONE_ROT_Y", RootMotionFlags::ANIMATE_BONE_ROT_Y,
						"ANIMATE_BONE_ROT_X", RootMotionFlags::ANIMATE_BONE_ROT_Z));
				JIMARA_SERIALIZE_FIELD_GET_SET(RootMotionTarget, SetRootMotionTarget, "Root Motion Body",
					"Rigidbody that should be moved instead of the bone [If null, parent transform will be used instead]");
			}
		};
	}

	void Animator::Update() {
		if (Destroyed()) return;
		Bind();
		static thread_local std::vector<PlaybackState*> reactivatedStates;
		reactivatedStates.clear();
		for (auto it = m_reactivatedChannels.begin(); it != m_reactivatedChannels.end(); ++it)
			if (((*it)->isPlaying) && size_t((*it) - m_channelStates.data()) < m_channelCount)
				reactivatedStates.push_back(*it);
		m_reactivatedChannels.clear();
		ReactivateChannels(reactivatedStates);
		Apply();
		AdvanceTime(reactivatedStates);
		DeactivateChannels();
		reactivatedStates.clear();
	}



	void Animator::Apply() {
		const FieldBindingInfo* ptr = m_flattenedFieldBindings.data();
		const FieldBindingInfo* const end = ptr + m_flattenedFieldBindings.size();
		while (ptr < end) {
			if (ptr->activeBindingCount > 0u)
				ptr->update(ptr->field, ptr->bindings, ptr->activeBindingCount);
			ptr++;
		}
	}

	void Animator::AdvanceTime(const std::vector<PlaybackState*>& reactivatedStates) {
		float deltaTime = Context()->Time()->ScaledDeltaTime() * m_playbackSpeed;
		for (size_t i = 0u; i < reactivatedStates.size(); i++) {
			assert(size_t(reactivatedStates[i] - m_channelStates.data()) < m_channelCount);
			m_activeChannelStates.insert(reactivatedStates[i]);
		}
		for (auto it = m_activeChannelStates.begin(); it != m_activeChannelStates.end(); ++it) {
			PlaybackState& state = **it;
			if (state.clip == nullptr || size_t((&state) - m_channelStates.data()) >= m_channelCount)
				state.isPlaying = false;
			if (!state.isPlaying) {
				m_completeClipBuffer.push_back(&state);
				continue;
			}
			const float clipDeltaTime = deltaTime * state.speed;
			float newTime = state.time + clipDeltaTime;
			const float clipDuration = state.clip->Duration();
			if (newTime < 0.0f || newTime > clipDuration) {
				if (!state.loop) {
					if (state.time > 0.0f && newTime < 0.0f)
						newTime = 0.0f;
					else if (state.time < clipDuration && newTime > clipDuration)
						newTime = clipDuration;
					else {
						state.isPlaying = false;
						newTime = (clipDeltaTime > 0.0f) ? 0.0f : clipDuration;
						m_completeClipBuffer.push_back(&state);
					}
				}
				else if (clipDuration > 0.0f)
					newTime = Math::FloatRemainder(newTime, clipDuration);
				else newTime = 0.0f;
			}
			state.time = newTime;
		}
		if (m_completeClipBuffer.size() > 0u) {
			for (size_t i = 0u; i < m_completeClipBuffer.size(); i++)
				m_activeChannelStates.erase(m_completeClipBuffer[i]);
			m_completeClipBuffer.clear();
		}
	}

	void Animator::Unbind() {
		if (!m_bound) return;
		m_activeChannelStates.clear();
		m_reactivatedChannels.clear();
		m_activeTrackBindings.clear();
		m_flattenedFieldBindings.clear();
		for (auto it = m_subscribedClips.begin(); it != m_subscribedClips.end(); ++it)
			(*it)->OnDirty() -= Callback(&Animator::OnAnimationClipDirty, this);
		m_subscribedClips.clear();
		for (ObjectBindings::const_iterator it = m_objectBindings.begin(); it != m_objectBindings.end(); ++it) {
			it->first->OnParentChanged() -= Callback(&Animator::OnTransformHeirarchyChanged, this);
			it->first->OnDestroyed() -= Callback(&Animator::OnComponentDead, this);
		}
		m_objectBindings.clear();
		m_bound = false;
	}

	struct Animator::BindingHelper {
		typedef FieldUpdateFn(*GetUpdaterFn)(const Serialization::SerializedObject&);
		typedef bool(*CheckTrackFn)(const AnimationTrack*);

		inline static Vector3 LerpAngles(const Vector3& a, const Vector3& b, float t) {
			if (std::abs(t - 1.0f) < std::numeric_limits<float>::epsilon()) return b;
			else if (std::abs(t) < std::numeric_limits<float>::epsilon()) return a;
			const Matrix4 matA = Math::MatrixFromEulerAngles(a);
			const Matrix4 matB = Math::MatrixFromEulerAngles(b);
			const Vector3 tmpRight = Math::Lerp(matA[0], matB[0], t);
			const Vector3 up = Math::Normalize(Math::Lerp(matA[1], matB[1], t));
			const Vector3 forward = Math::Normalize(Math::Cross(tmpRight, up));
			const Vector3 right = Math::Normalize(Math::Cross(up, forward));
			const Matrix4 matC = Matrix4(
				Vector4(right, 0.0f),
				Vector4(up, 0.0f),
				Vector4(forward, 0.0f),
				Vector4(Vector3(0.0f), 1.0f));
			const Vector3 c = Math::EulerAnglesFromMatrix(matC);
			return c;
		}

		inline static float LerpAngles(float a, float b, float t) { return Math::LerpAngles(a, b, t); }

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
				const float weight = trackBinding.state->weight;
				if (weight <= 0.0f) return false;
#pragma warning(disable: 26451)
				value += ValueType(curve->Value(trackBinding.state->time) * weight);
#pragma warning(default: 26451)
				totalWeight += weight;
				return true;
			}

			template<typename Type0, typename Type1, typename... Rest>
			inline bool AddValue(const TrackBinding& trackBinding) {
				if (AddValue<Type0>(trackBinding)) return true;
				else return AddValue<Type1, Rest...>(trackBinding);
			}

			template<typename... CastableTypes>
			inline static void Interpolate(const Animator::SerializedField& field, const Animator::TrackBinding* start, size_t count) {
				InterpolationState state;
				const Animator::TrackBinding* const end = start + count;
				for (const Animator::TrackBinding* ptr = start; ptr < end; ptr++)
					state.AddValue<ValueType, CastableTypes...>(*ptr);
				if (state.totalWeight > 0.0f) {
					state.value = ValueType(state.value / state.totalWeight);
					SetValue(field, state.value);
				}
			}

			template<typename Type>
			inline bool AddValueEuler(const TrackBinding& trackBinding) {
				const ParametricCurve<Type, float>* const curve = dynamic_cast<const ParametricCurve<Type, float>*>(trackBinding.track);
				if (curve == nullptr) return false;
				const float blendWeight = trackBinding.state->weight;
				if (blendWeight <= 0.0f) return false;
				totalWeight += blendWeight;
				const float relativeWeight = blendWeight / totalWeight;
				const Type curveValue = ValueType(curve->Value(trackBinding.state->time));
				value = LerpAngles(value, curveValue, relativeWeight);
				return true;
			}

			template<typename Type0, typename Type1, typename... Rest>
			inline bool AddValueEuler(const TrackBinding& trackBinding) {
				if (AddValueEuler<Type0>(trackBinding)) return true;
				else return AddValueEuler<Type1, Rest...>(trackBinding);
			}

			template<typename... CastableTypes>
			inline static void InterpolateEuler(const Animator::SerializedField& field, const Animator::TrackBinding* start, size_t count) {
				InterpolationState state;
				const Animator::TrackBinding* const end = start + count;
				for (const Animator::TrackBinding* ptr = start; ptr < end; ptr++)
					state.AddValueEuler<ValueType, CastableTypes...>(*ptr);
				if (state.totalWeight > 0.0f)
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
			inline static void SetFirst(const Animator::SerializedField& field, const Animator::TrackBinding* start, size_t count) {
				const Animator::TrackBinding* const end = start + count;
				for (const Animator::TrackBinding* ptr = start; ptr < end; ptr++)
					if (SetValue<ValueType, CastableTypes...>(field, *ptr)) return;
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
				, FieldUpdateFn> GetUpdateFn(const Serialization::SerializedObject&) {
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
				, FieldUpdateFn> GetUpdateFn(const Serialization::SerializedObject&) {
				return InterpolationState<ValueType>::template Interpolate<CastableTypes...>;
			}

			template<typename ValueType, typename... CastableTypes>
			inline static std::enable_if_t<
				std::is_same_v<ValueType, float> ||
				std::is_same_v<ValueType, Vector3>
				, FieldUpdateFn> GetUpdateFn(const Serialization::SerializedObject& object) {
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

		inline static FieldUpdateFn GetUpdateFn(const Serialization::SerializedObject& serializedField, const AnimationClip::Track* track) {
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

		


		struct RootMotion {
			using FieldUpdater = std::pair<SerializedField, FieldUpdateFn>;

			struct Serializer : public virtual Serialization::SerializerList::From<Animator> {
				inline Serializer(const std::string_view& name) : Serialization::ItemSerializer(name, "") {}
				inline virtual ~Serializer() {}
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>&, Animator*)const final override { assert(false); }
			};

			static SerializedField MovementField(Animator* self) {
				static const Serializer instance("Root Movement");
				SerializedField field;
				field.serializer = &instance;
				field.targetAddr = (void*)self;
				return field;
			}

			static SerializedField RotationField(Animator* self) {
				static const Serializer instance("Root Rotation");
				SerializedField field;
				field.serializer = &instance;
				field.targetAddr = (void*)self;
				return field;
			}

			static void MovementUpdater(const SerializedField& field, const TrackBinding* start, size_t bindingCount) {
				Animator* self = (Animator*)field.targetAddr;
				const RootMotionFlags flags = self->RootMotionSettings();
				auto hasFlag = [&](RootMotionFlags flag) {
					return (
						static_cast<std::underlying_type_t<RootMotionFlags>>(flag) &
						static_cast<std::underlying_type_t<RootMotionFlags>>(flags)) != 0u;
				};

				const float deltaTime = self->Context()->Time()->ScaledDeltaTime();
				const float animatorDeltaTime = deltaTime * self->m_playbackSpeed;

				Vector3 deltaSum = Vector3(0.0f);
				Vector3 startPosSum = Vector3(0.0f);
				float totalWeight = 0.0f;

				const TrackBinding* const end = start + bindingCount;
				for (const TrackBinding* ptr = start; ptr < end; ptr++) {
					const PlaybackState& playbackState = *ptr->state;
					if (playbackState.weight <= 0.0f)
						continue;
					const AnimationTrack* const track = ptr->track;
					const ParametricCurve<Vector3, float>* curve = dynamic_cast<const ParametricCurve<Vector3, float>*>(track);

					Vector3 localDelta;
					const float animationDuration = std::abs(track->Duration());
					const Vector3 startPos = curve->Value(playbackState.time);
					if (animationDuration > std::numeric_limits<float>::epsilon()) {
						const float trackDeltaTime = animatorDeltaTime * playbackState.speed;
						assert(playbackState.time >= 0.0f);
						const float nextTime = playbackState.time + trackDeltaTime;
						auto loopedDistance = [&](auto loopT) {
							return
								((curve->Value(animationDuration) - curve->Value(0.0f)) * (trackDeltaTime / animationDuration)) +
								(curve->Value(Math::FloatRemainder(nextTime, animationDuration)) - curve->Value(loopT));
						};
						if (nextTime < 0) {
							if (playbackState.loop)
								localDelta = loopedDistance(animationDuration);
							else localDelta = curve->Value(0.0f) - startPos;
						}
						else if (nextTime > animationDuration) {
							if (playbackState.loop)
								localDelta = loopedDistance(0.0f);
							else localDelta = curve->Value(animationDuration) - startPos;
						}
						else localDelta = curve->Value(nextTime) - startPos;
					}
					else localDelta = Vector3(0.0f);

					deltaSum += localDelta * playbackState.weight;
					startPosSum += startPos * playbackState.weight;
					totalWeight += playbackState.weight;
				}
				if (totalWeight <= 0.0f)
					return;

				Rigidbody* const body = self->RootMotionTarget();
				Transform* const transform = (body == nullptr) ? self->GetTransfrom() : body->GetTransfrom();
				Transform* const rootMotionSource = self->RootMotionSource();
				if (rootMotionSource == nullptr)
					return;
				else {
					const Vector3 rootMotionSourceOldPos = rootMotionSource->LocalPosition();
					const Vector3 rootMotionSourceNewPos = (totalWeight > 0.0f) ? (startPosSum / totalWeight) : rootMotionSourceOldPos;
					rootMotionSource->SetLocalPosition(Vector3(
						hasFlag(RootMotionFlags::ANIMATE_BONE_POS_X) ? rootMotionSourceNewPos.x : rootMotionSourceOldPos.x,
						hasFlag(RootMotionFlags::ANIMATE_BONE_POS_Y) ? rootMotionSourceNewPos.y : rootMotionSourceOldPos.y,
						hasFlag(RootMotionFlags::ANIMATE_BONE_POS_Z) ? rootMotionSourceNewPos.z : rootMotionSourceOldPos.z));
				}

				if (transform == nullptr)
					return;

				const Vector3 bonePositionDelta =
					(totalWeight > 0.0f) ? (deltaSum / totalWeight) : Vector3(0.0f);
				Vector3 bodyPositionDelta = bonePositionDelta;
				for (Transform* ptr = rootMotionSource->GetComponentInParents<Transform>(false);
					(ptr != transform && ptr != nullptr);
					ptr = ptr->GetComponentInParents<Transform>(false))
					bodyPositionDelta = ptr->LocalMatrix() * Vector4(bodyPositionDelta, 0.0f);

				if (body == nullptr) {
					transform->SetLocalPosition(transform->LocalPosition() + Vector3(
						hasFlag(RootMotionFlags::MOVE_X) ? bodyPositionDelta.x : 0.0f,
						hasFlag(RootMotionFlags::MOVE_Y) ? bodyPositionDelta.y : 0.0f,
						hasFlag(RootMotionFlags::MOVE_Z) ? bodyPositionDelta.z : 0.0f));
				}
				else {
					const Matrix4 worldMatrix = transform->WorldRotationMatrix();
					const Vector3 forward = Math::Normalize((Vector3)worldMatrix[2]);
					const Vector3 right = Math::Normalize((Vector3)worldMatrix[0]);
					const Vector3 up = Math::Normalize((Vector3)worldMatrix[1]);

					const Vector3 oldAbsoluteVelocity = body->Velocity();
					const Vector3 oldVelocity = Vector3(
						Math::Dot(right, oldAbsoluteVelocity),
						Math::Dot(up, oldAbsoluteVelocity),
						Math::Dot(forward, oldAbsoluteVelocity));
					const Vector3 newVelocity = bodyPositionDelta *
						((std::abs(deltaTime) > 0.0f) ? (1.0f / deltaTime) : 0.0f);
					body->SetVelocity(worldMatrix * Vector4(
						hasFlag(RootMotionFlags::MOVE_X) ? newVelocity.x : oldVelocity.x,
						hasFlag(RootMotionFlags::MOVE_Y) ? newVelocity.y : oldVelocity.y,
						hasFlag(RootMotionFlags::MOVE_Z) ? newVelocity.z : oldVelocity.z, 0.0f));
				}
			}

			static void RotationUpdater(const SerializedField& field, const TrackBinding* start, size_t bindingCount) {
				Animator* self = (Animator*)field.targetAddr;
				const RootMotionFlags flags = self->RootMotionSettings();
				auto hasFlag = [&](RootMotionFlags flag) {
					return (
						static_cast<std::underlying_type_t<RootMotionFlags>>(flag) &
						static_cast<std::underlying_type_t<RootMotionFlags>>(flags)) != 0u;
				};

				const float deltaTime = self->Context()->Time()->ScaledDeltaTime();
				const float animatorDeltaTime = deltaTime * self->m_playbackSpeed;

				Vector3 startAngle = Vector3(0.0f);
				Vector3 endAngle = Vector3(0.0f);
				float weightSoFar = 0.0f;

				const TrackBinding* const end = start + bindingCount;
				for (const TrackBinding* ptr = start; ptr < end; ptr++) {
					const PlaybackState& playbackState = *ptr->state;
					const AnimationTrack* const track = ptr->track;
					const ParametricCurve<Vector3, float>* curve = dynamic_cast<const ParametricCurve<Vector3, float>*>(track);
					if (playbackState.weight <= 0.0f)
						continue;

					const float animationDuration = std::abs(track->Duration());
					const float trackDeltaTime = animatorDeltaTime * playbackState.speed;
					const float nextTime =
						(animationDuration <= std::numeric_limits<float>::epsilon()) ? 0.0f :
						(playbackState.loop) ? Math::FloatRemainder(playbackState.time + trackDeltaTime, animationDuration) :
						Math::Min(Math::Max(0.0f, playbackState.time + trackDeltaTime), animationDuration);

					weightSoFar += playbackState.weight;
					const float weightFraction = (playbackState.weight / weightSoFar);
					startAngle = LerpAngles(startAngle, curve->Value(playbackState.time), weightFraction);
					endAngle = LerpAngles(endAngle, curve->Value(nextTime), weightFraction);
				}
				if (weightSoFar <= 0.0f)
					return;

				Rigidbody* const body = self->RootMotionTarget();
				Transform* const transform = (body == nullptr) ? self->GetTransfrom() : body->GetTransfrom();
				Transform* const rootMotionSource = self->RootMotionSource();
				if (rootMotionSource == nullptr)
					return;
				else {
					const Vector3 oldBoneRotation = rootMotionSource->LocalEulerAngles();
					rootMotionSource->SetLocalEulerAngles(Vector3(
						hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_X) ? startAngle.x : oldBoneRotation.x,
						hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_Y) ? startAngle.y : oldBoneRotation.y,
						hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_Z) ? startAngle.z : oldBoneRotation.z));
				}

				if (transform == nullptr)
					return;

				Matrix4 startRotationMatrix = Math::MatrixFromEulerAngles(startAngle);
				Matrix4 endRotationMatrix = Math::MatrixFromEulerAngles(endAngle);
				for (Transform* ptr = rootMotionSource->GetComponentInParents<Transform>(false);
					(ptr != transform && ptr != nullptr);
					ptr = ptr->GetComponentInParents<Transform>(false)) {
					const Matrix4 localRotationMatrix = ptr->LocalRotationMatrix();
					startRotationMatrix = localRotationMatrix * startRotationMatrix;
					endRotationMatrix = localRotationMatrix * endRotationMatrix;
				}
				auto angleDelta = [](const Matrix4& start, const Matrix4& end) {
					// endRotationMatrix = startRotationMatrix * deltaRotationMatrix =>
					//	deltaRotationMatrix = (1 / startRotationMatrix) * endRotationMatrix;
					return Math::EulerAnglesFromMatrix(Math::Inverse(start) * end);
				};
				if (body == nullptr) {
					const Vector3 delta = angleDelta(startRotationMatrix, endRotationMatrix);
					transform->SetLocalEulerAngles(transform->LocalEulerAngles() + Vector3(
						hasFlag(RootMotionFlags::ROTATE_X) ? delta.x : 0.0f,
						hasFlag(RootMotionFlags::ROTATE_Y) ? delta.y : 0.0f,
						hasFlag(RootMotionFlags::ROTATE_Z) ? delta.z : 0.0f));
				}
				else {
					const Matrix4 globalRotationMatrix = transform->WorldRotationMatrix();
					const Vector3 oldAngularVelocity = body->AngularVelocity();
					const Vector3 newAngularVelocity =
						angleDelta(globalRotationMatrix * startRotationMatrix, globalRotationMatrix * endRotationMatrix) *
						((std::abs(deltaTime) > 0.0f) ? (1.0f / deltaTime) : 0.0f);
					body->SetAngularVelocity(Vector3(
						hasFlag(RootMotionFlags::ROTATE_X) ? newAngularVelocity.x : oldAngularVelocity.x,
						hasFlag(RootMotionFlags::ROTATE_Y) ? newAngularVelocity.y : oldAngularVelocity.y,
						hasFlag(RootMotionFlags::ROTATE_Z) ? newAngularVelocity.z : oldAngularVelocity.z));
				}
			}
		};
	};

	void Animator::Bind() {
		if (Destroyed() || m_bound) return;
		m_reactivatedChannels.clear();
		for (size_t channelId = 0u; channelId < m_channelCount; channelId++) {
			PlaybackState& channelState = m_channelStates[channelId];
			AnimationClip* const clip = channelState.clip;
			if (clip == nullptr)
				continue;
			if (m_subscribedClips.find(clip) == m_subscribedClips.end()) {
				clip->OnDirty() += Callback(&Animator::OnAnimationClipDirty, this);
				m_subscribedClips.insert(clip);
			}
			if (channelState.isPlaying)
				m_reactivatedChannels.insert(&channelState);
			
			for (size_t trackId = 0; trackId < clip->TrackCount(); trackId++) {
				const AnimationClip::Track* track = clip->GetTrack(trackId);
				if (track == nullptr) continue;
				
				// Find target component:
				Component* animatedComponent = dynamic_cast<Component*>(track->FindTarget(Parent()));
				if (animatedComponent == nullptr) continue;
				
				// Find target serialized field:
				SerializedField serializedObject;
				FieldUpdateFn updateFn = nullptr;
				{
					if (animatedComponent == RootMotionSource() && 
						dynamic_cast<const ParametricCurve<Vector3, float>*>(track) != nullptr) {
						auto setUpdateFn = [&](FieldUpdateFn fn, const SerializedField& field) {
							serializedObject = field;
							updateFn = fn;
						};
						if (track->TargetField() == "Position")
							setUpdateFn(BindingHelper::RootMotion::MovementUpdater, BindingHelper::RootMotion::MovementField(this));
						if (track->TargetField() == "Rotation")
							setUpdateFn(BindingHelper::RootMotion::RotationUpdater, BindingHelper::RootMotion::RotationField(this));
					}
					if (serializedObject.serializer == nullptr) {
						auto processField = [&](const Serialization::SerializedObject& serializedField) {
							if (serializedField.Serializer() == nullptr || serializedObject.serializer != nullptr) return;
							updateFn = BindingHelper::GetUpdateFn(serializedField, track);
							if (updateFn == nullptr) return;
							serializedObject.serializer = serializedField.Serializer();
							serializedObject.targetAddr = serializedField.TargetAddr();
						};
						animatedComponent->GetFields(Callback<Serialization::SerializedObject>::FromCall(&processField));
					}
					if (serializedObject.serializer == nullptr) 
						continue;
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
				binding.tracks[&channelState].Push(track);
				binding.bindingCount++;
			}
		}

		// Flatten field bindings for less randomized access patterns:
		{
			size_t totalBindingCount = 0u;
			for (ObjectBindings::const_iterator objectIt = m_objectBindings.begin(); objectIt != m_objectBindings.end(); ++objectIt) {
				const FieldBindings& fieldBindings = objectIt->second;
				for (FieldBindings::const_iterator fieldIt = fieldBindings.begin(); fieldIt != fieldBindings.end(); ++fieldIt)
					totalBindingCount += fieldIt->second.bindingCount;
			}
			m_activeTrackBindings.resize(totalBindingCount);
			m_flattenedFieldBindings.clear();
			TrackBinding* bindingPtr = m_activeTrackBindings.data();
			for (ObjectBindings::const_iterator objectIt = m_objectBindings.begin(); objectIt != m_objectBindings.end(); ++objectIt) {
				const FieldBindings& fieldBindings = objectIt->second;
				for (FieldBindings::const_iterator fieldIt = fieldBindings.begin(); fieldIt != fieldBindings.end(); ++fieldIt) {
					FieldBindingInfo info = {};
					info.field = fieldIt->first;
					info.update = fieldIt->second.update;
					info.tracks = &fieldIt->second.tracks;
					info.bindings = bindingPtr;
					info.activeBindingCount = 0u;
					info.fieldBindingCount = fieldIt->second.bindingCount;
					bindingPtr += info.fieldBindingCount;
					m_flattenedFieldBindings.push_back(info);
				}
			}
		}
		m_bound = true;
	}

	void Animator::ReactivateChannels(const std::vector<PlaybackState*>& reactivatedStates) {
		FieldBindingInfo* ptr = m_flattenedFieldBindings.data();
		FieldBindingInfo* const end = ptr + m_flattenedFieldBindings.size();
		const PlaybackState* const* const firstState = reactivatedStates.data();
		const PlaybackState* const* const lastState = firstState + reactivatedStates.size();
		while (ptr < end) {
			FieldBindingInfo& info = *ptr;
			ptr++;
			for (const PlaybackState* const* statePtr = firstState; statePtr < lastState; statePtr++) {
				const PlaybackState* const state = *statePtr;

				const auto tracks = info.tracks->find(state);
				if (tracks == info.tracks->end())
					continue;

				const AnimationTrack* const* trackIt = tracks->second.Data();
				const AnimationTrack* const* const trackEnd = trackIt + tracks->second.Size();
				while (trackIt < trackEnd) {
					const AnimationTrack* const track = *trackIt;
					trackIt++;

					// Find insertion index:
					const size_t activeIndex = BinarySearch_LE(info.activeBindingCount, [&](size_t index) {
						return
							(info.bindings[index].state > state) ||
							(info.bindings[index].state == state && info.bindings[index].track > track);
						});
					if (activeIndex < info.activeBindingCount &&
						info.bindings[activeIndex].state == state &&
						info.bindings[activeIndex].track == track)
						continue;
					const size_t insertionIndex = (activeIndex < info.activeBindingCount)
						? (activeIndex + 1u) : 0u;

					// Insert track record:
					for (size_t index = info.activeBindingCount; index > insertionIndex; index--)
						info.bindings[index] = info.bindings[index - 1u];
					info.bindings[insertionIndex] = { track, state };
					info.activeBindingCount++;
				}

#ifndef NDEBUG
				// Debug only... Make sure states are still ordered:
				for (size_t index = 1u; index < info.activeBindingCount; index++)
					assert(info.bindings[index].state > info.bindings[index - 1u].state ||
						(info.bindings[index].state == info.bindings[index - 1u].state &&
							info.bindings[index].track > info.bindings[index - 1u].track));
#endif
			}
		}
	}

	void Animator::DeactivateChannels() {
		FieldBindingInfo* fieldPtr = m_flattenedFieldBindings.data();
		FieldBindingInfo* const fieldEnd = fieldPtr + m_flattenedFieldBindings.size();
		while (fieldPtr < fieldEnd) {
			FieldBindingInfo& info = *fieldPtr;
			fieldPtr++;
			TrackBinding* ptr = info.bindings;
			TrackBinding* retainedPtr = ptr;
			TrackBinding* const end = ptr + info.activeBindingCount;
			while (ptr < end) {
				const TrackBinding& binding = *ptr;
				ptr++;
				if (binding.state->isPlaying) {
					(*retainedPtr) = binding;
					retainedPtr++;
				}
			}
			info.activeBindingCount = (retainedPtr - info.bindings);
		}
	}

	void Animator::OnAnimationClipDirty(const AnimationClip*) { Unbind(); }

	void Animator::OnTransformHeirarchyChanged(ParentChangeInfo) { Unbind(); }
	
	void Animator::OnComponentDead(Component* component) { Unbind(); }

	template<> void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Animator>(
			"Animator", "Jimara/Animation/Animator", "Component, responsible for AnimationClip playback and blending");
		report(factory);
	}
}
