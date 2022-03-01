#pragma once
#include "../Transform.h"
#include "../../Data/Animation.h"
#include "../../Core/Helpers.h"


namespace Jimara {
	/// <summary> This will make sure, Animator is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Animator);

	/// <summary>
	/// Component, responsible for AnimationClip playback
	/// </summary>
	class Animator : public virtual Scene::LogicContext::UpdatingComponent {
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
		struct ClipPlaybackState {
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
		void SetClipState(const AnimationClip* clip, ClipPlaybackState state);

		/// <summary>
		/// Short for SetClipState(ClipPlaybackState(timeOffset, weight, speed, loop))
		/// </summary>
		/// <param name="clip"> Clip to play (nullptr will be ignored) </param>
		/// <param name="loop"> If true, the animation will be looping </param>
		/// <param name="speed"> Playback speed </param>
		/// <param name="weight"> Blending weight </param>
		/// <param name="timeOffset"> Animation time </param>
		void Play(const AnimationClip* clip, bool loop = false, float speed = 1.0f, float weight = 1.0f, float timeOffset = 0.0f);

		/// <summary>
		/// Checks current playback state per clip
		/// </summary>
		/// <param name="clip"> Clip to check </param>
		/// <returns> Current playback state </returns>
		ClipPlaybackState ClipState(const AnimationClip* clip)const;

		/// <summary>
		/// Checks current playback state per clip
		/// </summary>
		/// <param name="clip"> Clip to check </param>
		/// <returns> True, if the clip is playing </returns>
		bool ClipPlaying(const AnimationClip* clip)const;

		/// <summary> Checks if any clip is currently playing back </summary>
		bool Playing()const;

		/// <summary> Engine logic Update callback (should not be invoked by hand) </summary>
		virtual void Update()override;


	private:
		// Becomes true, once the Animator goes out of scope
		bool m_dead = false;

		// True, if the fields are mapped with the target objects
		bool m_bound = false;

		// Clip state collection
		typedef std::map<Reference<const AnimationClip>, ClipPlaybackState> ClipStates;
		ClipStates m_clipStates;

		// Temporary storage for the clips that are no longer playing back, but we still need them in m_clipStates
		std::vector<Reference<const AnimationClip>> m_completeClipBuffer;

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
		
		// Some internal functions are stored here...
		struct BindingHelper;

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
		void OnTransformHeirarchyChanged(const Component*);
		void OnComponentDead(Component* component);
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Animator>(const Callback<TypeId>& report) { report(TypeId::Of<Scene::LogicContext::UpdatingComponent>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<Animator>(const Callback<const Object*>& report);
}
