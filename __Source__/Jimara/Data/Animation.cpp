#include "Animation.h"


namespace Jimara {
	AnimationClip::AnimationClip(const std::string_view& name) : m_name(name) {}

	AnimationClip::~AnimationClip() {
		for (size_t i = 0; i < m_tracks.size(); i++)
			m_tracks[i]->m_owner = nullptr;
	}

	const std::string& AnimationClip::Name()const { return m_name; }

	float AnimationClip::Duration()const { return m_duration; }

	size_t AnimationClip::TrackCount()const { return m_tracks.size(); }

	const AnimationClip::Track* AnimationClip::GetTrack(size_t index)const { return m_tracks[index]; }

	Event<const AnimationClip*>& AnimationClip::OnDirty()const { return m_onDirty; }


	float AnimationClip::Track::Duration()const { return m_owner == nullptr ? 0.0f : m_owner->Duration(); }

	const AnimationClip* AnimationClip::Track::Owner()const { return m_owner; }


	AnimationClip::Writer::Writer(AnimationClip* animation)
		: m_animation(animation) {
		m_animation->m_changeLock.lock();
	}

	AnimationClip::Writer::Writer(AnimationClip& animation) : Writer(&animation) {}

	AnimationClip::Writer::~Writer() {
		m_animation->m_changeLock.unlock();
		m_animation->m_onDirty(m_animation);
	}

	std::string& AnimationClip::Writer::Name()const { return m_animation->m_name; }

	float AnimationClip::Writer::Duration()const { return m_animation->m_duration; }

	void AnimationClip::Writer::SetDuration(float duration)const {
		if (duration < 0.0f) duration = 0.0f;
		m_animation->m_duration = duration;
	}

	size_t AnimationClip::Writer::TrackCount()const { return m_animation->m_tracks.size(); }

	AnimationTrack* AnimationClip::Writer::Track(size_t index)const { return m_animation->m_tracks[index]; }

	void AnimationClip::Writer::SwapTracks(size_t indexA, size_t indexB)const {
		std::vector<Reference<AnimationClip::Track>>& tracks = m_animation->m_tracks;
		std::swap(tracks[indexA], tracks[indexB]);
	}

	void AnimationClip::Writer::RemoveTrack(size_t index)const {
		if (TrackCount() <= index) return;
		size_t end = (TrackCount() - 1);
		std::vector<Reference<AnimationClip::Track>>& tracks = m_animation->m_tracks;
		while (index < end) {
			size_t next = (index + 1);
			tracks[index] = tracks[next];
			index = next;
		}
		tracks.pop_back();
	}

	void AnimationClip::Writer::PopTrack()const { m_animation->m_tracks.pop_back(); }
}
