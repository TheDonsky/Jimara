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

	void Animator::Bind() {
		if (m_dead || m_bound) return;
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
				{
					TypeId componentTypeId;
					if (!TypeId::Find(typeid(*animatedComponent), componentTypeId)) return;
					const ComponentSerializer* serializer = componentTypeId.FindAttributeOfType<ComponentSerializer>(true);
					if (serializer == nullptr) continue;
					Serialization::SerializedObject serialized = serializer->Serialize(animatedComponent);
					serialized.GetFields([&](Serialization::SerializedObject serializedField) {
						if (serializedField.Serializer() == nullptr || serializedObject.serializer != nullptr) return;
						else if (track->TargetField() == serializedField.Serializer()->TargetName()) {
							serializedObject.serializer = serializedField.Serializer();
							serializedObject.targetAddr = serializedField.TargetAddr();
						}
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
				if (binding.bindings.Size() <= 0)
					binding.update = GetFieldUpdater(serializedObject.serializer);
				binding.bindings.Push({ track, &clipState });
			}
		}
		m_bound = true;
	}

	namespace {
		inline static float ToFloat(float f) { return f; }
		inline static float ToFloat(const Vector3& v) { return Math::Magnitude(v); }
	}

	Callback<const Animator::SerializedField&, const Animator::FieldBinding&> Animator::GetFieldUpdater(const Serialization::ItemSerializer* serializer) {
		typedef void(*UpdateFn)(const Animator::SerializedField&, const Animator::FieldBinding&);
		UpdateFn updateFn = Unused<const Animator::SerializedField&, const Animator::FieldBinding&>;
		if (dynamic_cast<const Serialization::Vector3Serializer*>(serializer) != nullptr) {
			updateFn = 
				(serializer->FindAttributeOfType<Serialization::EulerAnglesAttribute>() != nullptr) ?
				(UpdateFn)[](const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				Vector3 value(0.0f);
				float totalWeight = 0.0f;
				for (size_t i = 0; i < binding.bindings.Size(); i++) {
					const TrackBinding& trackBinding = binding.bindings[i];
					auto addValue = [&](const auto* curve) -> bool {
						if (curve == nullptr) return false;
						totalWeight += trackBinding.state->weight;
						value = Math::LerpAngles(value, Vector3(curve->Value(trackBinding.state->time)), trackBinding.state->weight / totalWeight);
						return true;
					};
					if (addValue(dynamic_cast<const AnimationCurve<Vector3>*>(trackBinding.track))) continue;
					else if (addValue(dynamic_cast<const AnimationCurve<float>*>(trackBinding.track))) continue;
				}
				dynamic_cast<const Serialization::Vector3Serializer*>(field.serializer.operator->())->Set(value, field.targetAddr);
			} : (UpdateFn)[](const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				Vector3 value(0.0f);
				float totalWeight = 0.0f;
				for (size_t i = 0; i < binding.bindings.Size(); i++) {
					const TrackBinding& trackBinding = binding.bindings[i];
					auto addValue = [&](const auto* curve) -> bool {
						if (curve == nullptr) return false;
						value += curve->Value(trackBinding.state->time) * trackBinding.state->weight;
						totalWeight += trackBinding.state->weight;
						return true;
					};
					if (addValue(dynamic_cast<const AnimationCurve<Vector3>*>(trackBinding.track))) continue;
					else if (addValue(dynamic_cast<const AnimationCurve<float>*>(trackBinding.track))) continue;
				}
				if (totalWeight != 0.0f) value /= totalWeight;
				dynamic_cast<const Serialization::Vector3Serializer*>(field.serializer.operator->())->Set(value, field.targetAddr);
			};
		}
		else if (dynamic_cast<const Serialization::FloatSerializer*>(serializer) != nullptr) {
			updateFn = [](const Animator::SerializedField& field, const Animator::FieldBinding& binding) {
				float value = 0.0f;
				float totalWeight = 0.0f;
				for (size_t i = 0; i < binding.bindings.Size(); i++) {
					const TrackBinding& trackBinding = binding.bindings[i];
					auto addValue = [&](const auto* curve) -> bool {
						if (curve == nullptr) return false;
						value += ToFloat(curve->Value(trackBinding.state->time)) * trackBinding.state->weight;
						totalWeight += trackBinding.state->weight;
						return true;
					};
					if (addValue(dynamic_cast<const AnimationCurve<float>*>(trackBinding.track))) continue;
					else if (addValue(dynamic_cast<const AnimationCurve<Vector3>*>(trackBinding.track))) continue;
				}
				if (totalWeight != 0.0f) value /= totalWeight;
				dynamic_cast<const Serialization::FloatSerializer*>(field.serializer.operator->())->Set(value, field.targetAddr);
			};
		}
		return Callback(updateFn);
	}

	void Animator::OnAnimationClipDirty(const AnimationClip*) { Unbind(); }

	void Animator::OnTransformHeirarchyChanged(const Component*) { Unbind(); }
	
	void Animator::OnComponentDead(Component* component) {
		if (component == this)
			m_dead = true;
		Unbind();
	}
}
