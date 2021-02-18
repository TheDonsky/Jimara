#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"
#include "../OS/Logging/Logger.h"
#include <utility>
#include <vector>
#include <string>


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
		inline Mesh(const std::string& name = "") : m_name(name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Mesh() {}

		/// <summary>
		/// Copy-constructor
		/// </summary>
		/// <param name="other"> Mesh to copy data from </param>
		inline Mesh(const Mesh& other) 
			: m_name(other.m_name), m_vertices(other.m_vertices), m_faces(other.m_faces) {}

		/// <summary>
		/// Copy-assignment
		/// </summary>
		/// <param name="other"> Mesh to copy from </param>
		/// <returns> self </returns>
		inline Mesh& operator=(const Mesh& other) {
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


		/// <summary> Mesh name </summary>
		inline std::string& Name() { return m_name; }

		/// <summary> Mesh name </summary>
		inline const std::string& Name()const { return m_name; }


		/// <summary> Number of mesh vertices </summary>
		inline uint32_t VertCount()const { return static_cast<uint32_t>(m_vertices.size()); }

		/// <summary>
		/// Mesh vertex by index
		/// </summary>
		/// <param name="index"> Vertex index [valid from 0 to VertCount()] </param>
		/// <returns> Index'th vertex </returns>
		inline VertexType& Vert(uint32_t index) { return m_vertices[index]; }

		/// <summary>
		/// Mesh vertex by index
		/// </summary>
		/// <param name="index"> Vertex index [valid from 0 to VertCount()] </param>
		/// <returns> Index'th vertex </returns>
		inline const VertexType& Vert(uint32_t index)const { return m_vertices[index]; }

		/// <summary>
		/// Adds a vertex to the mesh
		/// </summary>
		/// <param name="vert"> Vertex to add </param>
		/// <returns> self </returns>
		inline Mesh& AddVert(const VertexType& vert) { m_vertices.push_back(vert); return (*this); }


		/// <summary> Number of mesh faces </summary>
		inline uint32_t FaceCount()const { return static_cast<uint32_t>(m_faces.size()); }
		
		/// <summary>
		/// Mesh face by index
		/// </summary>
		/// <param name="index"> Mesh face index [valid from 0 to FaceCount()] </param>
		/// <returns> Index'th face </returns>
		inline FaceType& Face(uint32_t index) { return m_faces[index]; }

		/// <summary>
		/// Mesh face by index
		/// </summary>
		/// <param name="index"> Mesh face index [valid from 0 to FaceCount()] </param>
		/// <returns> Index'th face </returns>
		inline const FaceType& Face(uint32_t index)const { return m_faces[index]; }

		/// <summary>
		/// Adds a face to the mesh
		/// </summary>
		/// <param name="face"> Face to add </param>
		/// <returns> self </returns>
		inline Mesh& AddFace(const FaceType& face) { m_faces.push_back(face); return (*this); }


	private:
		// Mesh name
		std::string m_name;

		// Vertices
		std::vector<VertexType> m_vertices;

		// Faces
		std::vector<FaceType> m_faces;
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
		TriMesh(const std::string& name);

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
		/// Loads a mesh from a wavefront obj file
		/// </summary>
		/// <param name="filename"> .obj file name </param>
		/// <param name="objectName"> Name of an individual object within the file </param>
		/// <param name="logger"> Logger for error & warning reporting </param>
		/// <returns> Instance of a loaded mesh (nullptr, if failed) </returns>
		static Reference<TriMesh> FromOBJ(const std::string& filename, const std::string& objectName, OS::Logger* logger = nullptr);

		/// <summary>
		/// Loads all meshes from a wavefront obj file
		/// </summary>
		/// <param name="filename"> .obj file name </param>
		/// <param name="logger"> Logger for error & warning reporting </param>
		/// <returns> List of all objects within the file (empty, if failed) </returns>
		static std::vector<Reference<TriMesh>> FromOBJ(const std::string& filename, OS::Logger* logger = nullptr);

		/// <summary>
		/// Generates an axis aligned bounding box
		/// </summary>
		/// <param name="start"> "Left-bottom-closest" point </param>
		/// <param name="end"> "Right-top-farthest" point </param>
		/// <param name="name"> Name of the object </param>
		/// <returns> Box-shaped mesh instance </returns>
		static Reference<TriMesh> Box(const Vector3& start, const Vector3& end, const std::string& name = "Box");

		/// <summary>
		/// Generates a spherical mesh
		/// </summary>
		/// <param name="center"> Mesh center </param>
		/// <param name="radius"> Sphere radius </param>
		/// <param name="segments"> Radial segment count </param>
		/// <param name="rings"> Horizontal ring count </param>
		/// <param name="name"> ame of the object </param>
		/// <returns> Sphere-shaped mesh instance </returns>
		static Reference<TriMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string& name = "Sphere");

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
		inline static Reference<TriMesh> ShadeFlat(const TriMesh* mesh) { return ShadeFlat(mesh, mesh->Name()); }
	};
}
