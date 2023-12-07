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
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Image name </param>
			UIClickArea(Component* parent, const std::string_view& name = "UIClickArea");

			/// <summary> Virtual destructor </summary>
			virtual ~UIClickArea();


			static Reference<UIClickArea> FocusedArea(SceneContext* context);


		protected:
			/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
			virtual void OnComponentEnabled()override;

			/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
			virtual void OnComponentDisabled()override;

		private:
			// Underlying updater
			Reference<Object> m_updater;

			// Private stuff resides in here..
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIClickArea>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIClickArea>(const Callback<const Object*>& report);
}
