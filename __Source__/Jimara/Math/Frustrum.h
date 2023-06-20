#pragma once
#include "Math.h"


namespace Jimara {
	/// <summary>
	/// Simple utility for frustrum shape
	/// </summary>
	class FrustrumShape {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="view"> View matrix </param>
		/// <param name="projection"> Projection matrix </param>
		inline FrustrumShape(const Matrix4& view, const Matrix4& projection)
			: m_view(view), m_projection(projection) {
			auto safeInvert = [](const Matrix4& matrix) {
				const Matrix4 inverse = Math::Inverse(matrix);
				auto hasNanOrInf = [](const Vector4& column) {
					auto check = [](float f) { return std::isnan(f) || std::isinf(f); };
					return (check(column.x) || check(column.y) || check(column.z) || check(column.w));
				};
				if (hasNanOrInf(inverse[0]) ||
					hasNanOrInf(inverse[1]) ||
					hasNanOrInf(inverse[2]) ||
					hasNanOrInf(inverse[3])) return Math::Identity();
				return inverse;
			};
			m_inversePose = safeInvert(m_view);
			m_inverseProjection = safeInvert(m_projection);
		};

		/// <summary>
		/// Translates from clip-space to world-space
		/// </summary>
		/// <param name="clipSpacePos"> Clip-space position </param>
		/// <returns> World-space position </returns>
		inline Vector3 ClipToWorldSpace(const Vector3& clipSpacePos)const {
			const Vector4 viewPos = m_inverseProjection * Vector4(clipSpacePos, 1.0f);
			return Vector3(m_inversePose * (viewPos / viewPos.w));
		}

		/// <summary>
		/// Translates from world-space to clip-space
		/// </summary>
		/// <param name="worldSpacePos"> Clip-space position </param>
		/// <returns> World-space position </returns>
		inline Vector3 WorldToClipSpace(const Vector3& worldSpacePos)const {
			const Vector4 clipPos = m_projection * m_view * Vector4(worldSpacePos, 1.0f);
			return clipPos / clipPos.w;
		}

	private:
		// View matrix
		Matrix4 m_view;

		// Projection matrix
		Matrix4 m_projection;

		// Inverse view matrix
		Matrix4 m_inversePose;

		// Inverse projection matrix
		Matrix4 m_inverseProjection;
	};
}
