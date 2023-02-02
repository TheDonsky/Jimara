#include "UITransform.h"


namespace Jimara {
	namespace UI {
		struct UITransform::Helpers {
			inline static void SetCanvasReference(UITransform* self, UI::Canvas* canvas) {
				if (self->m_canvas == canvas) return;

				const Callback<Component*> onCanvasDestroyed(Helpers::OnCanvasDestroyed, self);
				
				if (self->m_canvas != nullptr) 
					self->m_canvas->OnDestroyed() -= onCanvasDestroyed;
				
				self->m_canvas = canvas;
				
				if (self->m_canvas != nullptr) 
					self->m_canvas->OnDestroyed() += onCanvasDestroyed;
			}

			inline static void FindCanvasReference(UITransform* self) { SetCanvasReference(self, self->GetComponentInParents<UI::Canvas>()); }
			inline static void ClearCanvasReference(UITransform* self) { SetCanvasReference(self, nullptr); }

			inline static void OnTransformParentChanged(UITransform* self, ParentChangeInfo) { 
				FindCanvasReference(self); 
			}
			inline static void OnCanvasDestroyed(UITransform* self, Component*) { 
				FindCanvasReference(self); 
			}
			inline static void OnTransformDestroyed(UITransform* self, Component*) { 
				ClearCanvasReference(self); 
			}
		};

		UITransform::UITransform(Component* parent, const std::string_view& name)
			: Component(parent, name) {
			OnParentChanged() += Callback<ParentChangeInfo>(Helpers::OnTransformParentChanged, this);
			OnDestroyed() += Callback<Component*>(Helpers::OnTransformDestroyed, this);
			Helpers::FindCanvasReference(this);
		}

		UITransform::~UITransform() {
			OnParentChanged() -= Callback<ParentChangeInfo>(Helpers::OnTransformParentChanged, this);
			OnDestroyed() -= Callback<Component*>(Helpers::OnTransformDestroyed, this);
			Helpers::ClearCanvasReference(this);
		}

		UI::Canvas* UITransform::Canvas() { return m_canvas; }

		void UITransform::SetPivot(const Vector2& pivot) {
			m_pivot = pivot;
			// __TODO__: Implement this crap...
		}

		void UITransform::SetAnchorRect(const Rect& anchors) {
			m_anchorRect = anchors;
			// __TODO__: Implement this crap...
		}

		void UITransform::SetBorderSize(const Vector2& size) {
			m_borderSize = size;
			// __TODO__: Implement this crap...
		}

		void UITransform::SetBorderOffset(const Vector2& offset) {
			m_borderOffset = offset;
			// __TODO__: Implement this crap...
		}

		void UITransform::SetRotation(float rotation) {
			m_rotation = rotation;
			// __TODO__: Implement this crap...
		}

		void UITransform::SetLocalScale(const Vector2& scale) {
			m_scale = scale;
			// __TODO__: Implement this crap...
		}

		void UITransform::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			// __TODO__: Implement this crap...
		}
	}
}
