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
