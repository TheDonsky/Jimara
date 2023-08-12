#include "LinearAnimationBlend.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	LinearAnimationBlend::LinearAnimationBlend(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	LinearAnimationBlend::~LinearAnimationBlend() {}

	void LinearAnimationBlend::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD(m_clips, "Clips", "Clips to blend");
			JIMARA_SERIALIZE_WRAPPER(m_playbackSpeed, "Speed", "[Optional] Base playback speed multiplier (will be multiplied by Speed multiplier)");
			JIMARA_SERIALIZE_WRAPPER(m_value, "Value", "Input value for blending");
		};
	}

	void LinearAnimationBlend::ClipData::Serializer::GetFields(const Callback<Jimara::Serialization::SerializedObject>& recordElement, ClipData* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			JIMARA_SERIALIZE_WRAPPER(target->animation, "Animation", "Animation Blend State Provider");
			JIMARA_SERIALIZE_WRAPPER(target->playbackSpeedMultiplier, "Playback Speed", "Playback speed multiplier");
			JIMARA_SERIALIZE_FIELD(target->value, "Value", "Input value for which the blend state is fully set to this animation");
		};
	}

	void LinearAnimationBlend::GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState> reportClipState) {
#ifndef NDEBUG
		static thread_local std::vector<LinearAnimationBlend*> blendStateStack;
		for (size_t i = 0u; i < blendStateStack.size(); i++)
			if (blendStateStack[i] == this) {
				Context()->Log()->Error("LinearAnimationBlend::GetBlendState - Recursion detected! (would crash in release mode)");
				return;
			}
		blendStateStack.push_back(this);
#endif

		const float input = Jimara::InputProvider<float>::GetInput(m_value, 0.0f);
		static const constexpr size_t NO_ID = ~size_t(0);
		size_t lowerIndex = NO_ID;
		size_t upperIndex = NO_ID;
		for (size_t i = 0u; i < m_clips.size(); i++) {
			const float clipValue = m_clips[i].value;
			if (clipValue <= input && (lowerIndex == NO_ID || m_clips[lowerIndex].value < clipValue))
				lowerIndex = i;
			if (clipValue > input && (upperIndex == NO_ID || m_clips[upperIndex].value > clipValue))
				upperIndex = i;
		}

		const float basePlaybackSpeed =
			Jimara::InputProvider<float>::GetInput(m_playbackSpeed, 1.0f);
		const float upperContribution =
			(lowerIndex == NO_ID) ? 0.0f :
			(upperIndex == NO_ID) ? 1.0f :
			((input - m_clips[lowerIndex].value) / (m_clips[upperIndex].value - m_clips[lowerIndex].value));
		if (upperIndex == NO_ID)
			upperIndex = lowerIndex;
		if (lowerIndex == NO_ID)
			lowerIndex = upperIndex;

		for (size_t i = 0u; i < m_clips.size(); i++) {
			const ClipData& data = m_clips[i];
			Reference<AnimationBlendStateProvider> provider = data.animation;
			if (provider == nullptr)
				continue;

			const float playbackSpeedMultiplier =
				basePlaybackSpeed * Jimara::InputProvider<float>::GetInput(data.playbackSpeedMultiplier, 1.0f);
			const float weightMultiplier =
				(i == lowerIndex) ? ((lowerIndex == upperIndex) ? 1.0f : (1.0f - upperContribution)) :
				(i == upperIndex) ? upperContribution : 0.0f;

			auto processClip = [&](const AnimationBlendStateProvider::ClipBlendState& state) {
				reportClipState(AnimationBlendStateProvider::ClipBlendState(
					state.clip,
					state.playbackSpeed * playbackSpeedMultiplier,
					state.weight * weightMultiplier));
			};
			provider->GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState>::FromCall(&processClip));
		}


#ifndef NDEBUG
		assert(blendStateStack.back() == this);
		blendStateStack.pop_back();
#endif
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Jimara::LinearAnimationBlend>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<LinearAnimationBlend>(
			"Linear Animation Blend", "Jimara/Animation/LinearAnimationBlend", 
			"AnimationBlendStateProvider that blends between several other AnimationBlendStateProvider objects based on some floating point input");
		report(factory);
	}
}
