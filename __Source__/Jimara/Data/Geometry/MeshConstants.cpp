#include "MeshConstants.h"
#include "MeshGenerator.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace MeshConstants {
		namespace {
			template<typename MeshType>
			class MeshContants_MeshAsset : public Asset::Of<MeshType> {
			public:
				typedef Reference<MeshType>(*CreateFn)();

				inline MeshContants_MeshAsset(CreateFn createFn) 
					: Asset(GUID::Generate()), m_create(createFn) {}

				inline ~MeshContants_MeshAsset() {}

			protected:
				inline virtual Reference<MeshType> LoadItem()final override { return m_create(); }

			private:
				const CreateFn m_create;
			};

			class MeshContants_WireCapsuleCache : public virtual ObjectCache<uint64_t> {
			private:
#pragma warning(disable: 4250)
				class CapsuleMeshAsset
					: public virtual Asset::Of<TriMesh>
					, public virtual ObjectCache<uint64_t>::StoredObject {
					const float m_radius;
					const float m_height;

				public:
					inline CapsuleMeshAsset(float radius, float height)
						: Asset(GUID::Generate()), m_radius(radius), m_height(height) {}

					inline ~CapsuleMeshAsset() {}

				protected:
					inline virtual Reference<TriMesh> LoadItem()final override {
						Reference<TriMesh> mesh = Object::Instantiate<TriMesh>();
						TriMesh::Writer writer(mesh);

						// Name:
						{
							std::stringstream stream;
							stream << "WireCapsule[R=" << m_radius << "; H:" << m_height << "]";
							writer.Name() = stream.str();
						}

						// Connects vertices:
						auto connectVerts = [&](uint32_t a, uint32_t b) {
							writer.AddFace(TriangleFace(a, b, b));
						};

						// Creates ark:
						static const constexpr uint32_t arkDivisions = 32;
						auto addArk = [&](uint32_t first, uint32_t last, Vector3 center, Vector3 up, Vector3 right) {
							static const constexpr float angleStep = Math::Radians(360) / static_cast<float>(arkDivisions);
							auto addVert = [&](uint32_t vertId) {
								const float angle = angleStep * vertId;
								MeshVertex vertex = {};
								vertex.normal = (std::cos(angle) * right) + (std::sin(angle) * up);
								vertex.position = center + (vertex.normal * m_radius);
								vertex.uv = Vector2(0.5f);
								writer.AddVert(vertex);
							};
							addVert(first);
							for (uint32_t vertId = first + 1; vertId < last; vertId++) {
								addVert(vertId);
								connectVerts(writer.VertCount() - 2, writer.VertCount() - 1);
							}
						};

						// Create shape on given 'right' and 'forward' axis:
						const constexpr Vector3 up = Math::Up();
						auto createOutline = [&](Vector3 right) {
							const uint32_t base = writer.VertCount();
							addArk(0, (arkDivisions / 2) + 1, m_height * 0.5f * up, up, right);
							connectVerts(writer.VertCount() - 1, writer.VertCount());
							addArk(arkDivisions / 2, arkDivisions + 1, -m_height * 0.5f * up, up, right);
							connectVerts(writer.VertCount() - 1, base);
						};
						createOutline(Math::Right());
						createOutline(Math::Forward());

						// Create rings on top & bottom:
						auto createRing = [&](float elevation) {
							const uint32_t base = writer.VertCount();
							addArk(0, arkDivisions, elevation * up, Math::Right(), Math::Forward());
							connectVerts(writer.VertCount() - 1, base);
						};
						createRing(m_height * 0.5f);
						createRing(m_height * -0.5f);

						return mesh;
					}
				};
#pragma warning(default: 4250)

			public:
				inline static Reference<TriMesh> GetFor(float radius, float height) {
					static_assert(sizeof(float) == sizeof(uint32_t));
					static_assert((sizeof(uint32_t) << 1) == sizeof(uint64_t));
					uint64_t key;
					{
						float* words = reinterpret_cast<float*>(&key);
						words[0] = radius;
						words[1] = height;
					}
					static MeshContants_WireCapsuleCache cache;
					Reference<CapsuleMeshAsset> asset = cache.GetCachedOrCreate(key, [&]() -> Reference<CapsuleMeshAsset> {
						return Object::Instantiate<CapsuleMeshAsset>(radius, height);
						});
					return asset->Load();
				}
			};

			inline static void MeshConstants_CreateCircle(const TriMesh::Writer& writer, const Vector3& x, const Vector3& y) {
				const constexpr uint32_t segments = 32;
				const constexpr float angleStep = Math::Radians(360.0f) / static_cast<float>(segments);
				const uint32_t base = writer.VertCount();
				auto addVert = [&](uint32_t id) {
					const float angle = angleStep * id;
					MeshVertex vertex = {};
					vertex.position = vertex.normal = (x * std::cos(angle)) + (y * std::sin(angle));
					vertex.uv = Vector2(0.0f);
					writer.AddVert(vertex);
				};
				auto connect = [&](uint32_t a, uint32_t b) {
					writer.AddFace(TriangleFace(a, b, a));
				};
				addVert(0);
				for (uint32_t i = 1; i < segments; i++) {
					addVert(i);
					connect(writer.VertCount() - 2, writer.VertCount() - 1);
				}
				connect(writer.VertCount() - 1, base);
			}
		}

#define MeshContants_MeshAsset_ConstantMeshBody(createFn) \
	static const MeshAsset::CreateFn MeshContants_MeshAsset_ConstantMeshBody_createFn = []() { return MeshContants_MeshAsset_Generate_Namespace::createFn; }; \
	static const Reference<MeshAsset> MeshContants_MeshAsset_ConstantMeshBody_asset = Object::Instantiate<MeshAsset>(MeshContants_MeshAsset_ConstantMeshBody_createFn); \
	return MeshContants_MeshAsset_ConstantMeshBody_asset->Load();

#define MeshContants_MeshAsset_Cube MeshContants_MeshAsset_ConstantMeshBody(Box(Vector3(-0.5f), Vector3(0.5f)))
#define MeshContants_MeshAsset_Sphere MeshContants_MeshAsset_ConstantMeshBody(Sphere(Vector3(0.0f), 1.0f, 32, 16))
#define MeshContants_MeshAsset_Capsule MeshContants_MeshAsset_ConstantMeshBody(Capsule(Vector3(0.0f), 1.0f, 1.0f, 32, 8))
#define MeshContants_MeshAsset_Cylinder MeshContants_MeshAsset_ConstantMeshBody(Cylinder(Vector3(0.0f), 1.0f, 1.0f, 32))
#define MeshContants_MeshAsset_Cone MeshContants_MeshAsset_ConstantMeshBody(Cone(Vector3(0.0f), 1.0f, 1.0f, 32))
#define MeshContants_MeshAsset_Torus MeshContants_MeshAsset_ConstantMeshBody(Torus(Vector3(0.0f), 1.0f, 0.5f, 32, 16))
#define MeshContants_MeshAsset_Plane MeshContants_MeshAsset_ConstantMeshBody(Plane(Vector3(0.0f)))

		namespace Tri {
#define MeshContants_MeshAsset_Generate_Namespace GenerateMesh::Tri
			typedef MeshContants_MeshAsset<TriMesh> MeshAsset;
			Reference<TriMesh> Cube() { MeshContants_MeshAsset_Cube; }
			Reference<TriMesh> WireCube() {
				static const MeshAsset::CreateFn createFn = []() -> Reference<TriMesh> {
					const Reference<TriMesh> mesh = Object::Instantiate<TriMesh>("WireCube");
					const TriMesh::Writer writer(mesh);

					auto addVert = [&](Vector3 pos) {
						MeshVertex vertex = {};
						vertex.position = pos;
						vertex.normal = Math::Normalize(pos);
						vertex.uv = Vector2(0.0f);
						writer.AddVert(vertex);
					};
					
					addVert(Vector3(-0.5f, -0.5f, -0.5f));
					addVert(Vector3(-0.5f, -0.5f, 0.5f));
					addVert(Vector3(0.5f, -0.5f, 0.5f));
					addVert(Vector3(0.5f, -0.5f, -0.5f));

					addVert(Vector3(-0.5f, 0.5f, -0.5f));
					addVert(Vector3(-0.5f, 0.5f, 0.5f));
					addVert(Vector3(0.5f, 0.5f, 0.5f));
					addVert(Vector3(0.5f, 0.5f, -0.5f));

					auto connect = [&](uint32_t a, uint32_t b) {
						writer.AddFace(TriangleFace(a, b, a));
					};

					auto makeQuad = [&](uint32_t base) {
						connect(base, base + 1);
						connect(base + 1, base + 2);
						connect(base + 2, base + 3);
						connect(base + 3, base);
					};
					makeQuad(0);
					makeQuad(4);

					for (uint32_t i = 0; i < 4; i++)
						connect(i, i + 4);

					return mesh;
				};
				static const Reference<MeshAsset> asset = Object::Instantiate<MeshAsset>(createFn);
				return asset->Load();
			}
			Reference<TriMesh> Sphere() { MeshContants_MeshAsset_Sphere; }
			Reference<TriMesh> WireSphere() {
				static const MeshAsset::CreateFn createFn = []() -> Reference<TriMesh> {
					const Reference<TriMesh> mesh = Object::Instantiate<TriMesh>("WireSphere");
					const TriMesh::Writer writer(mesh);
					MeshConstants_CreateCircle(writer, Math::Right(), Math::Up());
					MeshConstants_CreateCircle(writer, Math::Forward(), Math::Up());
					MeshConstants_CreateCircle(writer, Math::Right(), Math::Forward());
					return mesh;
				};
				static const Reference<MeshAsset> asset = Object::Instantiate<MeshAsset>(createFn);
				return asset->Load();
			}
			Reference<TriMesh> Capsule() { MeshContants_MeshAsset_Capsule; }
			Reference<TriMesh> WireCapsule(float radius, float height) {
				return MeshConstants::MeshContants_WireCapsuleCache::GetFor(radius, height);
			}
			Reference<TriMesh> Cylinder() { MeshContants_MeshAsset_Cylinder; }
			Reference<TriMesh> Cone() { MeshContants_MeshAsset_Cone; }
			Reference<TriMesh> Torus() { MeshContants_MeshAsset_Torus; }
			Reference<TriMesh> WireCircle() {
				static const MeshAsset::CreateFn createFn = []() -> Reference<TriMesh> {
					const Reference<TriMesh> mesh = Object::Instantiate<TriMesh>("WireCircle");
					const TriMesh::Writer writer(mesh);
					MeshConstants_CreateCircle(writer, Math::Right(), Math::Up());
					return mesh;
				};
				static const Reference<MeshAsset> asset = Object::Instantiate<MeshAsset>(createFn);
				return asset->Load();
			}
			Reference<TriMesh> Plane() { MeshContants_MeshAsset_Plane; }
#undef MeshContants_MeshAsset_Generate_Namespace
		}

		namespace Poly {
#define MeshContants_MeshAsset_Generate_Namespace GenerateMesh::Poly
			typedef MeshContants_MeshAsset<PolyMesh> MeshAsset;
			Reference<PolyMesh> Cube() { MeshContants_MeshAsset_Cube; }
			Reference<PolyMesh> Sphere() { MeshContants_MeshAsset_Sphere; }
			Reference<PolyMesh> Capsule() { MeshContants_MeshAsset_Capsule; }
			Reference<PolyMesh> Cylinder() { MeshContants_MeshAsset_Cylinder; }
			Reference<PolyMesh> Cone() { MeshContants_MeshAsset_Cone; }
			Reference<PolyMesh> Torus() { MeshContants_MeshAsset_Torus; }
			Reference<PolyMesh> Plane() { MeshContants_MeshAsset_Plane; }
#undef MeshContants_MeshAsset_Generate_Namespace
		}
	}

#undef MeshContants_MeshAsset_ConstantMeshBody
#undef MeshContants_MeshAsset_Sphere
#undef MeshContants_MeshAsset_Capsule
#undef MeshContants_MeshAsset_Cylinder
#undef MeshContants_MeshAsset_Cone
#undef MeshContants_MeshAsset_Torus
#undef MeshContants_MeshAsset_Plane

#undef MeshContants_MeshAsset_CreateBox_Args
}
