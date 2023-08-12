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
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<AnimationStateFromRegistry>(
			"AnimationState From Registry", "Jimara/Animation/StateFromRegistry",
			"Registry reference of an AnimationState");
		report(factory);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AnimationBlendStateFromRegistry>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<AnimationBlendStateFromRegistry>(
			"Animation Blend State FromRegistry", "Jimara/Animation/BlendStateFromRegistry",
			"Registry reference of an AnimationBlendStateProvider");
		report(factory);
	}
}
