#include "ShapeHandles.h"
#include <Data/Generators/MeshGenerator.h>
#include <Data/Generators/MeshModifiers.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> SPHERE = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.05f, 16, 8);

			static const Reference<TriMesh> ARROW = []() -> Reference<TriMesh> {
				const Reference<TriMesh> base = GenerateMesh::Tri::Box(Vector3(-0.01f, -0.01f, 0.0f), Vector3(0.01f, 0.01f, 0.5f), "Arrow");
				const Reference<TriMesh> cone = GenerateMesh::Tri::Cone(Vector3(0.0f, 0.5f, 0.0f), 0.125f, 0.05f, 8);
				const Matrix4 rotation = Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
				const Reference<TriMesh> orientedCone = ModifyMesh::Transformed(rotation, cone);
				return ModifyMesh::Merge(base, orientedCone, "Arrow");
			}();

			static const Reference<TriMesh> PLANE = GenerateMesh::Tri::Box(Vector3(0.0f, 0.0f, -0.0025f), Vector3(0.15f, 0.15f, 0.0025f), "Plane");
		}

		Reference<DragHandle> FreeMoveSphereHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name);
			Object::Instantiate<MeshRenderer>(handle, "Renderer", SPHERE)->SetMaterialInstance(
				SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			return handle;
		}

		Reference<DragHandle> FixedAxisArrowHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name, DragHandle::Flags::DRAG_Z);
			Object::Instantiate<MeshRenderer>(handle, "Renderer", ARROW)->SetMaterialInstance(
				SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			return handle;
		}

		Reference<DragHandle> FixedPlanehandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name, DragHandle::Flags::DRAG_XY);
			Object::Instantiate<MeshRenderer>(handle, "Renderer", PLANE)->SetMaterialInstance(
				SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			return handle;
		}
	}
}
