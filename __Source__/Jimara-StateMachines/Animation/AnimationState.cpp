#include "AnimationState.h"
#include <Jimara/Components/Level/Subscene.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>
#include <Jimara/Math/BinarySearch.h>


namespace Jimara {
	struct AnimationState::Helpers {
		static void ClearAnimatorOnDestroyed(AnimationState* self, Jimara::Component*) {
			Jimara::Animator* animator = self->m_channelBlock.Animator();
			if (animator != nullptr)
				animator->OnDestroyed() -= Callback<Jimara::Component*>(Helpers::ClearAnimatorOnDestroyed, self);
			self->m_channelBlock.Reset(nullptr);
		}

		struct PhaseInfo {
			float phase;
			float speed;
		};

		static PhaseInfo UpdateChannels(AnimationState* self) {
			std::vector<AnimationBlendStateProvider::ClipBlendState>& blendState = [&]() 
				-> std::vector<AnimationBlendStateProvider::ClipBlendState>& {
				static thread_local std::vector<AnimationBlendStateProvider::ClipBlendState> states;
				assert(states.size() == 0u);
				auto addClip = [&](const AnimationBlendStateProvider::ClipBlendState& state) {
					states.push_back(state);
				};
				Reference<AnimationBlendStateProvider> provider = self->Animation();
				if (provider != nullptr)
					provider->GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState>::FromCall(&addClip));
				return states;
			}();

			self->m_channelBlock.SetChannelCount(blendState.size());

			const float blendStateDuration = [&]() {
				float totalWeight = 0.0f;
				float averageDuration = 0.0f;
				const AnimationBlendStateProvider::ClipBlendState* const end = blendState.data() + self->m_channelBlock.ChannelCount();
				for (const AnimationBlendStateProvider::ClipBlendState* ptr = blendState.data(); ptr < end; ptr++) {
					if (ptr->weight <= 0.0f || ptr->clip == nullptr)
						continue;
					totalWeight += ptr->weight;
					averageDuration = Math::Lerp(averageDuration, std::abs(ptr->clip->Duration() / ptr->playbackSpeed), ptr->weight / totalWeight);
				}
				return averageDuration;
			}();

			float phase = std::numeric_limits<float>::infinity();
			float baseChannelPlaybackSpeed = 0.0f;
			float baseChannelWeight = -std::numeric_limits<float>::infinity();

			bool somethingPlaying = false;
			for (size_t i = 0u; i < self->m_channelBlock.ChannelCount(); i++) {
				const AnimationBlendStateProvider::ClipBlendState& state = blendState[i];
				Jimara::Animator::AnimationChannel channel = self->m_channelBlock[i];
				const float clipDuration = (state.clip == nullptr) ? 0.0f : state.clip->Duration();
				const float playbackSpeed =
					(std::abs(blendStateDuration) > std::numeric_limits<float>::epsilon()) ? (clipDuration / blendStateDuration) :
					(blendStateDuration >= 0.0f) ? std::numeric_limits<float>::infinity() : -std::numeric_limits<float>::infinity();
				channel.SetClip(state.clip);
				channel.SetLooping(self->m_looping);
				channel.SetBlendWeight(Math::Max(state.weight * self->m_blendWeight, 0.0f));
				channel.SetSpeed(playbackSpeed);
				if (baseChannelWeight < state.weight &&
					Math::Min(std::abs(clipDuration), std::abs(playbackSpeed)) > std::numeric_limits<float>::epsilon() &&
					channel.Playing()) {
					phase = Math::FloatRemainder(channel.Time() / clipDuration, 1.0f);
					baseChannelPlaybackSpeed = playbackSpeed;
					baseChannelWeight = state.weight;
				}
				somethingPlaying |= channel.Playing();
			}

			if (baseChannelWeight > 0.0f)
				for (size_t i = 0u; i < self->m_channelBlock.ChannelCount(); i++) {
					const AnimationBlendStateProvider::ClipBlendState& state = blendState[i];
					if (state.weight >= baseChannelWeight)
						continue;
					Jimara::Animator::AnimationChannel channel = self->m_channelBlock[i];
					const float clipDuration = (state.clip == nullptr) ? 0.0f : state.clip->Duration();
					const float playbackSpeed = channel.Speed();
					channel.SetTime(clipDuration * (((baseChannelPlaybackSpeed * playbackSpeed) < 0.0f) ? (1.0f - phase) : phase));
				}

			blendState.clear();
			return PhaseInfo{ phase, baseChannelPlaybackSpeed };
		}

		static void FireEvents(AnimationState* self, float startPhase, float endPhase, float direction) {
			if (self->m_animationEventsDirty.load() || self->m_animationEvents.size() != self->m_animationEventOrder.size()) {
				self->m_animationEventOrder.resize(self->m_animationEvents.size());
				for (size_t i = 0u; i < self->m_animationEventOrder.size(); i++)
					self->m_animationEventOrder[i] = i;
				std::sort(self->m_animationEventOrder.begin(), self->m_animationEventOrder.end(), [&](size_t a, size_t b) {
					return self->m_animationEvents[a].Phase() < self->m_animationEvents[b].Phase();
					});
				self->m_animationEventsDirty.store(false);
			}
			if (self->m_animationEventOrder.size() <= 0u)
				return;
			auto findPoint = [&](float phase) {
				size_t pnt = BinarySearch_LE(self->m_animationEventOrder.size(), [&](size_t index) {
					return self->m_animationEvents[self->m_animationEventOrder[index]].Phase() > phase;
					});
				if (pnt >= self->m_animationEventOrder.size())
					pnt = 0u;
				return pnt;
				};
			size_t startIndex = findPoint(startPhase);
			size_t endIndex = findPoint(endPhase);
			size_t dir = (direction >= 0.0f) ? 1u : (self->m_animationEventOrder.size() - 1u);
			if (direction < 0) {
				std::swap(startIndex, endIndex);
				startIndex = (startIndex + 1u) % self->m_animationEventOrder.size();
				endIndex = (endIndex + 1u) % self->m_animationEventOrder.size();
			}
			if (startPhase > endPhase)
				std::swap(startPhase, endPhase);
			size_t i = startIndex;
			while (true) {
				const auto& event = self->m_animationEvents[self->m_animationEventOrder[i]];
				if (event.Phase() >= startPhase && event.Phase() < endPhase)
					event.Invoke();
				if (i == endIndex)
					break;
				else i = (i + dir) % self->m_animationEventOrder.size();
			}
		}

		static void RestartChannels(AnimationState* self) {
			for (size_t i = 0u; i < self->m_channelBlock.ChannelCount(); i++) {
				Jimara::Animator::AnimationChannel channel = self->m_channelBlock[i];
				channel.Stop();
				channel.Play();
			}
		}

		static void FadeIn(AnimationState* self, Jimara::StateMachine::Context* context) {
			self->m_fadeInTime -= self->Context()->Time()->ScaledDeltaTime();
			if (self->m_fadeInTime <= 0.0f) {
				self->m_blendWeight = 1.0f;
				self->m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::Update, self);
			}
			else self->m_blendWeight = Math::Max(1.0f - (self->m_fadeInTime / self->m_totalFadeInTime), self->m_blendWeight);
			Update(self, context);
		}

		static void PerformTransitions(AnimationState* self, Jimara::StateMachine::Context* context, float phase) {
			auto startTransition = [&](const Transition& transition, auto checkCondition) {
				if (phase < transition.exitTime)
					return false;

				if (!checkCondition())
					return false;

				Reference<AnimationState> nextState = InputProvider<Reference<AnimationState>>::GetInput(transition.state, nullptr);
				if (nextState == self) {
					// No transition necessary; Just phase reset...
					RestartChannels(self);
					return true;
				}

				self->m_totalFadeOutTime = self->m_fadeOutTime = transition.fadeTime;

				self->m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::FadeOut, self);
				if (nextState != nullptr) {
					nextState->m_totalFadeInTime = nextState->m_fadeInTime = transition.fadeTime;
					nextState->m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::FadeIn, nextState.operator->());
					context->AddState(nextState);
				}

				return true;
				};
			for (size_t i = 0u; i < self->m_conditionalTransitions.size(); i++) {
				if (startTransition(self->m_conditionalTransitions[i], [&]() {
					return Jimara::InputProvider<bool>::GetInput(self->m_conditionalTransitions[i].condition, false);
					}))
					return;
			}
			if (!self->IsLooping())
				startTransition(self->m_endTransition, [&]() { return true; });
		}

		static void Update(AnimationState* self, Jimara::StateMachine::Context* context) {
			const auto phase = UpdateChannels(self);
			FireEvents(self, self->m_lastPhase, phase.phase, phase.speed);
			self->m_lastPhase = phase.phase;
			PerformTransitions(self, context, phase.phase);
		}

		static void FadeOut(AnimationState* self, Jimara::StateMachine::Context* context) {
			// Transition has already happened...
			self->m_fadeOutTime -= self->Context()->Time()->ScaledDeltaTime();
			if (self->m_fadeOutTime <= 0.0f) {
				self->m_blendWeight = 0.0f;
				context->RemoveState(self);
			}
			else self->m_blendWeight = Math::Min(self->m_blendWeight, self->m_fadeOutTime / self->m_totalFadeOutTime);
			UpdateChannels(self);
		}

		static void GetCommonTransitionFields(const Callback<Jimara::Serialization::SerializedObject>& recordElement, Transition* target) {
			JIMARA_SERIALIZE_FIELDS(target, recordElement) {
				JIMARA_SERIALIZE_WRAPPER(target->state, "State", "If transition requirenments are met, state machine will move onto this state");
				JIMARA_SERIALIZE_FIELD(target->fadeTime, "Fade time", "State fade duration");
				JIMARA_SERIALIZE_FIELD(target->exitTime, "Exit time", "Minimal animation phase before the transition starts",
					Object::Instantiate<Jimara::Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				target->exitTime = Math::Min(Math::Max(0.0f, target->exitTime), 1.0f);
			};
		}
	};

	AnimationState::AnimationState(Component* parent, const std::string_view& name)
		: Component(parent, name) {
		OnDestroyed() += Callback<Jimara::Component*>(Helpers::ClearAnimatorOnDestroyed, this);
	}

	AnimationState::~AnimationState() {
		OnDestroyed() -= Callback<Jimara::Component*>(Helpers::ClearAnimatorOnDestroyed, this);
	}

	Jimara::Animator* AnimationState::Animator()const {
		return m_channelBlock.Animator();
	}

	void AnimationState::SetAnimator(Jimara::Animator* animator) {
		if (animator == m_channelBlock.Animator())
			return;
		Helpers::ClearAnimatorOnDestroyed(this, nullptr);
		if (animator != nullptr) {
			m_channelBlock.Reset(animator);
			animator->OnDestroyed() += Callback<Jimara::Component*>(Helpers::ClearAnimatorOnDestroyed, this);
		}
	}

	Reference<AnimationBlendStateProvider> AnimationState::Animation()const {
		return m_animation;
	}

	void AnimationState::SetAnimation(AnimationBlendStateProvider* provider) {
		m_animation = provider;
	}

	bool AnimationState::IsLooping()const {
		return m_looping;
	}

	void AnimationState::SetLooping(bool looping) {
		m_looping = looping;
	}

	size_t AnimationState::TransitionCount()const {
		return m_conditionalTransitions.size();
	}

	const AnimationState::ConditionalTransition& AnimationState::GetTransition(size_t index)const {
#ifndef NDEBUG
		assert(index < m_conditionalTransitions.size());
#endif // NDEBUG
		return m_conditionalTransitions[index];
	}

	void AnimationState::SetTransition(size_t index, const ConditionalTransition& transition) {
#ifndef NDEBUG
		assert(index < m_conditionalTransitions.size());
#endif // NDEBUG
		ConditionalTransition& t = m_conditionalTransitions[index];
		t = transition;
		t.exitTime = Math::Min(Math::Max(0.0f, t.exitTime), 1.0f);
	}

	void AnimationState::AddTransition(const ConditionalTransition& transition) {
		m_conditionalTransitions.push_back(transition);
		ConditionalTransition& t = m_conditionalTransitions.back();
		t.exitTime = Math::Min(Math::Max(0.0f, t.exitTime), 1.0f);
	}

	void AnimationState::RemoveTransition(size_t index) {
		if (index < m_conditionalTransitions.size())
			m_conditionalTransitions.erase(m_conditionalTransitions.begin() + index);
	}

	const AnimationState::EndTransition& AnimationState::GetEndTransition()const {
		return m_endTransition;
	}

	void AnimationState::SetEndTransition(const EndTransition& transition) {
		m_endTransition = transition;
		m_endTransition.exitTime = Math::Min(Math::Max(0.0f, m_endTransition.exitTime), 1.0f);
	}

	size_t AnimationState::AnimationEventCount()const {
		return m_animationEvents.size();
	}

	const AnimationState::AnimationEvent& AnimationState::GetAnimationEvent(size_t index)const {
		return m_animationEvents[index];
	}

	void AnimationState::SetAnimationEvent(size_t index, const AnimationEvent& event) {
		if (index >= m_animationEvents.size())
			return;
		assert(m_animationEvents[index].m_dirty.dirty == (&m_animationEventsDirty));
		m_animationEvents[index] = event;
		assert(m_animationEvents[index].m_dirty.dirty == (&m_animationEventsDirty));
		assert(m_animationEventsDirty.load());
	}

	void AnimationState::AddAnimationEvent(const AnimationEvent& event) {
		AnimationEvent& addedEvent = m_animationEvents.emplace_back(event);
		addedEvent.m_dirty.dirty = (&m_animationEventsDirty);
		m_animationEventsDirty.store(true);
	}

	void AnimationState::RemoveAnimationEvent(size_t index) {
		if (index >= m_animationEvents.size())
			return;
		for (size_t i = index; i < (m_animationEvents.size() - 1u); i++)
			m_animationEvents[i] = std::move(m_animationEvents[i + 1u]);
		m_animationEvents.pop_back();
		m_animationEventsDirty.store(true);
#ifndef NDEBUG
		for (size_t i = index; i < m_animationEvents.size(); i++)
			assert(m_animationEvents[i].m_dirty.dirty == (&m_animationEventsDirty));
#endif
	}

	inline AnimationState::AnimationEvent::AnimationEvent(const AnimationEvent& event) 
		: Serialization::SerializedCallback::ProvidedInstance(event) {
		SetPhase(event.Phase());
	}

	inline AnimationState::AnimationEvent& AnimationState::AnimationEvent::operator=(const AnimationEvent& event) {
		Serialization::SerializedCallback::ProvidedInstance& self = (*this);
		const Serialization::SerializedCallback::ProvidedInstance& other = event;
		self = other;
		SetPhase(event.Phase());
		return *this;
	}

	inline AnimationState::AnimationEvent::AnimationEvent(AnimationEvent&& event) noexcept
		: Serialization::SerializedCallback::ProvidedInstance(std::move(event)) {
		SetPhase(event.Phase());
	}

	inline AnimationState::AnimationEvent& AnimationState::AnimationEvent::operator=(AnimationEvent&& event) noexcept {
		Serialization::SerializedCallback::ProvidedInstance& self = (*this);
		const Serialization::SerializedCallback::ProvidedInstance& other = std::move(event);
		self = other;
		SetPhase(event.Phase());
		return *this;
	}

	void AnimationState::AnimationEvent::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Serialization::SerializedCallback::ProvidedInstance::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Phase, SetPhase, "Phase",
				"Animation state phase at which the event is fired\n"
				"Values less than 0 and greater than 1 will effectively mean 'never', unless the animation state exits and REQUIRE_BEFORE_EXIT flag is set.",
				Object::Instantiate<Jimara::Serialization::SliderAttribute<float>>(0.0f, 1.0f));
			//JIMARA_SERIALIZE_FIELD(m_flags, "Flags", "Flags for the animation event",
			//	Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<AnimationEventFlags>>>(true,
			//		"REQUIRE_BEFORE_EXIT", AnimationEventFlags::REQUIRE_BEFORE_EXIT));
		};
	}


	void AnimationState::EndTransition::Serializer::GetFields(
		const Callback<Jimara::Serialization::SerializedObject>& recordElement, EndTransition* target)const {
		Helpers::GetCommonTransitionFields(recordElement, target);
	}

	void AnimationState::ConditionalTransition::Serializer::GetFields(
		const Callback<Jimara::Serialization::SerializedObject>& recordElement, ConditionalTransition* target)const {
		Helpers::GetCommonTransitionFields(recordElement, target);
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			Reference<Jimara::InputProvider<bool>> condition = target->condition;
			JIMARA_SERIALIZE_FIELD(condition, "Condition", "Transition will happen if animation phase is no less than exit time and condition is satisfied");
			target->condition = condition;
		};
	}

	void AnimationState::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Animator, SetAnimator, "Animator", 
				"Target animator (Optional; if not present, will automatically be found when the state is entered)");
			JIMARA_SERIALIZE_WRAPPER(m_animation, "Animation", "Animation blend state provider");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsLooping, SetLooping, "Loop", "If true, animation will loop");
			JIMARA_SERIALIZE_FIELD(m_conditionalTransitions, "Transitions", "Conditional transitions");
			if (!IsLooping())
				JIMARA_SERIALIZE_FIELD(m_endTransition, "End transition", "Next state to transition to after the animation ends");
			{
				size_t initialEventCount = AnimationEventCount();
				JIMARA_SERIALIZE_FIELD(m_animationEvents, "Animation Events", "Events, triggered at certain phases of the animation");
				if (initialEventCount != AnimationEventCount())
					m_animationEventsDirty.store(true);
			}
		};
	}

	void AnimationState::UpdateState(Jimara::StateMachine::Context* context) {
		m_updateFn(context);
	}

	void AnimationState::OnStateEnter() {
		if (Animator() == nullptr) {
			Jimara::Animator* animator = nullptr;
			Component* base = this;
			while (base != nullptr) {
				animator = GetComponentInParents<Jimara::Animator>();
				if (animator == nullptr)
					base = Jimara::Subscene::GetSubscene(this);
				else break;
			}
			if (animator != nullptr)
				SetAnimator(animator);
		}
		m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::FadeIn, this);
		m_blendWeight = 0.0f;
		m_lastPhase = 0.0f;
		Helpers::UpdateChannels(this);
		Helpers::RestartChannels(this);
	}

	void AnimationState::OnStateReEnter() {
		m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::FadeIn, this);
	}

	void AnimationState::OnStateExit() {
		m_channelBlock.StopAllChannels();
		m_blendWeight = 0.0f;
		m_lastPhase = 0.0f;
		m_updateFn = Callback<Jimara::StateMachine::Context*>(Jimara::Unused<Jimara::StateMachine::Context*>);
		m_totalFadeInTime = m_fadeInTime = m_blendWeight = 0.0f;
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<AnimationState>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<AnimationState>(
			"Animation State", "Jimara/Animation/AnimationState",
			"State machine state for animation blending and transitions");
		report(factory);
	}
}
