#include "AudioBuffer.h"



namespace Jimara {
	namespace Audio {
		namespace {
			class MonoLayout : public virtual AudioChannelLayout {
			public:
				virtual Vector3 ChannelPosition(size_t channelId)const override { return Vector3(0.0f, 0.0f, 0.0f); }
			};
		}
		Reference<const AudioChannelLayout> AudioChannelLayout::Mono() {
			static const MonoLayout layout;
			return &layout;
		}

		namespace {
			class StereoLayout : public virtual AudioChannelLayout {
			public:
				virtual Vector3 ChannelPosition(size_t channelId)const override {
					static const Vector3 POSITIONS[] = { Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) };
					return POSITIONS[channelId & 1];
				}
			};
		}
		Reference<const AudioChannelLayout> AudioChannelLayout::Stereo() {
			static const StereoLayout layout;
			return &layout;
		}

		namespace {
			class Surround_5_1_Layout : public virtual AudioChannelLayout {
			public:
				virtual Vector3 ChannelPosition(size_t channelId)const override {
					static const Vector3 POSITIONS[] = {
						Vector3(-1.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 0.0f, 1.0f),
						Vector3(-1.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, -1.0f),
						Vector3(0.0f, -1.0f, 0.0f)
					};
					return POSITIONS[channelId % (sizeof(POSITIONS) / sizeof(Vector3))];
				}
			};
		}
		Reference<const AudioChannelLayout> AudioChannelLayout::Surround_5_1() {
			static const Surround_5_1_Layout layout;
			return &layout;
		}
	}
}
