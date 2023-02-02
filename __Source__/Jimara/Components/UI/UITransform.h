#pragma once
#include "Canvas.h"


namespace Jimara {
	namespace UI {
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

			UI::Canvas* Canvas();

			inline Vector2 Pivot()const { return m_pivot; }

			void SetPivot(const Vector2& pivot);

			inline Rect AnchorRect()const { return m_anchorRect; }

			void SetAnchorRect(const Rect& anchors);

			inline Vector2 BorderSize()const { return m_borderSize; }

			void SetBorderSize(const Vector2& size);

			inline Vector2 BorderOffset()const { return m_borderOffset; }

			void SetBorderOffset(const Vector2& offset);

			inline float Rotation()const { return m_rotation; }

			void SetRotation(float rotation);

			inline Vector2 LocalScale()const { return m_scale; }

			void SetLocalScale(const Vector2& scale);

			struct Transformation {
				Vector2 position;
				Vector2 right;
				Vector2 size;
				inline Vector2 Up()const { return Vector2(-right.y, right.x); }
			};

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		private:
			Vector2 m_pivot = Vector2(0.0f);

			Rect m_anchorRect = Rect();

			Vector2 m_borderSize = Vector2(128.0f, 128.0f);

			Vector2 m_borderOffset = Vector2(0.0f);

			float m_rotation = 0.0f;

			Vector2 m_scale = Vector2(1.0f);

			Reference<UI::Canvas> m_canvas;

			// Some private functionality resides here...
			struct Helpers;
		};
	}
}
