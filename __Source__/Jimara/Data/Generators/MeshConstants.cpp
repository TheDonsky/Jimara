#include "MeshConstants.h"
#include "MeshGenerator.h"


namespace Jimara {
	namespace MeshContants {
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
			Reference<TriMesh> Sphere() { MeshContants_MeshAsset_Sphere; }
			Reference<TriMesh> WireSphere() {
				static const MeshAsset::CreateFn createFn = []() -> Reference<TriMesh> {
					const Reference<TriMesh> mesh = Object::Instantiate<TriMesh>("WireSphere");
					const TriMesh::Writer writer(mesh);
					auto createArk = [&](Vector3 x, Vector3 y) {
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
					};
					createArk(Math::Right(), Math::Up());
					createArk(Math::Forward(), Math::Up());
					createArk(Math::Right(), Math::Forward());
					return mesh;
				};
				static const Reference<MeshAsset> asset = Object::Instantiate<MeshAsset>(createFn);
				return asset->Load();
			}
			Reference<TriMesh> Capsule() { MeshContants_MeshAsset_Capsule; }
			Reference<TriMesh> Cylinder() { MeshContants_MeshAsset_Cylinder; }
			Reference<TriMesh> Cone() { MeshContants_MeshAsset_Cone; }
			Reference<TriMesh> Torus() { MeshContants_MeshAsset_Torus; }
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
