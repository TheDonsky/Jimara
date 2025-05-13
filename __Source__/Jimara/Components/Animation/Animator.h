#pragma once
#include "../Physics/Rigidbody.h"
#include "../../Data/Animation.h"
#include "../../Core/Helpers.h"


namespace Jimara {
	/// <summary> This will make sure, Animator is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Animator);

	/// <summary>
	/// Component, responsible for AnimationClip playback
	/// </summary>
	class JIMARA_API Animator : public virtual Scene::LogicContext::UpdatingComponent {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		Animator(Component* parent, const std::string_view& name = "Animator");

		/// <summary> Virtual destructor </summary>
		virtual ~Animator();

		/// <summary>
		/// Animator blends animations from multiple channels that are running simultaneously.
		/// This thin wrapper provides API for manipulating playback state on the channel.
		/// </summary>
		class JIMARA_API AnimationChannel {
		public:
			/// <summary> Channel index </summary>
			size_t Index()const;

			/// <summary> Clip, tied to this track </summary>
			AnimationClip* Clip()const;

			/// <summary>
			/// Swaps-out animation clip on the channel
			/// </summary>
			/// <param name="clip"> Clip to play </param>
			void SetClip(AnimationClip* clip);

			/// <summary> Animation time </summary>
			float Time()const;

			/// <summary>
			/// Sets animation clip time
			/// </summary>
			/// <param name="time"> Animation time (will automatically fit in [0 - Clip()->Duration()] range) </param>
			void SetTime(float time);

			/// <summary> Blending weight </summary>
			float BlendWeight()const;

			/// <summary>
			/// Sets blend weight
			/// </summary>
			/// <param name="weight"> Blending weight to use (can not be negative; 0 auto-disables playback) </param>
			void SetBlendWeight(float weight);

			/// <summary> Channel playback speed multiplier </summary>
			float Speed()const;

			/// <summary>
			/// Sets channel playback speed multiplier
			/// </summary>
			/// <param name="speed"> Speed multiplier </param>
			void SetSpeed(float speed);

			/// <summary> If true, animation will loop around </summary>
			bool Looping()const;

			/// <summary>
			/// Sets looping flag
			/// </summary>
			/// <param name="loop"> If true, playback will loop animation </param>
			void SetLooping(bool loop);

			/// <summary> True, if a track is playing on the layer </summary>
			bool Playing()const;

			/// <summary> "Activates" playback on the channel </summary>
			void Play();

			/// <summary> Stops and resets channel playback </summary>
			void Stop();

			/// <summary> Stops playback, but preserves playback state </summary>
			void Pause();

		private:
			// Animator
			const Reference<Animator> m_animator;
			
			// Clip index
			const size_t m_index;

			// Only animator can create an instance
			friend class Animator;
			inline AnimationChannel(Animator* animator, size_t index) : m_animator(animator), m_index(index) {}
		};

		/// <summary> Number of channels to blend </summary>
		size_t ChannelCount()const;

		/// <summary>
		/// Sets simultaneous channel count
		/// </summary>
		/// <param name="count"> Number of channels </param>
		void SetChannelCount(size_t count);

		/// <summary>
		/// Gives access to channel state
		/// </summary>
		/// <param name="index"> Channel index (ChannelCount or higher will result in automatic channel addition) </param>
		/// <returns> Channel </returns>
		AnimationChannel Channel(size_t index);
		
		/// <summary> Checks if any clip is currently playing back </summary>
		bool Playing()const;

		/// <summary> 'Global' animator speed </summary>
		float PlaybackSpeed()const;

		/// <summary>
		/// Sets 'global' animator speed
		/// </summary>
		/// <param name="speed"> Animator speed </param>
		void SetPlaybackSpeed(float speed);

		/// <summary> Root motion flags </summary>
		enum class JIMARA_API RootMotionFlags : uint32_t {
			/// <summary> Nothing; Root bone not moving/rotating and motion not applied to Rigidbody/Transform </summary>
			NONE = 0u,

			/// <summary> Causes Rigidbody/Transform to move on X axis (Body/Transform may be effected slightly differently) </summary>
			MOVE_X = (1u << 0u),

			/// <summary> Causes Rigidbody/Transform to move on Y axis (Body/Transform may be effected slightly differently) </summary>
			MOVE_Y = (1u << 1u),

			/// <summary> Causes Rigidbody/Transform to move on Z axis (Body/Transform may be effected slightly differently) </summary>
			MOVE_Z = (1u << 2u),

			/// <summary> Causes Rigidbody/Transform to rotate around X axis (Body/Transform may be effected slightly differently) </summary>
			ROTATE_X = (1u << 3u),

			/// <summary> Causes Rigidbody/Transform to rotate around Y axis (Body/Transform may be effected slightly differently) </summary>
			ROTATE_Y = (1u << 4u),

			/// <summary> Causes Rigidbody/Transform to rotate around Z axis (Body/Transform may be effected slightly differently) </summary>
			ROTATE_Z = (1u << 5u),
			
			/// <summary> Keeps root bone movement on X axis </summary>
			ANIMATE_BONE_POS_X = (1u << 6u),

			/// <summary> Keeps root bone movement on Y axis </summary>
			ANIMATE_BONE_POS_Y = (1u << 7u),

			/// <summary> Keeps root bone movement on Z axis </summary>
			ANIMATE_BONE_POS_Z = (1u << 8u),

			/// <summary> Keeps root bone rotation around X axis </summary>
			ANIMATE_BONE_ROT_X = (1u << 9u),

			/// <summary> Keeps root bone rotation around Y axis </summary>
			ANIMATE_BONE_ROT_Y = (1u << 10u),

			/// <summary> Keeps root bone rotation around Z axis </summary>
			ANIMATE_BONE_ROT_Z = (1u << 11u)
		};

		/// <summary> Enumeration attribute for root-motion flags </summary>
		static const Object* RootMotionFlagsEnumAttribute();

		/// <summary> Bone, animations of which should be understood as root motion </summary>
		Transform* RootMotionSource()const;

		/// <summary>
		/// Sets root motion bone
		/// </summary>
		/// <param name="source"> Root motion bone </param>
		void SetRootMotionSource(Transform* source);

		/// <summary> Root motion flags </summary>
		RootMotionFlags RootMotionSettings()const;

		/// <summary>
		/// Sets roor motion flags
		/// </summary>
		/// <param name="flags"> Root motion settings </param>
		void SetRootMotionSettings(RootMotionFlags flags);

		/// <summary> 
		/// Rigidbody, that'll move instead of the root motion source
		/// <para/> If null, parent transform will move instead.
		/// </summary>
		Rigidbody* RootMotionTarget()const;

		/// <summary>
		/// Sets root motion target
		/// </summary>
		/// <param name="body"> Rigidbody to move instead of the root bone </param>
		void SetRootMotionTarget(Rigidbody* body);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;


	protected:
		/// <summary> Engine logic Update callback (should not be invoked by hand) </summary>
		virtual void Update()override;

	private:
		// True, if the fields are mapped with the target objects
		bool m_bound = false;

		// Global playback speed
		float m_playbackSpeed = 1.0f;

		// Clip state list
		struct PlaybackState {
			float time = 0.0f;
			float weight = 1.0f;
			float speed = 1.0f;
			bool loop = false;
			bool isPlaying = false;
			Reference<AnimationClip> clip;
		};

		// New clip states:
		std::vector<PlaybackState> m_channelStates;
		std::set<PlaybackState*> m_activeChannelStates;
		size_t m_channelCount = 0u;
		std::unordered_set<Reference<AnimationClip>> m_subscribedClips;

		// Temporary storage for the clips that are no longer playing back, but we still need them in m_clipStates
		std::vector<PlaybackState*> m_completeClipBuffer;

		// Root motion objects and flags
		WeakReference<Transform> m_rootMotionSource;
		WeakReference<Rigidbody> m_rootRigidbody;
		RootMotionFlags m_rootMotionSettings = RootMotionFlags::NONE;

		// Serialized field (target) data
		struct SerializedField {
			Reference<const Serialization::ItemSerializer> serializer = nullptr;
			void* targetAddr = nullptr;

			inline bool operator<(const SerializedField& other)const {
				if (serializer < other.serializer) return true;
				else if (serializer > other.serializer) return false;
				else return targetAddr < other.targetAddr;
			}
		};

		// AnimationTrack and state information
		struct TrackBinding {
			const AnimationTrack* track = nullptr;
			const PlaybackState* state = nullptr;
		};

		// Generic field update function
		typedef void(*FieldUpdateFn)(const SerializedField&, const TrackBinding*, size_t);

		// TrackBinding objects for SerializedField
		struct FieldBinding {
			using PerChannelTracks = std::map<const PlaybackState*, Stacktor<const AnimationTrack*, 1u>>;
			PerChannelTracks tracks;
			size_t bindingCount = 0u;
			FieldUpdateFn update = Unused<const SerializedField&, const TrackBinding*, size_t>;
		};

		// Binding information storage
		typedef std::map<SerializedField, FieldBinding> FieldBindings;
		typedef std::map<Reference<Component>, FieldBindings> ObjectBindings;
		ObjectBindings m_objectBindings;

		// Flattened binding information for faster simulation-time traversal
		std::set<PlaybackState*> m_reactivatedChannels;
		std::vector<TrackBinding> m_activeTrackBindings;
		struct FieldBindingInfo {
			SerializedField field;
			FieldUpdateFn update = nullptr;
			// Bindings is always a concatenation of: [Active Track Bindings sorted in ascending order by ClipPlaybackState] [Some preallocation you do not need to worry about]
			TrackBinding* bindings = nullptr;
			size_t activeBindingCount = 0u;
			size_t fieldBindingCount = 0u;
			const FieldBinding::PerChannelTracks* tracks = nullptr;
		};
		std::vector<FieldBindingInfo> m_flattenedFieldBindings;
		
		// Some internal functions and helpers are stored here...
		struct BindingHelper;
		struct SerializedPlayState;

		// Invokes update() function for all FieldBinding objects
		void Apply();

		// Increments AnimationTime-s for each ClipPlaybackState and prunes finished animations
		void AdvanceTime(const std::vector<PlaybackState*>& reactivatedStates);

		// Clears all bindings
		void Unbind();

		// (Re)Binds the animation tracks to corresponding serialized fields
		void Bind();

		// Activates channel bindings
		void ReactivateChannels(const std::vector<PlaybackState*>& reactivatedStates);

		// Deactivates channel bindings that are no longer active
		void DeactivateChannels();

		// The functions below unbind fields and tracks when heirarchy or clip data changes:
		void OnAnimationClipDirty(const AnimationClip*);
		void OnTransformHeirarchyChanged(ParentChangeInfo);
		void OnComponentDead(Component* component);
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Animator>(const Callback<TypeId>& report) { report(TypeId::Of<Scene::LogicContext::UpdatingComponent>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report);
}
