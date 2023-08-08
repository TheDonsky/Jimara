#pragma once
#include "../Types.h"
#include <Jimara/Components/Animation/Animator.h>

namespace Jimara {
	/// <summary>
	/// Animation states take their animation clip data from AnimationBlendStateProvider objects;
	/// This is an interface that provides a list of clip blend states to the user.
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimationBlendStateProvider : public virtual WeaklyReferenceable {
	public:
		/// <summary>
		/// Information about a clip blend state
		/// </summary>
		struct JIMARA_STATE_MACHINES_API ClipBlendState final {
			/// <summary> Animation clip to play </summary>
			Reference<AnimationClip> clip;

			/// <summary> 'Native' clip playback speed </summary>
			float playbackSpeed;

			/// <summary> Clip blending weight </summary>
			float weight;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="animation"> Animation clip to play </param>
			/// <param name="speed"> 'Native' clip playback speed </param>
			/// <param name="blendWeight"> Clip blending weight </param>
			inline ClipBlendState(AnimationClip* animation = nullptr, float speed = 1.0f, float blendWeight = 1.0f)
				: clip(animation), playbackSpeed(speed), weight(blendWeight) {}
		};

		/// <summary>
		/// Retrievs clip states
		/// </summary>
		/// <param name="reportClipState"> State of each clip will be reported through this callback </param>
		virtual void GetBlendState(Callback<ClipBlendState> reportClipState) = 0;
	};


	// Expose parent types
	template<> inline void TypeIdDetails::GetParentTypesOf<AnimationBlendStateProvider>(const Callback<TypeId>& report) {
		report(TypeId::Of<WeaklyReferenceable>());
	}
}
