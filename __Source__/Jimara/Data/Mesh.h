#pragma once
#include "../Core/Object.h"
#include "../Core/Systems/Event.h"
#include "../Core/Collections/Stacktor.h"
#include "../Math/Math.h"
#include "../OS/Logging/Logger.h"
#include <map>
#include <utility>
#include <vector>
#include <string>
#include <cstdint>
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

			// Copy not allowed:
			inline Reader(const Reader&) = delete;
			inline Reader& operator=(const Reader&) = delete;

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

			/// <summary> Virtual destructor </summary>
			inline virtual ~Reader() {}

			/// <summary> "Target" mesh </summary>
			inline const Mesh* Target()const { return m_mesh; }

			/// <summary> Mesh name </summary>
			inline const std::string& Name()const { return m_mesh->m_name; }

			/// <summary> Number of mesh vertices </summary>
			inline uint32_t VertCount()const { return static_cast<uint32_t>(m_mesh->m_vertices.size()); }

			/// <summary>
			/// Mesh vertex by index
			/// </summary>
			/// <param name="index"> Vertex index [valid from 0 to VertCount()] </param>
			/// <returns> Index'th vertex </returns>
			inline const VertexType& Vert(uint32_t index)const { return m_mesh->m_vertices[index]; }

			/// <summary> Number of mesh faces </summary>
			inline uint32_t FaceCount()const { return static_cast<uint32_t>(m_mesh->m_faces.size()); }

			/// <summary>
			/// Mesh face by index
			/// </summary>
			/// <param name="index"> Mesh face index [valid from 0 to FaceCount()] </param>
			/// <returns> Index'th face </returns>
			inline const FaceType& Face(uint32_t index)const { return m_mesh->m_faces[index]; }
		};

		/// <summary> Utility for writing mesh data with a decent amount of thread-safety </summary>
		class Writer {
		private:
			// Mesh to read data from
			const Reference<Mesh> m_mesh;

			// Copy not allowed:
			inline Writer(const Writer&) = delete;
			inline Writer& operator=(const Writer&) = delete;

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

			/// <summary> "Target" mesh </summary>
			inline Mesh* Target()const { return m_mesh; }

			/// <summary> Mesh name </summary>
			inline std::string& Name()const { return m_mesh->m_name; }

			/// <summary> Number of mesh vertices </summary>
			inline uint32_t VertCount()const { return static_cast<uint32_t>(m_mesh->m_vertices.size()); }

			/// <summary>
			/// Mesh vertex by index
			/// </summary>
			/// <param name="index"> Vertex index [valid from 0 to VertCount()] </param>
			/// <returns> Index'th vertex </returns>
			inline VertexType& Vert(uint32_t index)const { return m_mesh->m_vertices[index]; }

			/// <summary>
			/// Adds a Vertex to the mesh
			/// </summary>
			/// <param name="vertex"> Vertex to append </param>
			inline void AddVert(const VertexType& vertex)const { m_mesh->m_vertices.push_back(vertex); }

			/// <summary>
			/// Adds a Vertex to the mesh
			/// </summary>
			/// <param name="vertex"> Vertex to append </param>
			inline void AddVert(VertexType&& vertex)const { m_mesh->m_vertices.push_back(std::move(vertex)); }

			/// <summary> Removes last vertex (faces that include it will not be automatically fixed) </summary>
			inline void PopVert()const { m_mesh->m_vertices.pop_back(); }

			/// <summary> Number of mesh faces </summary>
			inline uint32_t FaceCount()const { return static_cast<uint32_t>(m_mesh->m_faces.size()); }

			/// <summary>
			/// Mesh face by index
			/// </summary>
			/// <param name="index"> Mesh face index [valid from 0 to FaceCount()] </param>
			/// <returns> Index'th face </returns>
			inline FaceType& Face(uint32_t index)const { return m_mesh->m_faces[index]; }

			/// <summary>
			/// Adds a Face to the mesh
			/// </summary>
			/// <param name="face"> Face to append </param>
			inline void AddFace(const FaceType& face)const { m_mesh->m_faces.push_back(face); }

			/// <summary>
			/// Adds a Face to the mesh
			/// </summary>
			/// <param name="face"> Face to append </param>
			inline void AddFace(FaceType&& face)const { m_mesh->m_faces.push_back(std::move(face)); }

			/// <summary> Removes last face </summary>
			inline void PopFace()const { m_mesh->m_faces.pop_back(); }
		};
		
		/// <summary> Invoked, whenever a mesh Writer goes out of scope </summary>
		inline Event<const Mesh*>& OnDirty()const { return m_onDirty; }

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
		mutable EventInstance<const Mesh*> m_onDirty;
	};

	/// <summary>
	/// Arbitrary mesh with some skinning information
	/// </summary>
	/// <typeparam name="VertexType"> Type of the mesh vertex </typeparam>
	/// <typeparam name="FaceType"> Type of a mesh face </typeparam>
	/// <typeparam name="BoneDataType"> Type of the information per bone; mostly expected to be a transformation matrix of sorts, representing a reference pose </typeparam>
	template<typename VertexType, typename FaceType, typename BoneDataType>
	class SkinnedMesh : public virtual Mesh<VertexType, FaceType> {
	public:
		/// <summary> Type definition for BoneDataType </summary>
		typedef BoneDataType BoneData;

		/// <summary>
		/// Bone index and corresponding weight pair
		/// </summary>
		struct BoneWeight {
			/// <summary> Bone index </summary>
			alignas(4) uint32_t boneIndex;

			/// <summary> Bone weight (for actual SkinnedMesh objects, this will always be positive; sum per bone weight is not guaranteed to be 1 in order to make writers simple) </summary>
			alignas(4) float boneWeight;

			/// <summary>
			/// Constructor
			/// </summary>
			inline BoneWeight(uint32_t boneId = 0, float weight = 0.0f) : boneIndex(boneId), boneWeight(weight) {}
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Mesh name </param>
		inline SkinnedMesh(const std::string_view& name = "") : Mesh<VertexType, FaceType>(name) {}

		/// <summary> Utility for reading skinned mesh data with a decent amount of thread-safety </summary>
		class Reader : public Mesh<VertexType, FaceType>::Reader {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Reader(const SkinnedMesh* mesh) : Mesh<VertexType, FaceType>::Reader(mesh) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Reader(const SkinnedMesh& mesh) : Reader(&mesh) {}

			/// <summary> "Target" mesh </summary>
			inline const SkinnedMesh* Target()const { return dynamic_cast<const SkinnedMesh*>(Mesh<VertexType, FaceType>::Reader::Target()); }

			/// <summary> Number of bones </summary>
			inline uint32_t BoneCount()const { return static_cast<uint32_t>(Target()->m_boneData.size()); }

			/// <summary>
			/// Bone data by bone index
			/// </summary>
			/// <param name="index"> Bone index [valid from 0 to BoneCount()] </param>
			/// <returns> Data for the bone with given index </returns>
			inline const BoneDataType& BoneData(uint32_t index)const { return Target()->m_boneData[index]; }

			/// <summary>
			/// Number of bone weights for given vertex
			/// Note: Sum of all bone weights is not guaranteed to be 1; That case is up to the user to handle
			/// </summary>
			/// <param name="vertexIndex"> Vertex index [valid from 0 to VertCount()] </param>
			/// <returns> Number of bone weights for the given vertex </returns>
			inline uint32_t WeightCount(uint32_t vertexIndex)const {
				if (Target()->m_boneWeightStartIdPerVertex.size() <= vertexIndex) return 0;
				else return static_cast<uint32_t>(Target()->m_boneWeightStartIdPerVertex[static_cast<size_t>(vertexIndex) + 1u] - Target()->m_boneWeightStartIdPerVertex[vertexIndex]); 
			}

			/// <summary>
			/// Bone id and weight pair for the given Vertex by index
			/// Note: Sum of all bone weights is not guaranteed to be 1; That case is up to the user to handle
			/// </summary>
			/// <param name="vertexIndex"> Vertex index [valid from 0 to VertCount()] </param>
			/// <param name="weightIndex"> Bone&weight index [valid from 0 to WeightCount(vertexIndex)] </param>
			/// <returns> Bone Id and the corresponding weight pair </returns>
			inline const BoneWeight& Weight(uint32_t vertexIndex, uint32_t weightIndex)const {
				return Target()->m_boneWeights[Target()->m_boneWeightStartIdPerVertex[vertexIndex] + weightIndex];
			}
		};

		/// <summary> Utility for writing skinned mesh data with a decent amount of thread-safety </summary>
		class Writer : public Mesh<VertexType, FaceType>::Writer {
		private:
			// Bone weights, stored for modification (we use map, because bones in sorted order are more appropriate for the result)
			typedef std::map<uint32_t, float> BoneWeightMap;
			std::vector<BoneWeightMap> m_boneWeightMappings;

		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Writer(SkinnedMesh* mesh) : Mesh<VertexType, FaceType>::Writer(mesh) {
				m_boneWeightMappings.resize(Mesh<VertexType, FaceType>::Writer::VertCount());
				for (uint32_t vertexIndex = 0; vertexIndex < m_boneWeightMappings.size(); vertexIndex++) {
					BoneWeightMap& mappings = m_boneWeightMappings[vertexIndex];
					const BoneWeight* const mappingsStart = Target()->m_boneWeights.data() + Target()->m_boneWeightStartIdPerVertex[vertexIndex];
					const BoneWeight* const mappingsEnd = Target()->m_boneWeights.data() + Target()->m_boneWeightStartIdPerVertex[static_cast<size_t>(vertexIndex) + 1u];
					for (const BoneWeight* i = mappingsStart; i < mappingsEnd; i++)
						mappings[i->boneIndex] = i->boneWeight;
				}
			}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> Mesh to read data from </param>
			inline Writer(SkinnedMesh& mesh) : Writer(&mesh) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~Writer() {
				SkinnedMesh* target = Target();
				target->m_boneWeights.clear();
				target->m_boneWeightStartIdPerVertex.clear();
				target->m_boneWeightStartIdPerVertex.push_back(0);
				const uint32_t commonCount = min(
					static_cast<uint32_t>(Mesh<VertexType, FaceType>::Writer::VertCount()), static_cast<uint32_t>(m_boneWeightMappings.size()));
				for (uint32_t i = 0; i < commonCount; i++) {
					const BoneWeightMap& mappings = m_boneWeightMappings[i];
					for (BoneWeightMap::const_iterator it = mappings.begin(); it != mappings.end(); ++it)
						if (it->first < BoneCount() && it->second > std::numeric_limits<float>::epsilon()) 
							target->m_boneWeights.push_back(BoneWeight(it->first, it->second));
					target->m_boneWeightStartIdPerVertex.push_back(target->m_boneWeights.size());
				}
				for (uint32_t i = commonCount; i < Mesh<VertexType, FaceType>::Writer::VertCount(); i++)
					target->m_boneWeightStartIdPerVertex.push_back(target->m_boneWeights.size());
			}

			/// <summary> "Target" mesh </summary>
			inline SkinnedMesh* Target()const { return dynamic_cast<SkinnedMesh*>(Mesh<VertexType, FaceType>::Writer::Target()); }

			/// <summary> Number of bones </summary>
			inline uint32_t BoneCount()const { return static_cast<uint32_t>(Target()->m_boneData.size()); }

			/// <summary>
			/// Bone data by bone index
			/// </summary>
			/// <param name="index"> Bone index [valid from 0 to BoneCount()] </param>
			/// <returns> Data for the bone with given index </returns>
			inline BoneDataType& BoneData(uint32_t index)const { return Target()->m_boneData[index]; }

			/// <summary>
			/// Adds a new bone with given bone data
			/// </summary>
			/// <param name="boneData"> Data for the new bone </param>
			inline void AddBone(const BoneDataType& boneData)const { Target()->m_boneData.push_back(boneData); }

			/// <summary> Removes the last bone data </summary>
			inline void PopBone()const { Target()->m_boneData.pop_back(); }

			/// <summary>
			/// Bone weight for the given Vertex and Bone by index
			/// Note: Removing vertices will not erase weight data
			/// </summary>
			/// <param name="vertexIndex"> Vertex index [valid from 0 to VertCount()] </param>
			/// <param name="boneIndex"> Bone index [valid from 0 to BoneCount()] </param>
			/// <returns> Bone weight </returns>
			inline float& Weight(uint32_t vertexIndex, uint32_t boneIndex) {
				while (m_boneWeightMappings.size() <= vertexIndex) m_boneWeightMappings.push_back(BoneWeightMap());
				return m_boneWeightMappings[vertexIndex][boneIndex];
			}
		};


	private:
		// Bone data buffer
		std::vector<BoneDataType> m_boneData;

		// Bone weight buffer
		std::vector<BoneWeight> m_boneWeights;

		// Bone weight regions per vertex are marked such that 
		// m_boneWeights[m_boneWeightStartIdPerVertex[vertexId]] to m_boneWeights[m_boneWeightStartIdPerVertex[vertexId + 1]] represent bones for given vertex
		std::vector<size_t> m_boneWeightStartIdPerVertex;
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
	typedef Stacktor<uint32_t, 4> PolygonFace;

	/// <summary>
	/// Polygonal mesh
	/// </summary>
	typedef Mesh<MeshVertex, PolygonFace> PolyMesh;

	/// <summary>
	/// Skinned Polygonal mesh
	/// </summary>
	typedef SkinnedMesh<MeshVertex, PolygonFace, Matrix4> SkinnedPolyMesh;

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

	/// <summary>
	/// Skinned Triangulated mesh
	/// </summary>
	typedef SkinnedMesh<MeshVertex, TriangleFace, Matrix4> SkinnedTriMesh;


	/// <summary> Translates PolyMesh into TriMesh </summary>
	Reference<TriMesh> ToTriMesh(const PolyMesh* polyMesh);

	/// <summary> Translates TriMesh into PolyMesh </summary>
	Reference<PolyMesh> ToPolyMesh(const TriMesh* triMesh);

	/// <summary> Translates SkinnedPolyMesh/PolyMesh into SkinnedTriMesh (skinning will be transfered if the source mesh is skinned) </summary>
	Reference<SkinnedTriMesh> ToSkinnedTriMesh(const PolyMesh* polyMesh);

	/// <summary> Translates SkinnedTriMesh/TriMesh into SkinnedTriMesh (just creates a clone if triMesh is a skinned mesh; skinning will be transfered if the source mesh is skinned) </summary>
	Reference<SkinnedTriMesh> ToSkinnedTriMesh(const TriMesh* triMesh);

	/// <summary> Translates SkinnedPolyMesh/PolyMesh into SkinnedPolyMesh (just creates a clone if polyMesh is a skinned mesh; skinning will be transfered if the source mesh is skinned) </summary>
	Reference<SkinnedPolyMesh> ToSkinnedPolyMesh(const TriMesh* polyMesh);

	/// <summary> Translates SkinnedTriMesh/TriMesh into SkinnedPolyMesh (skinning will be transfered if the source mesh is skinned) </summary>
	Reference<SkinnedPolyMesh> ToSkinnedPolyMesh(const TriMesh* triMesh);
}
