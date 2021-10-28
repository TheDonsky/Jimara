#pragma once
#include "../Transform.h"
#include "../Interfaces/Updatable.h"
#include "../../Data/Animation.h"
#include "../../Core/Unused.h"


namespace Jimara {
	class Animator : public virtual Component, public virtual Updatable {
	public:
		Animator(Component* parent, const std::string_view& name);

		virtual ~Animator();

		struct ClipPlaybackState {
			float time;
			float weight;
			float speed;
			bool loop;

			inline ClipPlaybackState(float t = 0.0f, float w = 1.0f, float s = 1.0f, bool l = false) 
				: time(t), weight(w), speed(s), loop(l) {}

			inline bool operator==(const ClipPlaybackState& other)const {
				return
					(time == other.time) &&
					(weight == other.weight) &&
					(speed == other.speed) &&
					(loop == other.loop);
			}
		};

		void SetClipState(const AnimationClip* clip, ClipPlaybackState state);

		ClipPlaybackState ClipState(const AnimationClip* clip)const;

		virtual void Update()override;

	private:
		bool m_dead = false;
		bool m_bound = false;

		typedef std::map<Reference<const AnimationClip>, ClipPlaybackState> ClipStates;
		ClipStates m_clipStates;
		std::vector<Reference<const AnimationClip>> m_completeClipBuffer;

		struct SerializedField {
			Reference<const Serialization::ItemSerializer> serializer = nullptr;
			void* targetAddr = nullptr;

			inline bool operator<(const SerializedField& other)const {
				if (serializer < other.serializer) return true;
				else if (serializer > other.serializer) return false;
				else return targetAddr < other.targetAddr;
			}
		};

		struct TrackBinding {
			const AnimationTrack* track = nullptr;
			const ClipPlaybackState* state = nullptr;
		};

		struct FieldBinding {
			std::vector<TrackBinding> bindings;

			Callback<const SerializedField&, const FieldBinding&> update = Callback(Unused<const SerializedField&, const FieldBinding&>);
		};

		typedef std::map<SerializedField, FieldBinding> FieldBindings;
		typedef std::map<Reference<Component>, FieldBindings> ObjectBindings;

		ObjectBindings m_objectBindings;
		
		void Apply();
		void AdvanceTime();

		void Unbind();
		void Bind();
		static Callback<const SerializedField&, const FieldBinding&> GetFieldUpdater(const Serialization::ItemSerializer* serializer);
		void OnAnimationClipDirty(const AnimationClip*);
		void OnTransformHeirarchyChanged(const Component*);
		void OnComponentDead(Component* component);
	};
}
