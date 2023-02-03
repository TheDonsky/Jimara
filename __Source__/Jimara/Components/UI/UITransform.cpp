#include "UITransform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


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

		UITransform::UIPose UITransform::Pose()const {
			static thread_local std::vector<const UITransform*> chain;

			// 'Gather' the chain of transform:
			{
				chain.clear();
				const Component* ptr = this;
				while (true) {
					const Component* parent = ptr->Parent();
					if (parent == nullptr || parent == ptr || parent == m_canvas) break;
					const UITransform* node = dynamic_cast<const UITransform*>(ptr);
					if (node != nullptr) chain.push_back(node);
					ptr = parent;
				}
			}

			// Default pose with correct size:
			UIPose pose = {};
			if (m_canvas != nullptr)
				pose.size = m_canvas->Size();
			Vector2 cumulativeScale = Vector2(1.0f);
			
			// Calculate actual pose:
			{
				const UITransform** ptr = chain.data();
				const UITransform** const end = ptr + chain.size();
				while (ptr < end) {
					const UITransform* node = *ptr;
					ptr++;

					const Vector2 scale = node->m_scale;
					cumulativeScale *= scale;

					const Vector2 anchorStart = pose.size * node->m_anchorRect.start;
					const Vector2 anchorEnd = pose.size * node->m_anchorRect.end;
					const Vector2 anchorCenter = (anchorStart + anchorEnd) * 0.5f;
					const Vector2 anchorSize = (anchorEnd - anchorStart) * scale;
					const Vector2 anchorOffset = anchorSize * node->m_anchorOffset;

					const Vector2 offset = cumulativeScale * node->m_offset;
					const Vector2 borderSize = cumulativeScale * node->m_borderSize;
					const Vector2 borderOffset = borderSize * node->m_borderOffset;

					const Vector2 center = anchorCenter + anchorOffset + offset + borderOffset;
					const Vector2 size = anchorSize + borderSize;

					const float angle = Math::Degrees(node->m_rotation);
					const Vector2 right = Vector2(std::cos(angle), std::sin(angle));

					pose.center += (pose.right * center.x) + (pose.Up() * center.y);
					pose.right = (pose.right * right.x) + (pose.Up() * right.y);
					pose.size = size;
				}
				pose.right = Math::Normalize(pose.right);
			}

			chain.clear();
			return pose;
		}

		void UITransform::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_anchorRect.start, "Anchor Min", "Anchor rectangle start");
				JIMARA_SERIALIZE_FIELD(m_anchorRect.end, "Anchor max", "Anchor rectangle end");
				JIMARA_SERIALIZE_FIELD_GET_SET(AnchorOffset, SetAnchorOffset, "Anchor Offset", "Fractional offset from the anchor center");

				JIMARA_SERIALIZE_FIELD_GET_SET(BorderSize, SetBorderSize, "Border Size", "Size of the additional covered area around the anchor rect");
				JIMARA_SERIALIZE_FIELD_GET_SET(BorderOffset, SetBorderOffset, "Border Offset", "Fractional offset of the border expansion pivot point");

				JIMARA_SERIALIZE_FIELD_GET_SET(Offset, SetOffset, "Offset", "Flat position offset in local space");
				
				JIMARA_SERIALIZE_FIELD_GET_SET(Rotation, SetRotation, "Rotation", "Rotation angle (degress)");
				JIMARA_SERIALIZE_FIELD_GET_SET(LocalScale, SetLocalScale, "Scale", "Local scale");
			};
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<UI::UITransform>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<UI::UITransform> serializer("Jimara/UI/UITransform", "UITransform");
		report(&serializer);
	}
}
