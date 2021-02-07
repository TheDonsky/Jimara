#pragma once
#include "../Core/Object.h"
#include "../Core/Event.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Graphics settings
		/// </summary>
		class GraphicsSettings : public virtual Object {
		public:
			//virtual MSAA Multisampling()const = 0;


			inline Event<GraphicsSettings*>& OnChanged()const { return m_onChanged; }

		protected:
			inline void NotifyChange() { m_onChanged(this); }

		private:
			mutable EventInstance<GraphicsSettings*> m_onChanged;
		};
	}
}
