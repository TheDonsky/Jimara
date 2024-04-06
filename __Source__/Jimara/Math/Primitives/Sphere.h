#pragma once
#include "Triangle.h"
#include "PosedAABB.h"


namespace Jimara {
	struct Sphere {
		float radius;

		inline Sphere(float r = 0.0f) : radius(r) {}

		inline AABB BoundingBox()const {
			const float absR = std::abs(radius);
			return AABB(Vector3(-absR), Vector3(absR));
		}
	};

	namespace Math {
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

		template<>
		inline SweepResult<Sphere, Triangle3> Sweep(const Sphere& sphere, const Triangle3& tri, const Vector3& position, const Vector3& direction) {
			// __TODO__: This is a rough-ish approximation. Make it more accurate down the line!

			float radius = std::abs(sphere.radius);
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
				radius = std::sqrt((radius * radius) - (deltaProjection * deltaProjection));
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

			auto findClosestPointPhase = [&](const Vector3& s, const Vector3& e) -> float {
				const Vector3 delta = e - s;
				const Vector3 deltaS = (s - position);
				const Vector3 deltaE = (e - position);
				float ds, de;

				const float edgeLen = Math::Magnitude(delta);
				if (edgeLen < std::numeric_limits<float>::epsilon())
					return 0.0f; // Zero size edge...
				const Vector3 edgeDir = delta / edgeLen;

				const Vector3 cross = Math::Cross(direction, edgeDir);
				if (Math::SqrMagnitude(cross) <= std::numeric_limits<float>::epsilon()) {
					// Edge is parallel to direction:
					ds = Math::Dot(deltaS, direction);
					de = Math::Dot(deltaE, direction);
				}
				else {
					// Projection plane found; Y is direction and we care about when it's crossed:
					const Vector3 right = Math::Normalize(Math::Cross(direction, cross));
					ds = Math::Dot(deltaS, right);
					de = Math::Dot(deltaE, right);
				}

				if (ds >= 0.0f && de >= 0.0f)
					return (ds <= ds) ? 0.0f : 1.0f;
				else if (ds <= 0.0f && de <= 0.0f)
					return (ds <= de) ? 1.0f : 0.0f;
				else {
					const float d = (ds - de);
					if (std::abs(d) <= std::numeric_limits<float>::epsilon())
						return 0.0f;
					else return ds / d;
				}
			};
			auto findClosestPoint = [&](const Vector3& s, const Vector3& e) {
				return Math::Lerp(s, e, findClosestPointPhase(s, e));
			};
			
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
			const Vector3* bestPoint = nullptr;
			auto tryImproveDistance = [&](const Vector3* pos) {
				const Vector3 delta = (*pos) - position;
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

			// Potential touch points:
			const Vector3& closestAB = findClosestPoint(a, b);
			const Vector3& closestBC = findClosestPoint(b, c);
			const Vector3& closestCA = findClosestPoint(c, a);
			tryImproveDistance(&closestAB);
			tryImproveDistance(&closestBC);
			tryImproveDistance(&closestCA);

			if (bestPoint == nullptr)
				return {};
			else return SweepResult<Sphere, Triangle3>(bestTime, *bestPoint);
		}
	}
}
