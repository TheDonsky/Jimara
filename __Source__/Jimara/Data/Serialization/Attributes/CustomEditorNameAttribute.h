#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Attribute, telling the editor to show a custom name on UI insetad of the regular TargetName of the serializer
		/// </summary>
		class JIMARA_API CustomEditorNameAttribute : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="customName"> Custom name to show </param>
			inline CustomEditorNameAttribute(const std::string_view& customName) : m_customName(customName) {}

			/// <summary> Custom name to show on Editor UI </summary>
			inline const std::string& CustomName()const { return m_customName; }

		private:
			// Custom name to show
			const std::string m_customName;
		};
	}
}
