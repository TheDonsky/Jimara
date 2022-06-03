#include "HandleProperties.h"


namespace Jimara {
	namespace Editor {
		class HandleProperties::Cache : public virtual ObjectCache<Reference<const Object>>{
		public:
			inline static Reference<HandleProperties> GetFor(EditorContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false, [&]()->Reference<HandleProperties> {
					Reference<HandleProperties> instance = new HandleProperties();
					instance->ReleaseRef();
					context->AddStorageObject(instance);
					return instance;
					});
			}
		};

		Reference<HandleProperties> HandleProperties::Of(EditorContext* context) {
			return Cache::GetFor(context);
		}

		float HandleProperties::HandleSizeFor(const GizmoViewport* viewport, const Vector3& position) {
			const Transform* viewportTransform = viewport->ViewportTransform();
			const Size2 resolution = viewport->Resolution();
			const float FieldOfView = viewport->FieldOfView();
			const float heightPerDistance = std::tan(Math::Radians(FieldOfView * 0.5f)) * 2.0f;
			const float pixelsPerDistance = heightPerDistance * static_cast<float>(resolution.y);
			if (pixelsPerDistance < 1.0f) return 0.0f;
			const float sizePerDistance = m_handleSize.load() / pixelsPerDistance;
			const Vector3 delta = position - viewportTransform->WorldPosition();
			const float distance = Math::Dot(viewportTransform->Forward(), delta);
			return sizePerDistance * distance;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::HandleProperties>() {
		// __TODO__: Implement this crap!
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::HandleProperties>() {
		// __TODO__: Implement this crap!
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::HandleProperties>(const Callback<const Object*>& report) {
		// __TODO__: Implement this crap!
	}
}
