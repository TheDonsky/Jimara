#pragma once
#include "../Components/Component.h"
#include "../Core/Collections/Stacktor.h"

namespace Jimara {
	class AnimationTrack : public virtual Object {
	public:
		virtual float Length()const = 0;
	};

	template<typename ValueType>
	class AnimationCurve : public virtual AnimationTrack {
	public:
		virtual ValueType Value(float time)const = 0;
	};

	class AnimationBinder : public virtual Object {
	public:
		virtual void Apply(Object* target, float time)const = 0;
	};

	template<typename ValueType>
	class AnimationCurveBlend : AnimationCurve<ValueType> {
	public:
		virtual void SetCurve(const AnimationCurve<ValueType>* curve, float blendWeight, float timeOffset = 0.0f, float timeScale = 1.0f) = 0;

		inline void RemoveCurve(const AnimationCurve<ValueType>* curve) { SetCurve(curve, 0.0f); }
	};

	template<typename ValueType>
	class AnimationCurveLerp : AnimationCurveBlend<ValueType> {
	private:
		struct Entry {
			Reference<const AnimationCurve<ValueType>> curve;
			float blendWeight = 0.0f;
			float timeOffset = 0.0f;
			float timeScale = 1.0f;
		};
		Stacktor<Entry, 4> m_curves;

	public:
		virtual void SetCurve(const AnimationCurve<ValueType>* curve, float blendWeight, float timeOffset = 0.0f, float timeScale = 1.0f) override {
			if (curve == nullptr) return;
			auto updateEntry = [&](Entry& entry) {
				entry.blendWeight = blendWeight;
				entry.timeOffset = timeOffset;
				entry.timeScale = timeScale;
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
			float totalWeight = 0.0f;
			for (size_t i = 0; i < m_curves.Size(); i++) {
				const Entry& entry = m_curves[i];
				float curveLength = entry.curve->Length();
				valueSum += entry.curve->Value(
					curveLength <= std::numeric_limits<float>::epsilon() ? 0.0f :
					Math::FloatRemainder((time * entry.timeScale) + entry.timeOffset, curveLength));
				totalWeight += entry.blendWeight;
			}
			return valueSum / totalWeight;
		}

		virtual float Length()const override {
			float totalLength = 0.0f;
			float totalWeight = 0.0f;
			for (size_t i = 0; i < m_curves.Size(); i++) {
				const Entry& entry = m_curves[i];
				totalLength += entry.curve->Length();
				totalWeight += entry.blendWeight;
			}
			return totalLength / totalWeight;
		}
	};
}
