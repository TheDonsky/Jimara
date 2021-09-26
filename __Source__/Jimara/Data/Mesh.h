#pragma once
#include "../Core/Object.h"
#include "../Core/Systems/Event.h"
#include "../Core/Collections/Stacktor.h"
#include "../Math/Math.h"
#include "../OS/Logging/Logger.h"
#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <shared_mutex>


namespace Jimara {
	/// <summary>
	/// Arbitrary mesh object
	/// </summary>
	/// <typeparam name="VertexType"> Type of the mesh vertex </typeparam>
	/// <typeparam name="FaceType"> Type of a mesh face </typeparam>
	template<typename VertexType, typename FaceType>
	class Mesh : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Mesh name </param>
		inline Mesh(const std::string_view& name = "") : m_name(name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Mesh() {}

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Mesh to copy data from </param>
		inline Mesh(const Mesh& other) { (*this) = other; }

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Mesh to copy from </param>
		/// <returns> self </returns>
		inline Mesh& operator=(const Mesh& other) {
			if (this == (&other)) return (*this);
			Reader reader(other);
			m_name = other.m_name;
			m_vertices = other.m_vertices;
			m_faces = other.m_faces;
			return (*this);
		}

		/// <summary>
		/// Move-constructor
		/// </summary>
		/// <param name="other"> Mesh to move from </param>
		inline Mesh(Mesh&& other) noexcept
			: m_name(std::move(other.m_name)), m_vertices(std::move(other.m_vertices)), m_faces(std::move(other.m_faces)) {}
		
		/// <summary>
		/// Move-assignment
		/// </summary>
		/// <param name="other"> Mesh to move from </param>
		/// <returns> self </returns>
		inline Mesh& operator=(Mesh&& other) noexcept {
			m_name = std::move(other.m_name);
			m_vertices = std::move(other.m_vertices);
			m_faces = std::move(other.m_faces);
			return (*this);
		}


		/// <summary> Utility for reading mesh data with a decent amount of thread-safety </summary>
		class Reader {
		private:
			// Mesh to read data from
			const Reference<const Mesh> m_mesh;
			
			// Reader lock
			const std::shared_lock<std::shared_mutex> m_lock;

		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Reader(const Mesh* mesh) : m_mesh(mesh), m_lock(mesh->m_changeLock) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Reader(const Mesh& mesh) : Reader(&mesh) {}

			/// <summary> Mesh name </summary>
			inline const std::string& Name()const { return m_mesh->m_name; }

			/// <summary> Number of mesh vertices </summary>
			inline size_t VertCount()const { return m_mesh->m_vertices.size(); }

			/// <summary>
			/// Mesh vertex by index
			/// </summary>
			/// <param name="index"> Vertex index [valid from 0 to VertCount()] </param>
			/// <returns> Index'th vertex </returns>
			inline const VertexType& Vert(size_t index)const { return m_mesh->m_vertices[index]; }

			/// <summary> Number of mesh faces </summary>
			inline size_t FaceCount()const { return m_mesh->m_faces.size(); }

			/// <summary>
			/// Mesh face by index
			/// </summary>
			/// <param name="index"> Mesh face index [valid from 0 to FaceCount()] </param>
			/// <returns> Index'th face </returns>
			inline const FaceType& Face(size_t index)const { return m_mesh->m_faces[index]; }
		};

		/// <summary> Utility for writing mesh data with a decent amount of thread-safety </summary>
		class Writer {
		private:
			// Mesh to read data from
			const Reference<Mesh> m_mesh;

		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to write data to </param>
			inline Writer(Mesh* mesh) : m_mesh(mesh) { m_mesh->m_changeLock.lock(); }

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to write data to </param>
			inline Writer(Mesh& mesh) : Writer(&mesh) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~Writer() {
				m_mesh->m_changeLock.unlock();
				m_mesh->m_onDirty(m_mesh);
			}

			/// <summary> Mesh name </summary>
			inline std::string& Name() { return m_mesh->m_name; }

			/// <summary> Access to mesh vertices </summary>
			std::vector<VertexType>& Verts()const { return m_mesh->m_vertices; }

			/// <summary> Access to mesh faces </summary>
			std::vector<FaceType>& Faces()const { return m_mesh->m_faces; }
		};
		
		/// <summary> Invoked, whenever a mesh Writer goes out of scope </summary>
		inline Event<Mesh*>& OnDirty()const { return m_onDirty; }


	private:
		// Mesh name
		std::string m_name;

		// Vertices
		std::vector<VertexType> m_vertices;

		// Faces
		std::vector<FaceType> m_faces;

		// Lock for change synchronisation
		mutable std::shared_mutex m_changeLock;

		// Invoked, whenever a mesh Writer goes out of scope
		mutable EventInstance<Mesh*> m_onDirty;
	};





	/// <summary>
	/// Vertex of a regular old classic mesh with position, normal and texture coordinates
	/// </summary>
	struct MeshVertex {
		/// <summary> Position </summary>
		alignas(16) Vector3 position;

		/// <summary> Normal </summary>
		alignas(16) Vector3 normal;

		/// <summary> Texture coordinates </summary>
		alignas(16) Vector2 uv;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="pos"> Position </param>
		/// <param name="norm"> Normal </param>
		/// <param name="tex"> Texture coordinate </param>
		inline MeshVertex(Vector3 pos = Vector3(), Vector3 norm = Vector3(), Vector2 tex = Vector2())
			: position(pos), normal(norm), uv(tex) {}
	};

	/// <summary>
	/// Index-based face for a polygonal mesh
	/// </summary>
	typedef Stacktor<uint32_t> PolygonFace;

	/// <summary>
	/// Polygonal mesh
	/// </summary>
	class PolyMesh : public virtual Mesh<MeshVertex, PolygonFace> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Mesh name </param>
		PolyMesh(const std::string_view& name);

		/// <summary> Virtual destructor </summary>
		virtual ~PolyMesh();

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Mesh to copy </param>
		PolyMesh(const Mesh<MeshVertex, PolygonFace>& other);

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Mesh to copy </param>
		/// <returns> self </returns>
		PolyMesh& operator=(const Mesh<MeshVertex, PolygonFace>& other);

		/// <summary>
		/// Move-constructor
		/// </summary>
		/// <param name="other"> Mesh to move </param>
		/// <returns> self </returns>
		PolyMesh(Mesh<MeshVertex, PolygonFace>&& other)noexcept;

		/// <summary>
		/// Move-assignment
		/// </summary>
		/// <param name="other"> Mesh to move </param>
		/// <returns> self </returns>
		PolyMesh& operator=(Mesh<MeshVertex, PolygonFace>&& other)noexcept;
	};

	/// <summary>
	/// Index-based face for a regular triangulated mesh
	/// </summary>
	struct TriangleFace {
		/// <summary> First vertex index </summary>
		alignas(4) uint32_t a;

		/// <summary> Second vertex index </summary>
		alignas(4) uint32_t b;

		/// <summary> Third vertex index </summary>
		alignas(4) uint32_t c;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="A"> First vertex index </param>
		/// <param name="B"> Second vertex index </param>
		/// <param name="C"> Third vertex index </param>
		inline TriangleFace(uint32_t A = uint32_t(), uint32_t B = uint32_t(), uint32_t C = uint32_t())
			: a(A), b(B), c(C) {}
	};

	/// <summary>
	/// Triangulated mesh
	/// </summary>
	class TriMesh : public virtual Mesh<MeshVertex, TriangleFace> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Mesh name </param>
		TriMesh(const std::string_view& name);

		/// <summary> Virtual destructor </summary>
		virtual ~TriMesh();

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Mesh to copy </param>
		TriMesh(const Mesh<MeshVertex, TriangleFace>& other);

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Mesh to copy </param>
		/// <returns> self </returns>
		TriMesh& operator=(const Mesh<MeshVertex, TriangleFace>& other);

		/// <summary>
		/// Move-constructor
		/// </summary>
		/// <param name="other"> Mesh to move </param>
		/// <returns> self </returns>
		TriMesh(Mesh<MeshVertex, TriangleFace>&& other)noexcept;

		/// <summary>
		/// Move-assignment
		/// </summary>
		/// <param name="other"> Mesh to move </param>
		/// <returns> self </returns>
		TriMesh& operator=(Mesh<MeshVertex, TriangleFace>&& other)noexcept;

		/// <summary>
		/// Generates an axis aligned bounding box
		/// </summary>
		/// <param name="start"> "Left-bottom-closest" point </param>
		/// <param name="end"> "Right-top-farthest" point </param>
		/// <param name="name"> Name of the object </param>
		/// <returns> Box-shaped mesh instance </returns>
		static Reference<TriMesh> Box(const Vector3& start, const Vector3& end, const std::string_view& name = "Box");

		/// <summary>
		/// Generates a spherical mesh
		/// </summary>
		/// <param name="center"> Mesh center </param>
		/// <param name="radius"> Sphere radius </param>
		/// <param name="segments"> Radial segment count </param>
		/// <param name="rings"> Horizontal ring count </param>
		/// <param name="name"> Name of the object </param>
		/// <returns> Sphere-shaped mesh instance </returns>
		static Reference<TriMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name = "Sphere");

		/// <summary>
		/// Generates a capsule mesh
		/// </summary>
		/// <param name="center"> Mesh center </param>
		/// <param name="radius"> Capsule radius </param>
		/// <param name="midHeight"> Height of the capsule mid section </param>
		/// <param name="segments"> Radial segment count </param>
		/// <param name="tipRings"> Horizontal ring count per upper and lower half-spheres </param>
		/// <param name="midDivisions"> Mid section division count </param>
		/// <param name="name"> Name of the object </param>
		/// <returns> Capsule-shaped mesh instance </returns>
		static Reference<TriMesh> Capsule(
			const Vector3& center, float radius, float midHeight,
			uint32_t segments, uint32_t tipRings, uint32_t midDivisions = 1,
			const std::string_view& name = "Capsule");

		/// <summary>
		/// Generates a flat rectangular mesh
		/// </summary>
		/// <param name="center"> Gemoetric center </param>
		/// <param name="u"> "U" direction (default assumption is "Right" direction; Vector size directly correlates to plane width; U and V define what "Up" is) </param>
		/// <param name="v"> "V" direction (default assumption is "Forward" direction; Vector size directly correlates to plane length; U and V define what "Up" is) </param>
		/// <param name="divisions"> Number of divisions across U and V axis </param>
		/// <param name="name"> Name of the generated mesh </param>
		/// <returns> Plane-shaped mesh instance </returns>
		static Reference<TriMesh> Plane(
			const Vector3& center,
			const Vector3& u = Vector3(1.0f, 0.0f, 0.0f), const Vector3& v = Vector3(0.0f, 0.0f, 1.0f), 
			Size2 divisions = Size2(1, 1), 
			const std::string_view& name = "Plane");

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <param name="name"> Generated mesh name </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		static Reference<TriMesh> ShadeFlat(const TriMesh* mesh, const std::string& name);

		/// <summary>
		/// Takes a mesh and generates another mesh with identical geometry, but shaded flat
		/// </summary>
		/// <param name="mesh"> Source mesh </param>
		/// <returns> Flat-shaded copy of the mesh </returns>
		inline static Reference<TriMesh> ShadeFlat(const TriMesh* mesh) { 
			Reader reader(mesh);
			return ShadeFlat(mesh, reader.Name());
		}
	};


	/// <summary> Translates PolyMesh into TriMesh </summary>
	Reference<TriMesh> ToTriMesh(const PolyMesh* polyMesh);
}
