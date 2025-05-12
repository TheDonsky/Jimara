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
	class JIMARA_STATE_MACHINES_API AnimationState 
		: public virtual StateMachine::StateComponent
		, public virtual InputProvider<Reference<AnimationState>> {
	public:
		/// <summary>
		/// General transition to another animation state
		/// </summary>
		struct JIMARA_STATE_MACHINES_API Transition {
			/// <summary> State to transition to </summary>
			WeakReference<InputProvider<Reference<AnimationState>>> state;

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
		/// An arbitrary event that can be fired as a part of the animation state
		/// </summary>
		class JIMARA_STATE_MACHINES_API AnimationEvent;



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


		/// <summary> Number of animation-events, associated with this AnimationState </summary>
		size_t AnimationEventCount()const;

		/// <summary>
		/// Animation event by index
		/// </summary>
		/// <param name="index"> Animation event index [0 - AnimationEventCount()) </param>
		/// <returns> Index'th animation event </returns>
		const AnimationEvent& GetAnimationEvent(size_t index)const;

		/// <summary>
		/// Sets animation event by index
		/// </summary>
		/// <param name="index"> Event index </param>
		/// <param name="event"> Event to use </param>
		void SetAnimationEvent(size_t index, const AnimationEvent& event);

		/// <summary>
		/// Adds animation event to the state
		/// </summary>
		/// <param name="event"> AnimationEvent to add </param>
		void AddAnimationEvent(const AnimationEvent& event);

		/// <summary>
		/// Removes animation event by index
		/// </summary>
		/// <param name="index"> Index of the event to remove </param>
		void RemoveAnimationEvent(size_t index);


		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

		/// <summary> Returns self </summary>
		inline virtual std::optional<Reference<AnimationState>> GetInput()override { return this; }

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

		// AnimationEvent management
		std::atomic_bool m_animationEventsDirty = true;
		std::vector<AnimationEvent> m_animationEvents;
		std::vector<size_t> m_animationEventOrder;
		float m_lastPhase = 0.0f;

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





#pragma warning(disable: 4275)
	/// <summary>
	/// An arbitrary event that can be fired as a part of the animation state
	/// </summary>
	class JIMARA_STATE_MACHINES_API AnimationState::AnimationEvent 
		: public virtual Serialization::SerializedCallback::ProvidedInstance {
	public:
		inline AnimationEvent() {}

		inline virtual ~AnimationEvent() {}

		inline AnimationEvent(const AnimationEvent& event);

		inline AnimationEvent& operator=(const AnimationEvent& event);

		inline AnimationEvent(AnimationEvent&& event)noexcept;

		inline AnimationEvent& operator=(AnimationEvent&& event)noexcept;

		/// <summary> 
		/// Animation state phase at which the event is fired 
		/// <para/> Values less than 0 and greater than 1 will effectively mean 'never', unless the animation state exits and REQUIRE_BEFORE_EXIT flag is set.
		/// </summary>
		inline float Phase()const { return m_phase; }

		/// <summary>
		/// Sets animation-state phase
		/// </summary>
		/// <param name="phase"> Phase to use </param>
		inline void SetPhase(float phase) { if (m_phase != phase) { m_phase = phase; m_dirty.SetDirty(); } }

		/// <summary>
		/// Exposes fields
		/// </summary>
		/// <param name="recordElement"> Fields will be exposed through this callback </param>
		virtual void GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement)override;

	private:
		float m_phase = 0.0f;

		// Revision pointer will be externally set by the AnimationState and it'll guarantee that the pointer remains fully controlled by the 'owner-state' and nothing else..
		struct DirtyPtr {
			std::atomic_bool* dirty = nullptr;
			inline void SetDirty() { if (dirty != nullptr) dirty->store(true); }
			inline DirtyPtr() { }
			inline ~DirtyPtr() { SetDirty(); }
			inline DirtyPtr(const DirtyPtr&) { }
			inline DirtyPtr& operator=(const DirtyPtr&) { SetDirty(); return (*this); }
			inline DirtyPtr(DirtyPtr&&)noexcept { }
			inline DirtyPtr& operator=(DirtyPtr&&)noexcept { SetDirty(); return (*this); }
		};
		DirtyPtr m_dirty = {};
		friend class AnimationState;
	};
#pragma warning(default: 4275)





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
