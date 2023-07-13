#include "Animator.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EulerAnglesAttribute.h"
#include "../../Data/Serialization/Attributes/HideInEditorAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


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


	void Animator::SetClipState(AnimationClip* clip, ClipPlaybackState state) {
		if (clip == nullptr || state == ClipState(clip)) return;
		{
			const float clipDuration = std::abs(clip->Duration());
			if (clipDuration > std::numeric_limits<float>::epsilon())
				state.time = Math::FloatRemainder(state.time, clipDuration);
			else state.time = 0.0f;
		}
		ClipStates::iterator it = m_clipStates.find(clip);
		if (it != m_clipStates.end()) {
			if (state.weight > 0.0f) {
				it->second = PlaybackState(state, it->second.insertionId);
				return; // No real need to unbind anything here...
			}
			else m_clipStates.erase(it);
		}
		else if (state.weight > 0.0f) {
			m_clipStates.insert(std::make_pair(Reference<AnimationClip>(clip), PlaybackState(state, m_numInsertions)));
			m_numInsertions++;
		}
		Unbind();
	}

	void Animator::Play(AnimationClip* clip, bool loop, float speed, float weight, float timeOffset) {
		SetClipState(clip, ClipPlaybackState(timeOffset, weight, speed, loop));
	}

	Animator::ClipPlaybackState Animator::ClipState(AnimationClip* clip)const {
		auto noState = []() { return ClipPlaybackState(0.0f, 0.0f, 0.0f, false); };
		if (clip == nullptr) return noState();
		ClipStates::const_iterator it = m_clipStates.find(clip);
		if (it != m_clipStates.end()) return it->second;
		else return noState();
	}

	bool Animator::ClipPlaying(AnimationClip* clip)const { return (m_clipStates.find(clip) != m_clipStates.end()); }

	bool Animator::Playing()const { return (!m_clipStates.empty()); }

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
		if (m_rootMotionSettings == flags)
			return;
		m_rootMotionSettings = flags;
		Unbind();
	}

	Rigidbody* Animator::RootMotionTarget()const {
		const Reference<Rigidbody> body = m_rootRigidbody;
		return body;
	}

	void Animator::SetRootMotionTarget(Rigidbody* body) {
		m_rootRigidbody = body;
	}


	struct Animator::SerializedPlayState : public virtual Serialization::Serializable {
		Reference<AnimationClip> clip;
		PlaybackState state;
		size_t index = 0;

		inline static const constexpr uint64_t NoId() { return ~uint64_t(0); }

		inline SerializedPlayState(AnimationClip* c = nullptr, const PlaybackState& s = {}) : clip(c), state(s) {}

		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(clip, "Clip", "Animation clip (set to nullptr to remove; set to any clip to add if nullptr)");
				if (clip != nullptr) {
					JIMARA_SERIALIZE_FIELD(state.time, "Time", "Animation time point");
					JIMARA_SERIALIZE_FIELD(state.weight, "Weight", "Blending weight (less than or equal to zeor will result in removing the clip)");
					JIMARA_SERIALIZE_FIELD(state.speed, "Speed", "Animation playback speed");
					JIMARA_SERIALIZE_FIELD(state.loop, "Loop", "If true, animation will be looping");
				}
				//JIMARA_SERIALIZE_FIELD(state.insertionId, "InsertionId", "AA");
			};
		}

		typedef std::vector<std::unique_ptr<SerializedPlayState>> List;

		struct EntryStack;
	};

	struct Animator::SerializedPlayState::EntryStack : public virtual Serialization::Serializable {
		List* list = nullptr;
		size_t startId = 0;
		size_t* endId = nullptr;
		Animator* animator = nullptr;

		inline void AddEntries(size_t listPtr) {
			while (list->size() < listPtr) {
				list->push_back(std::make_unique<SerializedPlayState>());
				list->back()->state.insertionId = NoId();
				list->back()->index = list->size() - 1;
			}
		}

		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) {
			size_t& listPtr = *endId;
			JIMARA_SERIALIZE_FIELDS(list, recordElement) {
				{
					size_t count = (listPtr - startId);
					JIMARA_SERIALIZE_FIELD(count, "Count", "Animation count", Serialization::HideInEditorAttribute::Instance());
					listPtr = startId + count;
					AddEntries(listPtr);
				}
				for (size_t i = startId; i < listPtr; i++)
					JIMARA_SERIALIZE_FIELD(*list->operator[](i), "Clip State", "Animation clip state");
			};
		}
	};

	void Animator::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);

		// List state:
		static thread_local SerializedPlayState::List list;
		static thread_local std::vector<uint64_t> initialInsertionIds;
		static thread_local size_t listPtr = 0;
		const size_t startId = listPtr;
		listPtr = startId + m_clipStates.size() + 1;

		// Fill list segment:
		{
			if (list.size() < listPtr) list.resize(listPtr);
			while (initialInsertionIds.size() < listPtr)
				initialInsertionIds.push_back(SerializedPlayState::NoId());
			
			{
				size_t i = startId;
				auto setEntry = [&](const SerializedPlayState& state) {
					auto& ptr = list[i];
					if (ptr == nullptr) {
						ptr = std::make_unique<SerializedPlayState>();
						ptr->index = i;
					}
					ptr->clip = state.clip;
					ptr->state = state.state;
					i++;
				};
				for (const auto& entry : m_clipStates)
					setEntry(SerializedPlayState(entry.first, entry.second));
				setEntry([]() {
					SerializedPlayState state = {};
					state.state.insertionId = SerializedPlayState::NoId();
					return state;
					}());
				if (listPtr != i)
					Context()->Log()->Error("Animator::GetFields - Internal error: (listPtr != i)! [File: '", __FILE__, "'; Line: ", __LINE__, "]");
			}
		}

		// Sort for consistency:
		std::sort(list.data() + startId, list.data() + listPtr,
			[](const std::unique_ptr<SerializedPlayState>& a, const std::unique_ptr<SerializedPlayState>& b) {
				return a->state.insertionId < b->state.insertionId ||
					(a->state.insertionId == b->state.insertionId && a->clip < b->clip);
			});

		// Recreate whatever needs recreating:
		for (size_t i = startId; i < listPtr; i++) {
			auto& ptr = list[i];
			uint64_t& initialInsertionId = initialInsertionIds[i];
			SerializedPlayState state = *ptr;
			if (initialInsertionId == SerializedPlayState::NoId())
				initialInsertionId = state.state.insertionId;
			if (initialInsertionId == state.state.insertionId) continue;
			ptr = std::make_unique<SerializedPlayState>(state);
			initialInsertionId = state.state.insertionId;
		}

		// Serialize entries:
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			SerializedPlayState::EntryStack stack = {};
			{
				stack.list = &list;
				stack.startId = startId;
				stack.endId = &listPtr;
				stack.animator = this;
			}
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

		// Restore initial order:
		{
			static thread_local SerializedPlayState::List fixOrderList;
			const size_t count = (listPtr - startId);
			if (fixOrderList.size() < count)
				fixOrderList.resize(count);
			for (size_t i = startId; i < listPtr; i++) {
				auto& elem = list[i];
				if (elem->index >= listPtr || elem->index < startId)
					Context()->Log()->Error("Animator::GetFields - Internal error: Element index mismatch! [File: '", __FILE__, "'; Line: ", __LINE__, "]");
				std::swap(fixOrderList[elem->index - startId], elem);
			}
			for (size_t i = startId; i < listPtr; i++)
				std::swap(list[i], fixOrderList[i - startId]);
		}

		// Clear list, recreate the internal state and 'free' list stack:
		{
			Unbind();
			m_clipStates.clear();
			for (size_t i = startId; i < listPtr; i++) {
				SerializedPlayState& entry = *list[i];
				if (entry.clip != nullptr && entry.state.weight > std::numeric_limits<float>::epsilon()) {
					if (entry.state.insertionId == SerializedPlayState::NoId()) {
						entry.state.insertionId = m_numInsertions;
						m_numInsertions++;
					}
					m_clipStates[entry.clip] = entry.state;
				}
				entry.clip = nullptr;
			}
			listPtr = startId;
		}
	}

	void Animator::Update() {
		if (Destroyed()) return;
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
		float deltaTime = Context()->Time()->ScaledDeltaTime() * m_playbackSpeed;
		for (ClipStates::iterator it = m_clipStates.begin(); it != m_clipStates.end(); ++it) {
			float clipDeltaTime = deltaTime * it->second.speed;
			float newTime = it->second.time + clipDeltaTime;
			const float clipDuration = it->first->Duration();
			if (newTime < 0.0f || newTime > clipDuration) {
				if (!it->second.loop)
					m_completeClipBuffer.push_back(it->first);
				else if (clipDuration > 0.0f)
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
#pragma warning(disable: 26451)
				value += ValueType(curve->Value(trackBinding.state->time) * trackBinding.state->weight);
#pragma warning(default: 26451)
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
				const float weight = trackBinding.state->weight / totalWeight;
				const Type curveValue = ValueType(curve->Value(trackBinding.state->time));
				value = LerpAngles(value, curveValue, weight);
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

		


		struct RootMotion {
			using FieldUpdater = std::pair<SerializedField, FieldBinding::UpdateFn>;

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

			static void MovementUpdater(const SerializedField& field, const FieldBinding& bindings) {
				Animator* self = (Animator*)field.targetAddr;
				Rigidbody* const body = self->RootMotionTarget();
				Transform* const transform = (body == nullptr) ? self->GetTransfrom() : body->GetTransfrom();
				Transform* const rootMotionSource = self->RootMotionSource();
				if (transform == nullptr || rootMotionSource == nullptr)
					return;
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

				const TrackBinding* const start = bindings.bindings.Data();
				const TrackBinding* const end = start + bindings.bindings.Size();
				for (const TrackBinding* ptr = start; ptr < end; ptr++) {
					const ClipPlaybackState playbackState = *ptr->state;
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
					const Vector3 oldVelocity = body->Velocity();
					const Vector3 newVelocity = (transform->WorldMatrix() * Vector4(bodyPositionDelta, 0.0f)) *
						((std::abs(deltaTime) > 0.0f) ? (1.0f / deltaTime) : 0.0f);
					body->SetVelocity(Vector3(
						hasFlag(RootMotionFlags::MOVE_X) ? newVelocity.x : oldVelocity.x,
						hasFlag(RootMotionFlags::MOVE_Y) ? newVelocity.y : oldVelocity.y,
						hasFlag(RootMotionFlags::MOVE_Z) ? newVelocity.z : oldVelocity.z));
				}

				const Vector3 rootMotionSourceOldPos = rootMotionSource->LocalPosition();
				const Vector3 rootMotionSourceNewPos = (totalWeight > 0.0f) ? (startPosSum / totalWeight) : rootMotionSourceOldPos;
				rootMotionSource->SetLocalPosition(Vector3(
					hasFlag(RootMotionFlags::ANIMATE_BONE_POS_X) ? rootMotionSourceNewPos.x : rootMotionSourceOldPos.x,
					hasFlag(RootMotionFlags::ANIMATE_BONE_POS_Y) ? rootMotionSourceNewPos.y : rootMotionSourceOldPos.y,
					hasFlag(RootMotionFlags::ANIMATE_BONE_POS_Z) ? rootMotionSourceNewPos.z : rootMotionSourceOldPos.z));
			}

			static void RotationUpdater(const SerializedField& field, const FieldBinding& bindings) {
				Animator* self = (Animator*)field.targetAddr;
				Rigidbody* const body = self->RootMotionTarget();
				Transform* const transform = (body == nullptr) ? self->GetTransfrom() : body->GetTransfrom();
				Transform* const rootMotionSource = self->RootMotionSource();
				if (transform == nullptr || rootMotionSource == nullptr)
					return;
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

				const TrackBinding* const start = bindings.bindings.Data();
				const TrackBinding* const end = start + bindings.bindings.Size();
				for (const TrackBinding* ptr = start; ptr < end; ptr++) {
					const ClipPlaybackState playbackState = *ptr->state;
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

				const Vector3 oldBoneRotation = rootMotionSource->LocalEulerAngles();
				rootMotionSource->SetLocalEulerAngles(Vector3(
					hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_X) ? startAngle.x : oldBoneRotation.x,
					hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_Y) ? startAngle.y : oldBoneRotation.y,
					hasFlag(RootMotionFlags::ANIMATE_BONE_ROT_Z) ? startAngle.z : oldBoneRotation.z));
			}
		};
	};

	void Animator::Bind() {
		if (Destroyed() || m_bound) return;
		for (ClipStates::const_iterator clipIt = m_clipStates.begin(); clipIt != m_clipStates.end(); ++clipIt) {
			AnimationClip* clip = clipIt->first;
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
				FieldBinding::UpdateFn updateFn = nullptr;
				{
					if (animatedComponent == RootMotionSource() && 
						dynamic_cast<const ParametricCurve<Vector3, float>*>(track) != nullptr) {
						auto setUpdateFn = [&](FieldBinding::UpdateFn fn, const SerializedField& field) {
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

	void Animator::OnTransformHeirarchyChanged(ParentChangeInfo) { Unbind(); }
	
	void Animator::OnComponentDead(Component* component) { Unbind(); }

	template<> void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Animator> serializer("Jimara/Animator", "Animator");
		report(&serializer);
	}
}
