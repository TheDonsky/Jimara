#pragma once
#include "../ItemSerializers.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling the editor not to create a dropdown for a serializer list
		/// </summary>
		class JIMARA_API InlineSerializerListAttribute final : public virtual Object{
		public:
			/// <summary> A condition that inspects the serialized object and tells if the InlineSerializerListAttribute has to work or not </summary>
			using CheckFn = Function<bool, Serialization::SerializedObject>;

			/// <summary> Always returns true; default check function for InlineSerializerListAttribute objects </summary>
			inline static bool DoNotCheck(Serialization::SerializedObject) { return true; }

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="checkFn"> A condition that inspects the serialized object and tells if the InlineSerializerListAttribute has to work or not </param>
			inline InlineSerializerListAttribute(const CheckFn& checkFn = CheckFn(InlineSerializerListAttribute::DoNotCheck)) : m_check(checkFn) {}

			/// <summary> Singleton instance (not necessary to use this, but it's kinda useful) </summary>
			inline static Reference<const InlineSerializerListAttribute> Instance() {
				static const InlineSerializerListAttribute instance;
				return &instance;
			}

			/// <summary>
			/// Checks condition using the check-function, passed through the constructor
			/// </summary>
			/// <param name="serializedObject"> Target serialized object </param>
			/// <returns> True, if the InlineSerializerListAttribute has to work </returns>
			inline bool Check(const Serialization::SerializedObject& serializedObject)const { return m_check(serializedObject); }

		private:
			const CheckFn m_check;
		};
	}
}
