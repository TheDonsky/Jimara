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

			/// <summary> Virtual destructor </summary>
			virtual ~UITransform();

			/// <summary> Canvas from parent hierarchy </summary>
			inline UI::Canvas* Canvas()const { return m_canvas; }

			/// <summary> Percentile anchor rectangle within the parent Canvas/UITransform </summary>
			inline Rect AnchorRect()const { return m_anchorRect; }

			/// <summary>
			/// Sets anchor rectangle
			/// </summary>
			/// <param name="anchors"> Anchor rect </param>
			inline void SetAnchorRect(const Rect& anchors) { m_anchorRect = anchors; }

			/// <summary> Fractional offset from the anchor center </summary>
			inline Vector2 AnchorOffset()const { return m_anchorOffset; }

			/// <summary>
			/// Sets anchor offset
			/// </summary>
			/// <param name="offset"> Fractional offset from the anchor center </param>
			inline void SetAnchorOffset(const Vector2& offset) { m_anchorOffset = offset; }

			/// <summary> 
			/// Size of the additional covered area around the anchor rect 
			/// (If BorderOffset is zero, basically double the size of the actual border) 
			/// </summary>
			inline Vector2 BorderSize()const { return m_borderSize; }

			/// <summary>
			/// Sets cumulative border size
			/// </summary>
			/// <param name="size"> Border size to use </param>
			inline void SetBorderSize(const Vector2& size) { m_borderSize = size; }

			/// <summary> Fractional offset of the border expansion pivot point </summary>
			inline Vector2 BorderOffset()const { return m_borderOffset; }

			/// <summary>
			/// Sets border size
			/// </summary>
			/// <param name="offset"> Fractional offset of the border expansion pivot point </param>
			inline void SetBorderOffset(const Vector2& offset) { m_borderOffset = offset; }

			/// <summary> "Flat" position offset in local space </summary>
			inline Vector2 Offset()const { return m_offset; }

			/// <summary>
			/// Sets local position offset
			/// </summary>
			/// <param name="offset"> "Flat" offset </param>
			inline void SetOffset(const Vector2& offset) { m_offset = offset; }

			/// <summary> Local rotation angle (degress) </summary>
			inline float Rotation()const { return m_rotation; }

			/// <summary>
			/// Sets local rotation
			/// </summary>
			/// <param name="rotation"> Local rotation angle (degress) </param>
			inline void SetRotation(float rotation) { m_rotation = rotation; }

			/// <summary> Local scale </summary>
			inline Vector2 LocalScale()const { return m_scale; }

			/// <summary>
			/// Sets local scale
			/// </summary>
			/// <param name="scale"> Local scale </param>
			inline void SetLocalScale(const Vector2& scale) { m_scale = scale; }

			/// <summary>
			/// UI transform pose
			/// </summary>
			struct JIMARA_API UIPose {
				/// <summary> Center offset from the canvas center point </summary>
				Vector2 center = Vector2(0.0f);

				/// <summary> 'Right' direction in canvas space </summary>
				Vector2 right = Vector2(1.0f, 0.0f);

				/// <summary> Size of the pose rectangle in canvas space </summary>
				Vector2 size = Vector2(1920.0f, 1080.0f);

				/// <summary> 'Up' direction in canvas space </summary>
				inline Vector2 Up()const { return Vector2(-right.y, right.x); }
			};

			/// <summary> Current pose relative to the canvas center </summary>
			UIPose Pose()const;

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		private:
			// Percentile anchor rectangle within the parent Canvas/UITransform
			Rect m_anchorRect = Rect(Vector2(0.0f), Vector2(0.0f));

			// Fractional offset from the anchor center
			Vector2 m_anchorOffset = Vector2(0.0f);

			// Size of the additional covered area around the anchor rect
			Vector2 m_borderSize = Vector2(128.0f, 128.0f);

			// Fractional offset of the border expansion pivot point
			Vector2 m_borderOffset = Vector2(0.0f);

			// "Flat" position offset in local space
			Vector2 m_offset = Vector2(0.0f);

			// Local rotation angle (degress)
			float m_rotation = 0.0f;

			// Local scale
			Vector2 m_scale = Vector2(1.0f);

			// Canvas from parent hierarchy
			Reference<UI::Canvas> m_canvas;

			// Some private functionality resides here...
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UITransform>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UITransform>(const Callback<const Object*>& report);
}
