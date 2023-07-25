#pragma once
#include "../Math.h"
#include "../../Core/JimaraApi.h"
#include "../../Core/Function.h"
#include <vector>

namespace Jimara {
	/// <summary>
	/// Some utilities for 2d polygons
	/// </summary>
	class JIMARA_API PolygonTools {
	public:
		/// <summary>
		/// Calculates 'Signed area' of a polygon
		/// <para/> Same as area if the polygon is clockwise; area times -1 if it is counter-clockwise
		/// </summary>
		/// <param name="vertices"> Polygon vertices </param>
		/// <param name="vertexCount"> Polygon vertex count </param>
		/// <returns> Signed area </returns>
		static float SignedArea(const Vector2* vertices, size_t vertexCount);

		/// <summary>
		/// Checks if the polygon is clockwise or counterclockwise
		/// </summary>
		/// <param name="vertices"> Polygon vertices </param>
		/// <param name="vertexCount"> Polygon vertex count </param>
		/// <returns> True, if the polygon is clockwise </returns>
		static bool IsClockwise(const Vector2* vertices, size_t vertexCount) { return SignedArea(vertices, vertexCount) > 0.0f; }

		/// <summary>
		/// Triangulates a simple polygon
		/// <para/> Triangles will always be reported in clockwise order
		/// </summary>
		/// <param name="vertices"> Polygon vertices </param>
		/// <param name="vertexCount"> Polygon vertex count </param>
		/// <param name="reportTriangle"> Callback for reporting triangle vertex indices </param>
		static void Triangulate(const Vector2* vertices, size_t vertexCount, const Callback<size_t, size_t, size_t>& reportTriangle);

		/// <summary>
		/// Triangulates a simple polygon
		/// <para/> Triangles will always be reported in clockwise order		
		/// </summary>
		/// <param name="vertices"> Polygon vertices </param>
		/// <param name="vertexCount"> Polygon vertex count </param>
		/// <returns> Vector, consisting of vertex index triplets, making a triangle </returns>
		static std::vector<size_t> Triangulate(const Vector2* vertices, size_t vertexCount);

	private:
		// PolygonTools is just a namespace-like thing with some private static internals; contructor is disabled!
		PolygonTools() = delete;

		// Triangulation tools are behind this struct..
		struct Triangulator;
	};
}
