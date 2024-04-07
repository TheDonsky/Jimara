#pragma once
#include "Triangle.h"
#include "PosedAABB.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// A simple sphere (a glorified radius value)
	/// </summary>
	struct Sphere {
		/// <summary> Radius </summary>
		float radius;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="r"> radius </param>
		inline Sphere(float r = 0.0f) : radius(r) {}

		/// <summary> Sphere bounding box </summary>
		inline AABB BoundingBox()const {
			const float absR = std::abs(radius);
			return AABB(Vector3(-absR), Vector3(absR));
		}
	};

	namespace Math {
		/// <summary>
		/// Performs a sphere-sweep operation on an axis aligned bounding box
		/// </summary>
		/// <param name="sphere"> Swept sphere </param>
		/// <param name="bbox"> Axis aligned bounding box </param>
		/// <param name="position"> Initial position of the sphere </param>
		/// <param name="direction"> Sweep direction </param>
		/// <returns> Sweep result/data </returns>
		template<>
		inline SweepResult<Sphere, AABB> Sweep(const Sphere& sphere, const AABB& bbox, const Vector3& position, const Vector3& direction) {
			float entryTime = 0.0f;
			float exitTime = std::numeric_limits<float>::infinity();
			Vector3 hitPoint = position;
			auto incorporatePlanes = [&](const Vector3& normal, const Vector3& rangeStart, const Vector3& rangeEnd) {
				float startDist = Math::Dot(normal, rangeStart - position) - sphere.radius;
				float endDist = Math::Dot(normal, rangeEnd - position) + sphere.radius;
				float speed = Math::Dot(direction, normal);
				if (std::abs(speed) > std::numeric_limits<float>::epsilon()) {
					if (speed < 0.0f)
						std::swap(startDist, endDist);
					const float entryT = startDist / speed;
					if (entryT > entryTime) {
						entryTime = entryT;
						hitPoint = position + entryT * direction + normal * ((speed >= 0.0f) ? sphere.radius : -sphere.radius);
					}
					exitTime = Math::Min(exitTime, endDist / speed);
				}
				else if ((startDist * endDist) > 0.0f)
					exitTime = -1.0f;
			};
			incorporatePlanes(Vector3(1.0f, 0.0f, 0.0f), bbox.start, bbox.end);
			incorporatePlanes(Vector3(0.0f, 1.0f, 0.0f), bbox.start, bbox.end);
			incorporatePlanes(Vector3(0.0f, 0.0f, 1.0f), bbox.start, bbox.end);
			if (entryTime < exitTime)
				return SweepResult<Sphere, AABB>(entryTime, hitPoint);
			else return {};
		}


		/// <summary>
		/// Performs a sphere-sweep operation on a posed axis aligned bounding box
		/// </summary>
		/// <param name="sphere"> Swept sphere </param>
		/// <param name="bbox"> Posed bounding box </param>
		/// <param name="position"> Initial position of the sphere </param>
		/// <param name="direction"> Sweep direction </param>
		/// <returns> Sweep result/data </returns>
		template<>
		inline SweepResult<Sphere, PosedAABB> Sweep(const Sphere& sphere, const PosedAABB& bbox, const Vector3& position, const Vector3& direction) {
			float entryTime = 0.0f;
			float exitTime = std::numeric_limits<float>::infinity();
			Vector3 hitPoint = position;
			auto incorporatePlanes = [&](const Vector3& normal, const Vector3& rangeStart, const Vector3& rangeEnd) {
				float startDist = Math::Dot(normal, rangeStart - position) - sphere.radius;
				float endDist = Math::Dot(normal, rangeEnd - position) + sphere.radius;
				float speed = Math::Dot(direction, normal);
				if (std::abs(speed) > std::numeric_limits<float>::epsilon()) {
					if (speed < 0.0f)
						std::swap(startDist, endDist);
					const float entryT = startDist / speed;
					if (entryT > entryTime) {
						entryTime = entryT;
						hitPoint = position + entryT * direction + normal * ((speed >= 0.0f) ? sphere.radius : -sphere.radius);
					}
					exitTime = Math::Min(exitTime, endDist / speed);
				}
				else if ((startDist * endDist) > 0.0f)
					exitTime = -1.0f;
			};

			const Vector3 a = bbox.pose * Vector4(bbox.bbox.start.x, bbox.bbox.start.y, bbox.bbox.start.z, 1u);
			const Vector3 b = bbox.pose * Vector4(bbox.bbox.start.x, bbox.bbox.end.y, bbox.bbox.start.z, 1u);
			const Vector3 c = bbox.pose * Vector4(bbox.bbox.start.x, bbox.bbox.end.y, bbox.bbox.end.z, 1u);
			const Vector3 d = bbox.pose * Vector4(bbox.bbox.start.x, bbox.bbox.start.y, bbox.bbox.end.z, 1u);
			const Vector3 e = bbox.pose * Vector4(bbox.bbox.end.x, bbox.bbox.start.y, bbox.bbox.start.z, 1u);

			auto incorporate = [&](const Vector3& tA, const Vector3& tB, const Vector3& tC, const Vector3& rangeEnd) {
				Vector3 normal = Math::Normalize(Math::Cross(tB - tA, tC - tA));
				if (Math::Dot(rangeEnd - a, normal) < 0.0f)
					normal = -normal;
				incorporatePlanes(normal, tA, rangeEnd);
			};
			incorporate(a, b, c, e);
			incorporate(a, b, e, d);
			incorporate(a, d, e, b);

			if (entryTime < exitTime)
				return SweepResult<Sphere, PosedAABB>(entryTime, hitPoint);
			else return {};
		}

		/// <summary>
		/// Performs a sphere-sweep operation on a 3d triangle
		/// </summary>
		/// <param name="sphere"> Swept sphere </param>
		/// <param name="tri"> Triangle </param>
		/// <param name="position"> Initial position of the sphere </param>
		/// <param name="direction"> Sweep direction </param>
		/// <returns> Sweep result/data </returns>
		template<>
		inline SweepResult<Sphere, Triangle3> Sweep(const Sphere& sphere, const Triangle3& tri, const Vector3& position, const Vector3& direction) {
			const float radius = std::abs(sphere.radius);
			if (std::abs(radius) < std::numeric_limits<float>::epsilon())
				return Raycast(tri, position, direction);

			const float speed = Math::Magnitude(direction);
			const Vector3 dir = direction / speed;

			const Vector3& a = tri.a;
			const Vector3& b = tri.b;
			const Vector3& c = tri.c;
			const Vector3 normal = Normalize(Cross(b - a, c - a));
			
			const float dirProjection = Math::Dot(direction, normal);
			const float deltaProjection = Math::Dot(a - position, normal);
			
			float startTime;
			if (std::abs(dirProjection) < std::numeric_limits<float>::epsilon()) {
				if (std::abs(deltaProjection) >= radius)
					return {};
				startTime = 0.0f;
				//radius = std::sqrt((radius * radius) - (deltaProjection * deltaProjection));
			}
			else {
				const float halfTime = std::abs(radius / dirProjection);
				const float centerTime = deltaProjection / dirProjection;
				const float endTime = centerTime + halfTime;
				if (endTime <= 0.0f)
					return {};
				const float entryTime = centerTime - halfTime;
				startTime = Math::Max(entryTime, 0.0f);
			}
			
			// Check if the touch point is inside the triangle:
			{
				const Vector3 startCenter = position + direction * startTime;
				const Vector3 touchPoint = startCenter - normal * Math::Dot(normal, startCenter - a);
				const ShapeOverlapVolume volume = Overlap(tri, touchPoint);
				if (volume)
					return SweepResult<Sphere, Triangle3>(startTime, touchPoint);
			}

			// Inspecting potential hits:
			float bestTime = std::numeric_limits<float>::infinity();
			std::optional<Vector3> bestPoint;
			auto tryImproveDistance = [&](const Vector3& pos) {
				const Vector3 delta = pos - position;
				const float posDistance = Dot(dir, delta);
				const float r = Math::Magnitude(delta - dir * posDistance);
				if (r >= radius)
					return;
				const float deltaDistance = std::sqrt((radius * radius) - (r * r));

				// Overlap happens between t0 and t1;
				const float t0 = (posDistance - deltaDistance) / speed;
				const float t1 = (posDistance + deltaDistance) / speed;

				float t;
				if (t0 < startTime) {
					// We may have started touching in 'past'/negative direction...
					if (t1 < startTime)
						return;
					else t = startTime;
				}
				else t = t0;

				if (bestTime < t)
					return;

				bestTime = t;
				bestPoint = pos;
			};

			// Examine potential touch points:
			auto findEdgeTouchPoint = [&](const Vector3& tA, const Vector3& tB) {
				const Vector3 delta = (tB - tA);
				const Vector3 offset = (tA - position);

				const float deltaSqr = Math::Dot(delta, delta);
				if (deltaSqr <= std::numeric_limits<float>::epsilon()) {
					// Zero-length edge.. Point still can get hit though:
					tryImproveDistance(tA);
					return;
				}
				
				// Math below does not cover the case when delta is parallel to the direction vector. This is an edge-case:
				{
					const Vector3 edgeDir = delta / std::sqrt(deltaSqr);
					const float match = Dot(dir, edgeDir);
					if (std::abs(match) > 0.99999f) {
						const float aT = Dot(offset, dir);
						const float bT = Dot(tB - position, dir);
						if ((aT * bT) >= 0.0f)
							tryImproveDistance(std::abs(aT) < std::abs(bT) ? tA : tB);
						else tryImproveDistance(Lerp(tA, tB, aT / (aT - bT)));
						return;
					}
				}

				/*
				magnitude((tA + p * delta) - (position + t * direction)) = radius => 
				=> magnitude(offset + p * delta - t * direction) = radius =>
				=> pow(offset.x + p * delta.x - t * direction.x, 2) + same_for.yZ = rSqr =>
				*/
				const float rSqr = (radius * radius);

				/*
				=> (offset.x * offset.x) + (p * (delta.x * offset.x)) - (t * (direction.x * offset.x)) +
				(p * (delta.x * offset.x)) + ((p * p) * (delta.x * delta.x)) - ((p * t) * (delta.x * direction.x)) +
				- (t * (direction.x * offset.x)) - ((p * t) * (delta.x * direction.x)) + (t * t) * (direction.x * direction.x) + same_for.yz = rSqr =>
				*/
				const float dirSqr = Math::Dot(direction, direction);
				const float offsetSqr = Math::Dot(offset, offset);
				const float deltaDotOffset = Math::Dot(delta, offset);
				const float dirDotOffset = -Math::Dot(direction, offset);
				const float dirDotDelta = -Math::Dot(direction, delta);

				/*
				=> offsetSqr + (2 * p * deltaDotOffset) + (2 * t * dirDotOffset) + ((p * p) * deltaSqr) + (2 * (p * t) * dirDotDelta) + ((t * t) * dirSqr) = rSqr =>
				=> deltaSqr * (p * p) + 2 * (t * dirDotDelta + deltaDotOffset) * p + ((t * t) * dirSqr + 2 * t * dirDotOffset + offsetSqr - rSqr) = 0; =>
				=> {
					A = deltaSqr;
					B = 2 * (t * dirDotDelta + deltaDotOffset);
					C = ((t * t) * dirSqr + 2 * t * dirDotOffset + offsetSqr - rSqr);
					D = B * B - 4 * A * C;
					p = ((+/-)sqrt(D) - B) / (2 * A) => pAvg = -B / (2 * A);
				} => If we know minimal positive t, such that D >= 0, we can find p as well =>
				D => 0 => pow(2 * (t * dirDotDelta + deltaDotOffset), 2) - 4 * deltaSqr * ((t * t) * dirSqr + 2 * t * dirDotOffset + offsetSqr - rSqr) >= 0 =>
				=> pow(t * dirDotDelta + deltaDotOffset, 2) - (((t * t) * dirSqr * deltaSqr) + (t * 2 * dirDotOffset * deltaSqr) + ((offsetSqr - rSqr) * deltaSqr)) >= 0 =>
				=> (t * t) * (dirDotDelta * dirDotDelta) + t * 2 * dirDotDelta * deltaDotOffset + (deltaDotOffset * deltaDotOffset) -
				(((t * t) * dirSqr * deltaSqr) + (t * 2 * dirDotOffset * deltaSqr) + ((offsetSqr - rSqr) * deltaSqr)) >= 0 =>
				*/
				const float a0 = (dirDotDelta * dirDotDelta) - (dirSqr * deltaSqr);
				const float b0 = 2.0f * ((dirDotDelta * deltaDotOffset) - (dirDotOffset * deltaSqr));
				const float c0 = (deltaDotOffset * deltaDotOffset) - ((offsetSqr - rSqr) * deltaSqr);

				/*
				=> (t * t) * a0 + t * b0 + c0 >= 0 =>
				=> Since we are working with physical objects, it's guaranteed that the touch starts at some time point and ends at some other time point.
				=> Therefore, it's more or less obvious the valid/touch time interval starts at one solution to the equasion and ends at the second.
				*/
				const float d0 = (b0 * b0) - (4.0f * a0 * c0);
				if (d0 < 0.0f || std::abs(a0) < std::numeric_limits<float>::epsilon())
					return; // No touch!
				const float sqrtD0 = std::sqrt(d0);
				const float t0 = 0.5f * (sqrtD0 - b0) / a0;
				const float t1 = 0.5f * (-sqrtD0 - b0) / a0;
				const float tMin = Min(t0, t1);
				const float tMax = Max(t0, t1);
				if (tMax < 0.0f)
					return; // Touch happens in the "past"!
				const float t = Max(0.0f, tMin);

				const float A = deltaSqr;
				const float B = 2.0f * (t * dirDotDelta + deltaDotOffset);
				const float C = ((t * t) * dirSqr + 2 * t * dirDotOffset + offsetSqr - rSqr);
				const float sqrtD = std::sqrt(Max(B * B - 4 * A * C, 0.0f)); // Max with 0.0f to make sure there's no floating point error...
				const float p0 = 0.5f * (sqrtD - B) / A;
				const float p1 = 0.5f * (-sqrtD - B) / A;
				const float pMin = Min(p0, p1);
				const float pMax = Max(p0, p1);

				if (pMin > 1.0f || pMax < 0.0f) {
					// No touch at that point, but we should check if either of the endpoints gets hit:
					tryImproveDistance(tA);
					tryImproveDistance(tB);
					return;
				}
				
				if (t >= bestTime)
					return; // Better time point has already been found!

				// Update result:
				const float p = (Math::Max(pMin, 0.0f) + Math::Min(pMax, 1.0f)) * 0.5f;
				bestTime = t;
				bestPoint = Math::Lerp(tA, tB, p);
			};
			findEdgeTouchPoint(a, b);
			findEdgeTouchPoint(b, c);
			findEdgeTouchPoint(c, a);

			if (bestPoint.has_value())
				return SweepResult<Sphere, Triangle3>(bestTime, bestPoint.value());
			else return {};
		}
	}
}
