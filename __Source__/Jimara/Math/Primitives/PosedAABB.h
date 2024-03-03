#pragma once
#include "Tetrahedron.h"


namespace Jimara {
	/// <summary>
	/// Bounding box with an arbitrary transform
	/// </summary>
	struct JIMARA_API PosedAABB {
		/// <summary> Axis-aligned bounding box </summary>
		AABB bbox = {};

		/// <summary> BBox transform </summary>
		Matrix4 pose = Math::Identity();

		/// <summary> Bounding box of the transformed bbox </summary>
		inline AABB BoundingBox()const { return pose * bbox; }

		/// <summary>
		/// Checks overlap between a PosedAABB and a regular AABB
		/// <para/> Exact hit point and volume are not calculated for now, at least when it comes to geometric accuracy
		/// </summary>
		/// <param name="boundingBox"> Axis-aligned bounding box </param>
		/// <returns> Basic overlap info </returns>
		inline Math::ShapeOverlapResult<PosedAABB, AABB> Overlap(const AABB& boundingBox)const {

			static const auto checkOverlap = [](
				const AABB& shape, const Matrix4& transform, const AABB box, const auto& toWorldSpace) -> Math::ShapeOverlapResult<PosedAABB, AABB> {
				Math::ShapeOverlapResult<PosedAABB, AABB> rv;
				rv.volume = 0.0f;
				rv.center = (box.start + box.end) * 0.5f;

				size_t numOverlaps = 0u;
				auto check = [&](const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
					Tetrahedron(a, b, c, d).CheckOverlap(box, [&](Tetrahedron t) {
						t = toWorldSpace(t);
						const float volume = t.Volume();
						const Vector3 center = t.Center();
						if (volume > std::numeric_limits<float>::epsilon()) {
							rv.volume += volume;
							rv.center = Math::Lerp(rv.center, center, volume / rv.volume);
						}
						else if (rv.volume < std::numeric_limits<float>::epsilon())
							rv.center = Math::Lerp(rv.center, center, 1.0f / float(numOverlaps + 1u));
						numOverlaps++;
						return false;
						});
				};

				const Vector3 a = transform * Vector4(shape.start.x, shape.start.y, shape.start.z, 1u);
				const Vector3 b = transform * Vector4(shape.start.x, shape.end.y, shape.start.z, 1u);
				const Vector3 c = transform * Vector4(shape.start.x, shape.end.y, shape.end.z, 1u);
				const Vector3 d = transform * Vector4(shape.start.x, shape.start.y, shape.end.z, 1u);
				const Vector3 e = transform * Vector4(shape.end.x, shape.start.y, shape.start.z, 1u);
				const Vector3 f = transform * Vector4(shape.end.x, shape.end.y, shape.start.z, 1u);
				const Vector3 g = transform * Vector4(shape.end.x, shape.end.y, shape.end.z, 1u);
				const Vector3 h = transform * Vector4(shape.end.x, shape.start.y, shape.end.z, 1u);

				check(a, b, c, e);
				check(a, c, d, e);
				check(b, c, f, e);
				check(c, g, f, e);
				check(d, c, g, e);
				check(g, g, h, e);

				if (numOverlaps <= 0u)
					return {};
				else return rv;
			};
			const Math::ShapeOverlapResult<PosedAABB, AABB> ab = checkOverlap(bbox, pose, boundingBox, 
				[&](const Tetrahedron& t) { return t; });
			if (static_cast<Math::ShapeOverlapVolume>(ab))
				return ab;
			else return checkOverlap(boundingBox, Math::Inverse(pose), bbox, 
				[&](const Tetrahedron& t) { return pose * t; });
		}

		/// <summary>
		/// Casts a ray on the posed AABB
		/// </summary>
		/// <param name="rayOrigin"> Ray origin point </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Raycast info </returns>
		inline Math::RaycastResult<AABB> Raycast(const Vector3& rayOrigin, const Vector3& direction)const {
			const Matrix4 inverseTransform = Math::Inverse(pose);
			const Math::RaycastResult<AABB> res = Math::Raycast(bbox,
				Vector3(inverseTransform * Vector4(rayOrigin, 1.0f)),
				Math::Normalize(Vector3(inverseTransform * Vector4(direction, 0.0f))));
			if (!std::isfinite(res.distance))
				return {};
			Math::RaycastResult<AABB> rv;
			rv.hitPoint = pose * Vector4(res.hitPoint, 1.0f);
			rv.distance = Math::Magnitude(rv.hitPoint - rayOrigin);
			return rv;
		}
	};

	namespace Math {
		/// <summary>
		/// Shape overlap override for AABB and PosedAABB pair
		/// <para/> Exact hit point and volume are not calculated for now, at least when it comes to geometric accuracy
		/// </summary>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="posedBBox"> Posed bounding box </param>
		/// <returns> Overlap info </returns>
		template<>
		inline Math::ShapeOverlapResult<AABB, PosedAABB> Overlap<AABB, PosedAABB>(const AABB& bbox, const PosedAABB& posedBBox) {
			return posedBBox.Overlap(bbox);
		}
	}
}
