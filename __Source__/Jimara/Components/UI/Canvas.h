#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/RenderStack.h"


namespace Jimara {
	/// <summary> Let the system know of the type </summary>
	JIMARA_REGISTER_TYPE(Jimara::UI::Canvas);

	namespace UI {
		/// <summary>
		/// UI Canvas for in-game HUD
		/// </summary>
		class JIMARA_API Canvas : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component [Can not be nullptr] </param>
			/// <param name="name"> Canvas name </param>
			Canvas(Component* parent, const std::string_view& name = "Canvas");

			/// <summary> Virtual destructor </summary>
			virtual ~Canvas();

			/// <summary> Reference 'resolution'/virtual size of the canvas </summary>
			inline Vector2 ReferenceSize()const { return m_referenceResolution; }

			/// <summary>
			/// Sets reference resolution of the canvas
			/// </summary>
			/// <param name="size"> Reference size to use </param>
			inline void SetReferenceSize(const Vector2& size) { m_referenceResolution = Vector2(Math::Max(size.x, 0.0f), Math::Max(size.y, 0.0f)); }

			/// <summary> Value between 0 and 1, indicating whether the virtual size of the canvas scales horizontally(0.0f) or vertically(1.0f) </summary>
			inline float WidthBias()const { return m_widthBias; }

			/// <summary>
			/// Sets width bias
			/// </summary>
			/// <param name="bias"> 0.0f for the canvas size to scale horizontally, 1.0f for pure vertical scaling and values in the middle for interpolation </param>
			inline void SetWidthBias(float bias) { m_widthBias = Math::Max(0.0f, Math::Min(bias, 1.0f)); }

			/// <summary>
			/// Virtual size of the canvas
			/// <para/> Note that this is calculated based on the target render stack resolution, width bias and reference size.
			/// </summary>
			Vector2 Size()const;

			/// <summary> 
			/// Renderer category for render stack 
			/// <para/> Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
			/// </summary>
			inline uint32_t RendererCategory()const { return m_renderCategory; }

			/// <summary>
			/// Sets renderer category for render stack
			/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
			/// </summary>
			/// <param name="category"> Category to set </param>
			inline void SetRendererCategory(uint32_t category) { m_renderCategory = category; }

			/// <summary> 
			/// Renderer priority for render stack 
			/// <para/> Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
			/// </summary>
			inline uint32_t RendererPriority()const { return m_renderPriority; }

			/// <summary>
			/// Sets renderer priority for render stack
			/// <para/> Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
			/// </summary>
			/// <param name="priority"> Priority to set </param>
			inline void SetRendererPriority(uint32_t priority) { m_renderPriority = priority; }

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;


		private:
			// Reference 'resolution'/virtual size of the canvas
			Vector2 m_referenceResolution = Vector2(1920.0f, 1080.0f);

			// Value between 0 and 1, indicating whether the virtual size of the canvas scales horizontally(0.0f) or vertically(1.0f)
			float m_widthBias = 0.0f;

			// Render stack, this canvas is tied to
			Reference<RenderStack> m_renderStack;

			// Renderer category for render stack (higher category will render later)
			uint32_t m_renderCategory = 2048;

			// Renderer priority for render stack (higher priority will render earlier within the same category)
			uint32_t m_renderPriority = 0;

			// Some private functionality resides here...
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::Canvas>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::Canvas>(const Callback<const Object*>& report);
}
