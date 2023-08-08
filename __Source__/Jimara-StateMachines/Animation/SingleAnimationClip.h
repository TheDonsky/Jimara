#pragma once
#include "AnimationBlendStateProvider.h"
#include <Jimara/Core/Systems/InputProvider.h>


namespace Jimara {
	/// <summary> Let the system know about the Component type </summary>
	JIMARA_REGISTER_TYPE(Jimara::SingleAnimationClip);

#pragma warning(disable: 4250)
	/// <summary>
	/// AnimationBlendStateProvider that directly reports a single animation clip
	/// </summary>
	class JIMARA_STATE_MACHINES_API SingleAnimationClip : public virtual Component, public virtual AnimationBlendStateProvider {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		SingleAnimationClip(Component* parent, const std::string_view& name = "SingleAnimationClip");

		/// <summary> Virtual destructor </summary>
		virtual ~SingleAnimationClip();

		/// <summary> Animation clip </summary>
		inline AnimationClip* Clip()const { return m_clip; }

		/// <summary>
		/// Sets animation clip
		/// </summary>
		/// <param name="clip"> Clip to use </param>
		inline void SetClip(AnimationClip* clip) { m_clip = clip; }

		/// <summary> Playback speed input (1 if nullptr) </summary>
		inline Reference<InputProvider<float>> PlaybackSpeed()const { return m_playbackSpeed; }

		/// <summary>
		/// Sets playback speed provider
		/// </summary>
		/// <param name="speedProvider"> Speed provider </param>
		inline void SetPlaybackSpeed(InputProvider<float>* speedProvider) { m_playbackSpeed = speedProvider; }

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports clip
		/// </summary>
		/// <param name="reportClipState"> Clip will be reported through this callback </param>
		virtual void GetBlendState(Callback<AnimationBlendStateProvider::ClipBlendState> reportClipState)override;

	private:
		// Animation clip
		Reference<AnimationClip> m_clip;

		// Multiplier for playback speed
		WeakReference<InputProvider<float>> m_playbackSpeed;
	};
#pragma warning(default: 4250)



	// Expose parent types and attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<SingleAnimationClip>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<AnimationBlendStateProvider>());
	}
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<SingleAnimationClip>(const Callback<const Object*>& report);
}
