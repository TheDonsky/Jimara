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
		/// <summary> Type definition for VertexType </summary>
		typedef VertexType Vertex;

		/// <summary> Type definition for FaceType </summary>
		typedef FaceType Face;

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
	typedef Mesh<MeshVertex, PolygonFace> PolyMesh;

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
	typedef Mesh<MeshVertex, TriangleFace> TriMesh;


	/// <summary> Translates PolyMesh into TriMesh </summary>
	Reference<TriMesh> ToTriMesh(const PolyMesh* polyMesh);

	/// <summary> Translates TriMesh into PolyMesh </summary>
	Reference<PolyMesh> ToPolyMesh(const TriMesh* triMesh);
}
