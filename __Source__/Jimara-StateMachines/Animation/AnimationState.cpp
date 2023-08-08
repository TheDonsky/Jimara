#include "AnimationState.h"
#include <Jimara/Components/Level/Subscene.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>


namespace Jimara {
	struct AnimationState::Helpers {
		static void ClearAnimatorOnDestroyed(AnimationState* self, Jimara::Component*) {
			Jimara::Animator* animator = self->m_channelBlock.Animator();
			if (animator != nullptr)
				animator->OnDestroyed() -= Callback<Jimara::Component*>(Helpers::ClearAnimatorOnDestroyed, self);
			self->m_channelBlock.Reset(nullptr);
		}

		static float UpdateChannels(AnimationState* self) {
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

			std::optional<float> phase;
			float baseChannelPlaybackSpeed = 0.0f;
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
				if (phase.has_value())
					channel.SetTime(clipDuration * (((baseChannelPlaybackSpeed * playbackSpeed) < 0.0f) ? (1.0f - phase.value()) : phase.value()));
				else if (Math::Min(std::abs(clipDuration), std::abs(playbackSpeed)) > std::numeric_limits<float>::epsilon() && channel.Playing()) {
					phase = Math::FloatRemainder(channel.Time() / clipDuration, 1.0f);
					baseChannelPlaybackSpeed = playbackSpeed;
				}
				somethingPlaying |= channel.Playing();
			}

			blendState.clear();
			return (phase.has_value() && somethingPlaying) ? phase.value() : std::numeric_limits<float>::infinity();
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

		static void Update(AnimationState* self, Jimara::StateMachine::Context* context) {
			const float phase = UpdateChannels(self);
			auto startTransition = [&](const Transition& transition, auto checkCondition) {
				if (phase < transition.exitTime)
					return false;

				if (!checkCondition())
					return false;

				Reference<AnimationState> nextState = transition.state;
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
				Reference<AnimationState> state = target->state;
				JIMARA_SERIALIZE_FIELD(state, "State", "If transition requirenments are met, state machine will move onto this state");
				target->state = state;
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
		Helpers::UpdateChannels(this);
		Helpers::RestartChannels(this);
	}

	void AnimationState::OnStateReEnter() {
		m_updateFn = Callback<Jimara::StateMachine::Context*>(Helpers::FadeIn, this);
	}

	void AnimationState::OnStateExit() {
		m_channelBlock.StopAllChannels();
		m_blendWeight = 0.0f;
		m_updateFn = Callback<Jimara::StateMachine::Context*>(Jimara::Unused<Jimara::StateMachine::Context*>);
		m_totalFadeInTime = m_fadeInTime = m_blendWeight = 0.0f;
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Jimara::AnimationState>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Jimara::AnimationState> serializer("Jimara/Animation/AnimationState", "AnimationState");
		report(&serializer);
	}
}
