#pragma once
#include "../Core/Object.h"
#include "../Core/Systems/Event.h"
#include "../Core/Collections/Stacktor.h"
#include "../Math/Math.h"
#include "../Math/Curves.h"


namespace Jimara {
	class AnimationTrack : public virtual Object {
	public:
		virtual float Duration()const = 0;
	};

	template<typename ValueType>
	class AnimationCurve : public virtual AnimationTrack, public virtual ParametricCurve<ValueType, float> {};

	template<typename ValueType>
	class AnimationBezier : public virtual AnimationCurve<ValueType>, public virtual TimelineCurve<ValueType, BezierNode<ValueType>> {};

	class AnimationClip : public virtual Object {
	public:
		class Track;
		class Writer;

		AnimationClip(const std::string_view& name = "");

		virtual ~AnimationClip();

		const std::string& Name()const;

		float Duration()const;

		size_t TrackCount()const;

		const Track* GetTrack(size_t index)const;

		/// <summary> Invoked, whenever a mesh Writer goes out of scope </summary>
		Event<const AnimationClip*>& OnDirty()const;


		class Track : public virtual AnimationTrack {
		public:
			virtual float Duration()const override;

			const AnimationClip* Owner()const;

			size_t Index()const;

			Object* FindTarget(Object* rootObject)const;

		private:
			const AnimationClip* m_owner = nullptr;
			size_t m_index = 0;

			struct BindChainNode {
				std::string name;
				bool(*checkType)(const Object*);
			};
			std::vector<BindChainNode> m_bindChain;

			friend class Writer;
			friend class AnimationClip;
		};

#pragma warning(disable: 4250)
		class FloatTrack : public virtual Track, public virtual AnimationCurve<float> {};

		class FloatBezier : public virtual FloatTrack, public virtual AnimationBezier<float> {};

		class Vector3Track : public virtual Track, public virtual AnimationCurve<Vector3> {
		public:
			enum class EvaluationMode : uint8_t {
				STANDARD = 0,
				XYZ_EULER = 1,
				XZY_EULER = 2,
				YXZ_EULER = 3,
				YZX_EULER = 4,
				ZXY_EULER = 5,
				ZYX_EULER = 6,
				MODE_COUNT = 7
			};

			Vector3Track(
				ParametricCurve<float, float>* x = nullptr, 
				ParametricCurve<float, float>* y = nullptr,
				ParametricCurve<float, float>* z = nullptr,
				EvaluationMode mode = EvaluationMode::STANDARD);

			void SetMode(EvaluationMode mode);

			inline Property<EvaluationMode> Mode() {
				EvaluationMode(*get)(Vector3Track*) = [](Vector3Track* track) -> EvaluationMode { return ((const Vector3Track*)track)->Mode(); };
				void(*set)(Vector3Track*, const EvaluationMode&) = [](Vector3Track* track, const EvaluationMode& mode) { track->SetMode(mode); };
				return Property<EvaluationMode>(get, set, this);
			}

			EvaluationMode Mode()const;

			Reference<ParametricCurve<float, float>>& X();

			const ParametricCurve<float, float>* X()const;

			Reference<ParametricCurve<float, float>>& Y();

			const ParametricCurve<float, float>* Y()const;

			Reference<ParametricCurve<float, float>>& Z();

			const ParametricCurve<float, float>* Z()const;

			virtual Vector3 Value(float time)const override;

		private:
			Reference<ParametricCurve<float, float>> m_x;
			Reference<ParametricCurve<float, float>> m_y;
			Reference<ParametricCurve<float, float>> m_z;
			EvaluationMode m_mode = EvaluationMode::MODE_COUNT;
			Function<Vector3, float, float, float> m_evaluate;
		};
#pragma warning(default: 4250)

		class Writer {
		public:
			Writer(AnimationClip* animation);

			Writer(AnimationClip& animation);

			virtual ~Writer();

			std::string& Name()const;

			float Duration()const;

			void SetDuration(float duration)const;

			size_t TrackCount()const;

			AnimationTrack* Track(size_t index)const;

			template<typename TrackType, typename... ConstructorArgs>
			inline TrackType* AddTrack(ConstructorArgs... args)const {
				Reference<TrackType> track = Object::Instantiate<TrackType>(args...);
				track->m_owner = m_animation;
				track->m_index = m_animation->m_tracks.size();
				m_animation->m_tracks.push_back(track);
				return track;
			}

			void SwapTracks(size_t indexA, size_t indexB)const;

			void RemoveTrack(size_t index)const;

			void PopTrack()const;

			void ClearTrackBindings(size_t trackId)const;

			template<typename TargetType>
			inline void AddTrackBinding(size_t trackId, const std::string_view& name)const {
				bool(*checkType)(const Object*) = [](const Object* object) -> bool { return dynamic_cast<const TargetType*>(object) != nullptr; };
				m_animation->m_tracks[trackId]->m_bindChain.push_back({ std::string(name), checkType });
			}

		private:
			// Animation to read data from
			const Reference<AnimationClip> m_animation;

			// Copy not allowed:
			inline Writer(const Writer&) = delete;
			inline Writer& operator=(const Writer&) = delete;
		};


	private:
		std::string m_name;
		float m_duration = 0.0f;
		std::vector<Reference<Track>> m_tracks;
		mutable std::mutex m_changeLock;
		mutable EventInstance<const AnimationClip*> m_onDirty;
	};

	template<typename ValueType>
	class AnimationCurveBlend : AnimationCurve<ValueType> {
	public:
		virtual void SetCurve(const AnimationCurve<ValueType>* curve, float blendWeight, float durationScale = 1.0f, float phaseShift = 0.0f) = 0;

		inline void RemoveCurve(const AnimationCurve<ValueType>* curve) { SetCurve(curve, 0.0f); }
	};

	template<typename ValueType>
	class AnimationCurveLerp : AnimationCurveBlend<ValueType> {
	private:
		struct Entry {
			Reference<const AnimationCurve<ValueType>> curve;
			float blendWeight = 0.0f;
			float durationScale = 1.0f;
			float phaseShift = 0.0f;
		};
		Stacktor<Entry, 4> m_curves;

	public:
		virtual void SetCurve(const AnimationCurve<ValueType>* curve, float blendWeight, float durationScale = 1.0f, float phaseShift = 0.0f) override {
			if (curve == nullptr) return;
			auto updateEntry = [&](Entry& entry) {
				entry.blendWeight = blendWeight;
				entry.durationScale = durationScale;
				entry.phaseShift = phaseShift;
			};
			for (size_t i = 0; i < m_curves.Size(); i++) {
				Entry& entry = m_curves[i];
				if (entry.curve == curve) {
					if (blendWeight <= std::numeric_limits<float>::epsilon()) m_curves.RemoveAt(i);
					else updateEntry(entry);
					return;
				}
			}
			if (blendWeight > std::numeric_limits<float>::epsilon()) {
				Entry entry;
				entry.curve = curve;
				updateEntry(entry);
				m_curves.Push(entry);
			}
		}

		virtual ValueType Value(float time)const override {
			ValueType valueSum(0.0f);
			if (m_curves.Size() <= 0) return valueSum;
			float totalWeight = 0.0f;
			float duration = Duration();
			float phase = Math::FloatRemainder((duration > std::numeric_limits<float>::epsilon()) ? (time / duration) : 0.0f, 1.0f);
			for (size_t i = 0; i < m_curves.Size(); i++) {
				const Entry& entry = m_curves[i];
				valueSum += entry.curve->Value(entry.curve->Duration() * Math::FloatRemainder(phase + entry.phaseShift, 1.0f));
				totalWeight += entry.blendWeight;
			}
			return valueSum / totalWeight;
		}

		virtual float Duration()const override {
			float durationSum = 0.0f;
			if (m_curves.Size() <= 0) return durationSum;
			float totalWeight = 0.0f;
			for (size_t i = 0; i < m_curves.Size(); i++) {
				const Entry& entry = m_curves[i];
				durationSum += entry.curve->Duration() * entry.durationScale;
				totalWeight += entry.blendWeight;
			}
			return durationSum / totalWeight;
		}
	};
}
