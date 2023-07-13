#pragma once
#include "../Physics/Rigidbody.h"
#include "../WeakComponentReference.h"
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
		/// Clip playback state information
		/// </summary>
		struct JIMARA_API ClipPlaybackState {
			/// <summary> Animation time </summary>
			float time;

			/// <summary> Blending weight </summary>
			float weight;

			/// <summary> Playback speed </summary>
			float speed;

			/// <summary> If true, the animation will be looping </summary>
			bool loop;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="t"> Animation time </param>
			/// <param name="w"> Blending weight </param>
			/// <param name="s"> Playback speed </param>
			/// <param name="l"> If true, the animation will be looping </param>
			inline ClipPlaybackState(float t = 0.0f, float w = 1.0f, float s = 1.0f, bool l = false) 
				: time(t), weight(w), speed(s), loop(l) {}

			/// <summary>
			/// Checks if the playback states are equal
			/// </summary>
			/// <param name="other"> State to compare to </param>
			/// <returns> True, if all fields match </returns>
			inline bool operator==(const ClipPlaybackState& other)const {
				return
					(time == other.time) &&
					(weight == other.weight) &&
					(speed == other.speed) &&
					(loop == other.loop);
			}
		};

		/// <summary>
		/// Sets clip playback state
		/// </summary>
		/// <param name="clip"> Clip to play (nullptr will be ignored) </param>
		/// <param name="state"> Clip playback state (0 or negative weight will result in playback removal) </param>
		void SetClipState(AnimationClip* clip, ClipPlaybackState state);

		/// <summary>
		/// Short for SetClipState(ClipPlaybackState(timeOffset, weight, speed, loop))
		/// </summary>
		/// <param name="clip"> Clip to play (nullptr will be ignored) </param>
		/// <param name="loop"> If true, the animation will be looping </param>
		/// <param name="speed"> Playback speed </param>
		/// <param name="weight"> Blending weight </param>
		/// <param name="timeOffset"> Animation time </param>
		void Play(AnimationClip* clip, bool loop = false, float speed = 1.0f, float weight = 1.0f, float timeOffset = 0.0f);

		/// <summary>
		/// Checks current playback state per clip
		/// </summary>
		/// <param name="clip"> Clip to check </param>
		/// <returns> Current playback state </returns>
		ClipPlaybackState ClipState(AnimationClip* clip)const;

		/// <summary>
		/// Checks current playback state per clip
		/// </summary>
		/// <param name="clip"> Clip to check </param>
		/// <returns> True, if the clip is playing </returns>
		bool ClipPlaying(AnimationClip* clip)const;

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

	protected:
		/// <summary> Engine logic Update callback (should not be invoked by hand) </summary>
		virtual void Update()override;

	private:
		// True, if the fields are mapped with the target objects
		bool m_bound = false;

		// Global playback speed
		float m_playbackSpeed = 1.0f;

		// Clip state collection
		struct PlaybackState : public ClipPlaybackState {
			uint64_t insertionId = 0;
			inline PlaybackState() {}
			inline PlaybackState(const ClipPlaybackState& base, uint64_t insertion) : ClipPlaybackState(base), insertionId(insertion) {}
		};
		typedef std::map<Reference<AnimationClip>, PlaybackState> ClipStates;
		ClipStates m_clipStates;
		uint64_t m_numInsertions = 0;

		// Temporary storage for the clips that are no longer playing back, but we still need them in m_clipStates
		std::vector<Reference<AnimationClip>> m_completeClipBuffer;

		// Root motion objects and flags
		WeakComponentReference<Transform> m_rootMotionSource;
		WeakComponentReference<Rigidbody> m_rootRigidbody;
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
			const ClipPlaybackState* state = nullptr;
		};

		// TrackBinding objects for SerializedField
		struct FieldBinding {
			Stacktor<TrackBinding, 4> bindings;

			typedef void(*UpdateFn)(const SerializedField&, const FieldBinding&);
			UpdateFn update = Unused<const SerializedField&, const FieldBinding&>;
		};

		// Binding information storage
		typedef std::map<SerializedField, FieldBinding> FieldBindings;
		typedef std::map<Reference<Component>, FieldBindings> ObjectBindings;
		ObjectBindings m_objectBindings;
		
		// Some internal functions and helpers are stored here...
		struct BindingHelper;
		struct SerializedPlayState;

		// Invokes update() function for all FieldBinding objects
		void Apply();

		// Increments AnimationTime-s for each ClipPlaybackState and prunes finished animations
		void AdvanceTime();

		// Clears all bindings
		void Unbind();

		// (Re)Binds the animation tracks to corresponding serialized fields
		void Bind();

		// The functions below unbind fields and tracks when heirarchy or clip data changes:
		void OnAnimationClipDirty(const AnimationClip*);
		void OnTransformHeirarchyChanged(ParentChangeInfo);
		void OnComponentDead(Component* component);
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Animator>(const Callback<TypeId>& report) { report(TypeId::Of<Scene::LogicContext::UpdatingComponent>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report);
}
