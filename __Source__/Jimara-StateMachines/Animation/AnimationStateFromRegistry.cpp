#include "AnimationStatesFromRegistry.h"


namespace Jimara {
	AnimationStateFromRegistry::AnimationStateFromRegistry(Component* parent, const std::string_view& name)
		: Component(parent, name) {}

	AnimationStateFromRegistry::~AnimationStateFromRegistry() {}

	std::optional<Reference<AnimationState>> AnimationStateFromRegistry::GetInput() {
		Reference<InputProvider<Reference<AnimationState>>> state = StoredObject();
		if (state == nullptr)
			return std::optional<Reference<AnimationState>>();
		else return state->GetInput();
	}

	AnimationBlendStateFromRegistry::AnimationBlendStateFromRegistry(Component* parent, const std::string_view& name)
		: Component(parent, name) {}

	AnimationBlendStateFromRegistry::~AnimationBlendStateFromRegistry() {}

	void AnimationBlendStateFromRegistry::GetBlendState(Callback<ClipBlendState> reportClipState) {
		Reference<AnimationBlendStateProvider> provider = StoredObject();
		if (provider != nullptr)
			provider->GetBlendState(reportClipState);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AnimationStateFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<AnimationStateFromRegistry> serializer(
			"Jimara/Animation/StateFromRegistry", "AnimationState From Registry");
		report(&serializer);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AnimationBlendStateFromRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<AnimationBlendStateFromRegistry> serializer(
			"Jimara/Animation/BlendStateFromRegistry", "AnimationBlendStateProvider From Registry");
		report(&serializer);
	}
}
