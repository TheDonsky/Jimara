#pragma once
#include "../Types.h"
#include <Jimara/Components/Animation/Animator.h>

namespace Jimara {
	/// <summary>
	/// Standard Animator Component API provides a way to manipulate animation blend&playback state using channels;
	/// More often than not, we would prefer to group channels for one reason or another like in case of the AnimationStates and their blend trees.
	/// AnimatorChannelBlock can be condidered as a 'sub-allocator' of channel id-s for any Animator.
	/// <para/> Notes:
	/// <para/>		0. Within this package AnimatorChannelBlock will primarily be used by AnimationStates, 
	///				but feel free to use it for anything else if you like it;
	/// <para/>		1. AnimatorChannelBlock is not aware of any other types of allocators and will start allocating 
	///				channel indices from 0 to however many are required by all the AnimatorChannelBlock instances for given Animator;
	///				Because of this, it is adviced not to have any channels created and managed externally (or from the Editor application)
	///				if the channel blocks are used.
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimatorChannelBlock final {
	public:
		/// <summary> Constructor </summary>
		AnimatorChannelBlock();

		/// <summary> Destructor </summary>
		~AnimatorChannelBlock();

		/// <summary> Animator, this AnimatorChannelBlock sub-allocates channels from </summary>
		Jimara::Animator* Animator()const;

		/// <summary>
		/// Frees all channels and sets target animator;
		/// <para/> Note: Channels will be cleaned even if the animator does not change
		/// </summary>
		/// <param name="animator"> Target animator to sub-allocate channels from </param>
		void Reset(Jimara::Animator* animator);

		/// <summary> Number of allocated channels for this block </summary>
		size_t ChannelCount()const;
		
		/// <summary>
		/// Sets allocated channel count
		/// <para/> Note: Values less than ChannelCount() will free extra channels, while greater values will allocate more
		/// </summary>
		/// <param name="count"> Number of channels for this block </param>
		void SetChannelCount(size_t count);

		/// <summary>
		/// Gives access to the sub-allocated channel by index
		/// <para/> Note: If channel is greater than or equal to ChannelCount(), new channels will be allocated automatically
		/// </summary>
		/// <param name="channel"> Channel index </param>
		/// <returns> Animator channel </returns>
		Jimara::Animator::AnimationChannel operator[](size_t channel);

		/// <summary>
		/// Stops all playing channels within the sub-allocation
		/// <para/> Note: This is preferable to iterating over all channels, because it also makes sure that if Animator does not have 
		///			corresponding channels allocated already, they will not be created because of this call.
		/// </summary>
		void StopAllChannels();

	private:
		// Shared object per Animator
		Reference<Object> m_allocator;

		// Target animator
		Reference<Jimara::Animator> m_animator;

		// Allocated indices
		Jimara::Stacktor<size_t, 4u> m_indirectionTable;

		// Private stuff is in here...
		struct Helpers;
	};
}
