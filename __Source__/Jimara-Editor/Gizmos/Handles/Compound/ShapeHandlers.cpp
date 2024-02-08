#include "ShapeHandles.h"
#include <Jimara/Data/Geometry/MeshGenerator.h>
#include <Jimara/Data/Geometry/MeshModifiers.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>

namespace Jimara {
	namespace Editor {
		namespace {
			static const Reference<TriMesh> SPHERE = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.1f, 16, 8);

			static const Reference<TriMesh> ARROW = []() -> Reference<TriMesh> {
				const Reference<TriMesh> base = GenerateMesh::Tri::Box(Vector3(-0.02f, -0.02f, 0.0f), Vector3(0.02f, 0.02f, 1.0f), "Arrow");
				const Reference<TriMesh> cone = GenerateMesh::Tri::Cone(Vector3(0.0f, 1.0f, 0.0f), 0.25f, 0.1f, 8);
				const Matrix4 rotation = Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
				const Reference<TriMesh> orientedCone = ModifyMesh::Transform(cone, rotation);
				return ModifyMesh::Merge(base, orientedCone, "Arrow");
			}();

			static const Reference<TriMesh> PLANE = GenerateMesh::Tri::Box(Vector3(0.0f, 0.0f, -0.0025f), Vector3(0.3f, 0.3f, 0.0025f), "Plane");
		}

		Reference<DragHandle> FreeMoveSphereHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name);
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", SPHERE);
			renderer->SetMaterialInstance(SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
			return handle;
		}

		Reference<DragHandle> FixedAxisArrowHandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name, DragHandle::Flags::DRAG_Z);
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", ARROW);
			renderer->SetMaterialInstance(SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
			return handle;
		}

		Reference<DragHandle> FixedPlanehandle(Component* parent, Vector4 color, const std::string_view& name) {
			if (parent == nullptr) return nullptr;
			Reference<DragHandle> handle = Object::Instantiate<DragHandle>(parent, name, DragHandle::Flags::DRAG_XY);
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(handle, "Renderer", PLANE);
			renderer->SetMaterialInstance(SampleDiffuseShader::MaterialInstance(parent->Context()->Graphics()->Device(), color));
			renderer->SetLayer(static_cast<Layer>(GizmoLayers::HANDLE));
			return handle;
		}
	}
}
