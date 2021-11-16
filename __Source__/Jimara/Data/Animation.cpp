#include "Animation.h"
#include "../Core/Helpers.h"
#include "../Components/Component.h"


namespace Jimara {
	AnimationClip::AnimationClip(const std::string_view& name) : m_name(name) {}

	AnimationClip::~AnimationClip() {
		for (size_t i = 0; i < m_tracks.size(); i++)
			m_tracks[i]->m_owner = nullptr;
	}

	const std::string& AnimationClip::Name()const { return m_name; }

	float AnimationClip::Duration()const { return m_duration; }

	size_t AnimationClip::TrackCount()const { return m_tracks.size(); }

	const AnimationClip::Track* AnimationClip::GetTrack(size_t index)const { return m_tracks[index]; }

	Event<const AnimationClip*>& AnimationClip::OnDirty()const { return m_onDirty; }


	float AnimationClip::Track::Duration()const { return m_owner == nullptr ? 0.0f : m_owner->Duration(); }

	const AnimationClip* AnimationClip::Track::Owner()const { return m_owner; }

	size_t AnimationClip::Track::Index()const { return m_index; }

	Object* AnimationClip::Track::FindTarget(Object* rootObject)const {
		Component* targetPtr = dynamic_cast<Component*>(rootObject);
		if (targetPtr == nullptr || m_bindChain.size() <= 0) {
			// __TODO__: Something like root transforms can be here...
			return targetPtr;
		}
		for (size_t nodeId = 0; nodeId < m_bindChain.size(); nodeId++) {
			const BindChainNode& node = m_bindChain[nodeId];
			Component* next = nullptr;
			for (size_t childId = 0; childId < targetPtr->ChildCount(); childId++) {
				Component* child = targetPtr->GetChild(childId);
				if (node.checkType(child) && node.name == child->Name()) {
					next = child;
					break;
				}
			}
			if (next == nullptr) return nullptr;
			else targetPtr = next;
		}
		return targetPtr;
	}

	const std::string& AnimationClip::Track::TargetField()const { return m_targetField; }



	AnimationClip::TripleFloatCombine::TripleFloatCombine(ParametricCurve<float, float>* x, ParametricCurve<float, float>* y, ParametricCurve<float, float>* z)
		: m_x(x), m_y(y), m_z(z) {
	}

	Reference<ParametricCurve<float, float>>& AnimationClip::TripleFloatCombine::X() { return m_x; }

	const ParametricCurve<float, float>* AnimationClip::TripleFloatCombine::X()const { return m_x; }

	Reference<ParametricCurve<float, float>>& AnimationClip::TripleFloatCombine::Y() { return m_y; }

	const ParametricCurve<float, float>* AnimationClip::TripleFloatCombine::Y()const { return m_y; }

	Reference<ParametricCurve<float, float>>& AnimationClip::TripleFloatCombine::Z() { return m_z; }

	const ParametricCurve<float, float>* AnimationClip::TripleFloatCombine::Z()const { return m_z; }

	Vector3 AnimationClip::TripleFloatCombine::Value(float time)const {
		return Vector3(
			m_x == nullptr ? 0.0f : m_x->Value(time),
			m_y == nullptr ? 0.0f : m_y->Value(time),
			m_z == nullptr ? 0.0f : m_z->Value(time));
	}



	AnimationClip::EulerAngleTrack::EulerAngleTrack(ParametricCurve<float, float>* x, ParametricCurve<float, float>* y, ParametricCurve<float, float>* z, EvaluationMode mode)
		: TripleFloatCombine(x, y, z), m_evaluate([](float, float, float) -> Vector3 { return Vector3(0.0f); }) {
		SetMode(mode);
	}

	void AnimationClip::EulerAngleTrack::SetMode(EvaluationMode mode) {
		if (mode >= EvaluationMode::MODE_COUNT) mode = EvaluationMode::YXZ_EULER;
		if (mode == m_mode) return;
		typedef Vector3(*EvalFn)(float, float, float);
		static const EvalFn* const EVAL_FUNCTIONS = []() ->const EvalFn* {
			static const uint8_t functionCount = static_cast<uint8_t>(EvaluationMode::MODE_COUNT);
			static EvalFn functions[functionCount];
			static const EvalFn noChange = [](float x, float y, float z) ->Vector3 { return Vector3(x, y, z); };
			for (size_t i = 0; i < functionCount; i++) functions[i] = noChange;
			functions[static_cast<uint8_t>(EvaluationMode::XYZ_EULER)] = [](float x, float y, float z)->Vector3 { 
				return Math::EulerAnglesFromMatrix(glm::eulerAngleXYZ(Math::Radians(x), Math::Radians(y), Math::Radians(z))); 
			};
			functions[static_cast<uint8_t>(EvaluationMode::XZY_EULER)] = [](float x, float y, float z)->Vector3 {
				return Math::EulerAnglesFromMatrix(glm::eulerAngleXZY(Math::Radians(x), Math::Radians(z), Math::Radians(y)));
			};
			functions[static_cast<uint8_t>(EvaluationMode::YXZ_EULER)] = [](float x, float y, float z)->Vector3 {
				return Vector3(Math::FloatRemainder(x, 360.0f), Math::FloatRemainder(y, 360.0f), Math::FloatRemainder(z, 360.0f));
			};
			functions[static_cast<uint8_t>(EvaluationMode::YZX_EULER)] = [](float x, float y, float z)->Vector3 {
				return Math::EulerAnglesFromMatrix(glm::eulerAngleYZX(Math::Radians(y), Math::Radians(z), Math::Radians(x)));
			};
			functions[static_cast<uint8_t>(EvaluationMode::ZXY_EULER)] = [](float x, float y, float z)->Vector3 {
				return Math::EulerAnglesFromMatrix(glm::eulerAngleZXY(Math::Radians(z), Math::Radians(x), Math::Radians(y)));
			};
			functions[static_cast<uint8_t>(EvaluationMode::ZYX_EULER)] = [](float x, float y, float z)->Vector3 {
				return Math::EulerAnglesFromMatrix(glm::eulerAngleZYX(Math::Radians(z), Math::Radians(y), Math::Radians(x)));
			};
			return functions;
		}();
		m_mode = mode;
		m_evaluate = Function<Vector3, float, float, float>(EVAL_FUNCTIONS[static_cast<uint8_t>(mode)]);
	}

	AnimationClip::EulerAngleTrack::EvaluationMode AnimationClip::EulerAngleTrack::Mode()const { return m_mode; }

	Vector3 AnimationClip::EulerAngleTrack::Value(float time)const {
		Vector3 base = TripleFloatCombine::Value(time);
		return m_evaluate(base.x, base.y, base.z);
	}

	AnimationClip::RotatedEulerAngleTrack::RotatedEulerAngleTrack(
		ParametricCurve<float, float>* x, ParametricCurve<float, float>* y, ParametricCurve<float, float>* z,
		EvaluationMode mode, const Matrix4& rotation)
		: TripleFloatCombine(x, y, z), EulerAngleTrack(x, y, z, mode), m_rotation(rotation) {}

	Matrix4& AnimationClip::RotatedEulerAngleTrack::Rotation() { return m_rotation; }

	Matrix4 AnimationClip::RotatedEulerAngleTrack::Rotation()const { return m_rotation; }

	Vector3 AnimationClip::RotatedEulerAngleTrack::Value(float time)const {
		return Math::EulerAnglesFromMatrix(m_rotation * Math::MatrixFromEulerAngles(EulerAngleTrack::Value(time)));
	}


	AnimationClip::Writer::Writer(AnimationClip* animation)
		: m_animation(animation) {
		m_animation->m_changeLock.lock();
	}

	AnimationClip::Writer::Writer(AnimationClip& animation) : Writer(&animation) {}

	AnimationClip::Writer::~Writer() {
		m_animation->m_changeLock.unlock();
		m_animation->m_onDirty(m_animation);
	}

	std::string& AnimationClip::Writer::Name()const { return m_animation->m_name; }

	float AnimationClip::Writer::Duration()const { return m_animation->m_duration; }

	void AnimationClip::Writer::SetDuration(float duration)const {
		if (duration < 0.0f) duration = 0.0f;
		m_animation->m_duration = duration;
	}

	size_t AnimationClip::Writer::TrackCount()const { return m_animation->m_tracks.size(); }

	AnimationClip::Track* AnimationClip::Writer::GetTrack(size_t index)const { return m_animation->m_tracks[index]; }

	void AnimationClip::Writer::SwapTracks(size_t indexA, size_t indexB)const {
		std::vector<Reference<AnimationClip::Track>>& tracks = m_animation->m_tracks;
		Reference<AnimationClip::Track>& a = tracks[indexA];
		Reference<AnimationClip::Track>& b = tracks[indexB];
		std::swap(a, b);
		std::swap(a->m_index, b->m_index);
	}

	void AnimationClip::Writer::RemoveTrack(size_t index)const {
		if (TrackCount() <= index) return;
		size_t end = (TrackCount() - 1);
		std::vector<Reference<AnimationClip::Track>>& tracks = m_animation->m_tracks;
		while (index < end) {
			size_t next = (index + 1);
			tracks[index] = tracks[next];
			tracks[index]->m_index = index;
			index = next;
		}
		tracks.pop_back();
	}

	void AnimationClip::Writer::PopTrack()const { m_animation->m_tracks.pop_back(); }

	void AnimationClip::Writer::ClearTrackBindings(size_t trackId)const { m_animation->m_tracks[trackId]->m_bindChain.clear(); }

	void AnimationClip::Writer::SetTrackTargetField(size_t trackId, const std::string_view& targetField)const {
		m_animation->m_tracks[trackId]->m_targetField = targetField;
	}
}
