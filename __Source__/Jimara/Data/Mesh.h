#pragma once
#include "../Core/Object.h"
#include "../Math/Math.h"
#include "../OS/Logging/Logger.h"
#include <utility>
#include <vector>
#include <string>


namespace Jimara {
	template<typename VertexType, typename FaceType>
	class Mesh : public virtual Object {
	public:
		inline Mesh(const std::string& name = "") : m_name(name) {}

		inline virtual ~Mesh() {}

		inline Mesh(const Mesh& other) 
			: m_name(other.m_name), m_vertices(other.m_vertices), m_faces(other.m_faces) {}

		inline Mesh& operator=(const Mesh& other) {
			m_name = other.m_name;
			m_vertices = other.m_vertices;
			m_faces = other.m_faces;
			return (*this);
		}

		inline Mesh(Mesh&& other) noexcept
			: m_name(std::move(other.m_name)), m_vertices(std::move(other.m_vertices)), m_faces(std::move(other.m_faces)) {}

		inline Mesh& operator=(Mesh&& other) noexcept {
			m_name = std::move(other.m_name);
			m_vertices = std::move(other.m_vertices);
			m_faces = std::move(other.m_faces);
			return (*this);
		}


		inline std::string& Name() { return m_name; }

		inline const std::string& Name()const { return m_name; }


		inline uint32_t VertCount()const { return static_cast<uint32_t>(m_vertices.size()); }

		inline VertexType& Vert(uint32_t index) { return m_vertices[index]; }

		inline VertexType Vert(uint32_t index)const { return m_vertices[index]; }

		inline Mesh& AddVert(const VertexType& vert) { m_vertices.push_back(vert); return (*this); }


		inline uint32_t FaceCount()const { return static_cast<uint32_t>(m_faces.size()); }
		
		inline FaceType& Face(uint32_t index) { return m_faces[index]; }

		inline FaceType Face(uint32_t index)const { return m_faces[index]; }

		inline Mesh& AddFace(const FaceType& face) { m_faces.push_back(face); return (*this); }


	private:
		std::string m_name;
		std::vector<VertexType> m_vertices;
		std::vector<FaceType> m_faces;
	};





	struct MeshVertex {
		alignas(16) Vector3 position;
		alignas(16) Vector3 normal;
		alignas(16) Vector2 uv;
	};

	struct TriangleFace {
		alignas(4) uint32_t a;
		alignas(4) uint32_t b;
		alignas(4) uint32_t c;
	};

	class TriMesh : public virtual Mesh<MeshVertex, TriangleFace> {
	public:
		TriMesh(const std::string& name);

		virtual ~TriMesh();

		TriMesh(const Mesh<MeshVertex, TriangleFace>& other);

		TriMesh& operator=(const Mesh<MeshVertex, TriangleFace>& other);

		TriMesh(Mesh<MeshVertex, TriangleFace>&& other)noexcept;

		TriMesh& operator=(Mesh<MeshVertex, TriangleFace>&& other)noexcept;


		static Reference<TriMesh> FromOBJ(const std::string& filename, const std::string& objectName, OS::Logger* logger = nullptr);

		static std::vector<Reference<TriMesh>> FromOBJ(const std::string& filename, OS::Logger* logger = nullptr);

		static Reference<TriMesh> Box(const Vector3& start, const Vector3& end, const std::string& name = "");

		static Reference<TriMesh> Sphere(uint32_t segments, uint32_t rings, float radius, const std::string& name = "");
	};
}
