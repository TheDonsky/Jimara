#pragma once
#include "../Mesh.h"
#include <stdint.h>


namespace Jimara {
	namespace MeshFromSpline {
		/// <summary>
		/// Point on an arbitrary 3d spline
		/// </summary>
		struct SplineVertex {
			/// <summary> Position in space </summary>
			Vector3 position;

			/// <summary> 'Right' direction, based on spline point rotation </summary>
			Vector3 right;

			/// <summary> 'Up' direction, based on spline point rotation </summary>
			Vector3 up;
		};

		/// <summary> Flags for Mesh generation </summary>
		enum class Flags : uint8_t {
			/// <summary> Spline and ring/shape will be treated as 'open'; no caps will be created </summary>
			NONE = 0,

			/// <summary> Spline will loop around and rings at 0 and (ringCount - 1) will be bridged </summary>
			CLOSE_SPLINE = (1 << 1),

			/// <summary> Ring shape will loop around and vertices at 0 and (ringSegments - 1) will be bridged </summary>
			CLOSE_SHAPE = (1 << 2),

			/// <summary> Both rings and spline will be looped around </summary>
			CLOSE_SPLINE_AND_SHAPE = CLOSE_SPLINE | CLOSE_SHAPE,

			/// <summary> Spline will be treated as open, but Ring will be closed and capping polygons will be created at endpoints </summary>
			CAP_ENDS = (1 << 3) | CLOSE_SHAPE
		};

		/// <summary> For eachh index in range [0, ringCount) this function should return corresponding SplineVertex </summary>
		typedef Function<SplineVertex, uint32_t> SplineCurve;

		/// <summary> For eachh index in range [0, ringSegments) this function should return corresponding RingVertex </summary>
		typedef Function<Vector2, uint32_t> RingCurve;

		/// <summary>
		/// Generates a mesh consisting of rings around a certain spline
		/// </summary>
		/// <param name="spline"> Spline to create rings around </param>
		/// <param name="ringCount"> Number of rings around the spline </param>
		/// <param name="ring"> Ring shape (coes not have to be a circle) </param>
		/// <param name="ringSegments"> Number of ring segments </param>
		/// <param name="flags"> Cap/Close flags </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Sphere-shaped mesh instance </returns>
		Reference<TriMesh> Tri(
			SplineCurve spline, uint32_t ringCount,
			RingCurve ring, uint32_t ringSegments,
			Flags flags, const std::string_view& name);

		/// <summary>
		/// Generates a mesh consisting of rings around a certain spline
		/// </summary>
		/// <param name="spline"> Spline to create rings around </param>
		/// <param name="ringCount"> Number of rings around the spline </param>
		/// <param name="ring"> Ring shape (coes not have to be a circle) </param>
		/// <param name="ringSegments"> Number of ring segments </param>
		/// <param name="flags"> Cap/Close flags </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Sphere-shaped mesh instance </returns>
		Reference<PolyMesh> Poly(
			SplineCurve spline, uint32_t ringCount,
			RingCurve ring, uint32_t ringSegments,
			Flags flags, const std::string_view& name);
	}
}
