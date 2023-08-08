#pragma once
#include "AnimationState.h"
#include <Jimara/Components/Level/RegistryReference.h>


namespace Jimara {
	/// <summary> Let the system know about the Component types </summary>
	JIMARA_REGISTER_TYPE(Jimara::AnimationStateFromRegistry);
	JIMARA_REGISTER_TYPE(Jimara::AnimationBlendStateFromRegistry);



#pragma warning(disable: 4250)
	/// <summary>
	/// Registry reference of an AnimationState
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimationStateFromRegistry
		: public virtual RegistryReference<InputProvider<Reference<AnimationState>>>
		, public virtual InputProvider<Reference<AnimationState>> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		AnimationStateFromRegistry(Component* parent, const std::string_view& name = "AnimationStateFromRegistry");

		/// <summary> Virtual destructor </summary>
		virtual ~AnimationStateFromRegistry();

		/// <summary> Virtual destructor </summary>
		virtual std::optional<Reference<AnimationState>> GetInput()override;
	};



	/// <summary>
	/// Registry reference of an AnimationBlendStateProvider
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimationBlendStateFromRegistry 
		: public virtual RegistryReference<AnimationBlendStateProvider>
		, public virtual AnimationBlendStateProvider {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		AnimationBlendStateFromRegistry(Component* parent, const std::string_view& name = "AnimationBlendStateFromRegistry");

		/// <summary> Virtual destructor </summary>
		virtual ~AnimationBlendStateFromRegistry();

		/// <summary>
		/// Retrievs clip states
		/// </summary>
		/// <param name="reportClipState"> State of each clip will be reported through this callback </param>
		virtual void GetBlendState(Callback<ClipBlendState> reportClipState) override;
	};
#pragma warning(default: 4250)



	// Expose parent types and attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<AnimationStateFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<RegistryReference<InputProvider<Reference<AnimationState>>>>());
		report(TypeId::Of<InputProvider<Reference<AnimationState>>>());
	}
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<AnimationStateFromRegistry>(const Callback<const Object*>& report);
	template<> inline void TypeIdDetails::GetParentTypesOf<AnimationBlendStateFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<RegistryReference<AnimationBlendStateProvider>>());
		report(TypeId::Of<AnimationBlendStateProvider>());
	}
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<AnimationBlendStateFromRegistry>(const Callback<const Object*>& report);
}

