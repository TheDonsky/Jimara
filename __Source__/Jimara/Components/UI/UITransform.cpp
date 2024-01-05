#include "UITransform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace UI {
		UITransform::UITransform(Component* parent, const std::string_view& name)
			: Component(parent, name) {}

		UITransform::~UITransform() {}

		UITransform::UIPose UITransform::Pose()const {
			static thread_local std::vector<const UITransform*> chain;
			const Canvas* canvas = nullptr;

			// 'Gather' the chain of transform:
			{
				chain.clear();
				const Component* ptr = this;
				while (true) {
					const UITransform* node = dynamic_cast<const UITransform*>(ptr);
					if (node != nullptr) 
						chain.push_back(node);
					canvas = dynamic_cast<const Canvas*>(ptr);
					const Component* parent = ptr->Parent();
					if (parent == nullptr || parent == ptr || canvas != nullptr)
						break;
					ptr = parent;
				}
			}

			// Default pose with correct size:
			UIPose pose = {};
			if (canvas != nullptr)
				pose.size = canvas->Size();
			
			// Calculate actual pose:
			{
				const UITransform** const end = chain.data();
				const UITransform** ptr = end + chain.size();
				while (ptr > end) {
					ptr--;
					const UITransform* node = *ptr;
					const Vector2 scale = node->m_scale;

					const float angle = Math::Radians(node->m_rotation);
					const Vector2 right = Vector2(std::cos(angle), std::sin(angle));
					const Vector2 up = Vector2(-right.y, right.x);

					const Vector2 anchorStart = pose.size * node->m_anchorRect.start;
					const Vector2 anchorEnd = pose.size * node->m_anchorRect.end;
					const Vector2 anchorCenter = (anchorStart + anchorEnd) * 0.5f;
					const Vector2 anchorSize = (anchorEnd - anchorStart);
					const Vector2 anchorOffsetSize = anchorSize * node->m_anchorOffset * scale;
					const Vector2 anchorOffset = right * anchorOffsetSize.x + up * anchorOffsetSize.y;

					const Vector2 offset = node->m_offset;
					const Vector2 borderSize = node->m_borderSize;
					const Vector2 borderOffsetSize = borderSize * node->m_borderOffset * scale;
					const Vector2 borderOffset = right * borderOffsetSize.x + up * borderOffsetSize.y;

					const Vector2 centerOffset = anchorOffset + offset + borderOffset;
					const Vector2 center = anchorCenter + centerOffset;
					const Vector2 size = anchorSize + borderSize;

					const Vector2 r = pose.right;
					pose.center += (r * center.x) + (pose.up * center.y);
					pose.right = (((r * right.x) + (pose.up * right.y)) * scale.x);
					pose.up = (((r * up.x) + (pose.up * up.y)) * scale.y);
					pose.size = size;
				}
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

				JIMARA_SERIALIZE_FIELD_GET_SET(BorderSize, SetBorderSize, "Border Size", 
					"Size of the additional covered area around the anchor rect (If BorderOffset is zero, basically double the size of the actual border)");
				JIMARA_SERIALIZE_FIELD_GET_SET(BorderOffset, SetBorderOffset, "Border Offset", "Fractional offset of the border expansion pivot point");

				JIMARA_SERIALIZE_FIELD_GET_SET(Offset, SetOffset, "Offset", "\"Flat\" position offset in local space");
				
				JIMARA_SERIALIZE_FIELD_GET_SET(Rotation, SetRotation, "Rotation", "Local rotation angle (degress)");
				JIMARA_SERIALIZE_FIELD_GET_SET(LocalScale, SetLocalScale, "Scale", "Local scale");
			};
		}

		Vector2 UITransform::UIPose::CanvasToLocalSpacePosition(const Vector2& canvasPos)const {
			const Vector2 scale = Scale();
			if (std::abs(scale.x * scale.y) <= std::numeric_limits<float>::epsilon())
				return Vector2(std::numeric_limits<float>::quiet_NaN());
			const Vector2 offset = canvasPos - center;
			const Vector2 r = right / scale.x;
			const Vector2 u = up / scale.y;
			const float cosA = Math::Dot(r, u);
			if (std::abs(cosA) >= (1.0f - std::numeric_limits<float>::epsilon()))
				return Vector2(std::numeric_limits<float>::quiet_NaN());
			const Vector2 proj = Vector2(Math::Dot(r, offset), Math::Dot(u, offset));
			const float x = (proj.x - cosA * proj.y) / (1.0f - cosA * cosA);
			const float y = proj.y - cosA * x;
			return Vector2(x / scale.x, y / scale.y);
		}

		Vector2 UITransform::UIPose::LocalToCanvasSpacePosition(const Vector2& localPos)const {
			return center + localPos.x * right + localPos.y * up;
		}

		bool UITransform::UIPose::Overlaps(const Vector2& canvasPos)const {
			const Vector2 localPosition = CanvasToLocalSpacePosition(canvasPos);
			return
				(!std::isnan(localPosition.x)) && (!std::isnan(localPosition.y)) &&
				std::abs(localPosition.x) < std::abs(size.x * 0.5f) &&
				std::abs(localPosition.y) <= std::abs(size.y * 0.5f);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<UI::UITransform>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UITransform>(
			"UI Transform", "Jimara/UI/UITransform", "HUD Transform");
		report(factory);
	}
}
