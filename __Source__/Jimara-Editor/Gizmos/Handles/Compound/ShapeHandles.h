#pragma once
#include "../FreeMoveHandle.h"
#include "../FixedAxisMoveHandle.h"
#include <Data/Generators/MeshGenerator.h>
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		inline Reference<FreeMoveHandle> FreeMoveSphereHandle(Component* parent, const std::string_view& name) {
			Reference<FreeMoveHandle> handle = Object::Instantiate<FreeMoveHandle>(parent, name);
			static const Reference<TriMesh> mesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.035f, 32, 16);
			Object::Instantiate<MeshRenderer>(handle, "Renderer", mesh);
			return handle;
		}

		inline Reference<FixedAxisMoveHandle> FixedAxisArrowHandle(Component* parent, const std::string_view& name) {
			Reference<FixedAxisMoveHandle> handle = Object::Instantiate<FixedAxisMoveHandle>(parent, name);
			static const Reference<TriMesh> mesh = GenerateMesh::Tri::Box(Vector3(-0.01f, -0.01f, 0.0f), Vector3(0.01f, 0.01f, 0.5f));
			Object::Instantiate<MeshRenderer>(handle, "Renderer", mesh);
			return handle;
		}
	}
}
