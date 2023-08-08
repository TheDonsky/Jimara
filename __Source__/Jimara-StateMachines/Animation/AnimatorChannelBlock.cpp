#include "AnimatorChannelBlock.h"


namespace Jimara {
	struct AnimatorChannelBlock::Helpers {
		class Allocator : public virtual Jimara::ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<Jimara::Animator> m_animator;
			Jimara::SpinLock m_lock;
			size_t m_channelCount = 0u;
			std::vector<size_t> m_freeChannels;

		public:
			inline Allocator(Jimara::Animator* animator) : m_animator(animator) {}
			inline virtual ~Allocator() {}

			inline void AllocateChannels(size_t* channels, size_t count) {
				std::unique_lock<Jimara::SpinLock> lock(m_lock);
				for (size_t i = 0u; i < count; i++) {
					if (!m_freeChannels.empty()) {
						channels[i] = m_freeChannels.back();
						m_freeChannels.pop_back();
					}
					else {
						channels[i] = m_channelCount;
						m_channelCount++;
					}
				}
				std::sort(channels, channels + count);
			}

			inline void FreeChannels(const size_t* channels, size_t count) {
				std::unique_lock<Jimara::SpinLock> lock(m_lock);
				for (size_t i = 0u; i < count; i++) {
					if (m_animator->ChannelCount() > channels[i])
						m_animator->Channel(channels[i]).SetClip(nullptr);
					m_freeChannels.push_back(channels[i]);
				}
			}
		};

		class AllocatorCache : public virtual Jimara::ObjectCache<Reference<const Object>> {
		public:
			static Reference<Allocator> Get(Jimara::Animator* animator) {
				if (animator == nullptr)
					return nullptr;
				static AllocatorCache cache;
				return cache.GetCachedOrCreate(animator, false, [&]() { return Object::Instantiate<Allocator>(animator); });
			}
		};
	};

	AnimatorChannelBlock::AnimatorChannelBlock() {}

	AnimatorChannelBlock::~AnimatorChannelBlock() {
		Reset(nullptr);
	}

	Jimara::Animator* AnimatorChannelBlock::Animator()const {
		return m_animator;
	}

	void AnimatorChannelBlock::Reset(Jimara::Animator* animator) {
		SetChannelCount(0u);
		if (animator == m_animator)
			return;
		m_animator = animator;
		m_allocator = Helpers::AllocatorCache::Get(m_animator);
	}

	size_t AnimatorChannelBlock::ChannelCount()const {
		return m_indirectionTable.Size();
	}

	void AnimatorChannelBlock::SetChannelCount(size_t count) {
		const size_t initialCount = m_indirectionTable.Size();
		if (initialCount == count || m_allocator == nullptr)
			return;
		else if (m_indirectionTable.Size() > count) {
			dynamic_cast<Helpers::Allocator*>(m_allocator.operator->())->FreeChannels(
				m_indirectionTable.Data() + count, m_indirectionTable.Size() - count);
			m_indirectionTable.Resize(count);
		}
		else {
			m_indirectionTable.Resize(count);
			dynamic_cast<Helpers::Allocator*>(m_allocator.operator->())->AllocateChannels(
				m_indirectionTable.Data() + initialCount, m_indirectionTable.Size() - initialCount);
		}
	}

	Jimara::Animator::AnimationChannel AnimatorChannelBlock::operator[](size_t channel) {
		if (channel >= m_indirectionTable.Size())
			SetChannelCount(channel + 1u);
		return m_animator->Channel(m_indirectionTable[channel]);
	}

	void AnimatorChannelBlock::StopAllChannels() {
		if (m_animator == nullptr)
			return;
		for (size_t i = 0u; i < m_indirectionTable.Size(); i++) {
			size_t channelId = m_indirectionTable[i];
			if (m_animator->ChannelCount() > channelId)
				m_animator->Channel(channelId).Stop();
		}
	}
}
