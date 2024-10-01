#pragma once
#include "../Core/Object.h"
#include "../Core/Systems/Event.h"
#include "../Core/Collections/Stacktor.h"
#include "../Core/TypeRegistration/TypeRegistration.h"
#include "../Math/Math.h"
#include "../Math/Curves.h"
#include "AssetDatabase/AssetDatabase.h"


namespace Jimara {
	/// <summary>
	/// Animation "Track", corresponding to a singular field of an animation clip
	/// </summary>
	class JIMARA_API AnimationTrack : public virtual Object {
	public:
		/// <summary> Animation duration </summary>
		virtual float Duration()const = 0;
	};

	/// <summary>
	/// AnimationTrack that happens to be stored as a parametric curve of some concrete type, parametrized by time
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	template<typename ValueType>
	class AnimationCurve : public virtual AnimationTrack, public virtual ParametricCurve<ValueType, float> {};

	/// <summary>
	/// AnimationCurve that is stored as a simple cubic bezier curve, parametrized by time
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	template<typename ValueType>
	class AnimationBezier : public virtual AnimationCurve<ValueType>, public virtual TimelineCurve<ValueType, BezierNode<ValueType>> {};

	/// <summary>
	/// Animation clip, well, storing a singular animation
	/// </summary>
	class JIMARA_API AnimationClip : public virtual Resource {
	public:
		class Track;
		class Writer;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Clip name </param>
		AnimationClip(const std::string_view& name = "");

		/// <summary> Virtual destructor </summary>
		virtual ~AnimationClip();

		/// <summary> Clip name </summary>
		const std::string& Name()const;

		/// <summary> Clip duration/length </summary>
		float Duration()const;

		/// <summary> Number of tracks within the clip </summary>
		size_t TrackCount()const;

		/// <summary>
		/// Animation track by index
		/// </summary>
		/// <param name="index"> Track index </param>
		/// <returns> Track </returns>
		const Track* GetTrack(size_t index)const;

		/// <summary> Invoked, whenever an animation Writer goes out of scope </summary>
		Event<const AnimationClip*>& OnDirty()const;

		/// <summary>
		/// AnimationTrack, tied directly to the clip
		/// </summary>
		class JIMARA_API Track : public virtual AnimationTrack {
		public:
			/// <summary> Clip duration/length </summary>
			virtual float Duration()const override;

			/// <summary> Owner clip </summary>
			const AnimationClip* Owner()const;

			/// <summary> Index within the owner clip </summary>
			size_t Index()const;

			/// <summary>
			/// Finds target object to animate using this track
			/// (currently, only Components are supported)
			/// </summary>
			/// <param name="rootObject"> "Root" object to start searching in </param>
			/// <returns> Target object if found, nullptr otherwise </returns>
			Object* FindTarget(Object* rootObject)const;

			/// <summary> Name of the target serialized field (if there's a duplicate name, well... that's, I guess, on the programmer that designed the class) </summary>
			const std::string& TargetField()const;

		private:
			// Owner clip
			const AnimationClip* m_owner = nullptr;

			// Index within the owner clip
			size_t m_index = 0;

			// Name of the target serialized field
			std::string m_targetField;

			// Binding chain for searching target from the root node:
			struct BindChainNode {
				std::string name;
				TypeId type;
			};
			std::vector<BindChainNode> m_bindChain;

			// Writer and AnimationClip kinda have to access some of the internals
			friend class Writer;
			friend class AnimationClip;
		};

#pragma warning(disable: 4250)
		/// <summary> 
		/// Arbitrary floating point track 
		/// </summary>
		class JIMARA_API FloatTrack : public virtual Track, public virtual AnimationCurve<float> {};

		/// <summary> 
		/// Floating point bezier track 
		/// </summary>
		class JIMARA_API FloatBezier : public virtual FloatTrack, public virtual AnimationBezier<float> {};

		/// <summary>
		/// 3d Vector track
		/// </summary>
		class JIMARA_API Vector3Track : public virtual Track, public virtual AnimationCurve<Vector3> {};

		/// <summary>
		/// 3d Vector bezier track
		/// </summary>
		class JIMARA_API Vector3Bezier : public virtual Vector3Track, public virtual AnimationBezier<Vector3> {};

		/// <summary>
		/// Combines 3 floating point tracks into a singular 3d vector track
		/// </summary>
		class JIMARA_API TripleFloatCombine : public virtual Vector3Track {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="x"> X value curve </param>
			/// <param name="y"> Y value curve </param>
			/// <param name="z"> Z value curve </param>
			TripleFloatCombine(
				ParametricCurve<float, float>* x = nullptr,
				ParametricCurve<float, float>* y = nullptr,
				ParametricCurve<float, float>* z = nullptr);

			/// <summary> X value curve </summary>
			Reference<ParametricCurve<float, float>>& X();

			/// <summary> X value curve </summary>
			const ParametricCurve<float, float>* X()const;

			/// <summary> Y value curve </summary>
			Reference<ParametricCurve<float, float>>& Y();

			/// <summary> Y value curve </summary>
			const ParametricCurve<float, float>* Y()const;

			/// <summary> Z value curve </summary>
			Reference<ParametricCurve<float, float>>& Z();

			/// <summary> Z value curve </summary>
			const ParametricCurve<float, float>* Z()const;

			/// <summary>
			/// Calculates curve value for the given time point
			/// </summary>
			/// <param name="time"> Time point </param>
			/// <returns> Combination of X, Y and Z </returns>
			virtual Vector3 Value(float time)const override;

		private:
			// X value curve
			Reference<ParametricCurve<float, float>> m_x;

			// Y value curve
			Reference<ParametricCurve<float, float>> m_y;

			// Z value curve
			Reference<ParametricCurve<float, float>> m_z;
		};

		/// <summary>
		/// Combines 3 floating point tracks into a singular 3d vector track
		/// </summary>
		class JIMARA_API EulerAngleTrack : public virtual TripleFloatCombine {
		public:
			/// <summary>
			/// Evaluation/Interpretation mode
			/// </summary>
			enum class EvaluationMode : uint8_t {
				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in XYZ order </summary>
				XYZ_EULER = 0,

				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in XZY order </summary>
				XZY_EULER = 1,

				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in YXZ order </summary>
				YXZ_EULER = 2,

				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in YZX order </summary>
				YZX_EULER = 3,

				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in ZXY order </summary>
				ZXY_EULER = 4,

				/// <summary> Interprets X, Y, Z as euler angles, storing rotation in ZYX order </summary>
				ZYX_EULER = 5,

				/// <summary> Just the enumeration choice count, not an actual interpretation mode </summary>
				MODE_COUNT = 6
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="x"> X value curve </param>
			/// <param name="y"> Y value curve </param>
			/// <param name="z"> Z value curve </param>
			/// <param name="mode"> Evaluation mode </param>
			EulerAngleTrack(
				ParametricCurve<float, float>* x = nullptr, 
				ParametricCurve<float, float>* y = nullptr,
				ParametricCurve<float, float>* z = nullptr,
				EvaluationMode mode = EvaluationMode::YXZ_EULER);

			/// <summary>
			/// Sets evaluation mode
			/// </summary>
			/// <param name="mode"> Mode to use </param>
			void SetMode(EvaluationMode mode);

			/// <summary> Evaluation mode property </summary>
			inline Property<EvaluationMode> Mode() {
				EvaluationMode(*get)(EulerAngleTrack*) = [](EulerAngleTrack* track) -> EvaluationMode { return ((const EulerAngleTrack*)track)->Mode(); };
				void(*set)(EulerAngleTrack*, const EvaluationMode&) = [](EulerAngleTrack* track, const EvaluationMode& mode) { track->SetMode(mode); };
				return Property<EvaluationMode>(get, set, this);
			}

			/// <summary> Evaluation mode </summary>
			EvaluationMode Mode()const;

			/// <summary>
			/// Calculates curve value for the given time point
			/// </summary>
			/// <param name="time"> Time point </param>
			/// <returns> Combination of X, Y and Z </returns>
			virtual Vector3 Value(float time)const override;

		private:
			// Evaluation mode
			EvaluationMode m_mode = EvaluationMode::MODE_COUNT;

			// Evaluation function, based on the mode
			Function<Vector3, float, float, float> m_evaluate;
		};

		/// <summary>
		/// EulerAngleTrack with 'preapplied' parent rotation
		/// </summary>
		class JIMARA_API RotatedEulerAngleTrack : public virtual EulerAngleTrack {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="x"> X value curve </param>
			/// <param name="y"> Y value curve </param>
			/// <param name="z"> Z value curve </param>
			/// <param name="mode"> Evaluation mode </param>
			/// <param name="rotation"> Rotation matrix </param>
			RotatedEulerAngleTrack(
				ParametricCurve<float, float>* x = nullptr,
				ParametricCurve<float, float>* y = nullptr,
				ParametricCurve<float, float>* z = nullptr,
				EvaluationMode mode = EvaluationMode::YXZ_EULER, 
				const Matrix4& rotation = Math::Identity());

			/// <summary> "Parent" rotation </summary>
			Matrix4& Rotation();

			/// <summary> "Parent" rotation </summary>
			Matrix4 Rotation()const;

			/// <summary>
			/// Calculates curve value for the given time point
			/// </summary>
			/// <param name="time"> Time point </param>
			/// <returns> Combination of X, Y and Z </returns>
			virtual Vector3 Value(float time)const override;

		private:
			// "Parent" Rotation matrix
			Matrix4 m_rotation;
		};
#pragma warning(default: 4250)


		/// <summary>
		/// One willing to modify AnimationClip or any of it's track bindings should do it via Writer for optimal Dirty reporting
		/// Note: Compared to those classes with Readers, this writer is not thread-safe and is only adviced to be used on the main Update thread 
		///		or during clip creation, before any other object has access to it;
		/// </summary>
		class JIMARA_API Writer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="animation"> Target clip </param>
			Writer(AnimationClip* animation);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="animation"> Target clip </param>
			Writer(AnimationClip& animation);

			/// <summary> Virtual destructor </summary>
			virtual ~Writer();

			/// <summary> Animation clip name </summary>
			std::string& Name()const;

			/// <summary> Clip duration </summary>
			float Duration()const;

			/// <summary>
			/// Sets clip duration
			/// </summary>
			/// <param name="duration"> New clip duration (negative values will be replaced by 0) </param>
			void SetDuration(float duration)const;

			/// <summary> Number of tracks within the clip </summary>
			size_t TrackCount()const;

			/// <summary>
			/// Animation track by index
			/// </summary>
			/// <param name="index"> Track index </param>
			/// <returns> Track </returns>
			Track* GetTrack(size_t index)const;

			/// <summary>
			/// Appends track of given type
			/// </summary>
			/// <typeparam name="TrackType"> Any type derived from Track </typeparam>
			/// <typeparam name="...ConstructorArgs"> Instantiaton arguments </typeparam>
			/// <param name="...args"> Instantiaton arguments </param>
			/// <returns> Newly added track </returns>
			template<typename TrackType, typename... ConstructorArgs>
			inline TrackType* AddTrack(ConstructorArgs... args)const {
				Reference<TrackType> track = Object::Instantiate<TrackType>(args...);
				track->m_owner = m_animation;
				track->m_index = m_animation->m_tracks.size();
				m_animation->m_tracks.push_back(track);
				return track;
			}

			/// <summary>
			/// Swaps track order
			/// </summary>
			/// <param name="indexA"> First index </param>
			/// <param name="indexB"> Second index </param>
			void SwapTracks(size_t indexA, size_t indexB)const;

			/// <summary>
			/// Removes track
			/// </summary>
			/// <param name="index"> Track index to remove at </param>
			void RemoveTrack(size_t index)const;

			/// <summary> Removes last track </summary>
			void PopTrack()const;

			/// <summary>
			/// Removes the binding chain/path of the track
			/// </summary>
			/// <param name="trackId"> Track index </param>
			void ClearTrackBindings(size_t trackId)const;

			/// <summary>
			/// Appends an Object name and type checker to the binding chain of the track
			/// </summary>
			/// <param name="trackId"> Index of the track to operate on </param>
			/// <param name="name"> Name of the binding chain link to check when searching for the target </param>
			/// <param name="type"> Type of the binding chain link </param>
			void AddTrackBinding(size_t trackId, const std::string_view& name, TypeId type)const;

			/// <summary>
			/// Appends an Object name and type checker to the binding chain of the track
			/// </summary>
			/// <typeparam name="TargetType"> Type of the binding chain link </typeparam>
			/// <param name="trackId"> Index of the track to operate on </param>
			/// <param name="name"> Name of the binding chain link to check when searching for the target </param>
			template<typename TargetType>
			inline void AddTrackBinding(size_t trackId, const std::string_view& name)const {
				AddTrackBinding(trackId, name, TypeId::Of<TargetType>());
			}

			/// <summary>
			/// Sets target field for the track
			/// </summary>
			/// <param name="trackId"> Index of the track to operate on </param>
			/// <param name="targetField"> Field name the track should be targeting </param>
			void SetTrackTargetField(size_t trackId, const std::string_view& targetField)const;

		private:
			// Animation to read data from
			const Reference<AnimationClip> m_animation;

			// Copy not allowed:
			inline Writer(const Writer&) = delete;
			inline Writer& operator=(const Writer&) = delete;
		};


	private:
		// AnimationClip name
		std::string m_name;

		// Clip duration
		float m_duration = 0.0f;

		// List of underlying tracks
		std::vector<Reference<Track>> m_tracks;

		// Lock for writers
		mutable std::mutex m_changeLock;

		// Invoked when a writer goes out of scope
		mutable EventInstance<const AnimationClip*> m_onDirty;
	};

	/// <summary>
	/// Blends AnimationCurve objects togather
	/// </summary>
	/// <typeparam name="ValueType"> AnimationCurve value type </typeparam>
	template<typename ValueType>
	class AnimationCurveBlend : AnimationCurve<ValueType> {
	public:
		/// <summary>
		/// Adds a curve to the blending logic
		/// </summary>
		/// <param name="curve"> Curve to blend in </param>
		/// <param name="blendWeight"> Blending weight </param>
		/// <param name="durationScale"> Basically, curve playback speed </param>
		/// <param name="phaseShift"> Curve phase shift (start time, divided by duration) </param>
		virtual void SetCurve(const AnimationCurve<ValueType>* curve, float blendWeight, float durationScale = 1.0f, float phaseShift = 0.0f) = 0;

		/// <summary>
		/// Removes curve from the blending equasion
		/// </summary>
		/// <param name="curve"> Curve to remove </param>
		inline void RemoveCurve(const AnimationCurve<ValueType>* curve) { SetCurve(curve, 0.0f); }
	};

	/// <summary>
	/// Linearly interpolates curve values
	/// </summary>
	/// <typeparam name="ValueType"> AnimationCurve value type </typeparam>
	template<typename ValueType>
	class AnimationCurveLerp : AnimationCurveBlend<ValueType> {
	private:
		// Curve blending info
		struct Entry {
			Reference<const AnimationCurve<ValueType>> curve;
			float blendWeight = 0.0f;
			float durationScale = 1.0f;
			float phaseShift = 0.0f;
		};
		Stacktor<Entry, 4> m_curves;

	public:
		/// <summary>
		/// Adds a curve to the blending logic
		/// </summary>
		/// <param name="curve"> Curve to blend in </param>
		/// <param name="blendWeight"> Blending weight </param>
		/// <param name="durationScale"> Basically, curve playback speed </param>
		/// <param name="phaseShift"> Curve phase shift (start time, divided by duration) </param>
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

		/// <summary>
		/// Lerps between blended curves
		/// </summary>
		/// <param name="time"> Animation time </param>
		/// <returns> Blended value at time </returns>
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

		/// <summary> Weighted average of the blended clip durations </summary>
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
