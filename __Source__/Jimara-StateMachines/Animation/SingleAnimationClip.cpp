#include "SingleAnimationClip.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	SingleAnimationClip::SingleAnimationClip(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	SingleAnimationClip::~SingleAnimationClip() {}

	void SingleAnimationClip::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Jimara::Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD(m_clip, "Clip", "Animation Clip");
			JIMARA_SERIALIZE_WRAPPER(m_playbackSpeed, "Playback Speed", "[Optional] Playback speed provider (1 if nullptr)");
		};
	}

	void SingleAnimationClip::GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState> reportClipState) {
		reportClipState(AnimationBlendStateProvider::ClipBlendState(m_clip,
			Jimara::InputProvider<float>::GetInput(m_playbackSpeed, 1.0f), 
			1.0f));
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SingleAnimationClip>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SingleAnimationClip>(
			"Single Animation Clip", "Jimara/Animation/SingleAnimationClip", "AnimationBlendStateProvider that directly reports a single animation clip");
		report(factory);
	}
}
