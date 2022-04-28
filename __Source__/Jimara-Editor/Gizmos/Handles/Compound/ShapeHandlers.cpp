#include "ShapeHandles.h"
#include <Data/Generators/MeshGenerator.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		Reference<DragHandle> FreeMoveSphereHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name);
			static const Reference<TriMesh> mesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.05f, 32, 16);
			Object::Instantiate<MeshRenderer>(handle, "Renderer", mesh)->SetMaterialInstance(
				SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			return handle;
		}

		Reference<DragHandle> FixedAxisArrowHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name, DragHandle::Flags::DRAG_Z);
			static const Reference<TriMesh> mesh = [&]() -> Reference<TriMesh> {
				const Reference<TriMesh> base = GenerateMesh::Tri::Box(Vector3(-0.01f, -0.01f, 0.0f), Vector3(0.01f, 0.01f, 0.5f), "Arrow");
				const Reference<TriMesh> cone = GenerateMesh::Tri::Cone(Vector3(0.0f, 0.5f, 0.0f), 0.125f, 0.05f, 8);
				const Matrix4 rotation = Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
				TriMesh::Writer writer(base);
				TriMesh::Reader reader(cone);
				const uint32_t baseVerts = writer.VertCount();
				for (uint32_t i = 0; i < reader.VertCount(); i++) {
					const MeshVertex& vertex = reader.Vert(i);
					writer.AddVert(MeshVertex(rotation * Vector4(vertex.position, 1.0f), rotation * Vector4(vertex.normal, 0.0f), vertex.uv));
				}
				for (uint32_t i = 0; i < reader.FaceCount(); i++) {
					const TriangleFace& face = reader.Face(i);
					writer.AddFace(TriangleFace(face.a + baseVerts, face.b + baseVerts, face.c + baseVerts));
				}
				return base;
			}();
			Object::Instantiate<MeshRenderer>(handle, "Renderer", mesh)->SetMaterialInstance(
				SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			return handle;
		}
	}
}
