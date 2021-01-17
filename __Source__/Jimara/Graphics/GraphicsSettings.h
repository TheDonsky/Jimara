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
			/// <summary> Multi sample anti aliazing option </summary>
			enum class MSAA : uint8_t {
				SAMPLE_COUNT_1 = 1,
				SAMPLE_COUNT_2 = 2,
				SAMPLE_COUNT_4 = 4,
				SAMPLE_COUNT_8 = 8,
				SAMPLE_COUNT_16 = 16,
				SAMPLE_COUNT_32 = 32,
				SAMPLE_COUNT_64 = 64,
				MAX_AVAILABLE = (uint8_t)~((uint8_t)0)
			};

			virtual MSAA Multisampling()const = 0;


			inline Event<GraphicsSettings*>& OnChanged()const { return m_onChanged; }

		protected:
			inline void NotifyChange() { m_onChanged(this); }

		private:
			mutable EventInstance<GraphicsSettings*> m_onChanged;
		};
	}
}
