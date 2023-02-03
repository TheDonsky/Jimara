#pragma once
#include "Canvas.h"


namespace Jimara {
	namespace UI {
		/// <summary> Let the system know of the type </summary>
		JIMARA_REGISTER_TYPE(Jimara::UI::UITransform);

		/// <summary>
		/// HUD Transform
		/// </summary>
		class JIMARA_API UITransform : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component [Can not be nullptr] </param>
			/// <param name="name"> Transform name </param>
			UITransform(Component* parent, const std::string_view& name = "Transform");

			virtual ~UITransform();

			inline UI::Canvas* Canvas()const { return m_canvas; }

			inline Rect AnchorRect()const { return m_anchorRect; }

			inline void SetAnchorRect(const Rect& anchors) { m_anchorRect = anchors; }

			inline Vector2 AnchorOffset()const { return m_anchorOffset; }

			inline void SetAnchorOffset(const Vector2& offset) { m_anchorOffset = offset; }

			inline Vector2 BorderSize()const { return m_borderSize; }

			inline void SetBorderSize(const Vector2& size) { m_borderSize = size; }

			inline Vector2 BorderOffset()const { return m_borderOffset; }

			inline void SetBorderOffset(const Vector2& offset) { m_borderOffset = offset; }

			inline Vector2 Offset()const { return m_offset; }

			inline void SetOffset(const Vector2& offset) { m_offset = offset; }

			inline float Rotation()const { return m_rotation; }

			inline void SetRotation(float rotation) { m_rotation = rotation; }

			inline Vector2 LocalScale()const { return m_scale; }

			inline void SetLocalScale(const Vector2& scale) { m_scale = scale; }

			struct JIMARA_API UIPose {
				Vector2 center = Vector2(0.0f);
				Vector2 right = Vector2(1.0f, 0.0f);
				Vector2 size = Vector2(1920.0f, 1080.0f);
				inline Vector2 Up()const { return Vector2(-right.y, right.x); }
			};

			UIPose Pose()const;

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		private:
			Rect m_anchorRect = Rect(Vector2(0.0f), Vector2(0.0f));

			Vector2 m_anchorOffset = Vector2(0.0f);

			Vector2 m_borderSize = Vector2(128.0f, 128.0f);

			Vector2 m_borderOffset = Vector2(0.0f);

			Vector2 m_offset = Vector2(0.0f);

			float m_rotation = 0.0f;

			Vector2 m_scale = Vector2(1.0f);

			Reference<UI::Canvas> m_canvas;

			// Some private functionality resides here...
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UITransform>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UITransform>(const Callback<const Object*>& report);
}
