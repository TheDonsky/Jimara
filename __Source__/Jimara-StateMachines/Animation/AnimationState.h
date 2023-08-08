#pragma once
#include "AnimationBlendStateProvider.h"
#include "AnimatorChannelBlock.h"
#include "../StateMachine.h"
#include <Jimara/Core/Systems/InputProvider.h>


namespace Jimara {
	/// <summary> Let the system know about the Component type </summary>
	JIMARA_REGISTER_TYPE(Jimara::AnimationState);

#pragma warning(disable: 4250)
	/// <summary>
	/// State machine state for animations
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimationState : public virtual Jimara::StateMachine::StateComponent {
	public:
		/// <summary>
		/// General transition to another animation state
		/// </summary>
		struct JIMARA_STATE_MACHINES_API Transition {
			/// <summary> State to transition to </summary>
			WeakReference<AnimationState> state;

			/// <summary> Animation fade duration </summary>
			float fadeTime = 0.1f;

			/// <summary> Minimal animation phase to start transition from  </summary>
			float exitTime = 0.0f;
		};

		/// <summary>
		/// Transition at the end of the clip playback
		/// <para/> (For non-looping states only)
		/// </summary>
		struct JIMARA_STATE_MACHINES_API EndTransition : Transition {
			/// <summary> Serializer for End transition </summary>
			struct Serializer;
		};

		/// <summary>
		/// Transition that happens if cetrain condition is met
		/// </summary>
		struct JIMARA_STATE_MACHINES_API ConditionalTransition : Transition {
			/// <summary> Condition for the transition (ignored if exit time requirenment is not met) </summary>
			WeakReference<InputProvider<bool>> condition;

			/// <summary> Serializer for Conditional transition </summary>
			struct Serializer;
		};



		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		AnimationState(Component* parent, const std::string_view& name = "AnimationState");

		/// <summary> Virtual destructor </summary>
		virtual ~AnimationState();

		/// <summary> Target animator (if not provided, animator will be automaitically found in parent hierarchy) </summary>
		Jimara::Animator* Animator()const;

		/// <summary>
		/// Sets target animator
		/// </summary>
		/// <param name="animator"> Target </param>
		void SetAnimator(Jimara::Animator* animator);

		/// <summary> Animation blend state provider </summary>
		Reference<AnimationBlendStateProvider> Animation()const;

		/// <summary>
		/// Sets blend state provider
		/// </summary>
		/// <param name="provider"> AnimationBlendStateProvider </param>
		void SetAnimation(AnimationBlendStateProvider* provider);

		/// <summary> If true, animation state will loop </summary>
		bool IsLooping()const;

		/// <summary> 
		/// Makes state loop or stop after playing once
		/// </summary>
		/// <param name="looping"> If true, state will loop </param>
		void SetLooping(bool looping);

		/// <summary> Number of conditional transitions </summary>
		size_t TransitionCount()const;

		/// <summary>
		/// Conditional transition by index
		/// </summary>
		/// <param name="index"> Index [0 - TransitionCount()) </param>
		/// <returns> Transition </returns>
		const ConditionalTransition& GetTransition(size_t index)const;

		/// <summary>
		/// Sets conditional transitioin settings
		/// </summary>
		/// <param name="index"> Transition index </param>
		/// <param name="transition"> Transition settings </param>
		void SetTransition(size_t index, const ConditionalTransition& transition);

		/// <summary>
		/// Adds conditional transition
		/// </summary>
		/// <param name="transition"> Transition settings </param>
		void AddTransition(const ConditionalTransition& transition);

		/// <summary>
		/// Removes conditional transition by index
		/// </summary>
		/// <param name="index"> Transition index </param>
		void RemoveTransition(size_t index);

		/// <summary>
		/// End transition
		/// <para/> Ignored if looping
		/// </summary>
		const EndTransition& GetEndTransition()const;

		/// <summary>
		/// Sets end transition settings
		/// </summary>
		/// <param name="transition"> Transition settings </param>
		void SetEndTransition(const EndTransition& transition);

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary>
		/// Invoked while the animation state is active
		/// </summary>
		/// <param name="context"> State machine context </param>
		virtual void UpdateState(Jimara::StateMachine::Context* context)override;

		/// <summary> Invoked when state is activated </summary>
		virtual void OnStateEnter()override;

		/// <summary> Invoked when state is reactivated </summary>
		virtual void OnStateReEnter()override;

		/// <summary> Invoked when state is deactivated </summary>
		virtual void OnStateExit()override;

	private:
		// Animation blend state provider
		WeakReference<AnimationBlendStateProvider> m_animation;

		// If true, state will loop animations
		bool m_looping = false;

		// List of conditional transitions
		std::vector<ConditionalTransition> m_conditionalTransitions;
		
		// End transition
		EndTransition m_endTransition;

		// Active transition state
		float m_totalFadeInTime = 0.0f;
		float m_fadeInTime = 0.0f;
		float m_totalFadeOutTime = 0.0f;
		float m_fadeOutTime = 0.0f;

		// Channel block, dedicated to this state
		AnimatorChannelBlock m_channelBlock;

		// Current stage update for UpdateState
		Callback<Jimara::StateMachine::Context*> m_updateFn = Callback<Jimara::StateMachine::Context*>(Jimara::Unused<Jimara::StateMachine::Context*>);

		// Current blend state
		float m_blendWeight = 0.0f;

		// Most of the implementation resides in here...
		struct Helpers;
	};
#pragma warning(default: 4250)





	/// <summary>
	/// Serializer for End transition
	/// </summary>
	struct JIMARA_STATE_MACHINES_API AnimationState::EndTransition::Serializer 
		: public virtual Jimara::Serialization::SerializerList::From<EndTransition> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Serializer name </param>
		/// <param name="hint"> Serialization hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
			: Jimara::Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Reports serialized fields
		/// </summary>
		/// <param name="recordElement"> Each field will be reported through this callback </param>
		/// <param name="target"> Target transition </param>
		virtual void GetFields(const Callback<Jimara::Serialization::SerializedObject>& recordElement, EndTransition* target)const override;
	};

	/// <summary>
	/// Serializer for Conditional transition
	/// </summary>
	struct JIMARA_STATE_MACHINES_API AnimationState::ConditionalTransition::Serializer 
		: public virtual Jimara::Serialization::SerializerList::From<ConditionalTransition> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Serializer name </param>
		/// <param name="hint"> Serialization hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
			: Jimara::Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Reports serialized fields
		/// </summary>
		/// <param name="recordElement"> Each field will be reported through this callback </param>
		/// <param name="target"> Target transition </param>
		virtual void GetFields(const Callback<Jimara::Serialization::SerializedObject>& recordElement, ConditionalTransition* target)const override;
	};





	// Expose parent types and attributes
	template<> inline void TypeIdDetails::GetParentTypesOf<Jimara::AnimationState>(const Callback<TypeId>& report) {
		report(TypeId::Of<StateMachine::StateComponent>());
	}
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<Jimara::AnimationState>(const Callback<const Object*>& report);
}
