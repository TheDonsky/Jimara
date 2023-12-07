#pragma once
#include "UITransform.h"


namespace Jimara {
	namespace UI {
		/// <summary> Let the system know about the type </summary>
		JIMARA_REGISTER_TYPE(Jimara::UI::UIClickArea);

		/// <summary>
		/// UI Component, that detects mouse-over UITransform and clicks
		/// </summary>
		class JIMARA_API UIClickArea : public virtual Component {
		public:
			/// <summary>
			/// Flags for area click detection
			/// </summary>
			enum class ClickAreaFlags : uint8_t {
				/// <summary> Empty bitmask </summary>
				NONE = 0u,

				/// <summary> Area click will start if left mouse button is clicked </summary>
				LEFT_BUTTON = 1u,

				/// <summary> Area click will start if right mouse button is clicked </summary>
				RIGHT_BUTTON = (1u << 1u),

				/// <summary> Area click will start if middle mouse button is clicked </summary>
				MIDDLE_BUTTON = (1u << 2u),

				/// <summary> Area click will end without button release if the cursor is no longer on top of the element </summary>
				AUTO_RELEASE_WHEN_OUT_OF_BOUNDS = (1u << 3u)
			};

			/// <summary> Enumeration bitmask attribute for ClickAreaFlags </summary>
			static const Object* ClickAreaFlagsAttribute();

			/// <summary>
			/// Flags for current area click/hover state
			/// </summary>
			enum class StateFlags : uint8_t {
				/// <summary> Empty bitmask (no hover and no click) </summary>
				NONE = 0u,

				/// <summary> 
				/// If this flag is present, the cursor is on top of the area 
				/// (can be missing, even if it's still pressed, but never, if it's not in focus) 
				/// </summary>
				HOVERED = 1u,

				/// <summary> True, while the area is considered as clicked </summary>
				PRESSED = (1u << 1u),

				/// <summary> True for a single frame when the area gets clicked </summary>
				GOT_PRESSED = (1u << 2u),

				/// <summary> True for a single frame when the area gets released </summary>
				GOT_RELEASED = (1u << 3u)
			};

			/// <summary> Currently focused/hovered/clicked area </summary>
			static Reference<UIClickArea> FocusedArea(SceneContext* context);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Image name </param>
			UIClickArea(Component* parent, const std::string_view& name = "UIClickArea");

			/// <summary> Virtual destructor </summary>
			virtual ~UIClickArea();

			/// <summary> Click area flags </summary>
			inline ClickAreaFlags ClickFlags()const { return m_clickFlags; }

			/// <summary>
			/// Sets click area flags
			/// </summary>
			/// <param name="flags"> Flags </param>
			inline void SetClickFlags(ClickAreaFlags flags) { m_clickFlags = flags; }

			/// <summary> Current click state </summary>
			inline StateFlags ClickState()const { return m_stateFlags; }

			/// <summary>
			/// Event, invoked when a cursor starts hovering on top of the clickable area
			/// if nothing prior to this happening is already clicked 
			/// (if there is a previous click, the invokation will be delayed till the click is released)
			/// </summary>
			inline Event<UIClickArea*>& OnFocusEnter() { return m_onFocusEnter; }

			/// <summary>
			/// Event, invoked on each frame after OnFocusEnter() while the area is hovered on, but not clicked
			/// </summary>
			inline Event<UIClickArea*>& OnHovered() { return m_onHovered; }

			/// <summary>
			/// Event, invoked when the area gets clicked
			/// <para/> This is always invoked after OnFocusEnter() even if the events happen on the same frame.
			/// </summary>
			inline Event<UIClickArea*>& OnClicked() { return m_onClicked; }

			/// <summary>
			/// Event, invoked on each frame in-between OnClicked() and OnReleased()
			/// </summary>
			inline Event<UIClickArea*>& OnPressed() { return m_onPressed; }

			/// <summary>
			/// Event, invoked when the mouse button is released or the cursor leaves the boundaries (in case of AUTO_RELEASE_WHEN_OUT_OF_BOUNDS)
			/// <para/> This is always invoked prior to OnFocusExit even if the events happen on the same frame.
			/// </summary>
			inline Event<UIClickArea*>& OnReleased() { return m_onReleased; }

			/// <summary>
			/// Event, invoked when the cursor goes beyond the area boundaries and the area is not in 'pressed' state
			/// </summary>
			inline Event<UIClickArea*>& OnFocusExit() { return m_onFocusExit; }

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;


		protected:
			/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
			virtual void OnComponentEnabled()override;

			/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
			virtual void OnComponentDisabled()override;

		private:
			// Underlying updater
			Reference<Object> m_updater;

			// Click flags
			ClickAreaFlags m_clickFlags = ClickAreaFlags::LEFT_BUTTON;

			// State flags
			StateFlags m_stateFlags = StateFlags::NONE;

			// State events
			EventInstance<UIClickArea*> m_onFocusEnter;
			EventInstance<UIClickArea*> m_onHovered;
			EventInstance<UIClickArea*> m_onClicked;
			EventInstance<UIClickArea*> m_onPressed;
			EventInstance<UIClickArea*> m_onReleased;
			EventInstance<UIClickArea*> m_onFocusExit;

			// Private stuff resides in here..
			struct Helpers;
		};


		/// <summary>
		/// Bitwise inverse of UIClickArea::ClickAreaFlags
		/// </summary>
		/// <param name="f"> Flags </param>
		/// <returns> ~f </returns>
		inline constexpr UIClickArea::ClickAreaFlags operator~(UIClickArea::ClickAreaFlags f) {
			return static_cast<UIClickArea::ClickAreaFlags>(~static_cast<std::underlying_type_t<UIClickArea::ClickAreaFlags>>(f));
		}

		/// <summary>
		/// Logical 'or' between two UIClickArea::ClickAreaFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a | b </returns>
		inline constexpr UIClickArea::ClickAreaFlags operator|(UIClickArea::ClickAreaFlags a, UIClickArea::ClickAreaFlags b) {
			return static_cast<UIClickArea::ClickAreaFlags>(
				static_cast<std::underlying_type_t<UIClickArea::ClickAreaFlags>>(a) | static_cast<std::underlying_type_t<UIClickArea::ClickAreaFlags>>(b));
		}

		/// <summary>
		/// Logical 'or' between two UIClickArea::ClickAreaFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a </returns>
		inline static UIClickArea::ClickAreaFlags& operator|=(UIClickArea::ClickAreaFlags& a, UIClickArea::ClickAreaFlags b) { return a = (a | b); }
		
		/// <summary>
		/// Logical 'and' between two UIClickArea::ClickAreaFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a & b </returns>
		inline constexpr UIClickArea::ClickAreaFlags operator&(UIClickArea::ClickAreaFlags a, UIClickArea::ClickAreaFlags b) {
			return static_cast<UIClickArea::ClickAreaFlags>(
				static_cast<std::underlying_type_t<UIClickArea::ClickAreaFlags>>(a) & static_cast<std::underlying_type_t<UIClickArea::ClickAreaFlags>>(b));
		}

		/// <summary>
		/// Logical 'and' between two UIClickArea::ClickAreaFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a </returns>
		inline static UIClickArea::ClickAreaFlags& operator&=(UIClickArea::ClickAreaFlags& a, UIClickArea::ClickAreaFlags b) { return a = (a & b); }

		/// <summary>
		/// Bitwise inverse of UIClickArea::StateFlags
		/// </summary>
		/// <param name="f"> Flags </param>
		/// <returns> ~f </returns>
		inline constexpr UIClickArea::StateFlags operator~(UIClickArea::StateFlags f) { 
			return static_cast<UIClickArea::StateFlags>(~static_cast<std::underlying_type_t<UIClickArea::StateFlags>>(f));
		}

		/// <summary>
		/// Logical 'or' between two UIClickArea::StateFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a | b </returns>
		inline constexpr UIClickArea::StateFlags operator|(UIClickArea::StateFlags a, UIClickArea::StateFlags b) {
			return static_cast<UIClickArea::StateFlags>(
				static_cast<std::underlying_type_t<UIClickArea::StateFlags>>(a) | static_cast<std::underlying_type_t<UIClickArea::StateFlags>>(b));
		}

		/// <summary>
		/// Logical 'or' between two UIClickArea::StateFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a </returns>
		inline static UIClickArea::StateFlags& operator|=(UIClickArea::StateFlags& a, UIClickArea::StateFlags b) { return a = (a | b); }

		/// <summary>
		/// Logical 'and' between two UIClickArea::StateFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a & b </returns>
		inline constexpr UIClickArea::StateFlags operator&(UIClickArea::StateFlags a, UIClickArea::StateFlags b) {
			return static_cast<UIClickArea::StateFlags>(
				static_cast<std::underlying_type_t<UIClickArea::StateFlags>>(a) & static_cast<std::underlying_type_t<UIClickArea::StateFlags>>(b));
		}

		/// <summary>
		/// Logical 'and' between two UIClickArea::StateFlags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a </returns>
		inline static UIClickArea::StateFlags& operator&=(UIClickArea::StateFlags& a, UIClickArea::StateFlags b) { return a = (a & b); }
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIClickArea>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIClickArea>(const Callback<const Object*>& report);
}
